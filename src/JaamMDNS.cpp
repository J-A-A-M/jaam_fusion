#include "JaamMDNS.h"
#include "JaamLogs.h"
#include <ESPmDNS.h>
#include <mdns.h>

JaamMDNS::JaamMDNS() : settings(nullptr), chipId(nullptr), fwVersion(nullptr),
    mdnsStarted(false), httpServiceActive(false), wsServiceActive(false), cachedApiPort(-1) {}

void JaamMDNS::setSettings(JaamSettings* settings) {
    this->settings = settings;
}

void JaamMDNS::setDeviceInfo(const char* chipId, const char* fwVersion) {
    this->chipId = chipId;
    this->fwVersion = fwVersion;
}

void JaamMDNS::begin() {
    if (!settings) {
        LOG.printf("[MDNS] Settings not initialized\n");
        return;
    }
    
    startMDNS();
    updateCachedParams();
    
    // Налаштовуємо HTTP сервіс (для веб-інтерфейсу)
    configureHttpService();
    
    // Перевіряємо чи увімкнено API і додаємо WebSocket сервіс якщо потрібно
    bool apiEnabled = settings->getBool(API_ENABLED);
    if (apiEnabled) {
        int apiPort = settings->getInt(API_PORT);
        LOG.printf("[MDNS] API is enabled, adding WebSocket service on port %d\n", apiPort);
        configureWsService(apiPort);
    }
}

void JaamMDNS::startMDNS() {
    if (!settings) {
        return;
    }
    
    const char* hostname = settings->getString(BROADCAST_NAME);
    if (!hostname || strlen(hostname) == 0) {
        LOG.printf("[MDNS] Cannot start: BROADCAST_NAME is empty\n");
        mdnsStarted = false;
        return;
    }
    
    if (!MDNS.begin(hostname)) {
        LOG.printf("[MDNS] Failed to start for hostname: %s\n", hostname);
        mdnsStarted = false;
        return;
    }
    
    mdnsStarted = true;
    LOG.printf("[MDNS] Started successfully: %s.local\n", hostname);
}

void JaamMDNS::restartMDNS() {
    LOG.printf("[MDNS] Restarting mDNS services\n");
    
    // Видаляємо існуючі сервіси
    if (wsServiceActive) {
        removeWsService();
    }
    
    if (httpServiceActive) {
        mdns_service_remove("_http", "_tcp");
        httpServiceActive = false;
        LOG.printf("[MDNS] HTTP service removed for restart\n");
    }
    
    // Зупиняємо MDNS
    if (mdnsStarted) {
        MDNS.end();
        mdnsStarted = false;
    }
    
    // Запускаємо заново
    startMDNS();
    
    // Відновлюємо HTTP сервіс
    if (!httpServiceActive) {
        configureHttpService();
    }
    
    // Відновлюємо WebSocket сервіс тільки якщо API увімкнено
    bool apiEnabled = settings->getBool(API_ENABLED);
    if (apiEnabled) {
        int apiPort = settings->getInt(API_PORT);
        LOG.printf("[MDNS] Restoring WebSocket service on port %d\n", apiPort);
        configureWsService(apiPort);
    }
    
    updateCachedParams();
}

void JaamMDNS::configureHttpService() {
    if (!mdnsStarted) {
        LOG.printf("[MDNS] Cannot configure HTTP service: mDNS not started\n");
        return;
    }
    
    if (!settings) {
        LOG.printf("[MDNS] Cannot configure HTTP service: settings not initialized\n");
        return;
    }
    
    // Додаємо HTTP сервіс (якщо вже існує - він буде оновлено)
    if (!MDNS.addService("http", "tcp", 80)) {
        LOG.printf("[MDNS] Failed to add HTTP service\n");
        return;
    }
    
    httpServiceActive = true;
    LOG.printf("[MDNS] HTTP service added on port 80\n");
    
    // Встановлюємо instance name для HTTP сервісу
    const char* deviceName = settings->getString(DEVICE_NAME);
    if (deviceName && strlen(deviceName) > 0) {
        if (mdns_service_instance_name_set("_http", "_tcp", deviceName)) {
            LOG.printf("[MDNS] Failed setting HTTP service instance name\n");
        } else {
            LOG.printf("[MDNS] HTTP service instance name set to: %s\n", deviceName);
        }
    }
    
    // Додаємо TXT записи для HTTP сервісу
    MDNS.addServiceTxt("http", "tcp", "path", "/");
    
    if (chipId) {
        MDNS.addServiceTxt("http", "tcp", "mac", chipId);
    }
    
    MDNS.addServiceTxt("http", "tcp", "manufacturer", "JAAM");
    
    if (fwVersion) {
        MDNS.addServiceTxt("http", "tcp", "version", fwVersion);
    }
    
    if (deviceName && strlen(deviceName) > 0) {
        MDNS.addServiceTxt("http", "tcp", "name", deviceName);
    }
    
    LOG.printf("[MDNS] HTTP TXT records added\n");
}

void JaamMDNS::configureWsService(int port) {
    if (!mdnsStarted) {
        LOG.printf("[MDNS] Cannot configure WS service: mDNS not started\n");
        return;
    }
    
    if (!settings) {
        LOG.printf("[MDNS] Cannot configure WS service: settings not initialized\n");
        return;
    }
    
    // Видаляємо старий сервіс якщо він існує
    if (wsServiceActive) {
        removeWsService();
    }
    
    // Додаємо WebSocket сервіс для Home Assistant
    if (!MDNS.addService("jaam-ws", "tcp", port)) {
        LOG.printf("[MDNS] Failed to add jaam-ws service\n");
        return;
    }
    
    wsServiceActive = true;
    LOG.printf("[MDNS] jaam-ws service added on port %d\n", port);
    
    // Встановлюємо instance name для jaam-ws сервісу
    const char* deviceName = settings->getString(DEVICE_NAME);
    if (deviceName && strlen(deviceName) > 0) {
        if (mdns_service_instance_name_set("_jaam-ws", "_tcp", deviceName)) {
            LOG.printf("[MDNS] Failed setting jaam-ws service instance name\n");
        } else {
            LOG.printf("[MDNS] jaam-ws service instance name set to: %s\n", deviceName);
        }
    }
    
    // Додаємо TXT metadata
    if (chipId) {
        MDNS.addServiceTxt("jaam-ws", "tcp", "chipId", chipId);
    }
    if (fwVersion) {
        MDNS.addServiceTxt("jaam-ws", "tcp", "version", fwVersion);
    }
    if (deviceName && strlen(deviceName) > 0) {
        MDNS.addServiceTxt("jaam-ws", "tcp", "deviceName", deviceName);
    }
    LOG.printf("[MDNS] jaam-ws TXT records added\n");
}

void JaamMDNS::removeWsService() {
    if (!wsServiceActive) {
        return;
    }
    
    if (mdns_service_remove("_jaam-ws", "_tcp")) {
        LOG.printf("[MDNS] Failed removing jaam-ws service\n");
    } else {
        LOG.printf("[MDNS] jaam-ws service removed\n");
    }
    
    wsServiceActive = false;
}

void JaamMDNS::updateCachedParams() {
    if (!settings) {
        return;
    }
    
    const char* hostname = settings->getString(BROADCAST_NAME);
    const char* deviceName = settings->getString(DEVICE_NAME);
    int apiPort = settings->getInt(API_PORT);
    
    cachedHost = hostname ? String(hostname) : "";
    cachedDeviceName = deviceName ? String(deviceName) : "";
    cachedApiPort = apiPort;
}

void JaamMDNS::updateInstanceNames() {
    if (!mdnsStarted || !settings) {
        return;
    }
    
    const char* deviceName = settings->getString(DEVICE_NAME);
    if (!deviceName || strlen(deviceName) == 0) {
        return;
    }
    
    // Оновлюємо instance name для HTTP сервісу
    if (httpServiceActive) {
        if (mdns_service_instance_name_set("_http", "_tcp", deviceName)) {
            LOG.printf("[MDNS] Failed updating HTTP service instance name\n");
        } else {
            LOG.printf("[MDNS] HTTP service instance name updated to: %s\n", deviceName);
        }
    }
    
    // Оновлюємо instance name для WebSocket сервісу
    if (wsServiceActive) {
        if (mdns_service_instance_name_set("_jaam-ws", "_tcp", deviceName)) {
            LOG.printf("[MDNS] Failed updating jaam-ws service instance name\n");
        } else {
            LOG.printf("[MDNS] jaam-ws service instance name updated to: %s\n", deviceName);
        }
    }
}

void JaamMDNS::reconfigureWsService() {
    if (!mdnsStarted || !settings) {
        return;
    }
    
    // Перевіряємо чи API увімкнено
    bool apiEnabled = settings->getBool(API_ENABLED);
    
    // Видаляємо старий WebSocket сервіс якщо він є
    if (wsServiceActive) {
        removeWsService();
    }
    
    // Додаємо WebSocket сервіс з новим портом тільки якщо API увімкнено
    if (apiEnabled) {
        int newPort = settings->getInt(API_PORT);
        configureWsService(newPort);
        LOG.printf("[MDNS] WebSocket service reconfigured on new port %d\n", newPort);
    } else {
        LOG.printf("[MDNS] API disabled, WebSocket service not added\n");
    }
}

void JaamMDNS::onSettingsChange(Type type, int intValue, float fltValue, const char* strValue) {
    if (!settings) {
        return;
    }
    
    // Відстежуємо зміни критичних налаштувань для mDNS
    switch (type) {
        case BROADCAST_NAME: {
            // Зміна hostname потребує повного перезапуску mDNS
            const char* hostname = settings->getString(BROADCAST_NAME);
            if (hostname && cachedHost != String(hostname)) {
                LOG.printf("[MDNS] Hostname changed, full restart required\n");
                restartMDNS();
                updateCachedParams();
            }
            break;
        }
        
        case DEVICE_NAME: {
            // Зміна device name - оновлюємо instance names для існуючих сервісів
            const char* deviceName = settings->getString(DEVICE_NAME);
            if (deviceName && cachedDeviceName != String(deviceName)) {
                LOG.printf("[MDNS] Device name changed, updating instance names\n");
                updateInstanceNames();
                cachedDeviceName = String(deviceName);
            }
            break;
        }
        
        case API_PORT: {
            // Зміна API порту - перезапускаємо тільки WebSocket сервіс
            int apiPort = settings->getInt(API_PORT);
            if (cachedApiPort != apiPort) {
                LOG.printf("[MDNS] API port changed from %d to %d, reconfiguring WS service\n", cachedApiPort, apiPort);
                reconfigureWsService();
                cachedApiPort = apiPort;
            }
            break;
        }
        
        case API_ENABLED: {
            // Відстежуємо увімкнення/вимкнення API для додавання/видалення WebSocket сервісу
            bool apiEnabled = settings->getBool(API_ENABLED);
            if (apiEnabled && !wsServiceActive) {
                // API увімкнено, але WS сервіс не активний - додаємо його
                int apiPort = settings->getInt(API_PORT);
                LOG.printf("[MDNS] API enabled, adding WebSocket service on port %d\n", apiPort);
                configureWsService(apiPort);
            } else if (!apiEnabled && wsServiceActive) {
                // API вимкнено, але WS сервіс активний - видаляємо його
                LOG.printf("[MDNS] API disabled, removing WebSocket service\n");
                removeWsService();
            }
            break;
        }
        
        default:
            break;
    }
}
