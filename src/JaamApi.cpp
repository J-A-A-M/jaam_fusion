#include "JaamApi.h"
#include "JaamConfig.h"
#include "JaamUtils.h"
#include "JaamLogs.h"
#include <ArduinoJson.h>

extern volatile bool needAdaptColors;

JaamApi::JaamApi() : settings(nullptr), chipId(nullptr), fwVersion(nullptr), wsServer(nullptr), isRunning(false) {}

void JaamApi::setSettings(JaamSettings* settings) {
    this->settings = settings;
}

void JaamApi::setDeviceInfo(const char* chipId, const char* fwVersion) {
    this->chipId = chipId;
    this->fwVersion = fwVersion;
}



void JaamApi::start() {
    if (!isRunning) {
        // Створюємо новий WebSocket сервер
        wsServer = new WebsocketsServer();
        wsServer->listen(81);
        isRunning = true;
        LOG.printf("[API] WebSocket server started on port 81\n");
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
        
        // Видаляємо WebSocket сервер
        if (wsServer) {
            delete wsServer;
            wsServer = nullptr;
        }
        
        isRunning = false;
        LOG.printf("[API] WebSocket server stopped\n");
    }
}

bool JaamApi::isApiRunning() const {
    return isRunning;
}

String JaamApi::getMapModeName(int mode) {
    switch (mode) {
        case MapModes::OFF: return "off";
        case MapModes::ALERT: return "alert";
        case MapModes::WEATHER: return "weather";
        case MapModes::FLAG: return "flag";
        case MapModes::LAMP: return "lamp";
        default: return "unknown";
    }
}

void JaamApi::sendInitialState(WebsocketsClient& client) {
    JsonDocument doc;
    doc["type"] = "initial_state";
    doc["connected"] = true;
    
    // Інформація про пристрій
    if (chipId) doc["chip_id"] = chipId;
    if (fwVersion) doc["fw_version"] = fwVersion;
    
    // Режим мапи
    int currentMode = settings->getInt(MAP_MODE);
    doc["map_mode"] = getMapModeName(currentMode);
    doc["map_mode_id"] = currentMode;
    
    // Домашній регіон
    doc["home_region"] = settings->getInt(HOME_DISTRICT);
    
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
        int newMode = -1;
        
        if (!doc["mode_id"].isNull()) {
            newMode = doc["mode_id"].as<int>();
        } else if (!doc["mode"].isNull()) {
            String modeName = doc["mode"].as<String>();
            modeName.toLowerCase();
            
            if (modeName == "off") newMode = MapModes::OFF;
            else if (modeName == "alert") newMode = MapModes::ALERT;
            else if (modeName == "weather") newMode = MapModes::WEATHER;
            else if (modeName == "flag") newMode = MapModes::FLAG;
            else if (modeName == "lamp") newMode = MapModes::LAMP;
        }
        
        if (newMode >= 0 && newMode <= 4) {
            settings->saveInt(MAP_MODE, newMode);
            needAdaptColors = true;
            LOG.printf("[API] Map mode changed to: %d\n", newMode);
        } else {
            LOG.printf("[API] Invalid map mode: %d\n", newMode);
        }
    }
    // Обробляємо команду set_lamp
    else if (type == "set_lamp") {
        if (!doc["color"].isNull()) {
            String color = doc["color"].as<String>();
            if (color.length() == 7 && color[0] == '#') {
                settings->saveString(COLOR_LAMP, color.c_str());
                needAdaptColors = true;
                LOG.printf("[API] Lamp color changed to: %s\n", color.c_str());
            }
        }
        
        if (!doc["brightness"].isNull()) {
            int brightness = doc["brightness"].as<int>();
            if (brightness >= 0 && brightness <= 100) {
                settings->saveInt(BRIGHTNESS_LAMP, brightness);
                needAdaptColors = true;
                LOG.printf("[API] Lamp brightness changed to: %d\n", brightness);
            }
        }
    }
    // Обробляємо команду set_home_region
    else if (type == "set_home_region") {
        if (!doc["region_id"].isNull()) {
            int regionId = doc["region_id"].as<int>();
            if (regionId >= 0 && regionId <= 100) {
                settings->saveInt(HOME_DISTRICT, regionId);
                needAdaptColors = true;
                LOG.printf("[API] Home region changed to: %d\n", regionId);
            }
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
            
            LOG.printf("[API] Client added, total clients: %d\n", wsClients.size());
        }
    }
    
    // Poll всіх клієнтів для обробки повідомлень
    for (auto& client : wsClients) {
        if (client.available()) {
            client.poll();
        }
    }
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
    doc["map_mode"] = getMapModeName(newMode);
    doc["map_mode_id"] = newMode;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast map mode change: %d\n", newMode);
}

void JaamApi::broadcastLampChange(const String& color, int brightness) {
    JsonDocument doc;
    doc["type"] = "lamp_change";
    JsonObject lamp = doc["lamp"].to<JsonObject>();
    lamp["color"] = color;
    lamp["brightness"] = brightness;
    
    String data;
    serializeJson(doc, data);
    broadcastWebSocket(data);
    
    LOG.printf("[API] Broadcast lamp change: %s, %d\n", color.c_str(), brightness);
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

// --- Обробка змін налаштувань ---

void JaamApi::onSettingsChange(Type type, int intValue, const char* strValue) {
    // Перевіряємо чи settings ініціалізовані
    if (!settings) {
        return;
    }
    
    // Відстежуємо зміни критичних налаштувань і broadcast-имо їх
    switch (type) {
        case MAP_MODE:
            broadcastMapModeChange(intValue);
            break;
            
        case BRIGHTNESS_LAMP:
            // Якщо змінилась яскравість лампи, відправляємо оновлення
            broadcastLampChange(settings->getString(COLOR_LAMP), intValue);
            break;
            
        case COLOR_LAMP:
            // Якщо змінився колір лампи, відправляємо оновлення
            if (strValue) {
                broadcastLampChange(strValue, settings->getInt(BRIGHTNESS_LAMP));
            }
            break;
            
        case HOME_DISTRICT:
            broadcastHomeRegionChange(intValue);
            break;
            
        default:
            break;
    }
}
