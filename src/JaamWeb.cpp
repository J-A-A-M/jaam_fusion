#include "JaamWeb.h"
#include "JaamLed.h"
#include "JaamLogs.h"
#include "JaamUtils.h"
#include "web_assets.h"
#include <esp_system.h>
#include <ArduinoJson.h>

#define DEST_FS_USES_LITTLEFS
#include <ESP32-targz.h>

#ifdef __cplusplus
extern "C" {
#endif
// ESP32 temperature sensor function
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif

extern RegionLedMapEntry                customMap[MAX_REGIONS];
extern uint32_t                         bgLedColors[MAX_BG_LEDS];
extern JaamFirmwareUpdate               fwUpdate;

// Функції для тестового відтворення та оновлення прошивки
extern void requestPlayTestMelody(int melodyId);
extern void requestPlayTestTrack(int trackId);
extern void requestFirmwareUpdate(const char* firmwareId);
extern void requestRecalculateLeds();
extern void requestAdaptColors();
extern void requestToRegenerateBgColorMap();

// Допоміжні функції для строгого парсингу
static bool parseStrictInt(const String& s, int& out) {
    if (s.length() == 0) return false;
    char* end = nullptr;
    long v = strtol(s.c_str(), &end, 10);
    if (*end != '\0') return false;
    out = (int)v;
    return true;
}

static bool parseStrictFloat(const String& s, float& out) {
    if (s.length() == 0) return false;
    char* end = nullptr;
    float v = strtof(s.c_str(), &end);
    if (*end != '\0') return false;
    out = v;
    return true;
}

// Типи значень параметрів
enum ValueType { 
    TYPE_INT, 
    TYPE_STRING, 
    TYPE_FLOAT, 
    TYPE_BOOL, 
    TYPE_SPECIAL 
};

// Маппінг параметрів з типом значення
struct ParamMapping {
    const char* name;
    Type settingType;
    ValueType valueType;
};

void sendLargeJson(WebServer* server, const String& json) {
    size_t jsonLen = json.length();
    LOG.printf("[WEB] Sending JSON, size: %d bytes\n", jsonLen);
    
    server->send(200, "application/json", json);
    if (server->client().connected()) {
        server->client().flush();
    }
}

void sendCompressedJson(WebServer* server, const String& json) {
    size_t jsonLen = json.length();
    uint8_t* compressedBytes = nullptr;
    
    // Compress using ESP32-targz LZPacker
    size_t compressedSize = LZPacker::compress((uint8_t*)json.c_str(), jsonLen, &compressedBytes);
    
    if (compressedSize == 0 || compressedBytes == nullptr) {
        LOG.printf("[WEB] Compression failed, sending uncompressed\n");
        if (compressedBytes) free(compressedBytes);
        sendLargeJson(server, json);
        return;
    }
    
    float ratio = (1.0f - (float)compressedSize / (float)jsonLen) * 100.0f;
    LOG.printf("[WEB] Compressed JSON: %d → %d bytes (%.1f%% reduction)\n", jsonLen, compressedSize, ratio);
    
    // Send compressed data
    server->sendHeader("Content-Encoding", "gzip");
    server->setContentLength(compressedSize);
    server->send(200, "application/json", "");
    
    WiFiClient client = server->client();
    if (client && client.connected()) {
        client.write(compressedBytes, compressedSize);
        client.flush();
    }
    
    free(compressedBytes);
}

void sendCompressedHtml(WebServer* server, const String& html) {
    size_t htmlLen = html.length();
    uint8_t* compressedBytes = nullptr;
    
    // Compress using ESP32-targz LZPacker
    size_t compressedSize = LZPacker::compress((uint8_t*)html.c_str(), htmlLen, &compressedBytes);
    
    if (compressedSize == 0 || compressedBytes == nullptr) {
        LOG.printf("[WEB] HTML compression failed, sending uncompressed\n");
        if (compressedBytes) free(compressedBytes);
        server->send(200, "text/html", html);
        return;
    }
    
    float ratio = (1.0f - (float)compressedSize / (float)htmlLen) * 100.0f;
    LOG.printf("[WEB] Compressed HTML: %d → %d bytes (%.1f%% reduction)\n", htmlLen, compressedSize, ratio);
    
    // Send compressed data
    server->sendHeader("Content-Encoding", "gzip");
    server->setContentLength(compressedSize);
    server->send(200, "text/html", "");
    
    WiFiClient client = server->client();
    if (client && client.connected()) {
        client.write(compressedBytes, compressedSize);
        client.flush();
    }
    
    free(compressedBytes);
}


void JaamWeb::setSettings(JaamSettings* settings) {
    this->settings = settings;
}

void JaamWeb::setStorage(JaamStorage* storage) {
    this->storage = storage;
}

void JaamWeb::setDeviceInfo(const char* chipId, const char* fwVersion) {
    this->chipId = chipId;
    this->fwVersion = fwVersion;
}

String JaamWeb::getMeta() {
    return R"HTML(
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<meta name='mobile-web-app-capable' content='yes' />
<meta name='application-name' content='JAAM' />
<meta name='msapplication-starturl' content='/' />
<meta name='apple-mobile-web-app-capable' content='yes' />
<meta name='apple-mobile-web-app-title' content='JAAM'>
<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />
<link rel='shortcut icon' href='favicon.png'>
<link rel='apple-touch-icon' href='apple-touch-icon.png'>
)HTML";
}

void JaamWeb::handleCss() {
    // Cache for 24 hours with hash-based ETag
    String etag = "\"" + String(styles_css_hash) + "\"";
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.sendHeader("ETag", etag);
    
    // Check If-None-Match for 304 Not Modified
    if (server.hasHeader("If-None-Match")) {
        String clientEtag = server.header("If-None-Match");
        if (clientEtag == etag) {
            server.send(304);
            return;
        }
    }
    
    // Send pre-compressed gzip data
    WiFiClient client = server.client();
    if (!client || !client.connected()) {
        return;
    }
    
    server.sendHeader("Content-Encoding", "gzip");
    server.setContentLength(styles_css_gz_len);
    server.send_P(200, "text/css", (const char*)styles_css_gz, styles_css_gz_len);
}

void JaamWeb::handleJs() {
    // Cache for 24 hours with hash-based ETag
    String etag = "\"" + String(scripts_js_hash) + "\"";
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.sendHeader("ETag", etag);
    
    // Check If-None-Match for 304 Not Modified
    if (server.hasHeader("If-None-Match")) {
        String clientEtag = server.header("If-None-Match");
        if (clientEtag == etag) {
            server.send(304);
            return;
        }
    }
    
    // Send pre-compressed gzip data
    WiFiClient client = server.client();
    if (!client || !client.connected()) {
        return;
    }
    
    server.sendHeader("Content-Encoding", "gzip");
    server.setContentLength(scripts_js_gz_len);
    server.send_P(200, "application/javascript", (const char*)scripts_js_gz, scripts_js_gz_len);
}

void JaamWeb::handleMapEditorCss() {
    // Cache for 24 hours with hash-based ETag
    String etag = "\"" + String(map_editor_css_hash) + "\"";
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.sendHeader("ETag", etag);
    
    // Check If-None-Match for 304 Not Modified
    if (server.hasHeader("If-None-Match")) {
        String clientEtag = server.header("If-None-Match");
        if (clientEtag == etag) {
            server.send(304);
            return;
        }
    }
    
    // Send pre-compressed gzip data
    WiFiClient client = server.client();
    if (!client || !client.connected()) {
        return;
    }
    
    server.sendHeader("Content-Encoding", "gzip");
    server.setContentLength(map_editor_css_gz_len);
    server.send_P(200, "text/css", (const char*)map_editor_css_gz, map_editor_css_gz_len);
}

void JaamWeb::handleMapEditorJs() {
    // Cache for 24 hours with hash-based ETag
    String etag = "\"" + String(map_editor_js_hash) + "\"";
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.sendHeader("ETag", etag);
    
    // Check If-None-Match for 304 Not Modified
    if (server.hasHeader("If-None-Match")) {
        String clientEtag = server.header("If-None-Match");
        if (clientEtag == etag) {
            server.send(304);
            return;
        }
    }
    
    // Send pre-compressed gzip data
    WiFiClient client = server.client();
    if (!client || !client.connected()) {
        return;
    }
    
    server.sendHeader("Content-Encoding", "gzip");
    server.setContentLength(map_editor_js_gz_len);
    server.send_P(200, "application/javascript", (const char*)map_editor_js_gz, map_editor_js_gz_len);
}

void JaamWeb::handleBgColorEditorCss() {
    // Cache for 24 hours with hash-based ETag
    String etag = "\"" + String(bg_color_editor_css_hash) + "\"";
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.sendHeader("ETag", etag);
    
    // Check If-None-Match for 304 Not Modified
    if (server.hasHeader("If-None-Match")) {
        String clientEtag = server.header("If-None-Match");
        if (clientEtag == etag) {
            server.send(304);
            return;
        }
    }
    
    // Send pre-compressed gzip data
    WiFiClient client = server.client();
    if (!client || !client.connected()) {
        return;
    }
    
    server.sendHeader("Content-Encoding", "gzip");
    server.setContentLength(bg_color_editor_css_gz_len);
    server.send_P(200, "text/css", (const char*)bg_color_editor_css_gz, bg_color_editor_css_gz_len);
}

void JaamWeb::handleBgColorEditorJs() {
    // Cache for 24 hours with hash-based ETag
    String etag = "\"" + String(bg_color_editor_js_hash) + "\"";
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.sendHeader("ETag", etag);
    
    // Check If-None-Match for 304 Not Modified
    if (server.hasHeader("If-None-Match")) {
        String clientEtag = server.header("If-None-Match");
        if (clientEtag == etag) {
            server.send(304);
            return;
        }
    }
    
    // Send pre-compressed gzip data
    WiFiClient client = server.client();
    if (!client || !client.connected()) {
        return;
    }
    
    server.sendHeader("Content-Encoding", "gzip");
    server.setContentLength(bg_color_editor_js_gz_len);
    server.send_P(200, "application/javascript", (const char*)bg_color_editor_js_gz, bg_color_editor_js_gz_len);
}


void JaamWeb::handleColorParameter() {
    if (!server.hasArg("name") || !server.hasArg("value")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    setCrossOrigin();
    String name = server.arg("name");
    String value = server.arg("value");
    const char* valuePtr = value.c_str();

    // Маппінг назв параметрів на типи налаштувань (всі STRING)
    static const ParamMapping mappings[] = {
        {"color_alert", COLOR_ALERT, TYPE_STRING},
        {"color_clear", COLOR_CLEAR, TYPE_STRING},
        {"color_explosion", COLOR_EXPLOSION, TYPE_STRING},
        {"color_missiles", COLOR_MISSILES, TYPE_STRING},
        {"color_drones", COLOR_DRONES, TYPE_STRING},
        {"color_recon_drones", COLOR_RECON_DRONES, TYPE_STRING},
        {"color_kab", COLOR_KABS, TYPE_STRING},
        {"color_ballistic", COLOR_BALLISTIC, TYPE_STRING},
        {"color_home", COLOR_HOME_DISTRICT, TYPE_STRING},
        {"color_bg", COLOR_BG, TYPE_STRING},
        {"color_lamp", COLOR_LAMP, TYPE_STRING},
    };

    // Шукаємо відповідний параметр
    Type settingType = UNKNOWN;
    for (const auto& mapping : mappings) {
        if (name == mapping.name) {
            settingType = mapping.settingType;
            break;
        }
    }

    if (settingType == UNKNOWN) {
        LOG.printf("[WEB] Unknown color parameter: %s\n", name.c_str());
        server.send(400, "application/json", "{\"error\":\"Unknown parameter\"}");
        return;
    }

    // Валідація формату #RRGGBB
    if (value.length() != 7 || value[0] != '#') {
        LOG.printf("[WEB] Invalid color format for %s (settingType: %d): '%s' - must be #RRGGBB\n", 
                   name.c_str(), settingType, valuePtr);
        server.send(400, "application/json", "{\"error\":\"Invalid color format, expected #RRGGBB\"}");
        return;
    }
    for (int i = 1; i < 7; i++) {
        char c = value[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
            LOG.printf("[WEB] Invalid hex character in color for %s (settingType: %d): '%s' at position %d\n", 
                       name.c_str(), settingType, valuePtr, i);
            server.send(400, "application/json", "{\"error\":\"Invalid hex character in color\"}");
            return;
        }
    }

    // Зберігаємо та перевіряємо результат
    bool success = settings->saveString(settingType, valuePtr);
    LOG.printf("[WEB] Setting %s: raw=%s (success: %d)\n", name.c_str(), valuePtr, success);

    if (success) {
        server.send(200, "text/plain", "OK");
    } else {
        LOG.printf("[WEB] Failed to save color setting %s\n", name.c_str());
        server.send(400, "application/json", "{\"error\":\"Failed to save setting\"}");
    }
}

void JaamWeb::handleParameter() {
    if (!server.hasArg("name") || !server.hasArg("value")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    setCrossOrigin();
    String name = server.arg("name");
    String value = server.arg("value");
    const char* valuePtr = value.c_str();

    // Визначаємо тип параметра та його тип значення
    static const ParamMapping mappings[] = {
        {"hardware", HARDWARE, TYPE_INT},
        {"district_mode_kyiv", DISTRICT_MODE_KYIV, TYPE_INT},
        {"district_mode_kharkiv", DISTRICT_MODE_KHARKIV, TYPE_INT},
        {"district_mode_zp", DISTRICT_MODE_ZP, TYPE_INT},
        {"home_district", HOME_DISTRICT, TYPE_INT},
        {"bg_led_mode", BG_LED_MODE, TYPE_INT},
        {"map_mode", MAP_MODE, TYPE_INT},
        {"brightness", BRIGHTNESS, TYPE_INT},
        {"brightness_day", BRIGHTNESS_DAY, TYPE_INT},
        {"brightness_night", BRIGHTNESS_NIGHT, TYPE_INT},
        {"night_mode_light_threshold", NIGHT_MODE_LIGHT_THRESHOLD, TYPE_INT},
        {"brightness_min", BRIGHTNESS_MIN, TYPE_INT},
        {"brightness_alert", BRIGHTNESS_ALERT, TYPE_INT},
        {"brightness_clear", BRIGHTNESS_CLEAR, TYPE_INT},
        {"brightness_explosion", BRIGHTNESS_EXPLOSION, TYPE_INT},
        {"brightness_missiles", BRIGHTNESS_MISSILES, TYPE_INT},
        {"brightness_drones", BRIGHTNESS_DRONES, TYPE_INT},
        {"brightness_recon_drones", BRIGHTNESS_RECON_DRONES, TYPE_INT},
        {"brightness_kabs", BRIGHTNESS_KABS, TYPE_INT},
        {"brightness_ballistic", BRIGHTNESS_BALLISTIC, TYPE_INT},
        {"brightness_home_district", BRIGHTNESS_HOME_DISTRICT, TYPE_INT},
        {"brightness_bg", BRIGHTNESS_BG, TYPE_INT},
        {"brightness_lamp", BRIGHTNESS_LAMP, TYPE_INT},
        {"time_zone", TIME_ZONE, TYPE_INT},
        {"brightness_service", BRIGHTNESS_SERVICE, TYPE_INT},
        {"brightness_animation_end", BRIGHTNESS_ANIMATION_END, TYPE_INT},
        {"main_led_color_format", MAIN_LED_COLOR_FORMAT, TYPE_INT},
        {"main_led_frequency", MAIN_LED_FREQUENCY, TYPE_INT},
        {"bg_led_color_format", BG_LED_COLOR_FORMAT, TYPE_INT},
        {"bg_led_frequency", BG_LED_FREQUENCY, TYPE_INT},
        {"service_led_color_format", SERVICE_LED_COLOR_FORMAT, TYPE_INT},
        {"service_led_frequency", SERVICE_LED_FREQUENCY, TYPE_INT},
        {"display_model", DISPLAY_MODEL, TYPE_INT},
        {"display_height", DISPLAY_HEIGHT, TYPE_INT},
        {"display_rotation", DISPLAY_ROTATION, TYPE_INT},
        {"clock_font", CLOCK_FONT, TYPE_INT},
        {"invert_display", INVERT_DISPLAY, TYPE_BOOL},
        {"display_alert_message_time", DISPLAY_ALERT_MESSAGE_TIME, TYPE_INT},
        {"enable_kabs", ENABLE_KABS, TYPE_BOOL},
        {"enable_missiles", ENABLE_MISSILES, TYPE_BOOL},
        {"enable_drones", ENABLE_DRONES, TYPE_BOOL},
        {"enable_recon_drones", ENABLE_RECON_DRONES, TYPE_BOOL},
        {"enable_ballistic", ENABLE_BALLISTIC, TYPE_BOOL},
        {"enable_explosions", ENABLE_EXPLOSIONS, TYPE_BOOL},
        {"enable_sync_animations", ENABLE_SYNC_ANIMATIONS, TYPE_BOOL},
        {"api_enabled", API_ENABLED, TYPE_BOOL},
        {"brightness_mode", BRIGHTNESS_MODE, TYPE_INT},
        {"day_start", DAY_START, TYPE_INT},
        {"night_start", NIGHT_START, TYPE_INT},
        {"alert_on_time", ALERT_ON_TIME, TYPE_INT},
        {"alert_off_time", ALERT_OFF_TIME, TYPE_INT},
        {"drone_time", DRONE_TIME, TYPE_INT},
        {"recon_drone_time", RECON_DRONE_TIME, TYPE_INT},
        {"missile_time", MISSILE_TIME, TYPE_INT},
        {"kab_time", KAB_TIME, TYPE_INT},
        {"ballistic_time", BALLISTIC_TIME, TYPE_INT},
        {"explosion_time", EXPLOSION_TIME, TYPE_INT},
        {"alert_on_cycle", ANIMATION_ALERT_ON_CYCLE_TIME, TYPE_INT},
        {"alert_off_cycle", ANIMATION_ALERT_OFF_CYCLE_TIME, TYPE_INT},
        {"drone_cycle", ANIMATION_DRONE_CYCLE_TIME, TYPE_INT},
        {"recon_drone_cycle", ANIMATION_RECON_DRONE_CYCLE_TIME, TYPE_INT},
        {"missile_cycle", ANIMATION_MISSILE_CYCLE_TIME, TYPE_INT},
        {"kab_cycle", ANIMATION_KAB_CYCLE_TIME, TYPE_INT},
        {"ballistic_cycle", ANIMATION_BALLISTIC_CYCLE_TIME, TYPE_INT},
        {"explosion_cycle", ANIMATION_EXPLOSION_CYCLE_TIME, TYPE_INT},
        {"alert_on_animation", ANIMATION_ALERT_ON_TYPE, TYPE_INT},
        {"alert_off_animation", ANIMATION_ALERT_OFF_TYPE, TYPE_INT},
        {"drone_animation", ANIMATION_DRONE_TYPE, TYPE_INT},
        {"recon_drone_animation", ANIMATION_RECON_DRONE_TYPE, TYPE_INT},
        {"missile_animation", ANIMATION_MISSILE_TYPE, TYPE_INT},
        {"kab_animation", ANIMATION_KAB_TYPE, TYPE_INT},
        {"ballistic_animation", ANIMATION_BALLISTIC_TYPE, TYPE_INT},
        {"explosion_animation", ANIMATION_EXPLOSION_TYPE, TYPE_INT},
        {"enable_battery", ENABLE_BATTERY_MONITORING, TYPE_BOOL},
        {"kyiv_led", KYIV_LED, TYPE_BOOL},
        {"weather_min_temp", WEATHER_MIN_TEMP, TYPE_INT},
        {"weather_max_temp", WEATHER_MAX_TEMP, TYPE_INT},
        {"temp_correction", TEMP_CORRECTION, TYPE_FLOAT},
        {"hum_correction", HUM_CORRECTION, TYPE_FLOAT},
        {"pressure_correction", PRESSURE_CORRECTION, TYPE_FLOAT},
        {"sound_source", SOUND_SOURCE, TYPE_INT},
        {"melody_on_alert", MELODY_ON_ALERT, TYPE_INT},
        {"melody_on_alert_end", MELODY_ON_ALERT_END, TYPE_INT},
        {"melody_on_explosion", MELODY_ON_EXPLOSION, TYPE_INT},
        {"melody_on_drones", MELODY_ON_DRONES, TYPE_INT},
        {"melody_on_missiles", MELODY_ON_MISSILES, TYPE_INT},
        {"melody_on_kabs", MELODY_ON_KABS, TYPE_INT},
        {"melody_on_ballistic", MELODY_ON_BALLISTIC, TYPE_INT},
        {"melody_on_recon_drones", MELODY_ON_RECON_DRONES, TYPE_INT},
        {"melody_volume_day", MELODY_VOLUME_DAY, TYPE_INT},
        {"melody_volume_night", MELODY_VOLUME_NIGHT, TYPE_INT},
        {"sound_on_alert", SOUND_ON_ALERT, TYPE_BOOL},
        {"sound_on_alert_end", SOUND_ON_ALERT_END, TYPE_BOOL},
        {"sound_on_explosion", SOUND_ON_EXPLOSION, TYPE_BOOL},
        {"sound_on_drones", SOUND_ON_DRONES, TYPE_BOOL},
        {"sound_on_missiles", SOUND_ON_MISSILES, TYPE_BOOL},
        {"sound_on_kabs", SOUND_ON_KABS, TYPE_BOOL},
        {"sound_on_ballistic", SOUND_ON_BALLISTIC, TYPE_BOOL},
        {"sound_on_recon_drones", SOUND_ON_RECON_DRONES, TYPE_BOOL},
        {"sound_on_every_hour", SOUND_ON_EVERY_HOUR, TYPE_BOOL},
        {"sound_on_button_click", SOUND_ON_BUTTON_CLICK, TYPE_BOOL},
        {"mute_sound_on_night", MUTE_SOUND_ON_NIGHT, TYPE_BOOL},
        {"ignore_mute_on_alert", IGNORE_MUTE_ON_ALERT, TYPE_BOOL},
        {"sound_on_min_of_sl", SOUND_ON_MIN_OF_SL, TYPE_BOOL},
        {"button_1_touch", USE_TOUCH_BUTTON_1, TYPE_BOOL},
        {"button_2_touch", USE_TOUCH_BUTTON_2, TYPE_BOOL},
        {"button_3_touch", USE_TOUCH_BUTTON_3, TYPE_BOOL},
        {"button_1_mode", BUTTON_1_MODE, TYPE_INT},
        {"button_2_mode", BUTTON_2_MODE, TYPE_INT},
        {"button_3_mode", BUTTON_3_MODE, TYPE_INT},
        {"button_1_mode_long", BUTTON_1_MODE_LONG, TYPE_INT},
        {"button_2_mode_long", BUTTON_2_MODE_LONG, TYPE_INT},
        {"button_3_mode_long", BUTTON_3_MODE_LONG, TYPE_INT},
        {"alert_clear_pin_mode", ALERT_CLEAR_PIN_MODE, TYPE_INT},
        {"alert_clear_pin_time", ALERT_CLEAR_PIN_TIME, TYPE_INT},
        {"alert_pin_active_level", ALERT_PIN_ACTIVE_LEVEL, TYPE_INT},
        {"alert_clear_pin_mode_2", ALERT_CLEAR_PIN_MODE_2, TYPE_INT},
        {"alert_clear_pin_time_2", ALERT_CLEAR_PIN_TIME_2, TYPE_INT},
        {"alert_pin_active_level_2", ALERT_PIN_ACTIVE_LEVEL_2, TYPE_INT},
        {"min_of_silence", MIN_OF_SILENCE, TYPE_BOOL},
        {"new_fw_notification", NEW_FW_NOTIFICATION, TYPE_BOOL},
        {"firmware_id", UNKNOWN, TYPE_SPECIAL},
    };

    // Шукаємо відповідний параметр
    Type settingType = UNKNOWN;
    ValueType valueType = TYPE_INT;
    
    for (const auto& mapping : mappings) {
        if (name == mapping.name) {
            settingType = mapping.settingType;
            valueType = mapping.valueType;
            break;
        }
    }

    if (settingType == UNKNOWN && valueType != TYPE_SPECIAL) {
        LOG.printf("[WEB] Unknown parameter: %s\n", name.c_str());
        server.send(400, "application/json", "{\"error\":\"Unknown parameter\"}");
        return;
    }

    // Обробка спеціальних випадків
    if (valueType == TYPE_SPECIAL) {
        if (name == "firmware_id") {
            LOG.printf("[WEB] Setting firmware_id: %s\n", valuePtr);
            requestFirmwareUpdate(valuePtr);
            server.send(200, "text/plain", "OK");
            return;
        }
    }

    // Викликаємо відповідний save метод
    bool success = false;
    
    switch (valueType) {
        case TYPE_INT: {
            int intValue;
            if (!parseStrictInt(value, intValue)) {
                LOG.printf("[WEB] Invalid integer value for %s (settingType: %d): '%s'\n", 
                           name.c_str(), settingType, valuePtr);
                server.send(400, "application/json", "{\"error\":\"Invalid integer value\"}");
                return;
            }
            
            success = settings->saveInt(settingType, intValue);
            LOG.printf("[WEB] Setting %s: %d (success: %d)\n", name.c_str(), intValue, success);
            
            // Додаткові дії для мелодій
            if (success && (settingType == MELODY_ON_ALERT || settingType == MELODY_ON_ALERT_END ||
                settingType == MELODY_ON_EXPLOSION || settingType == MELODY_ON_DRONES ||
                settingType == MELODY_ON_MISSILES || settingType == MELODY_ON_KABS ||
                settingType == MELODY_ON_BALLISTIC || settingType == MELODY_ON_RECON_DRONES)) {
                requestPlayTestMelody(intValue);
            }
            break;
        }
            
        case TYPE_BOOL: {
            int intValue;
            if (!parseStrictInt(value, intValue)) {
                LOG.printf("[WEB] Invalid boolean value for %s (settingType: %d): '%s'\n", 
                           name.c_str(), settingType, valuePtr);
                server.send(400, "application/json", "{\"error\":\"Invalid boolean value\"}");
                return;
            }
            
            success = settings->saveBool(settingType, intValue != 0);
            LOG.printf("[WEB] Setting %s: %d (success: %d)\n", name.c_str(), intValue != 0, success);
            break;
        }
            
        case TYPE_FLOAT: {
            float floatValue;
            if (!parseStrictFloat(value, floatValue)) {
                LOG.printf("[WEB] Invalid float value for %s (settingType: %d): '%s'\n", 
                           name.c_str(), settingType, valuePtr);
                server.send(400, "application/json", "{\"error\":\"Invalid float value\"}");
                return;
            }
            
            success = settings->saveFloat(settingType, floatValue);
            LOG.printf("[WEB] Setting %s: %.2f (success: %d)\n", name.c_str(), floatValue, success);
            break;
        }
            
        case TYPE_STRING:
            success = settings->saveString(settingType, valuePtr);
            LOG.printf("[WEB] Setting %s: %s (success: %d)\n", name.c_str(), valuePtr, success);
            break;
            
        default:
            success = false;
            break;
    }

    if (success) {
        server.send(200, "text/plain", "OK");
    } else {
        LOG.printf("[WEB] Failed to save setting %s\n", name.c_str());
        server.send(400, "application/json", "{\"error\":\"Failed to save setting\"}");
    }
}

void JaamWeb::handleTextParameter() {
    if (!server.hasArg("name") || !server.hasArg("value")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    setCrossOrigin();
    String name = server.arg("name");
    String value = server.arg("value");
    const char* valuePtr = value.c_str();

    // Визначаємо тип параметра та його тип значення
    static const ParamMapping mappings[] = {
        {"device_name", DEVICE_NAME, TYPE_STRING},
        {"device_description", DEVICE_DESCRIPTION, TYPE_STRING},
        {"broadcast_name", BROADCAST_NAME, TYPE_STRING},
        {"ws_server_host", WS_SERVER_HOST, TYPE_STRING},
        {"ws_server_port", WS_SERVER_PORT, TYPE_INT},
        {"ntp_host", NTP_HOST, TYPE_STRING},
        {"ha_mqtt_user", HA_MQTT_USER, TYPE_STRING},
        {"ha_mqtt_password", HA_MQTT_PASSWORD, TYPE_STRING},
        {"ha_broker_address", HA_BROKER_ADDRESS, TYPE_STRING},
        {"main_led_pin", MAIN_LED_PIN, TYPE_INT},
        {"main_led_count", MAIN_LED_COUNT, TYPE_INT},
        {"bg_led_pin", BG_LED_PIN, TYPE_INT},
        {"bg_led_count", BG_LED_COUNT, TYPE_INT},
        {"service_led_pin", SERVICE_LED_PIN, TYPE_INT},
        {"battery_pin", BATTERY_PIN, TYPE_INT},
        {"buzzer_pin", BUZZER_PIN, TYPE_INT},
        {"df_rx_pin", DF_RX_PIN, TYPE_INT},
        {"df_tx_pin", DF_TX_PIN, TYPE_INT},
        {"alert_pin", ALERT_PIN, TYPE_INT},
        {"clear_pin", CLEAR_PIN, TYPE_INT},
        {"alert_pin_2", ALERT_PIN_2, TYPE_INT},
        {"clear_pin_2", CLEAR_PIN_2, TYPE_INT},
        {"api_port", API_PORT, TYPE_INT},
        {"button_1_pin", BUTTON_1_PIN, TYPE_INT},
        {"button_2_pin", BUTTON_2_PIN, TYPE_INT},
        {"button_3_pin", BUTTON_3_PIN, TYPE_INT},
    };

    //Шукаємо відповідний параметр
    Type settingType = UNKNOWN;
    ValueType valueType = TYPE_INT;
    
    for (const auto& mapping : mappings) {
        if (name == mapping.name) {
            settingType = mapping.settingType;
            valueType = mapping.valueType;
            break;
        }
    }

    if (settingType == UNKNOWN) {
        LOG.printf("[WEB] Unknown parameter: %s\n", name.c_str());
        server.send(400, "application/json", "{\"error\":\"Unknown parameter\"}");
        return;
    }

    // Викликаємо відповідний save метод (валідація викликається всередині save*)
    bool success = false;
    switch (valueType) {
        case TYPE_INT:
            success = settings->saveInt(settingType, value.toInt());
            LOG.printf("[WEB] Setting %s: %d (success: %d)\n", name.c_str(), value.toInt(), success);
            break;
        case TYPE_STRING:
            success = settings->saveString(settingType, valuePtr);
            LOG.printf("[WEB] Setting %s: %s (success: %d)\n", name.c_str(), valuePtr, success);
            break;
        case TYPE_FLOAT:
            success = settings->saveFloat(settingType, value.toFloat());
            LOG.printf("[WEB] Setting %s: %.2f (success: %d)\n", name.c_str(), value.toFloat(), success);
            break;
        case TYPE_BOOL:
            success = settings->saveBool(settingType, value.toInt() != 0);
            LOG.printf("[WEB] Setting %s: %d (success: %d)\n", name.c_str(), value.toInt() != 0, success);
            break;
    }

    if (success) {
        server.send(200, "text/plain", "OK");
    } else {
        LOG.printf("[WEB] Failed to save setting %s\n", name.c_str());
        server.send(400, "application/json", "{\"error\":\"Failed to save setting\"}");
    }
}

void JaamWeb::setCrossOrigin() {
    server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    server.sendHeader(F("Access-Control-Max-Age"), F("600"));
    server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
    server.sendHeader(F("Access-Control-Allow-Headers"), F("Content-Type,Authorization"));
}

void JaamWeb::sendCrossOriginHeader(){
    LOG.printf("[WEB] sendCORSHeader\n");
    setCrossOrigin();
    server.send(204);
}

void JaamWeb::handleNotFound() {
    LOG.printf("[WEB] Not found: %s\n", server.uri().c_str());
    server.send(404, "text/plain", "Not found");
}

void JaamWeb::setStrips(Adafruit_NeoPixel* strip_main, Adafruit_NeoPixel* strip_bg, Adafruit_NeoPixel* strip_service) {
    this->strip_main = strip_main;
    this->strip_bg = strip_bg;
    this->strip_service = strip_service;
}

void JaamWeb::begin(Adafruit_NeoPixel* strip_main, Adafruit_NeoPixel* strip_bg, Adafruit_NeoPixel* strip_service) {
    setStrips(strip_main, strip_bg, strip_service);

    // Налаштування веб-сервера
    //server.enableCORS();
    server.on("/", HTTP_GET, [this]() { this->handleUiPage(); });
    server.on("/", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/styles.css", HTTP_GET, [this]() { this->handleCss(); });
    server.on("/scripts.js", HTTP_GET, [this]() { this->handleJs(); });
    server.on("/map-editor.css", HTTP_GET, [this]() { this->handleMapEditorCss(); });
    server.on("/map-editor.js", HTTP_GET, [this]() { this->handleMapEditorJs(); });
    server.on("/bg-color-editor.css", HTTP_GET, [this]() { this->handleBgColorEditorCss(); });
    server.on("/bg-color-editor.js", HTTP_GET, [this]() { this->handleBgColorEditorJs(); });
    server.on("/map-editor", HTTP_GET, [this]() { this->handleMapEditor(); });
    server.on("/save-map", HTTP_POST, [this]() { this->handleSaveMap(); });
    server.on("/bg-color-editor", HTTP_GET, [this]() { this->handleBgColorEditor(); });
    server.on("/bg-colors-data", HTTP_GET, [this]() { this->handleBgColorsData(); });
    server.on("/bg-colors-data", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/save-bg-colors", HTTP_POST, [this]() { this->handleSaveBgColors(); });
    //server.on("/parameter", HTTP_GET, [this]() { this->handleParameter(); });
    server.on("/parameter", HTTP_POST, [this]() { this->handleParameter(); });
    server.on("/parameter", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    //server.on("/color", HTTP_GET, [this]() { this->handleColorParameter(); });
    server.on("/color", HTTP_POST, [this]() { this->handleColorParameter(); });
    server.on("/color", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    //server.on("/text", HTTP_GET, [this]() { this->handleTextParameter(); });
    server.on("/text", HTTP_POST, [this]() { this->handleTextParameter(); });
    server.on("/text", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/system-info", HTTP_GET, [this]() { this->handleSystemInfo(); });
    server.on("/system-info", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/alerts-info", HTTP_GET, [this]() { this->handleAlertsInfo(); });
    server.on("/alerts-info", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/ui-schema/models", HTTP_GET, [this]() { this->handleUiSchemaModels(); });
    server.on("/ui-schema/models", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/ui-schema/sections", HTTP_GET, [this]() { this->handleUiSchemaSections(); });
    server.on("/ui-schema/sections", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/ui-schema/dropdown_lists", HTTP_GET, [this]() { this->handleUiSchemaDropdownLists(); });
    server.on("/ui-schema/dropdown_lists", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/ui-schema/controls", HTTP_GET, [this]() { this->handleUiSchemaControls(); });
    server.on("/ui-schema/controls", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/map-data", HTTP_GET, [this]() { this->handleMapData(); });
    server.on("/map-data", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    // Dynamic UI page that renders based on /ui-schema

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
    sendCompressedJson(&server, response);
    response.clear();
}

void JaamWeb::handleAlertsInfo() {
    setCrossOrigin();
    String response = getAlertsJson();
    sendCompressedJson(&server, response);
    response.clear();
}

void JaamWeb::handleMapData() {
    setCrossOrigin();
    
    JsonDocument doc;
    JsonArray regions = doc["regions"].to<JsonArray>();
    
    // Load current custom map
    storage->loadCustomMap(customMap);
    
    for (int i = 0; i < MAX_REGIONS; ++i) {
        if ((DISTRICTS[i].id == 0 && i > 0)) {
            continue;
        }
        
        JsonObject region = regions.add<JsonObject>();
        region["id"] = DISTRICTS[i].id;
        region["name"] = DISTRICTS[i].name;
        region["sub"] = DISTRICTS[i].sub;
        
        // Find LED positions for this region
        String leds_str = "";
        for (int j = 0; j < MAX_REGIONS; ++j) {
            if (customMap[j].region_id == DISTRICTS[i].id) {
                JsonArray leds = region["leds"].to<JsonArray>();
                for (int k = 0; k < customMap[j].led_count; ++k) {
                    leds.add(customMap[j].led_positions[k]);
                    if (k > 0) leds_str += ", ";
                    leds_str += String(customMap[j].led_positions[k]);
                }
                break;
            }
        }
        region["leds_string"] = leds_str;
    }
    
    // Серіалізуємо JSON у компактному форматі
    String response;
    serializeJson(doc, response);
    sendCompressedJson(&server, response);
    response.clear();
}

void JaamWeb::handleUiPage() {
        // Prevent HTML caching to ensure new CSS/JS versions are loaded
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "0");
        
        // Serve a dynamic UI page that mirrors getHtmlTemplate design but renders from /ui-schema
        String html;
        html.reserve(12000);

        // Head start
        html += R"HTML(
<!DOCTYPE html>
<html>
<head>
<title>)HTML";
        String deviceName = settings->getString(DEVICE_NAME);
        if (deviceName.isEmpty()) deviceName = "JAAM";
        html += deviceName;
        html += R"HTML(</title>
)HTML";
        // Inject meta and link to external CSS/JS with version hashes
        html += getMeta();
        html += "<link rel=\"stylesheet\" href=\"/styles.css?v=" + String(styles_css_hash) + "\">\n";
        html += "<script>window.JAAM_HASHES={uiModels:'" + String(ui_schema_models_json_hash) + "',uiSections:'" + String(ui_schema_sections_json_hash) + "'};</script>\n";
        html += "<script src=\"/scripts.js?v=" + String(scripts_js_hash) + "\"></script>\n";

        // Body start
        html += R"HTML(
</head>
<body>
    <div class='container'>
        <div class='header-container'>
            <h1>)HTML";
        String deviceDesc = settings->getString(DEVICE_DESCRIPTION);
        if (deviceDesc.isEmpty()) deviceDesc = "JAAM LED Control";
        html += deviceDesc;
        html += R"HTML(</h1>
            <div class='header-buttons'>
                <button class='control-button' id='systemPanelToggle' onclick='toggleSystemPanel()' title='Показати/сховати системну панель'>
                    <svg viewBox='0 0 24 24'>
                        <path d='M13,9V7H11V9H13M13,17V11H11V17H13M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z'/>
                    </svg>
                </button>
                <button class='control-button' id='alertsPanelToggle' onclick='toggleAlertsPanel()' title='Показати/сховати панель тривог'>
                    <svg viewBox='0 0 24 24'>
                        <path d='M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z'/>
                    </svg>
                </button>
                <button class='control-button theme-toggle' onclick='toggleTheme()' title='Перемкнути тему'>
                    <svg viewBox='0 0 24 24'>
                        <path d='M12,18C11.11,18 10.26,17.8 9.5,17.46C11.56,16.06 13,13.72 13,11A6.8,6.8 0 0,0 9.5,4.54C10.26,4.2 11.11,4 12,4A8,8 0 0,1 20,12A8,8 0 0,1 12,20M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z'/>
                    </svg>
                </button>
            </div>
        </div>

        <!-- System panel (rendered dynamically from /system-info schema) -->
        <div class='system-panel' id='systemPanel'>
            <span class='system-loading'>Завантаження...</span>
        </div>

        <!-- Alerts panel (content filled by updateAlertsInfo) -->
        <div class='alerts-panel' id='alertsPanel'>
            <div class='alert-metric'>
                <svg class='metric-icon' viewBox='0 0 24 24'>
                    <path d='M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z'/>
                </svg>
                <span class='metric-label'>Активні тривоги:</span>
                <span class='metric-value' id='alertsContent'>
                    <span class='alerts-loading'>Завантаження...</span>
                </span>
            </div>
            <div id='alertsDetailsPanel'></div>
        </div>

        <!-- Navigation and controls -->
        <div id='uiControls'></div>
    </div>
</body>
</html>
)HTML";
    sendCompressedHtml(&server, html);
    html.clear(); // Звільнення пам'яті
    // Перевіряємо, чи клієнт ще підключений перед flush
    WiFiClient client = server.client();
    if (client && client.connected()) {
        client.flush();
    }
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

void JaamWeb::handleSaveMap() {
    if (storage == nullptr) {
        LOG.printf("[WEB] Storage is not set\n");
        server.send(500, "text/plain", "Storage not initialized");
        return;
    }

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

    if (storage->saveCustomMap(customMap)) {
        //generateCustomRegionMap();
        LOG.printf("[WEB] Custom map saved successfully.\n");
        server.sendHeader("Location", "/map-editor", true);
        requestRecalculateLeds();
        requestAdaptColors();
        server.send(303);
    } else {
        LOG.printf("[WEB] Custom map saving error.\n");
        server.send(500, "text/plain", "Custom map error");
    }
}

void JaamWeb::handleMapEditor() {
    // Prevent HTML caching to ensure new CSS/JS versions are loaded
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    
    String html = "<!DOCTYPE html><html>";
    html += "<head>";
    String deviceName = settings->getString(DEVICE_NAME);
    if (deviceName.isEmpty()) deviceName = "JAAM";
    html += "<title>" + deviceName + "</title>";
    html += getMeta();
    html += "<link rel=\"stylesheet\" href=\"/styles.css?v=" + String(styles_css_hash) + "\">";
    html += "<script src=\"/scripts.js?v=" + String(scripts_js_hash) + "\"></script>";
    html += "<link rel=\"stylesheet\" href=\"/map-editor.css?v=" + String(map_editor_css_hash) + "\">";
    html += "<script src=\"/map-editor.js?v=" + String(map_editor_js_hash) + "\"></script>";
    
    html += "</head><body>";

    html += "<div class='container'>";
    html += "<div class='header-container'>";
    html += "<h1>Редактор власної карти LED</h1>";
    html += "<div class='header-buttons'>";
    html += "<button id='saveBtn' onclick='saveMap()' disabled class='control-button' title='Зберегти карту'>Завантаження...</button>";
    html += "<button class='control-button theme-toggle' onclick='toggleTheme()' title='Перемкнути тему'>";
    html += "<svg viewBox='0 0 24 24'>";
    html += "<path d='M12,18C11.11,18 10.26,17.8 9.5,17.46C11.56,16.06 13,13.72 13,11A6.8,6.8 0 0,0 9.5,4.54C10.26,4.2 11.11,4 12,4A8,8 0 0,1 20,12A8,8 0 0,1 12,20M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z'/>";
    html += "</svg>";
    html += "</button>";
    html += "</div>";
    html += "</div>";
    html += "</div>";
    
    // Container for dynamically loaded map content
    html += "<div id='mapContent'>";
    html += "<div style='text-align: center; padding: 20px; color: var(--secondary-text);'>Завантаження даних карти...</div>";
    html += "</div>";
    
    html += "</div></body></html>";
    sendCompressedHtml(&server, html);
    html.clear(); // Звільнення пам'яті
    WiFiClient client = server.client();
    if (client && client.connected()) {
        client.flush();
    }
}

void JaamWeb::handleBgColorsData() {
    setCrossOrigin();
    
    JsonDocument doc;
    JsonObject data = doc.to<JsonObject>();
    
    int bgLedCount = settings->getInt(BG_LED_COUNT);
    data["count"] = bgLedCount;
    
    JsonArray colors = data["colors"].to<JsonArray>();
    
    if (bgLedCount > 0) {
        // Використовуємо глобальну структуру bgLedColors
        for (int i = 0; i < bgLedCount; ++i) {
            JsonObject color = colors.add<JsonObject>();
            color["led"] = i;
            
            uint32_t ledColor = getBgLedColor(i);
            String colorHex = String(ledColor, HEX);
            while (colorHex.length() < 6) colorHex = "0" + colorHex;
            color["color"] = colorHex;
        }
    }
    
    // Серіалізуємо JSON у компактному форматі
    String response;
    serializeJson(doc, response);
    sendCompressedJson(&server, response);
    response.clear();
}

void JaamWeb::handleSaveBgColors() {
    LOG.printf("[WEB] Handling save BG colors request\n");
    if (storage == nullptr) {
        LOG.printf("[WEB] Storage is not set\n");
        server.send(500, "text/plain", "Storage not initialized");
        return;
    }
    
    int bgLedCount = settings->getInt(BG_LED_COUNT);
    if (bgLedCount <= 0) {
        server.send(400, "text/plain", "BG LED count not configured");
        return;
    }
    
    uint32_t* colors = new uint32_t[bgLedCount];
    
    // Ініціалізуємо чорним кольором
    for (int i = 0; i < bgLedCount; ++i) {
        colors[i] = 0x000000;
    }
    
    // Парсимо кольори з POST даних
    for (int i = 0; i < bgLedCount; ++i) {
        String paramName = "color_" + String(i);
        if (server.hasArg(paramName)) {
            String colorStr = server.arg(paramName);
            // Видаляємо символ '#' якщо є
            if (colorStr.startsWith("#")) {
                colorStr = String(colorStr.c_str() + 1); // Skip first character
            }
            colors[i] = strtol(colorStr.c_str(), nullptr, 16);
            LOG.printf("[WEB] Setting LED %d color: %s -> 0x%06X\n", i, colorStr.c_str(), colors[i]);
        }
    }
    
    if (storage->saveBgLedColors(colors, bgLedCount)) {
        LOG.printf("[WEB] BG LED colors saved successfully and global structure updated.\n");
        requestToRegenerateBgColorMap();
        requestAdaptColors();
        server.sendHeader("Location", "/bg-color-editor", true);
        server.send(303);
    } else {
        LOG.printf("[WEB] BG LED colors saving error.\n");
        server.send(500, "text/plain", "BG LED colors save error");
    }
    
    delete[] colors;
}

void JaamWeb::handleBgColorEditor() {
    // Prevent HTML caching to ensure new CSS/JS versions are loaded
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    
    String html = "<!DOCTYPE html><html>";
    html += "<head>";
    String deviceName = settings->getString(DEVICE_NAME);
    if (deviceName.isEmpty()) deviceName = "JAAM";
    html += "<title>" + deviceName + "</title>";
    html += getMeta();
    html += "<link rel=\"stylesheet\" href=\"/styles.css?v=" + String(styles_css_hash) + "\">";
    html += "<script src=\"/scripts.js?v=" + String(scripts_js_hash) + "\"></script>";
    html += "<link rel=\"stylesheet\" href=\"/bg-color-editor.css?v=" + String(bg_color_editor_css_hash) + "\">";
    html += "<script src=\"/bg-color-editor.js?v=" + String(bg_color_editor_js_hash) + "\"></script>";
    
    html += "</head><body>";

    html += "<div class='container'>";
    html += "<div class='header-container'>";
    html += "<h1>Редактор кольорів задніх LED</h1>";
    html += "<div class='header-buttons'>";
    html += "<button id='saveBtn' onclick='saveColors()' disabled class='control-button' title='Зберегти кольори'>Завантаження...</button>";
    html += "<button class='control-button theme-toggle' onclick='toggleTheme()' title='Перемкнути тему'>";
    html += "<svg viewBox='0 0 24 24'>";
    html += "<path d='M12,18C11.11,18 10.26,17.8 9.5,17.46C11.56,16.06 13,13.72 13,11A6.8,6.8 0 0,0 9.5,4.54C10.26,4.2 11.11,4 12,4A8,8 0 0,1 20,12A8,8 0 0,1 12,20M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z'/>";
    html += "</svg>";
    html += "</button>";
    html += "</div>";
    html += "</div>";
    html += "</div>";
    
    // Container for dynamically loaded color content
    html += "<div id='colorContent'>";
    html += "</div></body></html>";
    sendCompressedHtml(&server, html);
    html.clear(); // Звільнення пам'яті
    WiFiClient client = server.client();
    if (client && client.connected()) {
        client.flush();
    }
}

void JaamWeb::buildUiSchemaDropdownLists(JsonDocument& doc) {
    JsonObject dropdownLists = doc["dropdown_lists"].to<JsonObject>();
    {
        JsonArray arr = dropdownLists["hardware"].to<JsonArray>();
        appendOptionsList(arr, HARDWARE_OPTIONS, HARDWARE_OPTIONS_COUNT);
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
        JsonArray arr = dropdownLists["clock_font"].to<JsonArray>();
        appendOptionsList(arr, CLOCK_FONTS, CLOCK_FONTS_COUNT);
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
    {
        JsonArray arr = dropdownLists["sound_sources"].to<JsonArray>();
        appendOptionsList(arr, SOUND_SOURCES, SOUND_SOURCES_COUNT);
    }
    {
        JsonArray arr = dropdownLists["melodies"].to<JsonArray>();
        appendOptionsList(arr, MELODY_NAMES, MELODIES_COUNT);
    }
    {
        JsonArray arr = dropdownLists["button_modes_single_click"].to<JsonArray>();
        appendOptionsList(arr, SINGLE_CLICKS, SINGLE_CLICKS_COUNT);
    }
    {
        JsonArray arr = dropdownLists["button_modes_long_click"].to<JsonArray>();
        appendOptionsList(arr, LONG_CLICKS, LONG_CLICKS_COUNT);
    }
    {
        JsonArray arr = dropdownLists["alert_clear_pin_modes"].to<JsonArray>();
        appendOptionsList(arr, ALERT_CLEAR_PIN_MODES, ALERT_CLEAR_PIN_MODES_COUNT);
    }
    {
        JsonArray arr = dropdownLists["pin_levels"].to<JsonArray>();
        appendOptionsList(arr, PIN_LEVELS, PIN_LEVELS_COUNT);
    }
    {
        JsonArray arr = dropdownLists["timezones"].to<JsonArray>();
        appendOptionsList(arr, TIMEZONES, TIMEZONES_COUNT);
    }
    {
        JsonArray arr = dropdownLists["firmware_versions"].to<JsonArray>();
        const JaamFirmware* firmwares = fwUpdate.getFirmwares();
        for (int i = 0; i < 10; ++i) {
            if ((firmwares[i].major | firmwares[i].minor | firmwares[i].patch | firmwares[i].beta) == 0) continue;

            char buffer[32];
            if (firmwares[i].patch > 0) {
                if (firmwares[i].beta > 0) {
                     snprintf(buffer, sizeof(buffer), "%d.%d.%d-b%d", firmwares[i].major, firmwares[i].minor, firmwares[i].patch, firmwares[i].beta);
                } else {
                     snprintf(buffer, sizeof(buffer), "%d.%d.%d", firmwares[i].major, firmwares[i].minor, firmwares[i].patch);
                }
            } else {
                if (firmwares[i].beta > 0) {
                     snprintf(buffer, sizeof(buffer), "%d.%d-b%d", firmwares[i].major, firmwares[i].minor, firmwares[i].beta);
                } else {
                     snprintf(buffer, sizeof(buffer), "%d.%d", firmwares[i].major, firmwares[i].minor);
                }
            }

            JsonArray option = arr.add<JsonArray>();
            option.add(buffer); // ID (version string)
            option.add(buffer); // Display Name
            option.add(0);      // Sub (not used)
        }
    }
}

void JaamWeb::buildUiSchemaControls(JsonDocument& doc) {
    JsonArray controls = doc["controls"].to<JsonArray>();

    // Universal helper to build visibility condition
    auto buildVisibilityCondition = [](const char* fieldName, const char* operand, const uint8_t* values, size_t count) -> String {
        String result = "";
        for (size_t i = 0; i < count; i++) {
            if (i > 0) result += ",";
            result += fieldName;
            result += operand;
            result += static_cast<int>(values[i]);
        }
        return result;
    };

    // Predefined visibility conditions for common scenarios
    uint8_t hideForJaamHardware[] = {JAAM_1_3, JAAM_2_1, JAAM_3_0, JAAM_3_2};
    String exceptJaamHardware = buildVisibilityCondition("hardware", "!=", hideForJaamHardware, 4);
    
    uint8_t hideDisplaySettingsForJaam[] = {JAAM_1_3, JAAM_2_1, JAAM_3_0};
    String exceptJaam1And2And30 = buildVisibilityCondition("hardware", "!=", hideDisplaySettingsForJaam, 3);

    uint8_t hideADCSettingsForJaam[] = {JAAM_1_3, JAAM_2_1, JAAM_3_2};
    String exceptJaam1And2And32 = buildVisibilityCondition("hardware", "!=", hideADCSettingsForJaam, 3);

    uint8_t hideBuzzerSettings[] = {JAAM_2_1, JAAM_3_0, JAAM_3_2};
    String exceptJaam2And30And32 = buildVisibilityCondition("hardware", "!=", hideBuzzerSettings, 3);

    uint8_t hideDfPlayerSettings[] = {JAAM_3_0, JAAM_3_2};
    String exceptJaam30And32 = buildVisibilityCondition("hardware", "!=", hideDfPlayerSettings, 2);
    
    uint8_t hideButton2Settings[] = {JAAM_1_3};
    String exceptJaam1 = buildVisibilityCondition("hardware", "!=", hideButton2Settings, 1);
    
    uint8_t hideButton3Settings[] = {JAAM_1_3, JAAM_2_1};
    String exceptJaam1And2 = buildVisibilityCondition("hardware", "!=", hideButton3Settings, 2);
    
    uint8_t showMapEditorForCustom[] = {CUSTOM_MAPPING};
    String customOnly = buildVisibilityCondition("hardware", "==", showMapEditorForCustom, 1);
    
    uint8_t showColorEditorForIndividual[] = {2};
    String individualOnly = buildVisibilityCondition("bg_led_mode", "==", showColorEditorForIndividual, 1);
    
    uint8_t showApiPortWhenEnabled[] = {1};
    String apiEnabledOnly = buildVisibilityCondition("api_enabled", "==", showApiPortWhenEnabled, 1);

    uint8_t showLightSensorSettings[] = {2};
    String lightSensorModeOnly = buildVisibilityCondition("brightness_mode", "==", showLightSensorSettings, 1);

    uint8_t showDayNightSettings[] = {1};
    String dayNightModeOnly = buildVisibilityCondition("brightness_mode", "==", showDayNightSettings, 1);

    uint8_t showDayNightAndLightSensorSettings[] = {0};
    String dayNightAndLightSensorMode = buildVisibilityCondition("brightness_mode", "!=", showDayNightAndLightSensorSettings, 1);
    
    uint8_t showForPulseMode[] = {1};
    String pulseModeOnly = buildVisibilityCondition("alert_clear_pin_mode", "==", showForPulseMode, 1);

    uint8_t showForBiStableMode[] = {0};
    String biStableModeOnly = buildVisibilityCondition("alert_clear_pin_mode", "==", showForBiStableMode, 1);
    
    uint8_t showForPulseMode2[] = {1};
    String pulseModeOnly2 = buildVisibilityCondition("alert_clear_pin_mode_2", "==", showForPulseMode2, 1);

    uint8_t showForBiStableMode2[] = {0};
    String biStableModeOnly2 = buildVisibilityCondition("alert_clear_pin_mode_2", "==", showForBiStableMode2, 1);

    // Helper lambdas
    auto addDropdown = [&](const char* section, const char* name, const char* label, const char* listId, Type key, const char* visibility = nullptr){
        JsonArray c = controls.add<JsonArray>();
        c.add("dropdown"); c.add(name); c.add(label); c.add(listId); c.add(settings->getInt(key)); c.add(section); c.add(visibility == nullptr ? "" : visibility);
    };

    auto addDropdownConfirm = [&](const char* section, const char* name, const char* label, const char* listId, const String& current, const char* visibility = nullptr){
        JsonArray c = controls.add<JsonArray>();
        c.add("dropdown_confirm"); c.add(name); c.add(label); c.add(listId); c.add(current); c.add(section); c.add(visibility == nullptr ? "" : visibility);
    };

    auto addLabel = [&](const char* section, const char* text, const char* visibility = nullptr){
        JsonArray group = controls.add<JsonArray>();
        group.add("label"); group.add(text); group.add(section); group.add(visibility == nullptr ? "" : visibility);
    };

    auto addBool = [&](const char* section, const char* name, const char* label, Type key, const char* visibility = nullptr){
        JsonArray c = controls.add<JsonArray>();
        c.add("bool"); c.add(name); c.add(label); c.add(settings->getBool(key)); c.add(section); c.add(visibility == nullptr ? "" : visibility);
    };

    auto addText = [&](const char* section, const char* name, const char* label, const String& value, const char* placeholder, const char* visibility = nullptr){
        JsonArray c = controls.add<JsonArray>();
        c.add("text"); c.add(name); c.add(label); c.add(value); c.add(placeholder); c.add(section); c.add(visibility == nullptr ? "" : visibility);
    };

    auto addSlider = [&](const char* section, const char* name, const char* label, float minv, float maxv, float step, float current, const char* visibility = nullptr){
        JsonArray c = controls.add<JsonArray>();
        c.add("slider"); c.add(name); c.add(label); c.add(minv); c.add(maxv); c.add(step); c.add(current); c.add(section); c.add(visibility == nullptr ? "" : visibility);
    };

    auto addColor = [&](const char* section, const char* name, const char* label, Type key, const char* visibility = nullptr){
        JsonArray c = controls.add<JsonArray>();
        c.add("color"); c.add(name); c.add(label); c.add(String(settings->getString(key))); c.add(section); c.add(visibility == nullptr ? "" : visibility);
    };

    auto addInfo = [&](const char* section, const char* text, const char* color, const char* icon, const char* visibility = nullptr){
        JsonArray c = controls.add<JsonArray>();
        c.add("info"); c.add(text); c.add(color); c.add(icon); c.add(section); c.add(visibility == nullptr ? "" : visibility);
    };

    auto addButton = [&](const char* section, const char* name, const char* text, const char* bg_color, const char* uri, const char* visibility = nullptr){
        JsonArray c = controls.add<JsonArray>();
        c.add("button"); c.add(name); c.add(text); c.add(bg_color); c.add(uri); c.add(section); c.add(visibility == nullptr ? "" : visibility);
    };

    auto addInfoSuccess = [&](const char* section, const char* text, const char* visibility = nullptr){
        addInfo(section, text, "#28a745", "M21,7L9,19L3.5,13.5L4.91,12.09L9,16.17L19.59,5.59L21,7Z", visibility);
    };
    auto addInfoWarning = [&](const char* section, const char* text, const char* visibility = nullptr){
        addInfo(section, text, "#ffc107", "M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z", visibility);
    };
    auto addInfoError = [&](const char* section, const char* text, const char* visibility = nullptr){
        addInfo(section, text, "#dc3545", "M13,14H11V10H13M13,18H11V16H13M12,2C6.47,2 2,6.5 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z", visibility);
    };
    auto addInfoTips = [&](const char* section, const char* text, const char* visibility = nullptr){
        addInfo(section, text, "#17a2b8", "M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2M13,17H11V11H13V17M13,9H11V7H13V9Z", visibility);
    };

    // Загальні налаштування
    addInfo("general", "Оберіть режим прошивки відповідно до вашої версії пристрою", "#007bff", "M13,9H11V7H13M13,17H11V11H13M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z");  
    addDropdown("general", "hardware", "Режим прошивки", "hardware", HARDWARE);
    addButton("general", "map_editor", "Редактор мапи", "#007bff", "/map-editor", customOnly.c_str());
    addDropdown("general", "home_district", "Домашній регіон", "districts", HOME_DISTRICT);
    addDropdown("general", "bg_led_mode", "Режим фонової підствітки", "bg_led_mode", BG_LED_MODE, exceptJaam1.c_str());
    addButton("general", "color_editor", "Редактор кольорів", "#28a745", "/bg-color-editor", individualOnly.c_str());
    addDropdown("general", "map_mode", "Режим мапи", "map_mode", MAP_MODE);
    addBool("general", "min_of_silence", "Увімкнути режим \"Хвилина мовчання\" о 9:00", MIN_OF_SILENCE);
    addDropdown("general", "time_zone", "Часовий пояс", "timezones", TIME_ZONE);
    addText("general", "device_name", "Назва пристрою", String(settings->getString(DEVICE_NAME)), "JAAM");
    addText("general", "device_description", "Опис пристрою", String(settings->getString(DEVICE_DESCRIPTION)), "JAAM Informer");

    // Firmware update
    addInfo("firmware", "Налаштування оновлення прошивки. Вибір версії потребує підтвердження", "#9c1cf8", "M9,16.2L4.8,12L3.4,13.4L9,19L21,7L19.6,5.6L9,16.2Z");
    addBool("firmware", "new_fw_notification", "Показувати повідомлення про нову прошивку", NEW_FW_NOTIFICATION);
    addDropdownConfirm("firmware", "firmware_id", "Вибрати прошивку", "firmware_versions", String(fwUpdate.getUpdateId()));

    // Display settings
    addInfo("display", "Налаштуйте параметри дисплея та візуального відображення мапи", "#28a745", "M4,6H20V16H4M20,18A2,2 0 0,0 22,16V6C22,4.89 21.1,4 20,4H4C2.89,4 2,4.89 2,6V16A2,2 0 0,0 4,18H10V20H8V22H16V20H14V18H20Z");
    addDropdown("display", "display_model", "Тип дисплея", "display_model", DISPLAY_MODEL, exceptJaam1And2And30.c_str());
    addDropdown("display", "display_height", "Висота дисплея", "display_height", DISPLAY_HEIGHT, exceptJaam1And2And30.c_str());
    addDropdown("display", "display_rotation", "Поворот дисплея", "display_rotation", DISPLAY_ROTATION, exceptJaam1And2And30.c_str());
    addBool("display", "invert_display", "Інвертувати дисплей", INVERT_DISPLAY);
    addDropdown("display", "clock_font", "Шрифт годинника", "clock_font", CLOCK_FONT);
    addSlider("display", "display_alert_message_time", "Час сповіщень на екрані (секунди)", 1, 60, 1, settings->getInt(DISPLAY_ALERT_MESSAGE_TIME));

    // Мережеві налаштування
    addInfo("network", "Налаштуйте підключення до серверів та мережевих сервісів", "#17a2b8", "M17,3A2,2 0 0,1 19,5V15A2,2 0 0,1 17,17H13V19H14A1,1 0 0,1 15,20H22V22H15A1,1 0 0,1 14,21H10A1,1 0 0,1 9,22H2V20H9A1,1 0 0,1 10,19H11V17H7C5.89,17 5,16.1 5,15V5A2,2 0 0,1 7,3H17Z");
    addText("network", "broadcast_name", "Ім'я в мережі", String(settings->getString(BROADCAST_NAME)), "jaam");
    addText("network", "ws_server_host", "Сервер WebSocket", String(settings->getString(WS_SERVER_HOST)), "ws.jaam.net.ua");
    addText("network", "ws_server_port", "Порт WebSocket", String(settings->getInt(WS_SERVER_PORT)), "80");
    addText("network", "ntp_host", "NTP сервер", String(settings->getString(NTP_HOST)), "time.google.com");

    // Home Assistant
    addLabel("network", "Home Assistant");
    addBool("network", "api_enabled", "Увімкнути API (WebSocket)", API_ENABLED);
    addText("network", "api_port", "Порт API (WebSocket)", String(settings->getInt(API_PORT)), "81", apiEnabledOnly.c_str());
    addInfoWarning("network", "Увага: Порт 80 зарезервований для веб-сервера. Використовуйте інший порт (наприклад, 81).", apiEnabledOnly.c_str());

    // Піни та апаратні налаштування
    addInfo("hardware", "Конфігурація апаратних пінів та параметрів LED стрічок", "#6f42c1", "M9,7H11V17H9V19H15V17H13V7H15V5H9V7M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z");
    addText("hardware", "main_led_pin", "Основна стрічка (пін)", String(settings->getInt(MAIN_LED_PIN)), "13", exceptJaamHardware.c_str());
    addText("hardware", "main_led_count", "Основна стрічка (кількість)", String(settings->getInt(MAIN_LED_COUNT)), "26", customOnly.c_str());
    addDropdown("hardware", "main_led_color_format", "Основна стрічка (формат кольору)", "led_color_formats", MAIN_LED_COLOR_FORMAT, exceptJaamHardware.c_str());
    addDropdown("hardware", "main_led_frequency", "Основна стрічка (частота)", "led_frequencies", MAIN_LED_FREQUENCY);
    addText("hardware", "bg_led_pin", "Фонова стрічка (пін)", String(settings->getInt(BG_LED_PIN)), "-1", exceptJaamHardware.c_str());
    addText("hardware", "bg_led_count", "Фонова стрічка (кількість)", String(settings->getInt(BG_LED_COUNT)), "0", exceptJaamHardware.c_str());
    addDropdown("hardware", "bg_led_color_format", "Фонова стрічка (формат кольору)", "led_color_formats", BG_LED_COLOR_FORMAT, exceptJaamHardware.c_str());
    addDropdown("hardware", "bg_led_frequency", "Фонова стрічка (частота)", "led_frequencies", BG_LED_FREQUENCY, exceptJaam1.c_str());
    addText("hardware", "service_led_pin", "Сервісна стрічка (пін)", String(settings->getInt(SERVICE_LED_PIN)), "-1", exceptJaamHardware.c_str());
    addDropdown("hardware", "service_led_color_format", "Сервісна стрічка (формат кольору)", "led_color_formats", SERVICE_LED_COLOR_FORMAT, exceptJaamHardware.c_str());
    addDropdown("hardware", "service_led_frequency", "Сервісна стрічка (частота)", "led_frequencies", SERVICE_LED_FREQUENCY, exceptJaam1.c_str());
    addInfoError("hardware", "Увага: неправильна конфігурація пінів може призвести до пошкодження пристрою!");
    addLabel("hardware", "Кнопки");
    addText("hardware", "button_1_pin", "Пін кнопки 1", String(settings->getInt(BUTTON_1_PIN)), "-1", exceptJaamHardware.c_str());
    addBool("hardware", "button_1_touch", "Підтримка touch-кнопки TTP223 для кнопки 1", USE_TOUCH_BUTTON_1, exceptJaamHardware.c_str());
    addDropdown("hardware", "button_1_mode", "Режим кнопки 1 (Single Click)", "button_modes_single_click", BUTTON_1_MODE);
    addDropdown("hardware", "button_1_mode_long", "Режим кнопки 1 (Long Click)", "button_modes_long_click", BUTTON_1_MODE_LONG);
    addText("hardware", "button_2_pin", "Пін кнопки 2", String(settings->getInt(BUTTON_2_PIN)), "-1", exceptJaamHardware.c_str());
    addBool("hardware", "button_2_touch", "Підтримка touch-кнопки TTP223 для кнопки 2", USE_TOUCH_BUTTON_2, exceptJaamHardware.c_str());
    addDropdown("hardware", "button_2_mode", "Режим кнопки 2 (Single Click)", "button_modes_single_click", BUTTON_2_MODE, exceptJaam1.c_str());
    addDropdown("hardware", "button_2_mode_long", "Режим кнопки 2 (Long Click)", "button_modes_long_click", BUTTON_2_MODE_LONG, exceptJaam1.c_str());
    addText("hardware", "button_3_pin", "Пін кнопки 3", String(settings->getInt(BUTTON_3_PIN)), "-1", exceptJaamHardware.c_str());
    addBool("hardware", "button_3_touch", "Підтримка touch-кнопки TTP223 для кнопки 3", USE_TOUCH_BUTTON_3, exceptJaamHardware.c_str());
    addDropdown("hardware", "button_3_mode", "Режим кнопки 3 (Single Click)", "button_modes_single_click", BUTTON_3_MODE, exceptJaam1And2.c_str());
    addDropdown("hardware", "button_3_mode_long", "Режим кнопки 3 (Long Click)", "button_modes_long_click", BUTTON_3_MODE_LONG, exceptJaam1And2.c_str());
    addText("hardware", "buzzer_pin", "Буззер (пін)", String(settings->getInt(BUZZER_PIN)), "-1", exceptJaam2And30And32.c_str());
    addText("hardware", "df_rx_pin", "DF Player (RX) (пін)", String(settings->getInt(DF_RX_PIN)), "-1", exceptJaam30And32.c_str());
    addText("hardware", "df_tx_pin", "DF Player (TX) (пін)", String(settings->getInt(DF_TX_PIN)), "-1", exceptJaam30And32.c_str());
    addLabel("hardware", "Батарея", exceptJaam1And2And32.c_str());
    addBool("hardware", "enable_battery", "Моніторинг батареї", ENABLE_BATTERY_MONITORING, exceptJaam1And2And32.c_str());
    addText("hardware", "battery_pin", "ADC пін батареї", String(settings->getInt(BATTERY_PIN)), "-1", exceptJaam1And2And32.c_str());
    addSlider("hardware", "brightness_min", "Мінімальна яскравість", 0, 15, 1, settings->getInt(BRIGHTNESS_MIN), exceptJaamHardware.c_str());
    addInfoWarning("hardware", "Цей рівень вважатиметься нульовим значенням яскравості LED", exceptJaamHardware.c_str());

    // Налаштування сирени
    addInfo("siren", "У цьому розділі можна налаштувати поведінку пінів тривоги та відбою, а також режим і час їх активації. Можна використовувати для підключення сирен, замків, реле або інших пристроїв, які повинні реагувати на тривоги у домашньому регіоні. Заборонено використовувати один і той самий пін для різних пристроїв", "#e83e8c", "M12,2A10,10 0 0,1 22,12A10,10 0 0,1 12,22A10,10 0 0,1 2,12A10,10 0 0,1 12,2M12,4A8,8 0 0,0 4,12A8,8 0 0,0 12,20A8,8 0 0,0 20,12A8,8 0 0,0 12,4M11,17H13V11H11V17M11,9H13V7H11V9Z");
    addLabel("siren", "Пристрій 1");
    addDropdown("siren", "alert_clear_pin_mode", "Режим роботи", "alert_clear_pin_modes", ALERT_CLEAR_PIN_MODE);
    addText("siren", "alert_pin", "Пін тривоги", String(settings->getInt(ALERT_PIN)), "-1");
    addText("siren", "clear_pin", "Пін відбою", String(settings->getInt(CLEAR_PIN)), "-1", pulseModeOnly.c_str());
    addSlider("siren", "alert_clear_pin_time", "Час активації (мс)", 100, 10000, 100, settings->getInt(ALERT_CLEAR_PIN_TIME), pulseModeOnly.c_str());
    addDropdown("siren", "alert_pin_active_level", "Активний рівень", "pin_levels", ALERT_PIN_ACTIVE_LEVEL);
    addInfoTips("siren", "Для \"Бістабільного режиму\" пін тривоги буде перемикатись в активний стан під час тривоги у домашньому регіоні.  \"Активний стан\" визначається відповідним налаштуванням.", biStableModeOnly.c_str());
    addInfoTips("siren", "Для \"Імпульсного режиму\", пін тривоги та пін відбою будуть активними протягом вказаного часу після активації. \"Активний стан\" визначається відповідним налаштуванням. Можна використовувати один і той самий пін на тривогу та на відбій", pulseModeOnly.c_str());
    
    addLabel("siren", "Пристрій 2");
    addDropdown("siren", "alert_clear_pin_mode_2", "Режим роботи", "alert_clear_pin_modes", ALERT_CLEAR_PIN_MODE_2);
    addText("siren", "alert_pin_2", "Пін тривоги", String(settings->getInt(ALERT_PIN_2)), "-1");
    addText("siren", "clear_pin_2", "Пін відбою", String(settings->getInt(CLEAR_PIN_2)), "-1", pulseModeOnly2.c_str());
    addSlider("siren", "alert_clear_pin_time_2", "Час активації (мс)", 100, 10000, 100, settings->getInt(ALERT_CLEAR_PIN_TIME_2), pulseModeOnly2.c_str());
    addDropdown("siren", "alert_pin_active_level_2", "Активний рівень", "pin_levels", ALERT_PIN_ACTIVE_LEVEL_2);
    addInfoTips("siren", "Для \"Бістабільного режиму\" пін тривоги буде перемикатись в активний стан під час тривоги у домашньому регіоні.  \"Активний стан\" визначається відповідним налаштуванням.", biStableModeOnly2.c_str());
    addInfoTips("siren", "Для \"Імпульсного режиму\", пін тривоги та пін відбою будуть активними протягом вказаного часу після активації. \"Активний стан\" визначається відповідним налаштуванням. Можна використовувати один і той самий пін на тривогу та на відбій", pulseModeOnly2.c_str());

    // Налаштування погоди / температури
    addInfo("climate", "Налаштування погодних переметрів та кліматичних сенсорів", "#34f396", "M12,2A10,10 0 0,1 22,12A10,10 0 0,1 12,22A10,10 0 0,1 2,12A10,10 0 0,1 12,2M12,4A8,8 0 0,0 4,12A8,8 0 0,0 12,20A8,8 0 0,0 20,12A8,8 0 0,0 12,4M11,17H13V11H11V17M11,9H13V7H11V9Z");
    addLabel("climate", "Налаштування погоди");
    addSlider("climate", "weather_min_temp", "Мінімальна температура (°C)", -40, 40, 1, settings->getInt(WEATHER_MIN_TEMP));
    addSlider("climate", "weather_max_temp", "Максимальна температура (°C)", -40, 40, 1, settings->getInt(WEATHER_MAX_TEMP));

    addLabel("climate", "Налаштування температури");
    addSlider("climate", "temp_correction", "Корегування температури (°C)", -10.0f, 10.0f, 0.1f, settings->getFloat(TEMP_CORRECTION));
    addSlider("climate", "hum_correction", "Корегування вологості (%)", -20.0f, 20.0f, 0.5f, settings->getFloat(HUM_CORRECTION));
    addSlider("climate", "pressure_correction", "Корегування атмосферного тиску (мм.рт.ст.)", -50.0f, 50.0f, 1.0f, settings->getFloat(PRESSURE_CORRECTION));

    // Налаштування анімацій
    addInfo("animations", "Оберіть типи і налаштування анімацій для різних видів тривог та подій", "#fd7e14", "M12,2A10,10 0 0,1 22,12A10,10 0 0,1 12,22A10,10 0 0,1 2,12A10,10 0 0,1 12,2M12,4A8,8 0 0,0 4,12A8,8 0 0,0 12,20A8,8 0 0,0 20,12A8,8 0 0,0 12,4M11,17H13V11H11V17M11,9H13V7H11V9Z");
    addBool("animations", "enable_sync_animations", "Синхронні анімації", ENABLE_SYNC_ANIMATIONS);
    addDropdown("animations", "alert_on_animation", "Початок тривог", "animation_types", ANIMATION_ALERT_ON_TYPE);
    addDropdown("animations", "alert_off_animation", "Відбій тривог", "animation_types", ANIMATION_ALERT_OFF_TYPE);
    addDropdown("animations", "drone_animation", "Загроза ударних БПЛА", "animation_types", ANIMATION_DRONE_TYPE);
    addDropdown("animations", "recon_drone_animation", "Розвідувальні БПЛА", "animation_types", ANIMATION_RECON_DRONE_TYPE);
    addDropdown("animations", "missile_animation", "Загроза ракет", "animation_types", ANIMATION_MISSILE_TYPE);
    addDropdown("animations", "kab_animation", "Загроза КАБ", "animation_types", ANIMATION_KAB_TYPE);
    addDropdown("animations", "ballistic_animation", "Загроза балістичних ракет", "animation_types", ANIMATION_BALLISTIC_TYPE);
    addDropdown("animations", "explosion_animation", "Вибухи", "animation_types", ANIMATION_EXPLOSION_TYPE);

    // Таймінги (секунди)
    addLabel("animations", "Налаштування таймінгів (в секундах)");
    addInfoWarning("animations", "Увага: занадто малі значення таймінгів можуть призвести до частого миготіння");
    addSlider("animations", "alert_on_time", "Початок тривог", 5, 600, 5, settings->getInt(ALERT_ON_TIME));
    addSlider("animations", "alert_off_time", "Відбій тривог", 5, 600, 5, settings->getInt(ALERT_OFF_TIME));
    addSlider("animations", "drone_time", "Загроза ударних БПЛА", 5, 600, 5, settings->getInt(DRONE_TIME));
    addSlider("animations", "recon_drone_time", "Розвідувальні БПЛА", 5, 600, 5, settings->getInt(RECON_DRONE_TIME));
    addSlider("animations", "missile_time", "Загроза ракет", 5, 600, 5, settings->getInt(MISSILE_TIME));
    addSlider("animations", "kab_time", "Загроза КАБ", 5, 600, 5, settings->getInt(KAB_TIME));
    addSlider("animations", "ballistic_time", "Загроза балістичних ракет", 5, 600, 5, settings->getInt(BALLISTIC_TIME));
    addSlider("animations", "explosion_time", "Вибухи", 5, 600, 5, settings->getInt(EXPLOSION_TIME));

    // Цикли (мс)
    addLabel("animations", "Налаштування цикла (в мілісекундах)");
    addSlider("animations", "alert_on_cycle", "Початок тривог", 300, 5000, 100, settings->getInt(ANIMATION_ALERT_ON_CYCLE_TIME));
    addSlider("animations", "alert_off_cycle", "Відбій тривог", 300, 5000, 100, settings->getInt(ANIMATION_ALERT_OFF_CYCLE_TIME));
    addSlider("animations", "drone_cycle", "Загроза ударних БПЛА", 300, 5000, 100, settings->getInt(ANIMATION_DRONE_CYCLE_TIME));
    addSlider("animations", "recon_drone_cycle", "Розвідувальні БПЛА", 300, 5000, 100, settings->getInt(ANIMATION_RECON_DRONE_CYCLE_TIME));
    addSlider("animations", "missile_cycle", "Загроза ракет", 300, 5000, 100, settings->getInt(ANIMATION_MISSILE_CYCLE_TIME));
    addSlider("animations", "kab_cycle", "Загроза КАБ", 300, 5000, 100, settings->getInt(ANIMATION_KAB_CYCLE_TIME));
    addSlider("animations", "ballistic_cycle", "Загроза балістичних ракет", 300, 5000, 100, settings->getInt(ANIMATION_BALLISTIC_CYCLE_TIME));
    addSlider("animations", "explosion_cycle", "Вибухи", 300, 5000, 100, settings->getInt(ANIMATION_EXPLOSION_CYCLE_TIME));

    // Кольори
    addLabel("animations", "Налаштування кольорів");
    addInfo("animations", "Налаштуйте кольорову схему для різних типів тривог та елементів", "#dc3545", "M17.5,12A1.5,1.5 0 0,1 16,10.5A1.5,1.5 0 0,1 17.5,9A1.5,1.5 0 0,1 19,10.5A1.5,1.5 0 0,1 17.5,12M9,11A3,3 0 0,1 12,8A3,3 0 0,1 15,11A3,3 0 0,1 12,14A3,3 0 0,1 9,11M12,2A10,10 0 0,1 22,12A10,10 0 0,1 12,22C6.47,22 2,17.5 2,12A10,10 0 0,1 12,2Z");
    addColor("animations", "color_alert", "Тривога", COLOR_ALERT);
    addColor("animations", "color_clear", "Відбій", COLOR_CLEAR);
    addColor("animations", "color_explosion", "Вибухи", COLOR_EXPLOSION);
    addColor("animations", "color_missiles", "Ракети", COLOR_MISSILES);
    addColor("animations", "color_drones", "Ударні БПЛА", COLOR_DRONES);
    addColor("animations", "color_recon_drones", "Розвідувальні БПЛА", COLOR_RECON_DRONES);
    addColor("animations", "color_kab", "КАБ", COLOR_KABS);
    addColor("animations", "color_ballistic", "Балістичні ракети", COLOR_BALLISTIC);
    addColor("animations", "color_home", "Домашній регіон", COLOR_HOME_DISTRICT);
    addColor("animations", "color_bg", "Задня підсвітка", COLOR_BG);
    addColor("animations", "color_lamp", "Режим лампи", COLOR_LAMP);

    // Яскравість
    addInfo("brightness", "Контроль яскравості LED стрічок для різних режимів та часу доби", "#ffc107", "M12,8A4,4 0 0,0 8,12A4,4 0 0,0 12,16A4,4 0 0,0 16,12A4,4 0 0,0 12,8M12,18A6,6 0 0,1 6,12A6,6 0 0,1 12,6A6,6 0 0,1 18,12A6,6 0 0,1 12,18M20,8.69V4H15.31L12,0.69L8.69,4H4V8.69L0.69,12L4,15.31V20H8.69L12,23.31L15.31,20H20V15.31L23.31,12L20,8.69Z");
    addDropdown("brightness", "brightness_mode", "Режим яскравості", "auto_brightness_modes", BRIGHTNESS_MODE);
    addSlider("brightness", "day_start", "Початок дня", 0, 24, 1, settings->getInt(DAY_START), dayNightModeOnly.c_str());
    addSlider("brightness", "night_start", "Початок ночі", 0, 24, 1, settings->getInt(NIGHT_START), dayNightModeOnly.c_str());
    addInfoTips("brightness", "Поточний рівень освітленості можна побачити у системній панелі", lightSensorModeOnly.c_str());
    addSlider("brightness", "night_mode_light_threshold", "Освітленість переходу в нічний режим (lx)", 0, 500, 5, settings->getInt(NIGHT_MODE_LIGHT_THRESHOLD), lightSensorModeOnly.c_str());
    addSlider("brightness", "brightness", "Загальна", 0, 100, 1, settings->getInt(BRIGHTNESS));
    addSlider("brightness", "brightness_day", "День", 0, 100, 1, settings->getInt(BRIGHTNESS_DAY), dayNightAndLightSensorMode.c_str());
    addSlider("brightness", "brightness_night", "Ніч", 0, 100, 1, settings->getInt(BRIGHTNESS_NIGHT), dayNightAndLightSensorMode.c_str());
    addSlider("brightness", "brightness_alert", "Тривога", 0, 100, 1, settings->getInt(BRIGHTNESS_ALERT));
    addSlider("brightness", "brightness_clear", "Без тривоги", 0, 100, 1, settings->getInt(BRIGHTNESS_CLEAR));
    addSlider("brightness", "brightness_explosion", "Вибухи", 0, 100, 1, settings->getInt(BRIGHTNESS_EXPLOSION));
    addSlider("brightness", "brightness_missiles", "Крилаті та авіаційні ракети", 0, 100, 1, settings->getInt(BRIGHTNESS_MISSILES));
    addSlider("brightness", "brightness_drones", "Ударні БПЛА", 0, 100, 1, settings->getInt(BRIGHTNESS_DRONES));
    addSlider("brightness", "brightness_recon_drones", "Розвідувальні БПЛА", 0, 100, 1, settings->getInt(BRIGHTNESS_RECON_DRONES));
    addSlider("brightness", "brightness_kabs", "КАБ", 0, 100, 1, settings->getInt(BRIGHTNESS_KABS));
    addSlider("brightness", "brightness_ballistic", "Балістичні ракети", 0, 100, 1, settings->getInt(BRIGHTNESS_BALLISTIC));
    addSlider("brightness", "brightness_home_district", "Домашній регіон", 0, 100, 1, settings->getInt(BRIGHTNESS_HOME_DISTRICT));
    addSlider("brightness", "brightness_bg", "Фонова стрічка", 0, 100, 1, settings->getInt(BRIGHTNESS_BG));
    addSlider("brightness", "brightness_lamp", "Режим лампи", 0, 100, 1, settings->getInt(BRIGHTNESS_LAMP));
    addSlider("brightness", "brightness_service", "Сервісні діоди", 0, 100, 1, settings->getInt(BRIGHTNESS_SERVICE));
    addSlider("brightness", "brightness_animation_end", "Кінцева яскравість анімацій", 0, 100, 1, settings->getInt(BRIGHTNESS_ANIMATION_END));

    // Налаштування тривог
    addInfo("alerts", "Увімкніть або вимкніть відображення різних типів тривог", "#6c757d", "M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z");
    addBool("alerts", "enable_kabs", "Загроза КАБ", ENABLE_KABS);
    addBool("alerts", "enable_missiles", "Загроза крилатих та авіаційних ракет", ENABLE_MISSILES);
    addBool("alerts", "enable_drones", "Загроза ударних БПЛА", ENABLE_DRONES);
    addBool("alerts", "enable_recon_drones", "Розвідувальні БПЛА", ENABLE_RECON_DRONES);
    addBool("alerts", "enable_ballistic", "Загроза балістичних ракет", ENABLE_BALLISTIC);
    addBool("alerts", "enable_explosions", "Вибухи", ENABLE_EXPLOSIONS);

    // Налаштування звуку
    addInfo("sound", "Налаштування звуку", "#dc3545", "M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z");
    addDropdown("sound", "sound_source", "Джерело звуку", "sound_sources", SOUND_SOURCE);
    addSlider("sound", "melody_volume_day", "Гучність мелодії вдень", 0, 100, 1, settings->getInt(MELODY_VOLUME_DAY));
    addSlider("sound", "melody_volume_night", "Гучність мелодії вночі", 0, 100, 1, settings->getInt(MELODY_VOLUME_NIGHT));
    addBool("sound", "sound_on_alert", "Звукове сповіщення при тривозі у домашньому регіоні", SOUND_ON_ALERT);
    addBool("sound", "sound_on_alert_end", "Звукове сповіщення при завершенні тривоги у домашньому регіоні", SOUND_ON_ALERT_END);
    addBool("sound", "sound_on_explosion", "Звукове сповіщення при вибухах", SOUND_ON_EXPLOSION);
    addBool("sound", "sound_on_drones", "Звукове сповіщення при загрозі ударних БПЛА", SOUND_ON_DRONES);
    addBool("sound", "sound_on_recon_drones", "Звукове сповіщення при розвідувальних БПЛА", SOUND_ON_RECON_DRONES);
    addBool("sound", "sound_on_missiles", "Звукове сповіщення при загрозі ракет", SOUND_ON_MISSILES);
    addBool("sound", "sound_on_kabs", "Звукове сповіщення при загрозі КАБ", SOUND_ON_KABS);
    addBool("sound", "sound_on_ballistic", "Звукове сповіщення при загрозі балістики", SOUND_ON_BALLISTIC);
    addBool("sound", "sound_on_every_hour", "Звукове сповіщення щогодини", SOUND_ON_EVERY_HOUR);
    addBool("sound", "sound_on_button_click", "Сигнали при натисканні кнопки", SOUND_ON_BUTTON_CLICK);
    addBool("sound", "sound_on_min_of_sl", "Відтворювати звуки під час \"Хвилини мовчання\"", SOUND_ON_MIN_OF_SL);
    addBool("sound", "mute_sound_on_night", "Вимикати всі звуки у нічний час", MUTE_SOUND_ON_NIGHT);
    addBool("sound", "ignore_mute_on_alert", "Сигнали тривоги навіть у нічний час", IGNORE_MUTE_ON_ALERT);
    addDropdown("sound", "melody_on_alert", "Мелодія при тривозі у домашньому регіоні (буззер)", "melodies", MELODY_ON_ALERT);
    addDropdown("sound", "melody_on_alert_end", "Мелодія при скасуванні тривоги у домашньому регіоні (буззер)", "melodies", MELODY_ON_ALERT_END);
    addDropdown("sound", "melody_on_explosion", "Мелодія при вибухах (буззер)", "melodies", MELODY_ON_EXPLOSION);
    addDropdown("sound", "melody_on_drones", "Мелодія при загрозі ударних БПЛА (буззер)", "melodies", MELODY_ON_DRONES);
    addDropdown("sound", "melody_on_recon_drones", "Мелодія при розвідувальних БПЛА (буззер)", "melodies", MELODY_ON_RECON_DRONES);
    addDropdown("sound", "melody_on_missiles", "Мелодія при загрозі ракет (буззер)", "melodies", MELODY_ON_MISSILES);
    addDropdown("sound", "melody_on_kabs", "Мелодія при загрозі КАБ (буззер)", "melodies", MELODY_ON_KABS);
    addDropdown("sound", "melody_on_ballistic", "Мелодія при загрозі балістики (буззер)", "melodies", MELODY_ON_BALLISTIC);
}

void JaamWeb::handleUiSchemaModels() {
    setCrossOrigin();
    
    // Cache for 24 hours with hash-based ETag
    String etag = "\"" + String(ui_schema_models_json_hash) + "\"";
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.sendHeader("ETag", etag);
    
    // Check If-None-Match for 304 response
    if (server.hasHeader("If-None-Match") && server.header("If-None-Match") == etag) {
        server.send(304);
        return;
    }
    
    WiFiClient client = server.client();
    if (!client || !client.connected()) {
        return;
    }
    
    server.sendHeader("Content-Encoding", "gzip");
    server.setContentLength(ui_schema_models_json_gz_len);
    server.send_P(200, "application/json", (const char*)ui_schema_models_json_gz, ui_schema_models_json_gz_len);
}

void JaamWeb::handleUiSchemaSections() {
    setCrossOrigin();
    
    // Cache for 24 hours with hash-based ETag
    String etag = "\"" + String(ui_schema_sections_json_hash) + "\"";
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.sendHeader("ETag", etag);
    
    // Check If-None-Match for 304 response
    if (server.hasHeader("If-None-Match") && server.header("If-None-Match") == etag) {
        server.send(304);
        return;
    }
    
    WiFiClient client = server.client();
    if (!client || !client.connected()) {
        return;
    }
    
    server.sendHeader("Content-Encoding", "gzip");
    server.setContentLength(ui_schema_sections_json_gz_len);
    server.send_P(200, "application/json", (const char*)ui_schema_sections_json_gz, ui_schema_sections_json_gz_len);
}

void JaamWeb::handleUiSchemaDropdownLists() {
    setCrossOrigin();
    JsonDocument doc;
    buildUiSchemaDropdownLists(doc);
    String response;
    serializeJson(doc, response);
    sendCompressedJson(&server, response);
    response.clear();
}

void JaamWeb::handleUiSchemaControls() {
    setCrossOrigin();
    JsonDocument doc;
    buildUiSchemaControls(doc);
    String response;
    serializeJson(doc, response);
    sendCompressedJson(&server, response);
    response.clear();
}
