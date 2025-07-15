#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>


#include <WiFiManager.h>
#include <NTPtime.h>

#include <esp_system.h>
#include <async.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "JaamAnimation.h"
#include "JaamBattery.h"
#include "JaamLogs.h"
#include "JaamConfig.h"
#include "JaamSettings.h"
#include "JaamWeb.h"
#include "JaamLed.h"
#include "JaamUtils.h"
#include "JaamStorage.h"
#include "JaamDisplay.h"

using namespace websockets;

// --- MAIN Configuration ---
char                chipID[13];
char                currentFwVersion[25];

Async               async = Async(20);

NTPtime             timeClient(2);
DSTime              dst(3, 0, 7, 3, 10, 0, 7, 4); //https://en.wikipedia.org/wiki/Eastern_European_Summer_Time

JaamSettings        settings;
JaamFirmware        firmware;
JaamWeb             web;
JaamLed             led;
JaamBattery         battery;
JaamStorage         storage;
JaamDisplay         display;

// --- LED Configuration ---
Adafruit_NeoPixel*  strip_main = nullptr;
Adafruit_NeoPixel*  strip_bg = nullptr;
Adafruit_NeoPixel*  strip_service = nullptr;
SemaphoreHandle_t   stripMutex = nullptr;
uint32_t            num_leds_main = 0;
uint16_t            num_leds_service = 5;
uint8_t             currentBrightness = 0;
std::vector<int>    allLedsMain;
std::vector<int>    allLedsBg;

// --- ANIMATION Configuration ---
AnimationManager        animation;
AnimationParams::Type   animType;

// --- MAP Configuration ---
std::map<uint16_t, uint16_t>    alertsMap;
RegionLedMapEntry               customMap[MAX_REGIONS];

// --- TASKS Configuration ---
bool                needAdaptAnimationColors = false;
bool                needAdaptStripBrightness = false;
bool                needReconnectWebsocket = false;
bool                needAdaptColors = false;
bool                needRecalculateLeds = false;
bool                needReconnectMainStrip;
bool                needReconnectBgStrip;
bool                needReconnectServiceStrip;
bool                needUpdateBatteryPin = false; // Flag to update battery pin in web settings
bool                needReconfigureDisplay = false; // Flag to reconfigure display settings

// --- WIFI Configuration ---
WiFiManager         wm;

uint32_t            lastWifiConnectTime = 0;  // Track when WiFi was last connected
uint16_t            alertsHash = 0;
bool                wifiConnected = false;


// --- WEBSOCKET Configuration ---
WebsocketsClient    websocket;
time_t              websocketLastPingTime = 0;
bool                isFirstDataFetchCompleted = false;
bool                apiConnected;
bool                websocketReconnect = false;
uint32_t            lastWebsocketConnectTime = 0;



uint8_t             legacy = 0;

// --- CLOCK ---
bool needDivider = false;


// --- FreeRTOS Task Handles ---
// TaskHandle_t memoryTaskHandle = nullptr;
// TaskHandle_t wifiReconnectTaskHandle = nullptr;
// TaskHandle_t websocketProcessTaskHandle = nullptr;


void clearAllAlertsMaps() {
    // Логування стану пам'яті перед очищенням
    size_t memBefore = ESP.getFreeHeap();
    LOG.printf("[MEMORY] Free heap before clearing maps: %u bytes\n", memBefore);
    
    // Очищаємо всі map'и
    alertsMap.clear();
    
    // Додаткове очищення пам'яті після clear()
    // Для std::map викликаємо shrink_to_fit через swap з пустими контейнерами
    std::map<uint16_t, uint16_t>().swap(alertsMap);

    // Принудове очищення пам'яті
    forceMemoryCleanup("after maps clearing");
    
    // Дефрагментація пам'яті
    defragmentMemory("after maps clearing");
    
    // Логування результату
    size_t memAfter = ESP.getFreeHeap();
    int memReclaimed = (int)(memAfter - memBefore);
    
    LOG.printf("[MAIN] Clearing all alerts maps completed\n");
    LOG.printf("[MEMORY] Memory reclaimed: %+d bytes (before: %u, after: %u)\n", 
               memReclaimed, memBefore, memAfter);
}

void rebootDevice(int time = 2000, bool async = false) {
    LOG.printf("[MAIN] Rebooting in %d ms...\n", time);
    
    // Clean up all resources before reboot
    clearAllAlertsMaps();
    animation.clearAllAnimations();
    
    // Close websocket connection properly
    if (websocket.available()) {
        websocket.close();
    }
    
    // Add memory logging before reboot
    LOG.printf("[MEMORY] Free heap before reboot: %u bytes\n", ESP.getFreeHeap());
    
    delay(time);
    ESP.restart();
}

// --- WEBSOCKET Functions ---

void printHex(const String& data) {
    LOG.print("[WEBSOCKET] HEX: ");
    for (size_t i = 0; i < data.length(); ++i) {
        LOG.printf("%02X ", static_cast<uint8_t>(data[i]));
    }
    LOG.println();
}

// Структура для зберігання diff
struct AlertDiff {
    uint16_t region_id;
    uint16_t previous_flags;
    uint16_t current_flags;
    bool has_changes;
};

// Масив з описом типів тривог
const char* ALERT_TYPES[] = {
    "Air",      // bit 0
    "Artillery", // bit 1
    "Urban",    // bit 2
    "Chemical", // bit 3
    "Nuclear",  // bit 4
    "Drones",   // bit 5
    "Missiles", // bit 6
    "KAB",      // bit 7
    "Ballistic",// bit 8
    "Explosion" // bit 9
};
const int ALERT_TYPES_COUNT = sizeof(ALERT_TYPES) / sizeof(ALERT_TYPES[0]);

void servicePin(ServiceLed type) {
    if (strip_service != nullptr) {
        uint32_t color = getServicePinColor(type);
        animation.safeStripOperation(strip_service, [type, color](Adafruit_NeoPixel* strip) {
            strip->setPixelColor(type, color);
            strip->show();
        }); 
    }
}

void checkServicePins() {
    servicePin(POWER);
    servicePin(WIFI);
    servicePin(DATA);
}

// Функція для розрахунку diff
AlertDiff calculateAlertDiff(uint16_t region_id, uint16_t previous_flags, uint16_t current_flags) {
    AlertDiff diff;
    diff.region_id = region_id;
    diff.previous_flags = previous_flags;
    diff.current_flags = current_flags;
    diff.has_changes = (previous_flags != current_flags);
    return diff;
}

void animateLed(Adafruit_NeoPixel* strip, int led_position, int bit, uint16_t region_id, bool increase = true) {
    //LOG.printf("[ANIMATION] LED %d: region %d to %d\n", led_position, region_id, bit); 

    if (strip == nullptr) {
        LOG.printf("[ANIMATION] LED %d: strip is nullptr\n", led_position);
        return;
    }
    
    uint32_t color;
    uint32_t initialColor = 0x000000; // Початковий колір для анімації
    uint32_t period;
    uint32_t cycles;
    uint8_t startBrightness = 50;
    uint8_t endBrightness = 255;
    uint8_t ledCount;

    int actualBit = getHighestActualBit(bit);

    if (increase && actualBit != bit) {
        return;
    }

    switch (actualBit) {
        case -1: 
            color = animation.ledActualColor(strip, led_position, true);               
            animType = AnimationParams::Type::ONE_WAY_BLEND_FADE;
            cycles = 1;
            period = 10000;
            break;
        case 0:
            color = animation.colorFromHex(settings.getString(COLOR_ALERT)); 
            animType = (increase) ? AnimationParams::Type::FADE : AnimationParams::Type::ONE_WAY_BLEND_FADE;
            startBrightness = (increase) ? 50 : led.brightnessAbsolute(settings.getInt(BRIGHTNESS_ALERT));
            endBrightness = (increase) ? led.brightnessAbsolute(settings.getInt(BRIGHTNESS_ALERT)): 50; 
            period = (increase) ? 1000 : 10000;
            cycles = (increase) ? settings.getInt(ALERT_ON_TIME) * 60 : 1;
            break;
        case 5:
            color = animation.colorFromHex(settings.getString(COLOR_DRONES));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_EXPLOSION));
            endBrightness = 100; 
            animType = (increase) ? AnimationParams::Type::PULSE : AnimationParams::Type::ONE_WAY_BLEND_FADE;
            period = (increase) ? 1000 : 10000;
            cycles = (increase) ? settings.getInt(EXPLOSION_TIME) * 60 : 1; 
            break;
        case 6:
            color = animation.colorFromHex(settings.getString(COLOR_MISSILES));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_EXPLOSION));
            endBrightness = 100; 
            animType = (increase) ? AnimationParams::Type::PULSE : AnimationParams::Type::ONE_WAY_BLEND_FADE;
            period = (increase) ? 1000 : 10000;
            cycles = (increase) ? settings.getInt(EXPLOSION_TIME) * 60 : 1; 
            break;
        case 7:
            color = animation.colorFromHex(settings.getString(COLOR_KABS));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_EXPLOSION));
            endBrightness = 100; 
            animType = (increase) ? AnimationParams::Type::PULSE : AnimationParams::Type::ONE_WAY_BLEND_FADE; 
            period = (increase) ? 1000 : 10000;
            cycles = (increase) ? settings.getInt(EXPLOSION_TIME) * 60 : 1;   
            break;
        case 8:
            color = animation.colorFromHex(settings.getString(COLOR_BALLISTIC));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_EXPLOSION));
            endBrightness = 100; 
            animType = (increase) ? AnimationParams::Type::PULSE : AnimationParams::Type::ONE_WAY_BLEND_FADE;
            period = (increase) ? 1000 : 10000;
            cycles = (increase) ? settings.getInt(EXPLOSION_TIME) * 60 : 1; 
            break;
        case 9:
            color = animation.colorFromHex(settings.getString(COLOR_EXPLOSION));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_EXPLOSION));
            endBrightness = 100; 
            animType = (increase) ? AnimationParams::Type::PULSE : AnimationParams::Type::ONE_WAY_BLEND_FADE;
            period = (increase) ? 1000 : 10000;
            cycles = (increase) ? settings.getInt(EXPLOSION_TIME) * 60 : 1; 
            break;
        default:
            LOG.printf("[ANIMATION] LED %d: unknown bit %d\n", led_position, actualBit);
            return;
    }
    int ledsIdx[1];      // для strip_main та strip_service
    int* ledsPtr = nullptr;

    if (strip == strip_main) {
        ledsIdx[0] = led_position;
        ledsPtr = ledsIdx;
        ledCount = 1;
    } else if (strip == strip_bg) {
        ledsPtr = allLedsBg.data();
        ledCount = strip->numPixels();
    } else if (strip == strip_service) {
        ledsIdx[0] = led_position;
        ledsPtr = ledsIdx;
        ledCount = 1;
    } else {
        LOG.println("[ERROR] Invalid strip for animation");
        return;
    }
    
    if (!animation.createAnimation(
        animType,
        strip,
        ledsPtr,
        ledCount,
        color,
        initialColor,
        period,
        cycles,
        startBrightness,
        endBrightness,
        region_id
    )) {
        LOG.println("[ERROR] Failed to create animation");
        return;
    }
}

void analyzeAlertChanges(const std::vector<AlertDiff>& diffs) {

}

void onMessageCallback(WebsocketsMessage msg) {
    checkFreeHeap("Websockets onMessageCallback");

    // Ігноруємо текстові повідомлення
    if (!msg.isBinary()) {
        LOG.println("[WEBSOCKET] Message in not binary");
        return;
    } else {
        //LOG.printf("[WEBSOCKET] bytes payload len: %d\n", msg.length());
    }
    websocketLastPingTime = millis();

    WSString payload = msg.rawData();
    const uint8_t* data = reinterpret_cast<const uint8_t*>(payload.data());
    size_t len = payload.size();

    // 3) Мінімальна довжина — хоча б 1B на type
    if (len < HEADER_SZ) {
        LOG.printf("[ERROR] len < HEADER_SZ\n");
        return;
    }

    // 4) Перевіряємо тип пакета
    uint8_t type = data[0];
    if (type != TYPE_ALERTS_BATCH && type != TYPE_NOTIFICATIONS_BATCH) {
        LOG.printf("[ERROR] message type unknown\n");
        return;
    }

    // ітератор по пакету
    uint8_t ledCount;
    size_t bodyLen;

    if(type == TYPE_NOTIFICATIONS_BATCH) {
        LOG.printf("[WEBSOCKET] TYPE_NOTIFICATIONS_BATCH received\n");
        bodyLen = len - HEADER_SZ;

        // payloadLen має ділитися на RECORD_SZ
        if (bodyLen == 0 || (bodyLen % RECORD_SZ) != 0) {
            // некоректний фрейм — пропускаємо і реконнектимось
            LOG.printf("[ERROR] bodyLen == 0 || (bodyLen % RECORD_SZ\n");
            needReconnectWebsocket = true;
            return;
        }

        // Обчислюємо кількість записів
        size_t count = bodyLen / RECORD_SZ;

        // Розбираємо count записів по RECORD_SZ
        const uint8_t* ptr = data + HEADER_SZ;

        LOG.printf("[WEBSOCKET] TYPE_NOTIFICATIONS_BATCH data processing\n");

        bool needToAnimateBgHomeRegion = false;
        int homeRegionBit = 0;

        for (size_t i = 0; i < count; ++i) {
            uint16_t region_id = uint16_t(ptr[0]) | (uint16_t(ptr[1]) << 8);
            uint16_t flags16   = uint16_t(ptr[2]) | (uint16_t(ptr[3]) << 8);
            ptr += RECORD_SZ;

            const int* leds = getLedsForRegion(region_id, ledCount);
            if (leds == nullptr) {
                // Якщо такого регіону немає — пропускаємо цей запис
                LOG.printf("[WEBSOCKET] notification region %d: %d skipped - no leds associated\n", region_id, flags16);
                continue;
            } else {
                LOG.printf("[WEBSOCKET] notification region %d: %d\n", region_id, flags16);
            }

            int highestBitLed = -1;
            // Шукаємо LED для цього регіону
            const RegionLedMapEntry* entry = getRegionEntry(region_id);
            if (entry) {
                // Шукаємо всі леди для цього регіону
                for (uint8_t i = 0; i < entry->led_count; ++i) {
                    int led_position = entry->led_positions[i];
                    //LOG.printf("[WEBSOCKET] LED %d: ", led_position);
                    // Шукаємо всі регіони для цього LED
                    const std::vector<uint16_t>& regions = getRegionsForLed(led_position);
                    for (uint16_t rid : regions) {
                        LOG.printf(" %d ", rid);
                        // Шукаємо найвищий біт для регіону в alertsMap
                        auto alerts_it = alertsMap.find(rid);
                        if (alerts_it != alertsMap.end()) {
                            int highestBitRegion = findHighestBit16(alerts_it->second);
                            LOG.printf("[%d] ", highestBitRegion);

                            if (highestBitLed < highestBitRegion) {
                                highestBitLed = highestBitRegion;
                            }
                        }
                    }
                }  
                LOG.println();
            } else {
                LOG.printf("[WEBSOCKET] No LEDs found for region %d\n", region_id);
            }
    
            int actualBitLed = getHighestActualBit(highestBitLed);
            int actualBitDiff = getHighestActualBit(findHighestBit16(flags16, false));

            LOG.printf("[WEBSOCKET] actualBitLed %d, actualBitRegion %d\n", actualBitLed, actualBitDiff);

            

            if (actualBitDiff > actualBitLed) {
                for (int i = 0; i < ledCount; ++i) {
                    animateLed(strip_main,leds[i], actualBitDiff, region_id, true);
                    if (isLedInHomeRegion(leds[i])) {
                        needToAnimateBgHomeRegion = true;
                        homeRegionBit = actualBitDiff;
                    }
                }
            }
        }
        if (needToAnimateBgHomeRegion && settings.getInt(BG_LED_MODE) == 0 && strip_bg != nullptr) {
            LOG.printf("[WEBSOCKET] Animating home region LEDs: region %d, bit %d\n",
                settings.getInt(HOME_DISTRICT), homeRegionBit);
            animateLed(strip_bg, 0, homeRegionBit, settings.getInt(HOME_DISTRICT), true);
        }else {
            LOG.println("[WEBSOCKET] No changes in home region LEDs");
        }
    }

    if(type == TYPE_ALERTS_BATCH) {
        LOG.printf("[WEBSOCKET] TYPE_ALERTS_BATCH received\n");
        // Обчислюємо довжину payload після заголовка
        bodyLen = len - HEADER_SZ - HASH_SZ;

        // payloadLen має ділитися на RECORD_SZ
        if (bodyLen == 0 || (bodyLen % RECORD_SZ) != 0) {
            // некоректний фрейм — пропускаємо і реконнектимось
            LOG.printf("[ERROR] bodyLen == 0 || (bodyLen % RECORD_SZ\n");
            needReconnectWebsocket = true;
            return;
        }

        // Перевіряємо  хеш
        uint16_t actualHash = (static_cast<uint16_t>(data[1]) << 8) | data[2];
        uint16_t prevHash = (static_cast<uint16_t>(data[3]) << 8) | data[4];
        LOG.printf("[WEBSOCKET] hash check, local: [0x%04X] prev: [0x%04X], actual: [0x%04X]\n", alertsHash, prevHash, actualHash);

        if (prevHash != alertsHash) {
            // некоректний хеш — пропускаємо і реконнектимось
            LOG.printf("[ERROR] prevHash != alertsHash\n");
            needReconnectWebsocket = true;
            return;
        }

        // Обчислюємо кількість записів
        size_t count = bodyLen / RECORD_SZ;

        // Розбираємо count записів по RECORD_SZ
        const uint8_t* ptr = data + HEADER_SZ + HASH_SZ;

        LOG.printf("[WEBSOCKET] TYPE_ALERTS_BATCH data processing\n");

        std::vector<AlertDiff> diffs;
        // Створюємо тимчасову мапу для нових значень
        std::map<uint16_t, uint16_t> alertsMapActual;

        for (size_t i = 0; i < count; ++i) {
            uint16_t region_id = uint16_t(ptr[0]) | (uint16_t(ptr[1]) << 8);
            uint16_t flags16   = uint16_t(ptr[2]) | (uint16_t(ptr[3]) << 8);
            ptr += RECORD_SZ;

            const int* leds = getLedsForRegion(region_id, ledCount);
            if (leds == nullptr) {
                // Якщо такого регіону немає — пропускаємо цей запис
                LOG.printf("[WEBSOCKET] alert region %d: %d skipped - no leds associated\n", region_id, flags16);
                continue;
            } else {
                LOG.printf("[WEBSOCKET] alert region %d: %d\n", region_id, flags16);
            }

            // Отримуємо попередній стан
            uint16_t previous_flags = alertsMap[region_id];
            
            // Розраховуємо diff
            AlertDiff diff = calculateAlertDiff(region_id, previous_flags, flags16);
            if (diff.has_changes) {
                diffs.push_back(diff);
            }
            
            // зберігаємо нові значення в alertsMapActual для подальшого мержу після перевірки на зміни
            alertsMapActual[region_id] = flags16;
        }
        
        std::map<int, LedBit> led_bits_diff;
        std::map<int, LedBit> led_bits_alerts;

        LOG.printf("[WEBSOCKET] processing %d diffs\n", (int)diffs.size());
        //LOG.println();

        // Проходимо по всіх змінах
        bool needToAnimateBgHomeRegion = false;
        bool homeRegionIncrease = false;
        int homeRegionBit = 0;
        for (const auto& diff : diffs) {
            // Показуємо зміни для цього регіону
            // LOG.printf("[DIFF] Region %d: flags changed from 0x%04X to 0x%04X\n", 
            //     diff.region_id, diff.previous_flags, diff.current_flags);
            
            // Показуємо які біти змінилися
            // for (int bit = 0; bit < ALERT_TYPES_COUNT; bit++) {
            //     bool prev_state = diff.previous_flags & (1 << bit);
            //     bool curr_state = diff.current_flags & (1 << bit);
                
            //     if (prev_state != curr_state) {
            //         LOG.printf("[DIFF]   Bit %d (%s): %s -> %s\n", 
            //             bit,
            //             ALERT_TYPES[bit],
            //             prev_state ? "ON" : "OFF",
            //             curr_state ? "ON" : "OFF"
            //         );
            //     }
            // }

            // Знаходимо найвищий біт для цього регіону
            int highest_bit_diff = findHighestBit16(diff.current_flags);
            // LOG.printf("[DIFF]   Highest bit for region %d: %d\n", 
            //     diff.region_id, highest_bit_diff);

            // Шукаємо LED для цього регіону
            const RegionLedMapEntry* entry = getRegionEntry(diff.region_id);
            if (entry) {
                //LOG.printf("[DIFF]   LEDs for region %d: ", diff.region_id);
                for (uint8_t i = 0; i < entry->led_count; ++i) {
                    int led_position = entry->led_positions[i];
                    //LOG.printf("%d ", led_position);

                    // Оновлюємо найвищий біт для LED
                    auto it = led_bits_diff.find(led_position);
                    if (it == led_bits_diff.end() || highest_bit_diff > it->second.highest_bit) {
                        led_bits_diff[led_position] = {highest_bit_diff, diff.region_id};
                    }
                }
                // Шукаємо всі леди для цього регіону
                for (uint8_t i = 0; i < entry->led_count; ++i) {
                    int led_position = entry->led_positions[i];
                    //LOG.printf("\n[DIFF] LED %d regions: ", led_position);

                    // Шукаємо всі регіони для цього LED
                    const std::vector<uint16_t>& regions = getRegionsForLed(led_position);
                    for (uint16_t region_id : regions) {
                        //LOG.printf("%d ", region_id);

                        // Шукаємо найвищий біт для регіону в alertsMap
                        auto alerts_it = alertsMap.find(region_id);
                        if (alerts_it != alertsMap.end()) {
                            int highest_bit_region = findHighestBit16(alerts_it->second);
                            //LOG.printf("[%d] ", highest_bit_region);

                            // Оновлюємо найвищий біт для LED
                            auto it = led_bits_alerts.find(led_position);
                            if (it == led_bits_alerts.end() || highest_bit_region > it->second.highest_bit) {
                                led_bits_alerts[led_position] = {highest_bit_region, region_id};
                            }
                        }
                    }
                    //LOG.println();
                }  
                //LOG.println();
            } else {
                //LOG.printf("[DIFF]   No LEDs found for region %d\n", diff.region_id);
            }
        }

        // Виводимо фінальний список бітів для LED
        // LOG.printf("[DIFF] LED bits summary:\n");
        // for (const auto& led : led_bits_diff) {
        //     LOG.printf("[DIFF] LED diff %d: highest_bit=%d, region_id=%d\n",
        //         led.first,
        //         led.second.highest_bit,
        //         led.second.region_id
        //     );
        // }
        // for (const auto& led : led_bits_alerts) {
        //     LOG.printf("[DIFF] LED alerts %d: highest_bit=%d, region_id=%d\n",
        //         led.first,
        //         led.second.highest_bit,
        //         led.second.region_id
        //     );
        // }

        // вмерджуємо alertsMapActual в основний alertsMap
        for (const auto& pair : alertsMapActual) {
            alertsMap[pair.first] = pair.second;
        }

        if (isFirstDataFetchCompleted) {
            // Виводимо фінальний список бітів для LED
            //LOG.printf("\n[WEBSOCKET] LED bits summary:\n");
            for (const auto& led : led_bits_diff) {
                // Порівнюємо з led_bits_alerts
                auto alerts_it = led_bits_alerts.find(led.first);
                if (alerts_it != led_bits_alerts.end()) {
                    int diff_bit = led.second.highest_bit;
                    int alerts_bit = alerts_it->second.highest_bit;

                    if (diff_bit > alerts_bit) {
                        //LOG.printf("[WEBSOCKET] LED %d: region %d increasing bit from %d to %d\n",
                        //    led.first, led.second.region_id, alerts_bit, diff_bit);
                            animateLed(strip_main, led.first, diff_bit, led.second.region_id, true);
                            if (isLedInHomeRegion(led.first)) {
                                needToAnimateBgHomeRegion = true;
                                homeRegionBit = diff_bit;
                                homeRegionIncrease = true;
                            }
                    } else if (diff_bit < alerts_bit) {
                        //LOG.printf("[WEBSOCKET] LED %d: region %d decreasing bit from %d to %d\n",
                        //    led.first, led.second.region_id, alerts_bit, diff_bit);
                            animateLed(strip_main, led.first, diff_bit, led.second.region_id, false);
                            if (isLedInHomeRegion(led.first)) {
                                needToAnimateBgHomeRegion = true;
                                homeRegionBit = diff_bit;
                                homeRegionIncrease = false;
                            }
                    }
                }
            }
            if (needToAnimateBgHomeRegion && settings.getInt(BG_LED_MODE) == 0 && strip_bg != nullptr) {
                LOG.printf("[WEBSOCKET] Animating home region LEDs: region %d, bit %d, increase %d\n",
                    settings.getInt(HOME_DISTRICT), homeRegionBit, homeRegionIncrease);
                animateLed(strip_bg, 0, homeRegionBit, settings.getInt(HOME_DISTRICT), homeRegionIncrease);
            }else {
                LOG.println("[WEBSOCKET] No changes in home region LEDs");
            }
        }
        //LOG.println();
        alertsHash = actualHash;
    }
    if (!isFirstDataFetchCompleted) {
        LOG.printf("[WEBSOCKET] init processing\n");
        if (strip_main != nullptr) {
            animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
                for(uint32_t i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
        if (strip_bg != nullptr && settings.getInt(BG_LED_MODE) == 0) {
            animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
                uint32_t color = animation.stripActualColor(strip);
                for(uint32_t i = 0; i < strip->numPixels(); i++) {
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }    
    }

    isFirstDataFetchCompleted = true;
    checkFreeHeap("Websockets data processing");
}

void logWebsocketCloseReason(websockets::CloseReason reason) {
    switch (reason) {
        case websockets::CloseReason_None:
            LOG.println("[WEBSOCKET] WebSocket CloseReason: None (-1)");
            break;
        case websockets::CloseReason_NormalClosure:
            LOG.println("[WEBSOCKET] WebSocket CloseReason: Normal Closure (1000)");
            break;
        case websockets::CloseReason_GoingAway:
            LOG.println("[WEBSOCKET] WebSocket CloseReason: Going Away (1001)");
            break;
        case websockets::CloseReason_ProtocolError:
            LOG.println("[WEBSOCKET] WebSocket CloseReason: Protocol Error (1002)");
            break;
        case websockets::CloseReason_UnsupportedData:
            LOG.println("[WEBSOCKET] WebSocket CloseReason: Unsupported Data (1003)");
            break;
        case websockets::CloseReason_NoStatusRcvd:
            LOG.println("[WEBSOCKET] WebSocket CloseReason: No Status Received (1005)");
            break;
        case websockets::CloseReason_AbnormalClosure:
            LOG.println("[WEBSOCKET] WebSocket CloseReason: Abnormal Closure (1006)");
            break;
        case websockets::CloseReason_InvalidPayloadData:
            LOG.println("[WEBSOCKET] WebSocket CloseReason: Invalid Payload Data (1007)");
            break;
        case websockets::CloseReason_PolicyViolation:
            LOG.println("[WEBSOCKET] WebSocket CloseReason: Policy Violation (1008)");
            break;
        case websockets::CloseReason_MessageTooBig:
            LOG.println("[WEBSOCKET] WebSocket CloseReason: Message Too Big (1009)");
            break;
        case websockets::CloseReason_InternalServerError:
            LOG.println("[WEBSOCKET] WebSocket CloseReason: Internal Server Error (1011)");
            break;
        default:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: Unknown (%d)\n", static_cast<int>(reason));
            break;
    }
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if (event == WebsocketsEvent::ConnectionOpened) {
        apiConnected = true;
        servicePin(DATA);
        LOG.println("[WEBSOCKET] connnection opened");
        
        websocketLastPingTime = millis();
        //ha.setMapApiConnect(apiConnected);
    } else if (event == WebsocketsEvent::ConnectionClosed) {
        apiConnected = false;
        servicePin(DATA);
        LOG.println("[WEBSOCKET] connnection closed");
        isFirstDataFetchCompleted = false;
        LOG.printf("[MEMORY] Heap before close: %u\n", ESP.getFreeHeap());
        //websocket.close();
        auto reason = websocket.getCloseReason();
        logWebsocketCloseReason(reason);
        delay(500);
        LOG.printf("[MEMORY] Heap after close: %u\n", ESP.getFreeHeap());
        //ha.setMapApiConnect(apiConnected);
    } else if (event == WebsocketsEvent::GotPing) {
        LOG.printf("[WEBSOCKET] ping, payload: [%s], len: %d\n", data.c_str(), data.length());
        //printHex(data);
        websocketLastPingTime = millis();
    } else if (event == WebsocketsEvent::GotPong) {
        LOG.printf("[WEBSOCKET] pong, payload: [%s], len: %d\n", data.c_str(), data.length());
        //printHex(data);
    }
}

void socketConnect() {
    LOG.println("[WEBSOCKET] connection start...");
    servicePin(DATA);
    //showServiceMessage("підключення...", "Сервер даних");
    alertsHash = 0;
    websocket.onMessage(onMessageCallback);
    websocket.onEvent(onEventsCallback);
    long startTime = millis();
    char webSocketUrl[100];
    sprintf(
        webSocketUrl,
        "ws://%s:%d/data_fusion_v1",
        settings.getString(WS_SERVER_HOST),
        settings.getInt(WS_SERVER_PORT)
    );
    LOG.printf("[WEBSOCKET] url:%s\n", webSocketUrl);
    websocket.connect(webSocketUrl);
    if (websocket.available()) {
        apiConnected = true;
        servicePin(DATA);
        clearAllAlertsMaps();
        animation.clearAllAnimations();
        LOG.printf("[WEBSOCKET] connection time - %d ms\n", millis() - startTime);
        char chipIdInfo[25];
        sprintf(chipIdInfo, "chip_id:%s", chipID);
        LOG.printf("[WEBSOCKET] %s\n", chipIdInfo);
        websocket.send(chipIdInfo);
        char firmwareInfo[100];
        sprintf(firmwareInfo, "firmware:%s_%s", currentFwVersion, settings.getString(ID));
        LOG.printf("[WEBSOCKET] %s\n", firmwareInfo);
        websocket.send(firmwareInfo);
        char userInfo[250];
        JsonDocument userInfoJson;
        userInfoJson["legacy"] = legacy;
        sprintf(userInfo, "user_info:%s", userInfoJson.as<String>().c_str());
        LOG.printf("[WEBSOCKET] %s\n", userInfo);
        websocket.send(userInfo);
        websocket.ping("A");
        websocketReconnect = false;
        lastWebsocketConnectTime  = millis();
        
        //showServiceMessage("підключено!", "Сервер даних", 3000);
    } else {
        //showServiceMessage("недоступний", "Сервер даних", 3000);
    }
}

// --- LED Functions ---

// функція очищення
void cleanup() {
    if (strip_main != nullptr) {
        animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
            strip->clear();
            // Встановлюємо дефолтний колір
            for(int i = 0; i < strip->numPixels(); i++) {
                strip->setPixelColor(i, DefaultColors::OFF);
            }
            strip->show();
            delete strip;
            strip_main = nullptr;
        });
    }
    
    if (strip_bg != nullptr) {
        animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
            strip->clear();
            // Встановлюємо дефолтний колір
            for(int i = 0; i < settings.getInt(BG_LED_COUNT); i++) {
                strip->setPixelColor(i, DefaultColors::OFF);
            }
            strip->show();
            delete strip;
            strip_bg = nullptr;
        });
    }
    
    if (strip_service != nullptr) {
        animation.safeStripOperation(strip_service, [](Adafruit_NeoPixel* strip) {
            strip->clear();
            // Встановлюємо дефолтний колір
            for(int i = 0; i < num_leds_service; i++) {
                strip->setPixelColor(i, DefaultColors::OFF);
            }
            strip->show();
            delete strip;
            strip_service = nullptr;
        });
    }
}

void cleanupSingleStrip(Adafruit_NeoPixel*& strip, uint32_t defaultColor) {
    if (strip != nullptr) {
        animation.safeStripOperation(strip, [&strip, defaultColor](Adafruit_NeoPixel* stripPtr) {
            stripPtr->clear();
            // Встановлюємо дефолтний колір
            for(uint32_t i = 0; i < strip->numPixels(); i++) {
                stripPtr->setPixelColor(i, defaultColor);
            }
            stripPtr->show();
            delete stripPtr;
        });
        strip = nullptr;
    }
}

void initStripMain() {
    StripStatus status;

    legacy = settings.getInt(LEGACY);

    if (legacy == 4) {
        num_leds_main = 273;
    } else {
        num_leds_main = 26;
    }
    
    if (settings.getInt(MAIN_LED_PIN) > 0) {
        uint8_t ledType = settings.getInt(MAIN_LED_COLOR_FORMAT) + settings.getInt(MAIN_LED_FREQUENCY);
        LOG.printf("[LED] Initializing strip_main on pin %d with %d LEDs, type %d (format:%d + freq:%d)\n", 
                   settings.getInt(MAIN_LED_PIN), num_leds_main, ledType, 
                   settings.getInt(MAIN_LED_COLOR_FORMAT), settings.getInt(MAIN_LED_FREQUENCY));
        status = led.createStrip(strip_main, settings.getInt(MAIN_LED_PIN), num_leds_main, 0, DefaultColors::OFF, ledType);
        if (status != StripStatus::SUCCESS) {
            LOG.printf("[LED] ERROR: Failed to create strip_main: %d\n", status);
        } else {
            LOG.println("[LED] SUCCESS: strip_main");
        }
    }
    else {
        LOG.println("[LED] SKIP: strip_main (pin <= 0)");
    }
    // if (strip_main != nullptr) {
    //     // Спочатку встановлюємо LEDs з мінімальною яскравістю
    //     animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
    //         strip->setBrightness(led.brightnessMapped(0));
    //         for(uint16_t i = 0; i < strip->numPixels(); i++) {
    //             uint32_t color = animation.ledActualColor(strip, i);
    //             strip->setPixelColor(i, color);
    //         }
    //         strip->show();
    //     });
    //     needAdaptStripBrightness = true;
    // }
}

void initStripBg() {
    StripStatus status;

    legacy = settings.getInt(LEGACY);
    
    if (settings.getInt(BG_LED_PIN) > 0 && settings.getInt(BG_LED_COUNT) > 0) {
        uint8_t ledType = settings.getInt(BG_LED_COLOR_FORMAT) + settings.getInt(BG_LED_FREQUENCY);
        LOG.printf("[LED] Initializing strip_bg on pin %d with %d LEDs, type %d (format:%d + freq:%d)\n", 
                   settings.getInt(BG_LED_PIN), settings.getInt(BG_LED_COUNT), ledType,
                   settings.getInt(BG_LED_COLOR_FORMAT), settings.getInt(BG_LED_FREQUENCY));
        status = led.createStrip(strip_bg, settings.getInt(BG_LED_PIN), settings.getInt(BG_LED_COUNT), 0, DefaultColors::OFF, ledType);
        if (status != StripStatus::SUCCESS) {
            LOG.printf("[LED] ERROR: Failed to create strip_bg: %d\n", status);
        } else {
            LOG.println("[LED] SUCCESS: strip_bg");
        }
    } else {
        LOG.println("[LED] SKIP: strip_bg (pin <= 0 or count <= 0)");
    }
    // if (strip_bg != nullptr) {
    //     // Спочатку встановлюємо LEDs з мінімальною яскравістю
    //     animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
    //         strip->setBrightness(led.brightnessMapped(0));
    //         uint32_t color = animation.stripActualColor(strip);
    //         for (int i = 0; i < strip->numPixels(); i++) {
    //             strip->setPixelColor(i, color);
    //         }
    //         strip->show();
    //     });
    //     needAdaptStripBrightness = true;      
    // }
}

void initStripService() {
    StripStatus status;

    legacy = settings.getInt(LEGACY);
    
    if (settings.getInt(SERVICE_LED_PIN) > 0) {
        uint8_t ledType = settings.getInt(SERVICE_LED_COLOR_FORMAT) + settings.getInt(SERVICE_LED_FREQUENCY);
        LOG.printf("[LED] Initializing strip_service on pin %d with %d LEDs, type %d (format:%d + freq:%d)\n", 
                   settings.getInt(SERVICE_LED_PIN), num_leds_service, ledType,
                   settings.getInt(SERVICE_LED_COLOR_FORMAT), settings.getInt(SERVICE_LED_FREQUENCY));
        status = led.createStrip(strip_service, settings.getInt(SERVICE_LED_PIN), num_leds_service, 0, DefaultColors::OFF, ledType);
        if (status != StripStatus::SUCCESS) {
            LOG.printf("[LED] ERROR: Failed to create strip_service: %d\n", status);
        } else {
            LOG.println("[LED] SUCCESS: strip_service");
        }
    } else {
        LOG.println("[LED] SKIP: strip_service (pin <= 0)");
    }
    // if (strip_service != nullptr) {
    //     // Спочатку встановлюємо LEDs з мінімальною яскравістю
    //     // animation.safeStripOperation(strip_service, [](Adafruit_NeoPixel* strip) {
    //     //     strip->setBrightness(led.brightnessMapped(0));
    //     //     for(int i = 0; i < strip->numPixels(); i++) {
    //     //         uint32_t color = animation.ledActualColor(strip, i);
    //     //         strip->setPixelColor(i, color);
    //     //     }
    //     //     strip->show();
    //     // });
    //     needAdaptStripBrightness = true;
    //     servicePin(POWER);
    // }
}

void reconnectStripMain() {
    LOG.println("[LED] Reconnecting strip_main with new pin configuration...");
    //analyzeMemoryFragmentation("before strip_main reconnection");
    animation.clearAllAnimations();
    logMemoryUsage("after clearing animations");
    //defragmentMemory("before strip_main cleanup");
    cleanupSingleStrip(strip_main, DefaultColors::OFF);
    //defragmentMemory("after strip_main cleanup");
    led.verifyStripCleanup(strip_main);
    //analyzeMemoryFragmentation("before strip_main recreation");
    initStripMain();
    LOG.println("[LED] strip_main reconnection completed.");
    logMemoryUsage("after strip_main reconnection complete");
}

void reconnectStripBg() {
    LOG.println("[LED] Reconnecting strip_bg with new pin configuration...");
    //analyzeMemoryFragmentation("before strip_bg reconnection");
    animation.clearAllAnimations();
    logMemoryUsage("after clearing animations");
    //defragmentMemory("before strip_bg cleanup");
    cleanupSingleStrip(strip_bg, DefaultColors::OFF);
    //defragmentMemory("after strip_bg cleanup");
    led.verifyStripCleanup(strip_bg);
    //analyzeMemoryFragmentation("before strip_bg recreation");
    initStripBg();
    LOG.println("[LED] strip_bg reconnection completed.");
    logMemoryUsage("after strip_bg reconnection complete");
}

void reconnectStripService() {
    LOG.println("[LED] Reconnecting strip_service with new pin configuration...");
    //analyzeMemoryFragmentation("before strip_service reconnection");
    animation.clearAllAnimations();
    logMemoryUsage("after clearing animations");
    //defragmentMemory("before strip_service cleanup");
    cleanupSingleStrip(strip_service, DefaultColors::OFF);
    //defragmentMemory("after strip_service cleanup");
    led.verifyStripCleanup(strip_service);
    //analyzeMemoryFragmentation("before strip_service recreation");
    initStripService();
    LOG.println("[LED] strip_service reconnection completed.");
    logMemoryUsage("after strip_service reconnection complete");
}

void reconnectStrips() {
    LOG.println("[LED] Reconnecting strip");
    
    if (needReconnectMainStrip) {
        reconnectStripMain();
        needReconnectMainStrip = false;
    }
    if (needReconnectBgStrip) {
        reconnectStripBg();
        needReconnectBgStrip = false;
    }
    if (needReconnectServiceStrip) {
        reconnectStripService();
        needReconnectServiceStrip = false;
    }
    needAdaptStripBrightness = true;
    
    // Оновлюємо посилання в веб-інтерфейсі
    web.begin(strip_main, strip_bg, strip_service);
    
    LOG.println("[LED] Strip reconnection completed.");
    logMemoryUsage("after strip reconnection complete");
}

// --- TIME Functions ---

static void printNtpStatus(NTPtime* timeClient) {
  LOG.print("[TIME] NTP status: ");
    switch (timeClient->NTPstatus()) {
        case 0:
            LOG.println("[TIME] OK");
            LOG.print("[TIME] Current date and time: ");
            LOG.println(timeClient->unixToString("DD.MM.YYYY hh:mm:ss"));
            break;
        case 1:
            LOG.println("[TIME] NOT_STARTED");
            break;
        case 2:
            LOG.println("[TIME] NOT_CONNECTED_WIFI");
            break;
        case 3:
            LOG.println("[TIME] NOT_CONNECTED_TO_SERVER");
            break;
        case 4:
            LOG.println("[TIME] NOT_SENT_PACKET");
            break;
        case 5:
            LOG.println("[TIME] WAITING_REPLY");
            break;
        case 6:
            LOG.println("[TIME] TIMEOUT");
            break;
        case 7:
            LOG.println("[TIME] REPLY_ERROR");
            break;
        case 8:
            LOG.println("[TIME] NOT_CONNECTED_ETHERNET");
            break;
        default:
            LOG.println("[TIME] UNKNOWN_STATUS");
            break;
    }
}

void syncTime(int8_t attempts) {
    timeClient.tick();
    if (timeClient.status() == UNIX_OK) return;
    LOG.println("[TIME] Time not synced yet!");
    printNtpStatus(&timeClient);
    int8_t count = 1;
    while (timeClient.NTPstatus() != NTP_OK && count <= attempts) {
        LOG.printf("[TIME] Attempt #%d of %d\n", count, attempts);
        if (timeClient.NTPstatus() != NTP_WAITING_REPLY) {
        LOG.println("[TIME] Force update!");
        timeClient.updateNow();
        }
        timeClient.tick();
        if (count < attempts) delay(1000);
        count++;
        printNtpStatus(&timeClient);
    }
}

bool isItNightNow() {
    int dayStart = settings.getInt(DAY_START);
    int nightStart = settings.getInt(NIGHT_START);
    // if day and night start time is equels it means it's always day, return day
    if (dayStart == nightStart) return false;

    int currentHour = timeClient.hour();

    // handle case, when night start hour is bigger than day start hour, ex. night start at 22 and day start at 9
    if (nightStart > dayStart) return currentHour >= nightStart || currentHour < dayStart ? true : false;

    // handle case, when day start hour is bigger than night start hour, ex. night start at 1 and day start at 8
    return currentHour < dayStart && currentHour >= nightStart ? true : false;
}

uint8_t getCurrentBrightnes() {
    // if auto brightness set to day/night mode, check current hour and choose brightness
    if (settings.getInt(BRIGHTNESS_MODE) == 0) {
        return settings.getInt(BRIGHTNESS);
    }
    if (settings.getInt(BRIGHTNESS_MODE) == 1) {
        return isItNightNow() ? settings.getInt(BRIGHTNESS_NIGHT) : settings.getInt(BRIGHTNESS_DAY);
    }
    return settings.getInt(BRIGHTNESS);
}

// --- INIT Functions ---

void initSettings() {
    LOG.println("[INIT] Init settings");
    settings.init();
    firmware = parseFirmwareVersion(VERSION);
    LOG.printf("[INIT] major: %d, minor: %d, patch: %d, isBeta: %d, betaBuild: %d\n",
            firmware.major, firmware.minor, firmware.patch, firmware.isBeta, firmware.betaBuild);
    fillFwVersion(currentFwVersion, firmware);
    LOG.printf("[INIT] Current firmware version: %s\n", currentFwVersion);

    legacy = settings.getInt(LEGACY);
    num_leds_main = 26; // Default value

    if (legacy == LEGACY::JAAM_3_0) {
        num_leds_main = 273;
        settings.saveInt(DISPLAY_MODEL, static_cast<int>(JaamDisplayType::SH1106G));
        settings.saveInt(DISPLAY_HEIGHT, static_cast<int>(JaamDisplayHeight::HEIGHT_64));
    } else if (legacy == LEGACY::JAAM_2_1) {
        settings.saveInt(DISPLAY_MODEL, static_cast<int>(JaamDisplayType::SH1106G));
        settings.saveInt(DISPLAY_HEIGHT, static_cast<int>(JaamDisplayHeight::HEIGHT_64));
    } else if (legacy == LEGACY::JAAM_1_3) {
        settings.saveInt(DISPLAY_MODEL, static_cast<int>(JaamDisplayType::SSD1306));
        settings.saveInt(DISPLAY_HEIGHT, static_cast<int>(JaamDisplayHeight::HEIGHT_64));
    }

    // Заповнюємо allLedsMain згідно з num_leds_main
    allLedsMain.clear();
    for (uint32_t i = 0; i < num_leds_main; ++i) {
        allLedsMain.push_back(i);
    }
    allLedsBg.clear();
    for (uint32_t i = 0; i < settings.getInt(BG_LED_COUNT); ++i) {
        allLedsBg.push_back(i);
    }
}

void initDisplay() {
    LOG.println("[INIT] Init display");
    display.begin(static_cast<JaamDisplayType>(settings.getInt(DISPLAY_MODEL)), static_cast<JaamDisplayHeight>(settings.getInt(DISPLAY_HEIGHT)));
    display.drawIconWithText(JaamDisplayIcon::TRIDENT, "Jaam Fusion v" + String(VERSION) + " Слава Україні!");
}

void initMapping() {
    LOG.println("[INIT] Init mapping");
    // Ініціалізуємо мапінг регіонів
    generateCustomRegionMap();
}


void initChipID() {
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(chipID, "%04x%04x", (uint32_t)(chipid >> 32), (uint32_t)chipid);
  LOG.printf("[INIT] ChipID Inited: '%s'\n", chipID);
}

void initTime() {
    static bool timeInitialized = false;
    if (timeInitialized) {
        LOG.println("[TIME] Time already initialized, skipping...");
        return;
    }
    LOG.println("[TIME] Init time");
    LOG.printf("[TIME] NTP host: %s\n", settings.getString(NTP_HOST));
    timeClient.setHost(settings.getString(NTP_HOST));
    timeClient.setTimeZone(settings.getInt(TIME_ZONE));
    timeClient.setDSTauto(&dst); // auto update on summer/winter time.
    timeClient.setTimeout(5000); // 5 seconds waiting for reply
    timeClient.begin();
    syncTime(7);
    timeInitialized = true;
}

void initWeb() {
    static bool webInitialized = false;
    if (webInitialized) {
        LOG.println("[WEB] Web already initialized, skipping...");
        return;
    }
    web.begin(strip_main, strip_bg, strip_service);
    web.setSettings(&settings);
    webInitialized = true;
}

static void wifiEvents(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED: {
            char softApIp[16];
            strcpy(softApIp, WiFi.softAPIP().toString().c_str());
            //displayMessage(softApIp, "Введіть у браузері:");
            WiFi.removeEvent(wifiEvents);
            break;
        }
        default:
            break;
    }
}

void apCallback(WiFiManager* wifiManager) {
    const char* message = wifiManager->getConfigPortalSSID().c_str();
    //displayMessage(message, "Підключіться до WiFi:");
    WiFi.onEvent(wifiEvents);
}

void saveConfigCallback() {
    //showServiceMessage(wm.getWiFiSSID(true).c_str(), "Збережено AP:");
    delay(2000);
    rebootDevice();
    }

void initWifi() {
    if (!WiFiConfig::ENABLED) {
        LOG.println("[WIFI] WiFi disabled in configuration");
        return;
    }

    LOG.println("[WIFI] Initializing WiFi...");
    wifiConnected = false;
    servicePin(WIFI);

    // Очищення старих з'єднань
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    // Встановлюємо режим станції
    WiFi.mode(WIFI_STA);
    wm.setHostname(settings.getString(BROADCAST_NAME));
    wm.setTitle(settings.getString(DEVICE_NAME));
    wm.setConfigPortalBlocking(true);
    wm.setConnectTimeout(WiFiConfig::CONNECT_TIMEOUT);
    wm.setConnectRetries(WiFiConfig::CONNECT_RETRIES);
    wm.setAPCallback(apCallback);
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setConfigPortalTimeout(WiFiConfig::PORTAL_TIMEOUT);

    char apssid[32];
    snprintf(apssid, sizeof(apssid), "JAAM_FUSION_%s", chipID);
    if (!wm.autoConnect(apssid)) {
        LOG.println("[WIFI] Reboot");
        rebootDevice(5000);
        return;
    }
    // Connected to WiFi
    LOG.println("[WIFI] connected...yeey :)");
    lastWifiConnectTime = millis();
    wifiConnected = true;
    servicePin(WIFI);
    //showServiceMessage("Підключено до WiFi!");
    wm.setHttpPort(WiFiConfig::WEB_PORT);
    wm.startWebPortal();
    initTime();
    initWeb();
    
    // // Спочатку спробуємо підключитися до збереженої мережі без WiFiManager
    // WiFi.begin();
    
    // // Чекаємо підключення 10 секунд
    // int attempts = 0;
    // while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    //     delay(500);
    //     attempts++;
    //     LOG.print(".");
    // }
    
    // if (WiFi.status() == WL_CONNECTED) {
    //     lastWifiConnectTime = millis();
    //     wifiConnected = true;
    //     servicePin(WIFI);
    //     initTime();
    //     initWeb();
    //     LOG.println("\n[WIFI] Connected to saved WiFi");
    //     LOG.printf("[WIFI] IP address: %s\n", WiFi.localIP().toString().c_str());
    //     return;
    // }
    
    // LOG.println("\n[WIFI] Failed to connect to saved network");
    // LOG.println("[WIFI] Starting WiFiManager...");
    
    // // Create local WiFiManager to avoid global memory usage
    // {
    //     WiFiManager wm_temp;
        
    //     // Мінімальні налаштування для економії пам'яті
    //     wm_temp.setHostname("jaam_fusion");
    //     wm_temp.setConfigPortalBlocking(true);
    //     wm_temp.setConnectTimeout(15);
    //     wm_temp.setConnectRetries(2);
    //     wm_temp.setConfigPortalTimeout(120);
        
    //     // Простий колбек без додаткових операцій
    //     wm_temp.setAPCallback([](WiFiManager* myWiFiManager) {
    //         LOG.printf("[WIFI] Connect to WiFi: %s\n", myWiFiManager->getConfigPortalSSID().c_str());
    //     });
        
    //     // Створюємо ім'я AP з chip ID
    //     char apName[32];
    //     snprintf(apName, sizeof(apName), "JAAM_FUSION_%s", chipID);
        
    //     // Спроба підключення
    //     if (!wm_temp.autoConnect(apName)) {
    //         LOG.println("[WIFI] Failed to connect. Rebooting...");
    //         rebootDevice(3000);
    //         return;
    //     }
        
    //     lastWifiConnectTime = millis();
    //     LOG.println("[WIFI] Connected to WiFi via WiFiManager");
    //     LOG.printf("[WIFI] IP address: %s\n", WiFi.localIP().toString().c_str());
        
    // } // wm_temp destructor called here, freeing memory
    
    LOG.println("[WIFI] initialization completed");
    LOG.printf("[MEMORY] Free heap after WiFi init: %u bytes\n", ESP.getFreeHeap());
}

void initWebsocket() {
#if !defined(TEST_ANIMATION)
    socketConnect();
#endif
}

void initStrip() {
    // Створюємо м'ютекс для захисту доступу до стрічок
    stripMutex = xSemaphoreCreateMutex();
    if (stripMutex == NULL) {
        LOG.println("[ERROR] Failed to create stripMutex semaphore");
        return;
    }

    // Ініціалізуємо стрічки з бажаними пінами
    initStripMain();
    initStripBg();
    initStripService();   
    needAdaptStripBrightness = true;
}

void initBattery() {
    LOG.println("[BATTERY] Initializing battery monitoring...");
    // Ініціалізація моніторингу батареї
    battery.begin();
}

void initStorage() {
    LOG.println("[STORAGE] Initializing storage...");
    // Ініціалізація файлової системи
    if (storage.begin()) {
        storage.getStorageInfo();
        storage.getFilesInfo();
    }
}


// --- Cycle Functions ---

// --- FreeRTOS Task Functions ---
// void memoryTask(void* pvParameters) {
//     const TickType_t xDelay = pdMS_TO_TICKS(MEMORY_CHECK_INTERVAL);
//     while (true) {
//         memoryProcess();
//         vTaskDelay(xDelay);
//     }
// }

// void wifiReconnectTask(void* pvParameters) {
//     const TickType_t xDelay = pdMS_TO_TICKS(WIFI_CHECK_INTERVAL);
//     while (true) {
//         wifiProcess();
//         vTaskDelay(xDelay);
//     }
// }

// void websocketProcessTask(void* pvParameters) {
//     const TickType_t xDelay = pdMS_TO_TICKS(WEBSOCKET_CHECK_INTERVAL);
//     while (true) {
//         websocketProcess();
//         vTaskDelay(xDelay);
//     }
// }

void animations() {
    static int currentIdx = 0;
    int ledsIdx[1] = { currentIdx };
    uint8_t r = random(256), g = random(256), b = random(256);
    
    // Випадковий вибір стрічки
    Adafruit_NeoPixel* strip = nullptr;
    // int stripRand = random(0, 2);
    // switch(stripRand) {
    //     case 0:
    //         if (strip_main != nullptr) strip = strip_main;
    //         break;
    //     case 1:
    //         if (strip_bg != nullptr) strip = strip_bg;
    //         break;
    //     case 2:
    //         if (strip_service != nullptr) strip = strip_service;
    //         break;
    //     default:
    //         if (strip_main != nullptr) strip = strip_main;
    // }

    strip = strip_main;

    if (!strip) {
        LOG.println("[ERROR] No available initialized strips");
        return;
    }
    // r = 255;
    // g = 0;
    // b = 0;
    
    uint32_t color = strip->Color(r, g, b);

    // Випадковий вибір типу анімації
    
    int typeRand = random(0, 4); // 0, 1 або 2 для FADE, BLINK або BLEND_FADE
    switch(typeRand) {
        case 0:
            animType = AnimationParams::Type::FADE;
            break;
        case 1:
            animType = AnimationParams::Type::BLINK;
            break;
        case 2:
            animType = AnimationParams::Type::BLEND_FADE;
            break;
        case 3:
            animType = AnimationParams::Type::PULSE;
            break;
        default:
            animType = AnimationParams::Type::FADE;
    }
    //animType = AnimationParams::Type::BLEND_FADE;

    // Випадкові параметри для анімації з використанням конфігурації
    uint32_t period = random(AnimationConfig::MIN_PERIOD, AnimationConfig::MAX_PERIOD + 1);
    uint32_t cycles = 30; // random(AnimationConfig::MIN_CYCLES, AnimationConfig::MAX_CYCLES + 1);
    uint8_t startBrightness = 50; // random(AnimationConfig::MIN_START_BRIGHTNESS, AnimationConfig::MAX_START_BRIGHTNESS + 1);
    uint8_t endBrightness = 255; //   random(AnimationConfig::MIN_END_BRIGHTNESS, AnimationConfig::MAX_END_BRIGHTNESS + 1);
    
    // Створення анімації з обробкою помилок
    if (!animation.createAnimation(
        animType,
        strip,
        ledsIdx,
        1,
        color,
        color,
        period,
        cycles,
        startBrightness,
        endBrightness
    )) {
        LOG.println("ERROR: Failed to create animation");
        return;
    }

    currentIdx++;
    if (currentIdx >= num_leds_main) {
        currentIdx = 0;
    }
}

void showClock() {
    if (timeClient.status() != UNIX_OK) {
        LOG.println("[TIME] Clock not synced yet!");
        return;
    }
    needDivider = !needDivider; // toggle divider for clock display
    String time;
    if (needDivider) {
        time = timeClient.unixToString("hh:mm");
    } else {
        time = timeClient.unixToString("hh mm");
    }
    String date = timeClient.unixToString("DSTRUA DD.MM.YYYY");
    display.printClock(time, date);
}

void websocketProcess() {
    if (!wifiConnected) {
        LOG.println("[WEBSOCKET] Reconnecting... wifiConnected == false");
        return;
    }
    //if (millis() - websocketLastPingTime > 30000 && !websocketReconnect) {
    if (millis() - websocketLastPingTime > settings.getInt(WS_ALERT_TIME) && !websocketReconnect) {
        LOG.println("[WEBSOCKET] websocketReconnect = true; Reason: no ping/pong from server (WS_ALERT_TIME)");
        websocketReconnect = true;
        isFirstDataFetchCompleted = false;
        clearAllAlertsMaps();
        animation.clearAllAnimations();
        //int positions[] = {}; // not used in RUNNING_LIGHT
        animation.createAnimation(
            AnimationParams::Type::RUNNING_LIGHT,
            strip_main,         
            {}, // not used in RUNNING_LIGHT        
            0,            
            0xFF0000,      
            0x001100,        
            1000,
            180,
            50,
            200
        );
    }
    //if (millis() - websocketLastPingTime > 40000 && websocketReconnect) {
    if (millis() - websocketLastPingTime > settings.getInt(WS_REBOOT_TIME) && websocketReconnect) {
        LOG.println("[WEBSOCKET] websocketReconnect = true; Reason: WS_REBOOT_TIME exceeded, will reboot");
        rebootDevice(3000, true);
    }
    if (!websocket.available()) {
        LOG.println("[WEBSOCKET] Reconnecting... websocket.available() == false");
        isFirstDataFetchCompleted = false;
        apiConnected = false;
        socketConnect();
    }
    if (websocketReconnect) {
        LOG.println("[WEBSOCKET] Reconnecting... websocketReconnect == true");
        isFirstDataFetchCompleted = false;
        apiConnected = false;
        socketConnect();
    }
}

void memoryProcess() {
    static uint32_t loopCount = 1;
    size_t freeHeap    = ESP.getFreeHeap();
    size_t usedHeap    = ESP.getHeapSize() - freeHeap;
    size_t maxAlloc    = ESP.getMaxAllocHeap();
    uint32_t uptimeMin = millis() / 60000;

    // WiFi status information
    //bool wifiConnected = WiFi.status() == WL_CONNECTED;
    uint32_t wifiUptime = wifiConnected ? (millis() - lastWifiConnectTime) / 60000 : 0; 
    uint32_t websocketUptime = websocket.available() ? (millis() - lastWebsocketConnectTime) / 60000 : 0; // in seconds
    String wifiStatus = wifiConnected ? "Connected" : "Disconnected";
    String websocketStatus = websocket.available() ? "Connected" : "Disconnected";
    String wifiIP = wifiConnected ? WiFi.localIP().toString() : "N/A";

    LOG.printf(
        "[DEBUG] Loop %u: used heap %u B, free heap %u B, uptime %u min. WiFi: %s, %u min. WebSocket: %s, %u min\n",
        loopCount,
        usedHeap,
        freeHeap,
        uptimeMin,
        wifiStatus.c_str(),
        wifiUptime,
        websocketStatus.c_str(),
        websocketUptime
    );
    loopCount++;
}

// перевірка статусу wifi
// Якщо статус не WL_CONNECTED, то перепідключаємося
void wifiProcess() {
    static uint8_t reconnectAttempts = 0;
    const uint8_t MAX_RECONNECT_ATTEMPTS = 5;
    
    if (WiFi.status() != WL_CONNECTED) {
        if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
            LOG.println("[WIFI] Reconnect");
            wifiConnected = false;
            initWifi();
            reconnectAttempts++;
        } else {
            LOG.println("[WIFI] Max reconnect attempts reached, rebooting...");
            rebootDevice(3000);
            reconnectAttempts = 0;
        }
    } else {
        reconnectAttempts = 0;
    }
}

// Синхронізація часу
void timeProcess() {
    syncTime(2);
}

// Виводимо інформацію про активні анімації
void animationsLog() {
  
  animation.logActiveAnimations();
}

// Виводимо інформацію про леди без анімацій
void freeMinLedsLog() {
    if (strip_main == nullptr) {
        LOG.println("[LED] strip_main not initialized");
        return;
    }
    auto freeLeds = animation.getFreeLeds(strip_main, num_leds_main);
    LOG.printf("[LED] Free LEDs on strip_main: %d\n", (int)freeLeds.size());
    LOG.print("[LED] List: ");
    for (const auto& led : freeLeds) {
        LOG.printf("%d ", led.ledIdx);
    }
    LOG.println();
}

void mainThreadProcess() {
    // Ця функція виконується в основному циклі
    // Вона потрібна для асинхронного менеджера, щоб мати можливість виконувати інші задачі

    if (needReconnectMainStrip || needReconnectBgStrip || needReconnectServiceStrip) {
        LOG.println("[MAIN] Reconnecting LED strip");
        reconnectStrips();
    }

    if (needReconnectWebsocket && !needReconnectMainStrip) {
        LOG.println("[MAIN] Reconnecting WebSocket");
        isFirstDataFetchCompleted = false;
        needReconnectWebsocket = false;
        apiConnected = false;
        socketConnect();
    }

    if (needRecalculateLeds) {
        generateCustomRegionMap();
        LOG.println("[MAIN] Recalculating LEDs");
        needRecalculateLeds = false;
        needReconnectWebsocket = true;
    }

    if (needAdaptColors) {
        if (strip_main != nullptr) {
            LOG.printf("[WEB] Adjusting main colors\n");               
            animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
                for (int i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
        if (strip_bg != nullptr) {
            LOG.printf("[WEB] Adjusting bg colors\n");
            animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
                uint32_t color = animation.stripActualColor(strip);
                for (int i = 0; i < strip->numPixels(); i++) {
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });               
        }
        if (strip_service != nullptr) {
            LOG.printf("[WEB] Adjusting service colors\n");
            animation.safeStripOperation(strip_service, [](Adafruit_NeoPixel* strip) {
                for(uint16_t i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });               
        }
        needAdaptColors = false;
    }

    if (needAdaptAnimationColors) {
        animation.adaptAllAnimationColors();
        needAdaptAnimationColors = false;
    }
    if (needAdaptStripBrightness) {
        needAdaptStripBrightness = false;
        if (strip_main != nullptr) {
            animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
                strip->setBrightness(led.brightnessMapped(settings.getInt(CURRENT_BRIGHTNESS)));
                for(uint32_t i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
        if (strip_bg != nullptr) {
            animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
                strip->setBrightness(led.brightnessMapped(settings.getInt(CURRENT_BRIGHTNESS)));
                uint32_t color = animation.stripActualColor(strip);
                for (uint32_t i = 0; i < strip->numPixels(); i++) {
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
        if (strip_service != nullptr) {
            animation.safeStripOperation(strip_service, [](Adafruit_NeoPixel* strip) {
                strip->setBrightness(led.brightnessMapped(settings.getInt(CURRENT_BRIGHTNESS)));
                for(uint16_t i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
        // int ledsIdx[1] = { 0 };
        // if (!animation.createAnimation(
        //     AnimationParams::Type::SET_BRIGHTNESS,
        //     strip_main,
        //     ledsIdx,
        //     1,
        //     0x000000, // Колір не важливий для SET_BRIGHTNESS
        //     0x000000, // Початковий колір не важливий
        //     200,
        //     1,
        //     strip_main->getBrightness(),
        //     settings.getInt(BRIGHTNESS)
        // )) {
        //     LOG.println("[ERROR] Failed to create animation");
        //     return;
        // }
    }
    if (needUpdateBatteryPin) {
        LOG.println("[MAIN] Updating battery pin state");
        if (settings.getInt(BATTERY_PIN) > 0) {
            battery.setPin(settings.getInt(BATTERY_PIN));
        } else {
            LOG.println("[MAIN] Battery pin not configured, skipping update");
        }
        needUpdateBatteryPin = false;
    }

    if (needReconfigureDisplay) {
        LOG.println("[MAIN] Reconfiguring display");
        display.begin(static_cast<JaamDisplayType>(settings.getInt(DISPLAY_MODEL)), static_cast<JaamDisplayHeight>(settings.getInt(DISPLAY_HEIGHT)));
        needReconfigureDisplay = false;
    }
}

void brightnessProcess() {
    // if auto brightness set to day/night mode, check current hour and choose brightness
    uint8_t currentBrightness = getCurrentBrightnes();
    if (settings.getInt(CURRENT_BRIGHTNESS) != currentBrightness) {
        settings.saveInt(CURRENT_BRIGHTNESS, currentBrightness);
        LOG.printf("[BRIGHTNESS] Setting current_brightness to %d\n", currentBrightness);
        if (strip_main != nullptr) {
            animation.safeStripOperation(strip_main, [currentBrightness](Adafruit_NeoPixel* strip) {
                strip->setBrightness(led.brightnessMapped(currentBrightness));
                for(uint32_t i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
        if (strip_bg != nullptr) {
            animation.safeStripOperation(strip_bg, [currentBrightness](Adafruit_NeoPixel* strip) {
                strip->setBrightness(led.brightnessMapped(currentBrightness));
                uint32_t color = animation.stripActualColor(strip);
                for(int i = 0; i < strip->numPixels(); i++) {
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
        if (strip_service != nullptr) {
            animation.safeStripOperation(strip_service, [currentBrightness](Adafruit_NeoPixel* strip) {
                strip->setBrightness(led.brightnessMapped(currentBrightness));
                for(uint16_t i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
    }
}

// --- Battery Monitor Process ---
void batteryProcess() {
    battery.logVoltage();
}

// --- Display Process ---
void displayProcess() {
    showClock();
}

// --- SETUP ---
void setup() {
    LOG.begin(115200);
    delay(2000);

    checkFreeHeap("LOG initialization");

    initStorage();
    checkFreeHeap("SPIFFS initialization");

    initChipID();
    checkFreeHeap("chipID initialization");

    initSettings();
    checkFreeHeap("settings initialization");

    initDisplay();
    checkFreeHeap("display initialization");

    initMapping();
    checkFreeHeap("LED mapping initialization");

    initStrip();
    checkFreeHeap("LED strips initialization");

    initBattery();
    checkFreeHeap("battery initialization");

    // initWifi();
    // checkFreeHeap("WiFi initialization");

    // initWebsocket();
    // checkFreeHeap("WebSocket initialization");

    // initTime();
    // checkFreeHeap("time initialization");

    // Ініціалізація веб-інтерфейсу
    //initWeb();
    //web.begin(strip_main, strip_bg, strip_service);
    //web.setSettings(&settings);

    // Ініціалізуємо генератор випадкових чисел
    randomSeed(esp_random());
    
    // Передаємо settings в AnimationManager
    animation.setSettings(&settings);
    led.setSettings(&settings);
    battery.setSettings(&settings);
    checkFreeHeap("animation and LED settings");
    
    // Налаштовуємо асинхронні задачі
#if defined(TEST_ANIMATION)
    async.setInterval(animations, ANIMATION_INTERVAL);
#endif
    //async.setInterval(animationsLog, 1000);
    async.setInterval(memoryProcess, MEMORY_CHECK_INTERVAL);
    async.setInterval(wifiProcess, WIFI_CHECK_INTERVAL);
#if !defined(TEST_ANIMATION)
    async.setInterval(websocketProcess, WEBSOCKET_CHECK_INTERVAL);;
#endif
    async.setInterval(timeProcess, TIME_CHECK_INTERVAL);
    async.setInterval(mainThreadProcess, MAIN_THREAD_CHECK_INTERVAL);
    async.setInterval(brightnessProcess, MAIN_THREAD_CHECK_INTERVAL);
    async.setInterval(batteryProcess, 10000); // кожні 10 секунд
    async.setInterval(displayProcess, 1000); // кожну секунду
    checkFreeHeap("async tasks configuration");
    
    checkFreeHeap("web interface initialization");
    
    LOG.println("[SETUP] Initialization complete");
    checkFreeHeap("full setup");
}

void loop() {
    wm.process();
#if !defined(TEST_ANIMATION)
    websocket.poll();
#endif
    animation.update();
    async.run();
    web.handleClient();
}