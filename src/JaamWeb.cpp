#include "JaamWeb.h"

JaamWeb::JaamWeb() : server(80), settings(nullptr), strip_main(nullptr), strip_bg(nullptr), strip_service(nullptr) {}

String JaamWeb::getParameterHtml(const char* name, int min, int max, int value, const char* label) {
    String html = "<div class='slider-container'>";
    html += "<label for='" + String(name) + "'>" + String(label) + ":</label>";
    html += "<input type='range' min='" + String(min) + "' max='" + String(max) + "' value='" + String(value) + "' class='slider' id='" + String(name) + "' onchange='updateParameter(\"" + String(name) + "\", this.value)'>";
    html += "<span class='value' id='" + String(name) + "Value'>" + String(value) + "</span>";
    html += "</div>";
    return html;
}

String JaamWeb::getHtmlTemplate() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<title>JAAM LED Control</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:20px;background-color:#f0f0f0}";
    html += ".container{max-width:600px;margin:0 auto;background-color:white;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1)}";
    html += ".slider-container{margin:20px 0}";
    html += ".slider{width:100%;height:25px;background:#d3d3d3;outline:none;opacity:0.7;transition:opacity .2s}";
    html += ".slider:hover{opacity:1}";
    html += ".value{font-size:18px;margin-left:10px}";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>JAAM LED Control</h1>";
    
    // Додаємо слайдери для всіх параметрів
    html += getParameterHtml("brightness", 0, 100, settings->getInt(BRIGHTNESS), "Загальна яскравість");
    html += getParameterHtml("brightness_day", 0, 255, settings->getInt(BRIGHTNESS_DAY), "Яскравість дня");
    html += getParameterHtml("brightness_night", 0, 255, settings->getInt(BRIGHTNESS_NIGHT), "Яскравість ночі");
    html += getParameterHtml("brightness_mode", 0, 2, settings->getInt(BRIGHTNESS_MODE), "Режим яскравості");
    html += getParameterHtml("brightness_alert", 0, 255, settings->getInt(BRIGHTNESS_ALERT), "Яскравість тривоги");
    html += getParameterHtml("brightness_clear", 0, 255, settings->getInt(BRIGHTNESS_CLEAR), "Яскравість очищення");
    html += getParameterHtml("brightness_new_alert", 0, 255, settings->getInt(BRIGHTNESS_NEW_ALERT), "Яскравість нової тривоги");
    html += getParameterHtml("brightness_alert_over", 0, 255, settings->getInt(BRIGHTNESS_ALERT_OVER), "Яскравість завершення тривоги");
    html += getParameterHtml("brightness_explosion", 0, 255, settings->getInt(BRIGHTNESS_EXPLOSION), "Яскравість вибуху");
    html += getParameterHtml("brightness_home_district", 0, 255, settings->getInt(BRIGHTNESS_HOME_DISTRICT), "Яскравість домашнього району");
    html += getParameterHtml("brightness_bg", 0, 255, settings->getInt(BRIGHTNESS_BG), "Яскравість фону");
    html += getParameterHtml("brightness_service", 0, 255, settings->getInt(BRIGHTNESS_SERVICE), "Яскравість сервісу");
    
    html += "</div>";
    html += "<script>";
    html += "function updateParameter(name, value) {";
    html += "  document.getElementById(name + 'Value').textContent = value;";
    html += "  fetch('/parameter?name=' + name + '&value=' + value);";
    html += "}";
    html += "</script></body></html>";
    return html;
}

void JaamWeb::handleRoot() {
    server.sendHeader("Content-Type", "text/html; charset=UTF-8");
    server.send(200, "text/html", getHtmlTemplate());
}

void JaamWeb::handleParameter() {
    if (server.hasArg("name") && server.hasArg("value")) {
        String name = server.arg("name");
        int value = server.arg("value").toInt();
        
        // Знаходимо відповідний тип параметра за ім'ям
        if (name == "brightness") {
            settings->saveInt(BRIGHTNESS, value);
            // Оновлюємо яскравість всіх стріпок
            if (strip_main_initialized) {
                safeStripOperation(strip_main, [value](Adafruit_NeoPixel* strip) {
                    strip->setBrightness(brightnessVal(value));
                    strip->show();
                });
            }
            if (strip_bg_initialized) {
                safeStripOperation(strip_bg, [value](Adafruit_NeoPixel* strip) {
                    strip->setBrightness(brightnessVal(value));
                    strip->show();
                });
            }
            if (strip_service_initialized) {
                safeStripOperation(strip_service, [value](Adafruit_NeoPixel* strip) {
                    strip->setBrightness(brightnessVal(value));
                    strip->show();
                });
            }
        } else if (name == "brightness_day") {
            settings->saveInt(BRIGHTNESS_DAY, value);
        } else if (name == "brightness_night") {
            settings->saveInt(BRIGHTNESS_NIGHT, value);
        } else if (name == "brightness_mode") {
            settings->saveInt(BRIGHTNESS_MODE, value);
        } else if (name == "brightness_alert") {
            settings->saveInt(BRIGHTNESS_ALERT, value);
        } else if (name == "brightness_clear") {
            settings->saveInt(BRIGHTNESS_CLEAR, value);
        } else if (name == "brightness_new_alert") {
            settings->saveInt(BRIGHTNESS_NEW_ALERT, value);
        } else if (name == "brightness_alert_over") {
            settings->saveInt(BRIGHTNESS_ALERT_OVER, value);
        } else if (name == "brightness_explosion") {
            settings->saveInt(BRIGHTNESS_EXPLOSION, value);
        } else if (name == "brightness_home_district") {
            settings->saveInt(BRIGHTNESS_HOME_DISTRICT, value);
        } else if (name == "brightness_bg") {
            settings->saveInt(BRIGHTNESS_BG, value);
        } else if (name == "brightness_service") {
            settings->saveInt(BRIGHTNESS_SERVICE, value);
        }
        
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void JaamWeb::begin(JaamSettings* settings, Adafruit_NeoPixel* strip_main, Adafruit_NeoPixel* strip_bg, Adafruit_NeoPixel* strip_service) {
    this->settings = settings;
    this->strip_main = strip_main;
    this->strip_bg = strip_bg;
    this->strip_service = strip_service;

    // Налаштування WiFi через WiFiManager
    WiFiManager wifiManager;
    wifiManager.autoConnect("JAAM_LED_Setup");

    // Налаштування веб-сервера
    server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
    server.on("/parameter", HTTP_GET, [this]() { this->handleParameter(); });

    server.begin();
}

void JaamWeb::handleClient() {
    server.handleClient();
} 