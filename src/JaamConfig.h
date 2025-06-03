#pragma once

#include <Adafruit_NeoPixel.h>


// --- Default Colors ---
namespace DefaultColors {
    static const uint32_t MAIN_STRIP = 0x00FF00;  // Green
    static const uint32_t BG_STRIP = 0xFF0000;    // Red
    static const uint32_t SERVICE_STRIP = 0x0000FF;  // Blue
    static const uint32_t OFF = 0x000000;  // Blue
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
static constexpr size_t   HASH_SZ           = 4;  // 2B actual + 2B prev

// --- Region to LED mapping (fixed, задається один раз) ---
constexpr int MAX_REGIONS = 150;          // Кількість регіонів
constexpr int MAX_LEDS_PER_REGION = 7;    // Максимум LED на регіон

struct RegionLedMapEntry {
    uint16_t region_id;
    int led_positions[MAX_LEDS_PER_REGION];
    uint8_t led_count;
};
struct SettingListItem {
  uint16_t id;
  const char* name;
  bool ignore;
  bool sub;
};

static SettingListItem LED_COLOR_FORMATS[] = {
    {NEO_RGB, "NEO_RGB", false, false},
    {NEO_RBG, "NEO_RBG", false, false},
    {NEO_GRB, "NEO_GRB (рекомендовано)", false, false},
    {NEO_GBR, "NEO_GBR", false, false},
    {NEO_BRG, "NEO_BRG", false, false},
    {NEO_BGR, "NEO_BGR", false, false},
    {NEO_WRGB, "NEO_WRGB", false, false},
    {NEO_WGRB, "NEO_WGRB", false, false}
};

static SettingListItem LED_FREQUENCIES[] = {
    {NEO_KHZ400, "400 КГц", false, false},
    {NEO_KHZ800, "800 КГц (рекомендовано)", false, false}
};

constexpr int LED_COLOR_FORMATS_COUNT = 8;
constexpr int LED_FREQUENCIES_COUNT = 2; 


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
    MAIN_LED_COLOR_FORMAT,
    MAIN_LED_FREQUENCY,
    BG_LED_COLOR_FORMAT,
    BG_LED_FREQUENCY,
    SERVICE_LED_COLOR_FORMAT,
    SERVICE_LED_FREQUENCY
};

static SettingListItem DISTRICTS[MAX_REGIONS] = {
    // АР Крим
    {9999, "АР Крим", false, false},
    
    // Вінницька область та її райони
    {4, "Вінницька обл.", false, false},
    {32, "Тульчинський район", false, true},
    {35, "Жмеринський район", false, true},
    {36, "Вінницький район", false, true},
    {34, "Хмільницький район", false, true},
    {33, "Могилів-Подільський район", false, true},
    {37, "Гайсинський район", false, true},
    
    // Волинська область та її райони
    {8, "Волинська обл.", false, false},
    {39, "Луцький район", false, true},
    {38, "Володимир-Волинський район", false, true},
    {40, "Ковельський район", false, true},
    {41, "Камінь-Каширський район", false, true},
    
    // Дніпропетровська область та її райони
    {9, "Дніпропетровська обл.", false, false},
    {43, "Новомосковський район", false, true},
    {44, "Дніпровський район", false, true},
    {47, "Нікопольський район", false, true},
    {48, "Синельниківський район", false, true},
    {42, "Кам'янський район", false, true},
    {45, "Павлоградський район", false, true},
    {46, "Криворізький район", false, true},
    
    // Донецька область та її райони
    {28, "Донецька обл.", false, false},
    {56, "Покровський район", false, true},
    {51, "Горлівський район", false, true},
    {55, "Волноваський район", false, true},
    {53, "Донецький район", false, true},
    {49, "Кальміуський район", false, true},
    {52, "Маріупольський район", false, true},
    {50, "Краматорський район", false, true},
    {54, "Бахмутський район", false, true},
    
    // Житомирська область та її райони
    {10, "Житомирська обл.", false, false},
    {59, "Житомирський район", false, true},
    {58, "Коростенський район", false, true},
    {57, "Бердичівський район", false, true},
    {60, "Звягельський район", false, true},
    
    // Закарпатська область та її райони
    {11, "Закарпатська обл.", false, false},
    {66, "Ужгородський район", false, true},
    {61, "Берегівський район", false, true},
    {62, "Хустський район", false, true},
    {63, "Рахівський район", false, true},
    {64, "Тячівський район", false, true},
    {65, "Мукачівський район", false, true},
    
    // Запорізька область та її райони
    {12, "Запорізька обл.", false, false},
    {146, "Василівський район", false, true},
    {145, "Пологівський район", false, true},
    {149, "Запорізький район", false, true},
    {147, "Бердянський район", false, true},
    {148, "Мелітопольський район", false, true},
    {564, "м. Запоріжжя та Запорізька територіальна громада", false, true},
    
    // Івано-Франківська область та її райони
    {13, "Ів.-Франківська обл.", false, false},
    {68, "Івано-Франківський район", false, true},
    {67, "Верховинський район", false, true},
    {71, "Калуський район", false, true},
    {72, "Надвірнянський район", false, true},
    {70, "Коломийський район", false, true},
    {69, "Косівський район", false, true},
    
    // Київська область та її райони
    {14, "Київська обл.", false, false},
    {77, "Фастівський район", false, true},
    {73, "Білоцерківський район", false, true},
    {75, "Бучанський район", false, true},
    {76, "Обухівський район", false, true},
    {74, "Вишгородський район", false, true},
    {79, "Броварський район", false, true},
    {78, "Бориспільський район", false, true},
    {31, "м. Київ", false, true},
    
    // Кіровоградська область та її райони
    {15, "Кіровоградська обл.", false, false},
    {81, "Кропивницький район", false, true},
    {80, "Олександрійський район", false, true},
    {82, "Голованівський район", false, true},
    {83, "Новоукраїнський район", false, true},
    
    // Луганська область та її райони
    {16, "Луганська обл.", false, false},
    {86, "Старобільський район", false, true},
    {85, "Сватівський район", false, true},
    {84, "Сєвєродонецький район", false, true},
    {87, "Щастинський район", false, true},
    
    // Львівська область та її райони
    {27, "Львівська обл.", false, false},
    {90, "Львівський район", false, true},
    {89, "Стрийський район", false, true},
    {88, "Самбірський район", false, true},
    {91, "Дрогобицький район", false, true},
    {92, "Червоноградський район", false, true},
    {94, "Золочівський район", false, true},
    {93, "Яворівський район", false, true},
    
    // Миколаївська область та її райони
    {17, "Миколаївська обл.", false, false},
    {96, "Баштанський район", false, true},
    {95, "Вознесенський район", false, true},
    {97, "Первомайський район", false, true},
    {98, "Миколаївський район", false, true},
    
    // Одеська область та її райони
    {18, "Одеська обл.", false, false},
    {105, "Болградський район", false, true},
    {100, "Березівський район", false, true},
    {104, "Одеський район", false, true},
    {102, "Білгород-Дністровський район", false, true},
    {103, "Роздільнянський район", false, true},
    {101, "Ізмаїльський район", false, true},
    {99, "Подільський район", false, true},
    
    // Полтавська область та її райони
    {19, "Полтавська обл.", false, false},
    {107, "Кременчуцький район", false, true},
    {106, "Лубенський район", false, true},
    {109, "Полтавський район", false, true},
    {108, "Миргородський район", false, true},
    
    // Рівненська область та її райони
    {5, "Рівненська обл.", false, false},
    {110, "Вараський район", false, true},
    {111, "Дубенський район", false, true},
    {112, "Рівненський район", false, true},
    {113, "Сарненський район", false, true},
    
    // Сумська область та її райони
    {20, "Сумська обл.", false, false},
    {115, "Шосткинський район", false, true},
    {116, "Роменський район", false, true},
    {117, "Конотопський район", false, true},
    {114, "Сумський район", false, true},
    {118, "Охтирський район", false, true},
    
    // Тернопільська область та її райони
    {21, "Тернопільська обл.", false, false},
    {119, "Тернопільський район", false, true},
    {121, "Чортківський район", false, true},
    {120, "Кременецький район", false, true},
    
    // Харківська область та її райони
    {22, "Харківська обл.", false, false},
    {124, "Харківський район", false, true},
    {123, "Куп'янський район", false, true},
    {122, "Чугуївський район", false, true},
    {126, "Богодухівський район", false, true},
    {127, "Красноградський район", false, true},
    {125, "Ізюмський район", false, true},
    {128, "Лозівський район", false, true},
    {1293, "м. Харків та Харківська територіальна громада", false, true},
    
    // Херсонська область та її райони
    {23, "Херсонська обл.", false, false},
    {131, "Каховський район", false, true},
    {129, "Бериславський район", false, true},
    {130, "Скадовський район", false, true},
    {132, "Херсонський район", false, true},
    {133, "Генічеський район", false, true},
    
    // Хмельницька область та її райони
    {3, "Хмельницька обл.", false, false},
    {136, "Шепетівський район", false, true},
    {134, "Хмельницький район", false, true},
    {135, "Кам'янець-Подільський район", false, true},
    
    // Черкаська область та її райони
    {24, "Черкаська обл.", false, false},
    {153, "Золотоніський район", false, true},
    {152, "Черкаський район", false, true},
    {150, "Звенигородський район", false, true},
    {151, "Уманський район", false, true},
    
    // Чернівецька область та її райони
    {26, "Чернівецька обл.", false, false},
    {139, "Дністровський район", false, true},
    {138, "Вижницький район", false, true},
    {137, "Чернівецький район", false, true},
    
    // Чернігівська область та її райони
    {25, "Чернігівська обл.", false, false},
    {141, "Новгород-Сіверський район", false, true},
    {142, "Ніжинський район", false, true},
    {143, "Прилуцький район", false, true},
    {140, "Чернігівський район", false, true},
    {144, "Корюківський район", false, true},
};

// Фіксований масив відповідностей (заповнюється вручну)
const RegionLedMapEntry REGION_MAP_LED[MAX_REGIONS] = {
    // АР Крим
    {9999,  { 25 }, 1 },    // Автономна Республіка Крим
    
    // Вінницька область та її райони
    { 4,    { 6 },  1 },    // Вінницька область
    { 32,   { 6 },  1 },    // Тульчинський район
    { 35,   { 6 },  1 },    // Жмеринський район
    { 36,   { 6 },  1 },    // Вінницький район
    { 34,   { 6 },  1 },    // Хмільницький район
    { 33,   { 6 },  1 },    // Могилів-Подільський район
    { 37,   { 6 },  1 },    // Гайсинський район
    
    // Волинська область та її райони
    { 8,    { 13 }, 1 },    // Волинська область
    { 39,   { 13 }, 1 },    // Луцький район
    { 38,   { 13 }, 1 },    // Володимир-Волинський район
    { 40,   { 13 }, 1 },    // Ковельський район
    { 41,   { 13 }, 1 },    // Камінь-Каширський район
    
    // Дніпропетровська область та її райони
    { 9,    { 2 },  1 },    // Дніпропетровська область
    { 43,   { 2 },  1 },    // Новомосковський район
    { 44,   { 2 },  1 },    // Дніпровський район
    { 47,   { 2 },  1 },    // Нікопольський район
    { 48,   { 2 },  1 },    // Синельниківський район
    { 42,   { 2 },  1 },    // Кам'янський район
    { 45,   { 2 },  1 },    // Павлоградський район
    { 46,   { 2 },  1 },    // Криворізький район
    
    // Донецька область та її райони
    { 28,   { 22 }, 1 },    // Донецька область
    { 56,   { 22 }, 1 },    // Покровський район
    { 51,   { 22 }, 1 },    // Горлівський район
    { 55,   { 22 }, 1 },    // Волноваський район
    { 53,   { 22 }, 1 },    // Донецький район
    { 49,   { 22 }, 1 },    // Кальміуський район
    { 52,   { 22 }, 1 },    // Маріупольський район
    { 50,   { 22 }, 1 },    // Краматорський район
    { 54,   { 22 }, 1 },    // Бахмутський район
    
    // Житомирська область та її райони
    { 10,   { 15 }, 1 },    // Житомирська область
    { 59,   { 15 }, 1 },    // Житомирський район
    { 58,   { 15 }, 1 },    // Коростенський район
    { 57,   { 15 }, 1 },    // Бердичівський район
    { 60,   { 15 }, 1 },    // Звягельський район
    
    // Закарпатська область та її райони
    { 11,   { 9 },  1 },    // Закарпатська область
    { 66,   { 9 },  1 },    // Ужгородський район
    { 61,   { 9 },  1 },    // Берегівський район
    { 62,   { 9 },  1 },    // Хустський район
    { 63,   { 9 },  1 },    // Рахівський район
    { 64,   { 9 },  1 },    // Тячівський район
    { 65,   { 9 },  1 },    // Мукачівський район
    
    // Запорізька область та її райони
    { 12,   { 23 }, 1 },    // Запорізька область
    { 146,  { 23 }, 1 },    // Василівський район
    { 145,  { 23 }, 1 },    // Пологівський район
    { 149,  { 23 }, 1 },    // Запорізький район
    { 147,  { 23 }, 1 },    // Бердянський район
    { 148,  { 23 }, 1 },    // Мелітопольський район
    { 564,  { 23 }, 1 },    // м. Запоріжжя
    
    // Івано-Франківська область та її райони
    { 13,   { 10 }, 1 },    // Івано-Франківська область
    { 68,   { 10 }, 1 },    // Івано-Франківський район
    { 67,   { 10 }, 1 },    // Верховинський район
    { 71,   { 10 }, 1 },    // Калуський район
    { 72,   { 10 }, 1 },    // Надвірнянський район
    { 70,   { 10 }, 1 },    // Коломийський район
    { 69,   { 10 }, 1 },    // Косівський район
    
    // Київська область та її райони
    { 14,   { 16 }, 1 },    // Київська область
    { 77,   { 16 }, 1 },    // Фастівський район
    { 73,   { 16 }, 1 },    // Білоцерківський район
    { 75,   { 16 }, 1 },    // Бучанський район
    { 76,   { 16 }, 1 },    // Обухівський район
    { 74,   { 16 }, 1 },    // Вишгородський район
    { 79,   { 16 }, 1 },    // Броварський район
    { 78,   { 16 }, 1 },    // Бориспільський район
    { 31,   { 17 }, 1 },    // м. Київ
    
    // Кіровоградська область та її райони
    { 15,   { 5 },  1 },    // Кіровоградська область
    { 81,   { 5 },  1 },    // Кропивницький район
    { 80,   { 5 },  1 },    // Олександрійський район
    { 82,   { 5 },  1 },    // Голованівський район
    { 83,   { 5 },  1 },    // Новоукраїнський район
    
    // Луганська область та її райони
    { 16,   { 21 }, 1 },    // Луганська область
    { 86,   { 21 }, 1 },    // Старобільський район
    { 85,   { 21 }, 1 },    // Сватівський район
    { 84,   { 21 }, 1 },    // Сєвєродонецький район
    { 87,   { 21 }, 1 },    // Щастинський район
    
    // Львівська область та її райони
    { 27,   { 12 }, 1 },    // Львівська область
    { 90,   { 12 }, 1 },    // Львівський район
    { 89,   { 12 }, 1 },    // Стрийський район
    { 88,   { 12 }, 1 },    // Самбірський район
    { 91,   { 12 }, 1 },    // Дрогобицький район
    { 92,   { 12 }, 1 },    // Червоноградський район
    { 94,   { 12 }, 1 },    // Золочівський район
    { 93,   { 12 }, 1 },    // Яворівський район
    
    // Миколаївська область та її райони
    { 17,   { 1 },  1 },    // Миколаївська область
    { 96,   { 1 },  1 },    // Баштанський район
    { 95,   { 1 },  1 },    // Вознесенський район
    { 97,   { 1 },  1 },    // Первомайський район
    { 98,   { 1 },  1 },    // Миколаївський район
    
    // Одеська область та її райони
    { 18,   { 0 },  1 },    // Одеська область
    { 105,  { 0 },  1 },    // Болградський район
    { 100,  { 0 },  1 },    // Березівський район
    { 104,  { 0 },  1 },    // Одеський район
    { 102,  { 0 },  1 },    // Білгород-Дністровський район
    { 103,  { 0 },  1 },    // Роздільнянський район
    { 101,  { 0 },  1 },    // Ізмаїльський район
    { 99,   { 0 },  1 },    // Подільський район
    
    // Полтавська область та її райони
    { 19,   { 3 },  1 },    // Полтавська область
    { 107,  { 3 },  1 },    // Кременчуцький район
    { 106,  { 3 },  1 },    // Лубенський район
    { 109,  { 3 },  1 },    // Полтавський район
    { 108,  { 3 },  1 },    // Миргородський район
    
    // Рівненська область та її райони
    { 5,    { 14 }, 1 },    // Рівненська область
    { 110,  { 14 }, 1 },    // Вараський район
    { 111,  { 14 }, 1 },    // Дубенський район
    { 112,  { 14 }, 1 },    // Рівненський район
    { 113,  { 14 }, 1 },    // Сарненський район
    
    // Сумська область та її райони
    { 20,   { 19 }, 1 },    // Сумська область
    { 115,  { 19 }, 1 },    // Шосткинський район
    { 116,  { 19 }, 1 },    // Роменський район
    { 117,  { 19 }, 1 },    // Конотопський район
    { 114,  { 19 }, 1 },    // Сумський район
    { 118,  { 19 }, 1 },    // Охтирський район
    
    // Тернопільська область та її райони
    { 21,   { 11 }, 1 },    // Тернопільська область
    { 119,  { 11 }, 1 },    // Тернопільський район
    { 121,  { 11 }, 1 },    // Чортківський район
    { 120,  { 11 }, 1 },    // Кременецький район
    
    // Харківська область та її райони
    { 22,   { 20 }, 1 },    // Харківська область
    { 124,  { 20 }, 1 },    // Харківський район
    { 123,  { 20 }, 1 },    // Куп'янський район
    { 122,  { 20 }, 1 },    // Чугуївський район
    { 126,  { 20 }, 1 },    // Богодухівський район
    { 127,  { 20 }, 1 },    // Красноградський район
    { 125,  { 20 }, 1 },    // Ізюмський район
    { 128,  { 20 }, 1 },    // Лозівський район
    {1293,  { 20 }, 1 },    // м. Харків
    
    // Херсонська область та її райони
    { 23,   { 24 }, 1 },    // Херсонська область
    { 131,  { 24 }, 1 },    // Каховський район
    { 129,  { 24 }, 1 },    // Бериславський район
    { 130,  { 24 }, 1 },    // Скадовський район
    { 132,  { 24 }, 1 },    // Херсонський район
    { 133,  { 24 }, 1 },    // Генічеський район
    
    // Хмельницька область та її райони
    { 3,    { 7 },  1 },    // Хмельницька область
    { 136,  { 7 },  1 },    // Шепетівський район
    { 134,  { 7 },  1 },    // Хмельницький район
    { 135,  { 7 },  1 },    // Кам'янець-Подільський район
    
    // Черкаська область та її райони
    { 24,   { 4 },  1 },    // Черкаська область
    { 153,  { 4 },  1 },    // Золотоніський район
    { 152,  { 4 },  1 },    // Черкаський район
    { 150,  { 4 },  1 },    // Звенигородський район
    { 151,  { 4 },  1 },    // Уманський район
    
    // Чернівецька область та її райони
    { 26,   { 8 },  1 },    // Чернівецька область
    { 139,  { 8 },  1 },    // Дністровський район
    { 138,  { 8 },  1 },    // Вижницький район
    { 137,  { 8 },  1 },    // Чернівецький район
    
    // Чернігівська область та її райони
    { 25,   { 18 }, 1 },    // Чернігівська область
    { 141,  { 18 }, 1 },    // Новгород-Сіверський район
    { 142,  { 18 }, 1 },    // Ніжинський район
    { 143,  { 18 }, 1 },    // Прилуцький район
    { 140,  { 18 }, 1 },    // Чернігівський район
     { 144,  { 18 }, 1 },    // Корюківський район
};