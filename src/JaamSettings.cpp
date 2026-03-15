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
    {WS_SERVER_PORT, {"wsnp", 80}},
    {UPDATE_SERVER_PORT, {"upp", 80}},
    {HARDWARE, {"legacy", 1}},
#if ARDUINO_ESP32C3_DEV
    {MAIN_LED_PIN, {"pp", 2}},
#else
    {MAIN_LED_PIN, {"pp", 13}},
#endif
    {MAIN_LED_COUNT, {"pc", 26}},
    {BG_LED_PIN, {"bpp", -1}},
    {BG_LED_COUNT, {"bpc", 0}},
    {SERVICE_LED_PIN, {"slp", -1}},
    {BUTTON_1_PIN, {"bp", -1}},
    {BUTTON_2_PIN, {"b2p", -1}},
    {BUTTON_3_PIN, {"b3p", -1}},
    {ALERT_PIN, {"ap", -1}},
    {CLEAR_PIN, {"cp", -1}},
    {ALERT_CLEAR_PIN_MODE, {"acpm", 0}},
    {ALERT_PIN_ACTIVE_LEVEL, {"apal", 1}},
    {ALERT_CLEAR_PIN_TIME, {"acptm", 1000}},
    {ALERT_PIN_2, {"ap2", -1}},
    {CLEAR_PIN_2, {"cp2", -1}},
    {ALERT_CLEAR_PIN_MODE_2, {"acpm2", 0}},
    {ALERT_PIN_ACTIVE_LEVEL_2, {"apal2", 1}},
    {ALERT_CLEAR_PIN_TIME_2, {"acptm2", 1000}},
    {BUZZER_PIN, {"bzp", -1}},
    {DF_RX_PIN, {"dfrx", -1}},
    {DF_TX_PIN, {"dftx", -1}},
    {LIGHT_SENSOR_PIN, {"lp", -1}},
    {POWER_PIN, {"powp", -1}},
    {WIFI_PIN, {"wifip", -1}},
    {DATA_PIN, {"datap", -1}},
    {HA_PIN, {"hap", -1}},
    {UPD_AVAILABLE_PIN, {"resp", -1}},
    {CURRENT_BRIGHTNESS, {"cbr", 50}},
    {BRIGHTNESS, {"brightness", 50}},
    {BRIGHTNESS_DAY, {"brd", 50}},
    {BRIGHTNESS_NIGHT, {"brn", 5}},
    {BRIGHTNESS_MODE, {"bra", 0}},
    {HOME_ALERT_TIME, {"hat", 0}},
    {COLOR_ALERT, {"coloral", 0}},
    {COLOR_CLEAR, {"colorcl", 120}},
    {COLOR_NEW_ALERT, {"colorna", 30}},
    {COLOR_ALERT_OVER, {"colorao", 100}},
    {COLOR_EXPLOSION, {"colorex", 180}},
    {COLOR_MISSILES, {"colormi", 275}},
    {COLOR_DRONES, {"colordr", 330}},
    {COLOR_KABS, {"colorkab", 55}},
    {COLOR_HOME_DISTRICT, {"colorhd", 120}},
    {COLOR_BG_NEIGHBOR_ALERT, {"colorbna", 30}},
    {ENABLE_EXPLOSIONS, {"eex", 1}},
    {ENABLE_MISSILES, {"emi", 1}},
    {ENABLE_DRONES, {"edr", 1}},
    {ENABLE_RECON_DRONES, {"erd", 1}},
    {ENABLE_KABS, {"ekab", 1}},
    {ENABLE_BALLISTIC, {"ebal", 1}},
    {ENABLE_SYNC_ANIMATIONS, {"esa", 0}},
    {BRIGHTNESS_ALERT, {"ba", 100}},
    {BRIGHTNESS_CLEAR, {"bc", 100}},
    {BRIGHTNESS_NEW_ALERT, {"bna", 100}},
    {BRIGHTNESS_ALERT_OVER, {"bao", 100}},
    {BRIGHTNESS_EXPLOSION, {"bex", 100}},
    {BRIGHTNESS_MISSILES, {"bmi", 100}},
    {BRIGHTNESS_DRONES, {"bdr", 100}},
    {BRIGHTNESS_RECON_DRONES, {"brdr", 100}},
    {BRIGHTNESS_KABS, {"bkab", 100}},
    {BRIGHTNESS_BALLISTIC, {"bbal", 100}},
    {BRIGHTNESS_HOME_DISTRICT, {"bhd", 100}},
    {BRIGHTNESS_BG, {"bbg", 100}},
    {BRIGHTNESS_SERVICE, {"bs", 50}},
    {BRIGHTNESS_ANIMATION_END, {"baend", 20}},
    {BRIGHTNESS_MIN, {"brmin", 0}},
    {BRIGHTNESS_MAX, {"brmax", 0}},
    {BRIGHTNESS_MAX_ACCEPT, {"brmxa", 0}},
    {NIGHT_MODE_LIGHT_THRESHOLD, {"nmlt", 30}},
    {WEATHER_MIN_TEMP, {"mintemp", -10}},
    {WEATHER_MAX_TEMP, {"maxtemp", 30}},
    {RADIATION_MAX, {"maxrad", 2000}},
    {ALARMS_AUTO_SWITCH, {"aas", 1}},
    {HOME_DISTRICT, {"hmd", 31}},
    {MIGRATION_LED_MAPPING, {"mgrlm", 0}},
    {DISTRICT_MODE_KYIV, {"dmkv", 0}},
    {DISTRICT_MODE_KHARKIV, {"dmkh", 0}},
    {DISTRICT_MODE_ZP, {"dmzp", 0}},
    {KYIV_LED, {"kvld", 0}},
    {SERVICE_DIODES_MODE, {"sdm", 0}},
    {NEW_FW_NOTIFICATION, {"nfwn", 1}},
    {SOUND_SOURCE, {"ss", 0}},
    {SOUND_ON_MIN_OF_SL, {"somos", 0}},
    {SOUND_ON_ALERT, {"soa", 0}},
    {MELODY_ON_ALERT, {"moa", 4}},
    {TRACK_ON_ALERT, {"toa", 0}},
    {SOUND_ON_ALERT_END, {"soae", 0}},
    {MELODY_ON_ALERT_END, {"moae", 5}},
    {TRACK_ON_ALERT_END, {"toae", 5}},
    {SOUND_ON_EXPLOSION, {"soex", 1}},
    {MELODY_ON_EXPLOSION, {"moex", 18}},
    {TRACK_ON_EXPLOSION, {"toex", 18}},
    {SOUND_ON_DRONES, {"sodr", 1}},
    {MELODY_ON_DRONES, {"modr", 23}},
    {TRACK_ON_DRONES, {"todr", 23}},
    {SOUND_ON_MISSILES, {"somi", 1}},
    {MELODY_ON_MISSILES, {"momi", 24}},
    {TRACK_ON_MISSILES, {"tomi", 24}},
    {SOUND_ON_KABS, {"sokb", 1}},
    {MELODY_ON_KABS, {"mokb", 25}},
    {TRACK_ON_KABS, {"tokb", 25}},
    {SOUND_ON_BALLISTIC, {"sobl", 1}},
    {MELODY_ON_BALLISTIC, {"mobl", 26}},
    {TRACK_ON_BALLISTIC, {"tobl", 26}},
    {SOUND_ON_RECON_DRONES, {"sord", 1}},
    {MELODY_ON_RECON_DRONES, {"mord", 27}},
    {TRACK_ON_RECON_DRONES, {"tord", 27}},
    {SOUND_ON_CRITICAL_MIG, {"socrm", 1}},
    {MELODY_ON_CRITICAL_MIG, {"mocrm", 24}},
    {TRACK_ON_CRITICAL_MIG, {"tocrm", 24}},
    {SOUND_ON_CRITICAL_STRATEGIC, {"socrs", 1}},
    {MELODY_ON_CRITICAL_STRATEGIC, {"mocrs", 25}},
    {TRACK_ON_CRITICAL_STRATEGIC, {"tocrs", 25}},
    {SOUND_ON_CRITICAL_MIG_MISSILES, {"socrmm", 1}},
    {MELODY_ON_CRITICAL_MIG_MISSILES, {"mocrmm", 26}},
    {TRACK_ON_CRITICAL_MIG_MISSILES, {"tocrmm", 26}},
    {SOUND_ON_CRITICAL_STRATEGIC_MISSILES, {"socrsm", 1}},
    {MELODY_ON_CRITICAL_STRATEGIC_MISSILES, {"mocrsm", 27}},
    {TRACK_ON_CRITICAL_STRATEGIC_MISSILES, {"tocrsm", 27}},
    {SOUND_ON_CRITICAL_BALLISTIC_MISSILES, {"socrbm", 1}},
    {MELODY_ON_CRITICAL_BALLISTIC_MISSILES, {"mocrbm", 28}},
    {TRACK_ON_CRITICAL_BALLISTIC_MISSILES, {"tocrbm", 28}},
    {CRITICAL_NOTIFICATIONS_DISPLAY_TIME, {"crndt", 30}},
    {ENABLE_CRITICAL_NOTIFICATIONS, {"ecn", 1}},
    {SOUND_ON_EVERY_HOUR, {"soeh", 0}},
    {SOUND_ON_BUTTON_CLICK, {"sobc", 0}},
    {MUTE_SOUND_ON_NIGHT, {"mson", 0}},
    {MELODY_VOLUME_DAY, {"mv", 100}},
    {MELODY_VOLUME_NIGHT, {"mvn", 30}},
    {MELODY_VOLUME_CURRENT, {"mvc", 100}},
    {INVERT_DISPLAY, {"invd", 0}},
    {DIM_DISPLAY_ON_NIGHT, {"ddon", 1}},
    {IGNORE_MUTE_ON_ALERT, {"imoa", 0}},
    {MAP_MODE, {"mapmode", 1}},
    {DISPLAY_MODE, {"dm", 2}},
    {DISPLAY_MODE_TIME, {"dmt", 5}},
    {DISPLAY_OFF_AT_NIGHT, {"doan", 0}},
    {TOGGLE_MODE_WEATHER, {"tmw", 1}},
    {TOGGLE_MODE_RADIATION, {"tmr", 1}},
    {TOGGLE_MODE_ENERGY, {"tme", 1}},
    {TOGGLE_MODE_TEMP, {"tmt", 1}},
    {TOGGLE_MODE_HUM, {"tmh", 1}},
    {TOGGLE_MODE_PRESS, {"tmp", 1}},
    {BUTTON_1_MODE, {"bm", 0}},
    {BUTTON_2_MODE, {"b2m", 0}},
    {BUTTON_3_MODE, {"b3m", 0}},
    {BUTTON_1_MODE_LONG, {"bml", 0}},
    {BUTTON_2_MODE_LONG, {"b2ml", 0}},
    {BUTTON_3_MODE_LONG, {"b3ml", 0}},
    {USE_TOUCH_BUTTON_1, {"utb1", 0}},
    {USE_TOUCH_BUTTON_2, {"utb2", 0}},
    {USE_TOUCH_BUTTON_3, {"utb3", 0}},
    {ALARMS_NOTIFY_MODE, {"anm", 2}},
    {DISPLAY_MODEL, {"dsmd", 1}},
    {DISPLAY_WIDTH, {"dw", 128}},
    {DISPLAY_HEIGHT, {"dh", 32}},
    {DISPLAY_ROTATION, {"dr", 0}},
    {DISPLAY_ALERT_MESSAGE_TIME, {"damt", 5}},
    {CLOCK_FONT, {"clkf", 0}},
    {DAY_START, {"ds", 8}},
    {NIGHT_START, {"ns", 22}},
    {WS_ALERT_TIME, {"wsat", 180000}},
    {WS_REBOOT_TIME, {"wsrt", 300000}},
    {MIN_OF_SILENCE, {"mos", 1}},
    {LOGS_ENABLED, {"le", 0}},
    {FW_UPDATE_CHANNEL, {"fwuc", 0}},
    {ALERT_ON_TIME, {"aonte", 300}},
    {ALERT_OFF_TIME, {"aofte", 5}},
    {DRONE_TIME, {"drte", 300}},
    {RECON_DRONE_TIME, {"rdrte", 300}},
    {MISSILE_TIME, {"mite", 300}},
    {KAB_TIME, {"kabte", 300}},
    {BALLISTIC_TIME, {"balite", 300}},
    {EXPLOSION_TIME, {"exte", 300}},
    {ALERT_BLINK_TIME, {"abt", 300}},
    {MAIN_LED_COLOR_FORMAT, {"mlcf", NEO_GRB}},
    {MAIN_LED_FREQUENCY, {"mlfq", NEO_KHZ800}},
    {BG_LED_COLOR_FORMAT, {"blcf", NEO_GRB}},
    {BG_LED_FREQUENCY, {"blfq", NEO_KHZ800}},
    {SERVICE_LED_COLOR_FORMAT, {"slcf", NEO_GRB}},
    {SERVICE_LED_FREQUENCY, {"slfq", NEO_KHZ800}},
    {BG_LED_MODE, {"blmd", 0}},
    {BATTERY_PIN, {"batp", -1}},
    {ENABLE_BATTERY_MONITORING, {"ebatm", 0}},
    {ANIMATION_ALERT_ON_TYPE, {"aanot", 0}},
    {ANIMATION_ALERT_OFF_TYPE, {"aaoft",0}},
    {ANIMATION_DRONE_TYPE, {"adrt", 2}},
    {ANIMATION_RECON_DRONE_TYPE, {"ardrt", 1}},
    {ANIMATION_MISSILE_TYPE, {"amit", 2}},
    {ANIMATION_KAB_TYPE, {"akabt", 3}},
    {ANIMATION_BALLISTIC_TYPE, {"abalit", 3}},
    {ANIMATION_EXPLOSION_TYPE, {"aex", 3}},
    {ANIMATION_ALERT_ON_CYCLE_TIME, {"aacot", 1000}},
    {ANIMATION_ALERT_OFF_CYCLE_TIME, {"aacft", 1000}},
    {ANIMATION_DRONE_CYCLE_TIME, {"adct", 700}},
    {ANIMATION_RECON_DRONE_CYCLE_TIME, {"ardct", 500}},
    {ANIMATION_MISSILE_CYCLE_TIME, {"amct", 700}},
    {ANIMATION_KAB_CYCLE_TIME, {"akt", 500}},
    {ANIMATION_BALLISTIC_CYCLE_TIME, {"abct", 500}},
    {ANIMATION_EXPLOSION_CYCLE_TIME, {"aect", 500}},
    {BRIGHTNESS_LAMP, {"blamp", 50}},
    {TIME_ZONE, {"tzn", 0}},
    {API_ENABLED, {"apie", 0}},
    {API_PORT, {"apip", 81}},
};

std::map<Type, SettingItemString> stringSettings = {
    {ID, {"id", "github"}},
    {DEVICE_NAME, {"dn", "JAAM"}},
    {DEVICE_DESCRIPTION, {"dd", "JAAM Informer"}},
    {BROADCAST_NAME, {"bn", "jaam"}},
    {WS_SERVER_HOST, {"wshost", "ws.jaam.net.ua"}},
    {NTP_HOST, {"ntph", "time.google.com"}},
    {COLOR_ALERT, {"rgbcal", "#FF0000"}},
    {COLOR_CLEAR, {"rgbccl", "#00FF00"}},
    {COLOR_NEW_ALERT, {"rgbcna", "#FF3C00"}},
    {COLOR_ALERT_OVER, {"rgbcao", "#00FF3C"}},
    {COLOR_EXPLOSION, {"rgbcex", "#00FFFF"}},
    {COLOR_MISSILES, {"rgbcmi", "#AA00FF"}},
    {COLOR_DRONES, {"rgbcdr", "#FF00AA"}},
    {COLOR_RECON_DRONES, {"rgbcrdr", "#0000FF"}},
    {COLOR_KABS, {"rgbckab", "#FFFF00"}},
    {COLOR_BALLISTIC, {"rgbcbal", "#FFFFFF"}},
    {COLOR_HOME_DISTRICT, {"rgbchd", "#5CFF5C"}},
    {COLOR_BG, {"rgbcbg", "#00FF00"}},
    {COLOR_LAMP, {"rgbclamp", "#D707FF"}},
};

std::map<Type, SettingItemFloat> floatSettings = {
    {TEMP_CORRECTION, {"ltc", 0.0f}},
    {HUM_CORRECTION, {"lhc", 0.0f}},
    {PRESSURE_CORRECTION, {"lpc", 0.0f}},
    {LIGHT_SENSOR_FACTOR, {"lsf", 1.0f}},
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

    // Перевірка BRIGHTNESS_MAX: 0 (default) or percentage in [DEFAULT_MAX_PCT, ABSOLUTE_MAX_PCT]
    if (type == BRIGHTNESS_MAX) {
        if (value != 0) {
            if (value > JaamHardwareLed::BRIGHTNESS_ABSOLUTE_MAX_PCT) {
                LOG.printf("[SETTINGS] BRIGHTNESS_MAX %d%% exceeds absolute max %d%%\n", value, JaamHardwareLed::BRIGHTNESS_ABSOLUTE_MAX_PCT);
                return false;
            }
            if (value < JaamHardwareLed::BRIGHTNESS_DEFAULT_MAX_PCT) {
                LOG.printf("[SETTINGS] BRIGHTNESS_MAX %d%% below default max %d%%\n", value, JaamHardwareLed::BRIGHTNESS_DEFAULT_MAX_PCT);
                return false;
            }
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

    return restored;
}

void JaamSettings::setChangeCallback(SettingsChangeCallback callback) {
    changeCallback = callback;
}
