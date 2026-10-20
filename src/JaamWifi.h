#pragma once

#include <WiFiMulti.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <vector>
#include <functional>
#include "JaamSettings.h"
#include "JaamConfig.h"
#include "JaamWsBackoff.h"

struct SavedNetwork {
    String ssid;
    String pass;
};

struct ScanResult {
    String ssid;
    int8_t rssi;
    bool   open;
};

class JaamWifi {
public:
    using StringCb  = std::function<void(const String&, const String&)>; // (a, b)
    using StrCb     = std::function<void(const String&)>;               // (a)
    using VoidCb    = std::function<void()>;
    using RebootCb  = std::function<void(uint32_t)>;                    // (delayMs)

    // --- Dependency injection ---
    void setSettings(JaamSettings* s) { settings = s; }

    // --- Callbacks ---
    void setOnConnected(StringCb cb)       { onConnectedCb = cb; }       // (ssid, ip)
    void setOnDisconnected(VoidCb cb)      { onDisconnectedCb = cb; }
    void setOnPortalStarted(StrCb cb)    { onPortalStartedCb = cb; }    // (apSsid)
    void setOnClientConnected(StrCb cb)  { onClientConnectedCb = cb; }  // (ip)
    void setOnNetworkSaved(StrCb cb)     { onNetworkSavedCb = cb; }     // (ssid)
    void setOnReboot(RebootCb cb)        { onRebootCb = cb; }

    ~JaamWifi() { delete wifiMulti; }

    // --- Lifecycle ---
    void begin(const char* chipId);  // блокуючий: підключення або відкриття порталу
    void process();                  // перевірка стану, reconnect, обробка portal

    // --- State ---
    bool          isConnected()    const { return connected; }
    bool          isPortalActive() const { return portalActive; }
    String        getSSID()        const { return WiFi.SSID(); }
    int8_t        getRSSI()        const { return connected ? WiFi.RSSI() : 0; }
    unsigned long getConnectTime() const { return connectTime; }

    // --- Network management (used by JaamWeb) ---
    std::vector<SavedNetwork> getSavedNetworks();
    bool   addNetwork(const String& ssid, const String& pass);
    bool   removeNetwork(const String& ssid);
    void   startScan();
    bool   isScanDone();
    // Дедупліковано за SSID (mesh/кілька точок дають запис на кожен BSSID — лишається
    // найсильніший) і відсортовано за сигналом, найсильніші першими. Приховані мережі
    // відкидаються: рядок без назви у списку все одно не вибрати.
    std::vector<ScanResult> getScanResults();

private:
    WiFiMulti*    wifiMulti     = nullptr; // перестворюється в loadNetworksIntoMulti()
    WebServer     portalServer{80};
    DNSServer     dnsServer;
    JaamSettings* settings      = nullptr;

    bool          connected      = false;
    bool          portalActive   = false;
    unsigned long connectTime    = 0;
    unsigned long portalStartTime = 0;
    unsigned long lastPortalRetry = 0;
    uint8_t       reconnectAttempts = 0;

    // Неблокуючий реконект після втрати зв'язку: кожен виклик process() робить один короткий крок.
    enum RcState : uint8_t { RC_IDLE, RC_RADIO_OFF, RC_SETTLING, RC_CONNECTING, RC_WAITING };
    RcState       rcState        = RC_IDLE;
    uint32_t      rcAt           = 0;                      // дедлайн поточного кроку (signed-порівняння з millis())
    uint32_t      rcBackoffMs    = WIFI_RECONNECT_MIN_MS;  // база наступної паузи (backoffStep)
    char          apName[32]     = {};

    static const uint8_t  MAX_NETWORKS            = 5;
    static const uint8_t  MAX_RECONNECT_ATTEMPTS  = 10;     // невдач поспіль перед ребутом
    static const uint8_t  BOOT_CONNECT_ATTEMPTS   = 4;      // спроб при старті перед відкриттям порталу
    static const uint32_t MULTI_CONNECT_TIMEOUT   = 15000;  // ms при першому start
    static const uint32_t WIFI_RECONNECT_MIN_MS   = 2000;   // пауза між невдалими спробами: 2, 4, 8, 16, 30 с ...
    static const uint32_t WIFI_RECONNECT_MAX_MS   = 30000;
    static const uint32_t RECONNECT_ATTEMPT_TIMEOUT = 15000; // ms на одну спробу після втрати зв'язку
    static const uint32_t RADIO_OFF_DELAY         = 100;    // ms між WIFI_OFF і WIFI_STA
    static const uint32_t PORTAL_TIMEOUT          = 180000; // 3 хв
    static const uint32_t PORTAL_RETRY_INTERVAL   = 60000;  // як часто пробувати збережені мережі з порталу
    static const uint32_t PORTAL_RETRY_TIMEOUT    = 10000;  // ms на одну спробу з порталу
    static const uint32_t RADIO_SETTLE_DELAY      = 500;    // ms після esp_wifi_start() перед скануванням

    StringCb onConnectedCb;
    VoidCb   onDisconnectedCb;
    StrCb    onPortalStartedCb;
    StrCb    onClientConnectedCb;
    StrCb    onNetworkSavedCb;
    RebootCb onRebootCb;

    void loadNetworksIntoMulti();
    void migrateFromWifiManager();
    void setupWifiEvents();
    bool tryMultiConnect(uint32_t timeout);
    void resetRadio();
    void startReconnectAttempt();
    void failReconnectAttempt();
    void openCaptivePortal();
    void closeCaptivePortal();
    bool retrySavedNetworksFromPortal();
    void handleConnected();
    void handleDisconnected();
    void saveNetworksToNvs(const std::vector<SavedNetwork>& nets);
};
