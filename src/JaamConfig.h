#pragma once


// --- Default Colors ---
namespace DefaultColors {
    static const uint32_t MAIN_STRIP = 0x00FF00;  // Green
    static const uint32_t BG_STRIP = 0xFF0000;    // Red
    static const uint32_t SERVICE_STRIP = 0x0000FF;  // Blue
}

// --- Animation Configuration ---
namespace AnimationConfig {
    static const uint32_t MIN_PERIOD = 1000;
    static const uint32_t MAX_PERIOD = 2000;
    static const uint8_t MIN_CYCLES = 1;
    static const uint32_t MAX_CYCLES = 600;
    static const uint8_t MIN_START_BRIGHTNESS = 0;
    static const uint8_t MAX_START_BRIGHTNESS = 255;
    static const uint8_t MIN_END_BRIGHTNESS = 0;
    static const uint8_t MAX_END_BRIGHTNESS = 255;
}

// --- WiFi Configuration ---
namespace WiFiConfig {
    static const bool ENABLED = true;
    static const uint32_t CONNECT_TIMEOUT = 3000;  // 10 seconds
    static const uint32_t CONNECT_RETRIES = 10;
    static const uint32_t PORTAL_TIMEOUT = 180000;  // 3 minutes
    static const uint16_t WEB_PORT = 8080;
}

// --- Timing Configuration ---
static const uint32_t ANIMATION_INTERVAL = 1000;    // 1 second
static const uint32_t MEMORY_CHECK_INTERVAL = 60000;  // 1 minute
static const uint32_t WIFI_CHECK_INTERVAL = 1000;   // 1 second
static const uint32_t WEBSOCKET_CHECK_INTERVAL = 3000;   // 3 seconds
static const uint32_t TIME_CHECK_INTERVAL = 60000;   // 3 seconds
static const uint32_t MAIN_THREAD_CHECK_INTERVAL = 500;   // 3 seconds

static constexpr uint8_t  TYPE_ALERTS_BATCH = 0xA1;
static constexpr size_t   HEADER_SZ         = 1;  // лише 1 байт – type
static constexpr size_t   RECORD_SZ         = 4;  // 2B region_id + 2B flags16


// --- Region to LED mapping (fixed, задається один раз) ---
constexpr int MAX_REGIONS = 39;           // Кількість регіонів (змініть під свою задачу)
constexpr int MAX_LEDS_PER_REGION = 7;    // Максимум LED на регіон

enum Type {
    UNKNOWN = 0,
    DISTRICT,
    CITY,
    ID,
    DEVICE_NAME,
    DEVICE_DESCRIPTION,
    BROADCAST_NAME,
    WS_SERVER_HOST,
    WS_SERVER_PORT,
    UPDATE_SERVER_PORT,
    NTP_HOST,
    LEGACY,
    MAIN_LED_PIN,
    BG_LED_PIN,
    BG_LED_COUNT,
    SERVICE_LED_PIN,
    BUTTON_1_PIN,
    BUTTON_2_PIN,
    ALERT_PIN,
    CLEAR_PIN,
    BUZZER_PIN,
    DF_RX_PIN,
    DF_TX_PIN,
    LIGHT_SENSOR_PIN,
    POWER_PIN,
    WIFI_PIN,
    DATA_PIN,
    HA_PIN,
    UPD_AVAILABLE_PIN,
    ALERT_CLEAR_PIN_MODE,
    ALERT_CLEAR_PIN_TIME,
    HA_MQTT_PORT,
    HA_MQTT_USER,
    HA_MQTT_PASSWORD,
    HA_BROKER_ADDRESS,
    CURRENT_BRIGHTNESS,
    BRIGHTNESS,
    BRIGHTNESS_DAY,
    BRIGHTNESS_NIGHT,
    BRIGHTNESS_MODE,
    HOME_ALERT_TIME,
    COLOR_ALERT,
    COLOR_CLEAR,
    COLOR_NEW_ALERT,
    COLOR_ALERT_OVER,
    COLOR_EXPLOSION,
    COLOR_MISSILES,
    COLOR_KABS,
    COLOR_DRONES,
    COLOR_BALLISTIC,
    COLOR_HOME_DISTRICT,
    COLOR_BG_NEIGHBOR_ALERT,
    ENABLE_EXPLOSIONS,
    ENABLE_MISSILES,
    ENABLE_DRONES,
    ENABLE_KABS,
    ENABLE_BALLISTIC,
    BRIGHTNESS_ALERT,
    BRIGHTNESS_CLEAR,
    BRIGHTNESS_NEW_ALERT,
    BRIGHTNESS_ALERT_OVER,
    BRIGHTNESS_EXPLOSION,
    BRIGHTNESS_HOME_DISTRICT,
    BRIGHTNESS_BG,
    BRIGHTNESS_SERVICE,
    WEATHER_MIN_TEMP,
    WEATHER_MAX_TEMP,
    RADIATION_MAX,
    ALARMS_AUTO_SWITCH,
    HOME_DISTRICT,
    KYIV_DISTRICT_MODE,
    DISTRICT_MODE_KYIV,
    DISTRICT_MODE_KHARKIV,
    DISTRICT_MODE_ZP,
    KYIV_LED,
    MIGRATION_LED_MAPPING,
    SERVICE_DIODES_MODE,
    NEW_FW_NOTIFICATION,
    HA_LIGHT_BRIGHTNESS,
    HA_LIGHT_R,
    HA_LIGHT_G,
    HA_LIGHT_B,
    SOUND_SOURCE,
    SOUND_ON_MIN_OF_SL,
    SOUND_ON_ALERT,
    MELODY_ON_ALERT,
    TRACK_ON_ALERT,
    SOUND_ON_ALERT_END,
    MELODY_ON_ALERT_END,
    TRACK_ON_ALERT_END,
    SOUND_ON_EXPLOSION,
    MELODY_ON_EXPLOSION,
    TRACK_ON_EXPLOSION,
    SOUND_ON_CRITICAL_MIG,
    MELODY_ON_CRITICAL_MIG,
    TRACK_ON_CRITICAL_MIG,
    SOUND_ON_CRITICAL_STRATEGIC,
    MELODY_ON_CRITICAL_STRATEGIC,
    TRACK_ON_CRITICAL_STRATEGIC,
    SOUND_ON_CRITICAL_MIG_MISSILES,
    MELODY_ON_CRITICAL_MIG_MISSILES,
    TRACK_ON_CRITICAL_MIG_MISSILES,
    SOUND_ON_CRITICAL_STRATEGIC_MISSILES,
    MELODY_ON_CRITICAL_STRATEGIC_MISSILES,
    TRACK_ON_CRITICAL_STRATEGIC_MISSILES,
    SOUND_ON_CRITICAL_BALLISTIC_MISSILES,
    MELODY_ON_CRITICAL_BALLISTIC_MISSILES,
    TRACK_ON_CRITICAL_BALLISTIC_MISSILES,
    CRITICAL_NOTIFICATIONS_DISPLAY_TIME,
    ENABLE_CRITICAL_NOTIFICATIONS,
    SOUND_ON_EVERY_HOUR,
    SOUND_ON_BUTTON_CLICK,
    MUTE_SOUND_ON_NIGHT,
    IGNORE_MUTE_ON_ALERT,
    MELODY_VOLUME_NIGHT,
    MELODY_VOLUME_DAY,
    MELODY_VOLUME_CURRENT,
    INVERT_DISPLAY,
    DIM_DISPLAY_ON_NIGHT,
    MAP_MODE,
    DISPLAY_MODE,
    DISPLAY_MODE_TIME,
    TOGGLE_MODE_WEATHER,
    TOGGLE_MODE_ENERGY,
    TOGGLE_MODE_RADIATION,
    TOGGLE_MODE_TEMP,
    TOGGLE_MODE_HUM,
    TOGGLE_MODE_PRESS,
    BUTTON_1_MODE,
    BUTTON_2_MODE,
    BUTTON_1_MODE_LONG,
    BUTTON_2_MODE_LONG,
    USE_TOUCH_BUTTON_1,
    USE_TOUCH_BUTTON_2,
    ALARMS_NOTIFY_MODE,
    DISPLAY_MODEL,
    DISPLAY_WIDTH,
    DISPLAY_HEIGHT,
    DAY_START,
    NIGHT_START,
    WS_ALERT_TIME,
    WS_REBOOT_TIME,
    MIN_OF_SILENCE,
    FW_UPDATE_CHANNEL,
    TEMP_CORRECTION,
    HUM_CORRECTION,
    PRESSURE_CORRECTION,
    LIGHT_SENSOR_FACTOR,
    TIME_ZONE,
    ALERT_ON_TIME,
    ALERT_OFF_TIME,
    EXPLOSION_TIME,
    ALERT_BLINK_TIME,
};
struct RegionLedMapEntry {
    uint16_t region_id;
    int led_positions[MAX_LEDS_PER_REGION];
    uint8_t led_count;
};
struct SettingListItem {
  uint16_t id;
  const char* name;
  bool ignore;
};

static SettingListItem DISTRICTS[MAX_REGIONS] = {
    {9999, "АР Крим", false},
    {4, "Вінницька обл.", false},
    {8, "Волинська обл.", false},
    {9, "Дніпропетровська обл.", false},
    {42, "Кам'янський район", false},
    {43, "Новомосковський район", false},
    {44, "Дніпровський район", false},
    {45, "Павлоградський район", false},
    {46, "Криворізький район", false},
    {47, "Нікопольський район", false},
    {48, "Синельниківський район", false},
    {28, "Донецька обл.", false},
    {10, "Житомирська обл.", false},
    {11, "Закарпатська обл.", false},
    {12, "Запорізька обл.", false},
    {564, "м. Запоріжжя", false},
    {13, "Ів.-Франківська обл.", false},
    {14, "Київська обл.", false},
    {31, "м. Київ", false},
    {15, "Кіровоградська обл.", false},
    {150, "Звенигородський район", false},
    {151, "Уманський район", false},
    {152, "Черкаський район", false},
    {153, "Золотоніський район", false},
    {16, "Луганська обл.", false},
    {27, "Львівська обл.", false},
    {17, "Миколаївська обл.", false},
    {18, "Одеська обл.", false},
    {19, "Полтавська обл.", false},
    {5, "Рівненська обл.", false},
    {20, "Сумська обл.", false},
    {21, "Тернопільська обл.", false},
    {22, "Харківська обл.", false},
    {1293, "м. Харків", false},
    {23, "Херсонська обл.", false},
    {3, "Хмельницька обл.", false},
    {24, "Черкаська обл.", false},
    {26, "Чернівецька обл.", false},
    {25, "Чернігівська обл.", false},
};

// Фіксований масив відповідностей (заповнюється вручну)
const RegionLedMapEntry REGION_MAP_LED[MAX_REGIONS] = {
    { 11,   { 9 },  1 },    // Закарпатська область
    { 13,   { 10 }, 1 },    // Івано-Франківська область
    { 21,   { 11 }, 1 },    // Тернопільська область
    { 27,   { 12 }, 1 },    // Львівська область
    { 8,    { 13 }, 1 },    // Волинська область
    { 5,    { 14 }, 1 },    // Рівненська область
    { 10,   { 15 }, 1 },    // Житомирська область
    { 14,   { 16 }, 1 },    // Київська область
    { 25,   { 18 }, 1 },    // Чернігівська область
    { 20,   { 19 }, 1 },    // Сумська область
    { 22,   { 20 }, 1 },    // Харківська область
    { 16,   { 21 }, 1 },    // Луганська область
    { 28,   { 22 }, 1 },    // Донецька область
    { 12,   { 23 }, 1 },    // Запорізька область
    { 23,   { 24 }, 1 },    // Херсонська область
    {9999,  { 25 }, 1 },    // Автономна Республіка Крим
    { 18,   { 0 },  1 },    // Одеська область
    { 17,   { 1 },  1 },    // Миколаївська область
            
    { 9,    { 2 },  1 },    // Дніпропетровська область
    { 43,   { 2 },  1 },    // Новомосковський район
    { 44,   { 2 },  1 },    // Дніпровський район
    { 47,   { 2 },  1 },    // Нікопольський район
    { 48,   { 2 },  1 },    // Синельниківський район
    { 42,   { 2 },  1 },    // Кам’янський район
    { 45,   { 2 },  1 },    // Павлоградський район
    { 46,   { 2 },  1 },    // Криворізький район

    { 19,   { 3 },  1 },    // Полтавська область

    { 24,   { 4 },  1 },    // Черкаська область
    {153,   { 4 },  1 },    // Золотоніський район
    {152,   { 4 },  1 },    // Черкаський район
    {150,   { 4 },  1 },    // Звенигородський район
    {151,   { 4 },  1 },    // Уманський район

    { 15,   { 5 },  1 },    // Кіровоградська область
    { 4,    { 6 },  1 },    // Вінницька область
    { 3,    { 7 },  1 },    // Хмельницька область
    { 26,   { 8 },  1 },    // Чернівецька область
    { 31,   { 17 }, 1 },    // м. Київ
    {1293,  { 20 }, 1 },    // м. Київ
    {564,   { 23 }, 1 },    // м. Київ
};