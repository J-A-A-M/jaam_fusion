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
    AnimationManager animation; 
    JaamLed led;
    Adafruit_NeoPixel* strip_main;
    Adafruit_NeoPixel* strip_bg;
    Adafruit_NeoPixel* strip_service;
    String getHtmlTemplate();
    void handleRoot();
    void handleParameter();
    void handleColorParameter();
    String getParameterHtml(const char* name, int min, int max, int value, const char* label);
    String getBoolParameterHtml(const char* name, bool value, const char* label);
    String getColorPickerHtml(const char* name, const char* value, const char* label);
}; 