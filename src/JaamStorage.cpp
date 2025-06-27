#include "JaamStorage.h"
#include <ArduinoJson.h>

const char* JaamStorage::CUSTOM_MAP_PATH = "/custom_map.json";

JaamStorage::JaamStorage() {}

bool JaamStorage::begin() {
    if (!SPIFFS.begin(true)) {
        LOG.println("[STORAGE] SPIFFS Mount Failed");
        return false;
    }
    LOG.println("[STORAGE] SPIFFS Mounted");
    return true;
}

bool JaamStorage::saveCustomMap(const RegionLedMapEntry* map) {
    File file = SPIFFS.open(CUSTOM_MAP_PATH, "w");
    if (!file) {
        LOG.printf("[STORAGE] Failed to open %s for writing\n", CUSTOM_MAP_PATH);
        return false;
    }

    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < MAX_REGIONS; ++i) {
        if (map[i].led_count > 0 && map[i].region_id != 0) {
            JsonObject obj = array.add<JsonObject>();
            obj["region_id"] = map[i].region_id;
            JsonArray leds = obj["leds"].to<JsonArray>();
            for (int j = 0; j < map[i].led_count; ++j) {
                leds.add(map[i].led_positions[j]);
            }
        }
    }

    bool success = serializeJson(doc, file) > 0;
    file.close();
    if (success) {
        LOG.printf("[STORAGE] Successfully saved custom map to %s\n", CUSTOM_MAP_PATH);
    } else {
        LOG.printf("[STORAGE] Failed to serialize or write custom map to %s\n", CUSTOM_MAP_PATH);
    }
    return success;
}

bool JaamStorage::loadCustomMap(RegionLedMapEntry* map) {
    LOG.printf("[STORAGE] Attempting to open custom map file: %s\n", CUSTOM_MAP_PATH);
    File file = SPIFFS.open(CUSTOM_MAP_PATH, "r");
    
    if (!file) {
        LOG.println("[STORAGE] Custom map file not found or cannot be opened. Using default map.");
        return false;
    }

    LOG.printf("[STORAGE] Opened custom map file, size: %u\n", file.size());

    String json_content = file.readString();
    file.close();
    LOG.println("[STORAGE] File content read into string.");

    if (json_content.length() == 0) {
        LOG.println("[STORAGE] File is empty.");
        return false;
    }
    
    yield();

    JsonDocument* doc = new (std::nothrow) JsonDocument();
    if (!doc) {
        LOG.println("[STORAGE] Failed to allocate JsonDocument on heap.");
        return false;
    }

    LOG.println("[STORAGE] Deserializing JSON from string...");
    DeserializationError error = deserializeJson(*doc, json_content);
    
    if (error) {
        LOG.printf("[STORAGE] deserializeJson() failed: %s\n", error.c_str());
        delete doc;
        return false;
    }
    LOG.println("[STORAGE] JSON deserialized successfully.");

    memset(map, 0, sizeof(RegionLedMapEntry) * MAX_REGIONS);
    JsonArray array = doc->as<JsonArray>();

    for (JsonObject obj : array) {
        uint16_t region_id = obj["region_id"];
        int map_idx = -1;
        for(int i = 0; i < MAX_REGIONS; ++i) {
            if (DISTRICTS[i].id == region_id) {
                map_idx = i;
                break;
            }
        }

        if (map_idx != -1) {
            map[map_idx].region_id = region_id;
            JsonArray leds = obj["leds"];
            uint8_t count = 0;
            for (JsonVariant v : leds) {
                if (count < MAX_LEDS_PER_REGION) {
                    map[map_idx].led_positions[count++] = v.as<int>();
                }
            }
            map[map_idx].led_count = count;
        }
    }
    
    delete doc;
    LOG.println("[STORAGE] Finished processing custom map.");
    return true;
}

void JaamStorage::getStorageInfo() {
    //StorageInfo info;
    size_t totalBytes;
    size_t usedBytes;
    size_t freeBytes;
    totalBytes = SPIFFS.totalBytes();
    usedBytes = SPIFFS.usedBytes();
    freeBytes = totalBytes - usedBytes;
    LOG.printf("[STORAGE] %d/%d bytes used, %d bytes free\n", usedBytes, totalBytes, freeBytes);
    //return info;
}

void JaamStorage::getFilesInfo() {
    File root = SPIFFS.open("/");
    if(!root){
        LOG.println(F("[STORAGE] Failed to open root directory"));
        return;
    }
    File file = root.openNextFile();
    while(file){
        if(!file.isDirectory()){
            LOG.printf("[STORAGE] - %s (%u bytes)\n", file.name(), file.size());
        }
        file = root.openNextFile();
    }
    root.close();
}