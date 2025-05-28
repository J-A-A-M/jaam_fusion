#pragma once
#include "JaamLogs.h"
#include <math.h>
#include <string>
#include <map>
#include <set>
#include <utility> 
#include <vector>

extern std::map<uint16_t, uint16_t> alertsMap;

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
        if (regionLedMap[i].region_id == region_id) {
            return &regionLedMap[i];
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
        const RegionLedMapEntry& entry = regionLedMap[i];
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