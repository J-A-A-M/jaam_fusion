#pragma once

#include <ArduinoJson.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include "JaamSettings.h"
#include "JaamStorage.h"

class JaamWeb {
public:
    JaamWeb() : server(80), strip_main(nullptr), strip_bg(nullptr), strip_service(nullptr), storage(nullptr), chipId(nullptr), fwVersion(nullptr) {}
    void setSettings(JaamSettings* settings);
    void setStorage(JaamStorage* storage);
    void setDeviceInfo(const char* chipId, const char* fwVersion);
    void begin(Adafruit_NeoPixel* strip_main, Adafruit_NeoPixel* strip_bg, Adafruit_NeoPixel* strip_service);
    void handleClient();

private:
    WebServer server;
    JaamSettings* settings;
    JaamStorage* storage;
    const char* chipId;
    const char* fwVersion;
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
    void handleUiSchemaModels();
    void handleUiSchemaSections();
    void handleUiSchemaDropdownLists();
    void handleUiSchemaControls();
    void handleUiPage();
    void handleMapData();
    void handleCss();
    void handleJs();
    String getMeta();
    String getStyles();
    String getScripts();
    void buildUiSchemaModels(JsonDocument& doc);
    void buildUiSchemaSections(JsonDocument& doc);
    void buildUiSchemaDropdownLists(JsonDocument& doc);
    void buildUiSchemaControls(JsonDocument& doc);
    String minify(const String& source);
};