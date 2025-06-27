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
    bool saveCustomMap(const RegionLedMapEntry* map);
    bool loadCustomMap(RegionLedMapEntry* map);
    void getStorageInfo();
    void getFilesInfo();

private:
    static const char* CUSTOM_MAP_PATH;
};