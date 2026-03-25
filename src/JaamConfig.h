#pragma once
#include <Adafruit_NeoPixel.h>

// --- Default Colors ---
namespace DefaultColors {
    static const uint32_t MAIN_STRIP = 0x00FF00;  // Green
    static const uint32_t BG_STRIP = 0x0000FF;  // Blue
    static const uint32_t SERVICE_STRIP = 0xFFFFFF;  // White
    static const uint32_t OFF = 0x000000;  // Black
    static const uint32_t POWER = 0xFF0000;  // Red
    static const uint32_t WIFI = 0x0000FF;  // Blue
    static const uint32_t DATA = 0x00FF00;  // Green
    static const uint32_t API = 0xFFFF00;  // Yellow
    static const uint32_t UPD_AVAILABLE = 0xFFFFFF;  // White
    // Ukrainian flag colors
    static const uint32_t FLAG_BLUE = 0x0057B7;  // Blue
    static const uint32_t FLAG_YELLOW = 0xFFD700;  // Yellow
}
// --- ALERT Modes ---
namespace AlertModes {
    static const int NO_ALERT = -1;
    static const int ALERT = 0;
    static const int DRONES = 5;
    static const int MISSILES = 6;
    static const int KABS = 7;
    static const int BALLISTIC = 8;
    static const int EXPLOSION = 9;
    static const int RECON_DRONES = 10;
}

// --- MAP Modes ---
namespace MapModes {
    static const int OFF = 0;
    static const int ALERT = 1;
    static const int WEATHER = 2;
    static const int FLAG = 3;
    static const int RANDOM_COLORS = 4;
    static const int LAMP = 5;
}

// --- BG Led Modes ---
namespace BgLedModes {
    static const int HOME_REGION = 0;
    static const int CUSTOM_COLOR = 1;
    static const int COLOR_MAP = 2;
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

namespace AnimationTypes {
    static const int FADE = 0;
    static const int BLINK = 1;
    static const int BLEND_FADE = 2;
    static const int PULSE = 3;
    static const int ONE_WAY_BLEND_FADE = 4;
    static const int RUNNING_LIGHT = 5;
    static const int SET_BRIGHTNESS = 6;
    static const int OFF = 7;
    static const int COLOR_PULSE = 8;
    static const int COLOR_BLINK = 9;
}

// --- WiFi Configuration ---
namespace WiFiConfig {
    static const bool ENABLED = true;
    static const uint32_t CONNECT_TIMEOUT = 3;           // 10 seconds
    static const uint32_t CONNECT_RETRIES = 10;
    static const uint32_t PORTAL_TIMEOUT = 180;          // 3 minutes
    static const uint16_t WEB_PORT = 8080;
}

#define BH1750_POWER_PIN 19

// --- Timing Configuration ---
static const uint32_t ANIMATION_INTERVAL = 1000;            // 1 second
static const uint32_t MEMORY_CHECK_INTERVAL = 60000;        // 1 minute
static const uint32_t WIFI_CHECK_INTERVAL = 1000;           // 1 second
static const uint32_t WEBSOCKET_CHECK_INTERVAL = 3000;      // 3 seconds
static const uint32_t TIME_CHECK_INTERVAL = 60000;          // 1 minute
static const uint32_t MAIN_THREAD_CHECK_INTERVAL = 500;     // 0.5 seconds
static const uint32_t BATTERY_CHECK_INTERVAL = 10000;       // 10 seconds
static const uint32_t DISPLAY_CHECK_INTERVAL = 1000;        // 1 second
static const uint32_t CLIMATE_CHECK_INTERVAL = 10000;       // 10 seconds
static const uint32_t LIGHT_SENSOR_CHECK_INTERVAL = 1000;   // 1 second
static const uint32_t VOLUME_CHECK_INTERVAL = 1000;         // 1 second
static const uint32_t BEEP_HOUR_CHECK_INTERVAL = 1000;      // 1 second
static const uint32_t RAND_COLOR_MOD_ANIM_INTERVAL = 1000;  // 1 second

// --- Packet structure ---
static constexpr uint8_t  TYPE_ALERTS_BATCH = 0xA1;
static constexpr uint8_t  TYPE_NOTIFICATIONS_BATCH = 0xA2;
static constexpr uint8_t  TYPE_WEATHER_BATCH = 0xA3;
static constexpr uint8_t  TYPE_FIRMWARE_UPDATE_BETA_BATCH = 0xA6;
static constexpr uint8_t  TYPE_FIRMWARE_UPDATE_PROD_BATCH = 0xA7;
static constexpr size_t   HEADER_SZ         = 1;            // лише 1 байт – type
static constexpr size_t   RECORD_SZ         = 4;            // 2B region_id + 2B flags16
static constexpr size_t   RECORD_LZ         = 3;            // 2B region_id + 1B flags8
static constexpr size_t   RECORD_FW         = 5;            // 1B major + 1B minor + 1B patch + 2B beta
static constexpr size_t   HASH_SZ           = 4;            // 2B actual + 2B prev

// --- Region to LED mapping (fixed, задається один раз) ---
constexpr int MAX_REGIONS = 169;                            // Кількість регіонів
constexpr int MAX_LEDS_STRIP_MAIN = 500;                    // Максимальна кількість LED на strip_main
constexpr int MAX_LEDS_PER_REGION = 25;                      // Максимум LED на регіон

// Нова компактна структура метаданих (5 bytes замість 103)
struct RegionLedMapMeta {
    uint16_t region_id;      // ID регіону
    uint16_t start_index;    // Індекс початку в масиві led_positions
    uint8_t led_count;       // Кількість LED для цього регіону
};

// --- Background LED color mapping ---
constexpr int MAX_LEDS_STRIP_BG = 300;                            // Максимум задніх LED


// --- Service LED mapping ---
constexpr int MAX_LEDS_STRIP_SERVICE = 5;                    // Максимальна кількість LED на strip_service

struct BgLedColorEntry {
    uint16_t led_index;
    uint32_t color;
};

struct SettingListItem {
  int id;
  const char* name;
  bool ignore;
  bool sub;
  bool showDisabled;
};

// Структура для зберігання diff
struct AlertDiff {
    uint16_t region_id;
    uint16_t previous_flags;
    uint16_t current_flags;
    bool has_changes;
};

// --- Sound ---
enum SoundType {
  MIN_OF_SILINCE,
  MIN_OF_SILINCE_END,
  REGULAR,
  ALERT_ON,
  ALERT_OFF,
  EXPLOSIONS,
  DRONES,
  MISSILES,
  KABS,
  BALLISTIC,
  RECON_DRONES,
  CRITICAL_MIG,
  CRITICAL_STRATEGIC,
  CRITICAL_MIG_MISSILES,
  CRITICAL_BALLISTIC_MISSILES,
  CRITICAL_STRATEGIC_MISSILES,
  SINGLE_CLICK,
  LONG_CLICK
};

static const char UA_ANTHEM[]            PROGMEM = "UkraineAnthem:d=4,o=5,b=200:2d5,4d5,32p,4d5,32p,4d5,32p,4c5,4d5,4d#5,2f5,4f5,4d#5,2d5,2c5,2a#4,2d5,2a4,2d5,1g4,32p,1g4";
static const char OI_U_LUZI[]            PROGMEM = "OiULuzi:d=32,o=5,b=200:2d,32p,2d,2f.,4d,4e,4f,4e,4d,2c#,2a4,2d.,4e,2f,2e,2d.";
static const char COSSACKS_MARCH[]       PROGMEM = "CossacksMarch:d=32,o=5,b=200:2d.,8a4,8d,2f.,8d,8f,4d,8a4,8d,4f,8d,8f,4d,8a4,8d,4f,8d,8f,1d.";
static const char HARRY_POTTER[]         PROGMEM = "HarryPotter:d=8,o=6,b=100:b5,e.,16g,f#,4e,b,4a.,4f#.,e.,16g,f#,4d,f,2b5";
static const char SIREN[]                PROGMEM = "Siren:d=32,o=6,b=225:16c#,d,d#,4e.,d#,d,8c#,16c#,d,d#,4e.,d#,d,8c#,16c#,d,d#,4e.,d#,d,8c#";
static const char COMMUNICATION[]        PROGMEM = "Communicator:d=32,o=7,b=180:d#,e,g,d#,g,d#,f#,e,f,2p,d#,e,g,d#,g,d#,f#,e,f,2p,d#,e,g,d#,g,d#,f#,e,f";
static const char STAR_WARS[]            PROGMEM = "StarWars:d=4,o=5,b=180:8f,8f,8f,2a#.,2f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8d#6,2c6";
static const char IMPERIAL_MARCH[]       PROGMEM = "ImperialMarch:d=4,o=5,b=112:8d.,16p,8d.,16p,8d.,16p,8a#4,16p,16f,8d.,16p,8a#4,16p,16f,d.,8p,8a.,16p,8a.,16p,8a.,16p,8a#,16p,16f,8c#.,16p,8a#4,16p,16f,d.";
static const char STAR_TRACK[]           PROGMEM = "StarTrek:d=4,o=5,b=63:32p,8f.,16a#,d#.6,8d6,16a#.,16g.,16c.6,f6";
static const char INDIANA_JONES[]        PROGMEM = "IndianaJones:d=4,o=5,b=250:e,8p,8f,8g,8p,2c.6,8p.,d,8p,8e,1f,p.,g,8p,8a,8b,8p,2f.6,p,a,8p,8b,2c6,2d6,2e6";
static const char BACK_TO_THE_FUTURE[]   PROGMEM = "BackToTheFuture:d=4,o=6,b=180:2c,8b5,8a5,b5,a5,g5,1a5,p,d,2c,8b5,8a5,b5,a5,g5,1a5";
static const char KISS_I_WAS_MADE[]      PROGMEM = "KissIWasMade:d=4,o=5,b=125:c6,d6,8d#6,8p,8f6,8g6,8p,8g6,f6,d#6,d6,c6,d6,8d#6,8p,8f6,8g6,8p,8g6,f.6";
static const char THE_LITTLE_MERMAID[]   PROGMEM = "TheLittleMermaid:d=32,o=7,b=100:16c5,16f5,16a5,16c6,16p,16c6,16p,16c6,8a#5,8d6,8c6,8a5,16f4,16a4,16c5,16f5,16p,16f5,16p,16f5,8e5,8g5,8f5";
static const char NOKIA_TUN[]            PROGMEM = "NokiaTun:d=4,o=5,b=225:8e6,8d6,f#,g#,8c#6,8b,d,e,8b,8a,c#,e,2a";
static const char PACKMAN[]              PROGMEM = "Pacman:d=32,o=5,b=112:32p,b,p,b6,p,f#6,p,d#6,p,b6,f#6,16p,16d#6,16p,c6,p,c7,p,g6,p,e6,p,c7,g6,16p,16e6,16p,b,p,b6,p,f#6,p,d#6,p,b6,f#6,16p,16d#6,16p,d#6,e6,f6,p,f6,f#6,g6,p,g6,g#6,a6,p,b.6";
static const char SHCHEDRYK[]            PROGMEM = "Shchedryk:d=8,o=5,b=180:4a,g#,a,4f#,4a,g#,a,4f#";
static const char XMEN[]                 PROGMEM = "XMen:d=4,o=6,b=125:16d#4,16g4,16c5,16d#5,4d5,8c5,8g4,4p,16d#4,16g4,16c5,16d#5,4d5,8c5,8g#4,4p,16d#4,16g4,16c5,16d#5,4d5,8c5,8d#5,2p,8d5,8c5,8g5,16g5,32a5,32b5,4c6";
static const char AVENGERS[]             PROGMEM = "Avengers:d=16,o=6,b=70:4e4,4p.,16e4,16p,8e4,16p,16b4,4a4,4p,4g4,4f#4,16d4,16e4,8p,16e4,16f#4,8p,16d5,16e5,8p,16e5,16f#5,8p,4e4";
static const char SIREN2[]               PROGMEM = "Siren2:d=4,o=5,b=200:a.,8g#,a.,8g#,a.,8g#";
static const char SIREN3[]               PROGMEM = "Siren3:o=6,d=4,b=100:8b5,8d,8b5,8d,8b5,8d";
static const char SIREN4[]               PROGMEM = "Siren4:o=5,d=4,b=200:16a,16b,16a,16b,16a,16b,16a,16b,16a,16b,16a,16b,16a,16b,16a,16b,16a,16b";
static const char SQUIDGAME[]            PROGMEM = "SquidGame:d=32,o=4,b=200:8f,32p,8f,32p,8f,32p,4d,32p,8d#.,4f.,4p.,8f,32p,8f,32p,8f,32p,4d,32p,8d#.,4f.,4p.,4g,32p,8g.,32p,4g,32p,8c5.,32p,4a#,32p,8a.,4g,32p,8a.,4a#.,16p.,4a#.,16p.,4a#.";
static const char BANDERA[]              PROGMEM = "Bandera:d=32,o=4,b=140:8e,32p,8e,8c5,8b,8a,32p,8a,4p,8c5,32p,8c5,8d5,8c5,4b.,32p,8b,32p,8b,8b,8b,8b,8e5,8d5,8c5,8b,4a,32p,8a.,16a,32p,8a,8a";
static const char HUILO[]                PROGMEM = "Huilo:d=32,o=4,b=150:8e5,8p,4e5,4d5,2c5,2p5,8g,8c5,8d5,8e5,8d5,8c5,8e5,2a,2p,8g,8a,8b,8c5,8b,8a,8e,2f,2p,8e,8f,8e,8f,8e,8f,8f#,2g";
static const char HELLDIVERS[]           PROGMEM = "Helldivers:d=4,o=5,b=120:8f,8e,8d,1a4,4a4,4p,4c.,1d,2p,8f,8e,8d,1f,8c.,32p,8c.,8d.,4a,8d,2g";
static const char SIREN5[]               PROGMEM = "Siren5:d=16,o=5,b=200:c6,f6,c7,c6,f6,c7,c6,f6,c7,8p,c,f,c6,c,f,c6,c,f,c6";
static const char SIREN6[]               PROGMEM = "Siren6:d=16,o=6,b=160:8d,p,2d,p,8d,p,2d,p,8d,p,2d";
static const char SIREN7[]               PROGMEM = "Siren7:d=4,o=5,b=140:16c,16e,16g,16a,16c,16e,16g,16a,16c,16e,16g,16a";
static const char SIREN8[]               PROGMEM = "Siren8:=8,o=5,b=300:c,e,g,c,e,g,c,e,g,c6,e6,g6,c6,e6,g6,c6,e6,g6,c7,e7,g7,c7,e7,g7,c7,e7,g7";

static const char CLOCK_BEEP[]           PROGMEM = "ClockBeep:d=8,o=7,b=300:4g,32p,4g";
static const char MOS_BEEP[]             PROGMEM = "MosBeep:d=4,o=4,b=250:g";
static const char SINGLE_CLICK_SOUND[]   PROGMEM = "SingleClick:d=8,o=4,b=300:f";
static const char LONG_CLICK_SOUND[]     PROGMEM = "LongClick:d=8,o=4,b=300:4f";

constexpr int MELODIES_COUNT = 29;
static const char* MELODIES[MELODIES_COUNT] PROGMEM = {
  UA_ANTHEM,
  OI_U_LUZI,
  COSSACKS_MARCH,
  HARRY_POTTER,
  SIREN,
  COMMUNICATION,
  STAR_WARS,
  IMPERIAL_MARCH,
  STAR_TRACK,
  INDIANA_JONES,
  BACK_TO_THE_FUTURE,
  KISS_I_WAS_MADE,
  THE_LITTLE_MERMAID,
  NOKIA_TUN,
  PACKMAN,
  SHCHEDRYK,
  XMEN,
  AVENGERS,
  SIREN2,
  SQUIDGAME,
  BANDERA,
  HUILO,
  HELLDIVERS,
  SIREN3,
  SIREN4,
  SIREN5,
  SIREN6,
  SIREN7,
  SIREN8,
};

static SettingListItem MELODY_NAMES[MELODIES_COUNT] PROGMEM = {
  {0, "Гімн України", false},
  {20, "Батько наш Бандера", false},
  {15, "Щедрик", false},
  {1, "Ой у лузі", false},
  {2, "Козацький марш", false},
  {4, "Сирена 1", false},
  {18, "Сирена 2", false},
  {23, "Сирена 3", false},
  {24, "Сирена 4", false},
  {25, "Сирена 5", false},
  {26, "Сирена 6", false},
  {27, "Сирена 7", false},
  {28, "Сирена 8", false},
  {5, "Комунікатор", false},
  {3, "Гаррі Поттер", false},
  {6, "Зоряні війни", false},
  {7, "Імперський марш", false},
  {8, "Зоряний шлях", false},
  {9, "Індіана Джонс", false},
  {10, "Назад у майбутнє", false},
  {11, "Kiss - I Was Made", false},
  {12, "Little Mermaid - Under the Sea", false},
  {13, "Рінгтон Nokia", false},
  {14, "Пакмен", false},
  {16, "Люди Х", false},
  {17, "Месники", false},
  {19, "Squid Game", false},
  {21, "ПТН ХЙЛ", false},
  {22, "Helldivers 2 - A cup of Liber-Tea", false}
};

static const String DF_CLOCK_BEEP = "/01.mp3";
static const String DF_CLOCK_TICK = "/02.mp3";
static const String DF_UA_ANTHEM = "/03.mp3";
static const String DF_SIREN_1 = "/04.mp3";
static const String DF_SIREN_2 = "/05.mp3";
static const String DF_SIREN_3 = "/06.mp3";
static const String DF_SIREN_4 = "/07.mp3";
static const String DF_SIREN_5 = "/08.mp3";
static const String DF_SIREN_6 = "/09.mp3";
static const String DF_SIREN_7 = "/10.mp3";
static const String DF_SIREN_8 = "/11.mp3";
static const String DF_SIREN_9 = "/12.mp3";
static const String DF_SIREN_10 = "/13.mp3";
static const String DF_THE_HOBBIT = "/14.mp3";
static const String DF_THE_MATRIX = "/15.mp3";
static const String DF_AVENGERS = "/16.mp3";
static const String DF_TERMINATOR_SHORT = "/17.mp3";
static const String DF_PIRATES_OF_THE_CARRIBEAN = "/18.mp3";
static const String DF_SIREN_11 = "/19.mp3";
static const String DF_NOTIFICATION_NEWS = "/20.mp3";
static const String DF_GOOD_MORNING_VIETNAM = "/21.mp3";
static const String DF_NOTIFICATION_R2D2 = "/22.mp3";
static const String DF_NOTIFICATION_STARTREK = "/23.mp3";
static const String DF_AIR_RAID_1 = "/24.mp3";
static const String DF_CAROL_OF_THE_BELLS = "/25.mp3";
static const String DF_NOTIFICATION_BACK_TO_THE_FUTURE = "/26.mp3";
static const String DF_IMPERIAL_MARCH = "/27.mp3";
static const String DF_GOOD_BAD_UGLY = "/28.mp3";
static const String DF_HARRY_POTTER = "/29.mp3";
static const String DF_MARCH = "/30.mp3";
static const String DF_MANDALORIAN_CALL = "/31.mp3";
static const String DF_MARIO = "/32.mp3";
static const String DF_PACMAN = "/33.mp3";
static const String DF_HELLDIVERS = "/34.mp3";  

constexpr int TRACKS_COUNT = 3;
static String TRACKS[TRACKS_COUNT] = {
  DF_CLOCK_TICK,
  DF_CLOCK_BEEP,
  DF_UA_ANTHEM
};

static SettingListItem TRACK_NAMES[TRACKS_COUNT] PROGMEM = {
  {0, "Годинникова стрілка", false},
  {1, "Годинник", false},
  {2, "Гімн України", false}
};

constexpr int SOUND_SOURCES_COUNT = 3;
static SettingListItem SOUND_SOURCES[SOUND_SOURCES_COUNT] PROGMEM = {
  {-1, "Вимкнено", false},
  {0, "Buzzer", false},
  {1, "DF Player Pro", false}
};

// --- Other Settings ---
constexpr int LED_COLOR_FORMATS_COUNT = 8;
static SettingListItem LED_COLOR_FORMATS[] = {
    {NEO_RGB, "NEO_RGB"},
    {NEO_RBG, "NEO_RBG"},
    {NEO_GRB, "NEO_GRB (рекомендовано)"},
    {NEO_GBR, "NEO_GBR"},
    {NEO_BRG, "NEO_BRG"},
    {NEO_BGR, "NEO_BGR"},
    {NEO_WRGB, "NEO_WRGB"},
    {NEO_WGRB, "NEO_WGRB"}
};

constexpr int LED_FREQUENCIES_COUNT = 2; 
static SettingListItem LED_FREQUENCIES[] = {
    {NEO_KHZ400, "400 КГц"},
    {NEO_KHZ800, "800 КГц (рекомендовано)"}
};

constexpr int SINGLE_CLICKS_COUNT = 4;
static SettingListItem SINGLE_CLICKS[] = {
  {0, "Вимкнено"},
  {1, "Перемикання режимів мапи"},
  {2, "Перемикання режимів дисплею"},
  {3, "Увімк./Вимк. мапу"},
  // {4, "Увімк./Вимк. дисплей"},
  // {5, "Увімк./Вимк. мапу та дисплей"},
  // {6, "Увімк./Вимк. нічний режим"},
  // {7, "Увімк./Вимк. режим лампи", false},
};

constexpr int LONG_CLICKS_COUNT = 5;
static SettingListItem LONG_CLICKS[] = {
  {0, "Вимкнено"},
  {1, "Перемикання режимів мапи"},
  {2, "Перемикання режимів дисплею"},
  {3, "Увімк./Вимк. мапу"},
  // {4, "Увімк./Вимк. дисплей"},
  // {5, "Увімк./Вимк. мапу та дисплей"},
  // {6, "Увімк./Вимк. нічний режим"},
  // {8, "Збільшити яскравість лампи"},
  // {9, "Зменшити яскравість лампи"},
  {10, "Перезавантаження пристрою"},
};

constexpr int MAP_MODES_COUNT = 6;
static SettingListItem MAP_MODES[] = {
  {0, "Вимкнено"},
  {1, "Тривога"},
  // {6, "Енергосистема", false},
  {2, "Погода"},
  // {7, "Радіація", false},
  {3, "Прапор"},
  {4, "Випадкові кольори"},
  {5, "Лампа"},
};

constexpr int ALERT_CLEAR_PIN_MODES_COUNT = 2;
static SettingListItem ALERT_CLEAR_PIN_MODES[] = {
  {0, "Бістабільний"},
  {1, "Імпульсний"},
};

constexpr int PIN_LEVELS_COUNT = 2;
static SettingListItem PIN_LEVELS[] = {
  {0, "LOW"},
  {1, "HIGH"},
};

constexpr int TIMEZONES_COUNT = 23;
static SettingListItem TIMEZONES[] = {
  {0, "Europe/Kyiv (UTC+2)"},
  // Європейські часові пояси (географічно: захід → схід)
  {1, "Europe/London (UTC+0)"},
  {2, "Europe/Paris (UTC+1)"},
  {3, "Europe/Istanbul (UTC+3)"},
  // Американські часові пояси (географічно: захід → схід)
  {4, "Pacific/Honolulu (UTC-10)"},
  {5, "America/Anchorage (UTC-9)"},
  {6, "America/Los_Angeles (UTC-8)"},
  {7, "America/Denver (UTC-7)"},
  {8, "America/Chicago (UTC-6)"},
  {9, "America/Mexico_City (UTC-6)"},
  {10, "America/New_York (UTC-5)"},
  {11, "America/Lima (UTC-5)"},
  // Південноамериканські пояси
  {12, "America/Sao_Paulo (UTC-3)"},
  // Азіатські та Близькосхідні пояси (географічно: захід → схід)
  {13, "Asia/Dubai (UTC+4)"},
  {14, "Asia/Karachi (UTC+5)"},
  {15, "Asia/Kolkata (UTC+5:30)"},
  {16, "Asia/Bangkok (UTC+7)"},
  {17, "Asia/Shanghai (UTC+8)"},
  {18, "Asia/Tokyo (UTC+9)"},
  // Австралія та Океанія (географічно: захід → схід)
  {19, "Australia/Brisbane (UTC+10)"},
  {20, "Australia/Sydney (UTC+10)"},
  {21, "Pacific/Auckland (UTC+12)"},
  {22, "Pacific/Fiji (UTC+12)"},
};

struct TimezoneInfo {
    int id;          // ID з TIMEZONES
    int offset;      // Години
    int minutes;     // Хвилини
    bool hasDST;     // Чи використовується DST
    // DST параметри: month, week, dayOfWeek, hour
    int dstStart[4]; // Початок літнього часу
    int dstEnd[4];   // Кінець літнього часу
};

static TimezoneInfo TIMEZONE_OFFSETS[] = {
  // ID 0: Europe/Kyiv - Європейський східний час (EEST)
  {0, 2, 0, true, {3, 0, 7, 3}, {10, 0, 7, 4}},
  // ID 1: Europe/London - британський літній час (BST)
  {1, 0, 0, true, {3, 0, 7, 1}, {10, 0, 7, 2}},
  // ID 2: Europe/Paris - Європейський центральний літній час (CEST)
  {2, 1, 0, true, {3, 0, 7, 2}, {10, 0, 7, 3}},
  // ID 3: Europe/Istanbul - немає DST з 2016
  {3, 3, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 4: Pacific/Honolulu - немає DST
  {4, -10, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 5: America/Anchorage
  {5, -9, 0, true, {3, 2, 7, 2}, {11, 1, 7, 2}},
  // ID 6: America/Los_Angeles
  {6, -8, 0, true, {3, 2, 7, 2}, {11, 1, 7, 2}},
  // ID 7: America/Denver
  {7, -7, 0, true, {3, 2, 7, 2}, {11, 1, 7, 2}},
  // ID 8: America/Chicago
  {8, -6, 0, true, {3, 2, 7, 2}, {11, 1, 7, 2}},
  // ID 9: America/Mexico_City - немає DST
  {9, -6, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 10: America/New_York - 2-а неділя березня, 1-а неділя листопада
  {10, -5, 0, true, {3, 2, 7, 2}, {11, 1, 7, 2}},
  // ID 11: America/Lima - немає DST
  {11, -5, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 12: America/Sao_Paulo - немає DST
  {12, -3, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 13: Asia/Dubai - немає DST
  {13, 4, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 14: Asia/Karachi - немає DST
  {14, 5, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 15: Asia/Kolkata - немає DST
  {15, 5, 30, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 16: Asia/Bangkok - немає DST
  {16, 7, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 17: Asia/Shanghai - немає DST
  {17, 8, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 18: Asia/Tokyo - немає DST
  {18, 9, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 19: Australia/Brisbane - немає DST
  {19, 10, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 20: Australia/Sydney - 1-а неділя жовтня, 1-а неділя квітня
  {20, 10, 0, true, {10, 1, 7, 2}, {4, 1, 7, 3}},
  // ID 21: Pacific/Auckland - остання неділя вересня, 1-а неділя квітня
  {21, 12, 0, true, {9, 0, 7, 2}, {4, 1, 7, 3}},
  // ID 22: Pacific/Fiji - немає DST
  {22, 12, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
};

constexpr int DISPLAY_MODES_COUNT = 6;
static SettingListItem DISPLAY_MODES[] = {
  {0, "Вимкнено", false},
  {1, "Годинник", false},
  // {5, "Енергосистема", false},
  {2, "Погода", false},
  // {6, "Радіація", false},
  {3, "Технічна інформація", false},
  {4, "Мікроклімат", false},
  {9, "Комбінований", false},
};

constexpr int AUTO_BRIGHTNESS_OPTIONS_COUNT = 3;
static SettingListItem AUTO_BRIGHTNESS_MODES[] = {
  {0, "Вимкнено"},
  {1, "День/Ніч"},
  {2, "Сенсор освітлення"}
};

constexpr int BG_LED_MODES_COUNT = 3;
static SettingListItem BG_LED_MODES[] = {
  {0, "Домашній Регіон"},
  {1, "Власний колір"},
  {2, "Індивідуальні кольори"},
};

constexpr int DISPLAY_TYPES_COUNT = 4;
static SettingListItem DISPLAY_TYPES[] = {
  {0, "Відключено"},
  {1, "SSD1306"},
  {2, "SH1106G"},
  {3, "SH1107"},
};

constexpr int DISPLAY_HEIGHT_COUNT = 2;
static SettingListItem DISPLAY_HEIGHTS[] = {
  {32, "32"},
  {64, "64"},
};

constexpr int DISPLAY_ROTATION_COUNT = 4;
static SettingListItem DISPLAY_ROTATIONS[] = {
  {0, "0°"},
  {90, "90°"},
  {180, "180°"},
  {270, "270°"},
};

constexpr int CLOCK_FONTS_COUNT = 6;
static SettingListItem CLOCK_FONTS[] = {
  {0, "Reddit"},
  {1, "Victor"},
  {2, "M PLUS 1 CODE"},
  {3, "Old Standard"},
  {4, "DSEG7"},
  {5, "Bitcount Grid"},
};

enum HARDWARE {
    JAAM_1_3 = 0,
    ZAKARPATTIA = 1,
    ODESA = 2,
    JAAM_2_1 = 3,
    JAAM_3_0 = 4,
    CUSTOM_MAPPING = 5,
    JAAM_3_2 = 6,
    ZAKARPATTIA_KYIV = 7,
    ODESA_KYIV = 8
};

#if ARDUINO_ESP32_DEV
#define HARDWARE_OPTIONS_COUNT 9
#else
#define HARDWARE_OPTIONS_COUNT 5
#endif
static SettingListItem HARDWARE_OPTIONS[HARDWARE_OPTIONS_COUNT] = {
#if ARDUINO_ESP32_DEV
  {HARDWARE::JAAM_1_3, "Плата JAAM 1.3"},
  {HARDWARE::JAAM_2_1, "Плата JAAM 2.1"},
  {HARDWARE::JAAM_3_0, "Плата JAAM 3.0"},
  {HARDWARE::JAAM_3_2, "Плата JAAM 3.2"},
#endif
  {HARDWARE::ZAKARPATTIA, "Початок на Закарпатті"},
  {HARDWARE::ZAKARPATTIA_KYIV, "Початок на Закарпатті + Київ"},
  {HARDWARE::ODESA, "Початок на Одещині"},
  {HARDWARE::ODESA_KYIV, "Початок на Одещині + Київ"},
  {HARDWARE::CUSTOM_MAPPING, "Власна карта LED"},
};

constexpr int ANIMATION_TYPES_COUNT = 10;
static SettingListItem ANIMATION_TYPES[] = {
  {AnimationTypes::OFF, "Без анімації"},
  {AnimationTypes::FADE, "Циклічне затухання"},
  {AnimationTypes::BLINK, "Мерехтіння"},
  {AnimationTypes::COLOR_BLINK, "Кольорове мерехтіння"},
  {AnimationTypes::BLEND_FADE, "Перехід між кольорами"},
  {AnimationTypes::ONE_WAY_BLEND_FADE, "Односторонній перехід між кольорами"},
  {AnimationTypes::PULSE, "Пульсація"},
  {AnimationTypes::COLOR_PULSE, "Кольорова пульсація"},
  {AnimationTypes::RUNNING_LIGHT, "RUNNING_LIGHT", true},
  {AnimationTypes::SET_BRIGHTNESS, "SET_BRIGHTNESS", true}
};

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
    HARDWARE,
    MAIN_LED_PIN,
    MAIN_LED_COUNT,
    BG_LED_PIN,
    BG_LED_COUNT,
    BG_LED_MODE,
    SERVICE_LED_PIN,
    BUTTON_1_PIN,
    BUTTON_2_PIN,
    BUTTON_3_PIN,
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
    ALERT_PIN_ACTIVE_LEVEL,
    ALERT_PIN_2,
    CLEAR_PIN_2,
    ALERT_CLEAR_PIN_MODE_2,
    ALERT_CLEAR_PIN_TIME_2,
    ALERT_PIN_ACTIVE_LEVEL_2,
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
    COLOR_RECON_DRONES,
    COLOR_BALLISTIC,
    COLOR_HOME_DISTRICT,
    COLOR_BG_NEIGHBOR_ALERT,
    COLOR_BG,
    ENABLE_EXPLOSIONS,
    ENABLE_MISSILES,
    ENABLE_DRONES,
    ENABLE_RECON_DRONES,
    ENABLE_KABS,
    ENABLE_BALLISTIC,
    ENABLE_SYNC_ANIMATIONS,
    BRIGHTNESS_ALERT,
    BRIGHTNESS_CLEAR,
    BRIGHTNESS_NEW_ALERT,
    BRIGHTNESS_ALERT_OVER,
    BRIGHTNESS_EXPLOSION,
    BRIGHTNESS_MISSILES,
    BRIGHTNESS_DRONES,
    BRIGHTNESS_RECON_DRONES,
    BRIGHTNESS_KABS,
    BRIGHTNESS_BALLISTIC,
    BRIGHTNESS_HOME_DISTRICT,
    BRIGHTNESS_BG,
    BRIGHTNESS_SERVICE,
    BRIGHTNESS_ANIMATION_END,
    BRIGHTNESS_MIN,
    NIGHT_MODE_LIGHT_THRESHOLD,
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
    DISPLAY_OFF_AT_NIGHT,
    TOGGLE_MODE_WEATHER,
    TOGGLE_MODE_ENERGY,
    TOGGLE_MODE_RADIATION,
    TOGGLE_MODE_TEMP,
    TOGGLE_MODE_HUM,
    TOGGLE_MODE_PRESS,
    BUTTON_1_MODE,
    BUTTON_2_MODE,
    BUTTON_3_MODE,
    BUTTON_1_MODE_LONG,
    BUTTON_2_MODE_LONG,
    BUTTON_3_MODE_LONG,
    USE_TOUCH_BUTTON_1,
    USE_TOUCH_BUTTON_2,
    USE_TOUCH_BUTTON_3,
    ALARMS_NOTIFY_MODE,
    DISPLAY_MODEL,
    DISPLAY_WIDTH,
    DISPLAY_HEIGHT,
    DISPLAY_ROTATION,
    DISPLAY_ALERT_MESSAGE_TIME,
    CLOCK_FONT,
    DAY_START,
    NIGHT_START,
    WS_ALERT_TIME,
    WS_REBOOT_TIME,
    MIN_OF_SILENCE,
    LOGS_ENABLED,
    FW_UPDATE_CHANNEL,
    TEMP_CORRECTION,
    HUM_CORRECTION,
    PRESSURE_CORRECTION,
    LIGHT_SENSOR_FACTOR,
    TIME_ZONE,
    ALERT_ON_TIME,
    ALERT_OFF_TIME,
    DRONE_TIME,
    RECON_DRONE_TIME,
    MISSILE_TIME,
    KAB_TIME,
    BALLISTIC_TIME,
    EXPLOSION_TIME,
    ALERT_BLINK_TIME,
    MAIN_LED_COLOR_FORMAT,
    MAIN_LED_FREQUENCY,
    BG_LED_COLOR_FORMAT,
    BG_LED_FREQUENCY,
    SERVICE_LED_COLOR_FORMAT,
    SERVICE_LED_FREQUENCY,
    BATTERY_PIN,
    ENABLE_BATTERY_MONITORING,
    ANIMATION_ALERT_ON_TYPE,
    ANIMATION_ALERT_OFF_TYPE,
    ANIMATION_DRONE_TYPE,
    ANIMATION_RECON_DRONE_TYPE,
    ANIMATION_MISSILE_TYPE,
    ANIMATION_KAB_TYPE,
    ANIMATION_BALLISTIC_TYPE,
    ANIMATION_EXPLOSION_TYPE,
    ANIMATION_ALERT_ON_CYCLE_TIME,
    ANIMATION_ALERT_OFF_CYCLE_TIME,
    ANIMATION_DRONE_CYCLE_TIME,
    ANIMATION_RECON_DRONE_CYCLE_TIME,
    ANIMATION_MISSILE_CYCLE_TIME,
    ANIMATION_KAB_CYCLE_TIME,
    ANIMATION_BALLISTIC_CYCLE_TIME,
    ANIMATION_EXPLOSION_CYCLE_TIME,
    ENABLE_ANIMATION_PREVIEW,
    SOUND_ON_DRONES,
    MELODY_ON_DRONES,
    TRACK_ON_DRONES,
    SOUND_ON_MISSILES,
    MELODY_ON_MISSILES,
    TRACK_ON_MISSILES,
    SOUND_ON_KABS,
    MELODY_ON_KABS,
    TRACK_ON_KABS,
    SOUND_ON_BALLISTIC,
    MELODY_ON_BALLISTIC,
    TRACK_ON_BALLISTIC,
    SOUND_ON_RECON_DRONES,
    MELODY_ON_RECON_DRONES,
    TRACK_ON_RECON_DRONES,
    COLOR_LAMP,
    BRIGHTNESS_LAMP,
    API_ENABLED,
    API_PORT,
    BRIGHTNESS_MAX,
    BRIGHTNESS_MAX_ACCEPT,
};

// --- Адміністративні одиниці України (області + обласні центри + м. Київ + АР Крим) ---
static const uint8_t ADMIN_UNITS_COUNT = 28;
static const uint16_t ADMIN_UNITS[ADMIN_UNITS_COUNT] = {
    9999,  // АР Крим
    3,     // Хмельницька обл.
    4,     // Вінницька обл.
    5,     // Рівненська обл.
    8,     // Волинська обл.
    9,     // Дніпропетровська обл.
    10,    // Житомирська обл.
    11,    // Закарпатська обл.
    12,    // Запорізька обл.
    564,   // м. Запоріжжя
    13,    // Івано-Франківська обл.
    14,    // Київська обл.
    31,    // м. Київ
    15,    // Кіровоградська обл.
    16,    // Луганська обл.
    17,    // Миколаївська обл.
    18,    // Одеська обл.
    19,    // Полтавська обл.
    20,    // Сумська обл.
    21,    // Тернопільська обл.
    22,    // Харківська обл.
    1293,  // м. Харків
    23,    // Херсонська обл.
    24,    // Черкаська обл.
    25,    // Чернігівська обл.
    26,    // Чернівецька обл.
    27,    // Львівська обл.
    28     // Донецька обл.
};

static const SettingListItem DISTRICTS[MAX_REGIONS] = {
    // АР Крим
    {9999, "АР Крим", false, false},
    
    // Вінницька область та її райони
    {4, "Вінницька обл.", false, false},
    {36, "Вінницький район", false, true},
    {37, "Гайсинський район", false, true},
    {35, "Жмеринський район", false, true},
    {33, "Могилів-Подільський район", false, true},
    {32, "Тульчинський район", false, true},
    {34, "Хмільницький район", false, true},
    {155, "м. Вінниця та Вінницька територіальна громада", true, true, true},
    
    // Волинська область та її райони
    {8, "Волинська обл.", false, false},
    {38, "Володимирський район", false, true},
    {41, "Камінь-Каширський район", false, true},
    {40, "Ковельський район", false, true},
    {39, "Луцький район", false, true},
    {225, "м. Луцьк та Луцька територіальна громада", true, true, true},
    
    
    // Дніпропетровська область та її райони
    {9, "Дніпропетровська обл.", false, false},
    {44, "Дніпровський район", false, true},
    {42, "Кам'янський район", false, true},
    {46, "Криворізький район", false, true},
    {47, "Нікопольський район", false, true},
    {43, "Самарівський район", false, true},
    {45, "Павлоградський район", false, true},
    {48, "Синельниківський район", false, true},
    {332, "м. Дніпро та Дніпровська територіальна громада", true, true, true},
    
    // Донецька область та її райони
    {28, "Донецька обл.", false, false},
    {54, "Бахмутський район", false, true},
    {55, "Волноваський район", false, true},
    {51, "Горлівський район", false, true},
    {53, "Донецький район", false, true},
    {49, "Кальміуський район", false, true},
    {50, "Краматорський район", false, true},
    {52, "Маріупольський район", false, true},
    {56, "Покровський район", false, true},
    
    // Житомирська область та її райони
    {10, "Житомирська обл.", false, false},
    {57, "Бердичівський район", false, true},
    {59, "Житомирський район", false, true},
    {60, "Звягельський район", false, true},
    {58, "Коростенський район", false, true},
    {442, "м. Житомир та Житомирська територіальна громада", true, true, true},
    
    // Закарпатська область та її райони
    {11, "Закарпатська обл.", false, false},
    {61, "Берегівський район", false, true},
    {62, "Хустський район", false, true},
    {65, "Мукачівський район", false, true},
    {63, "Рахівський район", false, true},
    {64, "Тячівський район", false, true},
    {66, "Ужгородський район", false, true},
    {500, "м. Ужгород та Ужгородська територіальна громада", true, true, true},
    
    // Запорізька область та її райони
    {12, "Запорізька обл.", false, false},
    {146, "Василівський район", false, true},
    {147, "Бердянський район", false, true},
    {149, "Запорізький район", false, true},
    {148, "Мелітопольський район", false, true},
    {145, "Пологівський район", false, true},
    {564, "м. Запоріжжя та Запорізька територіальна громада", false, true},
    
    // Івано-Франківська область та її райони
    {13, "Ів.-Франківська обл.", false, false},
    {67, "Верховинський район", false, true},
    {68, "Івано-Франківський район", false, true},
    {71, "Калуський район", false, true},
    {70, "Коломийський район", false, true},
    {69, "Косівський район", false, true},
    {72, "Надвірнянський район", false, true},
    {632, "м. Івано-Франківськ та Івано-Франківська територіальна громада", true, true, true},

    // м. Київ 
    {31, "м. Київ", false, false},
    
    // Київська область та її райони
    {14, "Київська обл.", false, false},
    {73, "Білоцерківський район", false, true},
    {78, "Бориспільський район", false, true},
    {79, "Броварський район", false, true},
    {75, "Бучанський район", false, true},
    {74, "Вишгородський район", false, true},
    {76, "Обухівський район", false, true},
    {77, "Фастівський район", false, true},
  
    // Кіровоградська область та її райони
    {15, "Кіровоградська обл.", false, false},
    {82, "Голованівський район", false, true},
    {81, "Кропивницький район", false, true},
    {83, "Новоукраїнський район", false, true},
    {80, "Олександрійський район", false, true},
    {761, "м. Кропивницький та Кропивницька територіальна громада", true, true, true},
    
    // Луганська область та її райони
    {16, "Луганська обл.", false, false},
    {85, "Сватівський район", true, true, true},
    {84, "Сєвєродонецький район", true, true, true},
    {86, "Старобільський район", true, true, true},
    {87, "Щастинський район", true, true, true},
    
    // Львівська область та її райони
    {27, "Львівська обл.", false, false},
    {91, "Дрогобицький район", false, true},
    {94, "Золочівський район", false, true},
    {90, "Львівський район", false, true},
    {88, "Самбірський район", false, true},
    {89, "Стрийський район", false, true},
    {92, "Шептицький район", false, true},
    {93, "Яворівський район", false, true},
    {845, "м. Львів та Львівська територіальна громада", true, true, true},
    
    // Миколаївська область та її райони
    {17, "Миколаївська обл.", false, false},
    {96, "Баштанський район", false, true},
    {95, "Вознесенський район", false, true},
    {98, "Миколаївський район", false, true},
    {97, "Первомайський район", false, true},
    {926, "м. Миколаїв та Миколаївська територіальна громада", true, true, true},
    
    // Одеська область та її райони
    {18, "Одеська обл.", false, false},
    {100, "Березівський район", false, true},
    {102, "Білгород-Дністровський район", false, true},
    {105, "Болградський район", false, true},
    {101, "Ізмаїльський район", false, true},
    {104, "Одеський район", false, true},
    {99, "Подільський район", false, true},
    {103, "Роздільнянський район", false, true},
    {964, "м. Одеса та Одеська територіальна громада", true, true, true},
    
    // Полтавська область та її райони
    {19, "Полтавська обл.", false, false},
    {107, "Кременчуцький район", false, true},
    {106, "Лубенський район", false, true},
    {108, "Миргородський район", false, true},
    {109, "Полтавський район", false, true},
    {1060, "м. Полтава та Полтавська територіальна громада", true, true, true},
    
    // Рівненська область та її райони
    {5, "Рівненська обл.", false, false},
    {110, "Вараський район", false, true},
    {111, "Дубенський район", false, true},
    {112, "Рівненський район", false, true},
    {113, "Сарненський район", false, true},
    {1133, "м. Рівне та Рівненська територіальна громада", true, true, true},
    
    // Сумська область та її райони
    {20, "Сумська обл.", false, false},
    {117, "Конотопський район", false, true},
    {118, "Охтирський район", false, true},
    {116, "Роменський район", false, true},
    {114, "Сумський район", false, true},
    {115, "Шосткинський район", false, true},
    {1187, "м. Суми та Сумська територіальна громада", true, true, true},
    
    // Тернопільська область та її райони
    {21, "Тернопільська обл.", false, false},
    {120, "Кременецький район", false, true},
    {119, "Тернопільський район", false, true},
    {121, "Чортківський район", false, true},
    {1241, "м. Тернопіль та Тернопільська територіальна громада", true, true, true},
    
    // Харківська область та її райони
    {22, "Харківська обл.", false, false},
    {126, "Богодухівський район", false, true},
    {125, "Ізюмський район", false, true},
    {127, "Берестинський район", false, true},
    {123, "Куп'янський район", false, true},
    {128, "Лозівський район", false, true},
    {124, "Харківський район", false, true},
    {122, "Чугуївський район", false, true},
    {1293, "м. Харків та Харківська територіальна громада", false, true},
    
    // Херсонська область та її райони
    {23, "Херсонська обл.", false, false},
    {129, "Бериславський район", false, true},
    {133, "Генічеський район", false, true},
    {131, "Каховський район", false, true},
    {130, "Скадовський район", false, true},
    {132, "Херсонський район", false, true},
    {1370, "м. Херсон та Херсонська територіальна громада", true, true, true},
    
    // Хмельницька область та її райони
    {3, "Хмельницька обл.", false, false},
    {135, "Кам'янець-Подільський район", false, true},
    {134, "Хмельницький район", false, true},
    {136, "Шепетівський район", false, true},
    {1400, "м. Хмельницький та Хмельницька територіальна громада", true, true, true},
    
    // Черкаська область та її райони
    {24, "Черкаська обл.", false, false},
    {150, "Звенигородський район", false, true},
    {153, "Золотоніський район", false, true},
    {151, "Уманський район", false, true},
    {152, "Черкаський район", false, true},
    {1473, "м. Черкаси та Черкаська територіальна громада", true, true, true},
    
    // Чернівецька область та її райони
    {26, "Чернівецька обл.", false, false},
    {138, "Вижницький район", false, true},
    {139, "Дністровський район", false, true},
    {137, "Чернівецький район", false, true},
    {1542, "м. Чернівці та Чернівецька територіальна громада", true, true, true},
    
    // Чернігівська область та її райони
    {25, "Чернігівська обл.", false, false},
    {144, "Корюківський район", false, true},
    {142, "Ніжинський район", false, true},
    {141, "Новгород-Сіверський район", false, true},
    {143, "Прилуцький район", false, true},
    {140, "Чернігівський район", false, true},
    {1591, "м. Чернігів та Чернігівська територіальна громада", true, true, true},
};

// ==============================================================================
// REGION LED MAPS - MOVED TO JaamConfig_Generated.h  
// ==============================================================================
// Old large RegionLedMapEntry arrays (~17KB) moved to generated file.
// Now using compact flat array structures (~2KB) for memory efficiency.
// Data is auto-generated by tools/convert_region_map.py script.
// ==============================================================================

// Визначення пінів для різних плат
#if ARDUINO_ESP32_DEV
    #define SUPPORTED_LEDS_PINS {2, 4, 12, 13, 14, 15, 16, 17, 18, 25, 26, 27, 32, 33}
#elif ARDUINO_ESP32S3_DEV
    #define SUPPORTED_LEDS_PINS {2, 4, 12, 13, 14, 15, 18, 21, 25, 26, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42}
#elif ARDUINO_ESP32C3_DEV
    #define SUPPORTED_LEDS_PINS {2, 3, 4, 5, 6, 7, 8, 9, 10, 18, 19, 20, 21}
#else
    #error "Платформа не підтримується!"
#endif

// Включаємо згенеровані flat array мапи (економія ~15KB RAM)
#include "JaamConfig_Generated.h"
