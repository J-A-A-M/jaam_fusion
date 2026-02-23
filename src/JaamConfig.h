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
    static const int LAMP = 4;
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
}

// --- WiFi Configuration ---
namespace WiFiConfig {
    static const bool ENABLED = true;
    static const uint32_t CONNECT_TIMEOUT = 3;           // 10 seconds
    static const uint32_t CONNECT_RETRIES = 10;
    static const uint32_t PORTAL_TIMEOUT = 180;          // 3 minutes
    static const uint16_t WEB_PORT = 8080;
}

// --- Timing Configuration ---
static const uint32_t ANIMATION_INTERVAL = 1000;            // 1 second
static const uint32_t MEMORY_CHECK_INTERVAL = 60000;        // 1 minute
static const uint32_t WIFI_CHECK_INTERVAL = 1000;           // 1 second
static const uint32_t WEBSOCKET_CHECK_INTERVAL = 3000;      // 3 seconds
static const uint32_t TIME_CHECK_INTERVAL = 60000;          // 1 minute
static const uint32_t MAIN_THREAD_CHECK_INTERVAL = 500;     // 0.5 seconds
static const uint32_t BATTERY_CHECK_INTERVAL = 10000;         // 10 seconds
static const uint32_t DISPLAY_CHECK_INTERVAL = 1000;       // 1 second
static const uint32_t CLIMATE_CHECK_INTERVAL = 10000;      // 10 seconds
static const uint32_t VOLUME_CHECK_INTERVAL = 1000;        // 1 second
static const uint32_t BEEP_HOUR_CHECK_INTERVAL = 1000;      //

// --- Packet structure ---
static constexpr uint8_t  TYPE_ALERTS_BATCH = 0xA1;
static constexpr uint8_t  TYPE_NOTIFICATIONS_BATCH = 0xA2;
static constexpr uint8_t  TYPE_WEATHER_BATCH = 0xA3;
static constexpr size_t   HEADER_SZ         = 1;            // лише 1 байт – type
static constexpr size_t   RECORD_SZ         = 4;            // 2B region_id + 2B flags16
static constexpr size_t   RECORD_LZ         = 3;            // 2B region_id + 1B flags8
static constexpr size_t   HASH_SZ           = 4;            // 2B actual + 2B prev

// --- Region to LED mapping (fixed, задається один раз) ---
constexpr int MAX_REGIONS = 169;                            // Кількість регіонів
constexpr int MAX_LEDS_PER_REGION = 25;                      // Максимум LED на регіон

struct RegionLedMapEntry {
    uint16_t region_id;
    int led_positions[MAX_LEDS_PER_REGION];
    uint8_t led_count;
};

// --- Background LED color mapping ---
constexpr int MAX_BG_LEDS = 300;                            // Максимум задніх LED

struct BgLedColorEntry {
    uint16_t led_index;
    uint32_t color;
};

struct SettingListItem {
  int id;
  const char* name;
  bool ignore;
  bool sub;
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

constexpr int SINGLE_CLICKS_COUNT = 3;
static SettingListItem SINGLE_CLICKS[] = {
  {0, "Вимкнено"},
  {1, "Перемикання режимів мапи"},
  // {2, "Перемикання режимів дисплея"},
  {3, "Увімк./Вимк. мапу"},
  // {4, "Увімк./Вимк. дисплей"},
  // {5, "Увімк./Вимк. мапу та дисплей"},
  // {6, "Увімк./Вимк. нічний режим"},
  // {7, "Увімк./Вимк. режим лампи", false},
};

constexpr int LONG_CLICKS_COUNT = 4;
static SettingListItem LONG_CLICKS[] = {
  {0, "Вимкнено"},
  {1, "Перемикання режимів мапи"},
  // {2, "Перемикання режимів дисплея"},
  {3, "Увімк./Вимк. мапу"},
  // {4, "Увімк./Вимк. дисплей"},
  // {5, "Увімк./Вимк. мапу та дисплей"},
  // {6, "Увімк./Вимк. нічний режим"},
  // {8, "Збільшити яскравість лампи"},
  // {9, "Зменшити яскравість лампи"},
  {10, "Перезавантаження пристрою"},
};

constexpr int MAP_MODES_COUNT = 5;
static SettingListItem MAP_MODES[] = {
  {0, "Вимкнено"},
  {1, "Тривога"},
  // {6, "Енергосистема", false},
  {2, "Погода"},
  // {7, "Радіація", false},
  {3, "Прапор"},
  {4, "Лампа"},
};

constexpr int TIMEZONES_COUNT = 43;
static SettingListItem TIMEZONES[] = {
  {0, "Europe/Kyiv (UTC+2)"},
  // Європейські часові пояси (географічно: захід → схід)
  {1, "Europe/Lisbon (UTC+0)"},
  {2, "Europe/Dublin (UTC+0)"},
  {3, "Europe/London (UTC+0)"},
  {4, "Europe/Madrid (UTC+1)"},
  {5, "Europe/Paris (UTC+1)"},
  {6, "Europe/Amsterdam (UTC+1)"},
  {7, "Europe/Berlin (UTC+1)"},
  {8, "Europe/Prague (UTC+1)"},
  {9, "Europe/Warsaw (UTC+1)"},
  {10, "Europe/Rome (UTC+1)"},
  {11, "Europe/Athens (UTC+2)"},
  {12, "Europe/Helsinki (UTC+2)"},
  {13, "Europe/Bucharest (UTC+2)"},
  {14, "Europe/Istanbul (UTC+3)"},
  // Американські часові пояси (географічно: захід → схід)
  {15, "Pacific/Honolulu (UTC-10)"},
  {16, "America/Anchorage (UTC-9)"},
  {17, "America/Los_Angeles (UTC-8)"},
  {18, "America/Vancouver (UTC-8)"},
  {19, "America/Denver (UTC-7)"},
  {20, "America/Chicago (UTC-6)"},
  {21, "America/Mexico_City (UTC-6)"},
  {22, "America/Toronto (UTC-5)"},
  {23, "America/New_York (UTC-5)"},
  // Південноамериканські пояси (географічно: захід → схід)
  {24, "America/Lima (UTC-5)"},
  {25, "America/Sao_Paulo (UTC-3)"},
  {26, "America/Argentina/Buenos_Aires (UTC-3)"},
  // Азіатські та Близькосхідні пояси (географічно: захід → схід)
  {27, "Asia/Dubai (UTC+4)"},
  {28, "Asia/Karachi (UTC+5)"},
  {29, "Asia/Kolkata (UTC+5:30)"},
  {30, "Asia/Bangkok (UTC+7)"},
  {31, "Asia/Jakarta (UTC+7)"},
  {32, "Asia/Singapore (UTC+8)"},
  {33, "Asia/Hong_Kong (UTC+8)"},
  {34, "Asia/Shanghai (UTC+8)"},
  {35, "Asia/Tokyo (UTC+9)"},
  {36, "Asia/Seoul (UTC+9)"},
  // Австралія та Океанія (географічно: захід → схід)
  {37, "Australia/Perth (UTC+8)"},
  {38, "Australia/Brisbane (UTC+10)"},
  {39, "Australia/Melbourne (UTC+10)"},
  {40, "Australia/Sydney (UTC+10)"},
  {41, "Pacific/Auckland (UTC+12)"},
  {42, "Pacific/Fiji (UTC+12)"},
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
  // ID 1: Europe/Lisbon (WET/WEST)
  {1, 0, 0, true, {3, 0, 7, 1}, {10, 0, 7, 2}},
  // ID 2: Europe/Dublin (IST/GMT)
  {2, 0, 0, true, {3, 0, 7, 1}, {10, 0, 7, 2}},
  // ID 3: Europe/London - британський літній час (BST)
  {3, 0, 0, true, {3, 0, 7, 1}, {10, 0, 7, 2}},
  // ID 4: Europe/Madrid
  {4, 1, 0, true, {3, 0, 7, 2}, {10, 0, 7, 3}},
  // ID 5: Europe/Paris - Європейський центральний літній час (CEST)
  {5, 1, 0, true, {3, 0, 7, 2}, {10, 0, 7, 3}},
  // ID 6: Europe/Amsterdam
  {6, 1, 0, true, {3, 0, 7, 2}, {10, 0, 7, 3}},
  // ID 7: Europe/Berlin
  {7, 1, 0, true, {3, 0, 7, 2}, {10, 0, 7, 3}},
  // ID 8: Europe/Prague
  {8, 1, 0, true, {3, 0, 7, 2}, {10, 0, 7, 3}},
  // ID 9: Europe/Warsaw
  {9, 1, 0, true, {3, 0, 7, 2}, {10, 0, 7, 3}},
  // ID 10: Europe/Rome
  {10, 1, 0, true, {3, 0, 7, 2}, {10, 0, 7, 3}},
  // ID 11: Europe/Athens - Європейський східній літній час (EEST)
  {11, 2, 0, true, {3, 0, 7, 3}, {10, 0, 7, 4}},
  // ID 12: Europe/Helsinki
  {12, 2, 0, true, {3, 0, 7, 3}, {10, 0, 7, 4}},
  // ID 13: Europe/Bucharest
  {13, 2, 0, true, {3, 0, 7, 3}, {10, 0, 7, 4}},
  // ID 14: Europe/Istanbul - немає DST з 2016
  {14, 3, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 15: Pacific/Honolulu - немає DST
  {15, -10, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 16: America/Anchorage
  {16, -9, 0, true, {3, 2, 7, 2}, {11, 1, 7, 2}},
  // ID 17: America/Los_Angeles
  {17, -8, 0, true, {3, 2, 7, 2}, {11, 1, 7, 2}},
  // ID 18: America/Vancouver
  {18, -8, 0, true, {3, 2, 7, 2}, {11, 1, 7, 2}},
  // ID 19: America/Denver
  {19, -7, 0, true, {3, 2, 7, 2}, {11, 1, 7, 2}},
  // ID 20: America/Chicago
  {20, -6, 0, true, {3, 2, 7, 2}, {11, 1, 7, 2}},
  // ID 21: America/Mexico_City - немає DST
  {21, -6, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 22: America/Toronto
  {22, -5, 0, true, {3, 2, 7, 2}, {11, 1, 7, 2}},
  // ID 23: America/New_York - 2-а неділя березня, 1-а неділя листопада
  {23, -5, 0, true, {3, 2, 7, 2}, {11, 1, 7, 2}},
  // ID 24: America/Lima - немає DST
  {24, -5, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 25: America/Sao_Paulo - немає DST
  {25, -3, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 26: America/Argentina/Buenos_Aires - немає DST
  {26, -3, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 27: Asia/Dubai - немає DST
  {27, 4, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 28: Asia/Karachi - немає DST
  {28, 5, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 29: Asia/Kolkata - немає DST
  {29, 5, 30, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 30: Asia/Bangkok - немає DST
  {30, 7, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 31: Asia/Jakarta - немає DST
  {31, 7, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 32: Asia/Singapore - немає DST
  {32, 8, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 33: Asia/Hong_Kong - немає DST
  {33, 8, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 34: Asia/Shanghai - немає DST
  {34, 8, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 35: Asia/Tokyo - немає DST
  {35, 9, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 36: Asia/Seoul - немає DST
  {36, 9, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 37: Australia/Perth - немає DST
  {37, 8, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 38: Australia/Brisbane - немає DST
  {38, 10, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
  // ID 39: Australia/Melbourne - 1-а неділя жовтня, 1-а неділя квітня
  {39, 10, 0, true, {10, 1, 7, 2}, {4, 1, 7, 3}},
  // ID 40: Australia/Sydney - 1-а неділя жовтня, 1-а неділя квітня
  {40, 10, 0, true, {10, 1, 7, 2}, {4, 1, 7, 3}},
  // ID 41: Pacific/Auckland - остання неділя вересня, 1-а неділя квітня
  {41, 12, 0, true, {9, 0, 7, 2}, {4, 1, 7, 3}},
  // ID 42: Pacific/Fiji - немає DST
  {42, 12, 0, false, {0, 0, 0, 0}, {0, 0, 0, 0}},
};

constexpr int DISPLAY_MODES_COUNT = 2;
static SettingListItem DISPLAY_MODES[] = {
  {0, "Вимкнено", false},
  {1, "Годинник", false},
  // {5, "Енергосистема", false},
  // {2, "Погода", false},
  // {6, "Радіація", false},
  // {3, "Технічна інформація", false},
  // {4, "Мікроклімат", false},
  // {9, "Перемикання", false},
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

constexpr int ANIMATION_TYPES_COUNT = 7;
static SettingListItem ANIMATION_TYPES[] = {
  {AnimationTypes::FADE, "FADE"},
  {AnimationTypes::BLINK, "BLINK"},
  {AnimationTypes::BLEND_FADE, "BLEND_FADE"},
  {AnimationTypes::PULSE, "PULSE"},
  {AnimationTypes::ONE_WAY_BLEND_FADE, "ONE_WAY_BLEND_FADE"},
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
    {155, "м. Вінниця та Вінницька територіальна громада", false, true},
    
    // Волинська область та її райони
    {8, "Волинська обл.", false, false},
    {39, "Луцький район", false, true},
    {38, "Володимир-Волинський район", false, true},
    {40, "Ковельський район", false, true},
    {41, "Камінь-Каширський район", false, true},
    {225, "м. Луцьк та Луцька територіальна громада", false, true},
    
    
    // Дніпропетровська область та її райони
    {9, "Дніпропетровська обл.", false, false},
    {43, "Новомосковський район", false, true},
    {44, "Дніпровський район", false, true},
    {47, "Нікопольський район", false, true},
    {48, "Синельниківський район", false, true},
    {42, "Кам'янський район", false, true},
    {45, "Павлоградський район", false, true},
    {46, "Криворізький район", false, true},
    {332, "м. Дніпро та Дніпровська територіальна громада", false, true},
    
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
    {442, "м. Житомир та Житомирська територіальна громада", false, true},
    
    // Закарпатська область та її райони
    {11, "Закарпатська обл.", false, false},
    {66, "Ужгородський район", false, true},
    {61, "Берегівський район", false, true},
    {62, "Хустський район", false, true},
    {63, "Рахівський район", false, true},
    {64, "Тячівський район", false, true},
    {65, "Мукачівський район", false, true},
    {500, "м. Ужгород та Ужгородська територіальна громада", false, true},
    
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
    {632, "м. Івано-Франківськ та Івано-Франківська територіальна громада", false, true},
    
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
    {761, "м. Кропивницький та Кропивницька територіальна громада", false, true},
    
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
    {845, "м. Львів та Львівська територіальна громада", false, true},
    
    // Миколаївська область та її райони
    {17, "Миколаївська обл.", false, false},
    {96, "Баштанський район", false, true},
    {95, "Вознесенський район", false, true},
    {97, "Первомайський район", false, true},
    {98, "Миколаївський район", false, true},
    {926, "м. Миколаїв та Миколаївська територіальна громада", false, true},
    
    // Одеська область та її райони
    {18, "Одеська обл.", false, false},
    {105, "Болградський район", false, true},
    {100, "Березівський район", false, true},
    {104, "Одеський район", false, true},
    {102, "Білгород-Дністровський район", false, true},
    {103, "Роздільнянський район", false, true},
    {101, "Ізмаїльський район", false, true},
    {99, "Подільський район", false, true},
    {964, "м. Одеса та Одеська територіальна громада", false, true},
    
    // Полтавська область та її райони
    {19, "Полтавська обл.", false, false},
    {107, "Кременчуцький район", false, true},
    {106, "Лубенський район", false, true},
    {109, "Полтавський район", false, true},
    {108, "Миргородський район", false, true},
    {1060, "м. Полтава та Полтавська територіальна громада", false, true},
    
    // Рівненська область та її райони
    {5, "Рівненська обл.", false, false},
    {110, "Вараський район", false, true},
    {111, "Дубенський район", false, true},
    {112, "Рівненський район", false, true},
    {113, "Сарненський район", false, true},
    {1133, "м. Рівне та Рівненська територіальна громада", false, true},
    
    // Сумська область та її райони
    {20, "Сумська обл.", false, false},
    {115, "Шосткинський район", false, true},
    {116, "Роменський район", false, true},
    {117, "Конотопський район", false, true},
    {114, "Сумський район", false, true},
    {118, "Охтирський район", false, true},
    {1187, "м. Суми та Сумська територіальна громада", false, true},
    
    // Тернопільська область та її райони
    {21, "Тернопільська обл.", false, false},
    {119, "Тернопільський район", false, true},
    {121, "Чортківський район", false, true},
    {120, "Кременецький район", false, true},
    {1241, "м. Тернопіль та Тернопільська територіальна громада", false, true},
    
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
    {1370, "м. Херсон та Херсонська територіальна громада", false, true},
    
    // Хмельницька область та її райони
    {3, "Хмельницька обл.", false, false},
    {136, "Шепетівський район", false, true},
    {134, "Хмельницький район", false, true},
    {135, "Кам'янець-Подільський район", false, true},
    {1400, "м. Хмельницький та Хмельницька територіальна громада", false, true},
    
    // Черкаська область та її райони
    {24, "Черкаська обл.", false, false},
    {153, "Золотоніський район", false, true},
    {152, "Черкаський район", false, true},
    {150, "Звенигородський район", false, true},
    {151, "Уманський район", false, true},
    {1473, "м. Черкаси та Черкаська територіальна громада", false, true},
    
    // Чернівецька область та її райони
    {26, "Чернівецька обл.", false, false},
    {139, "Дністровський район", false, true},
    {138, "Вижницький район", false, true},
    {137, "Чернівецький район", false, true},
    {1542, "м. Чернівці та Чернівецька територіальна громада", false, true},
    
    // Чернігівська область та її райони
    {25, "Чернігівська обл.", false, false},
    {141, "Новгород-Сіверський район", false, true},
    {142, "Ніжинський район", false, true},
    {143, "Прилуцький район", false, true},
    {140, "Чернігівський район", false, true},
    {144, "Корюківський район", false, true},
    {1591, "м. Чернігів та Чернігівська територіальна громада", false, true},
};

// Фіксований масив відповідностей (заповнюється вручну)
const RegionLedMapEntry REGION_MAP_JAAM_3_0[MAX_REGIONS] = {
    // АР Крим
    { 9999, { 14, 15, 16, 17, 18, 19, 20, 21, 22 }, 9 },                          // Автономна Республіка Крим

    // Вінницька область та її райони
    {    4, { 94, 95, 96, 97, 98, 99, 100, 142, 143, 144, 145, 186 }, 12 },       // Вінницька область
    {   32, { 95, 99 }, 2 },                                                      // Тульчинський район
    {   35, { 98 }, 1 },                                                          // Жмеринський район
    {   36, { 142, 143, 144, 145 }, 4 },                                          // Вінницький район
    {   34, { 186 }, 1 },                                                         // Хмільницький район
    {   33, { 96, 97 }, 2 },                                                      // Могилів-Подільський район
    {   37, { 94, 100 }, 2 },                                                     // Гайсинський район
    {  155, { 144 }, 1 },                                                         // м. Вінниця та Вінницька територіальна громада

    // Волинська область та її райони
    {    8, { 218, 255, 256, 257, 258, 259, 260, 261, 262, 263, 265 }, 11 },      // Волинська область
    {   39, { 218, 255, 256 }, 3 },                                               // Луцький район
    {   38, { 257 }, 1 },                                                         // Володимир-Волинський район
    {   40, { 258, 259, 260, 261 }, 4 },                                          // Ковельський район
    {   41, { 262, 263, 265 }, 3 },                                               // Камінь-Каширський район
    {  225, { 256 }, 1 },                                                         // м. Луцьк та Луцька територіальна громада

    // Дніпропетровська область та її райони
    {    9, { 49, 50, 51, 62, 63, 64, 65, 66, 67, 81, 82, 83, 84, 85 }, 14 },     // Дніпропетровська область
    {   43, { 83 }, 1 },                                                          // Новомосковський район
    {   44, { 64, 84, 65 }, 3 },                                                  // Дніпровський район
    {   47, { 49 }, 1 },                                                          // Нікопольський район
    {   48, { 66, 67, 81 }, 3 },                                                  // Синельниківський район
    {   42, { 63, 85 }, 2 },                                                      // Кам'янський район
    {   45, { 82 }, 1 },                                                          // Павлоградський район
    {   46, { 50, 51, 62 }, 3 },                                                  // Криворізький район
    {  332, { 65 }, 1 },                                                          // м. Дніпро та Дніпровська територіальна громада

    // Донецька область та її райони
    {   28, { 42, 43, 44, 68, 69, 70, 71, 72, 78, 79, 80, 112, 115 }, 13 },       // Донецька область
    {   56, { 80 }, 1 },                                                          // Покровський район
    {   51, { 72 }, 1 },                                                          // Горлівський район
    {   55, { 44, 68 }, 2 },                                                      // Волноваський район
    {   53, { 69, 71 }, 2 },                                                      // Донецький район
    {   49, { 43, 70 }, 2 },                                                      // Кальміуський район
    {   52, { 42 }, 1 },                                                          // Маріупольський район
    {   50, { 79, 112, 115 }, 3 },                                                // Краматорський район
    {   54, { 78 }, 1 },                                                          // Бахмутський район

    // Житомирська область та її райони
    {   10, { 187, 206, 207, 208, 223, 224, 225, 226, 249, 250, 251, 270, 271, 272 }, 14 }, // Житомирська область
    {   59, { 206, 208, 224, 225, 226 }, 5 },                                     // Житомирський район
    {   58, { 249, 250, 270, 271, 272 }, 5 },                                     // Коростенський район
    {   57, { 187, 207 }, 2 },                                                    // Бердичівський район
    {   60, { 223, 251 }, 2 },                                                    // Звягельський район
    {  442, { 225 }, 1 },                                                         // м. Житомир та Житомирська територіальна громада

    // Закарпатська область та її райони
    {   11, { 155, 156, 157, 158, 159, 160, 161 }, 7 },                           // Закарпатська область
    {   66, { 161, 160 }, 2 },                                                    // Ужгородський район
    {   61, { 158 }, 1 },                                                         // Берегівський район
    {   62, { 157 }, 1 },                                                         // Хустський район
    {   63, { 155 }, 1 },                                                         // Рахівський район
    {   64, { 156 }, 1 },                                                         // Тячівський район
    {   65, { 159 }, 1 },                                                         // Мукачівський район
    {  500, { 160 }, 1 },                                                         // м. Ужгород та Ужгородська територіальна громада

    // Запорізька область та її райони
    {   12, { 24, 25, 26, 39, 40, 41, 45, 46, 47 }, 9 },                          // Запорізька область
    {  146, { 39 }, 1 },                                                          // Василівський район
    {  145, { 40, 45, 46 }, 3 },                                                  // Пологівський район
    {  149, { 47 }, 1 },                                                      // Запорізький район
    {  147, { 41 }, 1 },                                                          // Бердянський район
    {  148, { 24, 25, 26 }, 3 },                                                  // Мелітопольський район
    {  564, { 48 }, 1 },                                                          // м. Запоріжжя та Запорізька територіальна громада

    // Івано-Франківська область та її райони
    {   13, { 154, 163, 164, 165, 166, 171, 172, 173 }, 8 },                      // Івано-Франківська область
    {   68, { 171, 172, 173 }, 3 },                                              // Івано-Франківський район
    {   67, { 154 }, 1 },                                                         // Верховинський район
    {   71, { 163 }, 1 },                                                         // Калуський район
    {   72, { 164 }, 1 },                                                         // Надвірнянський район
    {   70, { 166 }, 1 },                                                         // Коломийський район
    {   69, { 165 }, 1 },                                                         // Косівський район
    {  632, { 172 }, 1 },                                                         // м. Івано-Франківськ та Івано-Франківська територіальна громада

    // Київська область та її райони
    {   14, { 139, 140, 141, 188, 189, 190, 202, 203, 205, 227, 228, 248 }, 12 }, // Київська область
    {   77, { 205 }, 1 },                                                         // Фастівський район
    {   73, { 140, 141, 188 }, 3 },                                               // Білоцерківський район
    {   75, { 227 }, 1 },                                                         // Бучанський район
    {   76, { 139, 189 }, 2 },                                                    // Обухівський район
    {   74, { 228, 248 }, 2 },                                                    // Вишгородський район
    {   79, { 202, 203 }, 2 },                                                    // Броварський район
    {   78, { 190 }, 1 },                                                         // Бориспільський район
    {   31, { 204 }, 1 },                                                         // м. Київ

    // Кіровоградська область та її райони
    {   15, { 53, 59, 60, 61, 86, 87, 88, 89, 90, 92 }, 10 },                     // Кіровоградська область
    {   81, { 53, 61, 88 , 89 }, 4 },                                              // Кропивницький район
    {   80, { 86, 87 }, 2 },                                                        // Олександрійський район
    {   82, { 59, 92 }, 2 },                                                      // Голованівський район
    {   83, { 60, 90 }, 2 },                                                      // Новоукраїнський район
    {  761, { 89 }, 1 },                                                          // м. Кропивницький та Кропивницька територіальна громада

    // Луганська область та її райони
    {   16, { 73, 74, 75, 76, 77, 116, 117, 118, 119, 120, 121, 122 }, 12 },      // Луганська область
    {   86, { 118, 119, 120 }, 3 },                                               // Старобільський район
    {   85, { 121, 122 }, 2 },                                                    // Сватівський район
    {   84, { 116 }, 1 },                                                         // Сєвєродонецький район
    {   87, { 117 }, 1 },                                                         // Щастинський район

    // Львівська область та її райони
    {   27, { 162, 174, 175, 176, 177, 178, 179, 215, 216, 217 }, 10 },           // Львівська область
    {   90, { 179, 216, 178 }, 3 },                                               // Львівський район
    {   89, { 162, 174 }, 2 },                                                    // Стрийський район
    {   88, { 176 }, 1 },                                                         // Самбірський район
    {   91, { 175 }, 1 },                                                         // Дрогобицький район
    {   92, { 217 }, 1 },                                                         // Червоноградський район
    {   94, { 215 }, 1 },                                                         // Золочівський район
    {   93, { 177 }, 1 },                                                         // Яворівський район
    {  845, { 178 }, 1 },                                                         // м. Львів та Львівська територіальна громада

    // Миколаївська область та її райони
    {   17, { 9, 30, 34, 35, 36, 52, 54, 55 }, 8 },                               // Миколаївська область
    {   96, { 36, 52 }, 2 },                                                      // Баштанський район
    {   95, { 34, 54 }, 2 },                                                      // Вознесенський район
    {   97, { 55 }, 1 },                                                          // Первомайський район
    {   98, { 9, 35, 30 }, 3 },                                                   // Миколаївський район
    {  926, { 30 }, 1 },                                                          // м. Миколаїв та Миколаївська територіальна громада

    // Одеська область та її райони
    {   18, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 31, 32, 33, 56, 57, 58 }, 15 },          // Одеська область
    {  105, { 1, 4 }, 2 },                                                        // Болградський район
    {  100, { 31, 33 }, 2 },                                                      // Березівський район
    {  104, { 6, 8 }, 2 },                                                        // Одеський район
    {  102, { 3, 5 }, 2 },                                                        // Білгород-Дністровський район
    {  103, { 7, 32 }, 2 },                                                       // Роздільнянський район
    {  101, { 0, 2 }, 2 },                                                        // Ізмаїльський район
    {   99, { 56, 57, 58 }, 3 },                                                  // Подільський район
    {  964, { 8 }, 1 },                                                           // м. Одеса та Одеська територіальна громада

    // Полтавська область та її райони
    {   19, { 106, 107, 108, 109, 131, 132, 133, 134, 135, 191, 192, 193, 198 }, 13 }, // Полтавська область
    {  107, { 106, 107 }, 2 },                                                    // Кременчуцький район
    {  106, { 135, 191 }, 2 },                                                    // Лубенський район
    {  109, { 108, 109, 131, 132, 133, 134, 193 }, 7 },                           // Полтавський район
    {  108, { 192, 198 }, 2 },                                                    // Миргородський район
    { 1060, { 134 }, 1 },                                                         // м. Полтава та Полтавська територіальна громада

    // Рівненська область та її райони
    {    5, { 219, 220, 221, 252, 253, 254, 264, 266, 267, 268, 269 }, 11 },      // Рівненська область
    {  110, { 264, 266 }, 2 },                                                    // Вараський район
    {  111, { 219, 220 }, 2 },                                                    // Дубенський район
    {  112, { 221, 252, 253, 254 }, 4 },                                          // Рівненський район
    {  113, { 267, 268, 269 }, 3 },                                               // Сарненський район
    { 1133, { 253 }, 1 },                                                         // м. Рівне та Рівненська територіальна громада

    // Сумська область та її райони
    {   20, { 194, 195, 196, 197, 199, 232, 233, 234, 235, 236, 237, 238 }, 12 }, // Сумська область
    {  115, { 237, 238 }, 2 },                                                    // Шосткинський район
    {  116, { 199, 233 }, 2 },                                                    // Роменський район
    {  117, { 232, 235, 236 }, 3 },                                               // Конотопський район
    {  114, { 195, 196, 197, 234 }, 4 },                                          // Сумський район
    {  118, { 194 }, 1 },                                                         // Охтирський район
    { 1187, { 196 }, 1 },                                                         // м. Суми та Сумська територіальна громада

    // Тернопільська область та її райони
    {   21, { 167, 169, 170, 180, 181, 182, 213, 214 }, 8 },                      // Тернопільська область
    {  119, { 180, 181, 182, 214 }, 4 },                                          // Тернопільський район
    {  121, { 167, 169, 170 }, 3 },                                               // Чортківський район
    {  120, { 213 }, 1 },                                                         // Кременецький район
    { 1241, { 181 }, 1 },                                                         // м. Тернопіль та Тернопільська територіальна громада

    // Харківська область та її райони
    {   22, { 110, 111, 113, 114, 123, 124, 125, 126, 127, 128, 130 }, 11 },      // Харківська область
    {  124, { 125 }, 1 },                                                         // Харківський район
    {  123, { 123 }, 1 },                                                         // Куп'янський район
    {  122, { 124, 126 }, 2 },                                                    // Чугуївський район
    {  126, { 130 }, 1 },                                                         // Богодухівський район
    {  127, { 110 }, 1 },                                                         // Красноградський район
    {  125, { 113, 114, 127 }, 3 },                                               // Ізюмський район
    {  128, { 111, 128 }, 2 },                                                    // Лозівський район
    { 1293, { 129 }, 1 },                                                         // м. Харків та Харківська територіальна громада

    // Херсонська область та її райони
    {   23, { 10, 11, 12, 13, 23, 27, 28, 29, 37, 38 }, 10 },                     // Херсонська область
    {  131, { 28, 38 }, 2 },                                                      // Каховський район
    {  129, { 37 }, 1 },                                                          // Бериславський район
    {  130, { 10, 11, 13 }, 3 },                                                  // Скадовський район
    {  132, { 12, 29 }, 2 },                                                      // Херсонський район
    {  133, { 23, 27 }, 2 },                                                      // Генічеський район
    { 1370, { 12 }, 1 },                                                          // м. Херсон та Херсонська територіальна громада

    // Хмельницька область та її райони
    {    3, { 146, 147, 168, 183, 184, 185, 209, 210, 211, 212, 222 }, 11 },      // Хмельницька область
    {  136, { 209, 212, 222 }, 3 },                                               // Шепетівський район
    {  134, { 146, 183, 184, 185, 210, 211 }, 6 },                                // Хмельницький район
    {  135, { 147, 168 }, 2 },                                                    // Кам'янець-Подільський район
    { 1400, { 184 }, 1 },                                                         // м. Хмельницький та Хмельницька територіальна громада

    // Черкаська область та її райони
    {   24, { 91, 93, 101, 102, 103, 104, 105, 136, 137, 138 }, 10 },             // Черкаська область
    {  153, { 136, 137 }, 2 },                                                    // Золотоніський район
    {  152, { 103, 104, 105, 138 }, 4 },                                          // Черкаський район
    {  150, { 91, 102 }, 2 },                                                     // Звенигородський район
    {  151, { 93, 101 }, 2 },                                                    // Уманський район
    { 1473, { 104 }, 1 },                                                         // м. Черкаси та Черкаська територіальна громада

    // Чернівецька область та її райони
    {   26, { 148, 149, 150, 151, 152, 153 }, 6 },                                // Чернівецька область
    {  139, { 148, 149 }, 2 },                                                    // Дністровський район
    {  138, { 153 }, 1 },                                                         // Вижницький район
    {  137, { 150, 151, 152 }, 3 },                                               // Чернівецький район
    { 1542, { 151 }, 1 },                                                         // м. Чернівці та Чернівецька територіальна громада

    // Чернігівська область та її райони
    {   25, { 200, 201, 229, 230, 231, 239, 240, 241, 242, 243, 244, 245, 246, 247 }, 14 }, // Чернігівська область
    {  141, { 239, 240 }, 2 },                                                    // Новгород-Сіверський район
    {  142, { 229, 230, 231 }, 3 },                                               // Ніжинський район
    {  143, { 200, 201 }, 2 },                                                    // Прилуцький район
    {  140, { 244, 245, 246, 247 }, 4 },                                          // Чернігівський район
    {  144, { 241, 242, 243 }, 3 },                                               // Корюківський район
    { 1591, { 244 }, 1 },                                                         // м. Чернігів та Чернігівська територіальна громада
};

// Фіксований масив відповідностей (заповнюється вручну)
const RegionLedMapEntry REGION_MAP_JAAM_3_2[MAX_REGIONS] = {
    // АР Крим
    { 9999, { 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 404}, 13 }, // Автономна Республіка Крим

    // Вінницька область та її райони
    {    4, { 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40}, 18 }, // Вінницька область
    {   32, { 35, 36}, 2 },                                                       // Тульчинський район
    {   35, { 34, 40 }, 2 },                                                      // Жмеринський район
    {   36, { 26, 27, 28, 31, 32, 33 }, 6 },                                      // Вінницький район
    {   34, { 29, 30 }, 2 },                                                      // Хмільницький район
    {   33, { 37, 38, 39 }, 3 },                                                  // Могилів-Подільський район
    {   37, { 23, 24, 25 }, 3 },                                                  // Гайсинський район
    {  155, { 32 }, 1 },                                                          // м. Вінниця та Вінницька територіальна громада

    // Волинська область та її райони
    {    8, { 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128}, 14 },      // Волинська область
    {   39, { 117, 118, 125 }, 3 },                                               // Луцький район
    {   38, { 115, 116 }, 2 },                                                         // Володимир-Волинський район
    {   40, { 119, 120, 121, 122, 123, 124 }, 6 },                                          // Ковельський район
    {   41, { 126, 127, 128 }, 3 },                                               // Камінь-Каширський район
    {  225, { 118 }, 1 },                                                         // м. Луцьк та Луцька територіальна громада

    // Дніпропетровська область та її райони
    {    9, { 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308 }, 18 },     // Дніпропетровська область
    {   43, { 297, 308 }, 2 },                                                          // Новомосковський район
    {   44, { 296, 298, 300, 307 }, 4 },                                                  // Дніпровський район
    {   47, { 301 }, 1 },                                                          // Нікопольський район
    {   48, { 291, 292, 293, 295 }, 4 },                                                  // Синельниківський район
    {   42, { 299, 305, 306}, 3 },                                                      // Кам'янський район
    {   45, { 294 }, 1 },                                                          // Павлоградський район
    {   46, { 302, 303, 304 }, 3 },                                                  // Криворізький район
    {  332, { 296 }, 1 },                                                          // м. Дніпро та Дніпровська територіальна громада

    // Донецька область та її райони
    {   28, { 254, 255, 256, 257, 258, 259, 260 ,261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272 }, 19 },       // Донецька область
    {   56, { 262, 266, 272 }, 3 },                                                          // Покровський район
    {   51, { 254, 265 }, 2 },                                                          // Горлівський район
    {   55, { 259, 260 ,261 }, 3 },                                                      // Волноваський район
    {   53, { 255, 263, 264 }, 3 },                                                      // Донецький район
    {   49, { 256, 257 }, 2 },                                                      // Кальміуський район
    {   52, { 258 }, 1 },                                                          // Маріупольський район
    {   50, { 267, 269, 270, 271 }, 4 },                                                // Краматорський район
    {   54, { 268 }, 1 },                                                          // Бахмутський район

    // Житомирська область та її райони
    {   10, { 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163}, 21 }, // Житомирська область
    {   59, { 151, 152, 153, 154, 158, 159 }, 6 },                                     // Житомирський район
    {   58, { 143, 144, 145, 146, 160, 161, 162, 163 }, 8 },                                     // Коростенський район
    {   57, { 155, 156, 157 }, 3 },                                                    // Бердичівський район
    {   60, { 147, 148, 149, 150 }, 4 },                                                    // Звягельський район
    {  442, { 153 }, 1 },                                                         // м. Житомир та Житомирська територіальна громада

    // Закарпатська область та її райони
    {   11, {  87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97 }, 11 },                           // Закарпатська область
    {   66, { 95, 96}, 2 },                                                    // Ужгородський район
    {   61, { 92, 94 }, 2 },                                                         // Берегівський район
    {   62, { 90, 91 }, 2 },                                                         // Хустський район
    {   63, { 87 }, 1 },                                                         // Рахівський район
    {   64, { 88, 89 }, 2 },                                                         // Тячівський район
    {   65, { 93, 97 }, 2 },                                                         // Мукачівський район
    {  500, { 95 }, 1 },                                                         // м. Ужгород та Ужгородська територіальна громада

    // Запорізька область та її райони
    {   12, { 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290 }, 18 },                          // Запорізька область
    {  146, { 284, 285, 286 }, 3 },                                                          // Василівський район
    {  145, { 273, 274, 275, 277 }, 4 },                                                  // Пологівський район
    {  149, { 288, 289, 290 }, 3 },                                                      // Запорізький район
    {  147, { 276, 278, 279 }, 3 },                                                          // Бердянський район
    {  148, { 280, 281, 282, 283 }, 4 },                                                  // Мелітопольський район
    {  564, { 287 }, 1 },                                                          // м. Запоріжжя та Запорізька територіальна громада

    // Івано-Франківська область та її райони
    {   13, { 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86 }, 12 },                      // Івано-Франківська область
    {   68, { 78, 79, 80 }, 3 },                                              // Івано-Франківський район
    {   67, { 86 }, 1 },                                                         // Верховинський район
    {   71, { 81, 82, 83 }, 3 },                                                         // Калуський район
    {   72, { 84 }, 1 },                                                         // Надвірнянський район
    {   70, { 75, 76, 77 }, 3 },                                                         // Коломийський район
    {   69, { 85 }, 1 },                                                         // Косівський район
    {  632, { 78 }, 1 },                                                         // м. Івано-Франківськ та Івано-Франківська територіальна громада

    // Київська область та її райони
    {   14, { 164, 165, 166, 167, 168, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182 }, 18 }, // Київська область
    {   77, { 171 }, 1 },                                                         // Фастівський район
    {   73, { 173, 174, 175, 176 }, 4 },                                               // Білоцерківський район
    {   75, { 170 }, 1 },                                                         // Бучанський район
    {   76, { 172, 177 }, 2 },                                                    // Обухівський район
    {   74, { 164, 165, 166, 167, 168 }, 5 },                                                    // Вишгородський район
    {   79, { 180, 182 }, 2 },                                                    // Броварський район
    {   78, { 178, 179, 181 }, 3 },                                                         // Бориспільський район
    {   31, { 169 }, 1 },                                                         // м. Київ

    // Кіровоградська область та її райони
    {   15, { 342, 343, 344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357}, 16 },                     // Кіровоградська область
    {   81, { 348, 349, 350, 351, 352, 357 }, 6 },                                              // Кропивницький район
    {   80, { 353, 354, 355, 356}, 4 },                                                        // Олександрійський район
    {   82, { 342, 343, 344 }, 3 },                                                      // Голованівський район
    {   83, { 345, 346, 347}, 3 },                                                      // Новоукраїнський район
    {  761, { 350 }, 1 },                                                          // м. Кропивницький та Кропивницька територіальна громада

    // Луганська область та її райони
    {   16, { 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253 }, 15},      // Луганська область
    {   86, { 242, 243, 244 }, 3 },                                               // Старобільський район
    {   85, { 239, 240, 241 }, 3 },                                                    // Сватівський район
    {   84, { 245, 246 }, 2 },                                                         // Сєвєродонецький район
    {   87, { 248, 249 }, 2 },                                                         // Щастинський район

    // Львівська область та її райони
    {   27, { 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114 }, 17 },           // Львівська область
    {   90, { 105, 107, 108, 109, 110 }, 5 },                                               // Львівський район
    {   89, { 98, 104 }, 2 },                                                    // Стрийський район
    {   88, { 99, 100, 102 }, 3 },                                                         // Самбірський район
    {   91, { 103 }, 1 },                                                         // Дрогобицький район
    {   92, { 113, 114 }, 2 },                                                         // Червоноградський район
    {   94, { 111, 112 }, 2 },                                                         // Золочівський район
    {   93, { 101, 106 }, 2 },                                                         // Яворівський район
    {  845, { 105 }, 1 },                                                         // м. Львів та Львівська територіальна громада

    // Миколаївська область та її райони
    {   17, { 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372}, 15 },                               // Миколаївська область
    {   96, { 358, 359, 371, 372}, 4 },                                                      // Баштанський район
    {   95, { 360, 361, 364, 365}, 4 },                                                      // Вознесенський район
    {   97, { 362, 363 }, 2 },                                                          // Первомайський район
    {   98, { 366, 367, 368, 369, 370}, 5 },                                                   // Миколаївський район
    {  926, { 369 }, 1 },                                                          // м. Миколаїв та Миколаївська територіальна громада

    // Одеська область та її райони
    {   18, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22}, 23 },          // Одеська область
    {  105, { 1, 4, 5}, 3 },                                                        // Болградський район
    {  100, { 11, 12, 13, 17 }, 4 },                                                      // Березівський район
    {  104, { 8, 9, 10}, 3 },                                                        // Одеський район
    {  102, { 3, 6, 7}, 3 },                                                        // Білгород-Дністровський район
    {  103, { 14, 15, 16}, 3 },                                                       // Роздільнянський район
    {  101, { 0, 2}, 2 },                                                        // Ізмаїльський район
    {   99, { 18, 19, 20, 21, 22 }, 5 },                                                  // Подільський район
    {  964, { 10 }, 1 },                                                           // м. Одеса та Одеська територіальна громада

    // Полтавська область та її райони
    {   19, { 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327}, 19 }, // Полтавська область
    {  107, { 318, 325, 326, 327 }, 4 },                                                    // Кременчуцький район
    {  106, { 315, 316, 317, 319}, 4 },                                                    // Лубенський район
    {  109, { 309, 310, 311, 320, 321, 322, 323, 324}, 8 },                           // Полтавський район
    {  108, { 312, 313, 314}, 3 },                                                    // Миргородський район
    { 1060, { 320 }, 1 },                                                         // м. Полтава та Полтавська територіальна громада

    // Рівненська область та її райони
    {    5, { 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142}, 14 },      // Рівненська область
    {  110, { 129, 130}, 2 },                                                    // Вараський район
    {  111, { 132, 133}, 2 },                                                    // Дубенський район
    {  112, { 131, 134, 135, 136, 137}, 5 },                                          // Рівненський район
    {  113, { 138, 139, 140, 141, 142}, 5 },                                               // Сарненський район
    { 1133, { 135 }, 1 },                                                         // м. Рівне та Рівненська територіальна громада

    // Сумська область та її райони
    {   20, { 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216}, 15 }, // Сумська область
    {  115, { 206, 207, 208 }, 3 },                                                    // Шосткинський район
    {  116, { 202, 203 }, 2 },                                                    // Роменський район
    {  117, { 204, 205, 209 }, 3 },                                               // Конотопський район
    {  114, { 210, 211, 212, 213, 214 }, 5 },                                          // Сумський район
    {  118, { 215, 216}, 2 },                                                         // Охтирський район
    { 1187, { 213 }, 1 },                                                         // м. Суми та Сумська територіальна громада

    // Тернопільська область та її райони
    {   21, { 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74}, 11 },                      // Тернопільська область
    {  119, { 66, 68, 71, 72, 73 }, 5 },                                          // Тернопільський район
    {  121, { 64, 65, 67, 74}, 4 },                                               // Чортківський район
    {  120, { 69, 70 }, 2 },                                                         // Кременецький район
    { 1241, { 72 }, 1 },                                                         // м. Тернопіль та Тернопільська територіальна громада

    // Харківська область та її райони
    {   22, { 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238}, 22 },      // Харківська область
    {  124, { 227, 229 }, 2 },                                                         // Харківський район
    {  123, { 237, 238}, 2 },                                                         // Куп'янський район
    {  122, { 230, 231, 232 }, 3 },                                                    // Чугуївський район
    {  126, { 217, 218, 219, 220 }, 4 },                                                         // Богодухівський район
    {  127, { 221, 222, 223 }, 3 },                                                         // Красноградський район
    {  125, { 233, 234, 235, 236}, 4 },                                               // Ізюмський район
    {  128, { 224, 225, 226}, 3 },                                                    // Лозівський район
    { 1293, { 228}, 1 },                                                         // м. Харків та Харківська територіальна громада

    // Херсонська область та її райони
    {   23, { 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389, 390, 391}, 19 },                     // Херсонська область
    {  131, { 382, 383, 384, 386}, 4 },                                                      // Каховський район
    {  129, { 373, 374, 385 }, 3 },                                                          // Бериславський район
    {  130, { 377, 378, 379, 381 }, 4 },                                                  // Скадовський район
    {  132, { 375, 376, 380}, 3 },                                                  // Херсонський район
    {  133, { 387, 388, 389, 390, 391 }, 5 },                                                      // Генічеський район
    { 1370, { 380 }, 1 },                                                          // м. Херсон та Херсонська територіальна громада

    // Хмельницька область та її райони
    {    3, { 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56 }, 16 },      // Хмельницька область
    {  136, { 45, 46, 47, 48 }, 4 },                                               // Шепетівський район
    {  134, { 41, 42, 43, 44, 49, 50, 51, 52 }, 8 },                                // Хмельницький район
    {  135, { 53, 54, 55, 56}, 4 },                                                    // Кам'янець-Подільський район
    { 1400, { 51 }, 1 },                                                         // м. Хмельницький та Хмельницька територіальна громада

    // Черкаська область та її райони
    {   24, { 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341 }, 14 },             // Черкаська область
    {  153, { 330, 331, 332 }, 3 },                                                    // Золотоніський район
    {  152, { 328, 329, 333, 334, 335 }, 5 },                                          // Черкаський район
    {  150, { 336, 337, 338 }, 3 },                                                     // Звенигородський район
    {  151, { 339, 340, 341}, 3 },                                                    // Уманський район
    { 1473, { 329 }, 1 },                                                         // м. Черкаси та Черкаська територіальна громада

    // Чернівецька область та її райони
    {   26, { 57, 58, 59, 60, 61, 62, 63 }, 7 },                                // Чернівецька область
    {  139, { 57, 58 }, 2 },                                                    // Дністровський район
    {  138, { 62}, 1 },                                                         // Вижницький район
    {  137, { 59, 60, 61, 63}, 4 },                                               // Чернівецький район
    { 1542, { 60 }, 1 },                                                         // м. Чернівці та Чернівецька територіальна громада

    // Чернігівська область та її райони
    {   25, { 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201}, 19 }, // Чернігівська область
    {  141, { 197, 198, 199}, 3 },                                                    // Новгород-Сіверський район
    {  142, { 189,190, 193, 200 }, 4 },                                               // Ніжинський район
    {  143, { 191, 192, 201}, 3 },                                                    // Прилуцький район
    {  140, { 183, 184, 185, 186, 187, 188}, 6 },                                          // Чернігівський район
    {  144, { 194, 195, 196}, 3 },                                               // Корюківський район
    { 1591, { 188 }, 1 },                                                         // м. Чернігів та Чернігівська територіальна громада
};

// Фіксований масив відповідностей (заповнюється вручну)
const RegionLedMapEntry STATE_MAP_LED_ODESA_WITH_KYIV[MAX_REGIONS] = {
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
    { 155,  { 6 },  1 },    // м. Вінниця

    // Волинська область та її райони
    { 8,    { 13 }, 1 },    // Волинська область
    { 39,   { 13 }, 1 },    // Луцький район
    { 38,   { 13 }, 1 },    // Володимир-Волинський район
    { 40,   { 13 }, 1 },    // Ковельський район
    { 41,   { 13 }, 1 },    // Камінь-Каширський район
    { 225,  { 13 }, 1 },    // м. Луцьк

    // Дніпропетровська область та її райони
    { 9,    { 2 },  1 },    // Дніпропетровська область
    { 43,   { 2 },  1 },    // Новомосковський район
    { 44,   { 2 },  1 },    // Дніпровський район
    { 47,   { 2 },  1 },    // Нікопольський район
    { 48,   { 2 },  1 },    // Синельниківський район
    { 42,   { 2 },  1 },    // Кам'янський район
    { 45,   { 2 },  1 },    // Павлоградський район
    { 46,   { 2 },  1 },    // Криворізький район
    { 332,  { 2 },  1 },    // м. Дніпро

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
    { 442,  { 15 }, 1 },    // м. Житомир

    // Закарпатська область та її райони
    { 11,   { 9 },  1 },    // Закарпатська область
    { 66,   { 9 },  1 },    // Ужгородський район
    { 61,   { 9 },  1 },    // Берегівський район
    { 62,   { 9 },  1 },    // Хустський район
    { 63,   { 9 },  1 },    // Рахівський район
    { 64,   { 9 },  1 },    // Тячівський район
    { 65,   { 9 },  1 },    // Мукачівський район
    { 500,  { 9 },  1 },    // м. Ужгород

    // Запорізька область та її райони
    { 12,   { 23 }, 1 },    // Запорізька область
    { 146,  { 23 }, 1 },    // Василівський район
    { 145,  { 23 }, 1 },    // Пологівський район
    { 149,  { 23 }, 1 },    // Запорізький район
    { 147,  { 23 }, 1 },    // Бердянський район
    { 148,  { 23 }, 1 },    // Мелітопольський район
    //{ 564,  { 23 }, 1 },    // м. Запоріжжя

    // Івано-Франківська область та її райони
    { 13,   { 10 }, 1 },    // Івано-Франківська область
    { 68,   { 10 }, 1 },    // Івано-Франківський район
    { 67,   { 10 }, 1 },    // Верховинський район
    { 71,   { 10 }, 1 },    // Калуський район
    { 72,   { 10 }, 1 },    // Надвірнянський район
    { 70,   { 10 }, 1 },    // Коломийський район
    { 69,   { 10 }, 1 },    // Косівський район
    { 632,  { 10 }, 1 },    // м. Івано-Франківськ

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
    { 761,  { 5 },  1 },    // м. Кропивницький

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
    { 845,  { 12 }, 1 },    // м. Львів

    // Миколаївська область та її райони
    { 17,   { 1 },  1 },    // Миколаївська область
    { 96,   { 1 },  1 },    // Баштанський район
    { 95,   { 1 },  1 },    // Вознесенський район
    { 97,   { 1 },  1 },    // Первомайський район
    { 98,   { 1 },  1 },    // Миколаївський район
    { 926,  { 1 },  1 },    // м. Миколаїв

    // Одеська область та її райони
    { 18,   { 0 },  1 },    // Одеська область
    { 105,  { 0 },  1 },    // Болградський район
    { 100,  { 0 },  1 },    // Березівський район
    { 104,  { 0 },  1 },    // Одеський район
    { 102,  { 0 },  1 },    // Білгород-Дністровський район
    { 103,  { 0 },  1 },    // Роздільнянський район
    { 101,  { 0 },  1 },    // Ізмаїльський район
    { 99,   { 0 },  1 },    // Подільський район
    { 964,  { 0 },  1 },    // м. Одеса

    // Полтавська область та її райони
    { 19,   { 3 },  1 },    // Полтавська область
    { 107,  { 3 },  1 },    // Кременчуцький район
    { 106,  { 3 },  1 },    // Лубенський район
    { 109,  { 3 },  1 },    // Полтавський район
    { 108,  { 3 },  1 },    // Миргородський район
    { 1060, { 3 },  1 },    // м. Полтава

    // Рівненська область та її райони
    { 5,    { 14 }, 1 },    // Рівненська область
    { 110,  { 14 }, 1 },    // Вараський район
    { 111,  { 14 }, 1 },    // Дубенський район
    { 112,  { 14 }, 1 },    // Рівненський район
    { 113,  { 14 }, 1 },    // Сарненський район
    { 1133, { 14 }, 1 },    // м. Рівне

    // Сумська область та її райони
    { 20,   { 19 }, 1 },    // Сумська область
    { 115,  { 19 }, 1 },    // Шосткинський район
    { 116,  { 19 }, 1 },    // Роменський район
    { 117,  { 19 }, 1 },    // Конотопський район
    { 114,  { 19 }, 1 },    // Сумський район
    { 118,  { 19 }, 1 },    // Охтирський район
    { 1187, { 19 }, 1 },    // м. Суми

    // Тернопільська область та її райони
    { 21,   { 11 }, 1 },    // Тернопільська область
    { 119,  { 11 }, 1 },    // Тернопільський район
    { 121,  { 11 }, 1 },    // Чортківський район
    { 120,  { 11 }, 1 },    // Кременецький район
    { 1241, { 11 }, 1 },    // м. Тернопіль

    // Харківська область та її райони
    { 22,   { 20 }, 1 },    // Харківська область
    { 124,  { 20 }, 1 },    // Харківський район
    { 123,  { 20 }, 1 },    // Куп'янський район
    { 122,  { 20 }, 1 },    // Чугуївський район
    { 126,  { 20 }, 1 },    // Богодухівський район
    { 127,  { 20 }, 1 },    // Красноградський район
    { 125,  { 20 }, 1 },    // Ізюмський район
    { 128,  { 20 }, 1 },    // Лозівський район
    //{1293,  { 20 }, 1 },    // м. Харків

    // Херсонська область та її райони
    { 23,   { 24 }, 1 },    // Херсонська область
    { 131,  { 24 }, 1 },    // Каховський район
    { 129,  { 24 }, 1 },    // Бериславський район
    { 130,  { 24 }, 1 },    // Скадовський район
    { 132,  { 24 }, 1 },    // Херсонський район
    { 133,  { 24 }, 1 },    // Генічеський район
    { 1370, { 24 }, 1 },    // м. Херсон

    // Хмельницька область та її райони
    { 3,    { 7 },  1 },    // Хмельницька область
    { 136,  { 7 },  1 },    // Шепетівський район
    { 134,  { 7 },  1 },    // Хмельницький район
    { 135,  { 7 },  1 },    // Кам'янець-Подільський район
    { 1400, { 7 },  1 },    // м. Хмельницький

    // Черкаська область та її райони
    { 24,   { 4 },  1 },    // Черкаська область
    { 153,  { 4 },  1 },    // Золотоніський район
    { 152,  { 4 },  1 },    // Черкаський район
    { 150,  { 4 },  1 },    // Звенигородський район
    { 151,  { 4 },  1 },    // Уманський район
    { 1473, { 4 },  1 },    // м. Черкаси

    // Чернівецька область та її райони
    { 26,   { 8 },  1 },    // Чернівецька область
    { 139,  { 8 },  1 },    // Дністровський район
    { 138,  { 8 },  1 },    // Вижницький район
    { 137,  { 8 },  1 },    // Чернівецький район
    { 1542, { 8 },  1 },    // м. Чернівці

    // Чернігівська область та її райони
    { 25,   { 18 }, 1 },    // Чернігівська область
    { 141,  { 18 }, 1 },    // Новгород-Сіверський район
    { 142,  { 18 }, 1 },    // Ніжинський район
    { 143,  { 18 }, 1 },    // Прилуцький район
    { 140,  { 18 }, 1 },    // Чернігівський район
    { 144,  { 18 }, 1 },    // Корюківський район
    { 1591, { 18 }, 1 },    // м. Чернігів
};

// Фіксований масив відповідностей (заповнюється вручну)
const RegionLedMapEntry STATE_MAP_LED_ODESA_WITHOUT_KYIV[MAX_REGIONS] = {
    // АР Крим
    {9999,  { 24 }, 1 },    // Автономна Республіка Крим

    // Вінницька область та її райони
    { 4,    { 6 },  1 },    // Вінницька область
    { 32,   { 6 },  1 },    // Тульчинський район
    { 35,   { 6 },  1 },    // Жмеринський район
    { 36,   { 6 },  1 },    // Вінницький район
    { 34,   { 6 },  1 },    // Хмільницький район
    { 33,   { 6 },  1 },    // Могилів-Подільський район
    { 37,   { 6 },  1 },    // Гайсинський район
    { 155,  { 6 },  1 },    // м. Вінниця

    // Волинська область та її райони
    { 8,    { 13 }, 1 },    // Волинська область
    { 39,   { 13 }, 1 },    // Луцький район
    { 38,   { 13 }, 1 },    // Володимир-Волинський район
    { 40,   { 13 }, 1 },    // Ковельський район
    { 41,   { 13 }, 1 },    // Камінь-Каширський район
    { 225,  { 13 }, 1 },    // м. Луцьк

    // Дніпропетровська область та її райони
    { 9,    { 2 },  1 },    // Дніпропетровська область
    { 43,   { 2 },  1 },    // Новомосковський район
    { 44,   { 2 },  1 },    // Дніпровський район
    { 47,   { 2 },  1 },    // Нікопольський район
    { 48,   { 2 },  1 },    // Синельниківський район
    { 42,   { 2 },  1 },    // Кам'янський район
    { 45,   { 2 },  1 },    // Павлоградський район
    { 46,   { 2 },  1 },    // Криворізький район
    { 332,  { 2 },  1 },    // м. Дніпро

    // Донецька область та її райони
    { 28,   { 21 }, 1 },    // Донецька область
    { 56,   { 21 }, 1 },    // Покровський район
    { 51,   { 21 }, 1 },    // Горлівський район
    { 55,   { 21 }, 1 },    // Волноваський район
    { 53,   { 21 }, 1 },    // Донецький район
    { 49,   { 21 }, 1 },    // Кальміуський район
    { 52,   { 21 }, 1 },    // Маріупольський район
    { 50,   { 21 }, 1 },    // Краматорський район
    { 54,   { 21 }, 1 },    // Бахмутський район

    // Житомирська область та її райони
    { 10,   { 15 }, 1 },    // Житомирська область
    { 59,   { 15 }, 1 },    // Житомирський район
    { 58,   { 15 }, 1 },    // Коростенський район
    { 57,   { 15 }, 1 },    // Бердичівський район
    { 60,   { 15 }, 1 },    // Звягельський район
    { 442,  { 15 }, 1 },    // м. Житомир

    // Закарпатська область та її райони
    { 11,   { 9 },  1 },    // Закарпатська область
    { 66,   { 9 },  1 },    // Ужгородський район
    { 61,   { 9 },  1 },    // Берегівський район
    { 62,   { 9 },  1 },    // Хустський район
    { 63,   { 9 },  1 },    // Рахівський район
    { 64,   { 9 },  1 },    // Тячівський район
    { 65,   { 9 },  1 },    // Мукачівський район
    { 500,  { 9 },  1 },    // м. Ужгород

    // Запорізька область та її райони
    { 12,   { 22 }, 1 },    // Запорізька область
    { 146,  { 22 }, 1 },    // Василівський район
    { 145,  { 22 }, 1 },    // Пологівський район
    { 149,  { 22 }, 1 },    // Запорізький район
    { 147,  { 22 }, 1 },    // Бердянський район
    { 148,  { 22 }, 1 },    // Мелітопольський район
    //{ 564,  { 22 }, 1 },    // м. Запоріжжя

    // Івано-Франківська область та її райони
    { 13,   { 10 }, 1 },    // Івано-Франківська область
    { 68,   { 10 }, 1 },    // Івано-Франківський район
    { 67,   { 10 }, 1 },    // Верховинський район
    { 71,   { 10 }, 1 },    // Калуський район
    { 72,   { 10 }, 1 },    // Надвірнянський район
    { 70,   { 10 }, 1 },    // Коломийський район
    { 69,   { 10 }, 1 },    // Косівський район
    { 632,  { 10 }, 1 },    // м. Івано-Франківськ

    // Київська область та її райони
    { 14,   { 16 }, 1 },    // Київська область
    { 77,   { 16 }, 1 },    // Фастівський район
    { 73,   { 16 }, 1 },    // Білоцерківський район
    { 75,   { 16 }, 1 },    // Бучанський район
    { 76,   { 16 }, 1 },    // Обухівський район
    { 74,   { 16 }, 1 },    // Вишгородський район
    { 79,   { 16 }, 1 },    // Броварський район
    { 78,   { 16 }, 1 },    // Бориспільський район
    //{ 31,   { 25 }, 1 },    // м. Київ

    // Кіровоградська область та її райони
    { 15,   { 5 },  1 },    // Кіровоградська область
    { 81,   { 5 },  1 },    // Кропивницький район
    { 80,   { 5 },  1 },    // Олександрійський район
    { 82,   { 5 },  1 },    // Голованівський район
    { 83,   { 5 },  1 },    // Новоукраїнський район
    { 761,  { 5 },  1 },    // м. Кропивницький

    // Луганська область та її райони
    { 16,   { 20 }, 1 },    // Луганська область
    { 86,   { 20 }, 1 },    // Старобільський район
    { 85,   { 20 }, 1 },    // Сватівський район
    { 84,   { 20 }, 1 },    // Сєвєродонецький район
    { 87,   { 20 }, 1 },    // Щастинський район

    // Львівська область та її райони
    { 27,   { 12 }, 1 },    // Львівська область
    { 90,   { 12 }, 1 },    // Львівський район
    { 89,   { 12 }, 1 },    // Стрийський район
    { 88,   { 12 }, 1 },    // Самбірський район
    { 91,   { 12 }, 1 },    // Дрогобицький район
    { 92,   { 12 }, 1 },    // Червоноградський район
    { 94,   { 12 }, 1 },    // Золочівський район
    { 93,   { 12 }, 1 },    // Яворівський район
    { 845,  { 12 }, 1 },    // м. Львів

    // Миколаївська область та її райони
    { 17,   { 1 },  1 },    // Миколаївська область
    { 96,   { 1 },  1 },    // Баштанський район
    { 95,   { 1 },  1 },    // Вознесенський район
    { 97,   { 1 },  1 },    // Первомайський район
    { 98,   { 1 },  1 },    // Миколаївський район
    { 926,  { 1 },  1 },    // м. Миколаїв

    // Одеська область та її райони
    { 18,   { 0 },  1 },    // Одеська область
    { 105,  { 0 },  1 },    // Болградський район
    { 100,  { 0 },  1 },    // Березівський район
    { 104,  { 0 },  1 },    // Одеський район
    { 102,  { 0 },  1 },    // Білгород-Дністровський район
    { 103,  { 0 },  1 },    // Роздільнянський район
    { 101,  { 0 },  1 },    // Ізмаїльський район
    { 99,   { 0 },  1 },    // Подільський район
    { 964,  { 0 },  1 },    // м. Одеса

    // Полтавська область та її райони
    { 19,   { 3 },  1 },    // Полтавська область
    { 107,  { 3 },  1 },    // Кременчуцький район
    { 106,  { 3 },  1 },    // Лубенський район
    { 109,  { 3 },  1 },    // Полтавський район
    { 108,  { 3 },  1 },    // Миргородський район
    { 1060, { 3 },  1 },    // м. Полтава

    // Рівненська область та її райони
    { 5,    { 14 }, 1 },    // Рівненська область
    { 110,  { 14 }, 1 },    // Вараський район
    { 111,  { 14 }, 1 },    // Дубенський район
    { 112,  { 14 }, 1 },    // Рівненський район
    { 113,  { 14 }, 1 },    // Сарненський район
    { 1133, { 14 }, 1 },    // м. Рівне

    // Сумська область та її райони
    { 20,   { 18 }, 1 },    // Сумська область
    { 115,  { 18 }, 1 },    // Шосткинський район
    { 116,  { 18 }, 1 },    // Роменський район
    { 117,  { 18 }, 1 },    // Конотопський район
    { 114,  { 18 }, 1 },    // Сумський район
    { 118,  { 18 }, 1 },    // Охтирський район
    { 1187, { 18 }, 1 },    // м. Суми

    // Тернопільська область та її райони
    { 21,   { 11 }, 1 },    // Тернопільська область
    { 119,  { 11 }, 1 },    // Тернопільський район
    { 121,  { 11 }, 1 },    // Чортківський район
    { 120,  { 11 }, 1 },    // Кременецький район
    { 1241, { 11 }, 1 },    // м. Тернопіль

    // Харківська область та її райони
    { 22,   { 19 }, 1 },    // Харківська область
    { 124,  { 19 }, 1 },    // Харківський район
    { 123,  { 19 }, 1 },    // Куп'янський район
    { 122,  { 19 }, 1 },    // Чугуївський район
    { 126,  { 19 }, 1 },    // Богодухівський район
    { 127,  { 19 }, 1 },    // Красноградський район
    { 125,  { 19 }, 1 },    // Ізюмський район
    { 128,  { 19 }, 1 },    // Лозівський район
    //{1293,  { 19 }, 1 },    // м. Харків

    // Херсонська область та її райони
    { 23,   { 23 }, 1 },    // Херсонська область
    { 131,  { 23 }, 1 },    // Каховський район
    { 129,  { 23 }, 1 },    // Бериславський район
    { 130,  { 23 }, 1 },    // Скадовський район
    { 132,  { 23 }, 1 },    // Херсонський район
    { 133,  { 23 }, 1 },    // Генічеський район
    { 1370, { 23 }, 1 },    // м. Херсон

    // Хмельницька область та її райони
    { 3,    { 7 },  1 },    // Хмельницька область
    { 136,  { 7 },  1 },    // Шепетівський район
    { 134,  { 7 },  1 },    // Хмельницький район
    { 135,  { 7 },  1 },    // Кам'янець-Подільський район
    { 1400, { 7 },  1 },    // м. Хмельницький

    // Черкаська область та її райони
    { 24,   { 4 },  1 },    // Черкаська область
    { 153,  { 4 },  1 },    // Золотоніський район
    { 152,  { 4 },  1 },    // Черкаський район
    { 150,  { 4 },  1 },    // Звенигородський район
    { 151,  { 4 },  1 },    // Уманський район
    { 1473, { 4 },  1 },    // м. Черкаси

    // Чернівецька область та її райони
    { 26,   { 8 },  1 },    // Чернівецька область
    { 139,  { 8 },  1 },    // Дністровський район
    { 138,  { 8 },  1 },    // Вижницький район
    { 137,  { 8 },  1 },    // Чернівецький район
    { 1542, { 8 },  1 },    // м. Чернівці

    // Чернігівська область та її райони
    { 25,   { 17 }, 1 },    // Чернігівська область
    { 141,  { 17 }, 1 },    // Новгород-Сіверський район
    { 142,  { 17 }, 1 },    // Ніжинський район
    { 143,  { 17 }, 1 },    // Прилуцький район
    { 140,  { 17 }, 1 },    // Чернігівський район
    { 144,  { 17 }, 1 },    // Корюківський район
    { 1591, { 17 }, 1 },    // м. Чернігів
};

// Фіксований масив відповідностей (заповнюється вручну)
const RegionLedMapEntry STATE_MAP_LED_TRANSCARPATHIA_WITH_KYIV[MAX_REGIONS] = {
    // АР Крим
    {9999,  { 16 }, 1 },    // Автономна Республіка Крим

    // Вінницька область та її райони
    { 4,    { 23 },  1 },    // Вінницька область
    { 32,   { 23 },  1 },    // Тульчинський район
    { 35,   { 23 },  1 },    // Жмеринський район
    { 36,   { 23 },  1 },    // Вінницький район
    { 34,   { 23 },  1 },    // Хмільницький район
    { 33,   { 23 },  1 },    // Могилів-Подільський район
    { 37,   { 23 },  1 },    // Гайсинський район
    { 155,  { 23 },  1 },    // м. Вінниця

    // Волинська область та її райони
    { 8,    { 4 }, 1 },     // Волинська область
    { 39,   { 4 }, 1 },     // Луцький район
    { 38,   { 4 }, 1 },     // Володимир-Волинський район
    { 40,   { 4 }, 1 },     // Ковельський район
    { 41,   { 4 }, 1 },     // Камінь-Каширський район
    { 225,  { 4 }, 1 },     // м. Луцьк

    // Дніпропетровська область та її райони
    { 9,    { 19 },  1 },   // Дніпропетровська область
    { 43,   { 19 },  1 },   // Новомосковський район
    { 44,   { 19 },  1 },   // Дніпровський район
    { 47,   { 19 },  1 },   // Нікопольський район
    { 48,   { 19 },  1 },   // Синельниківський район
    { 42,   { 19 },  1 },   // Кам'янський район
    { 45,   { 19 },  1 },   // Павлоградський район
    { 46,   { 19 },  1 },   // Криворізький район
    { 332,  { 19 },  1 },   // м. Дніпро

    // Донецька область та її райони
    { 28,   { 13 }, 1 },    // Донецька область
    { 56,   { 13 }, 1 },    // Покровський район
    { 51,   { 13 }, 1 },    // Горлівський район
    { 55,   { 13 }, 1 },    // Волноваський район
    { 53,   { 13 }, 1 },    // Донецький район
    { 49,   { 13 }, 1 },    // Кальміуський район
    { 52,   { 13 }, 1 },    // Маріупольський район
    { 50,   { 13 }, 1 },    // Краматорський район
    { 54,   { 13 }, 1 },    // Бахмутський район

    // Житомирська область та її райони
    { 10,   { 6 }, 1 },     // Житомирська область
    { 59,   { 6 }, 1 },     // Житомирський район
    { 58,   { 6 }, 1 },     // Коростенський район
    { 57,   { 6 }, 1 },     // Бердичівський район
    { 60,   { 6 }, 1 },     // Звягельський район
    { 442,  { 6 }, 1 },     // м. Житомир

    // Закарпатська область та її райони
    { 11,   { 0 },  1 },    // Закарпатська область
    { 66,   { 0 },  1 },    // Ужгородський район
    { 61,   { 0 },  1 },    // Берегівський район
    { 62,   { 0 },  1 },    // Хустський район
    { 63,   { 0 },  1 },    // Рахівський район
    { 64,   { 0 },  1 },    // Тячівський район
    { 65,   { 0 },  1 },    // Мукачівський район
    { 500,  { 0 },  1 },    // м. Ужгород

    // Запорізька область та її райони
    { 12,   { 14 }, 1 },    // Запорізька область
    { 146,  { 14 }, 1 },    // Василівський район
    { 145,  { 14 }, 1 },    // Пологівський район
    { 149,  { 14 }, 1 },    // Запорізький район
    { 147,  { 14 }, 1 },    // Бердянський район
    { 148,  { 14 }, 1 },    // Мелітопольський район
    //{ 564,  { 14 }, 1 },    // м. Запоріжжя

    // Івано-Франківська область та її райони
    { 13,   { 1 }, 1 },     // Івано-Франківська область
    { 68,   { 1 }, 1 },     // Івано-Франківський район
    { 67,   { 1 }, 1 },     // Верховинський район
    { 71,   { 1 }, 1 },     // Калуський район
    { 72,   { 1 }, 1 },     // Надвірнянський район
    { 70,   { 1 }, 1 },     // Коломийський район
    { 69,   { 1 }, 1 },     // Косівський район
    { 632,  { 1 }, 1 },     // м. Івано-Франківськ

    // Київська область та її райони
    { 14,   { 7 }, 1 },     // Київська область
    { 77,   { 7 }, 1 },     // Фастівський район
    { 73,   { 7 }, 1 },     // Білоцерківський район
    { 75,   { 7 }, 1 },     // Бучанський район
    { 76,   { 7 }, 1 },     // Обухівський район
    { 74,   { 7 }, 1 },     // Вишгородський район
    { 79,   { 7 }, 1 },     // Броварський район
    { 78,   { 7 }, 1 },     // Бориспільський район
    { 31,   { 8 }, 1 },     // м. Київ

    // Кіровоградська область та її райони
    { 15,   { 22 },  1 },   // Кіровоградська область
    { 81,   { 22 },  1 },   // Кропивницький район
    { 80,   { 22 },  1 },   // Олександрійський район
    { 82,   { 22 },  1 },   // Голованівський район
    { 83,   { 22 },  1 },   // Новоукраїнський район
    { 761,  { 22 },  1 },   // м. Кропивницький

    // Луганська область та її райони
    { 16,   { 12 }, 1 },    // Луганська область
    { 86,   { 12 }, 1 },    // Старобільський район
    { 85,   { 12 }, 1 },    // Сватівський район
    { 84,   { 12 }, 1 },    // Сєвєродонецький район
    { 87,   { 12 }, 1 },    // Щастинський район

    // Львівська область та її райони
    { 27,   { 3 }, 1 },     // Львівська область
    { 90,   { 3 }, 1 },     // Львівський район
    { 89,   { 3 }, 1 },     // Стрийський район
    { 88,   { 3 }, 1 },     // Самбірський район
    { 91,   { 3 }, 1 },     // Дрогобицький район
    { 92,   { 3 }, 1 },     // Червоноградський район
    { 94,   { 3 }, 1 },     // Золочівський район
    { 93,   { 3 }, 1 },     // Яворівський район
    { 845,  { 3 }, 1 },     // м. Львів

    // Миколаївська область та її райони
    { 17,   { 18 },  1 },   // Миколаївська область
    { 96,   { 18 },  1 },   // Баштанський район
    { 95,   { 18 },  1 },   // Вознесенський район
    { 97,   { 18 },  1 },   // Первомайський район
    { 98,   { 18 },  1 },   // Миколаївський район
    { 926,  { 18 },  1 },   // м. Миколаїв

    // Одеська область та її райони
    { 18,   { 17 },  1 },   // Одеська область
    { 105,  { 17 },  1 },   // Болградський район
    { 100,  { 17 },  1 },   // Березівський район
    { 104,  { 17 },  1 },   // Одеський район
    { 102,  { 17 },  1 },   // Білгород-Дністровський район
    { 103,  { 17 },  1 },   // Роздільнянський район
    { 101,  { 17 },  1 },   // Ізмаїльський район
    { 99,   { 17 },  1 },   // Подільський район
    { 964,  { 17 },  1 },   // м. Одеса

    // Полтавська область та її райони
    { 19,   { 20 },  1 },   // Полтавська область
    { 107,  { 20 },  1 },   // Кременчуцький район
    { 106,  { 20 },  1 },   // Лубенський район
    { 109,  { 20 },  1 },   // Полтавський район
    { 108,  { 20 },  1 },   // Миргородський район
    { 1060, { 20 },  1 },   // м. Полтава

    // Рівненська область та її райони
    { 5,    { 5 }, 1 },     // Рівненська область
    { 110,  { 5 }, 1 },     // Вараський район
    { 111,  { 5 }, 1 },     // Дубенський район
    { 112,  { 5 }, 1 },     // Рівненський район
    { 113,  { 5 }, 1 },     // Сарненський район
    { 1133, { 5 }, 1 },     // м. Рівне

    // Сумська область та її райони
    { 20,   { 10 }, 1 },     // Сумська область
    { 115,  { 10 }, 1 },     // Шосткинський район
    { 116,  { 10 }, 1 },     // Роменський район
    { 117,  { 10 }, 1 },     // Конотопський район
    { 114,  { 10 }, 1 },     // Сумський район
    { 118,  { 10 }, 1 },     // Охтирський район
    { 1187, { 10 }, 1 },     // м. Суми

    // Тернопільська область та її райони
    { 21,   { 2 }, 1 },     // Тернопільська область
    { 119,  { 2 }, 1 },     // Тернопільський район
    { 121,  { 2 }, 1 },     // Чортківський район
    { 120,  { 2 }, 1 },     // Кременецький район
    { 1241, { 2 }, 1 },     // м. Тернопіль

    // Харківська область та її райони
    { 22,   { 11 }, 1 },    // Харківська область
    { 124,  { 11 }, 1 },    // Харківський район
    { 123,  { 11 }, 1 },    // Куп'янський район
    { 122,  { 11 }, 1 },    // Чугуївський район
    { 126,  { 11 }, 1 },    // Богодухівський район
    { 127,  { 11 }, 1 },    // Красноградський район
    { 125,  { 11 }, 1 },    // Ізюмський район
    { 128,  { 11 }, 1 },    // Лозівський район
    //{1293,  { 11 }, 1 },    // м. Харків

    // Херсонська область та її райони
    { 23,   { 15 }, 1 },    // Херсонська область
    { 131,  { 15 }, 1 },    // Каховський район
    { 129,  { 15 }, 1 },    // Бериславський район
    { 130,  { 15 }, 1 },    // Скадовський район
    { 132,  { 15 }, 1 },    // Херсонський район
    { 133,  { 15 }, 1 },    // Генічеський район
    { 1370, { 15 }, 1 },    // м. Херсон

    // Хмельницька область та її райони
    { 3,    { 24 },  1 },   // Хмельницька область
    { 136,  { 24 },  1 },   // Шепетівський район
    { 134,  { 24 },  1 },   // Хмельницький район
    { 135,  { 24 },  1 },   // Кам'янець-Подільський район
    { 1400, { 24 },  1 },   // м. Хмельницький

    // Черкаська область та її райони
    { 24,   { 21 },  1 },   // Черкаська область
    { 153,  { 21 },  1 },   // Золотоніський район
    { 152,  { 21 },  1 },   // Черкаський район
    { 150,  { 21 },  1 },   // Звенигородський район
    { 151,  { 21 },  1 },   // Уманський район
    { 1473, { 21 },  1 },   // м. Черкаси

    // Чернівецька область та її райони
    { 26,   { 25 },  1 },   // Чернівецька область
    { 139,  { 25 },  1 },   // Дністровський район
    { 138,  { 25 },  1 },   // Вижницький район
    { 137,  { 25 },  1 },   // Чернівецький район
    { 1542, { 25 },  1 },   // м. Чернівці

    // Чернігівська область та її райони
    { 25,   { 9 }, 1 },     // Чернігівська область
    { 141,  { 9 }, 1 },     // Новгород-Сіверський район
    { 142,  { 9 }, 1 },     // Ніжинський район
    { 143,  { 9 }, 1 },     // Прилуцький район
    { 140,  { 9 }, 1 },     // Чернігівський район
    { 144,  { 9 }, 1 },     // Корюківський район
    { 1591, { 9 }, 1 },     // м. Чернігів
};

// Фіксований масив відповідностей (заповнюється вручну)
const RegionLedMapEntry STATE_MAP_LED_TRANSCARPATHIA_WITHOUT_KYIV[MAX_REGIONS] = {
    // АР Крим
    {9999,  { 15 }, 1 },    // Автономна Республіка Крим

    // Вінницька область та її райони
    { 4,    { 22 },  1 },    // Вінницька область
    { 32,   { 22 },  1 },    // Тульчинський район
    { 35,   { 22 },  1 },    // Жмеринський район
    { 36,   { 22 },  1 },    // Вінницький район
    { 34,   { 22 },  1 },    // Хмільницький район
    { 33,   { 22 },  1 },    // Могилів-Подільський район
    { 37,   { 22 },  1 },    // Гайсинський район
    { 155,  { 22 },  1 },    // м. Вінниця

    // Волинська область та її райони
    { 8,    { 4 }, 1 },    // Волинська область
    { 39,   { 4 }, 1 },    // Луцький район
    { 38,   { 4 }, 1 },    // Володимир-Волинський район
    { 40,   { 4 }, 1 },    // Ковельський район
    { 41,   { 4 }, 1 },    // Камінь-Каширський район
    { 225,  { 4 }, 1 },    // м. Луцьк

    // Дніпропетровська область та її райони
    { 9,    { 18 },  1 },    // Дніпропетровська область
    { 43,   { 18 },  1 },    // Новомосковський район
    { 44,   { 18 },  1 },    // Дніпровський район
    { 47,   { 18 },  1 },    // Нікопольський район
    { 48,   { 18 },  1 },    // Синельниківський район
    { 42,   { 18 },  1 },    // Кам'янський район
    { 45,   { 18 },  1 },    // Павлоградський район
    { 46,   { 18 },  1 },    // Криворізький район
    { 332,  { 18 },  1 },    // м. Дніпро

    // Донецька область та її райони
    { 28,   { 12 }, 1 },    // Донецька область
    { 56,   { 12 }, 1 },    // Покровський район
    { 51,   { 12 }, 1 },    // Горлівський район
    { 55,   { 12 }, 1 },    // Волноваський район
    { 53,   { 12 }, 1 },    // Донецький район
    { 49,   { 12 }, 1 },    // Кальміуський район
    { 52,   { 12 }, 1 },    // Маріупольський район
    { 50,   { 12 }, 1 },    // Краматорський район
    { 54,   { 12 }, 1 },    // Бахмутський район

    // Житомирська область та її райони
    { 10,   { 6 }, 1 },    // Житомирська область
    { 59,   { 6 }, 1 },    // Житомирський район
    { 58,   { 6 }, 1 },    // Коростенський район
    { 57,   { 6 }, 1 },    // Бердичівський район
    { 60,   { 6 }, 1 },    // Звягельський район
    { 442,  { 6 }, 1 },    // м. Житомир

    // Закарпатська область та її райони
    { 11,   { 0 },  1 },    // Закарпатська область
    { 66,   { 0 },  1 },    // Ужгородський район
    { 61,   { 0 },  1 },    // Берегівський район
    { 62,   { 0 },  1 },    // Хустський район
    { 63,   { 0 },  1 },    // Рахівський район
    { 64,   { 0 },  1 },    // Тячівський район
    { 65,   { 0 },  1 },    // Мукачівський район
    { 500,  { 0 },  1 },    // м. Ужгород

    // Запорізька область та її райони
    { 12,   { 13 }, 1 },    // Запорізька область
    { 146,  { 13 }, 1 },    // Василівський район
    { 145,  { 13 }, 1 },    // Пологівський район
    { 149,  { 13 }, 1 },    // Запорізький район
    { 147,  { 13 }, 1 },    // Бердянський район
    { 148,  { 13 }, 1 },    // Мелітопольський район
    //{ 564,  { 13 }, 1 },    // м. Запоріжжя

    // Івано-Франківська область та її райони
    { 13,   { 1 }, 1 },    // Івано-Франківська область
    { 68,   { 1 }, 1 },    // Івано-Франківський район
    { 67,   { 1 }, 1 },    // Верховинський район
    { 71,   { 1 }, 1 },    // Калуський район
    { 72,   { 1 }, 1 },    // Надвірнянський район
    { 70,   { 1 }, 1 },    // Коломийський район
    { 69,   { 1 }, 1 },    // Косівський район
    { 632,  { 1 }, 1 },    // м. Івано-Франківськ

    // Київська область та її райони
    { 14,   { 7 }, 1 },    // Київська область
    { 77,   { 7 }, 1 },    // Фастівський район
    { 73,   { 7 }, 1 },    // Білоцерківський район
    { 75,   { 7 }, 1 },    // Бучанський район
    { 76,   { 7 }, 1 },    // Обухівський район
    { 74,   { 7 }, 1 },    // Вишгородський район
    { 79,   { 7 }, 1 },    // Броварський район
    { 78,   { 7 }, 1 },    // Бориспільський район
    //{ 31,   { 25 }, 1 },    // м. Київ

    // Кіровоградська область та її райони
    { 15,   { 21 },  1 },    // Кіровоградська область
    { 81,   { 21 },  1 },    // Кропивницький район
    { 80,   { 21 },  1 },    // Олександрійський район
    { 82,   { 21 },  1 },    // Голованівський район
    { 83,   { 21 },  1 },    // Новоукраїнський район
    { 761,  { 21 },  1 },    // м. Кропивницький

    // Луганська область та її райони
    { 16,   { 11 }, 1 },    // Луганська область
    { 86,   { 11 }, 1 },    // Старобільський район
    { 85,   { 11 }, 1 },    // Сватівський район
    { 84,   { 11 }, 1 },    // Сєвєродонецький район
    { 87,   { 11 }, 1 },    // Щастинський район

    // Львівська область та її райони
    { 27,   { 3 }, 1 },    // Львівська область
    { 90,   { 3 }, 1 },    // Львівський район
    { 89,   { 3 }, 1 },    // Стрийський район
    { 88,   { 3 }, 1 },    // Самбірський район
    { 91,   { 3 }, 1 },    // Дрогобицький район
    { 92,   { 3 }, 1 },    // Червоноградський район
    { 94,   { 3 }, 1 },    // Золочівський район
    { 93,   { 3 }, 1 },    // Яворівський район
    { 845,  { 3 }, 1 },    // м. Львів

    // Миколаївська область та її райони
    { 17,   { 17 },  1 },    // Миколаївська область
    { 96,   { 17 },  1 },    // Баштанський район
    { 95,   { 17 },  1 },    // Вознесенський район
    { 97,   { 17 },  1 },    // Первомайський район
    { 98,   { 17 },  1 },    // Миколаївський район
    { 926,  { 17 },  1 },    // м. Миколаїв

    // Одеська область та її райони
    { 18,   { 16 },  1 },    // Одеська область
    { 105,  { 16 },  1 },    // Болградський район
    { 100,  { 16 },  1 },    // Березівський район
    { 104,  { 16 },  1 },    // Одеський район
    { 102,  { 16 },  1 },    // Білгород-Дністровський район
    { 103,  { 16 },  1 },    // Роздільнянський район
    { 101,  { 16 },  1 },    // Ізмаїльський район
    { 99,   { 16 },  1 },    // Подільський район
    { 964,  { 16 },  1 },    // м. Одеса

    // Полтавська область та її райони
    { 19,   { 19 },  1 },    // Полтавська область
    { 107,  { 19 },  1 },    // Кременчуцький район
    { 106,  { 19 },  1 },    // Лубенський район
    { 109,  { 19 },  1 },    // Полтавський район
    { 108,  { 19 },  1 },    // Миргородський район
    { 1060, { 19 },  1 },    // м. Полтава

    // Рівненська область та її райони
    { 5,    { 5 }, 1 },    // Рівненська область
    { 110,  { 5 }, 1 },    // Вараський район
    { 111,  { 5 }, 1 },    // Дубенський район
    { 112,  { 5 }, 1 },    // Рівненський район
    { 113,  { 5 }, 1 },    // Сарненський район
    { 1133, { 5 }, 1 },    // м. Рівне

    // Сумська область та її райони
    { 20,   { 9 }, 1 },    // Сумська область
    { 115,  { 9 }, 1 },    // Шосткинський район
    { 116,  { 9 }, 1 },    // Роменський район
    { 117,  { 9 }, 1 },    // Конотопський район
    { 114,  { 9 }, 1 },    // Сумський район
    { 118,  { 9 }, 1 },    // Охтирський район
    { 1187, { 9 }, 1 },    // м. Суми

    // Тернопільська область та її райони
    { 21,   { 2 }, 1 },    // Тернопільська область
    { 119,  { 2 }, 1 },    // Тернопільський район
    { 121,  { 2 }, 1 },    // Чортківський район
    { 120,  { 2 }, 1 },    // Кременецький район
    { 1241, { 2 }, 1 },    // м. Тернопіль

    // Харківська область та її райони
    { 22,   { 10 }, 1 },    // Харківська область
    { 124,  { 10 }, 1 },    // Харківський район
    { 123,  { 10 }, 1 },    // Куп'янський район
    { 122,  { 10 }, 1 },    // Чугуївський район
    { 126,  { 10 }, 1 },    // Богодухівський район
    { 127,  { 10 }, 1 },    // Красноградський район
    { 125,  { 10 }, 1 },    // Ізюмський район
    { 128,  { 10 }, 1 },    // Лозівський район
    //{1293,  { 10 }, 1 },    // м. Харків

    // Херсонська область та її райони
    { 23,   { 14 }, 1 },    // Херсонська область
    { 131,  { 14 }, 1 },    // Каховський район
    { 129,  { 14 }, 1 },    // Бериславський район
    { 130,  { 14 }, 1 },    // Скадовський район
    { 132,  { 14 }, 1 },    // Херсонський район
    { 133,  { 14 }, 1 },    // Генічеський район
    { 1370, { 14 }, 1 },    // м. Херсон

    // Хмельницька область та її райони
    { 3,    { 23 },  1 },    // Хмельницька область
    { 136,  { 23 },  1 },    // Шепетівський район
    { 134,  { 23 },  1 },    // Хмельницький район
    { 135,  { 23 },  1 },    // Кам'янець-Подільський район
    { 1400, { 23 },  1 },    // м. Хмельницький

    // Черкаська область та її райони
    { 24,   { 20 },  1 },    // Черкаська область
    { 153,  { 20 },  1 },    // Золотоніський район
    { 152,  { 20 },  1 },    // Черкаський район
    { 150,  { 20 },  1 },    // Звенигородський район
    { 151,  { 20 },  1 },    // Уманський район
    { 1473, { 20 },  1 },    // м. Черкаси

    // Чернівецька область та її райони
    { 26,   { 24 },  1 },    // Чернівецька область
    { 139,  { 24 },  1 },    // Дністровський район
    { 138,  { 24 },  1 },    // Вижницький район
    { 137,  { 24 },  1 },    // Чернівецький район
    { 1542, { 24 },  1 },    // м. Чернівці

    // Чернігівська область та її райони
    { 25,   { 8 }, 1 },    // Чернігівська область
    { 141,  { 8 }, 1 },    // Новгород-Сіверський район
    { 142,  { 8 }, 1 },    // Ніжинський район
    { 143,  { 8 }, 1 },    // Прилуцький район
    { 140,  { 8 }, 1 },    // Чернігівський район
    { 144,  { 8 }, 1 },    // Корюківський район
    { 1591, { 8 }, 1 },    // м. Чернігів
};