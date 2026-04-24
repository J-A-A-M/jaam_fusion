#pragma once

#include <WiFiMulti.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <vector>
#include <functional>
#include "JaamSettings.h"
#include "JaamConfig.h"

struct SavedNetwork {
    String ssid;
    String pass;
};

class JaamWifi {
public:
    using StringCb = std::function<void(const String&, const String&)>; // (a, b)
    using StrCb    = std::function<void(const String&)>;               // (a)
    using VoidCb   = std::function<void()>;

    // --- Dependency injection ---
    void setSettings(JaamSettings* s) { settings = s; }

    // --- Callbacks ---
    void setOnConnected(StringCb cb)       { onConnectedCb = cb; }       // (ssid, ip)
    void setOnDisconnected(VoidCb cb)      { onDisconnectedCb = cb; }
    void setOnPortalStarted(StrCb cb)    { onPortalStartedCb = cb; }    // (apSsid)
    void setOnClientConnected(StrCb cb)  { onClientConnectedCb = cb; }  // (ip)
    void setOnNetworkSaved(StrCb cb)     { onNetworkSavedCb = cb; }     // (ssid)
    void setOnReboot(VoidCb cb)          { onRebootCb = cb; }

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
    int    getScanCount();
    String getScanSSID(int i);
    int8_t getScanRSSI(int i);
    bool   isScanOpen(int i);

private:
    WiFiMulti     wifiMulti;
    WebServer     portalServer{80};
    DNSServer     dnsServer;
    JaamSettings* settings      = nullptr;

    bool          connected      = false;
    bool          portalActive   = false;
    unsigned long connectTime    = 0;
    unsigned long portalStartTime = 0;
    uint8_t       reconnectAttempts = 0;
    char          apName[32]     = {};

    static const uint8_t  MAX_NETWORKS            = 5;
    static const uint8_t  MAX_RECONNECT_ATTEMPTS  = 5;
    static const uint32_t MULTI_CONNECT_TIMEOUT   = 15000; // ms при першому start
    static const uint32_t MULTI_RECONNECT_TIMEOUT = 5000;  // ms при reconnect
    static const uint32_t PORTAL_TIMEOUT          = 180000; // 3 хв

    StringCb onConnectedCb;
    VoidCb   onDisconnectedCb;
    StrCb    onPortalStartedCb;
    StrCb    onClientConnectedCb;
    StrCb    onNetworkSavedCb;
    VoidCb   onRebootCb;

    void loadNetworksIntoMulti();
    void migrateFromWifiManager();
    void setupWifiEvents();
    bool tryMultiConnect(uint32_t timeout);
    void openCaptivePortal();
    void handleConnected();
    void handleDisconnected();
    void saveNetworksToNvs(const std::vector<SavedNetwork>& nets);
};
