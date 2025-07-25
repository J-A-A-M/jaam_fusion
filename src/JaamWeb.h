#pragma once

#include <WebServer.h>
#include <WiFiManager.h>
#include "JaamSettings.h"
#include "JaamLed.h"
#include "JaamAnimation.h"

class JaamWeb {
public:
    JaamWeb() : server(80), strip_main(nullptr), strip_bg(nullptr), strip_service(nullptr) {}
    void setSettings(JaamSettings* settings);
    void begin(Adafruit_NeoPixel* strip_main, Adafruit_NeoPixel* strip_bg, Adafruit_NeoPixel* strip_service);
    void handleClient();

private:
    WebServer server;
    JaamSettings* settings;
    Adafruit_NeoPixel* strip_main;
    Adafruit_NeoPixel* strip_bg;
    Adafruit_NeoPixel* strip_service;
    String getHtmlTemplate();
    void handleNotFound();
    void setCrossOrigin();
    void sendCrossOriginHeader();
    void handleRoot();
    void handleMapEditor();
    void handleSaveMap();
    void handleParameter();
    void handleColorParameter();
    void handleTextParameter();
    void handleSystemInfo();
    void handleAlertsInfo();
    String getMeta();
    String getStyles();
    String getScripts();
    String getParameterHtml(const char* name, int min, int max, int step, int value, const char* label);
    String getBoolParameterHtml(const char* name, bool value, const char* label);
    String getColorPickerHtml(const char* name, const char* value, const char* label);
    String getDropdownHtml(const String& name, const String& label, Type settingKey, SettingListItem items[], int itemCount);
    String getTextInputHtml(const char* name, const char* value, const char* label, const char* placeholder = "");
};