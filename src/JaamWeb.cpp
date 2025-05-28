#include "JaamWeb.h"
#include "JaamLed.h"
#include "JaamLogs.h"
#include "JaamUtils.h"

void JaamWeb::setSettings(JaamSettings* settings) {
    this->settings = settings;
}


String JaamWeb::getParameterHtml(const char* name, int min, int max, int value, const char* label) {
    String html = "<div class='slider-container'>";
    html += "<span class='value' id='" + String(name) + "Value'>[" + String(value) + "]</span>";
    html += "<label for='" + String(name) + "'>" + String(label) + ":</label>";
    html += "<input type='range' min='" + String(min) + "' max='" + String(max) + "' value='" + String(value) + "' class='slider' id='" + String(name) + "' onchange='updateParameter(\"" + String(name) + "\", this.value)'>";
    html += "</div>";
    return html;
}

String JaamWeb::getBoolParameterHtml(const char* name, bool value, const char* label) {
    String html = "<div class='switch-container'>";
    
    html += "<input type='checkbox' id='" + String(name) + "' class='switch' ";
    if (value) html += "checked ";
    html += "onchange='updateBoolParameter(\"" + String(name) + "\", this.checked)'>";
    html += "<label for='" + String(name) + "'>" + String(label) + "</label>";
    html += "</div>";
    return html;
}

String JaamWeb::getColorPickerHtml(const char* name, const char* value, const char* label) {
    String html = "<div class='color-picker-container'>";
    html += "<input class='value' type='color' id='" + String(name) + "' value='" + String(value) + "' onchange='updateColor(\"" + String(name) + "\", this.value)'>";
    html += "<label for='" + String(name) + "'>" + String(label) + "</label>";
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
    html += ".value{font-size:18px;margin-right:10px}";
    html += ".color-picker-container{margin:20px 0}";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>JAAM LED Control</h1>";


    
    // Додаємо слайдери для всіх параметрів
    html += getColorPickerHtml("color_alert", settings->getString(COLOR_ALERT), "Колір тривоги");
    html += getColorPickerHtml("color_clear", settings->getString(COLOR_CLEAR), "Колір відбою");
    html += getColorPickerHtml("color_new_alert", settings->getString(COLOR_NEW_ALERT), "Колір початку тривоги");
    html += getColorPickerHtml("color_alert_over", settings->getString(COLOR_ALERT_OVER), "Колір завершення тривоги");
    html += getColorPickerHtml("color_explosion", settings->getString(COLOR_EXPLOSION), "Колір вибухів");
    html += getColorPickerHtml("color_missiles", settings->getString(COLOR_MISSILES), "Колір ракет");
    html += getColorPickerHtml("color_drones", settings->getString(COLOR_DRONES), "Колір БПЛА");
    html += getColorPickerHtml("color_kab", settings->getString(COLOR_KABS), "Колір КАБ");
    html += getColorPickerHtml("color_home", settings->getString(COLOR_HOME_DISTRICT), "Колір домашнього регіону");
    html += getParameterHtml("brightness", 0, 100, settings->getInt(BRIGHTNESS), "Загальна яскравість");
    html += getParameterHtml("brightness_day", 0, 100, settings->getInt(BRIGHTNESS_DAY), "Яскравість дня");
    html += getParameterHtml("brightness_night", 0, 100, settings->getInt(BRIGHTNESS_NIGHT), "Яскравість ночі");
    html += getParameterHtml("brightness_alert", 0, 100, settings->getInt(BRIGHTNESS_ALERT), "Яскравість тривоги");
    html += getParameterHtml("brightness_clear", 0, 100, settings->getInt(BRIGHTNESS_CLEAR), "Яскравість очищення");
    html += getParameterHtml("brightness_new_alert", 0, 100, settings->getInt(BRIGHTNESS_NEW_ALERT), "Яскравість початку тривоги");
    html += getParameterHtml("brightness_alert_over", 0, 100, settings->getInt(BRIGHTNESS_ALERT_OVER), "Яскравість завершення тривоги");
    html += getParameterHtml("brightness_explosion", 0, 100, settings->getInt(BRIGHTNESS_EXPLOSION), "Яскравість вибухів, дронів, ракет");
    html += getParameterHtml("brightness_home_district", 0, 100, settings->getInt(BRIGHTNESS_HOME_DISTRICT), "Яскравість домашнього району");
    html += getParameterHtml("brightness_bg", 0, 100, settings->getInt(BRIGHTNESS_BG), "Яскравість фону");
    html += getParameterHtml("brightness_service", 0, 100, settings->getInt(BRIGHTNESS_SERVICE), "Яскравість сервісних ледів");
    html += getBoolParameterHtml("enable_kabs", settings->getBool(ENABLE_KABS), "Показувати загрозу КАБ");
    html += getBoolParameterHtml("enable_missiles", settings->getBool(ENABLE_MISSILES), "Показувати ракетну небезпеку");
    html += getBoolParameterHtml("enable_drones", settings->getBool(ENABLE_DRONES), "Показувати загрозу БПЛА");
    
    html += "</div>";
    html += "<script>";
    html += "function updateParameter(name, value) {";
    html += "  document.getElementById(name + 'Value').textContent = value;";
    html += "  fetch('/parameter?name=' + name + '&value=' + value);";
    html += "}";
    html += "function updateBoolParameter(name, checked) {";
    html += "  fetch('/parameter?name=' + name + '&value=' + (checked ? 1 : 0));";
    html += "}";
    html += "function updateColor(name, value) {";
    html += "  fetch('/color?name=' + name + '&value=' + encodeURIComponent(value));";
    html += "}";
    html += "</script></body></html>";
    return html;
}

void JaamWeb::handleRoot() {
    server.sendHeader("Content-Type", "text/html; charset=UTF-8");
    server.send(200, "text/html", getHtmlTemplate());
}

void JaamWeb::handleColorParameter() {
    if (server.hasArg("name") && server.hasArg("value")) {
        String name = server.arg("name");
        String value = server.arg("value");
        const char* paramValue = value.c_str();

        if (name == "color_alert") {
            settings->saveString(COLOR_ALERT, paramValue);
            LOG.printf("[WEB] Setting color_alert: raw=%s\n", paramValue);
        }
        if (name == "color_clear") {
            settings->saveString(COLOR_CLEAR, paramValue);
            LOG.printf("[WEB] Setting color_clear: raw=%s\n", paramValue);
        }
        if (name == "color_new_alert") {
            settings->saveString(COLOR_NEW_ALERT, paramValue);
            LOG.printf("[WEB] Setting color_new_alert: raw=%s\n", paramValue);
        }
        if (name == "color_alert_over") {
            settings->saveString(COLOR_ALERT_OVER, paramValue);
            LOG.printf("[WEB] Setting color_alert_over: raw=%s\n", paramValue);
        }
        if (name == "color_explosion") {
            settings->saveString(COLOR_EXPLOSION, paramValue);
            LOG.printf("[WEB] Setting color_explosion: raw=%s\n", paramValue);
        }
        if (name == "color_missiles") {
            settings->saveString(COLOR_MISSILES, paramValue);
            LOG.printf("[WEB] Setting color_ccolor_missileslear: raw=%s\n", paramValue);
        }
        if (name == "color_drones") {
            settings->saveString(COLOR_DRONES, paramValue);
            LOG.printf("[WEB] Setting color_drones: raw=%s\n", paramValue);
        }
        if (name == "color_kab") {
            settings->saveString(COLOR_KABS, paramValue);
            LOG.printf("[WEB] Setting color_kab: raw=%s\n", paramValue);
        }
        if (name == "color_home") {
            settings->saveString(COLOR_HOME_DISTRICT, paramValue);
            LOG.printf("[WEB] Setting color_home: raw=%s\n", paramValue);
        }
        if (strip_main_initialized) {
            LOG.printf("[WEB] Adjusting colors\n");               
            animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                for (int i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }

        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void JaamWeb::handleParameter() {
    if (server.hasArg("name") && server.hasArg("value")) {
        String name = server.arg("name");
        int value = server.arg("value").toInt();
        
        // Знаходимо відповідний тип параметра за ім'ям
        if (name == "brightness") {
            settings->saveInt(BRIGHTNESS, value);
            if (strip_main_initialized) {
                LOG.printf("[WEB] Setting brightness main: raw=%d, converted=%d\n", value, led.brightnessMapped(value));
                animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                    strip->setBrightness(led.brightnessMapped(value));
                    for (int i = 0; i < strip->numPixels(); i++) {
                        uint32_t color = animation.ledActualColor(strip, i);
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
        } else if (name == "brightness_day") {
            settings->saveInt(BRIGHTNESS_DAY, value);
        } else if (name == "brightness_night") {
            settings->saveInt(BRIGHTNESS_NIGHT, value);
        } else if (name == "brightness_alert") {
            settings->saveInt(BRIGHTNESS_ALERT, value);
            if (strip_main_initialized) {
                LOG.printf("[WEB] Setting brightness clear: raw=%d, converted=%d\n", value, led.brightnessAbsolute(value));               
                animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                    for (int i = 0; i < strip->numPixels(); i++) {
                        uint32_t color = animation.ledActualColor(strip, i);
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
        } else if (name == "brightness_clear") {
            settings->saveInt(BRIGHTNESS_CLEAR, value);
            if (strip_main_initialized) {
                LOG.printf("[WEB] Setting brightness clear: raw=%d, converted=%d\n", value, led.brightnessAbsolute(value));               
                animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                    for (int i = 0; i < strip->numPixels(); i++) {
                        uint32_t color = animation.ledActualColor(strip, i);
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
        } else if (name == "brightness_new_alert") {
            settings->saveInt(BRIGHTNESS_NEW_ALERT, value);
            if (strip_main_initialized) {
                LOG.printf("[WEB] Setting brightness clear: raw=%d, converted=%d\n", value, led.brightnessAbsolute(value));               
                animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                    for (int i = 0; i < strip->numPixels(); i++) {
                        uint32_t color = animation.ledActualColor(strip, i);
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
        } else if (name == "brightness_alert_over") {
            settings->saveInt(BRIGHTNESS_ALERT_OVER, value);
            if (strip_main_initialized) {
                LOG.printf("[WEB] Setting brightness clear: raw=%d, converted=%d\n", value, led.brightnessAbsolute(value));               
                animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                    for (int i = 0; i < strip->numPixels(); i++) {
                        uint32_t color = animation.ledActualColor(strip, i);
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
        } else if (name == "brightness_explosion") {
            settings->saveInt(BRIGHTNESS_EXPLOSION, value);
            if (strip_main_initialized) {
                LOG.printf("[WEB] Setting brightness clear: raw=%d, converted=%d\n", value, led.brightnessAbsolute(value));               
                animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                    for (int i = 0; i < strip->numPixels(); i++) {
                        uint32_t color = animation.ledActualColor(strip, i);
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
        } else if (name == "brightness_home_district") {
            settings->saveInt(BRIGHTNESS_HOME_DISTRICT, value);
            if (strip_main_initialized) {
                LOG.printf("[WEB] Setting brightness home district: raw=%d, converted=%d\n", value, led.brightnessAbsolute(value));               
                animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                    uint32_t color;
                    uint8_t count;
                    const int* leds = getLedsForRegion(homeDistrict, count);
                    for (int i = 0; i < count; ++i) {
                        int ledsIdx[1] = { leds[i] };
                        color = animation.ledActualColor(strip, leds[i]);
                        strip->setPixelColor(leds[i], color);
                    }
                    strip->show();
                });
            }
        } else if (name == "brightness_bg") {
            settings->saveInt(BRIGHTNESS_BG, value);
            if (strip_bg_initialized) {
                LOG.printf("[WEB] Setting home brightness bg: raw=%d, converted=%d\n", value, led.brightnessMapped(value));
                animation.safeStripOperation(strip_bg, [this, value](Adafruit_NeoPixel* strip) {
                    strip->setBrightness(led.brightnessMapped(value));
                    for (int i = 0; i < strip->numPixels(); i++) {
                        uint32_t color = animation.ledActualColor(strip, i);
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
        } else if (name == "brightness_service") {
            settings->saveInt(BRIGHTNESS_SERVICE, value);
            if (strip_service_initialized) {
                LOG.printf("[WEB] Setting home brightness service: raw=%d, converted=%d\n", value, led.brightnessMapped(value));
                animation.safeStripOperation(strip_service, [this, value](Adafruit_NeoPixel* strip) {
                    strip->setBrightness(led.brightnessMapped(value));
                    for (int i = 0; i < strip->numPixels(); i++) {
                        uint32_t color = animation.ledActualColor(strip, i);
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
        } else if (name == "enable_kabs") {
            settings->saveBool(ENABLE_KABS, value != 0);
            if (strip_main_initialized) {
                animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                    for (int i = 0; i < strip->numPixels(); i++) {
                        uint32_t color = animation.ledActualColor(strip, i);
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
        } else if (name == "enable_missiles") {
            settings->saveBool(ENABLE_MISSILES, value != 0);
            if (strip_main_initialized) {
                animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                    for (int i = 0; i < strip->numPixels(); i++) {
                        uint32_t color = animation.ledActualColor(strip, i);
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
        } else if (name == "enable_drones") {
            settings->saveBool(ENABLE_DRONES, value != 0);
            if (strip_main_initialized) {
                animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                    for (int i = 0; i < strip->numPixels(); i++) {
                        uint32_t color = animation.ledActualColor(strip, i);
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
        }
        
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void JaamWeb::begin(Adafruit_NeoPixel* strip_main, Adafruit_NeoPixel* strip_bg, Adafruit_NeoPixel* strip_service) {
    this->strip_main = strip_main;
    this->strip_bg = strip_bg;
    this->strip_service = strip_service;


    // Налаштування веб-сервера
    server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
    server.on("/parameter", HTTP_GET, [this]() { this->handleParameter(); });
    server.on("/color", HTTP_GET, [this]() { this->handleColorParameter(); });

    server.begin();
}

void JaamWeb::handleClient() {
    server.handleClient();
}