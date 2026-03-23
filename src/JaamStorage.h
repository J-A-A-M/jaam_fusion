#pragma once

#include <SPIFFS.h>
#include "JaamConfig.h"
#include "JaamLogs.h"

struct StorageInfo {
    size_t totalBytes;
    size_t usedBytes;
    size_t freeBytes;
};

class JaamStorage {
public:
    JaamStorage();
    bool begin();
    // Flat array methods for currentMap
    bool saveCurrentMap(const RegionLedMapMeta* meta, const uint16_t* leds, size_t metaCount);
    bool loadCurrentMap(RegionLedMapMeta*& meta, uint16_t*& leds, size_t& metaCount, size_t& totalLeds);
    bool saveBgLedColors(const uint32_t* colors, int count);
    bool loadBgLedColors(uint32_t* colors, int maxCount, int& actualCount);
    void getStorageInfo();
    void getFilesInfo();

private:
    static const char* CUSTOM_MAP_PATH;
    static const char* BG_LED_COLORS_PATH;
};
