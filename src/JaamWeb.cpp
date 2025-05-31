#include "JaamWeb.h"
#include "JaamLed.h"
#include "JaamLogs.h"
#include "JaamUtils.h"
#include <esp_system.h>
#include <ArduinoJson.h>

#ifdef __cplusplus
extern "C" {
#endif
// ESP32 temperature sensor function
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif

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

// Function to get CPU temperature in Celsius
float JaamWeb::getCpuTemperature() {
    // ESP32 internal temperature sensor
    // Convert raw value to Celsius (approximate formula)
    uint8_t raw_temp = temprature_sens_read();
    float temp_celsius = (raw_temp - 32) / 1.8;
    return temp_celsius;
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
    // html += "<option value=\"0\"";
    // if (currentValue == 0) html += " selected";
    // html += ">Не вибрано</option>";
    
    // Проходимо по масиву items і додаємо тільки ті, що мають ignore=false
    for (int i = 0; i < itemCount; i++) {
        if (!items[i].ignore) {
            html += "<option value=\"" + String(items[i].id) + "\"";
            if (currentValue == items[i].id) {
                html += " selected";
            }
            // Додаємо префікс "--" якщо sub=true
            String displayName = items[i].sub ? "-- " + String(items[i].name) : String(items[i].name);
            html += ">" + displayName + "</option>";
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
    html += ".system-panel{background:#f8f9fa;border:1px solid #dee2e6;border-radius:8px;padding:15px;margin-bottom:20px;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap}";
    html += ".system-metric{display:flex;align-items:center;margin:5px 0;min-width:120px}";
    html += ".metric-icon{width:16px;height:16px;margin-right:8px;fill:currentColor}";
    html += ".metric-label{font-size:12px;color:#6c757d;margin-right:5px}";
    html += ".metric-value{font-weight:bold;font-size:14px}";
    html += ".memory-bar{width:100px;height:8px;background:#e9ecef;border-radius:4px;overflow:hidden;margin-left:8px}";
    html += ".memory-fill{height:100%;transition:width 0.3s ease,background-color 0.3s ease}";
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
    
    // System Information Panel
    html += "<div class='system-panel' id='systemPanel'>";
    html += "<div class='system-metric'>";
    html += "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M4,6H20V16H4M20,18A2,2 0 0,0 22,16V6C22,4.89 21.1,4 20,4H4C2.89,4 2,4.89 2,6V16A2,2 0 0,0 4,18H11V20H7V22H17V20H13V18H20Z'/></svg>";
    html += "<span class='metric-label'>Пам'ять:</span>";
    html += "<span class='metric-value' id='memoryUsage'>--</span>";
    html += "<div class='memory-bar'><div class='memory-fill' id='memoryBar' style='width:0%'></div></div>";
    html += "</div>";
    html += "<div class='system-metric'>";
    html += "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M9,5V7H11V5H13V7H15V5H17V7H19A2,2 0 0,1 21,9V19A2,2 0 0,1 19,21H5A2,2 0 0,1 3,19V9A2,2 0 0,1 5,7H7V5M5,9V19H19V9H5M7,11H9V13H7V11M11,11H13V13H11V11M15,11H17V13H15V11M7,15H9V17H7V15M11,15H13V17H11V15M15,15H17V17H15V15Z'/></svg>";
    html += "<span class='metric-label'>Процесор:</span>";
    html += "<span class='metric-value' id='cpuTemp'>--°C</span>";
    html += "</div>";
    html += "<div class='system-metric'>";
    html += "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M12,2A10,10 0 0,1 22,12A10,10 0 0,1 12,22A10,10 0 0,1 2,12A10,10 0 0,1 12,2M12,4A8,8 0 0,0 4,12A8,8 0 0,0 12,20A8,8 0 0,0 20,12A8,8 0 0,0 12,4M12.5,7V12.25L17,14.92L16.25,16.15L11,13V7H12.5Z'/></svg>";
    html += "<span class='metric-label'>Час роботи:</span>";
    html += "<span class='metric-value' id='uptime'>--</span>";
    html += "</div>";
    html += "</div>";


    
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
    html += "<div class=\"warning\">⚠️ Увага: Зміна пінів або типів призведе до повного перезапуску всіх LED стрічок!</div>";
    html += getTextInputHtml("main_led_pin", String(settings->getInt(MAIN_LED_PIN)).c_str(), "Основна стрічка (пін)", "13");
    html += getDropdownHtml("main_led_color_format", "Основна стрічка (формат кольору)", MAIN_LED_COLOR_FORMAT, LED_COLOR_FORMATS, LED_COLOR_FORMATS_COUNT);
    html += getDropdownHtml("main_led_frequency", "Основна стрічка (частота)", MAIN_LED_FREQUENCY, LED_FREQUENCIES, LED_FREQUENCIES_COUNT);
    html += getTextInputHtml("bg_led_pin", String(settings->getInt(BG_LED_PIN)).c_str(), "Фонова стрічка (пін)", "-1");
    html += getTextInputHtml("bg_led_count", String(settings->getInt(BG_LED_COUNT)).c_str(), "Фонова стрічка (кількість)", "0");
    html += getDropdownHtml("bg_led_color_format", "Фонова стрічка (формат кольору)", BG_LED_COLOR_FORMAT, LED_COLOR_FORMATS, LED_COLOR_FORMATS_COUNT);
    html += getDropdownHtml("bg_led_frequency", "Фонова стрічка (частота)", BG_LED_FREQUENCY, LED_FREQUENCIES, LED_FREQUENCIES_COUNT);
    html += getTextInputHtml("service_led_pin", String(settings->getInt(SERVICE_LED_PIN)).c_str(), "Сервісна стрічка (пін)", "-1");
    html += getDropdownHtml("service_led_color_format", "Сервісна стрічка (формат кольору)", SERVICE_LED_COLOR_FORMAT, LED_COLOR_FORMATS, LED_COLOR_FORMATS_COUNT);
    html += getDropdownHtml("service_led_frequency", "Сервісна стрічка (частота)", SERVICE_LED_FREQUENCY, LED_FREQUENCIES, LED_FREQUENCIES_COUNT);
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
    html += getBoolParameterHtml("enable_missiles", settings->getBool(ENABLE_MISSILES), "Загроза крилатих та авіаційних ракет");
    html += getBoolParameterHtml("enable_drones", settings->getBool(ENABLE_DRONES), "Загроза БПЛА");
    html += getBoolParameterHtml("enable_ballistic", settings->getBool(ENABLE_BALLISTIC), "Загроза балістичних ракет");
    html += getBoolParameterHtml("enable_explosions", settings->getBool(ENABLE_EXPLOSIONS), "Вибухи");
    
    html += "</div>";
    html += "<script>";
    
    // System info update functions
    html += "function getMemoryColor(percent) {";
    html += "  if (percent < 50) return '#28a745';"; // зелений
    html += "  if (percent < 75) return '#ffc107';"; // жовтий  
    html += "  return '#dc3545';"; // червоний
    html += "}";
    html += "function updateSystemInfo() {";
    html += "  fetch('/system-info')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      document.getElementById('memoryUsage').textContent = Math.round(data.usedHeap/1024) + '/' + Math.round(data.totalHeap/1024) + ' KB';";
    html += "      const memoryBar = document.getElementById('memoryBar');";
    html += "      memoryBar.style.width = data.memoryUsagePercent + '%';";
    html += "      memoryBar.style.backgroundColor = getMemoryColor(data.memoryUsagePercent);";
    html += "      document.getElementById('cpuTemp').textContent = data.cpuTemp.toFixed(1) + '°C';";
    html += "      const hours = Math.floor(data.uptime / 3600);";
    html += "      const minutes = Math.floor((data.uptime % 3600) / 60);";
    html += "      document.getElementById('uptime').textContent = hours + 'г ' + minutes + 'хв';";
    html += "    })";
    html += "    .catch(error => console.error('Error fetching system info:', error));";
    html += "}";
    
    // Auto-refresh system info every 5 seconds
    html += "setInterval(updateSystemInfo, 5000);";
    html += "updateSystemInfo();"; // Initial load
    
    // Existing functions
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

void JaamWeb::handleParameter() {
    if (server.hasArg("name") && server.hasArg("value")) {
        String name = server.arg("name");
        String value = server.arg("value");
        
        const char* namePtr = name.c_str();
        const char* valuePtr = value.c_str();
        int intValue = value.toInt();

        if (name == "home_district") {
            settings->saveInt(HOME_DISTRICT, intValue);
            LOG.printf("[WEB] Setting home_district: %d\n", intValue);
        } else if (name == "brightness") {
            settings->saveInt(BRIGHTNESS, intValue);
            LOG.printf("[WEB] Setting brightness: %d\n", intValue);
            needAdaptStripBrightness = true;
        } else if (name == "brightness_day") {
            settings->saveInt(BRIGHTNESS_DAY, intValue);
            LOG.printf("[WEB] Setting brightness_day: %d\n", intValue);
        } else if (name == "brightness_night") {
            settings->saveInt(BRIGHTNESS_NIGHT, intValue);
            LOG.printf("[WEB] Setting brightness_night: %d\n", intValue);
        } else if (name == "brightness_alert") {
            settings->saveInt(BRIGHTNESS_ALERT, intValue);
            LOG.printf("[WEB] Setting brightness_alert: %d\n", intValue);
        } else if (name == "brightness_clear") {
            settings->saveInt(BRIGHTNESS_CLEAR, intValue);
            LOG.printf("[WEB] Setting brightness_clear: %d\n", intValue);
        } else if (name == "brightness_new_alert") {
            settings->saveInt(BRIGHTNESS_NEW_ALERT, intValue);
            LOG.printf("[WEB] Setting brightness_new_alert: %d\n", intValue);
        } else if (name == "brightness_alert_over") {
            settings->saveInt(BRIGHTNESS_ALERT_OVER, intValue);
            LOG.printf("[WEB] Setting brightness_alert_over: %d\n", intValue);
        } else if (name == "brightness_explosion") {
            settings->saveInt(BRIGHTNESS_EXPLOSION, intValue);
            LOG.printf("[WEB] Setting brightness_explosion: %d\n", intValue);
        } else if (name == "brightness_home_district") {
            settings->saveInt(BRIGHTNESS_HOME_DISTRICT, intValue);
            LOG.printf("[WEB] Setting brightness_home_district: %d\n", intValue);
        } else if (name == "brightness_bg") {
            settings->saveInt(BRIGHTNESS_BG, intValue);
            LOG.printf("[WEB] Setting brightness_bg: %d\n", intValue);
            needAdaptStripBrightness = true;
        } else if (name == "brightness_service") {
            settings->saveInt(BRIGHTNESS_SERVICE, intValue);
            LOG.printf("[WEB] Setting brightness_service: %d\n", intValue);
            needAdaptStripBrightness = true;
        } else if (name == "main_led_color_format") {
            settings->saveInt(MAIN_LED_COLOR_FORMAT, intValue);
            LOG.printf("[WEB] Setting main_led_color_format: %d\n", intValue);
            needReconnectMainStrip = true;
        } else if (name == "main_led_frequency") {
            settings->saveInt(MAIN_LED_FREQUENCY, intValue);
            LOG.printf("[WEB] Setting main_led_frequency: %d\n", intValue);
            needReconnectMainStrip = true;
        } else if (name == "bg_led_color_format") {
            settings->saveInt(BG_LED_COLOR_FORMAT, intValue);
            LOG.printf("[WEB] Setting bg_led_color_format: %d\n", intValue);
            needReconnectBgStrip = true;
        } else if (name == "bg_led_frequency") {
            settings->saveInt(BG_LED_FREQUENCY, intValue);
            LOG.printf("[WEB] Setting bg_led_frequency: %d\n", intValue);
            needReconnectBgStrip = true;
        } else if (name == "service_led_color_format") {
            settings->saveInt(SERVICE_LED_COLOR_FORMAT, intValue);
            LOG.printf("[WEB] Setting service_led_color_format: %d\n", intValue);
            needReconnectServiceStrip = true;
        } else if (name == "service_led_frequency") {
            settings->saveInt(SERVICE_LED_FREQUENCY, intValue);
            LOG.printf("[WEB] Setting service_led_frequency: %d\n", intValue);
            needReconnectServiceStrip = true;
        } else if (name == "enable_kabs") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_KABS, boolValue);
            LOG.printf("[WEB] Setting enable_kabs: %d\n", boolValue);
        } else if (name == "enable_missiles") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_MISSILES, boolValue);
            LOG.printf("[WEB] Setting enable_missiles: %d\n", boolValue);
        } else if (name == "enable_drones") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_DRONES, boolValue);
            LOG.printf("[WEB] Setting enable_drones: %d\n", boolValue);
        } else if (name == "enable_ballistic") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_BALLISTIC, boolValue);
            LOG.printf("[WEB] Setting enable_ballistic: %d\n", boolValue);
        } else if (name == "enable_explosions") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_EXPLOSIONS, boolValue);
            LOG.printf("[WEB] Setting enable_explosions: %d\n", boolValue);
        }

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
            settings->saveInt(MAIN_LED_PIN, value.toInt());
            LOG.printf("[WEB] Setting main_led_pin: %s\n", valuePtr);
            needReconnectMainStrip = true;
        } else if (name == "bg_led_pin") {
            settings->saveInt(BG_LED_PIN, value.toInt());
            LOG.printf("[WEB] Setting bg_led_pin: %s\n", valuePtr);
            needReconnectBgStrip = true;
        } else if (name == "bg_led_count") {
            settings->saveInt(BG_LED_COUNT, value.toInt());
            LOG.printf("[WEB] Setting bg_led_count: %s\n", valuePtr);
            needReconnectBgStrip = true;
        } else if (name == "service_led_pin") {
            settings->saveInt(SERVICE_LED_PIN, value.toInt());
            LOG.printf("[WEB] Setting service_led_pin: %s\n", valuePtr);
            needReconnectServiceStrip = true;
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
    server.on("/text", HTTP_GET, [this]() { this->handleTextParameter(); });
    server.on("/system-info", HTTP_GET, [this]() { this->handleSystemInfo(); });

    server.begin();
}

void JaamWeb::handleClient() {
    server.handleClient();
}

void JaamWeb::handleSystemInfo() {
    // Get system information
    size_t freeHeap = ESP.getFreeHeap();
    size_t totalHeap = ESP.getHeapSize();
    size_t usedHeap = totalHeap - freeHeap;
    size_t maxBlock = ESP.getMaxAllocHeap();
    float cpuTemp = getCpuTemperature();
    uint32_t uptime = millis() / 1000; // uptime in seconds
    
    // Create JSON response
    JsonDocument doc;
    doc["freeHeap"] = freeHeap;
    doc["totalHeap"] = totalHeap;
    doc["usedHeap"] = usedHeap;
    doc["maxBlock"] = maxBlock;
    doc["cpuTemp"] = cpuTemp;
    doc["uptime"] = uptime;
    doc["memoryUsagePercent"] = (float)usedHeap / totalHeap * 100.0;
    doc["fragmentationPercent"] = (1.0f - ((float)maxBlock / (float)freeHeap)) * 100.0;
    
    String response;
    serializeJson(doc, response);
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", response);
}