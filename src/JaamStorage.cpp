#include <ArduinoJson.h>
#include "JaamStorage.h"

const char* JaamStorage::CUSTOM_MAP_PATH = "/custom_map.json";
const char* JaamStorage::BG_LED_COLORS_PATH = "/bg_led_colors.json";

JaamStorage::JaamStorage() {}

bool JaamStorage::begin() {
    if (!SPIFFS.begin(true)) {
        LOG.printf("[STORAGE] SPIFFS Mount Failed\n");
        return false;
    }
    LOG.printf("[STORAGE] SPIFFS Mounted\n");
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
        LOG.printf("[STORAGE] Failed to open root directory\n");
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

bool JaamStorage::saveBgLedColors(const uint32_t* colors, int count) {
    File file = SPIFFS.open(BG_LED_COLORS_PATH, "w");
    if (!file) {
        LOG.printf("[STORAGE] Failed to open %s for writing\n", BG_LED_COLORS_PATH);
        return false;
    }

    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < count; ++i) {
        JsonObject obj = array.add<JsonObject>();
        obj["led"] = i;
        obj["color"] = String(colors[i], HEX);
    }

    bool success = serializeJson(doc, file) > 0;
    file.close();
    if (success) {
        LOG.printf("[STORAGE] Successfully saved BG LED colors to %s\n", BG_LED_COLORS_PATH);
    } else {
        LOG.printf("[STORAGE] Failed to serialize or write BG LED colors to %s\n", BG_LED_COLORS_PATH);
    }
    return success;
}

bool JaamStorage::loadBgLedColors(uint32_t* colors, int maxCount, int& actualCount) {
    LOG.printf("[STORAGE] Attempting to open BG LED colors file: %s\n", BG_LED_COLORS_PATH);
    File file = SPIFFS.open(BG_LED_COLORS_PATH, "r");
    
    if (!file) {
        LOG.printf("[STORAGE] BG LED colors file not found or cannot be opened. Using default black colors.\n");
        actualCount = 0;
        return false;
    }

    LOG.printf("[STORAGE] Opened BG LED colors file, size: %u\n", file.size());

    String json_content = file.readString();
    file.close();
    LOG.printf("[STORAGE] File content read into string: %s\n", json_content.c_str());

    if (json_content.length() == 0) {
        LOG.printf("[STORAGE] File is empty.\n");
        actualCount = 0;
        return false;
    }
    
    yield();

    JsonDocument* doc = new (std::nothrow) JsonDocument();
    if (!doc) {
        LOG.printf("[STORAGE] Failed to allocate JsonDocument on heap.\n");
        actualCount = 0;
        return false;
    }

    LOG.printf("[STORAGE] Deserializing JSON from string...\n");
    DeserializationError error = deserializeJson(*doc, json_content);
    
    if (error) {
        LOG.printf("[STORAGE] deserializeJson() failed: %s\n", error.c_str());
        delete doc;
        actualCount = 0;
        return false;
    }
    LOG.printf("[STORAGE] JSON deserialized successfully.\n");

    // Ініціалізуємо масив кольорів чорним кольором (відсутність підсвітки)
    for (int i = 0; i < maxCount; ++i) {
        colors[i] = 0x000000;
    }

    JsonArray array = doc->as<JsonArray>();
    actualCount = 0;

    for (JsonObject obj : array) {
        int led_index = obj["led"];
        String color_str = obj["color"];
        
        if (led_index >= 0 && led_index < maxCount) {
            // Конвертуємо HEX рядок в uint32_t
            uint32_t color = strtol(color_str.c_str(), nullptr, 16);
            colors[led_index] = color;
            actualCount = max(actualCount, led_index + 1);
        }
    }
    
    delete doc;
    LOG.printf("[STORAGE] Finished processing BG LED colors. Loaded %d colors.\n", actualCount);
    return true;
}

// ============================================================================
// Flat array methods for currentMap
// ============================================================================

bool JaamStorage::saveCurrentMap(const RegionLedMapMeta* meta, const uint16_t* leds, size_t metaCount) {
    if (!meta || !leds || metaCount == 0) {
        LOG.printf("[STORAGE] Invalid parameters for saveCurrentMap\n");
        return false;
    }
    
    File file = SPIFFS.open(CUSTOM_MAP_PATH, "w");
    if (!file) {
        LOG.printf("[STORAGE] Failed to open %s for writing\n", CUSTOM_MAP_PATH);
        return false;
    }

    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (size_t i = 0; i < metaCount; ++i) {
        if (meta[i].led_count > 0 && meta[i].region_id != 0) {
            JsonObject obj = array.add<JsonObject>();
            obj["region_id"] = meta[i].region_id;
            JsonArray ledsArray = obj["leds"].to<JsonArray>();
            
            const uint16_t* regionLeds = &leds[meta[i].start_index];
            for (uint8_t j = 0; j < meta[i].led_count; ++j) {
                ledsArray.add(regionLeds[j]);
            }
        }
    }

    bool success = serializeJson(doc, file) > 0;
    file.close();
    if (success) {
        LOG.printf("[STORAGE] Successfully saved current map to %s (%d regions)\n", CUSTOM_MAP_PATH, metaCount);
    } else {
        LOG.printf("[STORAGE] Failed to serialize or write current map to %s\n", CUSTOM_MAP_PATH);
    }
    return success;
}

bool JaamStorage::loadCurrentMap(RegionLedMapMeta*& meta, uint16_t*& leds, size_t& metaCount, size_t& totalLeds) {
    LOG.printf("[STORAGE] Attempting to open current map file: %s\n", CUSTOM_MAP_PATH);
    File file = SPIFFS.open(CUSTOM_MAP_PATH, "r");
    
    if (!file) {
        LOG.printf("[STORAGE] Current map file not found or cannot be opened.\n");
        meta = nullptr;
        leds = nullptr;
        metaCount = 0;
        totalLeds = 0;
        return false;
    }

    LOG.printf("[STORAGE] Opened current map file, size: %u\n", file.size());

    String json_content = file.readString();
    file.close();

    if (json_content.length() == 0) {
        LOG.printf("[STORAGE] File is empty.\n");
        meta = nullptr;
        leds = nullptr;
        metaCount = 0;
        totalLeds = 0;
        return false;
    }
    
    yield();

    JsonDocument* doc = new (std::nothrow) JsonDocument();
    if (!doc) {
        LOG.printf("[STORAGE] Failed to allocate JsonDocument on heap.\n");
        meta = nullptr;
        leds = nullptr;
        metaCount = 0;
        totalLeds = 0;
        return false;
    }

    LOG.printf("[STORAGE] Deserializing JSON from string...\n");
    DeserializationError error = deserializeJson(*doc, json_content);
    
    if (error) {
        LOG.printf("[STORAGE] deserializeJson() failed: %s\n", error.c_str());
        delete doc;
        meta = nullptr;
        leds = nullptr;
        metaCount = 0;
        totalLeds = 0;
        return false;
    }
    LOG.printf("[STORAGE] JSON deserialized successfully.\n");

    JsonArray array = doc->as<JsonArray>();
    metaCount = array.size();
    
    if (metaCount == 0) {
        LOG.printf("[STORAGE] Empty map array.\n");
        delete doc;
        meta = nullptr;
        leds = nullptr;
        totalLeds = 0;
        return false;
    }

    // Підрахунок загальної кількості LED
    totalLeds = 0;
    for (JsonObject obj : array) {
        JsonArray ledsArray = obj["leds"];
        totalLeds += ledsArray.size();
    }

    // Виділення пам'яті
    meta = new (std::nothrow) RegionLedMapMeta[metaCount];
    leds = new (std::nothrow) uint16_t[totalLeds];

    if (!meta || !leds) {
        LOG.printf("[STORAGE] Failed to allocate memory for map arrays\n");
        delete[] meta;
        delete[] leds;
        delete doc;
        meta = nullptr;
        leds = nullptr;
        metaCount = 0;
        totalLeds = 0;
        return false;
    }

    // Заповнення масивів
    size_t metaIdx = 0;
    size_t ledsIdx = 0;
    
    for (JsonObject obj : array) {
        uint16_t region_id = obj["region_id"];
        JsonArray ledsArray = obj["leds"];
        uint8_t led_count = ledsArray.size();

        meta[metaIdx].region_id = region_id;
        meta[metaIdx].led_count = led_count;
        meta[metaIdx].start_index = ledsIdx;

        for (uint8_t j = 0; j < led_count; ++j) {
            leds[ledsIdx++] = ledsArray[j].as<uint16_t>();
        }

        metaIdx++;
    }
    
    delete doc;
    LOG.printf("[STORAGE] Finished loading current map: %d regions, %d total LEDs\n", metaCount, totalLeds);
    return true;
}