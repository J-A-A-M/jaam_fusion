#pragma once

#include <WebServer.h>
#include <WiFiManager.h>
#include "JaamSettings.h"
#include "JaamLed.h"
#include "JaamAnimation.h"

class JaamWeb {
public:
    JaamWeb();
    void begin(JaamSettings* settings, Adafruit_NeoPixel* strip_main, Adafruit_NeoPixel* strip_bg, Adafruit_NeoPixel* strip_service);
    void handleClient();

private:
    WebServer server;
    JaamSettings* settings;
    AnimationManager animManager; 
    Adafruit_NeoPixel* strip_main;
    Adafruit_NeoPixel* strip_bg;
    Adafruit_NeoPixel* strip_service;
    String getHtmlTemplate();
    void handleRoot();
    void handleParameter();
    String getParameterHtml(const char* name, int min, int max, int value, const char* label);
}; 