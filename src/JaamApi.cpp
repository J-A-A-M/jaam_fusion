#include <ArduinoJson.h>
#include "JaamApi.h"
#include "JaamConfig.h"
#include "JaamUtils.h"
#include "JaamLogs.h"


extern void servicePin(ServiceLed type);
extern JaamFirmwareUpdate fwUpdate;
extern void requestFirmwareUpdate(const char* firmwareId);

JaamApi::JaamApi() : settings(nullptr), chipId(nullptr), fwVersion(nullptr), wsServer(nullptr), isRunning(false), currentPort(-1),
    usedMemory(0), uptime(0), wifiUptime(0), wifiSignal(0), websocketStatus(false), websocketUptime(0), homeAlertFlags(0), cpuTemp(0.0f), homeDistrictTemp(0),
    climateTemperature(NAN), climateHumidity(NAN), climatePressure(NAN), lightLevel(NAN) {
    fwLatestVersion[0] = '\0';
}

void JaamApi::setSettings(JaamSettings* settings) {
    this->settings = settings;
}

void JaamApi::setDeviceInfo(const char* chipId, const char* fwVersion) {
    this->chipId = chipId;
    this->fwVersion = fwVersion;
}

void JaamApi::setSystemInfo(uint32_t usedMemory, uint32_t uptime, uint32_t wifiUptime, int8_t wifiSignal, bool websocketStatus, uint32_t websocketUptime, float cpuTemp) {
    this->usedMemory = usedMemory;
    this->uptime = uptime;
    this->wifiUptime = wifiUptime;
    this->wifiSignal = wifiSignal;
    this->websocketStatus = websocketStatus;
    this->websocketUptime = websocketUptime;
    this->cpuTemp = cpuTemp;
}

void JaamApi::setHomeAlert(uint16_t flags16) {
    this->homeAlertFlags = flags16;
}

void JaamApi::setHomeDistrictTemp(int temp) {
    this->homeDistrictTemp = temp;
}

void JaamApi::setClimateData(float temperature, float humidity, float pressure) {
    climateTemperature = temperature;
    climateHumidity = humidity;
    climatePressure = pressure;
}

void JaamApi::setLightLevel(float level) {
    lightLevel = level;
}

void JaamApi::reconfigure() {
    if (!settings) {
        LOG.printf("[API] Settings not initialized, skipping reconfigure\n");
        return;
    }
    
    bool shouldBeRunning = settings->getBool(API_ENABLED);
    int newPort = settings->getInt(API_PORT);
    
    // Перевіряємо чи змінився порт
    if (isRunning && currentPort != newPort) {
        LOG.printf("[API] Port changed from %d to %d, restarting server\n", currentPort, newPort);
        stop();
    }
    
    if (shouldBeRunning) {
        start();
    } else {
        stop();
    }
}

void JaamApi::start() {
    if (!settings) {
        LOG.printf("[API] Cannot start: settings is null\n");
        return;
    }
    if (!isRunning) {
        // Створюємо новий WebSocket сервер
        wsServer = new WebsocketsServer();
        int port = settings->getInt(API_PORT);
        wsServer->listen(port);
        currentPort = port;
        isRunning = true;
        LOG.printf("[API] WebSocket server started on port %d\n", port);
    }
}

void JaamApi::stop() {
    if (isRunning) {
        // Закриваємо всіх підключених клієнтів
        for (auto& client : wsClients) {
            if (client.available()) {
                client.close();
            }
        }
        wsClients.clear();
        servicePin(API);
        // Видаляємо WebSocket сервер
        if (wsServer) {
            delete wsServer;
            wsServer = nullptr;
        }
        
        currentPort = -1;
        isRunning = false;
        LOG.printf("[API] WebSocket server stopped\n");
    }
}

bool JaamApi::isApiRunning() const {
    return isRunning;
}

void JaamApi::sendInitialState(WebsocketsClient& client) {
    if (!settings) {
        // Settings not initialized, only send fields not dependent on settings
        JsonDocument doc;
        doc["type"] = "initial_state";
        doc["connected"] = true;
        if (chipId) doc["chip_id"] = chipId;
        if (fwVersion) doc["fw_version"] = fwVersion;
        if (fwLatestVersion[0] != '\0') doc["fw_latest"] = fwLatestVersion;
        doc["home_alert_flags"] = homeAlertFlags;
        doc["home_district_temp"] = homeDistrictTemp;
        doc["used_memory"] = usedMemory;
        doc["uptime"] = uptime;
        doc["wifi_uptime"] = wifiUptime;
        doc["wifi_signal"] = wifiSignal;
        doc["websocket_status"] = websocketStatus;
        doc["websocket_uptime"] = websocketUptime;
        doc["cpu_temp"] = cpuTemp;
        String initialData;
        serializeJson(doc, initialData);
        LOG.printf("[API] Sending initial state (no settings): %s\n", initialData.c_str());
        client.send(initialData);
        return;
    }

    JsonDocument doc;
    doc["type"] = "initial_state";
    doc["connected"] = true;

    // Інформація про пристрій
    if (chipId) doc["chip_id"] = chipId;
    if (fwVersion) doc["fw_version"] = fwVersion;
    if (fwLatestVersion[0] != '\0') doc["fw_latest"] = fwLatestVersion;

    // Назва пристрою
    doc["device_name"] = settings->getString(DEVICE_NAME);
    
    // Режим мапи
    doc["map_mode_id"] = settings->getInt(MAP_MODE);
    
    // Режим дисплею
    doc["display_mode_id"] = settings->getInt(DISPLAY_MODE);
    
    // Домашній регіон
    doc["home_region"] = settings->getInt(HOME_DISTRICT);
    
    // Стан тривоги в домашньому регіоні
    doc["home_alert_flags"] = homeAlertFlags;

    // Температура в домашньому регіоні
    doc["home_district_temp"] = homeDistrictTemp;

    // Системна інформація
    doc["used_memory"] = usedMemory;
    doc["uptime"] = uptime;
    doc["wifi_uptime"] = wifiUptime;
    doc["wifi_signal"] = wifiSignal;
    doc["websocket_status"] = websocketStatus;
    doc["websocket_uptime"] = websocketUptime;
    doc["cpu_temp"] = cpuTemp;

    // Температура, вологість і тиск з кліматичного датчика, якщо є
    if (!isnan(climateTemperature)) {
        doc["climate_temp"] = climateTemperature;
    }
    if (!isnan(climateHumidity)) {
        doc["climate_humidity"] = climateHumidity;
    }
    if (!isnan(climatePressure)) {
        doc["climate_pressure"] = climatePressure;
    }

    // Рівень освітлення з датчика світла, якщо є
    if (!isnan(lightLevel)) {
        doc["light_level"] = lightLevel;
    }

    // Налаштування лампи
    JsonObject lamp = doc["lamp"].to<JsonObject>();
    lamp["color"] = settings->getString(COLOR_LAMP);
    lamp["brightness"] = settings->getInt(BRIGHTNESS_LAMP);
    
    String initialData;
    serializeJson(doc, initialData);

    LOG.printf("[API] Sending initial state: %s\n", initialData.c_str());
    client.send(initialData);
}

void JaamApi::handleWebSocketMessage(WebsocketsClient& client, WebsocketsMessage message) {
    LOG.printf("[API] WebSocket message received: %s\n", message.data().c_str());

    if (!settings) {
        LOG.printf("[API] Cannot process WebSocket message: settings is null\n");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message.data());

    if (error) {
        LOG.printf("[API] Failed to parse JSON: %s\n", error.c_str());
        return;
    }

    // Перевіряємо тип команди
    if (doc["type"].isNull()) {
        LOG.printf("[API] Missing 'type' field\n");
        return;
    }

    String type = doc["type"].as<String>();

    // Обробляємо команду set_map_mode
    if (type == "set_map_mode") {
        if (!doc["mode_id"].isNull()) {
            int newMode = doc["mode_id"].as<int>();

            if (newMode >= 0 && newMode <= 4) {
                saveMapMode(newMode);
                LOG.printf("[API] Map mode changed to: %d\n", newMode);
            } else {
                LOG.printf("[API] Invalid map mode: %d\n", newMode);
            }
        }
    }
    // Обробляємо команду set_display_mode
    else if (type == "set_display_mode") {
        if (!doc["mode_id"].isNull()) {
            int newMode = doc["mode_id"].as<int>();

            if (newMode >= 0 && newMode <= 9) {
                saveDisplayMode(newMode);
                LOG.printf("[API] Display mode changed to: %d\n", newMode);
            } else {
                LOG.printf("[API] Invalid display mode: %d\n", newMode);
            }
        }
    }
    // Обробляємо команду set_lamp
    else if (type == "set_lamp") {
        if (!doc["color"].isNull()) {
            String color = doc["color"].as<String>();
            if (color.length() == 7 && color[0] == '#') {
                settings->saveString(COLOR_LAMP, color.c_str());
                LOG.printf("[API] Lamp color changed to: %s\n", color.c_str());
            }
        }

        if (!doc["brightness"].isNull()) {
            int brightness = doc["brightness"].as<int>();
            if (brightness >= 0 && brightness <= 100) {
                settings->saveInt(BRIGHTNESS_LAMP, brightness);
                LOG.printf("[API] Lamp brightness changed to: %d\n", brightness);
            }
        }
    }
    // Обробляємо команду set_home_region
    else if (type == "set_home_region") {
        if (!doc["region_id"].isNull()) {
            int regionId = doc["region_id"].as<int>();
            // Валідація: перевіряємо чи region_id існує в DISTRICTS
            bool validRegion = false;
            for (int i = 0; i < MAX_REGIONS; i++) {
                if (DISTRICTS[i].id == regionId) {
                    validRegion = true;
                    break;
                }
            }
            if (validRegion) {
                settings->saveInt(HOME_DISTRICT, regionId);
                LOG.printf("[API] Home region changed to: %d\n", regionId);
            } else {
                LOG.printf("[API] Invalid region id: %d\n", regionId);
            }
        }
    }
    // Обробляємо запит на оновлення прошивки
    else if (type == "update_firmware") {
        if (!doc["version"].isNull()) {
            String version = doc["version"].as<String>();
            LOG.printf("[API] Firmware update requested, version: %s\n", version.c_str());
            requestFirmwareUpdate(version.c_str());
        }
    }
    else {
        LOG.printf("[API] Unknown command type: %s\n", type.c_str());
    }
}

void JaamApi::handleWebSocketClients() {
    if (!isRunning || !wsServer) {
        return; // API вимкнено, не обробляємо клієнтів
    }
    
    // Перевіряємо нові підключення
    if (wsServer->poll()) {
        WebsocketsClient client = wsServer->accept();
        if (client.available()) {
            LOG.printf("[API] New WebSocket client connected\n");
            
            // Встановлюємо callback для обробки повідомлень від клієнта
            client.onMessage([this](WebsocketsClient& c, WebsocketsMessage msg) {
                this->handleWebSocketMessage(c, msg);
            });
            
            // Відправляємо initial state при підключенні
            sendInitialState(client);
            
            // Зберігаємо клієнта в список
            wsClients.push_back(client);
            servicePin(API);
            LOG.printf("[API] Client added, total clients: %d\n", wsClients.size());
        }
    }
    
    // Poll всіх клієнтів для обробки повідомлень
    for (auto it = wsClients.begin(); it != wsClients.end(); ) {
        if (it->available()) {
            it->poll();
            // Check again after polling in case connection was closed
            if (!it->available()) {
                LOG.printf("[API] Client disconnected during poll, removing from list\n");
                it = wsClients.erase(it);
                servicePin(API);
            } else {
                ++it;
            }
        } else {
            // Client was already unavailable before polling
            LOG.printf("[API] Removing unavailable client from list\n");
            it = wsClients.erase(it);
            servicePin(API);
        }
    }
}

int JaamApi::getClientsCount() const {
    return wsClients.size();
}

void JaamApi::broadcastWebSocket(const String& jsonMessage) {
    // Відправляємо всім підключеним клієнтам
    for (auto& client : wsClients) {
        if (client.available()) {
            client.send(jsonMessage);
        }
    }
    
    // Видаляємо відключених клієнтів
    wsClients.erase(
        std::remove_if(wsClients.begin(), wsClients.end(),
            [](WebsocketsClient& client) {
                return !client.available();
            }),
        wsClients.end()
    );
}

// --- Broadcast методи ---

void JaamApi::broadcastMapModeChange(int newMode) {
    JsonDocument doc;
    doc["type"] = "map_mode_change";
    doc["map_mode_id"] = newMode;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast map mode change: %d\n", newMode);
}

void JaamApi::broadcastDisplayModeChange(int newMode) {
    JsonDocument doc;
    doc["type"] = "display_mode_change";
    doc["display_mode_id"] = newMode;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast display mode change: %d\n", newMode);
}

void JaamApi::broadcastLampChange(const char* color, int brightness) {
    JsonDocument doc;
    doc["type"] = "lamp_change";
    JsonObject lamp = doc["lamp"].to<JsonObject>();
    if (color) {
        lamp["color"] = color;
    }
    lamp["brightness"] = brightness;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast lamp change: %s, %d\n", color ? color : "(null)", brightness);
}

void JaamApi::broadcastHomeRegionChange(int regionId) {
    JsonDocument doc;
    doc["type"] = "home_region_change";
    doc["home_region"] = regionId;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast home region change: %d\n", regionId);
}

void JaamApi::broadcastAlertChange(int regionId, int alertType) {
    JsonDocument doc;
    doc["type"] = "alert_change";
    doc["region_id"] = regionId;
    doc["alert_type"] = alertType;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast alert change: region %d, type %d\n", regionId, alertType);
}

void JaamApi::broadcastHomeAlertChange(uint16_t flags16) {
    JsonDocument doc;
    doc["type"] = "home_alert_change";
    doc["home_alert_flags"] = flags16;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast home alert change: 0x%04X\n", flags16);
}

void JaamApi::broadcastDeviceNameChange(const char* deviceName) {
    const char* deviceNameSafe = deviceName ? deviceName : "(null)";
    
    JsonDocument doc;
    doc["type"] = "device_name_change";
    doc["device_name"] = deviceNameSafe;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast device name change: %s\n", deviceNameSafe);
}

void JaamApi::broadcastHomeDistrictTempChange(int temp) {
    JsonDocument doc;
    doc["type"] = "home_district_temp_change";
    doc["home_district_temp"] = temp;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast home district temp change: %d\n", temp);
}

void JaamApi::updateSystemInfo(uint32_t usedMemory, uint32_t uptime, uint32_t wifiUptime, int8_t wifiSignal, bool websocketStatus, uint32_t websocketUptime, float cpuTemp) {
    this->usedMemory = usedMemory;
    this->uptime = uptime;
    this->wifiUptime = wifiUptime;
    this->wifiSignal = wifiSignal;
    this->websocketStatus = websocketStatus;
    this->websocketUptime = websocketUptime;
    this->cpuTemp = cpuTemp;
    if (isRunning) {
        broadcastSystemInfo();
    }
}

void JaamApi::updateHomeAlert(uint16_t flags16) {
    this->homeAlertFlags = flags16;
    if (isRunning) {
        broadcastHomeAlertChange(flags16);
    }
}

void JaamApi::updateHomeDistrictTemp(int temp) {
    this->homeDistrictTemp = temp;
    if (isRunning) {
        broadcastHomeDistrictTempChange(temp);
    }
}

void JaamApi::updateClimateData(float temp, float humidity, float pressure) {
    // Перевіряємо чи змінилися значення (порівнюємо з поточними)
    bool hasChanges = false;
    
    // Порівнюємо з урахуванням NaN
    if ((isnan(temp) != isnan(climateTemperature)) || (!isnan(temp) && temp != climateTemperature)) {
        hasChanges = true;
    }
    if ((isnan(humidity) != isnan(climateHumidity)) || (!isnan(humidity) && humidity != climateHumidity)) {
        hasChanges = true;
    }
    if ((isnan(pressure) != isnan(climatePressure)) || (!isnan(pressure) && pressure != climatePressure)) {
        hasChanges = true;
    }
    
    // Оновлюємо поточні значення
    this->climateTemperature = temp;
    this->climateHumidity = humidity;
    this->climatePressure = pressure;
    
    // Broadcast тільки якщо є зміни
    if (isRunning && hasChanges) {
        broadcastClimateDataChange(temp, humidity, pressure);
    }
}

void JaamApi::updateLightLevel(float level) {
    // Перевіряємо чи змінилося значення (порівнюємо з поточним)
    bool hasChanged = (isnan(level) != isnan(lightLevel)) || (!isnan(level) && level != lightLevel);
    
    // Оновлюємо поточне значення
    this->lightLevel = level;
    
    // Broadcast тільки якщо є зміни
    if (isRunning && hasChanged) {
        broadcastLightLevelChange(level);
    }
}

void JaamApi::updateNewFirmwareInfo(const char* version) {
    if (!version) {
        LOG.printf("[API] updateNewFirmwareInfo called with null version\n");
        return;
    }
    snprintf(fwLatestVersion, sizeof(fwLatestVersion), "%s", version);
    if (isRunning) {
        broadcastFirmwareUpdate(fwLatestVersion);
    }
}

void JaamApi::updateFirmwareProgress(int progress) {
    if (isRunning) {
        JsonDocument doc;
        doc["type"] = "fw_update_progress";
        doc["progress"] = progress;
        
        String data;
        serializeJson(doc, data);
        broadcastWebSocket(data);
        
        LOG.printf("[API] Broadcast firmware update progress: %d%%\n", progress);
    }
}

void JaamApi::broadcastSystemInfo() {
    JsonDocument doc;
    doc["type"] = "system_info";
    doc["used_memory"] = usedMemory;
    doc["uptime"] = uptime;
    doc["wifi_uptime"] = wifiUptime;
    doc["wifi_signal"] = wifiSignal;
    doc["websocket_status"] = websocketStatus;
    doc["websocket_uptime"] = websocketUptime;
    doc["cpu_temp"] = cpuTemp;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
}

void JaamApi::broadcastClimateDataChange(float temp, float humidity, float pressure) {
    // Якщо всі значення NaN - не відправляємо нічого
    if (isnan(temp) && isnan(humidity) && isnan(pressure)) {
        return;
    }
    
    JsonDocument doc;
    doc["type"] = "climate_data_change";
    
    // Додаємо тільки валідні значення (не NaN)
    if (!isnan(temp)) {
        doc["climate_temp"] = temp;
    }
    if (!isnan(humidity)) {
        doc["climate_humidity"] = humidity;
    }
    if (!isnan(pressure)) {
        doc["climate_pressure"] = pressure;
    }
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast climate data change: temp=%s, humidity=%s, pressure=%s\n", 
               isnan(temp) ? "N/A" : String(temp, 2).c_str(),
               isnan(humidity) ? "N/A" : String(humidity, 2).c_str(),
               isnan(pressure) ? "N/A" : String(pressure, 2).c_str());
}

void JaamApi::broadcastLightLevelChange(float level) {
    if (isnan(level)) {
        return;
    }
    
    JsonDocument doc;
    doc["type"] = "light_level_change";
    doc["light_level"] = level;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast light level change: %s lx\n", String(level, 2).c_str());
}

void JaamApi::broadcastFirmwareUpdate(const char* version) {
    
    JsonDocument doc;
    doc["type"] = "firmware_update";
    doc["fw_latest"] = version;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast firmware update: %s\n", version);
}

// --- Обробка змін налаштувань ---

void JaamApi::onSettingsChange(Type type, int intValue, float fltValue, const char* strValue) {
    // Перевіряємо чи settings ініціалізовані
    if (!settings) {
        return;
    }

    if (type != API_ENABLED && !isRunning) {
        // Якщо API не запущено, немає сенсу обробляти зміни налаштувань для broadcast
        return;
    }
    
    // Відстежуємо зміни критичних налаштувань і broadcast-имо їх
    switch (type) {
        case API_ENABLED:
        case API_PORT:
            // При зміні API_ENABLED або API_PORT перезапускаємо сервер
            reconfigure();
            break;
            
        case MAP_MODE:
            broadcastMapModeChange(intValue);
            break;
            
        case DISPLAY_MODE:
            broadcastDisplayModeChange(intValue);
            break;
            
        case BRIGHTNESS_LAMP:
            // Якщо змінилась яскравість лампи, відправляємо оновлення
            broadcastLampChange(settings->getString(COLOR_LAMP), intValue);
            break;
            
        case COLOR_LAMP:
            // Якщо змінився колір лампи, відправляємо оновлення
            broadcastLampChange(strValue, settings->getInt(BRIGHTNESS_LAMP));
            break;
            
        case HOME_DISTRICT:
            broadcastHomeRegionChange(intValue);
            break;
            
        case DEVICE_NAME:
            // Якщо змінилась назва пристрою, відправляємо оновлення
            if (strValue) {
                broadcastDeviceNameChange(strValue);
            }
            break;
            
        default:
            break;
    }
}
