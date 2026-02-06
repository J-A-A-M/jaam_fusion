#pragma once

#include <WebServer.h>
#include <WiFiManager.h>
#include "JaamSettings.h"
#include "JaamStorage.h"
#include "JaamApi.h"

class JaamWeb {
public:
    JaamWeb() : server(80), api(), strip_main(nullptr), strip_bg(nullptr), strip_service(nullptr), storage(nullptr) {}
    void setSettings(JaamSettings* settings);
    void setStorage(JaamStorage* storage);
    void begin(Adafruit_NeoPixel* strip_main, Adafruit_NeoPixel* strip_bg, Adafruit_NeoPixel* strip_service);
    void handleClient();
    JaamApi* getApi() { return &api; }

private:
    WebServer server;
    JaamApi api;
    JaamSettings* settings;
    JaamStorage* storage;
    Adafruit_NeoPixel* strip_main;
    Adafruit_NeoPixel* strip_bg;
    Adafruit_NeoPixel* strip_service;
    void handleNotFound();
    void setCrossOrigin();
    void sendCrossOriginHeader();
    void handleMapEditor();
    void handleSaveMap();
    void handleBgColorEditor();
    void handleBgColorsData();
    void handleSaveBgColors();
    void handleParameter();
    void handleColorParameter();
    void handleTextParameter();
    void handleSystemInfo();
    void handleAlertsInfo();
    void handleUiSchema();
    void handleUiPage();
    void handleMapData();
    String getMeta();
    String getStyles();
    String getScripts();
};