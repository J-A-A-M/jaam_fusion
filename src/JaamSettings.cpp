#include <Preferences.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "JaamSettings.h"
#include "JaamUtils.h"

JaamSettings::JaamSettings() : changeCallback(nullptr) {
}

const char* PF_STRING = "S";
const char* PF_INT = "I";
const char* PF_FLOAT = "F";

// Settings that should NOT be reset (preserved during reset)
static const int PRESERVED_SETTINGS_COUNT = 2;
static const char* PRESERVED_SETTINGS[PRESERVED_SETTINGS_COUNT] = {
    "id",       // ID - device ID
    "legacy"    // HARDWARE - hardware type
};

struct SettingItemInt {
    const char* key;
    int value;
};

struct SettingItemString {
    const char* key;
    String value;
};

struct SettingItemFloat {
    const char* key;
    float value;
};

std::map<Type, SettingItemInt> intSettings = {
    // Network settings
    {WS_SERVER_PORT, {"wsnp", 80}},                        // WebSocket server port
    {UPDATE_SERVER_PORT, {"upp", 80}},                     // Update server port
    
    // Hardware configuration
    {HARDWARE, {"legacy", 8}},                             // 8 = ODESA_KYIV map
#if ARDUINO_ESP32C3_DEV
    {MAIN_LED_PIN, {"pp", 2}},                             // ESP32-C3 default pin
#else
    {MAIN_LED_PIN, {"pp", 13}},                            // ESP32 default pin
#endif
    {MAIN_LED_COUNT, {"pc", 26}},                          // Number of main LEDs
    {BG_LED_PIN, {"bpp", -1}},                             // -1 = disabled
    {BG_LED_COUNT, {"bpc", 0}},                            // Number of background LEDs
    {SERVICE_LED_PIN, {"slp", -1}},                        // -1 = disabled
    
    // Button pins
    {BUTTON_1_PIN, {"bp", -1}},                            // -1 = disabled
    {BUTTON_2_PIN, {"b2p", -1}},                           // -1 = disabled
    {BUTTON_3_PIN, {"b3p", -1}},                           // -1 = disabled
    
    // Siren device 1 pins
    {ALERT_PIN, {"ap", -1}},                               // -1 = disabled
    {CLEAR_PIN, {"cp", -1}},                               // -1 = disabled
    {ALERT_CLEAR_PIN_MODE, {"acpm", 0}},                   // 0 = pulse, 1 = toggle
    {ALERT_PIN_ACTIVE_LEVEL, {"apal", 1}},                 // 1 = HIGH, 0 = LOW
    {ALERT_CLEAR_PIN_TIME, {"acptm", 1000}},               // Pulse duration in ms
    
    // Siren device 2 pins
    {ALERT_PIN_2, {"ap2", -1}},                            // -1 = disabled
    {CLEAR_PIN_2, {"cp2", -1}},                            // -1 = disabled
    {ALERT_CLEAR_PIN_MODE_2, {"acpm2", 0}},                // 0 = pulse, 1 = toggle
    {ALERT_PIN_ACTIVE_LEVEL_2, {"apal2", 1}},              // 1 = HIGH, 0 = LOW
    {ALERT_CLEAR_PIN_TIME_2, {"acptm2", 1000}},            // Pulse duration in ms
    
    // Audio pins
    {BUZZER_PIN, {"bzp", -1}},                             // -1 = disabled
    {DF_RX_PIN, {"dfrx", -1}},                             // DFPlayer RX, -1 = disabled
    {DF_TX_PIN, {"dftx", -1}},                             // DFPlayer TX, -1 = disabled
    
    // Sensor and indicator pins
    {LIGHT_SENSOR_PIN, {"lp", -1}},                        // -1 = disabled
    {POWER_PIN, {"powp", -1}},                             // Service LED: power indicator
    {WIFI_PIN, {"wifip", -1}},                             // Service LED: WiFi indicator
    {DATA_PIN, {"datap", -1}},                             // Service LED: data indicator
    {HA_PIN, {"hap", -1}},                                 // Service LED: HA indicator
    {UPD_AVAILABLE_PIN, {"resp", -1}},                     // Service LED: update available
    
    // Brightness settings (0-100%)
    {CURRENT_BRIGHTNESS, {"cbr", 50}},                     // Current brightness level
    {BRIGHTNESS, {"brightness", 50}},                      // Manual brightness
    {BRIGHTNESS_DAY, {"brd", 50}},                         // Auto: day brightness
    {BRIGHTNESS_NIGHT, {"brn", 10}},                       // Auto: night brightness
    {BRIGHTNESS_MODE, {"bra", 0}},                         // 0 = manual, 1 = auto day/night, 2 = auto sensor
    
    // Home district settings
    {HOME_ALERT_TIME, {"hat", 0}},                         // Seconds delay before home alert
    
    // Enable/disable threat types (0 = disabled, 1 = enabled)
    {ENABLE_EXPLOSIONS, {"eex", 1}},                       // Show explosions
    {ENABLE_MISSILES, {"emi", 1}},                         // Show missiles
    {ENABLE_DRONES, {"edr", 1}},                           // Show drones
    {ENABLE_RECON_DRONES, {"erd", 1}},                     // Show recon drones
    {ENABLE_KABS, {"ekab", 1}},                            // Show KABs
    {ENABLE_BALLISTIC, {"ebal", 1}},                       // Show ballistic missiles
    {ENABLE_SYNC_ANIMATIONS, {"esa", 1}},                  // Sync animations across regions
    {ENABLE_ANIMATION_PREVIEW, {"eap", 1}},                // Show animation preview
    
    // Brightness per state (0-100%)
    {BRIGHTNESS_ALERT, {"ba", 100}},                       // Alert state
    {BRIGHTNESS_CLEAR, {"bc", 100}},                       // Clear state
    {BRIGHTNESS_NEW_ALERT, {"bna", 100}},                  // New alert
    {BRIGHTNESS_ALERT_OVER, {"bao", 100}},                 // Alert ended
    {BRIGHTNESS_EXPLOSION, {"bex", 100}},                  // Explosion threat
    {BRIGHTNESS_MISSILES, {"bmi", 100}},                   // Missile threat
    {BRIGHTNESS_DRONES, {"bdr", 100}},                     // Drone threat
    {BRIGHTNESS_RECON_DRONES, {"brdr", 100}},              // Recon drone threat
    {BRIGHTNESS_KABS, {"bkab", 100}},                      // KAB threat
    {BRIGHTNESS_BALLISTIC, {"bbal", 100}},                 // Ballistic threat
    {BRIGHTNESS_HOME_DISTRICT, {"bhd", 100}},              // Home district
    {BRIGHTNESS_BG, {"bbg", 100}},                         // Background LEDs
    {BRIGHTNESS_SERVICE, {"bs", 50}},                      // Service LEDs
    {BRIGHTNESS_ANIMATION_END, {"baend", 20}},             // Animation end brightness
    {BRIGHTNESS_MIN, {"brmin", 3}},                        // Minimum allowed brightness
    {BRIGHTNESS_MAX, {"brmax", 90}},                       // Maximum allowed brightness (0 = no limit)
    {BRIGHTNESS_MAX_ACCEPT, {"brmxa", 0}},                 // User accepted max brightness warning (0/1)
    
    // Environmental settings
    {NIGHT_MODE_LIGHT_THRESHOLD, {"nmlt", 10}},            // Light level for night mode (lux)
    {WEATHER_MIN_TEMP, {"mintemp", -10}},                  // Min temperature for color scale (°C)
    {WEATHER_MAX_TEMP, {"maxtemp", 30}},                   // Max temperature for color scale (°C)
    {WEATHER_AUTO_BOUNDS, {"wtab", 1}},                    // Auto-calculate temperature bounds from forecast (0/1)
    {RADIATION_MAX, {"maxrad", 2000}},                     // Max radiation for visualization (nSv/h)
    
    // Regional settings
    {ALARMS_AUTO_SWITCH, {"aas", 1}},                      // Auto-switch to alarm map mode (0/1)
    {HOME_DISTRICT, {"hmd", 31}},                          // Home district ID (31 = Kyiv)
    {MIGRATION_LED_MAPPING, {"mgrlm", 0}},                 // LED mapping migration flag (0/1)
    {DISTRICT_MODE_KYIV, {"dmkv", 0}},                     // Kyiv districts display mode
    {DISTRICT_MODE_KHARKIV, {"dmkh", 0}},                  // Kharkiv districts display mode
    {DISTRICT_MODE_ZP, {"dmzp", 0}},                       // Zaporizhzhia districts display mode
    {KYIV_LED, {"kvld", 0}},                               // Kyiv city LED position
    
    // Service LEDs and notifications
    {SERVICE_DIODES_MODE, {"sdm", 0}},                     // Service LEDs mode (0 = off, etc.)
    {NEW_FW_NOTIFICATION, {"nfwn", 1}},                    // Show new firmware notification (0/1)
    
    // Sound source
    {SOUND_SOURCE, {"ss", 0}},                             // 0 = buzzer, 1 = DFPlayer, 2 = none
    {SOUND_ON_MIN_OF_SL, {"somos", 1}},                    // Sound on minute of silence (0/1)
    
    // Sound events: Alert
    {SOUND_ON_ALERT, {"soa", 1}},                          // Play sound on alert (0/1)
    {MELODY_ON_ALERT, {"moa", 4}},                         // Melody index: 4 = Siren
    {TRACK_ON_ALERT, {"toa", 0}},                          // DFPlayer track index
    {SOUND_ON_ALERT_END, {"soae", 1}},                     // Play sound on alert end (0/1)
    {MELODY_ON_ALERT_END, {"moae", 15}},                   // Melody index: 15 = Shedryk
    {TRACK_ON_ALERT_END, {"toae", 5}},                     // DFPlayer track index
    
    // Sound events: Threats
    {SOUND_ON_EXPLOSION, {"soex", 1}},                     // Play sound on explosion (0/1)
    {MELODY_ON_EXPLOSION, {"moex", 18}},                   // Melody index
    {TRACK_ON_EXPLOSION, {"toex", 18}},                    // DFPlayer track index
    {SOUND_ON_DRONES, {"sodr", 1}},                        // Play sound on drones (0/1)
    {MELODY_ON_DRONES, {"modr", 23}},                      // Melody index
    {TRACK_ON_DRONES, {"todr", 23}},                       // DFPlayer track index
    {SOUND_ON_MISSILES, {"somi", 1}},                      // Play sound on missiles (0/1)
    {MELODY_ON_MISSILES, {"momi", 24}},                    // Melody index
    {TRACK_ON_MISSILES, {"tomi", 24}},                     // DFPlayer track index
    {SOUND_ON_KABS, {"sokb", 1}},                          // Play sound on KABs (0/1)
    {MELODY_ON_KABS, {"mokb", 25}},                        // Melody index
    {TRACK_ON_KABS, {"tokb", 25}},                         // DFPlayer track index
    {SOUND_ON_BALLISTIC, {"sobl", 1}},                     // Play sound on ballistic (0/1)
    {MELODY_ON_BALLISTIC, {"mobl", 26}},                   // Melody index
    {TRACK_ON_BALLISTIC, {"tobl", 26}},                    // DFPlayer track index
    {SOUND_ON_RECON_DRONES, {"sord", 1}},                  // Play sound on recon drones (0/1)
    {MELODY_ON_RECON_DRONES, {"mord", 27}},                // Melody index
    {TRACK_ON_RECON_DRONES, {"tord", 27}},                 // DFPlayer track index
    
    // Sound events: Critical threats
    {SOUND_ON_CRITICAL_MIG, {"socrm", 1}},                     // Play sound on critical MiG (0/1)
    {MELODY_ON_CRITICAL_MIG, {"mocrm", 24}},                   // Melody index
    {TRACK_ON_CRITICAL_MIG, {"tocrm", 24}},                    // DFPlayer track index
    {SOUND_ON_CRITICAL_STRATEGIC, {"socrs", 1}},               // Play sound on critical strategic (0/1)
    {MELODY_ON_CRITICAL_STRATEGIC, {"mocrs", 25}},             // Melody index
    {TRACK_ON_CRITICAL_STRATEGIC, {"tocrs", 25}},              // DFPlayer track index
    {SOUND_ON_CRITICAL_MIG_MISSILES, {"socrmm", 1}},           // Play sound on critical MiG missiles (0/1)
    {MELODY_ON_CRITICAL_MIG_MISSILES, {"mocrmm", 26}},         // Melody index
    {TRACK_ON_CRITICAL_MIG_MISSILES, {"tocrmm", 26}},          // DFPlayer track index
    {SOUND_ON_CRITICAL_STRATEGIC_MISSILES, {"socrsm", 1}},     // Play sound on critical strategic missiles (0/1)
    {MELODY_ON_CRITICAL_STRATEGIC_MISSILES, {"mocrsm", 27}},   // Melody index
    {TRACK_ON_CRITICAL_STRATEGIC_MISSILES, {"tocrsm", 27}},    // DFPlayer track index
    {SOUND_ON_CRITICAL_BALLISTIC_MISSILES, {"socrbm", 1}},     // Play sound on critical ballistic missiles (0/1)
    {MELODY_ON_CRITICAL_BALLISTIC_MISSILES, {"mocrbm", 28}},   // Melody index
    {TRACK_ON_CRITICAL_BALLISTIC_MISSILES, {"tocrbm", 28}},    // DFPlayer track index
    {CRITICAL_NOTIFICATIONS_DISPLAY_TIME, {"crndt", 30}},      // Display time for critical notifications (seconds)
    {ENABLE_CRITICAL_NOTIFICATIONS, {"ecn", 1}},               // Enable critical notifications (0/1)
    
    // Sound settings: General
    {SOUND_ON_EVERY_HOUR, {"soeh", 0}},                    // Sound on every hour (clock beep) (0/1)
    {SOUND_ON_BUTTON_CLICK, {"sobc", 1}},                  // Sound on button click (0/1)
    {MUTE_SOUND_ON_NIGHT, {"mson", 0}},                    // Mute sound at night (0/1)
    {MELODY_VOLUME_DAY, {"mv", 100}},                      // Sound volume during day (0-100%)
    {MELODY_VOLUME_NIGHT, {"mvn", 30}},                    // Sound volume at night (0-100%)
    {MELODY_VOLUME_CURRENT, {"mvc", 100}},                 // Current sound volume (0-100%)
    {IGNORE_MUTE_ON_ALERT, {"imoa", 0}},                   // Ignore mute on alert (0/1)
    {IGNORE_MUTE_ON_ALERT, {"imoa", 0}},                   // Ignore mute on alert (0/1)
    
    // Display settings
    {INVERT_DISPLAY, {"invd", 0}},                         // Invert display colors (0/1)
    {DIM_DISPLAY_ON_NIGHT, {"ddon", 1}},                   // Dim display at night (0/1)
    {MAP_MODE, {"mapmode", 1}},                            // 0 = off, 1 = alert, 2 = weather, 3 = flag, 4 = random, 5 = lamp
    {DISPLAY_MODE, {"dm", 9}},                             // 0 = off, 1 = clock, 2 = weather, 3 = tech. info, 4 = microclimate, 5 = combine
    {DISPLAY_MODE_TIME, {"dmt", 5}},                       // Time between display mode switches (seconds)
    {DISPLAY_OFF_AT_NIGHT, {"doan", 0}},                   // Turn off display at night (0/1)
    {TOGGLE_MODE_WEATHER, {"tmw", 1}},                     // Include weather in display rotation (0/1)
    {TOGGLE_MODE_ENERGY, {"tme", 1}},                      // Include energy in display rotation (0/1)
    {TOGGLE_MODE_RADIATION, {"tmr", 1}},                   // Include radiation in display rotation (0/1)
    {TOGGLE_MODE_MICROCLIMATE, {"tmm", 1}},                // Include microclimate in display rotation (0/1)
    
    // Button actions
    {BUTTON_1_MODE, {"bm", 0}},                            // Button 1 short press action
    {BUTTON_2_MODE, {"b2m", 0}},                           // Button 2 short press action
    {BUTTON_3_MODE, {"b3m", 0}},                           // Button 3 short press action
    {BUTTON_1_MODE_LONG, {"bml", 0}},                      // Button 1 long press action
    {BUTTON_2_MODE_LONG, {"b2ml", 0}},                     // Button 2 long press action
    {BUTTON_3_MODE_LONG, {"b3ml", 0}},                     // Button 3 long press action
    {USE_TOUCH_BUTTON_1, {"utb1", 0}},                     // Use touch button 1 (0/1)
    {USE_TOUCH_BUTTON_2, {"utb2", 0}},                     // Use touch button 2 (0/1)
    {USE_TOUCH_BUTTON_3, {"utb3", 0}},                     // Use touch button 3 (0/1)
    {ALARMS_NOTIFY_MODE, {"anm", 2}},                      // Alarms notification mode
    
    // Display hardware
    {DISPLAY_MODEL, {"dsmd", 1}},                          // Display model (1 = SSD1306, 2 = SH1106, etc.)
    {DISPLAY_WIDTH, {"dw", 128}},                          // Display width in pixels
    {DISPLAY_HEIGHT, {"dh", 32}},                          // Display height in pixels (32 or 64)
    {DISPLAY_ROTATION, {"dr", 0}},                         // Display rotation (0-3 = 0°, 90°, 180°, 270°)
    {DISPLAY_ALERT_MESSAGE_TIME, {"damt", 10}},            // Alert message display time (seconds)
    {CLOCK_FONT, {"clkf", 0}},                             // Clock font style (0-5)
    
    // Time settings
    {DAY_START, {"ds", 8}},                                // Day start hour (24h format)
    {NIGHT_START, {"ns", 22}},                             // Night start hour (24h format)
    {TIME_ZONE, {"tzn", 0}},                               // Timezone ID (0 = Europe/Kyiv)
    
    // WebSocket and connection settings
    {WS_ALERT_TIME, {"wsat", 180000}},                     // WebSocket alert timeout (ms)
    {WS_REBOOT_TIME, {"wsrt", 300000}},                    // WebSocket reboot timeout (ms)
    {MIN_OF_SILENCE, {"mos", 1}},                          // Enable minute of silence (0/1)
    
    // System settings
    {LOGS_ENABLED, {"le", 0}},                             // Enable serial logs (0/1)
    {FW_UPDATE_CHANNEL, {"fwuc", 0}},                      // Firmware update channel (0 = stable, 1 = beta)
    
    // Animation timers (seconds)
    {ALERT_ON_TIME, {"aonte", 300}},                       // Animation duration for alert on
    {ALERT_OFF_TIME, {"aofte", 300}},                      // Animation duration for alert off
    {DRONE_TIME, {"drte", 300}},                           // Animation duration for drones
    {RECON_DRONE_TIME, {"rdrte", 300}},                    // Animation duration for recon drones
    {MISSILE_TIME, {"mite", 300}},                         // Animation duration for missiles
    {KAB_TIME, {"kabte", 300}},                            // Animation duration for KABs
    {BALLISTIC_TIME, {"balite", 300}},                     // Animation duration for ballistic
    {EXPLOSION_TIME, {"exte", 300}},                       // Animation duration for explosions
    {ALERT_BLINK_TIME, {"abt", 300}},                      // Alert blink period (seconds)
    
    // LED Hardware configuration
    {MAIN_LED_COLOR_FORMAT, {"mlcf", NEO_GRB}},            // Main strip color format (NEO_GRB, NEO_RGB, etc.)
    {MAIN_LED_FREQUENCY, {"mlfq", NEO_KHZ800}},            // Main strip frequency (NEO_KHZ800 or NEO_KHZ400)
    {BG_LED_COLOR_FORMAT, {"blcf", NEO_GRB}},              // Background strip color format
    {BG_LED_FREQUENCY, {"blfq", NEO_KHZ800}},              // Background strip frequency
    {SERVICE_LED_COLOR_FORMAT, {"slcf", NEO_GRB}},         // Service strip color format
    {SERVICE_LED_FREQUENCY, {"slfq", NEO_KHZ800}},         // Service strip frequency
    {BG_LED_MODE, {"blmd", 0}},                            // 0 = home region, 1 = custom color, 2 = color map
    
    // Battery monitoring
    {BATTERY_PIN, {"batp", -1}},                           // Battery monitoring pin (-1 = disabled)
    {ENABLE_BATTERY_MONITORING, {"ebatm", 0}},             // Enable battery monitoring (0/1)
    
    // Animation types (0 = fade, 1 = blink, 2 = blend fade, 3 = pulse, etc.)
    {ANIMATION_ALERT_ON_TYPE, {"aanot", 0}},               // Alert on animation type
    {ANIMATION_ALERT_OFF_TYPE, {"aaoft",0}},               // Alert off animation type
    {ANIMATION_DRONE_TYPE, {"adrt", 2}},                   // Drone animation type
    {ANIMATION_RECON_DRONE_TYPE, {"ardrt", 9}},            // Recon drone animation type
    {ANIMATION_MISSILE_TYPE, {"amit", 8}},                 // Missile animation type
    {ANIMATION_KAB_TYPE, {"akabt", 3}},                    // KAB animation type
    {ANIMATION_BALLISTIC_TYPE, {"abalit", 3}},             // Ballistic animation type
    {ANIMATION_EXPLOSION_TYPE, {"aex", 3}},                // Explosion animation type
    
    // Animation cycle times (milliseconds)
    {ANIMATION_ALERT_ON_CYCLE_TIME, {"aacot", 1000}},      // Alert on animation cycle time
    {ANIMATION_ALERT_OFF_CYCLE_TIME, {"aacft", 1000}},     // Alert off animation cycle time
    {ANIMATION_DRONE_CYCLE_TIME, {"adct", 700}},           // Drone animation cycle time
    {ANIMATION_RECON_DRONE_CYCLE_TIME, {"ardct", 1000}},   // Recon drone animation cycle time
    {ANIMATION_MISSILE_CYCLE_TIME, {"amct", 700}},         // Missile animation cycle time
    {ANIMATION_KAB_CYCLE_TIME, {"akt", 500}},              // KAB animation cycle time
    {ANIMATION_BALLISTIC_CYCLE_TIME, {"abct", 500}},       // Ballistic animation cycle time
    {ANIMATION_EXPLOSION_CYCLE_TIME, {"aect", 500}},       // Explosion animation cycle time
    
    // Lamp mode
    {BRIGHTNESS_LAMP, {"blamp", 50}},                      // Lamp mode brightness (0-100%)
    
    // API settings
    {API_ENABLED, {"apie", 0}},                            // Enable HTTP API (0/1)
    {API_PORT, {"apip", 81}},                              // HTTP API port

    // Web authentication
    {WEB_AUTH_ENABLED, {"waue", 0}},                       // Enable web auth (0/1)
};

std::map<Type, SettingItemString> stringSettings = {
    // Device identification
    {ID, {"id", "github"}},                                 // Device ID (not restored in backup)
    {DEVICE_NAME, {"dn", "JAAM"}},                          // Device name for mDNS/WiFi
    {DEVICE_DESCRIPTION, {"dd", "JAAM Informer"}},          // Device description
    {BROADCAST_NAME, {"bn", "jaam"}},                       // Broadcast name for discovery
    
    // Network endpoints
    {WS_SERVER_HOST, {"wshost", "ws.jaam.net.ua"}},         // WebSocket server hostname
    {NTP_HOST, {"ntph", "time.google.com"}},                // NTP server for time sync
    
    // Colors in RGB hex format (#RRGGBB)
    {COLOR_ALERT, {"rgbcal", "#FF0000"}},                   // Alert color (red)
    {COLOR_CLEAR, {"rgbccl", "#00FF00"}},                   // Clear color (green)
    {COLOR_NEW_ALERT, {"rgbcna", "#FF3C00"}},               // New alert color (orange)
    {COLOR_ALERT_OVER, {"rgbcao", "#00FF3C"}},              // Alert over color (yellow-green)
    {COLOR_EXPLOSION, {"rgbcex", "#00FFFF"}},               // Explosion color (cyan)
    {COLOR_MISSILES, {"rgbcmi", "#9600FF"}},                // Missile color (purple)
    {COLOR_DRONES, {"rgbcdr", "#FF00FF"}},                  // Drone color (magenta)
    {COLOR_RECON_DRONES, {"rgbcrdr", "#0000FF"}},           // Recon drone color (blue)
    {COLOR_KABS, {"rgbckab", "#FFFF00"}},                   // KAB color (yellow)
    {COLOR_BALLISTIC, {"rgbcbal", "#FFFFFF"}},              // Ballistic color (white)
    {COLOR_HOME_DISTRICT, {"rgbchd", "#00FF64"}},           // Home district color (light blue)
    {COLOR_BG, {"rgbcbg", "#00FF00"}},                      // Background color (green)
    {COLOR_LAMP, {"rgbclamp", "#D707D7"}},                  // Lamp mode color (purple)

    // Web authentication
    {WEB_LOGIN, {"waulg", "admin"}},                         // Web auth login
    {WEB_PASSWORD, {"waupass", "admin"}},                     // Web auth password
};

std::map<Type, SettingItemFloat> floatSettings = {
    // Sensor calibration factors
    {TEMP_CORRECTION, {"ltc", 0.0f}},                       // Temperature correction offset (°C)
    {HUM_CORRECTION, {"lhc", 0.0f}},                        // Humidity correction offset (%)
    {PRESSURE_CORRECTION, {"lpc", 0.0f}},                   // Pressure correction offset (hPa)
    {LIGHT_SENSOR_FACTOR, {"lsf", 1.0f}},                   // Light sensor calibration factor (multiplier)
};

Preferences preferences;
const char* PREFS_NAME = "storage";

void JaamSettings::init() {
    preferences.begin(PREFS_NAME, false);

    for (auto it = stringSettings.begin(); it != stringSettings.end(); ++it) {
        SettingItemString setting = it->second;
        if (preferences.isKey(setting.key)) {
            setting.value = preferences.getString(setting.key);
        }
        it->second = setting;
    }

    for (auto it = intSettings.begin(); it != intSettings.end(); ++it) {
        SettingItemInt setting = it->second;
        if (preferences.isKey(setting.key)) {
            setting.value = preferences.getInt(setting.key);
        }
        it->second = setting;
    }

    for (auto it = floatSettings.begin(); it != floatSettings.end(); ++it) {
        SettingItemFloat setting = it->second;
        if (preferences.isKey(setting.key)) {
            setting.value = preferences.getFloat(setting.key);
        }
        it->second = setting;
    }

    preferences.end();
}

const char* JaamSettings::getKey(Type type) {
    if (intSettings.find(type) != intSettings.end()) {
        return intSettings[type].key;
    } else if (stringSettings.find(type) != stringSettings.end()) {
        return stringSettings[type].key;
    } else if (floatSettings.find(type) != floatSettings.end()) {
        return floatSettings[type].key;
    }
    LOG.printf("[SETTINGS] Unknown setting type\n");
    throw std::runtime_error("Unknown setting type");
}

int JaamSettings::getInt(Type type) {
    if (intSettings.find(type) != intSettings.end()) {
        return intSettings[type].value;
    }
    LOG.printf("[SETTINGS] Unknown int setting type\n");
    throw std::runtime_error("Unknown setting type");
}

bool JaamSettings::validateIntSetting(Type type, int value) {
    // Перевірка API_PORT != 80 (зарезервований для веб-сервера)
    if (type == API_PORT && value == 80) {
        LOG.printf("[SETTINGS] API_PORT cannot be 80 (reserved for web server)\n");
        return false;
    }
    
    // Перевірка BRIGHTNESS_MAX: 0 (default) або відсоток в діапазоні [0..ABSOLUTE_MAX]
    if (type == BRIGHTNESS_MAX) {
        // негативні значення яскравості завжди некоректні
        if (value < 0) {
            LOG.printf("[SETTINGS] BRIGHTNESS_MAX %d%% is negative and invalid\n", value);
            return false;
        }
        // перевищення абсолютного максимуму залишається помилкою
        if (value > JaamHardwareLed::BRIGHTNESS_ABSOLUTE_MAX) {
            LOG.printf("[SETTINGS] BRIGHTNESS_MAX %d%% exceeds absolute max %d%%\n", value, JaamHardwareLed::BRIGHTNESS_ABSOLUTE_MAX);
            return false;
        }
    }
    
    // Для JAAM2: піни сирени не можуть бути BH1750_POWER_PIN (керуючий пін живлення для сенсора освітлення)
    if (getInt(HARDWARE) == HARDWARE::JAAM_2_1) { // JAAM2
        if ((type == ALERT_PIN || type == CLEAR_PIN || type == ALERT_PIN_2 || type == CLEAR_PIN_2) && value == BH1750_POWER_PIN) {
            LOG.printf("[SETTINGS] Siren pins cannot be %d for JAAM2 (power control pin for light sensor)\n", BH1750_POWER_PIN);
            return false;
        }
    }
    
    // Перевірка перетину пінів першого та другого пристрою сирени
    if ((type == ALERT_PIN || type == CLEAR_PIN) && value > 0) {
        int alertPin2 = getInt(ALERT_PIN_2);
        int clearPin2 = getInt(CLEAR_PIN_2);
        if ((alertPin2 > 0 && value == alertPin2) || (clearPin2 > 0 && value == clearPin2)) {
            LOG.printf("[SETTINGS] Pin %d for first siren device conflicts with second siren device\n", value);
            return false;
        }
    }
    
    if ((type == ALERT_PIN_2 || type == CLEAR_PIN_2) && value > 0) {
        int alertPin = getInt(ALERT_PIN);
        int clearPin = getInt(CLEAR_PIN);
        if ((alertPin > 0 && value == alertPin) || (clearPin > 0 && value == clearPin)) {
            LOG.printf("[SETTINGS] Pin %d for second siren device conflicts with first siren device\n", value);
            return false;
        }
    }
    
    return true;
}

bool JaamSettings::saveInt(Type type, int value, bool saveToPrefs) {
    if (intSettings.find(type) != intSettings.end()) {
        // Валідація перед збереженням
        if (!validateIntSetting(type, value)) {
            return false;
        }
        
        SettingItemInt setting = intSettings[type];
        if (saveToPrefs) {
            preferences.begin(PREFS_NAME, false);
            preferences.putInt(setting.key, value);
            preferences.end();
        }
        setting.value = value;
        intSettings[type] = setting;
        LOG.printf("[SETTINGS] Saved setting %s: %d (to prefs - %s)\n", setting.key, value, saveToPrefs ? "true" : "false");
        
        // Якщо змінюємо HARDWARE на JAAM2, скидаємо піни сирени що дорівнюють BH1750_POWER_PIN
        if (type == HARDWARE && value == HARDWARE::JAAM_2_1) {
            if (getInt(ALERT_PIN) == BH1750_POWER_PIN) {
                LOG.printf("[SETTINGS] Resetting ALERT_PIN to -1 (conflicts with JAAM2 light sensor power pin)\n");
                saveInt(ALERT_PIN, -1, saveToPrefs);
            }
            if (getInt(CLEAR_PIN) == BH1750_POWER_PIN) {
                LOG.printf("[SETTINGS] Resetting CLEAR_PIN to -1 (conflicts with JAAM2 light sensor power pin)\n");
                saveInt(CLEAR_PIN, -1, saveToPrefs);
            }
            if (getInt(ALERT_PIN_2) == BH1750_POWER_PIN) {
                LOG.printf("[SETTINGS] Resetting ALERT_PIN_2 to -1 (conflicts with JAAM2 light sensor power pin)\n");
                saveInt(ALERT_PIN_2, -1, saveToPrefs);
            }
            if (getInt(CLEAR_PIN_2) == BH1750_POWER_PIN) {
                LOG.printf("[SETTINGS] Resetting CLEAR_PIN_2 to -1 (conflicts with JAAM2 light sensor power pin)\n");
                saveInt(CLEAR_PIN_2, -1, saveToPrefs);
            }
        }

        // Викликаємо callback якщо зареєстрований
        if (changeCallback) {
            changeCallback(type, value, 0.0f, nullptr);
        }
        
        return true;
    }
    LOG.printf("[SETTINGS] Unknown int setting type\n");
    return false;
}

const char* JaamSettings::getString(Type type) {
    if (stringSettings.find(type) != stringSettings.end()) {
        return stringSettings[type].value.c_str();
    }
    LOG.printf("[SETTINGS] Unknown string setting type\n");
    throw std::runtime_error("Unknown setting type");
}

bool JaamSettings::saveString(Type type, const char* value, bool saveToPrefs) {
    if (stringSettings.find(type) != stringSettings.end()) {
        SettingItemString setting = stringSettings[type];
        if (saveToPrefs) {
            preferences.begin(PREFS_NAME, false);
            preferences.putString(setting.key, value);
            preferences.end();
        }
        setting.value = value;
        stringSettings[type] = setting;
        LOG.printf("[SETTINGS] Saved setting %s: '%s' (to prefs - %s)\n", setting.key, value, saveToPrefs ? "true" : "false");
        
        // Викликаємо callback якщо зареєстрований
        if (changeCallback) {
            changeCallback(type, 0, 0.0f, value);
        }
        
        return true;
    }
    LOG.printf("[SETTINGS] Unknown stringsetting type\n");
    return false;
}

float JaamSettings::getFloat(Type type) {
    if (floatSettings.find(type) != floatSettings.end()) {
        return floatSettings[type].value;
    }
    LOG.printf("[SETTINGS] Unknown floatsetting type\n");
    throw std::runtime_error("Unknown setting type");
}

bool JaamSettings::saveFloat(Type type, float value, bool saveToPrefs) {
    if (floatSettings.find(type) != floatSettings.end()) {
        SettingItemFloat setting = floatSettings[type];
        if (saveToPrefs) {
            preferences.begin(PREFS_NAME, false);
            preferences.putFloat(setting.key, value);
            preferences.end();
        }
        setting.value = value;
        floatSettings[type] = setting;
        LOG.printf("[SETTINGS] Saved setting %s: %.1f (to prefs - %s)\n", setting.key, value, saveToPrefs ? "true" : "false");
        
        // Викликаємо callback якщо зареєстрований
        if (changeCallback) {
            changeCallback(type, 0, value, nullptr);
        }
          
        return true;
    }
    LOG.printf("[SETTINGS] Unknown floatsetting type\n");
    return false;
}

bool JaamSettings::getBool(Type type) {
    return getInt(type) == 1;
}

bool JaamSettings::saveBool(Type type, bool value, bool saveToPrefs) {
    return saveInt(type, value ? 1 : 0, saveToPrefs);
}

bool JaamSettings::hasKey(Type type) {
    const char* key = nullptr;
    try {
        key = getKey(type);
    } catch (const std::runtime_error&) {
        // Unknown type, treat as absent
        return false;
    }
    preferences.begin(PREFS_NAME, true);
    bool exists = preferences.isKey(key);
    preferences.end();
    return exists;
}

void JaamSettings::getSettingsBackup(Print* stream, const char* fwVersion, const char* chipID, const char* time) {
    JsonDocument doc;
    doc["fw_version"] = fwVersion;
    doc["chip_id"] = chipID;
    doc["time"] = time;
    JsonArray settingsArray = doc["settings"].to<JsonArray>();
    preferences.begin(PREFS_NAME, true);

    for (auto it = stringSettings.begin(); it != stringSettings.end(); ++it) {
        SettingItemString setting = it->second;
        const char* key = setting.key;
        if (preferences.isKey(key)) {
            JsonObject settingObj = settingsArray.add<JsonObject>();
            settingObj["key"] = key;
            settingObj["value"] = preferences.getString(key);
            settingObj["type"] = PF_STRING;
        }
    }

    for (auto it = intSettings.begin(); it != intSettings.end(); ++it) {
        SettingItemInt setting = it->second;
        const char* key = setting.key;
        if (preferences.isKey(key)) {
            JsonObject settingObj = settingsArray.add<JsonObject>();
            settingObj["key"] = key;
            settingObj["value"] = preferences.getInt(key);
            settingObj["type"] = PF_INT;
        }
    }

    for (auto it = floatSettings.begin(); it != floatSettings.end(); ++it) {
        SettingItemFloat setting = it->second;
        const char* key = setting.key;
        if (preferences.isKey(key)) {
            JsonObject settingObj = settingsArray.add<JsonObject>();
            settingObj["key"] = key;
            settingObj["value"] = preferences.getFloat(key);
            settingObj["type"] = PF_FLOAT;
        }
    }

    preferences.end();

    stream->print(doc.as<String>());
}

bool JaamSettings::restoreSettingsBackup(const char* settings) {
    JsonDocument doc;
    deserializeJson(doc, settings);
    JsonArray settingsArray = doc["settings"].as<JsonArray>();
    preferences.begin(PREFS_NAME, false);
    bool restored = false;

    for (JsonObject settingObj : settingsArray) {
        const char* key = settingObj["key"];
        const char* type = settingObj["type"];

        // skip id key, we do not need to restore it
        if (strcmp(key, "id") == 0) continue;

        if (strcmp(type, PF_STRING) == 0) {
            String valueString = settingObj["value"].as<String>();
            preferences.putString(key, valueString);
            LOG.printf("[SETTINGS] Restored setting: '%s' with value '%s'\n", key, valueString.c_str());
        } else if (strcmp(type, PF_INT) == 0) {
            int valueInt = settingObj["value"].as<int>();
            preferences.putInt(key, valueInt);
            LOG.printf("[SETTINGS] Restored setting: '%s' with value '%d'\n", key, valueInt);
        } else if (strcmp(type, PF_FLOAT) == 0) {
            float valueFloat = settingObj["value"].as<float>();
            preferences.putFloat(key, valueFloat);
            LOG.printf("[SETTINGS] Restored setting: '%s' with value '%.1f'\n", key, valueFloat);
        }
        restored = true;
    }

    preferences.end();
    
    // Reload settings from NVS to update in-memory maps
    if (restored) {
        init();
        LOG.println("[SETTINGS] Settings reload completed");
    }

    return restored;
}

bool JaamSettings::resetToDefaults() {
    LOG.println("[SETTINGS] Resetting all settings to defaults");
    
    preferences.begin(PREFS_NAME, false);
    
    // Save values of preserved settings before clearing
    struct PreservedValue {
        const char* key;
        String strValue;
        int intValue;
        float floatValue;
        char type; // 'S', 'I', 'F'
    };
    PreservedValue savedValues[PRESERVED_SETTINGS_COUNT];
    
    for (int i = 0; i < PRESERVED_SETTINGS_COUNT; i++) {
        const char* key = PRESERVED_SETTINGS[i];
        savedValues[i].key = key;
        
        // Determine type and save value
        if (preferences.isKey(key)) {
            // Check in which map this key exists
            bool found = false;
            for (auto it = intSettings.begin(); it != intSettings.end(); ++it) {
                if (strcmp(it->second.key, key) == 0) {
                    savedValues[i].type = 'I';
                    savedValues[i].intValue = preferences.getInt(key);
                    LOG.printf("[SETTINGS] Preserving int setting: '%s' = %d\n", key, savedValues[i].intValue);
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (auto it = stringSettings.begin(); it != stringSettings.end(); ++it) {
                    if (strcmp(it->second.key, key) == 0) {
                        savedValues[i].type = 'S';
                        savedValues[i].strValue = preferences.getString(key);
                        LOG.printf("[SETTINGS] Preserving string setting: '%s' = '%s'\n", key, savedValues[i].strValue.c_str());
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                for (auto it = floatSettings.begin(); it != floatSettings.end(); ++it) {
                    if (strcmp(it->second.key, key) == 0) {
                        savedValues[i].type = 'F';
                        savedValues[i].floatValue = preferences.getFloat(key);
                        LOG.printf("[SETTINGS] Preserving float setting: '%s' = %.2f\n", key, savedValues[i].floatValue);
                        found = true;
                        break;
                    }
                }
            }
        } else {
            savedValues[i].type = '\0'; // Mark as not present
        }
    }
    
    // Clear all preferences
    preferences.clear();
    LOG.println("[SETTINGS] Cleared all preferences");
    
    // Restore preserved settings
    for (int i = 0; i < PRESERVED_SETTINGS_COUNT; i++) {
        if (savedValues[i].type == 'I') {
            preferences.putInt(savedValues[i].key, savedValues[i].intValue);
            LOG.printf("[SETTINGS] Restored int setting: '%s' = %d\n", savedValues[i].key, savedValues[i].intValue);
        } else if (savedValues[i].type == 'S') {
            preferences.putString(savedValues[i].key, savedValues[i].strValue);
            LOG.printf("[SETTINGS] Restored string setting: '%s' = '%s'\n", savedValues[i].key, savedValues[i].strValue.c_str());
        } else if (savedValues[i].type == 'F') {
            preferences.putFloat(savedValues[i].key, savedValues[i].floatValue);
            LOG.printf("[SETTINGS] Restored float setting: '%s' = %.2f\n", savedValues[i].key, savedValues[i].floatValue);
        }
    }
    
    preferences.end();
    
    LOG.println("[SETTINGS] Settings reset to defaults completed. Reboot required to apply changes.");
    return true;
}

void JaamSettings::setChangeCallback(SettingsChangeCallback callback) {
    changeCallback = callback;
}
