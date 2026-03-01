#include "JaamWeb.h"
#include "JaamLed.h"
#include "JaamLogs.h"
#include "JaamUtils.h"
#include "web_assets.h"
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
extern volatile bool needReconfigureSound;
extern volatile bool needReconfigureSensors;
extern volatile bool needReconfigureButtons;
extern volatile bool needUpdateAnimationsMode;
extern volatile bool needAdaptClimate;
extern volatile bool needToRegenerateBgColorMap;
extern volatile bool needAdaptVolume;
extern volatile bool needUpdateHomeAlertBit;
extern volatile bool needUpdateTimezone;
extern volatile bool needPlayTestMelody;
extern volatile bool needPlayTestTrack;
extern volatile int testMelodyId;
extern volatile int testTrackId;

extern RegionLedMapEntry                customMap[MAX_REGIONS];
extern uint32_t                         bgLedColors[MAX_BG_LEDS];
extern JaamFirmwareUpdate               fwUpdate;

void sendLargeJson(WebServer* server, const String& json) {
    size_t jsonLen = json.length();
    LOG.printf("[WEB] Sending JSON, size: %d bytes\n", jsonLen);
    
    server->send(200, "application/json", json);
    if (server->client().connected()) {
        server->client().flush();
    }
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
            LOG.printf("[WEB] Setting color_missiles: raw=%s\n", valuePtr);
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
        if (name == "color_lamp") {
            settings->saveString(COLOR_LAMP, valuePtr);
            LOG.printf("[WEB] Setting color_lamp: raw=%s\n", valuePtr);
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

        if (name == "hardware") {
            settings->saveInt(HARDWARE, intValue);
            LOG.printf("[WEB] Setting hardware: %d\n", intValue);
            needRecalculateLeds = true;
            needReconnectMainStrip = true;
            needReconnectBgStrip = true;
            needReconnectServiceStrip = true;
            needReconfigureDisplay = true;
            needAdaptStripBrightness = true;
            needReconfigureButtons = true;
            needReconfigureSound = true;
            needReconfigureSensors = true;
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
            needUpdateHomeAlertBit = true;
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
        } else if (name == "night_mode_light_threshold") {
            settings->saveInt(NIGHT_MODE_LIGHT_THRESHOLD, intValue);
            LOG.printf("[WEB] Setting night_mode_light_threshold: %d\n", intValue);
            needAdaptStripBrightness = true;
        } else if (name == "brightness_min") {
            settings->saveInt(BRIGHTNESS_MIN, intValue);
            LOG.printf("[WEB] Setting brightness_min: %d\n", intValue);
            needAdaptStripBrightness = true;
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
        } else if (name == "brightness_lamp") {
            settings->saveInt(BRIGHTNESS_LAMP, intValue);
            LOG.printf("[WEB] Setting brightness_lamp: %d\n", intValue);
            needAdaptColors = true;
        } else if (name == "time_zone") {
            settings->saveInt(TIME_ZONE, intValue);
            LOG.printf("[WEB] Setting time_zone: %d\n", intValue);
            needUpdateTimezone = true;
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
        } else if (name == "display_alert_message_time") {
            settings->saveInt(DISPLAY_ALERT_MESSAGE_TIME, intValue);
            LOG.printf("[WEB] Setting display_alert_message_time: %d\n", intValue);
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
        } else if (name == "api_enabled") {
            bool boolValue = intValue != 0;
            settings->saveBool(API_ENABLED, boolValue);
            LOG.printf("[WEB] Setting api_enabled: %d\n", boolValue);
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
            LOG.printf("[WEB] Set kyiv_led: %d\n", intValue);
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
        } else if (name == "sound_source") {
            settings->saveInt(SOUND_SOURCE, intValue);
            LOG.printf("[WEB] Setting sound_source: %d\n", intValue);
            needReconfigureSound = true;
        } else if (name == "melody_on_alert") {
            settings->saveInt(MELODY_ON_ALERT, intValue);
            LOG.printf("[WEB] Setting melody_on_alert: %d\n", intValue);
            needPlayTestMelody = true;
            testMelodyId = intValue;
        } else if (name == "melody_on_alert_end") {
            settings->saveInt(MELODY_ON_ALERT_END, intValue);
            LOG.printf("[WEB] Setting melody_on_alert_end: %d\n", intValue);
            needPlayTestMelody = true;
            testMelodyId = intValue;
        } else if (name == "melody_on_explosion") {
            settings->saveInt(MELODY_ON_EXPLOSION, intValue);
            LOG.printf("[WEB] Setting melody_on_explosion: %d\n", intValue);
            needPlayTestMelody = true;
            testMelodyId = intValue; 
        } else if (name == "melody_on_drones") {
            settings->saveInt(MELODY_ON_DRONES, intValue);
            LOG.printf("[WEB] Setting melody_on_drones: %d\n", intValue);
            needPlayTestMelody = true;
            testMelodyId = intValue; 
        } else if (name == "melody_on_missiles") {
            settings->saveInt(MELODY_ON_MISSILES, intValue);
            LOG.printf("[WEB] Setting melody_on_missiles: %d\n", intValue);
            needPlayTestMelody = true;
            testMelodyId = intValue; 
        } else if (name == "melody_on_kabs") {
            settings->saveInt(MELODY_ON_KABS, intValue);
            LOG.printf("[WEB] Setting melody_on_kabs: %d\n", intValue);
            needPlayTestMelody = true;
            testMelodyId = intValue; 
        } else if (name == "melody_on_ballistic") {
            settings->saveInt(MELODY_ON_BALLISTIC, intValue);
            LOG.printf("[WEB] Setting melody_on_ballistic: %d\n", intValue);
            needPlayTestMelody = true;
            testMelodyId = intValue; 
        } else if (name == "melody_on_recon_drones") {
            settings->saveInt(MELODY_ON_RECON_DRONES, intValue);
            LOG.printf("[WEB] Setting melody_on_recon_drones: %d\n", intValue);
            needPlayTestMelody = true;
            testMelodyId = intValue; 
        } else if (name == "melody_volume_day") {
            settings->saveInt(MELODY_VOLUME_DAY, intValue);
            needAdaptVolume = true;
            LOG.printf("[WEB] Setting melody_volume_day: %d\n", intValue);
        } else if (name == "melody_volume_night") {
            settings->saveInt(MELODY_VOLUME_NIGHT, intValue);
            needAdaptVolume = true;
            LOG.printf("[WEB] Setting melody_volume_night: %d\n", intValue);
        } else if (name == "sound_on_alert") {
            bool boolValue = intValue != 0;
            settings->saveBool(SOUND_ON_ALERT, boolValue);
            LOG.printf("[WEB] Setting sound_on_alert: %d\n", boolValue);
        } else if (name == "sound_on_alert_end") {
            bool boolValue = intValue != 0;
            settings->saveBool(SOUND_ON_ALERT_END, boolValue);
            LOG.printf("[WEB] Setting sound_on_alert_end: %d\n", boolValue);
        } else if (name == "sound_on_explosion") {
            bool boolValue = intValue != 0;
            settings->saveBool(SOUND_ON_EXPLOSION, boolValue);
            LOG.printf("[WEB] Setting sound_on_explosion: %d\n", boolValue);
        } else if (name == "sound_on_drones") {
            bool boolValue = intValue != 0;
            settings->saveBool(SOUND_ON_DRONES, boolValue);
            LOG.printf("[WEB] Setting sound_on_drones: %d\n", boolValue);
        } else if (name == "sound_on_missiles") {
            bool boolValue = intValue != 0;
            settings->saveBool(SOUND_ON_MISSILES, boolValue);
            LOG.printf("[WEB] Setting sound_on_missiles: %d\n", boolValue);
        } else if (name == "sound_on_kabs") {
            bool boolValue = intValue != 0;
            settings->saveBool(SOUND_ON_KABS, boolValue);
            LOG.printf("[WEB] Setting sound_on_kabs: %d\n", boolValue);
        } else if (name == "sound_on_ballistic") {
            bool boolValue = intValue != 0;
            settings->saveBool(SOUND_ON_BALLISTIC, boolValue);
            LOG.printf("[WEB] Setting sound_on_ballistic: %d\n", boolValue);
        } else if (name == "sound_on_recon_drones") {
            bool boolValue = intValue != 0;
            settings->saveBool(SOUND_ON_RECON_DRONES, boolValue);
            LOG.printf("[WEB] Setting sound_on_recon_drones: %d\n", boolValue);
        } else if (name == "sound_on_every_hour") {
            bool boolValue = intValue != 0;
            settings->saveBool(SOUND_ON_EVERY_HOUR, boolValue);
            LOG.printf("[WEB] Setting sound_on_every_hour: %d\n", boolValue);
        } else if (name == "sound_on_button_click") {
            bool boolValue = intValue != 0;
            settings->saveBool(SOUND_ON_BUTTON_CLICK, boolValue);
            LOG.printf("[WEB] Setting sound_on_button_click: %d\n", boolValue);
        } else if (name == "mute_sound_on_night") {
            bool boolValue = intValue != 0;
            settings->saveBool(MUTE_SOUND_ON_NIGHT, boolValue);
            LOG.printf("[WEB] Setting mute_sound_on_night: %d\n", boolValue);
        } else if (name == "ignore_mute_on_alert") {
            bool boolValue = intValue != 0;
            settings->saveBool(IGNORE_MUTE_ON_ALERT, boolValue);
            LOG.printf("[WEB] Setting ignore_mute_on_alert: %d\n", boolValue);
        } else if (name == "sound_on_min_of_sl") {
            bool boolValue = intValue != 0;
            settings->saveBool(SOUND_ON_MIN_OF_SL, boolValue);
            LOG.printf("[WEB] Setting sound_on_min_of_sl: %d\n", boolValue);
        } else if (name == "button_1_touch") {
            bool boolValue = intValue != 0;
            settings->saveBool(USE_TOUCH_BUTTON_1, boolValue);
            needReconfigureButtons = true;
            LOG.printf("[WEB] Set button_1_touch: %d\n", intValue);
        } else if (name == "button_2_touch") {
            bool boolValue = intValue != 0;
            settings->saveBool(USE_TOUCH_BUTTON_2, boolValue);
            needReconfigureButtons = true;
            LOG.printf("[WEB] Set button_2_touch: %d\n", intValue);
        } else if (name == "button_3_touch") {
            bool boolValue = intValue != 0;
            settings->saveBool(USE_TOUCH_BUTTON_3, boolValue);
            needReconfigureButtons = true;
            LOG.printf("[WEB] Set button_3_touch: %d\n", intValue);
        } else if (name == "button_1_mode") {
            settings->saveInt(BUTTON_1_MODE, intValue);
            LOG.printf("[WEB] Setting button_1_mode: %d\n", intValue);
        } else if (name == "button_2_mode") {
            settings->saveInt(BUTTON_2_MODE, intValue);
            LOG.printf("[WEB] Setting button_2_mode: %d\n", intValue);
        } else if (name == "button_3_mode") {
            settings->saveInt(BUTTON_3_MODE, intValue);
            LOG.printf("[WEB] Setting button_3_mode: %d\n", intValue);
        } else if (name == "button_1_mode_long") {
            settings->saveInt(BUTTON_1_MODE_LONG, intValue);
            LOG.printf("[WEB] Setting button_1_mode_long: %d\n", intValue);
        } else if (name == "button_2_mode_long") {
            settings->saveInt(BUTTON_2_MODE_LONG, intValue);
            LOG.printf("[WEB] Setting button_2_mode_long: %d\n", intValue);
        } else if (name == "button_3_mode_long") {
            settings->saveInt(BUTTON_3_MODE_LONG, intValue);
            LOG.printf("[WEB] Setting button_3_mode_long: %d\n", intValue);
        } else if (name == "alert_clear_pin_mode") {
            settings->saveInt(ALERT_CLEAR_PIN_MODE, intValue);
            LOG.printf("[WEB] Setting alert_clear_pin_mode: %d\n", intValue);
        } else if (name == "alert_clear_pin_time") {
            settings->saveInt(ALERT_CLEAR_PIN_TIME, intValue);
            LOG.printf("[WEB] Setting alert_clear_pin_time: %d\n", intValue);
        } else if (name == "alert_pin_active_level") {
            settings->saveInt(ALERT_PIN_ACTIVE_LEVEL, intValue);
            LOG.printf("[WEB] Setting alert_pin_active_level: %d\n", intValue);
        } else if (name == "alert_clear_pin_mode_2") {
            settings->saveInt(ALERT_CLEAR_PIN_MODE_2, intValue);
            LOG.printf("[WEB] Setting alert_clear_pin_mode_2: %d\n", intValue);
        } else if (name == "alert_clear_pin_time_2") {
            settings->saveInt(ALERT_CLEAR_PIN_TIME_2, intValue);
            LOG.printf("[WEB] Setting alert_clear_pin_time_2: %d\n", intValue);
        } else if (name == "alert_pin_active_level_2") {
            settings->saveInt(ALERT_PIN_ACTIVE_LEVEL_2, intValue);
            LOG.printf("[WEB] Setting alert_pin_active_level_2: %d\n", intValue);
        } else if (name == "min_of_silence") {
            bool boolValue = intValue != 0;
            settings->saveBool(MIN_OF_SILENCE, boolValue);
            LOG.printf("[WEB] Setting min_of_silence: %d\n", boolValue);
        } else if (name == "firmware_id") {
            LOG.printf("[WEB] Setting firmware_id: %s\n", valuePtr);
            if (!fwUpdate.requestUpdate(valuePtr)) {
                LOG.printf("[WEB] Invalid firmware ID: %s\n", valuePtr);
            }
        } else if (name == "new_fw_notification") {
            bool boolValue = intValue != 0;
            settings->saveBool(NEW_FW_NOTIFICATION, boolValue);
            LOG.printf("[WEB] Setting new_fw_notification: %d\n", boolValue);
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
            LOG.printf("[WEB] Setting ws_server_port: %d\n", value.toInt());
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
        } else if (name == "main_led_count") {
            settings->saveInt(MAIN_LED_COUNT, value.toInt());
            LOG.printf("[WEB] Setting main_led_count: %s\n", valuePtr);
            needReconnectMainStrip = true;
            needRecalculateLeds = true;
        } else if (name == "bg_led_pin") {
            settings->saveInt(BG_LED_PIN, value.toInt());
            LOG.printf("[WEB] Setting bg_led_pin: %s\n", valuePtr);
            needReconnectBgStrip = true;
        } else if (name == "bg_led_count") {
            settings->saveInt(BG_LED_COUNT, value.toInt());
            LOG.printf("[WEB] Setting bg_led_count: %s\n", valuePtr);
            needReconnectBgStrip = true;
            needToRegenerateBgColorMap = true;
        } else if (name == "service_led_pin") {
            settings->saveInt(SERVICE_LED_PIN, value.toInt());
            LOG.printf("[WEB] Setting service_led_pin: %s\n", valuePtr);
            needReconnectServiceStrip = true;
        } else if (name == "battery_pin") {
            settings->saveInt(BATTERY_PIN, value.toInt());
            needUpdateBatteryPin = true;
            LOG.printf("[WEB] Set battery_pin: %d\n", value.toInt());
        } else if (name == "buzzer_pin") {
            settings->saveInt(BUZZER_PIN, value.toInt());
            needReconfigureSound = true;
            LOG.printf("[WEB] Set buzzer_pin: %d\n", value.toInt());
        } else if (name == "df_rx_pin") {
            settings->saveInt(DF_RX_PIN, value.toInt());
            needReconfigureSound = true;
            LOG.printf("[WEB] Set df_rx_pin: %d\n", value.toInt());
        } else if (name == "df_tx_pin") {
            settings->saveInt(DF_TX_PIN, value.toInt());
            needReconfigureSound = true;
            LOG.printf("[WEB] Set df_tx_pin: %d\n", value.toInt());
        } else if (name == "alert_pin") {
            int pin = value.toInt();
            // Перевіряємо конфлікт тільки з другим пристроєм
            if (pin > 0) {
                int alertPin2 = settings->getInt(ALERT_PIN_2);
                int clearPin2 = settings->getInt(CLEAR_PIN_2);
                if ((alertPin2 > 0 && pin == alertPin2) || (clearPin2 > 0 && pin == clearPin2)) {
                    LOG.printf("[WEB] alert_pin %d already used by another siren device\n", pin);
                    server.send(400, "application/json", "{\"error\":\"Пін вже використовується іншим пристроєм сирени\"}");
                    return;
                }
            }
            settings->saveInt(ALERT_PIN, pin);
            LOG.printf("[WEB] Set alert_pin: %d\n", pin);
        } else if (name == "clear_pin") {
            int pin = value.toInt();
            // Перевіряємо конфлікт тільки з другим пристроєм
            if (pin > 0) {
                int alertPin2 = settings->getInt(ALERT_PIN_2);
                int clearPin2 = settings->getInt(CLEAR_PIN_2);
                if ((alertPin2 > 0 && pin == alertPin2) || (clearPin2 > 0 && pin == clearPin2)) {
                    LOG.printf("[WEB] clear_pin %d already used by another siren device\n", pin);
                    server.send(400, "application/json", "{\"error\":\"Пін вже використовується іншим пристроєм сирени\"}");
                    return;
                }
            }
            settings->saveInt(CLEAR_PIN, pin);
            LOG.printf("[WEB] Set clear_pin: %d\n", pin);
        } else if (name == "alert_pin_2") {
            int pin = value.toInt();
            // Перевіряємо конфлікт тільки з першим пристроєм
            if (pin > 0) {
                int alertPin = settings->getInt(ALERT_PIN);
                int clearPin = settings->getInt(CLEAR_PIN);
                if ((alertPin > 0 && pin == alertPin) || (clearPin > 0 && pin == clearPin)) {
                    LOG.printf("[WEB] alert_pin_2 %d already used by another siren device\n", pin);
                    server.send(400, "application/json", "{\"error\":\"Пін вже використовується іншим пристроєм сирени\"}");
                    return;
                }
            }
            settings->saveInt(ALERT_PIN_2, pin);
            LOG.printf("[WEB] Set alert_pin_2: %d\n", pin);
        } else if (name == "clear_pin_2") {
            int pin = value.toInt();
            // Перевіряємо конфлікт тільки з першим пристроєм
            if (pin > 0) {
                int alertPin = settings->getInt(ALERT_PIN);
                int clearPin = settings->getInt(CLEAR_PIN);
                if ((alertPin > 0 && pin == alertPin) || (clearPin > 0 && pin == clearPin)) {
                    LOG.printf("[WEB] clear_pin_2 %d already used by another siren device\n", pin);
                    server.send(400, "application/json", "{\"error\":\"Пін вже використовується іншим пристроєм сирени\"}");
                    return;
                }
            }
            settings->saveInt(CLEAR_PIN_2, pin);
            LOG.printf("[WEB] Set clear_pin_2: %d\n", pin);
        } else if (name == "api_port") {
            // Перевіряємо що порт не 80 (зайнятий веб-сервером)
            int apiPort = value.toInt();
            if (apiPort == 80) {
                LOG.printf("[WEB] api_port cannot be 80 (used by web server)\n");
                server.send(400, "application/json", "{\"error\":\"Port 80 is reserved for web server\"}");
                return;
            }
            settings->saveInt(API_PORT, apiPort);
            LOG.printf("[WEB] Setting api_port: %d\n", apiPort);
        } else if (name == "button_1_pin") {
            settings->saveInt(BUTTON_1_PIN, value.toInt());
            needReconfigureButtons = true;
            LOG.printf("[WEB] Set button_1_pin: %d\n", value.toInt());
        } else if (name == "button_2_pin") {
            settings->saveInt(BUTTON_2_PIN, value.toInt());
            needReconfigureButtons = true;
            LOG.printf("[WEB] Set button_2_pin: %d\n", value.toInt());
        } else if (name == "button_3_pin") {
            settings->saveInt(BUTTON_3_PIN, value.toInt());
            needReconfigureButtons = true;
            LOG.printf("[WEB] Set button_3_pin: %d\n", value.toInt());
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
    LOG.printf("[WEB] sendCORSHeader\n");
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
    sendLargeJson(&server, response);
    response.clear();
}

void JaamWeb::handleAlertsInfo() {
    setCrossOrigin();
    String response = getAlertsJson();
    sendLargeJson(&server, response);
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
    sendLargeJson(&server, response);
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
    server.send(200, "text/html", html);
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
        needRecalculateLeds = true;
        LOG.printf("[WEB] Custom map saved successfully.\n");
        server.sendHeader("Location", "/map-editor", true);
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
    server.send(200, "text/html", html);
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
    sendLargeJson(&server, response);
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
        needToRegenerateBgColorMap = true;
        needAdaptColors = true;
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
    server.send(200, "text/html", html);
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
    addDropdownConfirm("firmware", "firmware_id", "Вибрати прошивку", "firmware_versions", String(fwUpdate.getUpdateId()));
    addBool("firmware", "new_fw_notification", "Показувати повідомлення про нову прошивку", NEW_FW_NOTIFICATION);

    // Display settings
    addInfo("display", "Налаштуйте параметри дисплея та візуального відображення мапи", "#28a745", "M4,6H20V16H4M20,18A2,2 0 0,0 22,16V6C22,4.89 21.1,4 20,4H4C2.89,4 2,4.89 2,6V16A2,2 0 0,0 4,18H10V20H8V22H16V20H14V18H20Z");
    addDropdown("display", "display_model", "Тип дисплея", "display_model", DISPLAY_MODEL, exceptJaam1And2And30.c_str());
    addDropdown("display", "display_height", "Висота дисплея", "display_height", DISPLAY_HEIGHT, exceptJaam1And2And30.c_str());
    addDropdown("display", "display_rotation", "Поворот дисплея", "display_rotation", DISPLAY_ROTATION, exceptJaam1And2And30.c_str());
    addBool("display", "invert_display", "Інвертувати дисплей", INVERT_DISPLAY);
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
    sendLargeJson(&server, response);
    response.clear();
}

void JaamWeb::handleUiSchemaControls() {
    setCrossOrigin();
    JsonDocument doc;
    buildUiSchemaControls(doc);
    String response;
    serializeJson(doc, response);
    sendLargeJson(&server, response);
    response.clear();
}
