#pragma once
#include "JaamLogs.h"
#include <math.h>
#include <string>
#include <map>
#include <set>
#include <utility> 
#include <vector>

extern std::map<uint16_t, uint16_t> alertsMap;

extern size_t  lastUsedHeap;

struct Firmware {
    int major = 0;
    int minor = 0;
    int patch = 0;
    int betaBuild = 0;
    bool isBeta = false;
  };
  
static Firmware parseFirmwareVersion(const char* version) {

    Firmware firmware;

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

static void fillFwVersion(char* result, Firmware firmware) {
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
inline uint8_t findHighestBit16(uint16_t value) {
    if (value == 0) return 255; // Повертаємо 255 як індикатор відсутності бітів
    

    // Перевіряємо наявність біта 0 (повітряна тривога)
    if (!(value & (1 << 0))) {
        return 255; // Якщо біт 0 не встановлений, повертаємо 255
    }

    uint8_t position = 0;
    while (value >>= 1) {
        position++;
    }
    return position;
}

// Функція для пошуку найстаршого біту для конкретного LED
inline uint8_t findHighestBitForLed(int position) {
    auto regions = getRegionsForLed(position);
    if (regions.empty()) {
        LOG.printf("[LED] LED %d does not belong to any region\n", position);
        return 255; // LED не належить жодному регіону
    }

    uint8_t globalHighestBit = 0;
    bool foundAnyBit = false;
    uint16_t highestBitRegion = 0;

    // Проходимо по всіх регіонах для цього LED
    for (uint16_t regionId : regions) {
        auto it = alertsMap.find(regionId);
        if (it != alertsMap.end() && it->second > 0) {
            uint8_t currentHighestBit = findHighestBit16(it->second);
            
            if (currentHighestBit != 255 && (!foundAnyBit || currentHighestBit > globalHighestBit)) {
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
    
    return 255; // Немає активних бітів в жодному з регіонів
}