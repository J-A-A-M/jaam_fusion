#pragma once
#include "JaamLogs.h"
#include "JaamSettings.h"
#include <math.h>
#include <string>
#include <map>
#include <set>
#include <utility> 
#include <vector>
#include <ArduinoJson.h>
#include <WiFi.h>

// External variables declarations
extern uint32_t                         lastWebsocketConnectTime;
extern std::map<uint16_t, uint16_t>     alertsMap;
extern size_t                           lastUsedHeap;
extern JaamSettings                     settings;
extern bool                             wifiConnected;
extern bool                             apiConnected;

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

// Отримати регіон по region_id
inline const RegionLedMapEntry* getRegionEntry(uint16_t region_id) {
    for (int i = 0; i < MAX_REGIONS; ++i) {
        if (REGION_MAP_LED[i].region_id == region_id) {
            return &REGION_MAP_LED[i];
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
    for (int i = 0; i < MAX_REGIONS; ++i) {
        const RegionLedMapEntry& entry = REGION_MAP_LED[i];
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

// Функція для пошуку номеру найстаршого біту в 16-бітному числі
inline int findHighestBit16(uint16_t value, bool checkBit0 = true) {
    if (value == 0) return -1; // Повертаємо -1 як індикатор відсутності бітів
    

    // Перевіряємо наявність біта 0 (повітряна тривога)
    if (!(value & (1 << 0)) && checkBit0) {
        return -1; // Якщо біт 0 не встановлений і потрібно перевіряти наявність біта 0, повертаємо -1
    }

    uint8_t position = 0;
    while (value >>= 1) {
        position++;
    }
    return position;
}

// Функція для пошуку найстаршого біту для конкретного LED
inline int findHighestBitForLed(int position) {
    auto regions = getRegionsForLed(position);
    if (regions.empty()) {
        LOG.printf("[LED] LED %d does not belong to any region\n", position);
        return -1; // LED не належить жодному регіону
    }

    uint8_t globalHighestBit = 0;
    uint16_t highestBitRegion = 0;
    bool foundAnyBit = false;

    // Проходимо по всіх регіонах для цього LED
    for (uint16_t regionId : regions) {
        auto it = alertsMap.find(regionId);
        if (it != alertsMap.end() && it->second > 0) {
            int currentHighestBit = findHighestBit16(it->second);
            
            if (currentHighestBit != -1 && (!foundAnyBit || currentHighestBit > globalHighestBit)) {
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
        LOG.printf("[LED] LED %d: highest bit %d ([%d] %s)\n", 
                  position, globalHighestBit, highestBitRegion, regionName);
        return globalHighestBit;
    }
    
    return -1; // Немає активних бітів в жодному з регіонів
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
            LOG.printf("[REGION] region_id=%d, Bit=%d\n", it->first, bit);
            if (bit != -1 && (maxBit == -1 || bit > maxBit)) {
                maxBit = bit;
            }
        }
    }
    LOG.printf("[REGION] region_id=%d, highestBit=%d\n", region_id, maxBit);
    return maxBit;
}

// Перевіряє, чи входить led_position у леди домашнього регіону
inline bool isLedInHomeRegion(int led_position) {
    LOG.printf("[HOME REGION] check led %d\n", led_position);
    // Отримуємо масив LED-ів для домашнього регіону
    uint8_t ledCount = 0;
    const int* leds = getLedsForRegion(settings.getInt(HOME_DISTRICT), ledCount);

    // Якщо для регіону немає LED-ів — повертаємо false
    if (leds == nullptr || ledCount == 0) {
        return false;
    }

    LOG.printf("[HOME REGION] leds: ");
    for (uint8_t i = 0; i < ledCount; ++i) {
        LOG.printf("%d ", leds[i]);
    }
    LOG.printf("\n");

    // Перевіряємо, чи входить led_position у масив
    for (uint8_t i = 0; i < ledCount; ++i) {
        if (leds[i] == led_position) {
            LOG.printf("[HOME REGION] led_position=%d\n", led_position);
            return true;
        } else {
            LOG.printf("[HOME REGION] led_position=%d not found in home region\n", led_position);
        }
    }
    return false;
}

inline int getHighestActualBit(int sourceBit) {
    int actualBit = -1;
    
    // Перебираємо біти від найвищого до найнижчого
    for (int bit = sourceBit; bit >= 0; bit--) {
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
        }

        // Якщо тип тривоги дозволено показувати - встановлюємо колір
        if (is_enabled) {
            if (bit == 0) {
                actualBit = 0;
            } else if (bit == 5) {
                actualBit = 5;
            } else if (bit == 6) {
                actualBit = 6;
            } else if (bit == 7) {
                actualBit = 7;
            } else if (bit == 8) {
                actualBit = 8;
            } else if (bit == 9) {
                actualBit = 9;
            }
            break; // Зупиняємося на першому дозволеному біті
        }
    }
    return actualBit;
}

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
    LOG.printf("[LED] getServicePinColor %d\n", type);
    uint32_t color = 0;
    switch (type) {
        case POWER:
            color = DefaultColors::POWER;
            break;
        case WIFI:
            LOG.printf("[LED] wifiConnected %d\n", wifiConnected);
            color = wifiConnected ? DefaultColors::WIFI : DefaultColors::OFF;
            break;
        case DATA:
            LOG.printf("[LED] apiConnected %d\n", apiConnected);
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
    float cpuTemp = temperatureRead();
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
    
    // Create JSON response
    JsonDocument doc;
    doc["freeHeap"] = freeHeap;
    doc["totalHeap"] = totalHeap;
    doc["usedHeap"] = usedHeap;
    doc["maxBlock"] = maxBlock;
    doc["cpuTemp"] = cpuTemp;
    doc["uptime"] = uptime;
    doc["memoryUsagePercent"] = (float)usedHeap / totalHeap * 100.0;
    doc["fragmentationPercent"] = (1.0f - ((float)maxBlock / (float)freeHeap)) * 100.0;
    doc["wifiRSSI"] = wifiRSSI;
    doc["websocketUptime"] = websocketUptime;
    
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