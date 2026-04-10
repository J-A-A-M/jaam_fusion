#pragma once

#include <ArduinoWebsockets.h>
#include <vector>
#include "JaamSettings.h"

using namespace websockets;

enum JaamApiButtonEventType : uint8_t {
    JAAM_BUTTON_EVENT_CLICK = 0,
    JAAM_BUTTON_EVENT_LONG_CLICK = 1
};

class JaamApi {
public:
    JaamApi();
    void setSettings(JaamSettings* settings);
    void setDeviceInfo(const char* chipId, const char* fwVersion);
    void setSystemInfo(uint32_t usedMemory, uint32_t uptime, uint32_t wifiUptime, int8_t wifiSignal, bool websocketStatus, uint32_t websocketUptime, float cpuTemp);
    void setHomeAlert(uint16_t flags16);
    void setHomeDistrictTemp(int temp);
    void setClimateData(float temperature, float humidity, float pressure);
    void setLightLevel(float lightLevel);
    void setSensorAvailability(bool tempAvailable, bool humidityAvailable, bool pressureAvailable, bool lightAvailable);
    void setNewFirmwareInfo(const char* version);
    void setFirmwareProgress(int progress);
    void setNightMode(bool state);
    void setMapEnabled(bool enabled);
    void setDisplayEnabled(bool enabled);

    // Broadcast button events to connected API (WebSocket) clients
    void broadcastButtonEvent(uint8_t buttonId, JaamApiButtonEventType eventType);
    void reconfigure();
    bool isApiRunning() const;
    void handleWebSocketClients();
    int getClientsCount() const;
    
    // Обробка змін налаштувань
    void onSettingsChange(Type type, int intValue, float fltValue, const char* strValue);

private:
    JaamSettings* settings;
    WebsocketsServer* wsServer;
    std::vector<WebsocketsClient> wsClients;
    const char* chipId;
    const char* fwVersion;
    bool isRunning;
    int currentPort;

    // Приватні методи управління сервісом
    void start();
    void stop();
    uint32_t usedMemory;
    uint32_t uptime;
    uint32_t wifiUptime;
    int8_t wifiSignal;
    bool websocketStatus;
    uint32_t websocketUptime;
    uint16_t homeAlertFlags;
    float cpuTemp;
    int homeDistrictTemp;

    // Climate sensor data
    float climateTemperature;
    float climateHumidity;
    float climatePressure;

    // Light sensor data
    float lightLevel;

    // Sensor availability flags
    bool sensorTempAvailable;
    bool sensorHumidityAvailable;
    bool sensorPressureAvailable;
    bool sensorLightAvailable;

    bool nightMode;
    bool mapEnabled;
    bool displayEnabled;

    // Latest firmware version available
    char fwLatestVersion[25];

    // Helper methods for validation
    bool isValidMapMode(int mode_id) const;
    bool isValidDisplayMode(int mode_id) const;
    bool isValidRegionId(int region_id) const;

    // WebSocket handlers
    void handleWebSocketMessage(WebsocketsClient& client, WebsocketsMessage message);
    void sendInitialState(WebsocketsClient& client);

    // Broadcast події до всіх підключених WebSocket клієнтів
    void broadcastMapModeChange(int newMode);
    void broadcastDisplayModeChange(int newMode);
    void broadcastLampChange(const char* color, int brightness);
    void broadcastHomeRegionChange(int regionId);
    void broadcastAlertChange(int regionId, int alertType);
    void broadcastHomeAlertChange(uint16_t flags16);
    void broadcastClimateDataChange(float temp, float humidity, float pressure);
    void broadcastLightLevelChange(float lightLevel);
    void broadcastSystemInfo();
    void broadcastNightModeChange(bool state);
    void broadcastMapEnabledChange(bool enabled);
    void broadcastDisplayEnabledChange(bool enabled);
    void broadcastWebSocket(const String& jsonMessage);
    void broadcastDeviceNameChange(const char* deviceName);
    void broadcastHomeDistrictTempChange(int temp);
    void broadcastFirmwareUpdate(const char* version);
};
