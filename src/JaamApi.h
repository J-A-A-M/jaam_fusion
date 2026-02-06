#pragma once

#include <ArduinoWebsockets.h>
#include "JaamSettings.h"
#include <vector>

using namespace websockets;

class JaamApi {
public:
    JaamApi();
    void setSettings(JaamSettings* settings);
    void setDeviceInfo(const char* chipId, const char* fwVersion);
    void start();
    void stop();
    bool isApiRunning() const;
    void handleWebSocketClients();
    
    // Broadcast події до всіх підключених WebSocket клієнтів
    void broadcastMapModeChange(int newMode);
    void broadcastLampChange(const String& color, int brightness);
    void broadcastHomeRegionChange(int regionId);
    void broadcastAlertChange(int regionId, int alertType);
    
    // Обробка змін налаштувань
    void onSettingsChange(Type type, int intValue, const char* strValue);

private:
    JaamSettings* settings;
    WebsocketsServer* wsServer;
    std::vector<WebsocketsClient> wsClients;
    const char* chipId;
    const char* fwVersion;
    bool isRunning;
    
    // WebSocket handlers
    void handleWebSocketMessage(WebsocketsClient& client, WebsocketsMessage message);
    void sendInitialState(WebsocketsClient& client);
    
    // Helper methods
    String getMapModeName(int mode);
    void broadcastWebSocket(const String& jsonMessage);
};
