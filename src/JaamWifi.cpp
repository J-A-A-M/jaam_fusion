#include "JaamWifi.h"
#include "JaamLogs.h"
#include "web_assets.h"
#include <esp_wifi.h>

// --- Lifecycle ---

void JaamWifi::begin(const char* chipId) {
    if (!settings) return;

    LOG.printf("[WIFI] Initializing...\n");

    snprintf(apName, sizeof(apName), "JAAM_%s", chipId);

    WiFi.setHostname(settings->getString(BROADCAST_NAME));
    WiFi.mode(WIFI_STA);

    setupWifiEvents();
    migrateFromWifiManager();
    loadNetworksIntoMulti();

    if (getSavedNetworks().empty()) {
        LOG.printf("[WIFI] No saved networks, opening captive portal\n");
        openCaptivePortal();
        return;
    }

    if (tryMultiConnect(MULTI_CONNECT_TIMEOUT)) {
        handleConnected();
    } else {
        LOG.printf("[WIFI] All networks failed, opening captive portal\n");
        openCaptivePortal();
    }
}

void JaamWifi::process() {
    if (portalActive) {
        dnsServer.processNextRequest();
        portalServer.handleClient();
        if (millis() - portalStartTime >= PORTAL_TIMEOUT) {
            portalStartTime = millis();
            LOG.printf("[WIFI] Captive portal timeout, rebooting...\n");
            if (onRebootCb) onRebootCb(5000);
        }
        return;
    }

    bool currentlyConnected = (WiFi.status() == WL_CONNECTED);

    if (currentlyConnected && connected) return;

    if (currentlyConnected) {
        reconnectAttempts = 0;
        handleConnected();
        return;
    }

    handleDisconnected();

    reconnectAttempts++;
    if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        LOG.printf("[WIFI] Reconnect attempt %d/%d\n", reconnectAttempts, MAX_RECONNECT_ATTEMPTS);
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        delay(100);
        WiFi.mode(WIFI_STA);
        tryMultiConnect(MULTI_RECONNECT_TIMEOUT);
    } else {
        LOG.printf("[WIFI] Max reconnect attempts reached, rebooting...\n");
        if (onRebootCb) onRebootCb(3000);
    }
}

// --- NVS Network Management ---

std::vector<SavedNetwork> JaamWifi::getSavedNetworks() {
    std::vector<SavedNetwork> nets;
    Preferences prefs;
    prefs.begin("wifi_nets", true);
    uint8_t count = prefs.getUChar("count", 0);
    for (uint8_t i = 0; i < count && i < MAX_NETWORKS; i++) {
        String keyS = "ssid" + String(i);
        String keyP = "pass" + String(i);
        String ssid = prefs.getString(keyS.c_str(), "");
        String pass = prefs.getString(keyP.c_str(), "");
        if (ssid.length() > 0) {
            nets.push_back({ssid, pass});
        }
    }
    prefs.end();
    return nets;
}

void JaamWifi::saveNetworksToNvs(const std::vector<SavedNetwork>& nets) {
    Preferences prefs;
    prefs.begin("wifi_nets", false);
    uint8_t count = (uint8_t)min((size_t)MAX_NETWORKS, nets.size());
    // 0xFF = sentinel "migration done, intentionally empty" — prevents re-migration on next boot
    prefs.putUChar("count", count == 0 ? 0xFF : count);
    for (uint8_t i = 0; i < count; i++) {
        String keyS = "ssid" + String(i);
        String keyP = "pass" + String(i);
        prefs.putString(keyS.c_str(), nets[i].ssid);
        prefs.putString(keyP.c_str(), nets[i].pass);
    }
    // Clear leftover slots
    for (uint8_t i = count; i < MAX_NETWORKS; i++) {
        String keyS = "ssid" + String(i);
        String keyP = "pass" + String(i);
        if (prefs.isKey(keyS.c_str())) prefs.remove(keyS.c_str());
        if (prefs.isKey(keyP.c_str())) prefs.remove(keyP.c_str());
    }
    prefs.end();
}

bool JaamWifi::addNetwork(const String& ssid, const String& pass) {
    if (ssid.length() == 0) return false;
    auto nets = getSavedNetworks();
    for (auto& n : nets) {
        if (n.ssid == ssid) {
            // Update password even when list is full
            n.pass = pass;
            saveNetworksToNvs(nets);
            LOG.printf("[WIFI] Updated password for: %s\n", ssid.c_str());
            return true;
        }
    }
    if (nets.size() >= MAX_NETWORKS) {
        LOG.printf("[WIFI] addNetwork: max networks reached\n");
        return false;
    }
    nets.push_back({ssid, pass});
    saveNetworksToNvs(nets);
    LOG.printf("[WIFI] Added network: %s\n", ssid.c_str());
    return true;
}

bool JaamWifi::removeNetwork(const String& ssid) {
    auto nets = getSavedNetworks();
    size_t before = nets.size();
    nets.erase(
        std::remove_if(nets.begin(), nets.end(),
            [&ssid](const SavedNetwork& n) { return n.ssid == ssid; }),
        nets.end()
    );
    if (nets.size() == before) {
        LOG.printf("[WIFI] removeNetwork: not found: %s\n", ssid.c_str());
        return false;
    }
    saveNetworksToNvs(nets);
    LOG.printf("[WIFI] Removed network: %s\n", ssid.c_str());
    return true;
}

// --- Scan API ---

void JaamWifi::startScan() {
    WiFi.scanNetworks(true); // async
    LOG.printf("[WIFI] Scan started\n");
}

bool JaamWifi::isScanDone() {
    return WiFi.scanComplete() >= 0;
}

int JaamWifi::getScanCount() {
    int n = WiFi.scanComplete();
    return n > 0 ? n : 0;
}

String JaamWifi::getScanSSID(int i) {
    return WiFi.SSID(i);
}

int8_t JaamWifi::getScanRSSI(int i) {
    return WiFi.RSSI(i);
}

bool JaamWifi::isScanOpen(int i) {
    return WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
}

// --- Private helpers ---

static String escapeJson(const String& s) {
    String out;
    out.reserve(s.length() + 4);
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else                out += c;
    }
    return out;
}

static String escapeHtml(const String& s) {
    String out;
    out.reserve(s.length() + 4);
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if      (c == '&')  out += "&amp;";
        else if (c == '<')  out += "&lt;";
        else if (c == '>')  out += "&gt;";
        else if (c == '"')  out += "&quot;";
        else if (c == '\'') out += "&#39;";
        else                out += c;
    }
    return out;
}

// --- Private ---

bool JaamWifi::tryMultiConnect(uint32_t timeout) {
    if (wifiMulti.run(timeout) == WL_CONNECTED) {
        LOG.printf("[WIFI] Connected to %s, IP: %s\n",
            WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        return true;
    }
    LOG.printf("[WIFI] WiFiMulti failed to connect\n");
    return false;
}

void JaamWifi::loadNetworksIntoMulti() {
    auto nets = getSavedNetworks();
    if (nets.empty()) {
        LOG.printf("[WIFI] No saved networks found\n");
        return;
    }
    for (auto& n : nets) {
        LOG.printf("[WIFI] Loading network: %s\n", n.ssid.c_str());
        wifiMulti.addAP(n.ssid.c_str(), n.pass.c_str());
    }
}

void JaamWifi::migrateFromWifiManager() {
    // Guard: skip only if we already have saved networks (count > 0)
    Preferences nvsCheck;
    nvsCheck.begin("wifi_nets", true);
    uint8_t existingCount = nvsCheck.getUChar("count", 0);
    nvsCheck.end();
    if (existingCount > 0) return;

    // Try 1: ESP32 WiFi NVS (what WiFi.begin() stores via esp_wifi_set_config)
    wifi_config_t conf;
    memset(&conf, 0, sizeof(conf));
    String ssid, pass;
    if (esp_wifi_get_config(WIFI_IF_STA, &conf) == ESP_OK) {
        ssid = String(reinterpret_cast<char*>(conf.sta.ssid));
        pass = String(reinterpret_cast<char*>(conf.sta.password));
    }
    LOG.printf("[WIFI] Migration try1 (WiFi NVS): ssid='%s'\n", ssid.c_str());

    // Try 2: WiFiManager Preferences namespace "wm" (fallback)
    if (ssid.length() == 0) {
        Preferences wmPrefs;
        wmPrefs.begin("wm", true);
        ssid = wmPrefs.getString("ssid", "");
        pass = wmPrefs.getString("pass", "");
        wmPrefs.end();
        LOG.printf("[WIFI] Migration try2 (wm prefs): ssid='%s'\n", ssid.c_str());
    }

    if (ssid.length() > 0) {
        addNetwork(ssid, pass);
        // Erase system WiFi credentials so migration won't re-read them on next boot
        WiFi.disconnect(false, true);
        LOG.printf("[WIFI] Migrated WiFi credentials for: %s\n", ssid.c_str());
        return;
    }

    // No prior credentials — initialize with empty list (write count=0)
    Preferences initPrefs;
    initPrefs.begin("wifi_nets", false);
    initPrefs.putUChar("count", 0);
    initPrefs.end();
    LOG.printf("[WIFI] No prior WiFi credentials found, starting fresh\n");
}

void JaamWifi::openCaptivePortal() {
    LOG.printf("[WIFI] Starting captive portal: %s\n", apName);

    WiFi.mode(WIFI_AP_STA);  // AP_STA дозволяє сканування в AP-режимі
    WiFi.softAP(apName);
    WiFi.SSID(); // щоб ініціалізувати WiFi і дозволити сканування

    IPAddress apIp = WiFi.softAPIP();
    LOG.printf("[WIFI] AP IP: %s\n", apIp.toString().c_str());

    dnsServer.start(53, "*", apIp);

    portalServer.on("/", HTTP_GET, [this]() {
        portalServer.sendHeader("Content-Encoding", "gzip");
        portalServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        portalServer.setContentLength(captive_portal_html_gz_len);
        portalServer.send_P(200, "text/html", (const char*)captive_portal_html_gz, captive_portal_html_gz_len);
    });

    portalServer.on("/styles.css", HTTP_GET, [this]() {
        portalServer.sendHeader("Content-Encoding", "gzip");
        portalServer.sendHeader("Cache-Control", "public, max-age=3600");
        portalServer.setContentLength(styles_css_gz_len);
        portalServer.send_P(200, "text/css", (const char*)styles_css_gz, styles_css_gz_len);
    });

    portalServer.on("/scan", HTTP_POST, [this]() {
        WiFi.scanNetworks(true);
        portalServer.send(200, "application/json", "{\"status\":\"scanning\"}");
    });

    portalServer.on("/scan-results", HTTP_GET, [this]() {
        int n = WiFi.scanComplete();
        if (n < 0) {
            portalServer.send(200, "application/json", "{\"status\":\"scanning\"}");
            return;
        }
        String json = "{\"networks\":[";
        for (int i = 0; i < n; i++) {
            if (i > 0) json += ",";
            json += "{\"ssid\":\"" + escapeJson(WiFi.SSID(i)) + "\""
                  + ",\"rssi\":" + String(WiFi.RSSI(i))
                  + ",\"open\":" + (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true" : "false")
                  + "}";
        }
        json += "]}";
        portalServer.send(200, "application/json", json);
    });

    portalServer.on("/save", HTTP_POST, [this]() {
        String ssid = portalServer.arg("ssid");
        String pass = portalServer.arg("pass");

        if (ssid.length() == 0) {
            portalServer.send(400, "text/html", "<h2>Помилка: SSID не може бути порожнім</h2>");
            return;
        }

        if (!addNetwork(ssid, pass)) {
            portalServer.send(409, "text/html",
                "<h2>Помилка: досягнуто максимальну кількість мереж</h2>");
            return;
        }
        LOG.printf("[WIFI] Network saved via portal: %s\n", ssid.c_str());
        if (onNetworkSavedCb) onNetworkSavedCb(ssid);

        String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
            "<meta name='viewport' content='width=device-width,initial-scale=1'>"
            "<style>body{font-family:sans-serif;max-width:400px;margin:40px auto;padding:20px;background:#1e1e2e;color:#cdd6f4;text-align:center}</style>"
            "</head><body><h2>Збережено!</h2><p>Підключення до <b>" + escapeHtml(ssid) + "</b>...</p>"
            "<p>Пристрій перезавантажується...</p></body></html>";
        portalServer.send(200, "text/html", html);

        delay(2000);
        if (onRebootCb) onRebootCb(2000);
    });

    portalServer.onNotFound([this]() {
        portalServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
        portalServer.send(302, "text/plain", "");
    });

    portalServer.begin();
    portalActive = true;
    portalStartTime = millis();

    if (onPortalStartedCb) onPortalStartedCb(String(apName));
}

void JaamWifi::setupWifiEvents() {
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
            String ip = WiFi.softAPIP().toString();
            LOG.printf("[WIFI] Client connected to AP, IP: %s\n", ip.c_str());
            if (onClientConnectedCb) onClientConnectedCb(ip);
        }
    });
}

void JaamWifi::handleConnected() {
    connected = true;
    connectTime = millis();
    if (onConnectedCb) onConnectedCb(WiFi.SSID(), WiFi.localIP().toString());
}

void JaamWifi::handleDisconnected() {
    connected = false;
    if (onDisconnectedCb) onDisconnectedCb();
}
