#pragma once
#include "JaamLogs.h"
#include "JaamSettings.h"
#include "JaamBattery.h"
#include "JaamStorage.h"
#include "JaamDisplay.h"
#include "JaamClimateSensor.h"
#include <math.h>
#include <string>
#include <map>
#include <set>
#include <utility> 
#include <vector>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <SPIFFS.h>
static const char* CUSTOM_MAP_PATH = "/custom_map.json";

// Масив з описом типів тривог
static const char* ALERT_TYPES[] = {
    "Air",      // bit 0
    "Artillery", // bit 1
    "Urban",    // bit 2
    "Chemical", // bit 3
    "Nuclear",  // bit 4
    "Drones",   // bit 5
    "Missiles", // bit 6
    "KAB",      // bit 7
    "Ballistic",// bit 8
    "Explosion", // bit 9
    "Recon Drones" // bit 10
};
static const int ALERT_TYPES_COUNT = sizeof(ALERT_TYPES) / sizeof(ALERT_TYPES[0]);

// Alert priority order - from highest to lowest priority
static const int ALERT_PRIORITY_ORDER[] = {9, 8, 7, 6, 5, 10, 0};
static const int ALERT_PRIORITY_COUNT = sizeof(ALERT_PRIORITY_ORDER) / sizeof(ALERT_PRIORITY_ORDER[0]);

// SVG Icons for system metrics (stored in PROGMEM to save RAM)
static const char ICON_MEMORY[] PROGMEM = "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M5 3C3.89543 3 3 3.89543 3 5H1V7H3V9H1V11H3V13H1V15H3V17H1V19H3C3 20.1046 3.89543 21 5 21H9C10.1046 21 11 20.1046 11 19H13C13 20.1046 13.8954 21 15 21H19C20.1046 21 21 20.1046 21 19H23V17H21V15H23V13H21V11H23V9H21V7H23V5H21C21 3.89543 20.1046 3 19 3H15C13.8954 3 13 3.89543 13 5H11C11 3.89543 10.1046 3 9 3H5ZM11 7V9H13V7H11ZM11 11V13H13V11H11ZM11 15V17H13V15H11ZM5 5H9V19H5V5ZM15 5H19V19H15V5Z'/></svg>";
static const char ICON_CPU[] PROGMEM = "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M7,2V4H6V6H4V7H2V9H4V11H2V13H4V15H2V17H4V18H6V20H7V22H9V20H11V22H13V20H15V22H17V20H18V18H20V17H22V15H20V13H22V11H20V9H22V7H20V6H18V4H17V2H15V4H13V2H11V4H9V2M8,6H16V18H8V6M10,8V16H14V8H10Z'/></svg>";
static const char ICON_CLOCK[] PROGMEM = "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M12,2A10,10 0 0,1 22,12A10,10 0 0,1 12,22A10,10 0 0,1 2,12A10,10 0 0,1 12,2M12,4A8,8 0 0,0 4,12A8,8 0 0,0 12,20A8,8 0 0,0 20,12A8,8 0 0,0 12,4M12.5,7V12.25L17,14.92L16.25,16.15L11,13V7H12.5Z'/></svg>";
static const char ICON_WIFI[] PROGMEM = "<svg class='metric-icon' viewBox='-1.5 0 19 19'><path d='M14.897 7.404a.553.553 0 0 1-.392-.163 9.192 9.192 0 0 0-13.01 0 .554.554 0 1 1-.784-.783 10.3 10.3 0 0 1 14.578 0 .554.554 0 0 1-.392.946zm-2.172 2.172a.553.553 0 0 1-.392-.162 6.127 6.127 0 0 0-8.666 0 .554.554 0 0 1-.784-.784 7.23 7.23 0 0 1 10.233 0 .554.554 0 0 1-.391.946zm-2.173 2.173a.553.553 0 0 1-.392-.162 3.054 3.054 0 0 0-4.32 0 .554.554 0 1 1-.784-.784 4.163 4.163 0 0 1 5.888 0 .554.554 0 0 1-.392.946zm-1.141 2.048a1.403 1.403 0 1 1-1.403-1.403 1.403 1.403 0 0 1 1.403 1.403z'/></svg>";
static const char ICON_WS[] PROGMEM = "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M12,2a7.71,7.71,0,0,0-1,15.37v.77h-1a1,1,0,0,0-1,1H2.35V21H9.11a1,1,0,0,0,1,1h3.86a1,1,0,0,0,1-1h6.76V19.11H14.89a1,1,0,0,0-1-1H13v-.77A7.71,7.71,0,0,0,12,2m0,1.67a15.43,15.43,0,0,1,1.21,2.9H10.81A15.83,15.83,0,0,1,12,3.67m-2.15.42a14,14,0,0,0-1,2.48H7A5.78,5.78,0,0,1,9.88,4.09m4.3,0A5.73,5.73,0,0,1,17,6.57H15.17a13,13,0,0,0-1-2.47m-8,4.4H8.48a7.48,7.48,0,0,0-.07,1,7.77,7.77,0,0,0,.07,1H6.33a5.23,5.23,0,0,1-.09-1,5,5,0,0,1,.09-1m3.74,0h3.58a7.48,7.48,0,0,1,.07,1,7.77,7.77,0,0,1-.07,1H10.41a7.77,7.77,0,0,1-.07-1,7.48,7.48,0,0,1,.07-1m5.45,0h1.87a5,5,0,0,1,.09,1,5.23,5.23,0,0,1-.09,1H15.58a7.77,7.77,0,0,0,.07-1,7.48,7.48,0,0,0-.07-1m-8.4,3.85h1.7a13.53,13.53,0,0,0,1,2.47A5.76,5.76,0,0,1,7,12.35m4,0h2.2A15.43,15.43,0,0,1,12,15.25a15.83,15.83,0,0,1-1.22-2.9m4.36,0H17a5.75,5.75,0,0,1-2.85,2.48A13.41,13.41,0,0,0,15.17,12.35Z'/></svg>";
static const char ICON_BATTERY[] PROGMEM = "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M16.67,4H15V2H9V4H7.33A1.33,1.33 0 0,0 6,5.33V20.67C6,21.4 6.6,22 7.33,22H16.67A1.33,1.33 0 0,0 18,20.67V5.33C18,4.6 17.4,4 16.67,4Z' /></svg>";
static const char ICON_TEMPERATURE[] PROGMEM = "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M15,13V5A3,3 0 0,0 9,5V13A5,5 0 1,0 15,13M12,4A1,1 0 0,1 13,5V8H11V5A1,1 0 0,1 12,4Z' /></svg>";
static const char ICON_HUMIDITY[] PROGMEM = "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M12,3.25C12,3.25 6,10 6,14C6,17.32 8.69,20 12,20A6,6 0 0,0 18,14C18,10 12,3.25 12,3.25Z' /></svg>";
static const char ICON_PRESSURE[] PROGMEM = "<svg class='metric-icon' viewBox='0 0 24 24'><path d='M6,14A1,1 0 0,1 7,13A1,1 0 0,1 8,14A5,5 0 0,0 13,19A1,1 0 0,1 12,20A1,1 0 0,1 11,19A7,7 0 0,1 4,12A1,1 0 0,1 5,11A1,1 0 0,1 6,12M17,10A2,2 0 0,1 19,12A7,7 0 0,1 12,19A2,2 0 0,1 10,17A4,4 0 0,0 14,13A2,2 0 0,1 12,11A2,2 0 0,1 14,9A4,4 0 0,0 10,5A2,2 0 0,1 12,3A7,7 0 0,1 19,10A2,2 0 0,1 17,12A2,2 0 0,1 15,10H17Z' /></svg>";

// External variables declarations
extern time_t                         lastWebsocketConnectTime;
extern time_t                         lastWifiConnectTime;
extern std::map<uint16_t, uint16_t>     alertsMap;
extern std::map<uint16_t, uint8_t>      temperatureMap; // weather: region -> temperature (int8 encoded)
extern JaamSettings                     settings;
extern JaamBattery                      battery;
extern JaamStorage                      storage;
extern JaamDisplay                      display;
extern bool                             wifiConnected;
extern bool                             apiConnected;
extern uint8_t                          legacy;
extern RegionLedMapEntry                customMap[MAX_REGIONS];
extern uint32_t                         bgLedColors[MAX_BG_LEDS];
extern JaamClimateSensor                climate;
extern int                              prevMapMode;

extern volatile bool                    needAdaptColors;

struct JaamFirmware {
    int major = 0;
    int minor = 0;
    int patch = 0;
    int betaBuild = 0;
    bool isBeta = false;
  };
struct LedBit {
    int highest_bit;
    uint16_t region_id;
};

enum ServiceLed {
    POWER,
    WIFI,
    DATA,
    HA,
    UPD_AVAILABLE
};

// Add memory monitoring function
inline void logMemoryUsage(const char* context) {
    size_t freeHeap = ESP.getFreeHeap();
    size_t totalHeap = ESP.getHeapSize();
    size_t usedHeap = totalHeap - freeHeap;
    size_t maxBlock = ESP.getMaxAllocHeap();
    
    LOG.printf("[MEMORY] %s - Used: %u bytes, Free: %u bytes, Total: %u bytes, Max block: %u bytes\n", 
              context, usedHeap, freeHeap, totalHeap, maxBlock);
    
    // Warning if free heap is getting low
    if (freeHeap < 10000) {
        LOG.printf("[MEMORY] WARNING: Low memory! Only %u bytes free\n", freeHeap);
    }
    
    // Warning if fragmentation is high
    // if (maxBlock < freeHeap * 0.7) {
    //     LOG.printf("[MEMORY] WARNING: High fragmentation! Max block: %u, Free: %u\n", maxBlock, freeHeap);
    // }
}
  
static JaamFirmware parseFirmwareVersion(const char* version) {

    JaamFirmware firmware;

    char* versionCopy = strdup(version);
    char* token = strtok(versionCopy, ".-");
    int part = 0;
    while (token) {
        if (isdigit(token[0])) {
        if (part == 0)
            firmware.major = atoi(token);
        else if (part == 1)
            firmware.minor = atoi(token);
        else if (part == 2)
            firmware.patch = atoi(token);
        part++;
        } else if (token[0] == 'b' && strcmp(token, "bin") != 0) {
        firmware.isBeta = true;
        firmware.betaBuild = atoi(token + 1); // Skip the 'b' character
        }
        token = strtok(NULL, ".-");
    }
    free(versionCopy);
    return firmware;
}

static void fillFwVersion(char* result, JaamFirmware firmware) {
    std::string version = std::to_string(firmware.major) + "." + std::to_string(firmware.minor);
  
    if (firmware.patch > 0) {
      version += "." + std::to_string(firmware.patch);
    }
  
    if (firmware.isBeta) {
      version += "-b" + std::to_string(firmware.betaBuild);
    }
    #if ARDUINO_ESP32S3_DEV
    version += "-s3";
    #elif ARDUINO_ESP32C3_DEV
    version += "-c3";
    #endif

    #if LITE
    version += "-lite";
    #endif

    #if TEST_MODE
    version += "-test";
    #endif

    #if FUSION
    version += "-fusion";
    #endif

    strcpy(result, version.c_str());
}

static float roundToDecimal(float value, int decimals) {
    if (decimals < 0) decimals = 0;
    if (decimals > 3) decimals = 3; 
    static const float mults[] = {1.0f, 10.0f, 100.0f, 1000.0f};
    float m = mults[decimals];
    return roundf(value * m) / m;
}

// Генерація customMap (викликається окремо при зміні налаштувань)
inline void generateCustomRegionMap() {
    // Очистити customMap перед копіюванням нового mapping
    memset(customMap, 0, sizeof(customMap));

    const RegionLedMapEntry* base = nullptr;
    int legacy = settings.getInt(LEGACY);
    int kyiv_led_position;
    // int kharkiv_led_position;
    // int zp_led_position;

    if (legacy == 5) {
        if (!storage.loadCustomMap(customMap)) {
            // Якщо файл не знайдено, завантажити карту за замовчуванням
            base = STATE_MAP_LED_ODESA_WITH_KYIV;
            memcpy(customMap, base, sizeof(customMap));
        }
        return;
    }

    if (legacy == 4) {
        base = REGION_MAP_LED;
    } else if (legacy == 0 || legacy == 3) {
        // kharkiv_led_position = 20;              // Позиція для Харківської області
        // zp_led_position = 23;                   // Позиція для Запорізької області
        base = STATE_MAP_LED_ODESA_WITH_KYIV;
    } else if (legacy == 2 && settings.getBool(KYIV_LED)) {
        // kharkiv_led_position = 20;              // Позиція для Харківської області
        // zp_led_position = 23;                   // Позиція для Запорізької області
        base = STATE_MAP_LED_ODESA_WITH_KYIV;
    } else if (legacy == 2) {
        // kharkiv_led_position = 19;              // Позиція для Харківської області
        // zp_led_position = 22;                   // Позиція для Запорізької області
        kyiv_led_position = 16;                 // Позиція для Київської області
        base = STATE_MAP_LED_ODESA_WITHOUT_KYIV;
    } else if (legacy == 1 && settings.getBool(KYIV_LED)) {
        // kharkiv_led_position = 11;              // Позиція для Харківської області
        // zp_led_position = 14;                   // Позиція для Запорізької області
        base = STATE_MAP_LED_TRANSCARPATHIA_WITH_KYIV;
    } else if (legacy == 1) {
        // kharkiv_led_position = 10;              // Позиція для Харківської області
        // zp_led_position = 13;                   // Позиція для Запорізької області
        kyiv_led_position = 7;                  // Позиція для Київської області
        base = STATE_MAP_LED_TRANSCARPATHIA_WITHOUT_KYIV;
    }

    if (!base) return;

    memcpy(customMap, base, sizeof(customMap));

    // Додатковий кастом для Києва
    // if ((legacy == 1 || legacy == 2) && !settings.getBool(KYIV_LED) && settings.getInt(DISTRICT_MODE_KYIV) > 0) {
    //     const std::set<uint16_t> regionSet = {14, 77, 73, 75, 76, 74, 79, 78}; // Київська область та райони
    //     for (int i = 0; i < MAX_REGIONS; ++i) {
    //         if (customMap[i].region_id == 31) {
    //             customMap[i].led_positions[0] = kyiv_led_position;
    //             customMap[i].led_count = 1;
    //             break;
    //         }
    //         if (regionSet.count(customMap[i].region_id) && settings.getInt(DISTRICT_MODE_KYIV) == 1) {
    //             customMap[i].led_count = 0;
    //         }
    //     }
    // }
    // if (settings.getInt(DISTRICT_MODE_KHARKIV) > 0) {
    //     const std::set<uint16_t> regionSet = {22, 124, 123, 122, 126, 127, 125, 128}; // Харківська область та райони
    //     for (int i = 0; i < MAX_REGIONS; ++i) {
    //         if (customMap[i].region_id == 1293) { // Харківська область
    //             customMap[i].led_positions[0] = kharkiv_led_position;
    //             customMap[i].led_count = 1;
    //             break;      
    //         }
    //         if (regionSet.count(customMap[i].region_id) && settings.getInt(DISTRICT_MODE_KHARKIV) == 1) {
    //             customMap[i].led_count = 0;
    //         }
    //     }
    // }
    // if (settings.getInt(DISTRICT_MODE_ZP) > 0) {
    //     const std::set<uint16_t> regionSet = {12, 146, 145, 149, 147, 148}; // Запорізька область та райони
    //     for (int i = 0; i < MAX_REGIONS; ++i) {
    //         if (customMap[i].region_id == 564) { // Запорізька область
    //             customMap[i].led_positions[0] = zp_led_position;
    //             customMap[i].led_count = 1;
    //             break;      
    //         }
    //         if (regionSet.count(customMap[i].region_id) && settings.getInt(DISTRICT_MODE_ZP) == 1) {
    //             customMap[i].led_count = 0;
    //         }
    //     }
    // }
}

// Генерація bgLedColors (викликається при ініціалізації)
inline void generateBgLedColorsMap() {
    // Очистити масив кольорів
    memset(bgLedColors, 0, sizeof(bgLedColors));
    
    int bgLedCount = settings.getInt(BG_LED_COUNT);
    if (bgLedCount <= 0 || bgLedCount > MAX_BG_LEDS) {
        LOG.printf("[INIT] BG LED count not configured or invalid.\n");
        return;
    }
    
    // Спробувати завантажити кольори з файлу
    uint32_t* tempColors = new uint32_t[bgLedCount];
    int actualCount = 0;
    
    if (storage.loadBgLedColors(tempColors, bgLedCount, actualCount)) {
        // Копіюємо завантажені кольори
        for (int i = 0; i < bgLedCount; ++i) {
            if (i < actualCount) {
                bgLedColors[i] = tempColors[i];
            } else {
                bgLedColors[i] = 0x000000; // Чорний за замовчуванням
            }
        }
        LOG.printf("[INIT] Loaded %d BG LED colors from file.\n", actualCount);
    } else {
        // Встановити всі кольори чорними за замовчуванням
        for (int i = 0; i < bgLedCount; ++i) {
            bgLedColors[i] = 0x000000;
        }
        LOG.printf("[INIT] Set %d BG LED colors to default black.\n", bgLedCount);
    }
    
    delete[] tempColors;
}

// Отримати колір для конкретного LED
inline uint32_t getBgLedColor(int ledIndex) {
    int bgLedCount = settings.getInt(BG_LED_COUNT);
    if (ledIndex >= 0 && ledIndex < bgLedCount && ledIndex < MAX_BG_LEDS) {
        return bgLedColors[ledIndex];
    }
    return 0x000000; // Чорний за замовчуванням для неіснуючих LED
}

// Повертає потрібний mapping (customMap або стандартний)
inline const RegionLedMapEntry* getRegionEntryLegacy(uint8_t legacy) {
    return customMap; // Повертаємо customMap, який був заповнений раніше
}

//inline const RegionLedMapEntry* getRegionEntryLegacy(uint8_t legacy) {
//     if (legacy == 4) {
//         return REGION_MAP_LED;
//     }
//     if (legacy == 0 || legacy == 3) {
//         return STATE_MAP_LED_ODESA_WITH_KYIV;
//     }
//     if (legacy == 2 && settings.getBool(KYIV_LED)){
//         return STATE_MAP_LED_ODESA_WITH_KYIV;
//     }
//     if (legacy == 1 && settings.getBool(KYIV_LED)){
//         return STATE_MAP_LED_TRANSCARPATHIA_WITH_KYIV;
//     }

//     const RegionLedMapEntry* base = nullptr;
//     int kyiv_led_position;
//     if (legacy == 1){
//         base = STATE_MAP_LED_TRANSCARPATHIA_WITHOUT_KYIV;
//         kyiv_led_position = 7;
//     } 
//     if (legacy == 2){
//         base = STATE_MAP_LED_ODESA_WITHOUT_KYIV;
//         kyiv_led_position = 16;
//     } 
    
//     static RegionLedMapEntry customMap[MAX_REGIONS];
//     memcpy(customMap, base, sizeof(customMap));
    

//     for (int i = 0; i < MAX_REGIONS; ++i) {
//         if (settings.getInt(DISTRICT_MODE_KYIV) == 1) {
//             if (customMap[i].region_id == 31 && settings.getInt(DISTRICT_MODE_KYIV) == 1) {
//                 customMap[i].led_positions[0] = kyiv_led_position;
//                 break;
//             }
//         }
//     }  
//     return customMap;
// }

// Отримати регіон по region_id
inline const RegionLedMapEntry* getRegionEntry(uint16_t region_id) {
    const RegionLedMapEntry* leds = getRegionEntryLegacy(settings.getInt(LEGACY));
    for (int i = 0; i < MAX_REGIONS; ++i) {
        if (leds[i].region_id == region_id) {
            return &leds[i];
        }
    }
    return nullptr;
}

// Отримати список LED для region_id
inline const int* getLedsForRegion(uint16_t region_id, uint8_t& count) {
    const RegionLedMapEntry* entry = getRegionEntry(region_id);
    if (entry) {
        count = entry->led_count;
        return entry->led_positions;
    }
    count = 0;
    return nullptr;
}

// Пошук усіх region_id по led_position
inline std::vector<uint16_t> getRegionsForLed(int led_position) {
    std::vector<uint16_t> regions;
    const RegionLedMapEntry* leds = getRegionEntryLegacy(settings.getInt(LEGACY));
    for (int i = 0; i < MAX_REGIONS; ++i) {
        const RegionLedMapEntry& entry = leds[i];
        for (int j = 0; j < entry.led_count; ++j) {
            if (entry.led_positions[j] == led_position) {
                regions.push_back(entry.region_id);
            }
        }
    }
    return regions;
}

// Перевірка чи є тривога на леді:
inline bool isAlertForLed(int led_position) {
    std::vector<uint16_t> regions = getRegionsForLed(led_position);
    for (uint16_t region_id : regions) {
        auto it = alertsMap.find(region_id);
        if (it != alertsMap.end() && it->second != 0) {
            LOG.printf("[REGION] Region %d led_position %d true\n", region_id, led_position);
            return true;            
        }
    }
    LOG.printf("[REGION] Region led_position %d false\n", led_position);
    return false;
}

// перевірка вільної пам'яті і порівняння з останнєю перевіркою
inline void checkFreeHeap(const char* label) {
    static size_t lastUsedHeap = 0; 
    size_t usedHeap = ESP.getHeapSize() - ESP.getFreeHeap();
    int heapDiff = lastUsedHeap > 0 ? (int)usedHeap - (int)lastUsedHeap : 0;
    
    if (lastUsedHeap > 0) {
        LOG.printf("[MEMORY] Used heap after %s: %u bytes (change: %+d bytes)\n", 
                  label, usedHeap, heapDiff);
    } else {
        LOG.printf("[MEMORY] Used heap after %s: %u bytes\n", label, usedHeap);
    }
    
    lastUsedHeap = usedHeap;
}

// Функція для пошуку номеру найстаршого дозволеного біту в 16-бітному числі
inline int findHighestBit16(uint16_t value, bool checkBit0 = true) {
    if (value == 0) return -1; // Повертаємо -1 як індикатор відсутності бітів
    

    // Перевіряємо наявність біта 0 (повітряна тривога)
    if (!(value & (1 << 0)) && checkBit0) {
        return -1; // Якщо біт 0 не встановлений і потрібно перевіряти наявність біта 0, повертаємо -1
    }

    // Пошук найвищого дозволеного біту за пріоритетом
    for (int i = 0; i < ALERT_PRIORITY_COUNT; ++i) {
        int bit = ALERT_PRIORITY_ORDER[i];
        if (value & (1 << bit)) {
            // Перевіряємо чи дозволено показувати цей тип тривоги
            bool is_enabled = false;
            
            if (bit == 0) {
                is_enabled = true; // Alert завжди показуємо
            } else if (bit == 5) {
                is_enabled = settings.getBool(ENABLE_DRONES);
            } else if (bit == 6) {
                is_enabled = settings.getBool(ENABLE_MISSILES);
            } else if (bit == 7) {
                is_enabled = settings.getBool(ENABLE_KABS);
            } else if (bit == 8) {
                is_enabled = settings.getBool(ENABLE_BALLISTIC);
            } else if (bit == 9) {
                is_enabled = settings.getBool(ENABLE_EXPLOSIONS);
            } else if (bit == 10) {
                is_enabled = settings.getBool(ENABLE_RECON_DRONES);
            }
            
            // Якщо тип тривоги дозволено показувати - повертаємо його
            if (is_enabled) {
                return bit;
            }
        }
    }
    
    return -1; // Жоден з дозволених пріоритетних бітів не встановлений
}

// Функція для порівняння пріоритетів двох бітів (повертає true, якщо bit1 має вищий пріоритет за bit2)
inline bool hasHigherPriority(int bit1, int bit2) {
    if (bit1 == -1) return false;
    if (bit2 == -1) return true;
    if (bit1 == bit2) return true;
    
    // Знаходимо індекси в масиві пріоритетів
    int index1 = -1, index2 = -1;
    for (int i = 0; i < ALERT_PRIORITY_COUNT; ++i) {
        if (ALERT_PRIORITY_ORDER[i] == bit1) index1 = i;
        if (ALERT_PRIORITY_ORDER[i] == bit2) index2 = i;
    }
    
    // Менший індекс означає вищий пріоритет
    return index1 != -1 && index2 != -1 && index1 < index2;
}

// Функція для пошуку найстаршого біту для конкретного LED
inline int findHighestBitForLed(int position) {
    auto regions = getRegionsForLed(position);
    if (regions.empty()) {
        LOG.printf("[LED] LED %d does not belong to any region\n", position);
        return -1; // LED не належить жодному регіону
    }

    uint8_t globalHighestBit = -1;
    uint16_t highestBitRegion = 0;
    bool foundAnyBit = false;

    // Проходимо по всіх регіонах для цього LED
    for (uint16_t regionId : regions) {
        auto it = alertsMap.find(regionId);
        if (it != alertsMap.end() && it->second > 0) {
            int currentHighestBit = findHighestBit16(it->second);
            
            if (currentHighestBit != -1 && (!foundAnyBit || hasHigherPriority(currentHighestBit, globalHighestBit))) {
                globalHighestBit = currentHighestBit;
                highestBitRegion = regionId;
                foundAnyBit = true;
            }
        }
    }
    
    if (foundAnyBit) {
        // Знаходимо назву регіону за його ID
        const char* regionName = "Невідомий регіон";
        for (int i = 0; i < MAX_REGIONS; i++) {
            if (DISTRICTS[i].id == highestBitRegion) {
                regionName = DISTRICTS[i].name;
                break;
            }
        }
        // LOG.printf("[LED] LED %d: highest bit %d ([%d] %s)\n", 
        //           position, globalHighestBit, highestBitRegion, regionName);
        return globalHighestBit;
    }
    
    return -1; // Немає активних бітів в жодному з регіонів
}

// Повертає найвищий біт для конкретного регіону по прямому входженню до alertsMap
inline int findHighestBitForRegionDirect(uint16_t region_id) {
    auto it = alertsMap.find(region_id);
    if (it != alertsMap.end() && it->second != 0) {
        int bit = findHighestBit16(it->second);
        //LOG.printf("[REGION DIRECT] region_id=%d, highestBit=%d\n", region_id, bit);
        return bit;
    }
    //LOG.printf("[REGION DIRECT] No alert data for region %d\n", region_id);
    return -1;
}

// Повертає найвищий біт серед усіх регіонів, до яких належать леди region_id
inline int findHighestBitForRegion(uint16_t region_id) {
    uint8_t ledCount = 0;
    const int* leds = getLedsForRegion(region_id, ledCount);
    if (!leds || ledCount == 0) {
        LOG.printf("[REGION] No leds for region %d\n", region_id);
        return -1;
    }

    std::set<uint16_t> allRegions;
    // Збираємо всі регіони, до яких належать ці леди
    for (uint8_t i = 0; i < ledCount; ++i) {
        auto regions = getRegionsForLed(leds[i]);
        allRegions.insert(regions.begin(), regions.end());
    }

    // Шукаємо найвищий біт серед усіх регіонів
    int maxBit = -1;
    for (uint16_t reg : allRegions) {
        auto it = alertsMap.find(reg);
        if (it != alertsMap.end() && it->second != 0) {
            int bit = findHighestBit16(it->second);
            //LOG.printf("[REGION] region_id=%d, Bit=%d\n", it->first, bit);
            if (bit != -1 && (maxBit == -1 || bit > maxBit)) {
                maxBit = bit;
            }
        }
    }
    //LOG.printf("[REGION] region_id=%d, highestBit=%d\n", region_id, maxBit);
    return maxBit;
}

// Перевіряє, чи входить led_position у леди домашнього регіону
inline bool isLedInHomeRegion(int led_position) {
    LOG.printf("[HOME DISTRICT] check led %d. ", led_position);
    // Отримуємо масив LED-ів для домашнього регіону
    uint8_t ledCount = 0;
    const int* leds = getLedsForRegion(settings.getInt(HOME_DISTRICT), ledCount);

    // Якщо для регіону немає LED-ів — повертаємо false
    if (leds == nullptr || ledCount == 0) {
        return false;
    }

    LOG.printf("Leds:");
    for (uint8_t i = 0; i < ledCount; ++i) {
        LOG.printf(" %d", leds[i]);
    }
    LOG.printf(". ");

    // Перевіряємо, чи входить led_position у масив
    for (uint8_t i = 0; i < ledCount; ++i) {
        if (leds[i] == led_position) {
            LOG.printf("Led %d is in home district\n", led_position);
            return true;
        } else {
            LOG.printf("Led %d not found in home district\n", led_position);
        }
    }
    return false;
}

// Перевіряє, чи є region_id домашнім регіоном
inline bool isHomeRegion(int region_id) {
    return region_id == settings.getInt(HOME_DISTRICT);
}

inline int getHighestActualBit(int sourceBit) {
    int actualBit = -1;
    
    // Знаходимо позицію sourceBit в ALERT_PRIORITY_ORDER
    int sourceBitIndex = -1;
    for (int i = 0; i < ALERT_PRIORITY_COUNT; ++i) {
        if (ALERT_PRIORITY_ORDER[i] == sourceBit) {
            sourceBitIndex = i;
            break;
        }
    }
    
    // Якщо sourceBit не знайдено в пріоритетах, повертаємо -1
    if (sourceBitIndex == -1) {
        return -1;
    }
    
    // Перебираємо біти за порядком пріоритету, починаючи з sourceBit
    for (int i = sourceBitIndex; i < ALERT_PRIORITY_COUNT; ++i) {
        int bit = ALERT_PRIORITY_ORDER[i];
        
        bool is_enabled = false;
        
        // Перевіряємо чи дозволено показувати цей тип тривоги
        if (bit == 0) {
            is_enabled = true; // Alert завжди показуємо
        } else if (bit == 5) {
            is_enabled = settings.getBool(ENABLE_DRONES);
        } else if (bit == 6) {
            is_enabled = settings.getBool(ENABLE_MISSILES);
        } else if (bit == 7) {
            is_enabled = settings.getBool(ENABLE_KABS);
        } else if (bit == 8) {
            is_enabled = settings.getBool(ENABLE_BALLISTIC);
        } else if (bit == 9) {
            is_enabled = settings.getBool(ENABLE_EXPLOSIONS);
        } else if (bit == 10) {
            is_enabled = settings.getBool(ENABLE_RECON_DRONES);
        }

        // Якщо тип тривоги дозволено показувати - встановлюємо його і виходимо
        if (is_enabled) {
            actualBit = bit;
            break; // Зупиняємося на першому дозволеному біті за пріоритетом
        }
    }
    return actualBit;
}


// Add function to force memory cleanup
inline void forceMemoryCleanup(const char* context) {
    LOG.printf("[MEMORY] Forcing cleanup at: %s\n", context);
    size_t before = ESP.getFreeHeap();
    
    // Force yield to system
    yield();
    delay(50);
    
    // Check if memory was reclaimed
    size_t after = ESP.getFreeHeap();
    if (after > before) {
        LOG.printf("[MEMORY] Cleanup successful: %d bytes reclaimed\n", (int)(after - before));
    } else {
        LOG.printf("[MEMORY] Cleanup completed: no additional memory reclaimed\n");
    }
}

// Add function to defragment memory by forcing garbage collection
inline void defragmentMemory(const char* context) {
    LOG.printf("[MEMORY] Starting defragmentation at: %s\n", context);
    size_t beforeFree = ESP.getFreeHeap();
    size_t beforeMaxBlock = ESP.getMaxAllocHeap();
    
    // Multiple yields and delays to allow system cleanup
    for (int i = 0; i < 5; i++) {
        yield();
        delay(20);
    }
    
    // Try to trigger garbage collection by allocating and freeing small blocks
    void* tempPtrs[10];
    for (int i = 0; i < 10; i++) {
        tempPtrs[i] = malloc(100);
        if (tempPtrs[i]) {
            free(tempPtrs[i]);
            tempPtrs[i] = nullptr;
        }
        yield();
    }
    
    size_t afterFree = ESP.getFreeHeap();
    size_t afterMaxBlock = ESP.getMaxAllocHeap();
    
    LOG.printf("[MEMORY] Defrag result - Free: %u->%u (%+d), MaxBlock: %u->%u (%+d)\n", 
              beforeFree, afterFree, (int)(afterFree - beforeFree),
              beforeMaxBlock, afterMaxBlock, (int)(afterMaxBlock - beforeMaxBlock));
}

// Check if memory allocation is likely to succeed
inline bool canAllocateMemory(size_t size, const char* context) {
    size_t maxBlock = ESP.getMaxAllocHeap();
    bool canAlloc = maxBlock >= size;
    
    if (!canAlloc) {
        LOG.printf("[MEMORY] %s - Cannot allocate %u bytes, max block: %u\n", 
                  context, size, maxBlock);
        
        // Try defragmentation if allocation would fail
        defragmentMemory(context);
        
        // Check again after defragmentation
        maxBlock = ESP.getMaxAllocHeap();
        canAlloc = maxBlock >= size;
        
        if (canAlloc) {
            LOG.printf("[MEMORY] %s - After defrag can allocate %u bytes, max block: %u\n", 
                      context, size, maxBlock);
        } else {
            LOG.printf("[MEMORY] %s - Still cannot allocate %u bytes after defrag, max block: %u\n", 
                      context, size, maxBlock);
        }
    }
    
    return canAlloc;
}

// Enhanced memory logging with fragmentation analysis
inline void analyzeMemoryFragmentation(const char* context) {
    size_t freeHeap = ESP.getFreeHeap();
    size_t maxBlock = ESP.getMaxAllocHeap();
    
    if (freeHeap > 0) {
        float fragmentation = 1.0f - ((float)maxBlock / (float)freeHeap);
        int fragPercent = (int)(fragmentation * 100);
        
        LOG.printf("[MEMORY] %s - Fragmentation: %d%% (Free: %u, MaxBlock: %u)\n", 
                  context, fragPercent, freeHeap, maxBlock);
        
        if (fragmentation > 0.5) { // More than 50% fragmented
            LOG.printf("[MEMORY] %s - HIGH FRAGMENTATION DETECTED! Consider defragmentation\n", context);
            return; // Return true to indicate high fragmentation
        }
    }
}

// Function to get service pin color
inline uint32_t getServicePinColor(int type) {
    //LOG.printf("[LED] getServicePinColor %d\n", type);
    uint32_t color = 0;
    switch (type) {
        case POWER:
            color = DefaultColors::POWER;
            break;
        case WIFI:
            LOG.printf("[SERVICE LED] wifiConnected %d\n", wifiConnected);
            color = wifiConnected ? DefaultColors::WIFI : DefaultColors::OFF;
            break;
        case DATA:
            LOG.printf("[SERVICE LED] apiConnected %d\n", apiConnected);
            color = apiConnected ? DefaultColors::DATA : DefaultColors::OFF;
            break;
        case HA:
            color = DefaultColors::OFF;
            break;
        case UPD_AVAILABLE:
            color = DefaultColors::OFF;
            break;
    }
    return color;
}

// Function to parse JSON payload and return JsonDocument
static JsonDocument parseJson(const char* payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        LOG.printf("[ERROR] Deserialization error: $s\n", error.f_str());
        return doc;
    } else {
        return doc;
    }
}


// Function to get system information as JSON string for web interface
inline String getSystemInfoJson() {
    // Get system information
    size_t freeHeap = ESP.getFreeHeap();
    size_t totalHeap = ESP.getHeapSize();
    size_t usedHeap = totalHeap - freeHeap;
    size_t maxBlock = ESP.getMaxAllocHeap();
    float cpuTemp = roundToDecimal(temperatureRead(), 0);
    uint32_t uptime = millis() / 1000; // uptime in seconds
    
    // WiFi information
    int32_t wifiRSSI = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
    
    // Websocket information - access via extern variable
    bool websocketAvailable = false;
    uint32_t websocketUptime = 0;
    
    if (lastWebsocketConnectTime > 0) {
        websocketUptime = (millis() - lastWebsocketConnectTime) / 1000; // in seconds
        websocketAvailable = true;
    }
    
    // WiFi uptime metric (in seconds)
    uint32_t wifiUptime = wifiConnected ? (millis() - lastWifiConnectTime) / 1000 : 0;

    JsonDocument doc;

    // models describe compact list fields
    JsonObject models = doc["system_models"].to<JsonObject>();
    {
        JsonArray mBar = models["bar"].to<JsonArray>();
        // [key, label, unit, iconSvg, used, total]
        mBar.add("key"); mBar.add("label"); mBar.add("unit"); mBar.add("iconSvg"); mBar.add("used"); mBar.add("total");
        JsonArray mNum = models["number"].to<JsonArray>();
        // [key, label, unit, iconSvg, value]
        mNum.add("key"); mNum.add("label"); mNum.add("unit"); mNum.add("iconSvg"); mNum.add("value");
        JsonArray mTime = models["time"].to<JsonArray>();
        // [key, label, iconSvg, seconds]
        mTime.add("key"); mTime.add("label"); mTime.add("iconSvg"); mTime.add("seconds");
    }

    JsonArray system = doc["system"].to<JsonArray>();
    JsonArray item;
    // Memory bar (values in KB)
    {    
        item = system.add<JsonArray>();
        item.add("bar");
        item.add("memory");
        item.add("Пам'ять");
        item.add("KB");
        item.add(ICON_MEMORY);
        item.add((int)(usedHeap / 1024));
        item.add((int)(totalHeap / 1024));
    }

    // CPU temperature (number)
    {    
        item = system.add<JsonArray>();
        item.add("number");
        item.add("cpuTemp");
        item.add("Процесор");
        item.add("°C");
        item.add(ICON_CPU);
        item.add(cpuTemp);
    }

    // Uptime (time)
    {    
        item = system.add<JsonArray>();
        item.add("time");
        item.add("uptime");
        item.add("Час роботи");
        item.add(ICON_CLOCK);
        item.add(uptime);
    }

    // WiFi RSSI (number)
    {    
        item = system.add<JsonArray>();
        item.add("number");
        item.add("wifiSignal");
        item.add("WiFi");
        item.add("dBm");
        item.add(ICON_WIFI);
        item.add(wifiRSSI);
    }

    // WiFi uptime (time)
    {    
        item = system.add<JsonArray>();
        item.add("time");
        item.add("wifiUptime");
        item.add("WiFi uptime");
        item.add(ICON_CLOCK);
        item.add(wifiUptime);
    }

    // Websocket uptime (time)
    {    
        item = system.add<JsonArray>();
        item.add("time");
        item.add("websocketUptime");
        item.add("Websocket");
        item.add(ICON_WS);
        item.add(websocketUptime);
    }

    // Battery voltage if available (> 0)
    {    
        if (battery.isEnabled()) {
            item = system.add<JsonArray>();
            item.add("number");
            item.add("batteryVoltage");
            item.add("Батарея");
            item.add("V");
            item.add(ICON_BATTERY);
            item.add(roundToDecimal(battery.readVoltage(), 2));
        }
    }

    // Climate sensor data (only if any sensor is available)
    {    
        if (climate.isTemperatureAvailable()) {
            item = system.add<JsonArray>();
            item.add("number");
            item.add("localTemp");
            item.add("Температура");
            item.add("°C");
            item.add(ICON_TEMPERATURE);
            item.add(roundToDecimal(climate.getTemperature(settings.getFloat(TEMP_CORRECTION)), 0));
        }
    }
    {   
        if (climate.isHumidityAvailable()) {
            item = system.add<JsonArray>();
            item.add("number");
            item.add("localHumidity");
            item.add("Вологість");
            item.add("%");
            item.add(ICON_HUMIDITY);
            item.add(roundToDecimal(climate.getHumidity(settings.getFloat(HUM_CORRECTION)), 0));
        }
    }
    {
        if (climate.isPressureAvailable()) {
            item = system.add<JsonArray>();
            item.add("number");
            item.add("localPressure");
            item.add("Тиск");
            item.add("mmHg");
            item.add(ICON_PRESSURE);
            item.add(roundToDecimal(climate.getPressure(settings.getFloat(PRESSURE_CORRECTION)), 0));
        }
    }


    String response;
    serializeJson(doc, response);
    return response;
}

// Function to get alerts information as JSON string for web interface
inline String getAlertsJson() {
    JsonDocument doc;
    JsonArray regions = doc["regions"].to<JsonArray>();
    
    // Перебираємо всі регіони з alertsMap
    for (const auto& alertEntry : alertsMap) {
        uint16_t region_id = alertEntry.first;
        uint16_t flags16 = alertEntry.second;
        
        // Перевіряємо чи є активний перший біт (повітряна тривога)
        bool airAlert = flags16 & (1 << 0);
        if (!airAlert) {
            continue; // Пропускаємо регіони без активної повітряної тривоги
        }
        
        // Перевіряємо чи регіон є в DISTRICTS
        bool foundInDistricts = false;
        String regionName = "";
        
        for (int i = 0; i < MAX_REGIONS; i++) {
            if (DISTRICTS[i].id == region_id) {
                foundInDistricts = true;
                regionName = DISTRICTS[i].name;
                break;
            }
        }
        
        if (!foundInDistricts) {
            continue; // Пропускаємо регіони не знайдені в DISTRICTS
        }
        
        // Створюємо об'єкт регіону
        JsonObject region = regions.add<JsonObject>();
        region["regionId"] = region_id;
        region["regionName"] = regionName;
        
        // Розкладаємо статуси тривог по бітам
        JsonObject alerts = region["alerts"].to<JsonObject>();
        alerts["air"] = (flags16 & (1 << 0)) ? 1 : 0;
        alerts["artillery"] = (flags16 & (1 << 1)) ? 1 : 0;
        alerts["urban"] = (flags16 & (1 << 2)) ? 1 : 0;
        alerts["chemical"] = (flags16 & (1 << 3)) ? 1 : 0;
        alerts["nuclear"] = (flags16 & (1 << 4)) ? 1 : 0;
        alerts["drones"] = (flags16 & (1 << 5)) ? 1 : 0;
        alerts["missiles"] = (flags16 & (1 << 6)) ? 1 : 0;
        alerts["kab"] = (flags16 & (1 << 7)) ? 1 : 0;
        alerts["ballistic"] = (flags16 & (1 << 8)) ? 1 : 0;
        alerts["explosion"] = (flags16 & (1 << 9)) ? 1 : 0;
    }
    
    String response;
    serializeJson(doc, response);
    return response;
}

inline const char* getNameById(SettingListItem list[], int id, int size) {
  for (int i = 0; i < size; i++) {
    if (list[i].id == id) {
      return list[i].name;
    }
  }
  return "";
}

inline int getIndexById(SettingListItem list[], int id, int size) {
  for (int i = 0; i < size; i++) {
    if (list[i].id == id) {
      return i;
    }
  }
  return 0;
}

inline bool saveMapMode(int newMapMode) {
  if (newMapMode == settings.getInt(MAP_MODE)) return false;

  if (newMapMode == 5) {
    prevMapMode = settings.getInt(MAP_MODE);
  }
  settings.saveInt(MAP_MODE, newMapMode);
  needAdaptColors = true;
  //reportSettingsChange("map_mode", newMapMode);
  //ha.setLampState(newMapMode == 5);
  //ha.setMapMode(haMapModeMap.second[newMapMode]);
  const char* mapModeName = getNameById(MAP_MODES, newMapMode, MAP_MODES_COUNT);
  display.showServiceMessage(mapModeName, "Режим мапи:");
  //ha.setMapModeCurrent(mapModeName);
  //showServiceMessage(mapModeName, "Режим мапи:");
  // update to selected mapMode
  //mapCycle();
  return true;
}

inline void nextMapMode() {
  int newIndex = getIndexById(MAP_MODES, settings.getInt(MAP_MODE), MAP_MODES_COUNT);
  do {
    if (newIndex >= MAP_MODES_COUNT - 1) {
      newIndex = 0;
    } else {
      newIndex++;
    }
  } while (MAP_MODES[newIndex].ignore);

  saveMapMode(MAP_MODES[newIndex].id);
}

inline bool saveDisplayMode(int newDisplayMode) {
  if (newDisplayMode == settings.getInt(DISPLAY_MODE)) return false;
  settings.saveInt(DISPLAY_MODE, newDisplayMode);
  //reportSettingsChange("display_mode", newDisplayMode);
  //if (display.isDisplayAvailable()) {
  //  ha.setDisplayMode(haDisplayModeMap.second[newDisplayMode]);
  //}
  //showServiceMessage(getNameById(DISPLAY_MODES, newDisplayMode, DISPLAY_MODE_OPTIONS_MAX), "Режим дисплея:", 1000);
  // update to selected displayMode
  //displayCycle();
  return true;
}

inline bool saveDisplayModeFromHa(int newIndex) {
  return saveDisplayMode(DISPLAY_MODES[newIndex].id);
}

inline void nextDisplayMode() {
  int newIndex = getIndexById(DISPLAY_MODES, settings.getInt(DISPLAY_MODE), DISPLAY_MODES_COUNT);
  do {
    if (newIndex >= DISPLAY_MODES_COUNT - 1) {
      newIndex = 0;
    } else {
      newIndex++;
    }
  } while (DISPLAY_MODES[newIndex].ignore);

  saveDisplayMode(DISPLAY_MODES[newIndex].id);
}