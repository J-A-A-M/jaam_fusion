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
extern volatile bool needAdaptColors;
extern volatile bool needAdaptAnimationBrightness;
extern volatile bool needAdaptAnimationPeriod;
extern volatile bool needAdaptAnimationType;
extern volatile bool needReconnectWebsocket;
extern volatile bool needReconnectMainStrip;
extern volatile bool needReconnectBgStrip;
extern volatile bool needReconnectServiceStrip;
extern volatile bool needUpdateBatteryPin;
extern volatile bool needRecalculateLeds;
extern volatile bool needReconfigureDisplay;
extern volatile bool needUpdateAnimationsMode;
extern volatile bool needAdaptClimate;

extern RegionLedMapEntry                customMap[MAX_REGIONS];


void JaamWeb::setSettings(JaamSettings* settings) {
    this->settings = settings;
}

String JaamWeb::getMeta() {
    String html = "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";

    html += "<meta name='mobile-web-app-capable' content='yes' />";
    html += "<meta name='application-name' content='JAAM' />";
    html += "<meta name='msapplication-starturl' content='/' /> ";
    html += "<meta name='apple-mobile-web-app-capable' content='yes' />";
    html += "<meta name='apple-mobile-web-app-title' content='JAAM'>";
    html += "<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />";
    html += "<link rel='shortcut icon' href='favicon.png'>";
    html += "<link rel='apple-touch-icon' href='apple-touch-icon.png'>";
    return html;
}

String JaamWeb::getStyles() {
    String html = "<style>";
    // Основні CSS змінні для темізації
    html += ":root{";
    html += "--bg-color:#f0f0f0;";
    html += "--container-bg:#ffffff;";
    html += "--text-color:#000000;";
    html += "--border-color:#dee2e6;";
    html += "--panel-bg:#f8f9fa;";
    html += "--alerts-panel-bg:#fff5f5;";
    html += "--alerts-panel-border:#fed7d7;";
    html += "--input-bg:#ffffff;";
    html += "--input-border:#ddd;";
    html += "--warning-bg:#fff3cd;";
    html += "--warning-border:#ffeaa7;";
    html += "--warning-text:#856404;";
    html += "--secondary-text:#6c757d;";
    html += "}";
    
    // Темна тема
    html += "[data-theme='dark']{";
    html += "--bg-color:#1a1a1a;";
    html += "--container-bg:#2d2d2d;";
    html += "--text-color:#ffffff;";
    html += "--border-color:#444444;";
    html += "--panel-bg:#3a3a3a;";
    html += "--alerts-panel-bg:#4a2c2c;";
    html += "--alerts-panel-border:#8b4545;";
    html += "--input-bg:#3a3a3a;";
    html += "--input-border:#555555;";
    html += "--warning-bg:#5a4a2d;";
    html += "--warning-border:#8b7355;";
    html += "--warning-text:#d4b855;";
    html += "--secondary-text:#aaaaaa;";
    html += "}";
    
    html += "body{font-family:Arial,sans-serif;margin:20px;background-color:var(--bg-color);color:var(--text-color);transition:background-color 0.3s ease,color 0.3s ease}";
    html += ".container{max-width:600px;margin:0 auto;background-color:var(--container-bg);padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.3);transition:background-color 0.3s ease}";
    html += ".header-container{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px}";
    html += "h1{margin:0;font-size:1.5em}";
    
    // Перемикач теми
    html += ".theme-toggle{background:none;border:1px solid var(--border-color);cursor:pointer;padding:8px;border-radius:8px;transition:background-color 0.3s ease,border-color 0.3s ease}";
    html += ".theme-toggle:hover{background-color:var(--panel-bg)}";
    html += ".theme-toggle svg{width:20px;height:20px;fill:var(--text-color);transition:fill 0.3s ease}";
    
    html += ".system-panel{background:var(--panel-bg);border:1px solid var(--border-color);border-radius:8px;padding:15px;margin-bottom:20px;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;transition:background-color 0.3s ease,border-color 0.3s ease}";
    html += ".alerts-panel{background:var(--alerts-panel-bg);border:1px solid var(--alerts-panel-border);border-radius:8px;padding:15px;margin-bottom:20px;display:block;justify-content:flex-start;align-items:center;flex-wrap:wrap;transition:background-color 0.3s ease,border-color 0.3s ease}";
    html += ".alert-metric{display:flex;align-items:center;margin:5px 0;min-width:120px}";
    html += ".alerts-loading{color:var(--secondary-text);font-size:12px}";
    html += ".alerts-no-alerts{color:#28a745;font-weight:bold;font-size:12px}";
    html += ".alerts-error{color:#dc3545;font-size:12px}";
    html += ".system-metric{display:flex;align-items:center;margin:5px 0;min-width:120px}";
    html += ".metric-icon{width:16px;height:16px;margin-right:8px;fill:currentColor}";
    html += ".metric-label{font-size:12px;color:var(--secondary-text);margin-right:5px}";
    html += ".metric-value{font-weight:bold;font-size:14px;color:var(--text-color)}";
    html += ".memory-bar{width:100px;height:8px;background:var(--border-color);border-radius:4px;overflow:hidden;margin-left:8px}";
    html += ".memory-fill{height:100%;transition:width 0.3s ease,background-color 0.3s ease}";
    html += ".slider-container{margin:20px 0}";
    html += ".slider{width:100%;height:25px;background:var(--border-color);outline:none;opacity:0.7;transition:opacity .2s}";
    html += ".slider:hover{opacity:1}";
    html += ".value{font-size:18px;margin-right:10px;width:60px;display:inline-block;text-align:right;color:var(--text-color)}";
    html += ".color-picker-container{margin:20px 0}";
    html += ".text-input-container{margin:20px 0}";
    html += ".text-input{width:100%;padding:8px;font-size:16px;border:1px solid var(--input-border);border-radius:4px;background-color:var(--input-bg);color:var(--text-color);box-sizing:border-box;transition:background-color 0.3s ease,border-color 0.3s ease,color 0.3s ease}";
    html += ".text-input-container label{display:block;margin-bottom:5px;font-weight:bold;color:var(--text-color)}";
    html += ".form-group{margin:20px 0}";
    html += ".form-control{width:100%;padding:8px;font-size:16px;border:1px solid var(--input-border);border-radius:4px;background-color:var(--input-bg);color:var(--text-color);transition:background-color 0.3s ease,border-color 0.3s ease,color 0.3s ease}";
    html += ".form-group label{display:block;margin-bottom:5px;font-weight:bold;color:var(--text-color)}";
    html += ".label{display:block;margin-bottom:5px;font-weight:bold;color:var(--text-color)}";
    html += ".warning{background-color:var(--warning-bg);border:1px solid var(--warning-border);color:var(--warning-text);padding:10px;border-radius:4px;margin:10px 0;font-size:14px;transition:background-color 0.3s ease,border-color 0.3s ease,color 0.3s ease}";
    html += ".switch-container{margin:20px 0;display:flex;align-items:center}";
    html += ".switch-container label{margin-left:8px;color:var(--text-color)}";
    html += ".switch{appearance:none;width:50px;height:24px;background:var(--border-color);border-radius:12px;position:relative;cursor:pointer;transition:background-color 0.3s ease}";
    html += ".switch:checked{background:#007bff}";
    html += ".switch::before{content:'';position:absolute;width:20px;height:20px;border-radius:50%;background:white;top:2px;left:2px;transition:transform 0.3s ease}";
    html += ".switch:checked::before{transform:translateX(26px)}";
    html += ".alerts-details-panel{background:var(--alerts-panel-bg);border:1px solid var(--alerts-panel-border);border-radius:8px;padding:15px;margin-bottom:20px;display:flex;flex-direction:column;gap:0;transition:background-color 0.3s ease,border-color 0.3s ease}";
    html += ".alert-detail-row{display:flex;align-items:center;margin:0 0;min-width:120px;}";
    html += ".alert-detail-region{font-size:12px;color:var(--secondary-text);margin-right:5px;font-weight:bold;min-width:60px;}";
    html += ".alert-detail-icon{width:16px;height:16px;fill:currentColor;margin-right:8px;}";
    html += "</style>";
    return html;
}

String JaamWeb::getScripts() {
    // JavaScript функції для теми в head
    String html = "<script>";
    html += "function detectSystemTheme() {";
    html += "  return window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';";
    html += "}";
    html += "function getThemeFromCookie() {";
    html += "  const cookies = document.cookie.split(';');";
    html += "  for(let cookie of cookies) {";
    html += "    const [name, value] = cookie.trim().split('=');";
    html += "    if(name === 'jaam_theme') return value;";
    html += "  }";
    html += "  return null;";
    html += "}";
    html += "function setThemeCookie(theme) {";
    html += "  document.cookie = 'jaam_theme=' + theme + '; max-age=31536000; path=/';";
    html += "}";
    html += "function applyTheme(theme) {";
    html += "  document.documentElement.setAttribute('data-theme', theme);";
    html += "  setThemeCookie(theme);";
    html += "}";
    html += "function toggleTheme() {";
    html += "  const currentTheme = document.documentElement.getAttribute('data-theme') || 'light';";
    html += "  const newTheme = currentTheme === 'dark' ? 'light' : 'dark';";
    html += "  applyTheme(newTheme);";
    html += "}";
    html += "function initTheme() {";
    html += "  const savedTheme = getThemeFromCookie();";
    html += "  const theme = savedTheme || detectSystemTheme();";
    html += "  applyTheme(theme);";
    html += "  if (!savedTheme && window.matchMedia) {";
    html += "    const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');";
    html += "    mediaQuery.addListener(function(e) {";
    html += "      if (!getThemeFromCookie()) {";
    html += "        applyTheme(e.matches ? 'dark' : 'light');";
    html += "      }";
    html += "    });";
    html += "  }";
    html += "}";
    html += "document.addEventListener('DOMContentLoaded', initTheme);";
    html += "</script>";
    return html;
}

String JaamWeb::getParameterHtml(const char* name, float min, float max, float step, float value, const char* label) {
    // Determine decimal precision based on step (e.g., 0.1 -> 1 decimal, 0.01 -> 2 decimals)
    int decimals = 0;
    float s = step;
    while (s > 0.0f && s < 1.0f && decimals < 4) { // cap at 4 decimals to avoid long strings
        s *= 10.0f;
        decimals++;
    }

    auto toStringWithDecimals = [decimals](float v) -> String {
        char buf[24];
        if (decimals > 0) {
            // Build format like "%.1f", "%.2f"
            char fmt[8];
            snprintf(fmt, sizeof(fmt), "%%.%df", decimals);
            snprintf(buf, sizeof(buf), fmt, v);
        } else {
            // No decimals needed
            snprintf(buf, sizeof(buf), "%.0f", v);
        }
        return String(buf);
    };

    String html = "<div class='slider-container'>";
    html += "<span class='value' id='" + String(name) + "Value'>[" + toStringWithDecimals(value) + "]</span>";
    html += "<label for='" + String(name) + "'>" + String(label) + ":</label>";
    html += "<input type='range' min='" + toStringWithDecimals(min) + "' max='" + toStringWithDecimals(max) + "' value='" + toStringWithDecimals(value) + "' step='" + toStringWithDecimals(step) + "' class='slider' id='" + String(name) + "' oninput='updateSliderValue(\"" + String(name) + "\", this.value)' onchange='updateParameter(\"" + String(name) + "\", this.value)'>";
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
    String html = "<!DOCTYPE html><html>";
    html += "<head>";
    html += "<title>JAAM LED Control</title>";
    html += getMeta();
    html += getStyles();
    html += getScripts();
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<div class='header-container'>";
    html += "<h1>JAAM LED Control</h1>";
    
    // Кнопка перемикання теми
    html += "<button class='theme-toggle' onclick='toggleTheme()' title='Перемкнути тему'>";
    html += "<svg viewBox='0 0 24 24'>";
    html += "<path d='M12,18C11.11,18 10.26,17.8 9.5,17.46C11.56,16.06 13,13.72 13,11A6.8,6.8 0 0,0 9.5,4.54C10.26,4.2 11.11,4 12,4A8,8 0 0,1 20,12A8,8 0 0,1 12,20M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z'/>";
    html += "</svg>";
    html += "</button>";
    html += "</div>";
    
    // System Information Panel
    html += "<div class='system-panel' id='systemPanel'>";
    html += "<div class='system-metric'>";
    html += "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M5 3C3.89543 3 3 3.89543 3 5H1V7H3V9H1V11H3V13H1V15H3V17H1V19H3C3 20.1046 3.89543 21 5 21H9C10.1046 21 11 20.1046 11 19H13C13 20.1046 13.8954 21 15 21H19C20.1046 21 21 20.1046 21 19H23V17H21V15H23V13H21V11H23V9H21V7H23V5H21C21 3.89543 20.1046 3 19 3H15C13.8954 3 13 3.89543 13 5H11C11 3.89543 10.1046 3 9 3H5ZM11 7V9H13V7H11ZM11 11V13H13V11H11ZM11 15V17H13V15H11ZM5 5H9V19H5V5ZM15 5H19V19H15V5Z'/></svg>";
    html += "<span class='metric-label'>Пам'ять:</span>";
    html += "<span class='metric-value' id='memoryUsage'>--</span>";
    html += "<div class='memory-bar'><div class='memory-fill' id='memoryBar' style='width:0%'></div></div>";
    html += "</div>";
    html += "<div class='system-metric'>";
    html += "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M7,2V4H6V6H4V7H2V9H4V11H2V13H4V15H2V17H4V18H6V20H7V22H9V20H11V22H13V20H15V22H17V20H18V18H20V17H22V15H20V13H22V11H20V9H22V7H20V6H18V4H17V2H15V4H13V2H11V4H9V2M8,6H16V18H8V6M10,8V16H14V8H10Z'/></svg>";
    html += "<span class='metric-label'>Процесор:</span>";
    html += "<span class='metric-value' id='cpuTemp'>--°C</span>";
    html += "</div>";
    html += "<div class='system-metric'>";
    html += "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M12,2A10,10 0 0,1 22,12A10,10 0 0,1 12,22A10,10 0 0,1 2,12A10,10 0 0,1 12,2M12,4A8,8 0 0,0 4,12A8,8 0 0,0 12,20A8,8 0 0,0 20,12A8,8 0 0,0 12,4M12.5,7V12.25L17,14.92L16.25,16.15L11,13V7H12.5Z'/></svg>";
    html += "<span class='metric-label'>Час роботи:</span>";
    html += "<span class='metric-value' id='uptime'>--</span>";
    html += "</div>";
    html += "<div class='system-metric'>";
    html += "<svg class='metric-icon' viewBox='-1.5 0 19 19'><path d='M14.897 7.404a.553.553 0 0 1-.392-.163 9.192 9.192 0 0 0-13.01 0 .554.554 0 1 1-.784-.783 10.3 10.3 0 0 1 14.578 0 .554.554 0 0 1-.392.946zm-2.172 2.172a.553.553 0 0 1-.392-.162 6.127 6.127 0 0 0-8.666 0 .554.554 0 0 1-.784-.784 7.23 7.23 0 0 1 10.233 0 .554.554 0 0 1-.391.946zm-2.173 2.173a.553.553 0 0 1-.392-.162 3.054 3.054 0 0 0-4.32 0 .554.554 0 1 1-.784-.784 4.163 4.163 0 0 1 5.888 0 .554.554 0 0 1-.392.946zm-1.141 2.048a1.403 1.403 0 1 1-1.403-1.403 1.403 1.403 0 0 1 1.403 1.403z'/></svg>";
    html += "<span class='metric-label'>WiFi:</span>";
    html += "<span class='metric-value' id='wifiSignal'>-- dBm</span>";
    html += "</div>";
    html += "<div class='system-metric'>";
    html += "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M6 20v-2h12v2H6zm6-18C7.48 2 4 5.48 4 10c0 3.87 3.13 7.43 7.55 11.54.29.26.71.26 1 0C16.87 17.43 20 13.87 20 10c0-4.52-3.48-8-8-8zm0 17C8.14 15.24 6 12.39 6 10c0-3.31 2.69-6 6-6s6 2.69 6 6c0 2.39-2.14 5.24-6 9z'/></svg>";
    html += "<span class='metric-label'>WiFi uptime:</span>";
    html += "<span class='metric-value' id='wifiUptime'>--</span>";
    html += "</div>";
    html += "<div class='system-metric'>";
    html += "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M12,2a7.71,7.71,0,0,0-1,15.37v.77h-1a1,1,0,0,0-1,1H2.35V21H9.11a1,1,0,0,0,1,1h3.86a1,1,0,0,0,1-1h6.76V19.11H14.89a1,1,0,0,0-1-1H13v-.77A7.71,7.71,0,0,0,12,2m0,1.67a15.43,15.43,0,0,1,1.21,2.9H10.81A15.83,15.83,0,0,1,12,3.67m-2.15.42a14,14,0,0,0-1,2.48H7A5.78,5.78,0,0,1,9.88,4.09m4.3,0A5.73,5.73,0,0,1,17,6.57H15.17a13,13,0,0,0-1-2.47m-8,4.4H8.48a7.48,7.48,0,0,0-.07,1,7.77,7.77,0,0,0,.07,1H6.33a5.23,5.23,0,0,1-.09-1,5,5,0,0,1,.09-1m3.74,0h3.58a7.48,7.48,0,0,1,.07,1,7.77,7.77,0,0,1-.07,1H10.41a7.77,7.77,0,0,1-.07-1,7.48,7.48,0,0,1,.07-1m5.45,0h1.87a5,5,0,0,1,.09,1,5.23,5.23,0,0,1-.09,1H15.58a7.77,7.77,0,0,0,.07-1,7.48,7.48,0,0,0-.07-1m-8.4,3.85h1.7a13.53,13.53,0,0,0,1,2.47A5.76,5.76,0,0,1,7,12.35m4,0h2.2A15.43,15.43,0,0,1,12,15.25a15.83,15.83,0,0,1-1.22-2.9m4.36,0H17a5.75,5.75,0,0,1-2.85,2.48A13.41,13.41,0,0,0,15.17,12.35Z'/></svg>";
    html += "<span class='metric-label'>Websocket:</span>";
    html += "<span class='metric-value' id='websocketUptime'>--</span>";
    html += "</div>";
    html += "<div class='system-metric'>";
    html += "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M16,4V3.5A1.5,1.5 0 0,0 14.5,2A1.5,1.5 0 0,0 13,3.5V4H7A2,2 0 0,0 5,6V20A2,2 0 0,0 7,22H17A2,2 0 0,0 19,20V6A2,2 0 0,0 17,4H16M7,6H17V20H7V6Z' /></svg>";
    html += "<span class='metric-label'>Батарея:</span>";
    html += "<span class='metric-value' id='batteryVoltage'>--</span>";
    html += "</div>";
    html += "<div class='system-metric'>";
    html += "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M16,4V3.5A1.5,1.5 0 0,0 14.5,2A1.5,1.5 0 0,0 13,3.5V4H7A2,2 0 0,0 5,6V20A2,2 0 0,0 7,22H17A2,2 0 0,0 19,20V6A2,2 0 0,0 17,4H16M7,6H17V20H7V6Z' /></svg>";
    html += "<span class='metric-label'>Температура:</span>";
    html += "<span class='metric-value' id='localTemp'>--</span>";
    html += "</div>";
    html += "</div>";

    // Alerts Information Panel (об'єднано)
    html += "<div class='alerts-panel' id='alertsPanel'>";
    html += "<div class='alert-metric'>";
    html += "<svg class='metric-icon' viewBox='0 0 24 24'>";
    html += "<path d='M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z'/></svg>";
    html += "<span class='metric-label'>Активні тривоги:</span>";
    html += "<span class='metric-value' id='alertsContent'>";
    html += "<span class='alerts-loading'>Завантаження...</span>";
    html += "</span>";
    html += "</div>";
    // Додаємо детальні рядки по регіонах прямо в цей же блок
    html += "<div id='alertsDetailsPanel'></div>";
    html += "</div>";
    
    // Додаємо слайдери для всіх параметрів
    html += getDropdownHtml("legacy", "Режим прошивки", LEGACY, LEGACY_OPTIONS, LEGACY_OPTIONS_COUNT);
    html += "<label class=\"label\">Налаштування дисплея</label>";
    html += getDropdownHtml("display_model", "Тип дисплея", DISPLAY_MODEL, DISPLAY_TYPES, DISPLAY_TYPES_COUNT);
    html += getDropdownHtml("display_height", "Висота дисплея", DISPLAY_HEIGHT, DISPLAY_HEIGHTS, DISPLAY_HEIGHT_COUNT);
    html += getDropdownHtml("display_rotation", "Поворот дисплея", DISPLAY_ROTATION, DISPLAY_ROTATIONS, DISPLAY_ROTATION_COUNT);
    html += getBoolParameterHtml("invert_display", settings->getBool(INVERT_DISPLAY), "Інвертувати дисплей");
    html += getBoolParameterHtml("kyiv_led", settings->getBool(KYIV_LED), "Київ як окремий LED");
    //html += getDropdownHtml("district_mode_kyiv", "Режим леда Київської області", DISTRICT_MODE_KYIV, LED_MODE_OPTIONS, LED_MODE_COUNT);
    //html += getDropdownHtml("district_mode_kharkiv", "Режим леда Харківської області", DISTRICT_MODE_KHARKIV, LED_MODE_OPTIONS, LED_MODE_COUNT);
    //html += getDropdownHtml("district_mode_zp", "Режим леда Запорізької області", DISTRICT_MODE_ZP, LED_MODE_OPTIONS, LED_MODE_COUNT);
    html += getDropdownHtml("home_district", "Домашній регіон", HOME_DISTRICT, DISTRICTS, MAX_REGIONS);
    html += getDropdownHtml("bg_led_mode", "Режим фонової підствітки", BG_LED_MODE, BG_LED_MODES, BG_LED_MODES_COUNT);
    html += getDropdownHtml("map_mode", "Режим мапи", MAP_MODE, MAP_MODES, MAP_MODES_COUNT);
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
    html += "<label class=\"label\">Налаштування погоди</label>";
    html += getParameterHtml("weather_min_temp", -40, 40, 1, settings->getInt(WEATHER_MIN_TEMP), "Мінімальна температура (°C)");
    html += getParameterHtml("weather_max_temp", -40, 40, 1, settings->getInt(WEATHER_MAX_TEMP), "Максимальна температура (°C)");
    html += "<label class=\"label\">Налаштування темпертарури</label>";
    html += getParameterHtml("temp_correction", -10.0f, 10, 0.1f, settings->getFloat(TEMP_CORRECTION), "Корегування температури (°C)");
    html += getParameterHtml("hum_correction", -20.0f, 20, 0.5f, settings->getFloat(HUM_CORRECTION), "Корегування вологості (%)");
    html += getParameterHtml("pressure_correction", -50.0f, 50, 1.0f, settings->getFloat(PRESSURE_CORRECTION), "Корегування атмосферного тиску (мм.рт.ст.)");
    html += "<label class=\"label\">Налаштування анімацій</label>";
    html += getBoolParameterHtml("enable_sync_animations", settings->getBool(ENABLE_SYNC_ANIMATIONS), "Синхронні анімації");
    html += getDropdownHtml("alert_on_animation", "Початок тривог", ANIMATION_ALERT_ON_TYPE, ANIMATION_TYPES, ANIMATION_TYPES_COUNT);
    html += getDropdownHtml("alert_off_animation", "Відбій тривог", ANIMATION_ALERT_OFF_TYPE, ANIMATION_TYPES, ANIMATION_TYPES_COUNT);
    html += getDropdownHtml("drone_animation", "Загроза ударних БПЛА", ANIMATION_DRONE_TYPE, ANIMATION_TYPES, ANIMATION_TYPES_COUNT);
    html += getDropdownHtml("recon_drone_animation", "Розвідувальні БПЛА", ANIMATION_RECON_DRONE_TYPE, ANIMATION_TYPES, ANIMATION_TYPES_COUNT);
    html += getDropdownHtml("missile_animation", "Загроза ракет", ANIMATION_MISSILE_TYPE, ANIMATION_TYPES, ANIMATION_TYPES_COUNT);
    html += getDropdownHtml("kab_animation", "Загроза КАБ", ANIMATION_KAB_TYPE, ANIMATION_TYPES, ANIMATION_TYPES_COUNT);
    html += getDropdownHtml("ballistic_animation", "Загроза балістичних ракет", ANIMATION_BALLISTIC_TYPE, ANIMATION_TYPES, ANIMATION_TYPES_COUNT);
    html += getDropdownHtml("explosion_animation", "Вибухи", ANIMATION_EXPLOSION_TYPE, ANIMATION_TYPES, ANIMATION_TYPES_COUNT);
    html += "<label class=\"label\">Налаштування таймінгів (в секундах)</label>";
    html += getParameterHtml("alert_on_time", 5, 600, 5, settings->getInt(ALERT_ON_TIME), "Початок тривог");
    html += getParameterHtml("alert_off_time", 5, 600, 5, settings->getInt(ALERT_OFF_TIME), "Відбій тривог");
    html += getParameterHtml("drone_time", 5, 600, 5, settings->getInt(DRONE_TIME), "Загроза ударних БПЛА");
    html += getParameterHtml("recon_drone_time", 5, 600, 5, settings->getInt(RECON_DRONE_TIME), "Розвідувальні БПЛА");
    html += getParameterHtml("missile_time", 5, 600, 5, settings->getInt(MISSILE_TIME), "Загроза ракет");
    html += getParameterHtml("kab_time", 5, 600, 5, settings->getInt(KAB_TIME), "Загроза КАБ");
    html += getParameterHtml("ballistic_time", 5, 600, 5, settings->getInt(BALLISTIC_TIME), "Загроза балістичних ракет");
    html += getParameterHtml("explosion_time", 5, 600, 5, settings->getInt(EXPLOSION_TIME), "Вибухи");
    html += "<label class=\"label\">Налаштування цикла (в мілісекундах)</label>";
    html += getParameterHtml("alert_on_cycle", 300, 5000, 100, settings->getInt(ANIMATION_ALERT_ON_CYCLE_TIME), "Початок тривог");
    html += getParameterHtml("alert_off_cycle", 300, 5000, 100, settings->getInt(ANIMATION_ALERT_OFF_CYCLE_TIME), "Відбій тривог");
    html += getParameterHtml("drone_cycle", 300, 5000, 100, settings->getInt(ANIMATION_DRONE_CYCLE_TIME), "Загроза ударних БПЛА");
    html += getParameterHtml("recon_drone_cycle", 300, 5000, 100, settings->getInt(ANIMATION_RECON_DRONE_CYCLE_TIME), "Розвідувальні БПЛА");
    html += getParameterHtml("missile_cycle", 300, 5000, 100, settings->getInt(ANIMATION_MISSILE_CYCLE_TIME), "Загроза ракет");
    html += getParameterHtml("kab_cycle", 300, 5000, 100, settings->getInt(ANIMATION_KAB_CYCLE_TIME), "Загроза КАБ");
    html += getParameterHtml("ballistic_cycle", 300, 5000, 100, settings->getInt(ANIMATION_BALLISTIC_CYCLE_TIME), "Загроза балістичних ракет");
    html += getParameterHtml("explosion_cycle", 300, 5000, 100, settings->getInt(ANIMATION_EXPLOSION_CYCLE_TIME), "Вибухи");
    html += "<label class=\"label\">Налаштування кольорів</label>";
    html += getColorPickerHtml("color_alert", settings->getString(COLOR_ALERT), "Тривога");
    html += getColorPickerHtml("color_clear", settings->getString(COLOR_CLEAR), "Відбій");
    html += getColorPickerHtml("color_explosion", settings->getString(COLOR_EXPLOSION), "Вибухи");
    html += getColorPickerHtml("color_missiles", settings->getString(COLOR_MISSILES), "Ракети");
    html += getColorPickerHtml("color_drones", settings->getString(COLOR_DRONES), "Ударні БПЛА");
    html += getColorPickerHtml("color_recon_drones", settings->getString(COLOR_RECON_DRONES), "Розвідувальні БПЛА");
    html += getColorPickerHtml("color_kab", settings->getString(COLOR_KABS), "КАБ");
    html += getColorPickerHtml("color_ballistic", settings->getString(COLOR_BALLISTIC), "Балістичні ракети");
    html += getColorPickerHtml("color_home", settings->getString(COLOR_HOME_DISTRICT), "Домашній регіон");
    html += getColorPickerHtml("color_bg", settings->getString(COLOR_BG), "Задня підсітка");
    html += "<label class=\"label\">Налаштування яскравості</label>";
    html += getDropdownHtml("brightness_mode", "Режим яскравості", BRIGHTNESS_MODE, AUTO_BRIGHTNESS_MODES, AUTO_BRIGHTNESS_OPTIONS_COUNT);
    html += getParameterHtml("day_start", 0, 24, 1, settings->getInt(DAY_START), "Початок дня");
    html += getParameterHtml("night_start", 0, 24, 1, settings->getInt(NIGHT_START), "Початок ночі");
    html += getParameterHtml("brightness", 0, 100, 1, settings->getInt(BRIGHTNESS), "Загальна");
    html += getParameterHtml("brightness_day", 0, 100, 1, settings->getInt(BRIGHTNESS_DAY), "День");
    html += getParameterHtml("brightness_night", 0, 100, 1, settings->getInt(BRIGHTNESS_NIGHT), "Нічь");
    html += getParameterHtml("brightness_alert", 0, 100, 1, settings->getInt(BRIGHTNESS_ALERT), "Тривога");
    html += getParameterHtml("brightness_clear", 0, 100, 1, settings->getInt(BRIGHTNESS_CLEAR), "Без тривоги");
    html += getParameterHtml("brightness_explosion", 0, 100, 1, settings->getInt(BRIGHTNESS_EXPLOSION), "Вибухи");
    html += getParameterHtml("brightness_missiles", 0, 100, 1, settings->getInt(BRIGHTNESS_MISSILES), "Крилаті та авіаційні ракети");
    html += getParameterHtml("brightness_drones", 0, 100, 1, settings->getInt(BRIGHTNESS_DRONES), "Ударні БПЛА");
    html += getParameterHtml("brightness_recon_drones", 0, 100, 1, settings->getInt(BRIGHTNESS_RECON_DRONES), "Розвідувальні БПЛА");
    html += getParameterHtml("brightness_kabs", 0, 100, 1, settings->getInt(BRIGHTNESS_KABS), "КАБ");
    html += getParameterHtml("brightness_ballistic", 0, 100, 1, settings->getInt(BRIGHTNESS_BALLISTIC), "Балістичні ракети");
    html += getParameterHtml("brightness_home_district", 0, 100, 1, settings->getInt(BRIGHTNESS_HOME_DISTRICT), "Домашній регіон");
    html += getParameterHtml("brightness_bg", 0, 100, 1, settings->getInt(BRIGHTNESS_BG), "Фонова стрічка");
    html += getParameterHtml("brightness_service", 0, 100, 1, settings->getInt(BRIGHTNESS_SERVICE), "Сервісні діоди");
    html += getParameterHtml("brightness_animation_end", 0, 100, 1, settings->getInt(BRIGHTNESS_ANIMATION_END), "Кінцева яскравість анімацій");
    html += "<label class=\"label\">Налаштування тривог</label>";
    html += getBoolParameterHtml("enable_kabs", settings->getBool(ENABLE_KABS), "Загроза КАБ");
    html += getBoolParameterHtml("enable_missiles", settings->getBool(ENABLE_MISSILES), "Загроза крилатих та авіаційних ракет");
    html += getBoolParameterHtml("enable_drones", settings->getBool(ENABLE_DRONES), "Загроза ударних БПЛА");
    html += getBoolParameterHtml("enable_recon_drones", settings->getBool(ENABLE_RECON_DRONES), "Розвідувальні БПЛА");
    html += getBoolParameterHtml("enable_ballistic", settings->getBool(ENABLE_BALLISTIC), "Загроза балістичних ракет");
    html += getBoolParameterHtml("enable_explosions", settings->getBool(ENABLE_EXPLOSIONS), "Вибухи");
    html += getBoolParameterHtml("enable_battery", settings->getBool(ENABLE_BATTERY_MONITORING), "Моніторинг батареї");
    html += getTextInputHtml("battery_pin", String(settings->getInt(BATTERY_PIN)).c_str(), "ADC пін батареї", "-1");
    
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
    html += "      document.getElementById('wifiSignal').textContent = data.wifiRSSI + ' dBm';";
    html += "      const wsHours = Math.floor(data.websocketUptime / 3600);";
    html += "      const wsMinutes = Math.floor((data.websocketUptime % 3600) / 60);";
    html += "      if (data.websocketUptime > 0) {";
    html += "        document.getElementById('websocketUptime').textContent = wsHours + 'г ' + wsMinutes + 'хв';";
    html += "      } else {";
    html += "        document.getElementById('websocketUptime').textContent = 'Відключено';";
    html += "      }";
    html += "      const wifiHours = Math.floor(data.wifiUptime / 3600);";
    html += "      const wifiMinutes = Math.floor((data.wifiUptime % 3600) / 60);";
    html += "      if (data.wifiUptime > 0) {";
    html += "        document.getElementById('wifiUptime').textContent = wifiHours + 'г ' + wifiMinutes + 'хв';";
    html += "      } else {";
    html += "        document.getElementById('wifiUptime').textContent = 'Відключено';";
    html += "      }";
    html += "      if(document.getElementById('batteryVoltage')) {";
    html += "        document.getElementById('batteryVoltage').textContent = data.batteryVoltage ? data.batteryVoltage.toFixed(2) + ' V' : '--';";
    html += "      }";
    html += "      if (data.localTemp > -100) {";
    html += "        document.getElementById('localTemp').textContent = data.localTemp.toFixed(1) + ' °C';";
    html += "      } else {";
    html += "        document.getElementById('localTemp').textContent = '--';";
    html += "      }";
    html += "    })";
    html += "    .catch(error => console.error('Error fetching system info:', error));";
    html += "}";
    
    // Auto-refresh system info every 5 seconds
    html += "setInterval(updateSystemInfo, 5000);";
    html += "setInterval(updateAlertsInfo, 10000);"; // Update alerts every 10 seconds
    html += "updateSystemInfo();"; // Initial load
    html += "updateAlertsInfo();"; // Initial load
    
    // Alerts update function
    html += "function updateAlertsInfo() {";
    html += "  fetch('/alerts-info')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      const alertsContent = document.getElementById('alertsContent');";
    html += "      if (!data.regions || data.regions.length === 0) {";
    html += "        alertsContent.innerHTML = '<span class=\"alerts-no-alerts\">Немає активних тривог</span>';";
    html += "        document.getElementById('alertsDetailsPanel').innerHTML = '';";
    html += "        return;";
    html += "      }";
    html += "      let activeRegions = data.regions.length;";
    html += "      let totalAlerts = 0;";
    html += "      data.regions.forEach(region => {";
    html += "        Object.values(region.alerts).forEach(alert => {";
    html += "          if (alert) totalAlerts++;";
    html += "        });";
    html += "      });";
    html += "      alertsContent.innerHTML = '<span class=\"alerts-error\">' + activeRegions + ' регіонів (' + totalAlerts + ' тривог)</span>';";
    html += "      const alertsDetailsPanel = document.getElementById('alertsDetailsPanel');";
    html += "      let detailsHtml = '';";
    html += "      const alertTypes = [";
    html += "        {key: 'air', label: 'Повітряна тривога', icon: '<svg class=\"alert-detail-icon\" viewBox=\"0 0 24 24\"><path d=\"M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z\"/></svg>'},";
    html += "        {key: 'artillery', label: 'Артобстріл', icon: '<svg class=\"alert-detail-icon\" viewBox=\"0 0 24 24\"><path d=\"M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z\"/></svg>'},";
    html += "        {key: 'urban', label: 'Вуличні бої', icon: '<svg class=\"alert-detail-icon\" viewBox=\"0 0 24 24\"><path d=\"M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z\"/></svg>'},";
    html += "        {key: 'chemical', label: 'Хімічна', icon: '<svg class=\"alert-detail-icon\" viewBox=\"0 0 24 24\"><path d=\"M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z\"/></svg>'},";
    html += "        {key: 'nuclear', label: 'Ядерна', icon: '<svg class=\"alert-detail-icon\" viewBox=\"0 0 24 24\"><path d=\"M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z\"/></svg>'},";
    html += "        {key: 'drones', label: 'Дрони', icon: '<svg class=\"alert-detail-icon\" viewBox=\"0 0 32 32\"><circle cx=\"16\" cy=\"16\" r=\"16\" fill=\"#921D14\"/><path fill-rule=\"evenodd\" clip-rule=\"evenodd\" d=\"M16 29.998c7.73 0 14-6.267 14-13.998S23.73 2.002 16 2.002 2 8.269 2 16s6.269 13.998 14 13.998M4.625 16C4.7 14.7 7.5 14.3 10.5 14.3h1l9.4-6 .5-.2h2.9v15.8h-2.9l-.5-.2-9.4-6h-1c-3 0-5.8-.4-5.875-1.7\" fill=\"#fff\"/></svg>'},";
    html += "        {key: 'missiles', label: 'Ракети', icon: '<svg class=\"alert-detail-icon\" viewBox=\"0 0 24 24\"><path d=\"M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z\"/></svg>'},";
    html += "        {key: 'kab', label: 'КАБ', icon: '<svg class=\"alert-detail-icon\" viewBox=\"0 0 24 24\"><path d=\"M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z\"/></svg>'},";
    html += "        {key: 'ballistic', label: 'Балістична', icon: '<svg class=\"alert-detail-icon\" viewBox=\"0 0 24 24\"><path d=\"M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z\"/></svg>'},";
    html += "        {key: 'explosion', label: 'Вибухи', icon: '<svg class=\"alert-detail-icon\" viewBox=\"0 0 24 24\"><path d=\"M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z\"/></svg>'}";
    html += "      ];";
    html += "      data.regions.forEach(region => {";
    html += "        let icons = '';";
    html += "        alertTypes.forEach(type => {";
    html += "          if (region.alerts && region.alerts[type.key]) {";
    html += "            icons += `<span title='${type.label}'>${type.icon}</span>`;";
    html += "          }";
    html += "        });";
    html += "        detailsHtml += `<div class=\"alert-detail-row\"><span class=\"alert-detail-region\">${region.regionName}</span>${icons}</div>`;";
    html += "      });";
    html += "      alertsDetailsPanel.innerHTML = detailsHtml;";
    html += "    })";
    html += "    .catch(error => {";
    html += "      console.error('Error fetching alerts info:', error);";
    html += "      document.getElementById('alertsContent').innerHTML = '<span class=\"alerts-error\">Помилка завантаження</span>';";
    html += "      document.getElementById('alertsDetailsPanel').innerHTML = '';";
    html += "    });";
    html += "}";
    
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
        setCrossOrigin();
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
        if (name == "color_explosion") {
            settings->saveString(COLOR_EXPLOSION, valuePtr);
            LOG.printf("[WEB] Setting color_explosion: raw=%s\n", valuePtr);
        }
        if (name == "color_missiles") {
            settings->saveString(COLOR_MISSILES, valuePtr);
            LOG.printf("[WEB] Setting color_color_missiles: raw=%s\n", valuePtr);
        }
        if (name == "color_drones") {
            settings->saveString(COLOR_DRONES, valuePtr);
            LOG.printf("[WEB] Setting color_drones: raw=%s\n", valuePtr);
        }
        if (name == "color_recon_drones") {
            settings->saveString(COLOR_RECON_DRONES, valuePtr);
            LOG.printf("[WEB] Setting color_recon_drones: raw=%s\n", valuePtr);
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
        if (name == "color_bg") {
            settings->saveString(COLOR_BG, valuePtr);
            LOG.printf("[WEB] Setting color_bg: raw=%s\n", valuePtr);
        }
        needAdaptColors = true;
        needAdaptAnimationColors = true;

        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void JaamWeb::handleParameter() {
    if (server.hasArg("name") && server.hasArg("value")) {
        setCrossOrigin();
        String name = server.arg("name");
        String value = server.arg("value");
        
        const char* namePtr = name.c_str();
        const char* valuePtr = value.c_str();
        int intValue = value.toInt();
        float floatValue = value.toFloat();

        if (name == "legacy") {
            settings->saveInt(LEGACY, intValue);
            LOG.printf("[WEB] Setting legacy: %d\n", intValue);
            needRecalculateLeds = true;
            needReconfigureDisplay = true;
        } else if (name == "district_mode_kyiv") {
            settings->saveInt(DISTRICT_MODE_KYIV, intValue);
            LOG.printf("[WEB] Setting district_mode_kyiv: %d\n", intValue);
            needRecalculateLeds = true;
        } else if (name == "district_mode_kharkiv") {
            settings->saveInt(DISTRICT_MODE_KHARKIV, intValue);
            LOG.printf("[WEB] Setting district_mode_kharkiv: %d\n", intValue);
            needRecalculateLeds = true;
        } else if (name == "district_mode_zp") {
            settings->saveInt(DISTRICT_MODE_ZP, intValue);
            LOG.printf("[WEB] Setting district_mode_zp: %d\n", intValue);
            needRecalculateLeds = true;
        } else if (name == "home_district") {
            settings->saveInt(HOME_DISTRICT, intValue);
            LOG.printf("[WEB] Setting home_district: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationColors = true;
        } else if (name == "bg_led_mode") {
            settings->saveInt(BG_LED_MODE, intValue);
            LOG.printf("[WEB] Setting bg_led_mode: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationColors = true;
        } else if (name == "map_mode") {
            settings->saveInt(MAP_MODE, intValue);
            LOG.printf("[WEB] Setting map_mode: %d\n", intValue);
            needAdaptColors = true;
        } else if (name == "brightness") {
            settings->saveInt(BRIGHTNESS, intValue);
            LOG.printf("[WEB] Setting brightness: %d\n", intValue);
            needAdaptStripBrightness = true;
        } else if (name == "brightness_day") {
            settings->saveInt(BRIGHTNESS_DAY, intValue);
            LOG.printf("[WEB] Setting brightness_day: %d\n", intValue);
            needAdaptStripBrightness = true;
            // needAdaptColors = true;
            // needAdaptAnimationColors = true;
        } else if (name == "brightness_night") {
            settings->saveInt(BRIGHTNESS_NIGHT, intValue);
            LOG.printf("[WEB] Setting brightness_night: %d\n", intValue);
            needAdaptStripBrightness = true;
            // needAdaptColors = true;
            // needAdaptAnimationColors = true;
        } else if (name == "brightness_alert") {
            settings->saveInt(BRIGHTNESS_ALERT, intValue);
            LOG.printf("[WEB] Setting brightness_alert: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_clear") {
            settings->saveInt(BRIGHTNESS_CLEAR, intValue);
            LOG.printf("[WEB] Setting brightness_clear: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_explosion") {
            settings->saveInt(BRIGHTNESS_EXPLOSION, intValue);
            LOG.printf("[WEB] Setting brightness_explosion: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_missiles") {
            settings->saveInt(BRIGHTNESS_MISSILES, intValue);
            LOG.printf("[WEB] Setting brightness_missiles: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_drones") {
            settings->saveInt(BRIGHTNESS_DRONES, intValue);
            LOG.printf("[WEB] Setting brightness_drones: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_recon_drones") {
            settings->saveInt(BRIGHTNESS_RECON_DRONES, intValue);
            LOG.printf("[WEB] Setting brightness_recon_drones: %d\n", intValue  );
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_kabs") {
            settings->saveInt(BRIGHTNESS_KABS, intValue);
            LOG.printf("[WEB] Setting brightness_kabs: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_ballistic") {
            settings->saveInt(BRIGHTNESS_BALLISTIC, intValue);
            LOG.printf("[WEB] Setting brightness_ballistic: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_home_district") {
            settings->saveInt(BRIGHTNESS_HOME_DISTRICT, intValue);
            LOG.printf("[WEB] Setting brightness_home_district: %d\n", intValue);
            needAdaptColors = true; 
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_bg") {
            settings->saveInt(BRIGHTNESS_BG, intValue);
            LOG.printf("[WEB] Setting brightness_bg: %d\n", intValue);
            needAdaptColors = true; 
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_service") {
            settings->saveInt(BRIGHTNESS_SERVICE, intValue);
            LOG.printf("[WEB] Setting brightness_service: %d\n", intValue);
            needAdaptColors = true; 
        } else if (name == "brightness_animation_end") {
            settings->saveInt(BRIGHTNESS_ANIMATION_END, intValue);
            LOG.printf("[WEB] Setting brightness_animation_end: %d\n", intValue);
            needAdaptAnimationBrightness = true;
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
        } else if (name == "display_model") {
            settings->saveInt(DISPLAY_MODEL, intValue);
            LOG.printf("[WEB] Setting display_model: %d\n", intValue);
            needReconfigureDisplay = true;
        } else if (name == "display_height") {
            settings->saveInt(DISPLAY_HEIGHT, intValue);
            LOG.printf("[WEB] Setting display_height: %d\n", intValue);
            needReconfigureDisplay = true;
        } else if (name == "display_rotation") {
            settings->saveInt(DISPLAY_ROTATION, intValue);
            LOG.printf("[WEB] Setting display_rotation: %d\n", intValue);
            needReconfigureDisplay = true;
        } else if (name == "invert_display") {
            bool boolValue = intValue != 0;
            settings->saveBool(INVERT_DISPLAY, boolValue);
            LOG.printf("[WEB] Setting invert_display: %d\n", boolValue);
            needReconfigureDisplay = true;
        } else if (name == "enable_kabs") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_KABS, boolValue);
            LOG.printf("[WEB] Setting enable_kabs: %d\n", boolValue);
            needAdaptColors = true;
        } else if (name == "enable_missiles") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_MISSILES, boolValue);
            LOG.printf("[WEB] Setting enable_missiles: %d\n", boolValue);
            needAdaptColors = true;
        } else if (name == "enable_drones") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_DRONES, boolValue);
            LOG.printf("[WEB] Setting enable_drones: %d\n", boolValue);
            needAdaptColors = true;
        } else if (name == "enable_recon_drones") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_RECON_DRONES, boolValue);
            LOG.printf("[WEB] Setting enable_recon_drones: %d\n", boolValue);
            needAdaptColors = true;
        } else if (name == "enable_ballistic") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_BALLISTIC, boolValue);
            LOG.printf("[WEB] Setting enable_ballistic: %d\n", boolValue);
            needAdaptColors = true;
        } else if (name == "enable_explosions") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_EXPLOSIONS, boolValue);
            LOG.printf("[WEB] Setting enable_explosions: %d\n", boolValue);
            needAdaptColors = true;
        } else if (name == "enable_sync_animations") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_SYNC_ANIMATIONS, boolValue);
            LOG.printf("[WEB] Setting enable_sync_animations: %d\n", boolValue);
            needUpdateAnimationsMode = true;
        } else if (name == "brightness_mode") {
            settings->saveInt(BRIGHTNESS_MODE, intValue);
            LOG.printf("[WEB] Setting brightness_mode: %d\n", intValue);
               } else if (name == "day_start") {
            settings->saveInt(DAY_START, intValue);
            LOG.printf("[WEB] Setting day_start: %d\n", intValue);
        } else if (name == "night_start") {
            settings->saveInt(NIGHT_START, intValue);
            LOG.printf("[WEB] Setting night_start: %d\n", intValue);
        } else if (name == "alert_on_time") {
            settings->saveInt(ALERT_ON_TIME, intValue);
            LOG.printf("[WEB] Setting alert_on_time: %d\n", intValue);
        } else if (name == "alert_off_time") {
            settings->saveInt(ALERT_OFF_TIME, intValue);
            LOG.printf("[WEB] Setting alert_off_time: %d\n", intValue);
        } else if (name == "drone_time") {
            settings->saveInt(DRONE_TIME, intValue);
            LOG.printf("[WEB] Setting drone_time: %d\n", intValue);
        } else if (name == "recon_drone_time") {
            settings->saveInt(RECON_DRONE_TIME, intValue);
            LOG.printf("[WEB] Setting recon_drone_time: %d\n", intValue);
        } else if (name == "missile_time") {
            settings->saveInt(MISSILE_TIME, intValue);
            LOG.printf("[WEB] Setting missile_time: %d\n", intValue);
        } else if (name == "kab_time") {
            settings->saveInt(KAB_TIME, intValue);
            LOG.printf("[WEB] Setting kab_time: %d\n", intValue);
        } else if (name == "ballistic_time") {
            settings->saveInt(BALLISTIC_TIME, intValue);
            LOG.printf("[WEB] Setting ballistic_time: %d\n", intValue);
        } else if (name == "explosion_time") {
            settings->saveInt(EXPLOSION_TIME, intValue);
            LOG.printf("[WEB] Setting explosion_time: %d\n", intValue);
        } else if (name == "alert_on_cycle") {
            settings->saveInt(ANIMATION_ALERT_ON_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting alert_on_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "alert_off_cycle") {
            settings->saveInt(ANIMATION_ALERT_OFF_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting alert_off_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "drone_cycle") {
            settings->saveInt(ANIMATION_DRONE_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting drone_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "recon_drone_cycle") {
            settings->saveInt(ANIMATION_RECON_DRONE_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting recon_drone_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "missile_cycle") {
            settings->saveInt(ANIMATION_MISSILE_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting missile_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "kab_cycle") {
            settings->saveInt(ANIMATION_KAB_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting kab_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "ballistic_cycle") {
            settings->saveInt(ANIMATION_BALLISTIC_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting ballistic_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "explosion_cycle") {
            settings->saveInt(ANIMATION_EXPLOSION_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting explosion_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "alert_on_animation") {
            settings->saveInt(ANIMATION_ALERT_ON_TYPE, intValue);
            LOG.printf("[WEB] Setting alert_on_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "alert_off_animation") {
            settings->saveInt(ANIMATION_ALERT_OFF_TYPE, intValue);
            LOG.printf("[WEB] Setting alert_off_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "drone_animation") {
            settings->saveInt(ANIMATION_DRONE_TYPE, intValue);
            LOG.printf("[WEB] Setting drone_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "recon_drone_animation") {
            settings->saveInt(ANIMATION_RECON_DRONE_TYPE, intValue);
            LOG.printf("[WEB] Setting recon_drone_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "missile_animation") {
            settings->saveInt(ANIMATION_MISSILE_TYPE, intValue);
            LOG.printf("[WEB] Setting missile_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "kab_animation") {
            settings->saveInt(ANIMATION_KAB_TYPE, intValue);
            LOG.printf("[WEB] Setting kab_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "ballistic_animation") {
            settings->saveInt(ANIMATION_BALLISTIC_TYPE, intValue);
            LOG.printf("[WEB] Setting ballistic_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "explosion_animation") {
            settings->saveInt(ANIMATION_EXPLOSION_TYPE, intValue);
            LOG.printf("[WEB] Setting explosion_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "enable_battery") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_BATTERY_MONITORING, boolValue);
            needUpdateBatteryPin = true;
            LOG.printf("[WEB] Set enable_battery: %d\n", intValue);
        } else if (name == "kyiv_led") {
            bool boolValue = intValue != 0;
            settings->saveBool(KYIV_LED, boolValue);
            needRecalculateLeds = true;
            LOG.printf("[WEB] Set enable_battery: %d\n", intValue);
        } else if (name == "weather_min_temp") {
            settings->saveInt(WEATHER_MIN_TEMP, intValue);
            needAdaptColors = true;
            LOG.printf("[WEB] Set weather_min_temp: %d\n", intValue);
        } else if (name == "weather_max_temp") {
            settings->saveInt(WEATHER_MAX_TEMP, intValue);
            needAdaptColors = true;
            LOG.printf("[WEB] Set weather_max_temp: %d\n", intValue);
        } else if (name == "temp_correction") {
            settings->saveFloat(TEMP_CORRECTION, floatValue);
            needAdaptClimate = true;
            LOG.printf("[WEB] Set temp_correction: %.2f\n", floatValue);
        } else if (name == "hum_correction") {
            settings->saveFloat(HUM_CORRECTION, floatValue);
            needAdaptClimate = true;
            LOG.printf("[WEB] Set hum_correction: %.2f\n", floatValue);
        } else if (name == "pressure_correction") {
            settings->saveFloat(PRESSURE_CORRECTION, floatValue);
            needAdaptClimate = true;
            LOG.printf("[WEB] Set pressure_correction: %.2f\n", floatValue);
        }

        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void JaamWeb::handleTextParameter() {
    if (server.hasArg("name") && server.hasArg("value")) {
        setCrossOrigin();
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
            needReconnectWebsocket = true;
            LOG.printf("[WEB] Setting ws_server_host: %s\n", valuePtr);
        } else if (name == "ws_server_port") {
            settings->saveInt(WS_SERVER_PORT, value.toInt());
            needReconnectWebsocket = true;
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
        } else if (name == "battery_pin") {
            settings->saveInt(BATTERY_PIN, value.toInt());
            needUpdateBatteryPin = true;
            LOG.printf("[WEB] Set battery_pin: %d\n", valuePtr);
        }
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void JaamWeb::setCrossOrigin() {
    server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    server.sendHeader(F("Access-Control-Max-Age"), F("600"));
    server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
    server.sendHeader(F("Access-Control-Allow-Headers"), F("Content-Type,Authorization"));
}

void JaamWeb::sendCrossOriginHeader(){
    LOG.printf("[WEB] sendCORSHeader");
    setCrossOrigin();
    server.send(204);
}

void JaamWeb::handleNotFound() {
    LOG.printf("[WEB] Not found: %s\n", server.uri().c_str());
    server.send(404, "text/plain", "Not found");
}

void JaamWeb::begin(Adafruit_NeoPixel* strip_main, Adafruit_NeoPixel* strip_bg, Adafruit_NeoPixel* strip_service) {
    this->strip_main = strip_main;
    this->strip_bg = strip_bg;
    this->strip_service = strip_service;


    // Налаштування веб-сервера
    server.enableCORS();
    server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
    server.on("/map-editor", HTTP_GET, [this]() { this->handleMapEditor(); });
    server.on("/save-map", HTTP_POST, [this]() { this->handleSaveMap(); });
    server.on("/parameter", HTTP_GET, [this]() { this->handleParameter(); });
    server.on("/parameter", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/color", HTTP_GET, [this]() { this->handleColorParameter(); });
    server.on("/color", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/text", HTTP_GET, [this]() { this->handleTextParameter(); });
    server.on("/text", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/system-info", HTTP_GET, [this]() { this->handleSystemInfo(); });
    server.on("/system-info", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/alerts-info", HTTP_GET, [this]() { this->handleAlertsInfo(); });
    server.on("/alerts-info", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/ui-schema", HTTP_GET, [this]() { this->handleUiSchema(); });
    server.on("/ui-schema", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    // Dynamic UI page that renders based on /ui-schema
    server.on("/ui", HTTP_GET, [this]() { this->handleUiPage(); });
    server.on("/ui", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/favicon.png", HTTP_GET, [this]() { server.send(204); });
    server.onNotFound([this]() { this->handleNotFound(); });

    server.begin();
}

void JaamWeb::handleClient() {
    if (!wifiConnected) {
        return;
    }
    server.handleClient();
}

void JaamWeb::handleSystemInfo() {
    setCrossOrigin();
    String response = getSystemInfoJson();
    server.send(200, "application/json", response);
}

void JaamWeb::handleAlertsInfo() {
    setCrossOrigin();
    String response = getAlertsJson();
    server.send(200, "application/json", response);
}

void JaamWeb::handleUiPage() {
    // Serve a dynamic UI page that mirrors getHtmlTemplate design but renders from /ui-schema
    String html = "<!DOCTYPE html><html>";
    html += "<head>";
    html += "<title>JAAM UI</title>";
    html += getMeta();
    html += getStyles();
    // Reuse system theme and utils scripts
    html += getScripts();
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<div class='header-container'>";
    html += "<h1>JAAM LED Control (Dynamic UI)</h1>";
    html += "<button class='theme-toggle' onclick='toggleTheme()' title='Перемкнути тему'>";
    html += "<svg viewBox='0 0 24 24'><path d='M12,18C11.11,18 10.26,17.8 9.5,17.46C11.56,16.06 13,13.72 13,11A6.8,6.8 0 0,0 9.5,4.54C10.26,4.2 11.11,4 12,4A8,8 0 0,1 20,12A8,8 0 0,1 12,20M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z'/></svg></button>";
    html += "</div>";

    // System panel reused
    html += "<div class='system-panel' id='systemPanel'>";
    html += "<div class='system-metric'><svg class='metric-icon' viewBox='0 0 24 24'><path d='M5 3C3.89543 3 3 3.89543 3 5H1V7H3V9H1V11H3V13H1V15H3V17H1V19H3C3 20.1046 3.89543 21 5 21H9C10.1046 21 11 20.1046 11 19H13C13 20.1046 13.8954 21 15 21H19C20.1046 21 21 20.1046 21 19H23V17H21V15H23V13H21V11H23V9H21V7H23V5H21C21 3.89543 20.1046 3 19 3H15C13.8954 3 13 3.89543 13 5H11C11 3.89543 10.1046 3 9 3H5ZM11 7V9H13V7H11ZM11 11V13H13V11H11ZM11 15V17H13V15H11ZM5 5H9V19H5V5ZM15 5H19V19H15V5Z'/></svg><span class='metric-label'>Пам'ять:</span><span class='metric-value' id='memoryUsage'>--</span><div class='memory-bar'><div class='memory-fill' id='memoryBar' style='width:0%'></div></div></div>";
    html += "<div class='system-metric'><svg class='metric-icon' viewBox='0 0 24 24'><path d='M7,2V4H6V6H4V7H2V9H4V11H2V13H4V15H2V17H4V18H6V20H7V22H9V20H11V22H13V20H15V22H17V20H18V18H20V17H22V15H20V13H22V11H20V9H22V7H20V6H18V4H17V2H15V4H13V2H11V4H9V2M8,6H16V18H8V6M10,8V16H14V8H10Z'/></svg><span class='metric-label'>Процесор:</span><span class='metric-value' id='cpuTemp'>--°C</span></div>";
    html += "<div class='system-metric'><svg class='metric-icon' viewBox='0 0 24 24'><path d='M12,2A10,10 0 0,1 22,12A10,10 0 0,1 12,22A10,10 0 0,1 2,12A10,10 0 0,1 12,2M12,4A8,8 0 0,0 4,12A8,8 0 0,0 12,20A8,8 0 0,0 20,12A8,8 0 0,0 12,4M12.5,7V12.25L17,14.92L16.25,16.15L11,13V7H12.5Z'/></svg><span class='metric-label'>Час роботи:</span><span class='metric-value' id='uptime'>--</span></div>";
    html += "<div class='system-metric'><svg class='metric-icon' viewBox='-1.5 0 19 19'><path d='M14.897 7.404a.553.553 0 0 1-.392-.163 9.192 9.192 0 0 0-13.01 0 .554.554 0 1 1-.784-.783 10.3 10.3 0 0 1 14.578 0 .554.554 0 0 1-.392.946zm-2.172 2.172a.553.553 0 0 1-.392-.162 6.127 6.127 0 0 0-8.666 0 .554.554 0 0 1-.784-.784 7.23 7.23 0 0 1 10.233 0 .554.554 0 0 1-.391.946zm-2.173 2.173a.553.553 0 0 1-.392-.162 3.054 3.054 0 0 0-4.32 0 .554.554 0 1 1-.784-.784 4.163 4.163 0 0 1 5.888 0 .554.554 0 0 1-.392.946zm-1.141 2.048a1.403 1.403 0 1 1-1.403-1.403 1.403 1.403 0 0 1 1.403 1.403z'/></svg><span class='metric-label'>WiFi:</span><span class='metric-value' id='wifiSignal'>-- dBm</span></div>";
    html += "<div class='system-metric'><svg class='metric-icon' viewBox='0 0 24 24'><path d='M6 20v-2h12v2H6zm6-18C7.48 2 4 5.48 4 10c0 3.87 3.13 7.43 7.55 11.54.29.26.71.26 1 0C16.87 17.43 20 13.87 20 10c0-4.52-3.48-8-8-8zm0 17C8.14 15.24 6 12.39 6 10c0-3.31 2.69-6 6-6s6 2.69 6 6c0 2.39-2.14 5.24-6 9z'/></svg><span class='metric-label'>WiFi uptime:</span><span class='metric-value' id='wifiUptime'>--</span></div>";
    html += "<div class='system-metric'><svg class='metric-icon' viewBox='0 0 24 24'><path d='M12,2a7.71,7.71,0,0,0-1,15.37v.77h-1a1,1,0,0,0-1,1H2.35V21H9.11a1,1,0,0,0,1,1h3.86a1,1,0,0,0,1-1h6.76V19.11H14.89a1,1,0,0,0-1-1H13v-.77A7.71,7.71,0,0,0,12,2m0,1.67a15.43,15.43,0,0,1,1.21,2.9H10.81A15.83,15.83,0,0,1,12,3.67m-2.15.42a14,14,0,0,0-1,2.48H7A5.78,5.78,0,0,1,9.88,4.09m4.3,0A5.73,5.73,0,0,1,17,6.57H15.17a13,13,0,0,0-1-2.47m-8,4.4H8.48a7.48,7.48,0,0,0-.07,1,7.77,7.77,0,0,0,.07,1H6.33a5.23,5.23,0,0,1-.09-1,5,5,0,0,1,.09-1m3.74,0h3.58a7.48,7.48,0,0,1,.07,1,7.77,7.77,0,0,1-.07,1H10.41a7.77,7.77,0,0,1-.07-1,7.48,7.48,0,0,1,.07-1m5.45,0h1.87a5,5,0,0,1,.09,1,5.23,5.23,0,0,1-.09,1H15.58a7.77,7.77,0,0,0,.07-1,7.48,7.48,0,0,0-.07-1m-8.4,3.85h1.7a13.53,13.53,0,0,0,1,2.47A5.76,5.76,0,0,1,7,12.35m4,0h2.2A15.43,15.43,0,0,1,12,15.25a15.83,15.83,0,0,1-1.22-2.9m4.36,0H17a5.75,5.75,0,0,1-2.85,2.48A13.41,13.41,0,0,0,15.17,12.35Z'/></svg><span class='metric-label'>Websocket:</span><span class='metric-value' id='websocketUptime'>--</span></div>";
    html += "</div>"; // system-panel

    // Alerts panel reused (just shell); content filled by updateAlertsInfo()
    html += "<div class='alerts-panel' id='alertsPanel'>";
    html += "<div class='alert-metric'><svg class='metric-icon' viewBox='0 0 24 24'><path d='M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z'/></svg><span class='metric-label'>Активні тривоги:</span><span class='metric-value' id='alertsContent'><span class='alerts-loading'>Завантаження...</span></span></div>";
    html += "<div id='alertsDetailsPanel'></div>";
    html += "</div>";

    // Placeholder where controls will be rendered
    html += "<div id='uiControls'></div>";

    // Rendering + utility scripts
    html += "<script>";
    html += R"JS(
function getMemoryColor(percent){ if(percent < 50) return '#28a745'; if(percent < 75) return '#ffc107'; return '#dc3545'; }
function updateSystemInfo(){ fetch('/system-info').then(r=>r.json()).then(data=>{ document.getElementById('memoryUsage').textContent = Math.round(data.usedHeap/1024) + '/' + Math.round(data.totalHeap/1024) + ' KB'; const memoryBar = document.getElementById('memoryBar'); memoryBar.style.width = data.memoryUsagePercent + '%'; memoryBar.style.backgroundColor = getMemoryColor(data.memoryUsagePercent); document.getElementById('cpuTemp').textContent = data.cpuTemp.toFixed(1) + '°C'; const hours = Math.floor(data.uptime / 3600); const minutes = Math.floor((data.uptime % 3600) / 60); document.getElementById('uptime').textContent = hours + 'г ' + minutes + 'хв'; document.getElementById('wifiSignal').textContent = data.wifiRSSI + ' dBm'; const wsHours = Math.floor(data.websocketUptime / 3600); const wsMinutes = Math.floor((data.websocketUptime % 3600) / 60); if (data.websocketUptime > 0) { document.getElementById('websocketUptime').textContent = wsHours + 'г ' + wsMinutes + 'хв'; } else { document.getElementById('websocketUptime').textContent = 'Відключено'; } const wifiHours = Math.floor(data.wifiUptime / 3600); const wifiMinutes = Math.floor((data.wifiUptime % 3600) / 60); if (data.wifiUptime > 0) { document.getElementById('wifiUptime').textContent = wifiHours + 'г ' + wifiMinutes + 'хв'; } else { document.getElementById('wifiUptime').textContent = 'Відключено'; } if(document.getElementById('batteryVoltage')) { document.getElementById('batteryVoltage').textContent = data.batteryVoltage ? data.batteryVoltage.toFixed(2) + ' V' : '--'; } const localTempEl = document.getElementById('localTemp'); if (localTempEl) { if (data.localTemp > -100) { localTempEl.textContent = data.localTemp.toFixed(1) + ' °C'; } else { localTempEl.textContent = '--'; } } }).catch(err=>console.error('Error fetching system info:', err)); }
function updateAlertsInfo(){ fetch('/alerts-info').then(r=>r.json()).then(data=>{ const alertsContent = document.getElementById('alertsContent'); if (!data.regions || data.regions.length === 0) { alertsContent.innerHTML = '<span class="alerts-no-alerts">Немає активних тривог</span>'; document.getElementById('alertsDetailsPanel').innerHTML=''; return; } let activeRegions = data.regions.length; let totalAlerts = 0; data.regions.forEach(region=>{ Object.values(region.alerts).forEach(alert=>{ if(alert) totalAlerts++; }); }); alertsContent.innerHTML = '<span class="alerts-error">' + activeRegions + ' регіонів (' + totalAlerts + ' тривог)</span>'; const alertsDetailsPanel = document.getElementById('alertsDetailsPanel'); let detailsHtml = ''; const alertTypes = [ {key:'air',label:'Повітряна тривога',icon:'<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'}, {key:'artillery',label:'Артобстріл',icon:'<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'}, {key:'urban',label:'Вуличні бої',icon:'<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'}, {key:'chemical',label:'Хімічна',icon:'<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'}, {key:'nuclear',label:'Ядерна',icon:'<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'}, {key:'drones',label:'Дрони',icon:'<svg class="alert-detail-icon" viewBox="0 0 32 32"><circle cx="16" cy="16" r="16" fill="#921D14"/><path fill-rule="evenodd" clip-rule="evenodd" d="M16 29.998c7.73 0 14-6.267 14-13.998S23.73 2.002 16 2.002 2 8.269 2 16s6.269 13.998 14 13.998M4.625 16C4.7 14.7 7.5 14.3 10.5 14.3h1l9.4-6 .5-.2h2.9v15.8h-2.9l-.5-.2-9.4-6h-1c-3 0-5.8-.4-5.875-1.7" fill="#fff"/></svg>'}, {key:'missiles',label:'Ракети',icon:'<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'}, {key:'kab',label:'КАБ',icon:'<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'}, {key:'ballistic',label:'Балістична',icon:'<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'}, {key:'explosion',label:'Вибухи',icon:'<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'} ]; data.regions.forEach(region=>{ let icons=''; alertTypes.forEach(type=>{ if(region.alerts && region.alerts[type.key]){ icons += '<span title="'+type.label+'">'+type.icon+'</span>'; } }); detailsHtml += '<div class="alert-detail-row"><span class="alert-detail-region">'+region.regionName+'</span>'+icons+'</div>'; }); alertsDetailsPanel.innerHTML = detailsHtml; }).catch(err=>{ console.error('Error fetching alerts info:', err); document.getElementById('alertsContent').innerHTML = '<span class="alerts-error">Помилка завантаження</span>'; document.getElementById('alertsDetailsPanel').innerHTML=''; }); }
function updateParameter(name, value){ var valueElement=document.getElementById(name + 'Value'); if(valueElement){ valueElement.textContent='['+value+']'; } fetch('/parameter?name=' + name + '&value=' + value).then(r=>{ if(!r.ok){ console.error('Error updating parameter:', name, value); } }).catch(e=>console.error('Network error:', e)); }
function updateSliderValue(name, value){ var valueElement=document.getElementById(name + 'Value'); if(valueElement){ valueElement.textContent='['+value+']'; } }
function updateBoolParameter(name, checked){ fetch('/parameter?name=' + name + '&value=' + (checked ? 1 : 0)); }
function updateColor(name, value){ fetch('/color?name=' + name + '&value=' + encodeURIComponent(value)); }
function updateTextParameter(name, value){ fetch('/text?name=' + name + '&value=' + encodeURIComponent(value)).then(r=>{ if(!r.ok){ console.error('Error updating text parameter:', name, value); } }).catch(e=>console.error('Network error:', e)); }
async function fetchSchema(){ const res = await fetch('/ui-schema'); return await res.json(); }
function optionEl(id, name, sub){ const opt = document.createElement('option'); opt.value = id; opt.textContent = sub ? ('-- ' + name) : name; return opt; }
function labelEl(text){ const label = document.createElement('label'); label.className='label'; label.textContent = text; return label; }
function groupDiv(){ const div=document.createElement('div'); div.className='form-group'; return div; }
function renderControl(ctrl, lists){ const type=ctrl[0]; if(type==='label'){ return labelEl(ctrl[1]); } if(type==='dropdown'){ const [_, name, label, list, current]=ctrl; const div=groupDiv(); const lab=document.createElement('label'); lab.setAttribute('for', name); lab.textContent=label+':'; const sel=document.createElement('select'); sel.className='form-control'; sel.id=name; sel.name=name; const opts=lists[list]||[]; for(const o of opts){ const el=optionEl(o[0], o[1], o[2]); if (String(o[0])===String(current)) el.selected=true; sel.appendChild(el);} sel.onchange=(e)=>updateParameter(name, e.target.value); div.appendChild(lab); div.appendChild(sel); return div; } if(type==='bool'){ const [_, name, label, current]=ctrl; const div=document.createElement('div'); div.className='switch-container'; const input=document.createElement('input'); input.type='checkbox'; input.id=name; input.className='switch'; input.checked=!!current; input.onchange=(e)=>updateBoolParameter(name, e.target.checked); const lab=document.createElement('label'); lab.setAttribute('for', name); lab.textContent=label; div.appendChild(input); div.appendChild(lab); return div; } if(type==='text'){ const [_, name, label, current, placeholder]=ctrl; const div=document.createElement('div'); div.className='text-input-container'; const lab=document.createElement('label'); lab.setAttribute('for', name); lab.textContent=label+':'; const inp=document.createElement('input'); inp.type='text'; inp.id=name; inp.value=current; inp.placeholder=placeholder||''; inp.className='text-input'; inp.onchange=(e)=>updateTextParameter(name, e.target.value); div.appendChild(lab); div.appendChild(inp); return div; } if(type==='color'){ const [_, name, label, current]=ctrl; const div=document.createElement('div'); div.className='color-picker-container'; const span=document.createElement('span'); span.className='value'; const inp=document.createElement('input'); inp.type='color'; inp.id=name; inp.value=current; inp.onchange=(e)=>updateColor(name, e.target.value); span.appendChild(inp); const lab=document.createElement('label'); lab.setAttribute('for', name); lab.textContent=label; div.appendChild(span); div.appendChild(lab); return div; } if(type==='slider'){ const [_, name, label, min, max, step, current]=ctrl; const div=document.createElement('div'); div.className='slider-container'; const val=document.createElement('span'); val.className='value'; val.id=name+'Value'; val.textContent='['+current+']'; const lab=document.createElement('label'); lab.setAttribute('for', name); lab.textContent=label+':'; const rng=document.createElement('input'); rng.type='range'; rng.min=min; rng.max=max; rng.step=step; rng.value=current; rng.className='slider'; rng.id=name; rng.oninput=(e)=>updateSliderValue(name, e.target.value); rng.onchange=(e)=>updateParameter(name, e.target.value); div.appendChild(val); div.appendChild(lab); div.appendChild(rng); return div; } return document.createTextNode(''); }
async function renderUI(){ try { const schema = await fetchSchema(); const lists = schema.dropdown_lists || {}; const controls = schema.controls || []; const root = document.getElementById('uiControls'); root.innerHTML=''; for(const ctrl of controls){ root.appendChild(renderControl(ctrl, lists)); } } catch(e){ console.error('UI render error', e); } }
document.addEventListener('DOMContentLoaded', ()=>{ renderUI(); updateSystemInfo(); updateAlertsInfo(); setInterval(updateSystemInfo,5000); setInterval(updateAlertsInfo,10000); });
)JS";
    html += "</script>";

    html += "</div></body></html>";
    server.send(200, "text/html", html);
}

// Helper to push dropdown options to a JsonArray as compact lists: [id, name, sub]
static void appendOptionsList(JsonArray arr, SettingListItem items[], int itemCount) {
    for (int i = 0; i < itemCount; ++i) {
        if (items[i].ignore) continue;
        JsonArray opt = arr.add<JsonArray>();
        opt.add(items[i].id);
        opt.add(items[i].name);
        opt.add(items[i].sub ? 1 : 0);
    }
}

void JaamWeb::handleUiSchema() {
    setCrossOrigin();

    JsonDocument doc;
    // Top-level models glossary (compact field lists)
    {
        JsonObject models = doc["models"].to<JsonObject>();
        JsonArray mD = models["dropdown"].to<JsonArray>(); // Dropdown
        // Use 'list' to refer to a named options list in top-level dropdown_lists
        mD.add("name"); mD.add("label"); mD.add("list"); mD.add("current");
        JsonArray mB = models["bool"].to<JsonArray>(); // Bool
        mB.add("name"); mB.add("label"); mB.add("current");
        JsonArray mT = models["text"].to<JsonArray>(); // Text
        mT.add("name"); mT.add("label"); mT.add("current"); mT.add("placeholder");
        JsonArray mC = models["color"].to<JsonArray>(); // Color
        mC.add("name"); mC.add("label"); mC.add("current");
        JsonArray mS = models["slider"].to<JsonArray>(); // Slider
        mS.add("name"); mS.add("label"); mS.add("min"); mS.add("max"); mS.add("step"); mS.add("current");
        JsonArray mL = models["label"].to<JsonArray>(); // Label (section header)
        mL.add("label");
        // Option item compact model spec at top-level as well
        JsonArray mO = models["option"].to<JsonArray>(); // Option item for dropdowns
        mO.add("id"); mO.add("name"); mO.add("sub");
    }

    // Top-level dropdown option lists to be referenced by name from controls
    {
        JsonObject dropdownLists = doc["dropdown_lists"].to<JsonObject>();
        {
            JsonArray arr = dropdownLists["legacy"].to<JsonArray>();
            appendOptionsList(arr, LEGACY_OPTIONS, LEGACY_OPTIONS_COUNT);
        }
        {
            JsonArray arr = dropdownLists["display_model"].to<JsonArray>();
            appendOptionsList(arr, DISPLAY_TYPES, DISPLAY_TYPES_COUNT);
        }
        {
            JsonArray arr = dropdownLists["display_height"].to<JsonArray>();
            appendOptionsList(arr, DISPLAY_HEIGHTS, DISPLAY_HEIGHT_COUNT);
        }
        {
            JsonArray arr = dropdownLists["display_rotation"].to<JsonArray>();
            appendOptionsList(arr, DISPLAY_ROTATIONS, DISPLAY_ROTATION_COUNT);
        }
        {
            JsonArray arr = dropdownLists["districts"].to<JsonArray>();
            appendOptionsList(arr, DISTRICTS, MAX_REGIONS);
        }
        {
            JsonArray arr = dropdownLists["bg_led_mode"].to<JsonArray>();
            appendOptionsList(arr, BG_LED_MODES, BG_LED_MODES_COUNT);
        }
        {
            JsonArray arr = dropdownLists["map_mode"].to<JsonArray>();
            appendOptionsList(arr, MAP_MODES, MAP_MODES_COUNT);
        }
        {
            JsonArray arr = dropdownLists["led_color_formats"].to<JsonArray>();
            appendOptionsList(arr, LED_COLOR_FORMATS, LED_COLOR_FORMATS_COUNT);
        }
        {
            JsonArray arr = dropdownLists["led_frequencies"].to<JsonArray>();
            appendOptionsList(arr, LED_FREQUENCIES, LED_FREQUENCIES_COUNT);
        }
        {
            JsonArray arr = dropdownLists["animation_types"].to<JsonArray>();
            appendOptionsList(arr, ANIMATION_TYPES, ANIMATION_TYPES_COUNT);
        }
        {
            JsonArray arr = dropdownLists["auto_brightness_modes"].to<JsonArray>();
            appendOptionsList(arr, AUTO_BRIGHTNESS_MODES, AUTO_BRIGHTNESS_OPTIONS_COUNT);
        }
    }

    JsonArray controls = doc["controls"].to<JsonArray>();

    // Helper to add a dropdown control referencing a named list and reading current from settings key
    auto addDropdown = [&](const char* name, const char* label, const char* listId, Type key){
        JsonArray c = controls.add<JsonArray>();
        c.add("dropdown"); c.add(name); c.add(label); c.add(listId); c.add(settings->getInt(key));
    };

    // Helper to add a label/section header
    auto addLabel = [&](const char* text){
        JsonArray group = controls.add<JsonArray>();
        group.add("label"); group.add(text);
    };

    // Helper to add a boolean control
    auto addBool = [&](const char* name, const char* label, Type key){
        JsonArray c = controls.add<JsonArray>();
        c.add("bool"); c.add(name); c.add(label); c.add(settings->getBool(key));
    };

    auto addText = [&](const char* name, const char* label, const String& value, const char* placeholder){
        JsonArray c = controls.add<JsonArray>();
        c.add("text"); c.add(name); c.add(label); c.add(value); c.add(placeholder);
    };

    auto addSlider = [&](const char* name, const char* label, float minv, float maxv, float step, float current){
        JsonArray c = controls.add<JsonArray>();
        c.add("slider"); c.add(name); c.add(label); c.add(minv); c.add(maxv); c.add(step); c.add(current);
    };

    auto addColor = [&](const char* name, const char* label, Type key){
        JsonArray c = controls.add<JsonArray>();
        c.add("color"); c.add(name); c.add(label); c.add(String(settings->getString(key)));
    };

    addDropdown("legacy", "Режим прошивки", "legacy", LEGACY);

    // Display settings
    addLabel("Налаштування дисплея");
    addDropdown("display_model", "Тип дисплея", "display_model", DISPLAY_MODEL);
    addDropdown("display_height", "Висота дисплея", "display_height", DISPLAY_HEIGHT);
    addDropdown("display_rotation", "Поворот дисплея", "display_rotation", DISPLAY_ROTATION);
    addBool("invert_display", "Інвертувати дисплей", INVERT_DISPLAY);
    addBool("kyiv_led", "Київ як окремий LED", KYIV_LED);
    addDropdown("home_district", "Домашній регіон", "districts", HOME_DISTRICT);
    addDropdown("bg_led_mode", "Режим фонової підствітки", "bg_led_mode", BG_LED_MODE);
    addDropdown("map_mode", "Режим мапи", "map_mode", MAP_MODE);

    // Загальні налаштування
    addLabel("Загальні налаштування");

    addText("device_name", "Назва пристрою", String(settings->getString(DEVICE_NAME)), "JAAM");
    addText("device_description", "Опис пристрою", String(settings->getString(DEVICE_DESCRIPTION)), "JAAM Informer");
    addText("broadcast_name", "Ім'я в мережі", String(settings->getString(BROADCAST_NAME)), "jaam");

    // Мережеві налаштування
    addLabel("Мережеві налаштування");
    addText("ws_server_host", "Сервер WebSocket", String(settings->getString(WS_SERVER_HOST)), "ws.jaam.net.ua");
    addText("ws_server_port", "Порт WebSocket", String(settings->getInt(WS_SERVER_PORT)), "80");
    addText("ntp_host", "NTP сервер", String(settings->getString(NTP_HOST)), "time.google.com");

    // Home Assistant
    addLabel("Home Assistant");
    addText("ha_mqtt_user", "MQTT користувач", String(settings->getString(HA_MQTT_USER)), "");
    addText("ha_mqtt_password", "MQTT пароль", String(settings->getString(HA_MQTT_PASSWORD)), "");
    addText("ha_broker_address", "Адреса брокера", String(settings->getString(HA_BROKER_ADDRESS)), "");

    // Піни LED стрічок
    addLabel("Піни LED стрічок");
    addText("main_led_pin", "Основна стрічка (пін)", String(settings->getInt(MAIN_LED_PIN)), "13");
    addDropdown("main_led_color_format", "Основна стрічка (формат кольору)", "led_color_formats", MAIN_LED_COLOR_FORMAT);
    addDropdown("main_led_frequency", "Основна стрічка (частота)", "led_frequencies", MAIN_LED_FREQUENCY);
    addText("bg_led_pin", "Фонова стрічка (пін)", String(settings->getInt(BG_LED_PIN)), "-1");
    addText("bg_led_count", "Фонова стрічка (кількість)", String(settings->getInt(BG_LED_COUNT)), "0");
    addDropdown("bg_led_color_format", "Фонова стрічка (формат кольору)", "led_color_formats", BG_LED_COLOR_FORMAT);
    addDropdown("bg_led_frequency", "Фонова стрічка (частота)", "led_frequencies", BG_LED_FREQUENCY);
    addText("service_led_pin", "Сервісна стрічка (пін)", String(settings->getInt(SERVICE_LED_PIN)), "-1");
    addDropdown("service_led_color_format", "Сервісна стрічка (формат кольору)", "led_color_formats", SERVICE_LED_COLOR_FORMAT);
    addDropdown("service_led_frequency", "Сервісна стрічка (частота)", "led_frequencies", SERVICE_LED_FREQUENCY);

    // Налаштування погоди / температури — sliders
    addLabel("Налаштування погоди");
    addSlider("weather_min_temp", "Мінімальна температура (°C)", -40, 40, 1, settings->getInt(WEATHER_MIN_TEMP));
    addSlider("weather_max_temp", "Максимальна температура (°C)", -40, 40, 1, settings->getInt(WEATHER_MAX_TEMP));

    addLabel("Налаштування темпертарури");
    addSlider("temp_correction", "Корегування температури (°C)", -10.0f, 10.0f, 0.1f, settings->getFloat(TEMP_CORRECTION));
    addSlider("hum_correction", "Корегування вологості (%)", -20.0f, 20.0f, 0.5f, settings->getFloat(HUM_CORRECTION));
    addSlider("pressure_correction", "Корегування атмосферного тиску (мм.рт.ст.)", -50.0f, 50.0f, 1.0f, settings->getFloat(PRESSURE_CORRECTION));

    // Налаштування анімацій
    addLabel("Налаштування анімацій");
    addBool("enable_sync_animations", "Синхронні анімації", ENABLE_SYNC_ANIMATIONS);
    addDropdown("alert_on_animation", "Початок тривог", "animation_types", ANIMATION_ALERT_ON_TYPE);
    addDropdown("alert_off_animation", "Відбій тривог", "animation_types", ANIMATION_ALERT_OFF_TYPE);
    addDropdown("drone_animation", "Загроза ударних БПЛА", "animation_types", ANIMATION_DRONE_TYPE);
    addDropdown("recon_drone_animation", "Розвідувальні БПЛА", "animation_types", ANIMATION_RECON_DRONE_TYPE);
    addDropdown("missile_animation", "Загроза ракет", "animation_types", ANIMATION_MISSILE_TYPE);
    addDropdown("kab_animation", "Загроза КАБ", "animation_types", ANIMATION_KAB_TYPE);
    addDropdown("ballistic_animation", "Загроза балістичних ракет", "animation_types", ANIMATION_BALLISTIC_TYPE);
    addDropdown("explosion_animation", "Вибухи", "animation_types", ANIMATION_EXPLOSION_TYPE);

    // Таймінги (секунди)
    addLabel("Налаштування таймінгів (в секундах)");
    addSlider("alert_on_time", "Початок тривог", 5, 600, 5, settings->getInt(ALERT_ON_TIME));
    addSlider("alert_off_time", "Відбій тривог", 5, 600, 5, settings->getInt(ALERT_OFF_TIME));
    addSlider("drone_time", "Загроза ударних БПЛА", 5, 600, 5, settings->getInt(DRONE_TIME));
    addSlider("recon_drone_time", "Розвідувальні БПЛА", 5, 600, 5, settings->getInt(RECON_DRONE_TIME));
    addSlider("missile_time", "Загроза ракет", 5, 600, 5, settings->getInt(MISSILE_TIME));
    addSlider("kab_time", "Загроза КАБ", 5, 600, 5, settings->getInt(KAB_TIME));
    addSlider("ballistic_time", "Загроза балістичних ракет", 5, 600, 5, settings->getInt(BALLISTIC_TIME));
    addSlider("explosion_time", "Вибухи", 5, 600, 5, settings->getInt(EXPLOSION_TIME));

    // Цикли (мс)
    addLabel("Налаштування цикла (в мілісекундах)");
    addSlider("alert_on_cycle", "Початок тривог", 300, 5000, 100, settings->getInt(ANIMATION_ALERT_ON_CYCLE_TIME));
    addSlider("alert_off_cycle", "Відбій тривог", 300, 5000, 100, settings->getInt(ANIMATION_ALERT_OFF_CYCLE_TIME));
    addSlider("drone_cycle", "Загроза ударних БПЛА", 300, 5000, 100, settings->getInt(ANIMATION_DRONE_CYCLE_TIME));
    addSlider("recon_drone_cycle", "Розвідувальні БПЛА", 300, 5000, 100, settings->getInt(ANIMATION_RECON_DRONE_CYCLE_TIME));
    addSlider("missile_cycle", "Загроза ракет", 300, 5000, 100, settings->getInt(ANIMATION_MISSILE_CYCLE_TIME));
    addSlider("kab_cycle", "Загроза КАБ", 300, 5000, 100, settings->getInt(ANIMATION_KAB_CYCLE_TIME));
    addSlider("ballistic_cycle", "Загроза балістичних ракет", 300, 5000, 100, settings->getInt(ANIMATION_BALLISTIC_CYCLE_TIME));
    addSlider("explosion_cycle", "Вибухи", 300, 5000, 100, settings->getInt(ANIMATION_EXPLOSION_CYCLE_TIME));

    // Кольори
    addLabel("Налаштування кольорів");
    addColor("color_alert", "Тривога", COLOR_ALERT);
    addColor("color_clear", "Відбій", COLOR_CLEAR);
    addColor("color_explosion", "Вибухи", COLOR_EXPLOSION);
    addColor("color_missiles", "Ракети", COLOR_MISSILES);
    addColor("color_drones", "Ударні БПЛА", COLOR_DRONES);
    addColor("color_recon_drones", "Розвідувальні БПЛА", COLOR_RECON_DRONES);
    addColor("color_kab", "КАБ", COLOR_KABS);
    addColor("color_ballistic", "Балістичні ракети", COLOR_BALLISTIC);
    addColor("color_home", "Домашній регіон", COLOR_HOME_DISTRICT);
    addColor("color_bg", "Задня підсітка", COLOR_BG);

    // Яскравість
    addLabel("Налаштування яскравості");
    addDropdown("brightness_mode", "Режим яскравості", "auto_brightness_modes", BRIGHTNESS_MODE);
    addSlider("day_start", "Початок дня", 0, 24, 1, settings->getInt(DAY_START));
    addSlider("night_start", "Початок ночі", 0, 24, 1, settings->getInt(NIGHT_START));
    addSlider("brightness", "Загальна", 0, 100, 1, settings->getInt(BRIGHTNESS));
    addSlider("brightness_day", "День", 0, 100, 1, settings->getInt(BRIGHTNESS_DAY));
    addSlider("brightness_night", "Нічь", 0, 100, 1, settings->getInt(BRIGHTNESS_NIGHT));
    addSlider("brightness_alert", "Тривога", 0, 100, 1, settings->getInt(BRIGHTNESS_ALERT));
    addSlider("brightness_clear", "Без тривоги", 0, 100, 1, settings->getInt(BRIGHTNESS_CLEAR));
    addSlider("brightness_explosion", "Вибухи", 0, 100, 1, settings->getInt(BRIGHTNESS_EXPLOSION));
    addSlider("brightness_missiles", "Крилаті та авіаційні ракети", 0, 100, 1, settings->getInt(BRIGHTNESS_MISSILES));
    addSlider("brightness_drones", "Ударні БПЛА", 0, 100, 1, settings->getInt(BRIGHTNESS_DRONES));
    addSlider("brightness_recon_drones", "Розвідувальні БПЛА", 0, 100, 1, settings->getInt(BRIGHTNESS_RECON_DRONES));
    addSlider("brightness_kabs", "КАБ", 0, 100, 1, settings->getInt(BRIGHTNESS_KABS));
    addSlider("brightness_ballistic", "Балістичні ракети", 0, 100, 1, settings->getInt(BRIGHTNESS_BALLISTIC));
    addSlider("brightness_home_district", "Домашній регіон", 0, 100, 1, settings->getInt(BRIGHTNESS_HOME_DISTRICT));
    addSlider("brightness_bg", "Фонова стрічка", 0, 100, 1, settings->getInt(BRIGHTNESS_BG));
    addSlider("brightness_service", "Сервісні діоди", 0, 100, 1, settings->getInt(BRIGHTNESS_SERVICE));
    addSlider("brightness_animation_end", "Кінцева яскравість анімацій", 0, 100, 1, settings->getInt(BRIGHTNESS_ANIMATION_END));

    // Налаштування тривог
    addLabel("Налаштування тривог");
    addBool("enable_kabs", "Загроза КАБ", ENABLE_KABS);
    addBool("enable_missiles", "Загроза крилатих та авіаційних ракет", ENABLE_MISSILES);
    addBool("enable_drones", "Загроза ударних БПЛА", ENABLE_DRONES);
    addBool("enable_recon_drones", "Розвідувальні БПЛА", ENABLE_RECON_DRONES);
    addBool("enable_ballistic", "Загроза балістичних ракет", ENABLE_BALLISTIC);
    addBool("enable_explosions", "Вибухи", ENABLE_EXPLOSIONS);
    addBool("enable_battery", "Моніторинг батареї", ENABLE_BATTERY_MONITORING);
    addText("battery_pin", "ADC пін батареї", String(settings->getInt(BATTERY_PIN)), "-1");

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void JaamWeb::handleSaveMap() {

    for (int i = 0; i < server.args(); ++i) {
        String argName = server.argName(i);
        if (argName.startsWith("region_")) {
            uint16_t region_id = argName.substring(7).toInt();
            String value = server.arg(i);
            value.trim();

            int map_idx = -1;
            for(int j = 0; j < MAX_REGIONS; ++j) {
                if (DISTRICTS[j].id == region_id) {
                    map_idx = j;
                    break;
                }
            }

            if (map_idx != -1) {
                customMap[map_idx].region_id = region_id;
                uint8_t count = 0;
                
                int last_comma = -1;
                for (int k = 0; k < value.length(); ++k) {
                    if (value.charAt(k) == ',') {
                        String num_str = value.substring(last_comma + 1, k);
                        num_str.trim();
                        if (num_str.length() > 0 && count < MAX_LEDS_PER_REGION) {
                            customMap[map_idx].led_positions[count++] = num_str.toInt();
                        }
                        last_comma = k;
                    }
                }
                String num_str = value.substring(last_comma + 1);
                num_str.trim();
                if (num_str.length() > 0 && count < MAX_LEDS_PER_REGION) {
                    customMap[map_idx].led_positions[count++] = num_str.toInt();
                }
                customMap[map_idx].led_count = count;
            }
        }
    }

    if (storage.saveCustomMap(customMap)) {
        //generateCustomRegionMap();
        needRecalculateLeds = true;
        LOG.println("[WEB] Custom map saved successfully.");
        server.sendHeader("Location", "/map-editor", true);
        server.send(303);
    } else {
        LOG.println("[WEB] Custom map saving error.");
        server.send(500, "text/plain", "Custom map error");
    }
}

void JaamWeb::handleMapEditor() {
    String html = "<!DOCTYPE html><html>";
    html += "<head>";
    html += "<title>JAAM LED Control</title>";
    html += getMeta();
    html += getStyles();
    html += getScripts();
    html += "</head><body>";

    html += "<div class='container'>";
    html += "<div class='header-container'>";
    html += "<h1>Редактор власної карти LED</h1>";
    html += "<button class='theme-toggle' onclick='toggleTheme()' title='Перемкнути тему'><svg viewBox='0 0 24 24'><path d='M12,18C11.11,18 10.26,17.8 9.5,17.46C11.56,16.06 13,13.72 13,11A6.8,6.8 0 0,0 9.5,4.54C10.26,4.2 11.11,4 12,4A8,8 0 0,1 20,12A8,8 0 0,1 12,20M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z'/></svg></button>";
    html += "</div>";
    
    // Навігація та перемикач теми
    html += "<a href='/' style='display: block; margin-bottom: 20px; color: var(--text-color); text-decoration: none;'>&larr; На головну</a>";

    html += "<form action='/save-map' method='post'>";

    storage.loadCustomMap(customMap);

    for (int i = 0; i < MAX_REGIONS; ++i) {
        if ((DISTRICTS[i].id == 0 && i > 0)) {
            continue;
        }
        String leds_str = "";
        for (int j = 0; j < MAX_REGIONS; ++j) {
            if (customMap[j].region_id == DISTRICTS[i].id) {
                for (int k = 0; k < customMap[j].led_count; ++k) {
                    leds_str += String(customMap[j].led_positions[k]);
                    if (k < customMap[j].led_count - 1) leds_str += ", ";
                }
                break;
            }
        }
        
        String displayName = DISTRICTS[i].sub ? "&nbsp;&nbsp;&nbsp;&nbsp;" + String(DISTRICTS[i].name) : "<b>" + String(DISTRICTS[i].name) + "</b>";
        
        // Використання структури та класів з головної сторінки
        html += "<div class='text-input-container'>";
        html += "<label for='region_" + String(DISTRICTS[i].id) + "'>" + displayName + "</label>";
        html += "<input type='text' id='region_" + String(DISTRICTS[i].id) + "' name='region_" + String(DISTRICTS[i].id) + "' value='" + leds_str + "' placeholder='номери LED, через кому' class='text-input'>";
        html += "</div>";
    }

    html += "<button type='submit' class='btn-submit'>Зберегти Карту</button></form></div></body></html>";
    server.send(200, "text/html", html);
}