#pragma once

#include "JaamSettings.h"

class JaamMDNS {
public:
    JaamMDNS();
    
    void setSettings(JaamSettings* settings);
    void setDeviceInfo(const char* chipId, const char* fwVersion);
    
    // Ініціалізація mDNS з поточними параметрами
    void begin();
    
    // Обробка змін налаштувань
    void onSettingsChange(Type type, int intValue, float fltValue, const char* strValue);
    
private:
    JaamSettings* settings;
    const char* chipId;
    const char* fwVersion;
    
    // Збережені параметри для відстеження змін
    String cachedHost;
    int cachedApiPort;
    String cachedDeviceName;
    
    bool mdnsStarted;
    bool httpServiceActive;
    bool wsServiceActive;
    
    // Внутрішні методи
    void startMDNS();
    void restartMDNS();
    void updateCachedParams();
    
    // Налаштування сервісів
    void configureHttpService();
    void configureWsService(int port);
    void removeWsService();
    
    // Оновлення конфігурації
    void updateInstanceNames();
    void reconfigureWsService();
};
