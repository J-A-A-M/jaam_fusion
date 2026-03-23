#pragma once

#include <ArduinoWebsockets.h>
#include <vector>
#include "JaamSettings.h"

using namespace websockets;

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
    void reconfigure();
    bool isApiRunning() const;
    void handleWebSocketClients();
    int getClientsCount() const;

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

    // Оновлення даних
    void updateSystemInfo(uint32_t usedMemory, uint32_t uptime, uint32_t wifiUptime, int8_t wifiSignal, bool websocketStatus, uint32_t websocketUptime, float cpuTemp);
    void updateHomeAlert(uint16_t flags16);
    void updateHomeDistrictTemp(int temp);
    void updateClimateData(float temp, float humidity, float pressure);
    void updateLightLevel(float lightLevel);
    void updateNewFirmwareInfo(const char* version);
    void updateFirmwareProgress(int progress);

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

    // Latest firmware version available
    char fwLatestVersion[25];

    // WebSocket handlers
    void handleWebSocketMessage(WebsocketsClient& client, WebsocketsMessage message);
    void sendInitialState(WebsocketsClient& client);
    void broadcastWebSocket(const String& jsonMessage);
    void broadcastDeviceNameChange(const char* deviceName);
    void broadcastHomeDistrictTempChange(int temp);
    void broadcastFirmwareUpdate(const char* version);
};
