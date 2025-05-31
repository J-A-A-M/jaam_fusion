#include "JaamWeb.h"
#include "JaamLed.h"
#include "JaamLogs.h"
#include "JaamUtils.h"

extern volatile bool needAdaptAnimationColors;
extern volatile bool needAdaptStripBrightness;
extern volatile bool needToReconnectWebsocket;
extern volatile bool needReconnectStrips;
extern volatile bool needReconnectMainStrip;
extern volatile bool needReconnectBgStrip;
extern volatile bool needReconnectServiceStrip;

// extern volatile bool needAdaptNonAnimationColors;
// extern volatile bool needAdaptAlertClearColors;
// extern volatile bool needAdaptAlertColors;
// extern volatile bool needAdaptAlertExplosionColors;
// extern volatile bool needAdaptAlertHomeDistrictColors;

// Forward declaration for socketConnect function from JaamFusion.cpp
extern void socketConnect();
extern void clearAllAlertsMaps();
extern void reconnectStrips();


void JaamWeb::setSettings(JaamSettings* settings) {
    this->settings = settings;
}


String JaamWeb::getParameterHtml(const char* name, int min, int max, int value, const char* label) {
    String html = "<div class='slider-container'>";
    html += "<span class='value' id='" + String(name) + "Value'>[" + String(value) + "]</span>";
    html += "<label for='" + String(name) + "'>" + String(label) + ":</label>";
    html += "<input type='range' min='" + String(min) + "' max='" + String(max) + "' value='" + String(value) + "' class='slider' id='" + String(name) + "' oninput='updateSliderValue(\"" + String(name) + "\", this.value)' onchange='updateParameter(\"" + String(name) + "\", this.value)'>";
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
    html += "<span class='value'><input type='color' id='" + String(name) + "' value='" + String(value) + "' onchange='updateColor(\"" + String(name) + "\", this.value)'></span>";
    html += "<label for='" + String(name) + "'>" + String(label) + "</label>";
    html += "</div>";
    return html;
}

String JaamWeb::getTextInputHtml(const char* name, const char* value, const char* label, const char* placeholder) {
    String html = "<div class='text-input-container'>";
    html += "<label for='" + String(name) + "'>" + String(label) + ":</label>";
    html += "<input type='text' id='" + String(name) + "' value='" + String(value) + "' placeholder='" + String(placeholder) + "' class='text-input' onchange='updateTextParameter(\"" + String(name) + "\", this.value)'>";
    html += "</div>";
    return html;
}

String JaamWeb::getDropdownHtml(const String& name, const String& label, Type settingKey, SettingListItem items[], int itemCount) {
    String html = "<div class=\"form-group\">";
    html += "<label for=\"" + name + "\">" + label + ":</label>";
    html += "<select class=\"form-control\" id=\"" + name + "\" name=\"" + name + "\" onchange='console.log(\"Dropdown changed:\", this.value); updateParameter(\"" + name + "\", this.value);'>";
    
    uint16_t currentValue = settings->getInt(settingKey);
    
    // Додаємо опцію "Не вибрано"
    html += "<option value=\"0\"";
    if (currentValue == 0) html += " selected";
    html += ">Не вибрано</option>";
    
    // Проходимо по масиву items і додаємо тільки ті, що мають ignore=false
    for (int i = 0; i < itemCount; i++) {
        if (!items[i].ignore) {
            html += "<option value=\"" + String(items[i].id) + "\"";
            if (currentValue == items[i].id) {
                html += " selected";
            }
            html += ">" + String(items[i].name) + "</option>";
        }
    }
    
    html += "</select>";
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
    html += ".value{font-size:18px;margin-right:10px;width:60px;display:inline-block;text-align:right}";
    html += ".color-picker-container{margin:20px 0}";
    html += ".text-input-container{margin:20px 0}";
    html += ".text-input{width:100%;padding:8px;font-size:16px;border:1px solid #ddd;border-radius:4px;background-color:white;box-sizing:border-box}";
    html += ".text-input-container label{display:block;margin-bottom:5px;font-weight:bold}";
    html += ".form-group{margin:20px 0}";
    html += ".form-control{width:100%;padding:8px;font-size:16px;border:1px solid #ddd;border-radius:4px;background-color:white}";
    html += ".form-group label{display:block;margin-bottom:5px;font-weight:bold}";
    html += ".label{display:block;margin-bottom:5px;font-weight:bold}";
    html += ".warning{background-color:#fff3cd;border:1px solid #ffeaa7;color:#856404;padding:10px;border-radius:4px;margin:10px 0;font-size:14px}";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>JAAM LED Control</h1>";


    
    // Додаємо слайдери для всіх параметрів
    html += getDropdownHtml("home_district", "Домашний регіон", HOME_DISTRICT, DISTRICTS, MAX_REGIONS);
    html += "<label class=\"label\">Загальні налаштування</label>";
    html += getTextInputHtml("device_name", settings->getString(DEVICE_NAME), "Назва пристрою", "JAAM");
    html += getTextInputHtml("device_description", settings->getString(DEVICE_DESCRIPTION), "Опис пристрою", "JAAM Informer");
    html += getTextInputHtml("broadcast_name", settings->getString(BROADCAST_NAME), "Ім'я в мережі", "jaam");
    html += "<label class=\"label\">Мережеві налаштування</label>";
    html += getTextInputHtml("ws_server_host", settings->getString(WS_SERVER_HOST), "Сервер WebSocket", "ws.jaam.net.ua");
    html += getTextInputHtml("ws_server_port", String(settings->getInt(WS_SERVER_PORT)).c_str(), "Порт WebSocket", "80");
    html += getTextInputHtml("ntp_host", settings->getString(NTP_HOST), "NTP сервер", "time.google.com");
    html += "<label class=\"label\">Home Assistant</label>";
    html += getTextInputHtml("ha_mqtt_user", settings->getString(HA_MQTT_USER), "MQTT користувач", "");
    html += getTextInputHtml("ha_mqtt_password", settings->getString(HA_MQTT_PASSWORD), "MQTT пароль", "");
    html += getTextInputHtml("ha_broker_address", settings->getString(HA_BROKER_ADDRESS), "Адреса брокера", "");
    html += "<label class=\"label\">Піни LED стрічок</label>";
    html += "<div class=\"warning\">⚠️ Увага: Зміна пінів призведе до повного перезапуску всіх LED стрічок!</div>";
    html += getTextInputHtml("main_led_pin", String(settings->getInt(MAIN_LED_PIN)).c_str(), "Основна стрічка (пін)", "13");
    html += getTextInputHtml("bg_led_pin", String(settings->getInt(BG_LED_PIN)).c_str(), "Фонова стрічка (пін)", "-1");
    html += getTextInputHtml("bg_led_count", String(settings->getInt(BG_LED_COUNT)).c_str(), "Фонова стрічка (кількість)", "0");
    html += getTextInputHtml("service_led_pin", String(settings->getInt(SERVICE_LED_PIN)).c_str(), "Сервісна стрічка (пін)", "-1");
    html += "<label class=\"label\">Налаштування кольорів</label>";
    html += getColorPickerHtml("color_alert", settings->getString(COLOR_ALERT), "Тривога");
    html += getColorPickerHtml("color_clear", settings->getString(COLOR_CLEAR), "Відбій");
    html += getColorPickerHtml("color_new_alert", settings->getString(COLOR_NEW_ALERT), "Початок тривоги");
    html += getColorPickerHtml("color_alert_over", settings->getString(COLOR_ALERT_OVER), "Завершення тривоги");
    html += getColorPickerHtml("color_explosion", settings->getString(COLOR_EXPLOSION), "Вибухи");
    html += getColorPickerHtml("color_missiles", settings->getString(COLOR_MISSILES), "Ракети");
    html += getColorPickerHtml("color_drones", settings->getString(COLOR_DRONES), "БПЛА");
    html += getColorPickerHtml("color_kab", settings->getString(COLOR_KABS), "КАБ");
    html += getColorPickerHtml("color_ballistic", settings->getString(COLOR_BALLISTIC), "Балістичні ракети");
    html += getColorPickerHtml("color_home", settings->getString(COLOR_HOME_DISTRICT), "Домашній регіон");
    html += "<label class=\"label\">Налаштування яскравості</label>";
    html += getParameterHtml("brightness", 0, 100, settings->getInt(BRIGHTNESS), "Загальна");
    html += getParameterHtml("brightness_day", 0, 100, settings->getInt(BRIGHTNESS_DAY), "День");
    html += getParameterHtml("brightness_night", 0, 100, settings->getInt(BRIGHTNESS_NIGHT), "Нічь");
    html += getParameterHtml("brightness_alert", 0, 100, settings->getInt(BRIGHTNESS_ALERT), "Тривога");
    html += getParameterHtml("brightness_clear", 0, 100, settings->getInt(BRIGHTNESS_CLEAR), "Без тривоги");
    html += getParameterHtml("brightness_new_alert", 0, 100, settings->getInt(BRIGHTNESS_NEW_ALERT), "Початок тривоги");
    html += getParameterHtml("brightness_alert_over", 0, 100, settings->getInt(BRIGHTNESS_ALERT_OVER), "Відбій тривоги");
    html += getParameterHtml("brightness_explosion", 0, 100, settings->getInt(BRIGHTNESS_EXPLOSION), "Вибухи, дрони, ракети");
    html += getParameterHtml("brightness_home_district", 0, 100, settings->getInt(BRIGHTNESS_HOME_DISTRICT), "Домашній регіон");
    html += getParameterHtml("brightness_bg", 0, 100, settings->getInt(BRIGHTNESS_BG), "Фонова стрічка");
    html += getParameterHtml("brightness_service", 0, 100, settings->getInt(BRIGHTNESS_SERVICE), "Сервісні діоди");
    html += "<label class=\"label\">Налаштування тривог</label>";
    html += getBoolParameterHtml("enable_kabs", settings->getBool(ENABLE_KABS), "Загроза КАБ");
    html += getBoolParameterHtml("enable_missiles", settings->getBool(ENABLE_MISSILES), "Ракетна небезпека");
    html += getBoolParameterHtml("enable_drones", settings->getBool(ENABLE_DRONES), "Загроза БПЛА");
    html += getBoolParameterHtml("enable_ballistic", settings->getBool(ENABLE_BALLISTIC), "Загроза балістичних ракет");
    html += getBoolParameterHtml("enable_explosions", settings->getBool(ENABLE_EXPLOSIONS), "Вибухи");
    
    html += "</div>";
    html += "<script>";
    html += "function updateParameter(name, value) {";
    html += "  var valueElement = document.getElementById(name + 'Value');";
    html += "  if (valueElement) {"; // Перевіряємо чи існує елемент (для слайдерів)
    html += "    valueElement.textContent = '[' + value + ']';";
    html += "  }";
    html += "  fetch('/parameter?name=' + name + '&value=' + value)";
    html += "    .then(response => {";
    html += "      if (!response.ok) {";
    html += "        console.error('Error updating parameter:', name, value);";
    html += "      }";
    html += "    })";
    html += "    .catch(error => {";
    html += "      console.error('Network error:', error);";
    html += "    });";
    html += "}";
    html += "function updateSliderValue(name, value) {";
    html += "  var valueElement = document.getElementById(name + 'Value');";
    html += "  if (valueElement) {";
    html += "    valueElement.textContent = '[' + value + ']';";
    html += "  }";
    html += "}";
    html += "function updateBoolParameter(name, checked) {";
    html += "  fetch('/parameter?name=' + name + '&value=' + (checked ? 1 : 0));";
    html += "}";
    html += "function updateColor(name, value) {";
    html += "  fetch('/color?name=' + name + '&value=' + encodeURIComponent(value));";
    html += "}";
    html += "function updateTextParameter(name, value) {";
    // html += "  // Перевіряємо чи це PIN стрічки";
    // html += "  if (name.includes('_pin') || name === 'bg_led_count') {";
    // html += "    if (!confirm('Ви впевнені, що хочете змінити цей параметр? Це призведе до перезапуску всіх LED стрічок!')) {";
    // html += "      return;";
    // html += "    }";
    // html += "  }";
    html += "  fetch('/text?name=' + name + '&value=' + encodeURIComponent(value))";
    html += "    .then(response => {";
    html += "      if (!response.ok) {";
    html += "        console.error('Error updating text parameter:', name, value);";
    // html += "      } else if (name.includes('_pin') || name === 'bg_led_count') {";
    // html += "        alert('Параметр збережено. LED стрічки будуть перезапущені.');";
    html += "      }";
    html += "    })";
    html += "    .catch(error => {";
    html += "      console.error('Network error:', error);";
    html += "    });";
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
        
        // Use c_str() directly to avoid string copying
        const char* namePtr = name.c_str();
        const char* valuePtr = value.c_str();

        if (name == "color_alert") {
            settings->saveString(COLOR_ALERT, valuePtr);
            LOG.printf("[WEB] Setting color_alert: raw=%s\n", valuePtr);
        }
        if (name == "color_clear") {
            settings->saveString(COLOR_CLEAR, valuePtr);
            LOG.printf("[WEB] Setting color_clear: raw=%s\n", valuePtr);
        }
        if (name == "color_new_alert") {
            settings->saveString(COLOR_NEW_ALERT, valuePtr);
            LOG.printf("[WEB] Setting color_new_alert: raw=%s\n", valuePtr);
        }
        if (name == "color_alert_over") {
            settings->saveString(COLOR_ALERT_OVER, valuePtr);
            LOG.printf("[WEB] Setting color_alert_over: raw=%s\n", valuePtr);
        }
        if (name == "color_explosion") {
            settings->saveString(COLOR_EXPLOSION, valuePtr);
            LOG.printf("[WEB] Setting color_explosion: raw=%s\n", valuePtr);
        }
        if (name == "color_missiles") {
            settings->saveString(COLOR_MISSILES, valuePtr);
            LOG.printf("[WEB] Setting color_ccolor_missileslear: raw=%s\n", valuePtr);
        }
        if (name == "color_drones") {
            settings->saveString(COLOR_DRONES, valuePtr);
            LOG.printf("[WEB] Setting color_drones: raw=%s\n", valuePtr);
        }
        if (name == "color_kab") {
            settings->saveString(COLOR_KABS, valuePtr);
            LOG.printf("[WEB] Setting color_kab: raw=%s\n", valuePtr);
        }
        if (name == "color_ballistic") {
            settings->saveString(COLOR_BALLISTIC, valuePtr);
            LOG.printf("[WEB] Setting color_ballistic: raw=%s\n", valuePtr);
        }
        if (name == "color_home") {
            settings->saveString(COLOR_HOME_DISTRICT, valuePtr);
            LOG.printf("[WEB] Setting color_home: raw=%s\n", valuePtr);
        }
        if (strip_main_initialized) {
            LOG.printf("[WEB] Adjusting colors\n");               
            animation.safeStripOperation(strip_main, [this](Adafruit_NeoPixel* strip) {
                for (int i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
        needAdaptAnimationColors = true;

        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void JaamWeb::handleTextParameter() {
    if (server.hasArg("name") && server.hasArg("value")) {
        String name = server.arg("name");
        String value = server.arg("value");
        
        // Use c_str() directly to avoid string copying
        const char* namePtr = name.c_str();
        const char* valuePtr = value.c_str();

        if (name == "device_name") {
            settings->saveString(DEVICE_NAME, valuePtr);
            LOG.printf("[WEB] Setting device_name: %s\n", valuePtr);
        } else if (name == "device_description") {
            settings->saveString(DEVICE_DESCRIPTION, valuePtr);
            LOG.printf("[WEB] Setting device_description: %s\n", valuePtr);
        } else if (name == "broadcast_name") {
            settings->saveString(BROADCAST_NAME, valuePtr);
            LOG.printf("[WEB] Setting broadcast_name: %s\n", valuePtr);
        } else if (name == "ws_server_host") {
            settings->saveString(WS_SERVER_HOST, valuePtr);
            needToReconnectWebsocket = true;
            LOG.printf("[WEB] Setting ws_server_host: %s\n", valuePtr);
        } else if (name == "ws_server_port") {
            settings->saveInt(WS_SERVER_PORT, value.toInt());
            needToReconnectWebsocket = true;
            LOG.printf("[WEB] Setting ws_server_port: %s\n", valuePtr);
        } else if (name == "ntp_host") {
            settings->saveString(NTP_HOST, valuePtr);
            LOG.printf("[WEB] Setting ntp_host: %s\n", valuePtr);
        } else if (name == "ha_mqtt_user") {
            settings->saveString(HA_MQTT_USER, valuePtr);
            LOG.printf("[WEB] Setting ha_mqtt_user: %s\n", valuePtr);
        } else if (name == "ha_mqtt_password") {
            settings->saveString(HA_MQTT_PASSWORD, valuePtr);
            LOG.printf("[WEB] Setting ha_mqtt_password: %s\n", valuePtr);
        } else if (name == "ha_broker_address") {
            settings->saveString(HA_BROKER_ADDRESS, valuePtr);
            LOG.printf("[WEB] Setting ha_broker_address: %s\n", valuePtr);
        } else if (name == "main_led_pin") {
            int newPin = value.toInt();
            int currentPin = settings->getInt(MAIN_LED_PIN);
            if (newPin != currentPin) {
                LOG.printf("[WEB] Changing main_led_pin from %d to %d\n", currentPin, newPin);
                logMemoryUsage("before main_led_pin change");
                settings->saveInt(MAIN_LED_PIN, newPin);
                needReconnectMainStrip = true;
                LOG.printf("[WEB] Setting main_led_pin: %d\n", newPin);
            }
        } else if (name == "bg_led_pin") {
            int newPin = value.toInt();
            int currentPin = settings->getInt(BG_LED_PIN);
            if (newPin != currentPin) {
                LOG.printf("[WEB] Changing bg_led_pin from %d to %d\n", currentPin, newPin);
                logMemoryUsage("before bg_led_pin change");
                settings->saveInt(BG_LED_PIN, newPin);
                needReconnectBgStrip= true;
                LOG.printf("[WEB] Setting bg_led_pin: %d\n", newPin);
            }
        } else if (name == "bg_led_count") {
            int newCount = value.toInt();
            int currentCount = settings->getInt(BG_LED_COUNT);
            if (newCount != currentCount) {
                LOG.printf("[WEB] Changing bg_led_count from %d to %d\n", currentCount, newCount);
                logMemoryUsage("before bg_led_count change");
                settings->saveInt(BG_LED_COUNT, newCount);
                needReconnectBgStrip = true;
                LOG.printf("[WEB] Setting bg_led_count: %d\n", newCount);
            }
        } else if (name == "service_led_pin") {
            int newPin = value.toInt();
            int currentPin = settings->getInt(SERVICE_LED_PIN);
            if (newPin != currentPin) {
                LOG.printf("[WEB] Changing service_led_pin from %d to %d\n", currentPin, newPin);
                logMemoryUsage("before service_led_pin change");
                settings->saveInt(SERVICE_LED_PIN, newPin);
                needReconnectServiceStrip = true;
                LOG.printf("[WEB] Setting service_led_pin: %d\n", newPin);
            }
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
                needAdaptStripBrightness = true;
            }
        } else if (name == "brightness_day") {
            settings->saveInt(BRIGHTNESS_DAY, value);
        } else if (name == "brightness_night") {
            settings->saveInt(BRIGHTNESS_NIGHT, value);
        } else if (name == "brightness_alert") {
            settings->saveInt(BRIGHTNESS_ALERT, value);
            if (strip_main_initialized) {
                LOG.printf("[WEB] Setting brightness clear: raw=%d, converted=%d\n", value, led.brightnessAbsolute(value));               
                // needAdaptNonAnimationColors = true;
                // needAdaptAlertColors = true; 
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
                // needAdaptNonAnimationColors = true;
                // needAdaptAlertClearColors = true;            
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
                // needAdaptNonAnimationColors = true;
                // needAdaptAlertExplosionColors = true; 
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
                //needAdaptNonAnimationColors = true;
                //needAdaptAlertHomeDistrictColors = true; 
                animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                    uint8_t count;
                    const int* leds = getLedsForRegion(settings->getInt(HOME_DISTRICT), count);
                    for (int i = 0; i < count; ++i) {
                        int ledsIdx[1] = { leds[i] };
                        uint32_t color = animation.ledActualColor(strip, leds[i]);
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
        } else if (name == "enable_ballistic") {
            settings->saveBool(ENABLE_BALLISTIC, value != 0);
            if (strip_main_initialized) {
                animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                    for (int i = 0; i < strip->numPixels(); i++) {
                        uint32_t color = animation.ledActualColor(strip, i);
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
        } else if (name == "enable_explosions") {
            settings->saveBool(ENABLE_EXPLOSIONS, value != 0);
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

        // Додаємо обробку home_district
        else if (name == "home_district") {
            // Перевіряємо чи є такий ID в списку DISTRICTS
            bool validId = false;
            if (value == 0) {
                validId = true; // "Не вибрано" - валідний варіант
            } else {
                for (int i = 0; i < MAX_REGIONS; i++) {
                    if (DISTRICTS[i].id == value && !DISTRICTS[i].ignore) {
                        validId = true;
                        break;
                    }
                }
            }
            if (validId) {
                settings->saveInt(HOME_DISTRICT, value);
                LOG.printf("[WEB] Home district set: %d\n", value);
                if (strip_main_initialized) {
                    animation.safeStripOperation(strip_main, [this, value](Adafruit_NeoPixel* strip) {
                        for (int i = 0; i < strip->numPixels(); i++) {
                            uint32_t color = animation.ledActualColor(strip, i);
                            strip->setPixelColor(i, color);
                        }
                        strip->show();
                    });
                }
            } else {
                LOG.printf("[WEB] Error: invalid district ID: %d\n", value);
            }
        }

        needAdaptAnimationColors = true;
        
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
    server.on("/text", HTTP_GET, [this]() { this->handleTextParameter(); });

    server.begin();
}

void JaamWeb::handleClient() {
    server.handleClient();
}