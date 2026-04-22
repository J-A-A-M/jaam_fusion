#pragma once

#include <ArduinoJson.h>
#include <WebServer.h>
#include "JaamSettings.h"
#include "JaamStorage.h"
#include "JaamWifi.h"

class JaamWeb {
public:
    JaamWeb() : server(80), settings(nullptr), strip_main(nullptr), strip_bg(nullptr), strip_service(nullptr), storage(nullptr), wifi(nullptr), chipId(nullptr), fwVersion(nullptr) {}
    void setSettings(JaamSettings* settings);
    void setStorage(JaamStorage* storage);
    void setWifi(JaamWifi* wifi);
    void setDeviceInfo(const char* chipId, const char* fwVersion);
    void begin(Adafruit_NeoPixel* strip_main, Adafruit_NeoPixel* strip_bg, Adafruit_NeoPixel* strip_service);
    void setStrips(Adafruit_NeoPixel* strip_main, Adafruit_NeoPixel* strip_bg, Adafruit_NeoPixel* strip_service);
    void handleClient();

private:
    WebServer server;
    JaamSettings* settings;
    JaamStorage* storage;
    JaamWifi* wifi;
    const char* chipId;
    const char* fwVersion;
    Adafruit_NeoPixel* strip_main;
    Adafruit_NeoPixel* strip_bg;
    Adafruit_NeoPixel* strip_service;
    void handleNotFound();
    void setCrossOrigin();
    void sendCrossOriginHeader();
    bool validateMutatingRequest();
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
    void handleLogsInfo();
    void handleUiSchemaModels();
    void handleUiSchemaSections();
    void handleUiSchemaDropdownLists();
    void handleUiSchemaControls();
    void handleUiSchemaControlsValues();
    void handleUiPage();
    void handleMapData();
    void handleCss();
    void handleJs();
    void handleMapEditorCss();
    void handleMapEditorJs();
    void handleBgColorEditorCss();
    void handleBgColorEditorJs();
    void handleSettingsBackup();
    void handleSettingsRestore();
    void handleSettingsReset();
    void handleWifiPage();
    void handleWifiNetworks();
    void handleWifiAdd();
    void handleWifiRemove();
    void handleWifiScan();
    void handleWifiScanResults();
    String getMeta();
    void buildUiSchemaDropdownLists(JsonDocument& doc);
    void buildUiSchemaControlsValues(JsonDocument& doc);
};