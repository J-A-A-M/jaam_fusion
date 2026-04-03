#include "JaamConfig.h"

// --- Sound Settings ---
const SettingListItem SOUND_SOURCES[SOUND_SOURCES_COUNT] PROGMEM = {
  {-1, "Вимкнено", false},
  {0, "Buzzer", false},
  {1, "DF Player Pro", true, false, true}
};

// --- Other Settings ---
const SettingListItem LED_COLOR_FORMATS[LED_COLOR_FORMATS_COUNT] = {
    {NEO_RGB, "NEO_RGB"},
    {NEO_RBG, "NEO_RBG"},
    {NEO_GRB, "NEO_GRB (рекомендовано)"},
    {NEO_GBR, "NEO_GBR"},
    {NEO_BRG, "NEO_BRG"},
    {NEO_BGR, "NEO_BGR"},
    {NEO_WRGB, "NEO_WRGB"},
    {NEO_WGRB, "NEO_WGRB"}
};

const SettingListItem LED_FREQUENCIES[LED_FREQUENCIES_COUNT] = {
    {NEO_KHZ400, "400 КГц"},
    {NEO_KHZ800, "800 КГц (рекомендовано)"}
};

const SettingListItem SINGLE_CLICKS[SINGLE_CLICKS_COUNT] = {
  {0, "Вимкнено"},
  {1, "Перемикання режимів мапи"},
  {2, "Перемикання режимів дисплею"},
  {3, "Увімк./Вимк. мапу"},
  {4, "Увімк./Вимк. дисплей"},
  {5, "Увімк./Вимк. мапу та дисплей"},
  {7, "Увімк./Вимк. режим лампи", true, false, true},
};

const SettingListItem LONG_CLICKS[LONG_CLICKS_COUNT] = {
  {0, "Вимкнено"},
  {1, "Перемикання режимів мапи"},
  {2, "Перемикання режимів дисплею"},
  {3, "Увімк./Вимк. мапу"},
  {4, "Увімк./Вимк. дисплей"},
  {5, "Увімк./Вимк. мапу та дисплей"},
  {6, "Увімк./Вимк. режим нічної яскравості"},
  {8, "Збільшити яскравість лампи", true, false, true},
  {9, "Зменшити яскравість лампи", true, false, true},
  {10, "Перезавантаження пристрою"},
};

const SettingListItem MAP_MODES[MAP_MODES_COUNT] = {
  {0, "Вимкнено"},
  {1, "Тривога"},
  {6, "Енергосистема", true, false, true},
  {2, "Погода"},
  {7, "Радіація", true, false, true},
  {3, "Прапор"},
  {4, "Випадкові кольори"},
  {5, "Лампа"},
};

const SettingListItem ALERT_CLEAR_PIN_MODES[ALERT_CLEAR_PIN_MODES_COUNT] = {
  {0, "Бістабільний"},
  {1, "Імпульсний"},
};

const SettingListItem PIN_LEVELS[PIN_LEVELS_COUNT] = {
  {0, "LOW"},
  {1, "HIGH"},
};

const SettingListItem TIMEZONES[TIMEZONES_COUNT] = {
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

const TimezoneInfo TIMEZONE_OFFSETS[TIMEZONES_COUNT] = {
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

SettingListItem DISPLAY_MODES[DISPLAY_MODES_COUNT] = {
  {0, "Вимкнено", false},
  {1, "Годинник", false},
  {5, "Енергосистема", true, false, true},
  {2, "Погода", false},
  {6, "Радіація", true, false, true},
  {3, "Технічна інформація", false},
  {4, "Мікроклімат", false},
  {9, "Комбінований", false},
};

SettingListItem AUTO_BRIGHTNESS_MODES[AUTO_BRIGHTNESS_OPTIONS_COUNT] = {
    {0, "Вимкнено"},
    {1, "День/Ніч"},
    {2, "Сенсор освітлення"}
};

const SettingListItem BG_LED_MODES[BG_LED_MODES_COUNT] = {
  {0, "Домашній Регіон"},
  {1, "Власний колір"},
  {2, "Індивідуальні кольори"},
};

const SettingListItem DISPLAY_TYPES[DISPLAY_TYPES_COUNT] = {
  {0, "Відключено"},
  {1, "SSD1306"},
  {2, "SH1106G"},
  {3, "SH1107"},
};

const SettingListItem DISPLAY_HEIGHTS[DISPLAY_HEIGHT_COUNT] = {
  {32, "32"},
  {64, "64"},
};

const SettingListItem DISPLAY_ROTATIONS[DISPLAY_ROTATION_COUNT] = {
  {0, "0°"},
  {90, "90°"},
  {180, "180°"},
  {270, "270°"},
};

const SettingListItem CLOCK_FONTS[CLOCK_FONTS_COUNT] = {
  {0, "Reddit"},
  {1, "Victor"},
  {2, "M PLUS 1 CODE"},
  {3, "Old Standard"},
  {4, "DSEG7"},
  {5, "Bitcount Grid"},
};

const SettingListItem HARDWARE_OPTIONS[HARDWARE_OPTIONS_COUNT] = {
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

const SettingListItem ANIMATION_TYPES[ANIMATION_TYPES_COUNT] = {
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

// --- Melodies and Tracks ---
const char* MELODIES[MELODIES_COUNT] PROGMEM = {
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

const SettingListItem MELODY_NAMES[MELODIES_COUNT] PROGMEM = {
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

// --- DF Player Track Paths ---
const String DF_CLOCK_BEEP = "/01.mp3";
const String DF_CLOCK_TICK = "/02.mp3";
const String DF_UA_ANTHEM = "/03.mp3";
const String DF_SIREN_1 = "/04.mp3";
const String DF_SIREN_2 = "/05.mp3";
const String DF_SIREN_3 = "/06.mp3";
const String DF_SIREN_4 = "/07.mp3";
const String DF_SIREN_5 = "/08.mp3";
const String DF_SIREN_6 = "/09.mp3";
const String DF_SIREN_7 = "/10.mp3";
const String DF_SIREN_8 = "/11.mp3";
const String DF_SIREN_9 = "/12.mp3";
const String DF_SIREN_10 = "/13.mp3";
const String DF_THE_HOBBIT = "/14.mp3";
const String DF_THE_MATRIX = "/15.mp3";
const String DF_AVENGERS = "/16.mp3";
const String DF_TERMINATOR_SHORT = "/17.mp3";
const String DF_PIRATES_OF_THE_CARRIBEAN = "/18.mp3";
const String DF_SIREN_11 = "/19.mp3";
const String DF_NOTIFICATION_NEWS = "/20.mp3";
const String DF_GOOD_MORNING_VIETNAM = "/21.mp3";
const String DF_NOTIFICATION_R2D2 = "/22.mp3";
const String DF_NOTIFICATION_STARTREK = "/23.mp3";
const String DF_AIR_RAID_1 = "/24.mp3";
const String DF_CAROL_OF_THE_BELLS = "/25.mp3";
const String DF_NOTIFICATION_BACK_TO_THE_FUTURE = "/26.mp3";
const String DF_IMPERIAL_MARCH = "/27.mp3";
const String DF_GOOD_BAD_UGLY = "/28.mp3";
const String DF_HARRY_POTTER = "/29.mp3";
const String DF_MARCH = "/30.mp3";
const String DF_MANDALORIAN_CALL = "/31.mp3";
const String DF_MARIO = "/32.mp3";
const String DF_PACMAN = "/33.mp3";
const String DF_HELLDIVERS = "/34.mp3";

const String TRACKS[TRACKS_COUNT] = {
  DF_CLOCK_TICK,
  DF_CLOCK_BEEP,
  DF_UA_ANTHEM
};

const SettingListItem TRACK_NAMES[TRACKS_COUNT] PROGMEM = {
  {0, "Годинникова стрілка", false},
  {1, "Годинник", false},
  {2, "Гімн України", false}
};

// --- Administrative Units ---
const uint16_t ADMIN_UNITS[ADMIN_UNITS_COUNT] = {
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

const SettingListItem DISTRICTS[MAX_REGIONS] = {
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
    {564, "м. Запоріжжя та Запорізька територіальна громада", false, false},
    
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
