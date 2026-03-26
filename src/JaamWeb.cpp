#include <esp_system.h>
#include <ArduinoJson.h>
#include "JaamWeb.h"
#include "JaamLed.h"
#include "JaamLogs.h"
#include "JaamUtils.h"
#include "web_assets.h"

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

extern CurrentRegionMap                 currentMap;
extern uint32_t                         bgLedColors[MAX_LEDS_STRIP_BG];
extern JaamFirmwareUpdate               fwUpdate;

// Функції для тестового відтворення та оновлення прошивки
extern void requestPlayTestMelody(int melodyId);
extern void requestPlayTestTrack(int trackId);
extern void requestFirmwareUpdate(const char* firmwareId);
extern void requestRecalculateLeds();
extern void requestAdaptColors();
extern void requestToRegenerateBgColorMap();
extern void requestWebsocketReconnect();

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

// Global parameter mapping array used by all parameter handlers and value builders
static const ParamMapping ALL_PARAM_MAPPINGS[] = {
    // General settings
    {"hardware", HARDWARE, TYPE_INT},
    {"home_district", HOME_DISTRICT, TYPE_INT},
    {"bg_led_mode", BG_LED_MODE, TYPE_INT},
    {"map_mode", MAP_MODE, TYPE_INT},
    {"min_of_silence", MIN_OF_SILENCE, TYPE_BOOL},
    {"logs_enabled", LOGS_ENABLED, TYPE_BOOL},
    {"time_zone", TIME_ZONE, TYPE_INT},
    {"device_name", DEVICE_NAME, TYPE_STRING},
    {"device_description", DEVICE_DESCRIPTION, TYPE_STRING},
    
    // Firmware
    {"new_fw_notification", NEW_FW_NOTIFICATION, TYPE_BOOL},
    {"fw_update_channel", FW_UPDATE_CHANNEL, TYPE_INT},
    {"firmware_id", UNKNOWN, TYPE_SPECIAL},
    
    // Display
    {"display_model", DISPLAY_MODEL, TYPE_INT},
    {"display_height", DISPLAY_HEIGHT, TYPE_INT},
    {"display_rotation", DISPLAY_ROTATION, TYPE_INT},
    {"invert_display", INVERT_DISPLAY, TYPE_BOOL},
    {"display_mode", DISPLAY_MODE, TYPE_INT},
    {"display_mode_time", DISPLAY_MODE_TIME, TYPE_INT},
    {"toggle_mode_microclimate", TOGGLE_MODE_MICROCLIMATE, TYPE_BOOL},
    {"display_off_at_night", DISPLAY_OFF_AT_NIGHT, TYPE_BOOL},
    {"clock_font", CLOCK_FONT, TYPE_INT},
    {"display_alert_message_time", DISPLAY_ALERT_MESSAGE_TIME, TYPE_INT},
    
    // Network
    {"broadcast_name", BROADCAST_NAME, TYPE_STRING},
    {"ws_server_host", WS_SERVER_HOST, TYPE_STRING},
    {"ws_server_port", WS_SERVER_PORT, TYPE_INT},
    {"ntp_host", NTP_HOST, TYPE_STRING},
    {"api_enabled", API_ENABLED, TYPE_BOOL},
    {"api_port", API_PORT, TYPE_INT},
    
    // Hardware
    {"main_led_pin", MAIN_LED_PIN, TYPE_INT},
    {"main_led_count", MAIN_LED_COUNT, TYPE_INT},
    {"main_led_color_format", MAIN_LED_COLOR_FORMAT, TYPE_INT},
    {"main_led_frequency", MAIN_LED_FREQUENCY, TYPE_INT},
    {"bg_led_pin", BG_LED_PIN, TYPE_INT},
    {"bg_led_count", BG_LED_COUNT, TYPE_INT},
    {"bg_led_color_format", BG_LED_COLOR_FORMAT, TYPE_INT},
    {"bg_led_frequency", BG_LED_FREQUENCY, TYPE_INT},
    {"service_led_pin", SERVICE_LED_PIN, TYPE_INT},
    {"service_led_color_format", SERVICE_LED_COLOR_FORMAT, TYPE_INT},
    {"service_led_frequency", SERVICE_LED_FREQUENCY, TYPE_INT},
    {"button_1_pin", BUTTON_1_PIN, TYPE_INT},
    {"button_1_touch", USE_TOUCH_BUTTON_1, TYPE_BOOL},
    {"button_1_mode", BUTTON_1_MODE, TYPE_INT},
    {"button_1_mode_long", BUTTON_1_MODE_LONG, TYPE_INT},
    {"button_2_pin", BUTTON_2_PIN, TYPE_INT},
    {"button_2_touch", USE_TOUCH_BUTTON_2, TYPE_BOOL},
    {"button_2_mode", BUTTON_2_MODE, TYPE_INT},
    {"button_2_mode_long", BUTTON_2_MODE_LONG, TYPE_INT},
    {"button_3_pin", BUTTON_3_PIN, TYPE_INT},
    {"button_3_touch", USE_TOUCH_BUTTON_3, TYPE_BOOL},
    {"button_3_mode", BUTTON_3_MODE, TYPE_INT},
    {"button_3_mode_long", BUTTON_3_MODE_LONG, TYPE_INT},
    {"buzzer_pin", BUZZER_PIN, TYPE_INT},
    {"df_rx_pin", DF_RX_PIN, TYPE_INT},
    {"df_tx_pin", DF_TX_PIN, TYPE_INT},
    {"enable_battery", ENABLE_BATTERY_MONITORING, TYPE_BOOL},
    {"battery_pin", BATTERY_PIN, TYPE_INT},
    {"brightness_min", BRIGHTNESS_MIN, TYPE_INT},
    {"brightness_max", BRIGHTNESS_MAX, TYPE_INT},
    {"brightness_max_accept", BRIGHTNESS_MAX_ACCEPT, TYPE_BOOL},
    
    // Siren
    {"alert_clear_pin_mode", ALERT_CLEAR_PIN_MODE, TYPE_INT},
    {"alert_pin", ALERT_PIN, TYPE_INT},
    {"clear_pin", CLEAR_PIN, TYPE_INT},
    {"alert_clear_pin_time", ALERT_CLEAR_PIN_TIME, TYPE_INT},
    {"alert_pin_active_level", ALERT_PIN_ACTIVE_LEVEL, TYPE_INT},
    {"alert_clear_pin_mode_2", ALERT_CLEAR_PIN_MODE_2, TYPE_INT},
    {"alert_pin_2", ALERT_PIN_2, TYPE_INT},
    {"clear_pin_2", CLEAR_PIN_2, TYPE_INT},
    {"alert_clear_pin_time_2", ALERT_CLEAR_PIN_TIME_2, TYPE_INT},
    {"alert_pin_active_level_2", ALERT_PIN_ACTIVE_LEVEL_2, TYPE_INT},
    
    // Climate
    {"weather_min_temp", WEATHER_MIN_TEMP, TYPE_INT},
    {"weather_max_temp", WEATHER_MAX_TEMP, TYPE_INT},
    {"temp_correction", TEMP_CORRECTION, TYPE_FLOAT},
    {"hum_correction", HUM_CORRECTION, TYPE_FLOAT},
    {"pressure_correction", PRESSURE_CORRECTION, TYPE_FLOAT},
    
    // Animations
    {"enable_sync_animations", ENABLE_SYNC_ANIMATIONS, TYPE_BOOL},
    {"enable_animation_preview", ENABLE_ANIMATION_PREVIEW, TYPE_BOOL},
    {"alert_on_animation", ANIMATION_ALERT_ON_TYPE, TYPE_INT},
    {"alert_off_animation", ANIMATION_ALERT_OFF_TYPE, TYPE_INT},
    {"drone_animation", ANIMATION_DRONE_TYPE, TYPE_INT},
    {"recon_drone_animation", ANIMATION_RECON_DRONE_TYPE, TYPE_INT},
    {"missile_animation", ANIMATION_MISSILE_TYPE, TYPE_INT},
    {"kab_animation", ANIMATION_KAB_TYPE, TYPE_INT},
    {"ballistic_animation", ANIMATION_BALLISTIC_TYPE, TYPE_INT},
    {"explosion_animation", ANIMATION_EXPLOSION_TYPE, TYPE_INT},
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
    
    // Brightness
    {"brightness_mode", BRIGHTNESS_MODE, TYPE_INT},
    {"day_start", DAY_START, TYPE_INT},
    {"night_start", NIGHT_START, TYPE_INT},
    {"night_mode_light_threshold", NIGHT_MODE_LIGHT_THRESHOLD, TYPE_INT},
    {"brightness", BRIGHTNESS, TYPE_INT},
    {"brightness_day", BRIGHTNESS_DAY, TYPE_INT},
    {"brightness_night", BRIGHTNESS_NIGHT, TYPE_INT},
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
    {"brightness_service", BRIGHTNESS_SERVICE, TYPE_INT},
    {"brightness_animation_end", BRIGHTNESS_ANIMATION_END, TYPE_INT},
    
    // Alerts
    {"enable_kabs", ENABLE_KABS, TYPE_BOOL},
    {"enable_missiles", ENABLE_MISSILES, TYPE_BOOL},
    {"enable_drones", ENABLE_DRONES, TYPE_BOOL},
    {"enable_recon_drones", ENABLE_RECON_DRONES, TYPE_BOOL},
    {"enable_ballistic", ENABLE_BALLISTIC, TYPE_BOOL},
    {"enable_explosions", ENABLE_EXPLOSIONS, TYPE_BOOL},
    
    // Sound
    {"sound_source", SOUND_SOURCE, TYPE_INT},
    {"melody_volume_day", MELODY_VOLUME_DAY, TYPE_INT},
    {"melody_volume_night", MELODY_VOLUME_NIGHT, TYPE_INT},
    {"sound_on_alert", SOUND_ON_ALERT, TYPE_BOOL},
    {"sound_on_alert_end", SOUND_ON_ALERT_END, TYPE_BOOL},
    {"sound_on_explosion", SOUND_ON_EXPLOSION, TYPE_BOOL},
    {"sound_on_drones", SOUND_ON_DRONES, TYPE_BOOL},
    {"sound_on_recon_drones", SOUND_ON_RECON_DRONES, TYPE_BOOL},
    {"sound_on_missiles", SOUND_ON_MISSILES, TYPE_BOOL},
    {"sound_on_kabs", SOUND_ON_KABS, TYPE_BOOL},
    {"sound_on_ballistic", SOUND_ON_BALLISTIC, TYPE_BOOL},
    {"sound_on_every_hour", SOUND_ON_EVERY_HOUR, TYPE_BOOL},
    {"sound_on_button_click", SOUND_ON_BUTTON_CLICK, TYPE_BOOL},
    {"sound_on_min_of_sl", SOUND_ON_MIN_OF_SL, TYPE_BOOL},
    {"mute_sound_on_night", MUTE_SOUND_ON_NIGHT, TYPE_BOOL},
    {"ignore_mute_on_alert", IGNORE_MUTE_ON_ALERT, TYPE_BOOL},
    {"melody_on_alert", MELODY_ON_ALERT, TYPE_INT},
    {"melody_on_alert_end", MELODY_ON_ALERT_END, TYPE_INT},
    {"melody_on_explosion", MELODY_ON_EXPLOSION, TYPE_INT},
    {"melody_on_drones", MELODY_ON_DRONES, TYPE_INT},
    {"melody_on_recon_drones", MELODY_ON_RECON_DRONES, TYPE_INT},
    {"melody_on_missiles", MELODY_ON_MISSILES, TYPE_INT},
    {"melody_on_kabs", MELODY_ON_KABS, TYPE_INT},
    {"melody_on_ballistic", MELODY_ON_BALLISTIC, TYPE_INT},
    
    // Legacy/unused parameters for compatibility
    {"district_mode_kyiv", DISTRICT_MODE_KYIV, TYPE_INT},
    {"district_mode_kharkiv", DISTRICT_MODE_KHARKIV, TYPE_INT},
    {"district_mode_zp", DISTRICT_MODE_ZP, TYPE_INT},
    {"kyiv_led", KYIV_LED, TYPE_BOOL},
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
    // LOG.printf("[WEB] Compressed JSON: %d -> %d bytes (%.1f%% reduction)\n", jsonLen, compressedSize, ratio);
    
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
    // LOG.printf("[WEB] Compressed HTML: %d -> %d bytes (%.1f%% reduction)\n", htmlLen, compressedSize, ratio);
    
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

    // Use global parameter mappings
    Type settingType = UNKNOWN;
    for (const auto& mapping : ALL_PARAM_MAPPINGS) {
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

    // Use global parameter mappings
    Type settingType = UNKNOWN;
    ValueType valueType = TYPE_INT;
    
    for (const auto& mapping : ALL_PARAM_MAPPINGS) {
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

    // Use global parameter mappings
    Type settingType = UNKNOWN;
    ValueType valueType = TYPE_INT;
    
    for (const auto& mapping : ALL_PARAM_MAPPINGS) {
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
    server.on("/logs-info", HTTP_GET, [this]() { this->handleLogsInfo(); });
    server.on("/logs-info", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/ui-schema/models", HTTP_GET, [this]() { this->handleUiSchemaModels(); });
    server.on("/ui-schema/models", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/ui-schema/sections", HTTP_GET, [this]() { this->handleUiSchemaSections(); });
    server.on("/ui-schema/sections", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/ui-schema/dropdown_lists", HTTP_GET, [this]() { this->handleUiSchemaDropdownLists(); });
    server.on("/ui-schema/dropdown_lists", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/ui-schema/controls", HTTP_GET, [this]() { this->handleUiSchemaControls(); });
    server.on("/ui-schema/controls", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/ui-schema/controls/values", HTTP_GET, [this]() { this->handleUiSchemaControlsValues(); });
    server.on("/ui-schema/controls/values", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
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

void JaamWeb::handleLogsInfo() {
    setCrossOrigin();
    
    // Get limit from query parameter (default 100)
    int limit = 100;
    if (server.hasArg("limit")) {
        String limitStr = server.arg("limit");
        int parsed = 0;
        if (parseStrictInt(limitStr, parsed)) {
            limit = constrain(parsed, 10, 500);
        }
    }
    
    String response = logsManager.getLogsJson(limit);
    sendCompressedJson(&server, response);
    response.clear();
}

void JaamWeb::handleMapData() {
    setCrossOrigin();
    
    JsonDocument doc;
    JsonArray regions = doc["regions"].to<JsonArray>();
    
    // Use already loaded currentMap
    for (int i = 0; i < MAX_REGIONS; ++i) {
        if ((DISTRICTS[i].id == 0 && i > 0)) {
            continue;
        }
        
        JsonObject region = regions.add<JsonObject>();
        region["id"] = DISTRICTS[i].id;
        region["name"] = DISTRICTS[i].name;
        region["sub"] = DISTRICTS[i].sub;
        
        // Find LED positions for this region in currentMap
        const RegionLedMapMeta* meta = findRegionMeta(DISTRICTS[i].id);
        if (meta && meta->led_count > 0) {
            const uint16_t* leds = getRegionLeds(meta);
            if (!leds) {
                region["leds_string"] = "";
                continue;
            }
            JsonArray ledsArray = region["leds"].to<JsonArray>();
            String leds_str = "";
            
            for (uint8_t k = 0; k < meta->led_count; ++k) {
                ledsArray.add(leds[k]);
                if (k > 0) leds_str += ", ";
                leds_str += String(leds[k]);
            }
            region["leds_string"] = leds_str;
        } else {
            region["leds_string"] = "";
        }
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
        html += "<script>window.JAAM_HASHES={uiModels:'" + String(ui_schema_models_json_hash) + "',uiSections:'" + String(ui_schema_sections_json_hash) + "',uiControls:'" + String(ui_schema_controls_json_hash) + "'};</script>\n";
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
                <button class='control-button' id='logsPanelToggle' onclick='toggleLogsPanel()' title='Показати/сховати логи'>
                    <svg viewBox='0 0 24 24'>
                        <path d='M14,2H6A2,2 0 0,0 4,4V20A2,2 0 0,0 6,22H18A2,2 0 0,0 20,20V8L14,2M18,20H6V4H13V9H18V20Z'/>
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

        <!-- Logs panel (content filled by updateLogsInfo) -->
        <div class='logs-panel' id='logsPanel'>
            <div class='logs-header'>
                <h3>Логи пристрою</h3>
                <div class='logs-buttons'>
                    <button class='logs-control-btn' id='logsToggleBtn' onclick='toggleLogStream()'>Показати</button>
                    <button class='logs-control-btn logs-clear-btn' onclick='clearLogs()' title='Очистити логи'>✕</button>
                </div>
            </div>
            <div class='logs-content' id='logsContent'>
                <div class='logs-empty'>Натисніть "Показати" для отримання логів</div>
            </div>
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

// Helper to push dropdown options to a JsonArray as compact lists: [id, name, sub, disabled]
static void appendOptionsList(JsonArray arr, const SettingListItem items[], int itemCount) {
    for (int i = 0; i < itemCount; ++i) {
        // Skip items with ignore=true unless they have showDisabled=true
        if (items[i].ignore && !items[i].showDisabled) continue;
        
        JsonArray opt = arr.add<JsonArray>();
        opt.add(items[i].id);
        opt.add(items[i].name);
        opt.add(items[i].sub ? 1 : 0);
        opt.add(items[i].showDisabled ? 1 : 0);  // disabled
    }
}

void JaamWeb::handleSaveMap() {
    if (storage == nullptr) {
        LOG.printf("[WEB] Storage is not set\n");
        server.send(500, "text/plain", "Storage not initialized");
        return;
    }

    // Спочатку збираємо дані у тимчасові структури
    struct TempRegion {
        uint16_t region_id;
        std::vector<uint16_t> leds;
    };
    std::vector<TempRegion> tempRegions;

    // Парсимо параметри з форми
    for (int i = 0; i < server.args(); ++i) {
        String argName = server.argName(i);
        if (argName.startsWith("region_")) {
            int parsedRegionId = 0;
            if (!parseStrictInt(argName.substring(7), parsedRegionId) ||
                parsedRegionId < 0 || parsedRegionId > 65535) {
                server.send(400, "text/plain", "Invalid region id");
                return;
            }
            uint16_t region_id = static_cast<uint16_t>(parsedRegionId);
            String value = server.arg(i);
            value.trim();

            if (value.length() > 0) {
                TempRegion tempRegion;
                tempRegion.region_id = region_id;
                
                // Парсимо LED позиції (comma-separated)
                int lastComma = -1;
                for (int k = 0; k < value.length(); ++k) {
                    if (value.charAt(k) == ',') {
                        String num_str = value.substring(lastComma + 1, k);
                        num_str.trim();
                        if (num_str.length() > 0) {
                            int parsedLed = 0;
                            if (!parseStrictInt(num_str, parsedLed) ||
                                parsedLed < 0 || parsedLed >= MAX_LEDS_STRIP_MAIN) {
                                server.send(400, "text/plain", "Invalid LED index");
                                return;
                            }
                            if (tempRegion.leds.size() >= MAX_LEDS_PER_REGION) {
                                server.send(400, "text/plain", "Too many LEDs in region");
                                return;
                            }
                            tempRegion.leds.push_back(static_cast<uint16_t>(parsedLed));
                        }
                        lastComma = k;
                    }
                }
                // Останнє число після останньої коми
                String num_str = value.substring(lastComma + 1);
                num_str.trim();
                if (num_str.length() > 0) {
                    int parsedLed = 0;
                    if (!parseStrictInt(num_str, parsedLed) ||
                        parsedLed < 0 || parsedLed >= MAX_LEDS_STRIP_MAIN) {
                        server.send(400, "text/plain", "Invalid LED index");
                        return;
                    }
                    if (tempRegion.leds.size() >= MAX_LEDS_PER_REGION) {
                        server.send(400, "text/plain", "Too many LEDs in region");
                        return;
                    }
                    tempRegion.leds.push_back(static_cast<uint16_t>(parsedLed));
                }
                
                if (tempRegion.leds.size() > 0) {
                    tempRegions.push_back(tempRegion);
                }
            }
        }
    }

    if (tempRegions.empty()) {
        LOG.printf("[WEB] No regions to save\n");
        server.send(400, "text/plain", "No regions to save");
        return;
    }

    // Підраховуємо загальну кількість LED
    size_t totalLeds = 0;
    for (const auto& region : tempRegions) {
        totalLeds += region.leds.size();
    }

    LOG.printf("[WEB] Saving custom map: %zu regions, %zu LEDs\n", tempRegions.size(), totalLeds);

    // Створюємо flat array структури
    RegionLedMapMeta* meta = new (std::nothrow) RegionLedMapMeta[tempRegions.size()];
    uint16_t* leds = new (std::nothrow) uint16_t[totalLeds];

    if (!meta || !leds) {
        LOG.printf("[WEB] Failed to allocate memory for custom map\n");
        if (meta) delete[] meta;
        if (leds) delete[] leds;
        server.send(500, "text/plain", "Memory allocation failed");
        return;
    }

    // Заповнюємо дані
    size_t currentLedIndex = 0;
    for (size_t i = 0; i < tempRegions.size(); ++i) {
        meta[i].region_id = tempRegions[i].region_id;
        meta[i].start_index = currentLedIndex;
        meta[i].led_count = tempRegions[i].leds.size();

        for (size_t j = 0; j < tempRegions[i].leds.size(); ++j) {
            leds[currentLedIndex++] = tempRegions[i].leds[j];
        }
    }

    // Зберігаємо
    bool success = storage->saveCurrentMap(meta, leds, tempRegions.size());
    
    delete[] meta;
    delete[] leds;

    if (success) {
        LOG.printf("[WEB] Custom map saved successfully.\n");
        server.sendHeader("Location", "/map-editor", true);
        requestRecalculateLeds();
        requestAdaptColors();
        requestWebsocketReconnect();
        server.send(303);
    } else {
        LOG.printf("[WEB] Custom map saving error.\n");
        server.send(500, "text/plain", "Failed to save custom map");
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
    html += "<button id='exportBtn' onclick='exportMap()' class='control-button' title='Експортувати карту в JSON' style='display: none;'>Експорт</button>";
    html += "<button id='importBtn' onclick='document.getElementById(\"importFileInput\").click()' class='control-button' title='Імпортувати карту з JSON' style='display: none;'>Імпорт</button>";
    html += "<input type='file' id='importFileInput' accept='.json' style='display: none;' onchange='importMapFromFile(event)'>";
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
        JsonArray arr = dropdownLists["display_mode"].to<JsonArray>();
        appendOptionsList(arr, DISPLAY_MODES, DISPLAY_MODES_COUNT);
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
        JsonArray arr = dropdownLists["firmware_versions_beta"].to<JsonArray>();
        const JaamFirmware* firmwares = fwUpdate.getFirmwaresBeta();
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
    {
        JsonArray arr = dropdownLists["firmware_versions_prod"].to<JsonArray>();
        const JaamFirmware* firmwares = fwUpdate.getFirmwaresProd();
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
    {
        JsonArray arr = dropdownLists["fw_update_channel_options"].to<JsonArray>();
        JsonArray opt0 = arr.add<JsonArray>(); opt0.add(0); opt0.add("Prod"); opt0.add(0);
        JsonArray opt1 = arr.add<JsonArray>(); opt1.add(1); opt1.add("Beta"); opt1.add(0);
    }
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
    
    // Cache for 24 hours with hash-based ETag
    String etag = "\"" + String(ui_schema_controls_json_hash) + "\"";
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
    server.setContentLength(ui_schema_controls_json_gz_len);
    server.send_P(200, "application/json", (const char*)ui_schema_controls_json_gz, ui_schema_controls_json_gz_len);
}

void JaamWeb::buildUiSchemaControlsValues(JsonDocument& doc) {
    JsonArray values = doc["values"].to<JsonArray>();

    // Iterate through global parameter mappings and add values based on type
    for (const auto& mapping : ALL_PARAM_MAPPINGS) {
        JsonArray v = values.add<JsonArray>();
        v.add(mapping.name);
        
        switch (mapping.valueType) {
            case TYPE_INT:
                v.add(settings->getInt(mapping.settingType));
                break;
            case TYPE_BOOL:
                v.add(settings->getBool(mapping.settingType));
                break;
            case TYPE_FLOAT:
                v.add(settings->getFloat(mapping.settingType));
                break;
            case TYPE_STRING:
                v.add(settings->getString(mapping.settingType));
                break;
            case TYPE_SPECIAL:
                // Handle special cases like firmware_id
                if (strcmp(mapping.name, "firmware_id") == 0) {
                    v.add(String(fwUpdate.getUpdateId()));
                }
                break;
        }
    }
}

void JaamWeb::handleUiSchemaControlsValues() {
    setCrossOrigin();
    
    // Prevent caching to ensure fresh values on every request
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    
    JsonDocument doc;
    buildUiSchemaControlsValues(doc);
    String response;
    serializeJson(doc, response);
    sendCompressedJson(&server, response);
    response.clear();
}
