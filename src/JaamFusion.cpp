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
#include "JaamLogs.h"
#include "JaamConfig.h"
#include "JaamSettings.h"
#include "JaamWeb.h"
#include "JaamLed.h"
#include "JaamUtils.h"

using namespace websockets;

// --- MAIN Configuration ---
char                chipID[13];
char                currentFwVersion[25];
int                 currentIdx = 0;
uint32_t            loopCount = 0;

Async               async = Async(20);

NTPtime             timeClient(2);
DSTime              dst(3, 0, 7, 3, 10, 0, 7, 4); //https://en.wikipedia.org/wiki/Eastern_European_Summer_Time

JaamSettings        settings;
JaamFirmware        firmware;
JaamWeb             web;
JaamLed             led;

// --- LED Configuration ---
Adafruit_NeoPixel*  strip_main = nullptr;
Adafruit_NeoPixel*  strip_bg = nullptr;
Adafruit_NeoPixel*  strip_service = nullptr;
SemaphoreHandle_t   stripMutex = nullptr;
uint16_t            num_leds_main = 26;
uint16_t            num_leds_service = 5;
uint8_t             currentBrightness = 0;
std::vector<int>    allLedsMain;
std::vector<int>    allLedsBg;
bool                strip_main_initialized = false;
bool                strip_bg_initialized = false;
bool                strip_service_initialized = false;

// --- ANIMATION Configuration ---
AnimationManager        animation;
AnimationParams::Type   animType;

// --- MAP Configuration ---
std::map<uint16_t, uint16_t>    alertsMap;

// --- TASKS Configuration ---
bool                needAdaptAnimationColors = false;
bool                needAdaptStripBrightness = false;
bool                needAdaptStripMainBrightness = false;
bool                needAdaptStripBgBrightness = false;
bool                needAdaptStripServiceBrightness = false;
bool                needToReconnectWebsocket = false;
bool                needAdaptColors = false;
bool                needReconnectStrips = false;
bool                needReconnectMainStrip;
bool                needReconnectBgStrip;
bool                needReconnectServiceStrip;

// --- WIFI Configuration ---
WiFiManager         wm;
WebsocketsClient    websocket;
uint32_t            lastWifiConnectTime = 0;  // Track when WiFi was last connected
uint16_t            alertsHash = 0;

time_t              websocketLastPingTime = 0;
bool                isFirstDataFetchCompleted = false;
bool                apiConnected;
uint8_t             legacy = 4;
bool                websocketReconnect = false;
uint32_t            lastWebsocketConnectTime = 0;
size_t              lastUsedHeap = 0; 


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

// Функція для розрахунку diff
AlertDiff calculateAlertDiff(uint16_t region_id, uint16_t previous_flags, uint16_t current_flags) {
    AlertDiff diff;
    diff.region_id = region_id;
    diff.previous_flags = previous_flags;
    diff.current_flags = current_flags;
    diff.has_changes = (previous_flags != current_flags);
    return diff;
}

void animateLed(int led_position, int bit, uint16_t region_id, bool increase = true) {
    LOG.printf("[ANIMATION] LED %d: region %d to %d\n", led_position, region_id, bit); 
    
    uint32_t color;
    uint32_t initialColor = 0x000000; // Початковий колір для анімації
    uint32_t period;
    uint32_t cycles;
    uint8_t startBrightness = 50;
    uint8_t endBrightness = 255;
    uint8_t ledCount;

    switch (bit) {
        case -1:
            initialColor = strip_main->getPixelColor(led_position);  
            color = animation.ledActualColor(strip_main, led_position, true);               
            animType = AnimationParams::Type::ONE_WAY_BLEND_FADE;
            cycles = 1;
            period = 3000;//10000;
            break;
        case 0:
            color = (increase) ? animation.colorFromHex(settings.getString(COLOR_NEW_ALERT)) : animation.colorFromHex(settings.getString(COLOR_ALERT)); 
            endBrightness = (increase) ? led.brightnessAbsolute(settings.getInt(BRIGHTNESS_NEW_ALERT)) : led.brightnessAbsolute(settings.getInt(BRIGHTNESS_ALERT));
            animType = (increase) ? AnimationParams::Type::FADE : AnimationParams::Type::ONE_WAY_BLEND_FADE;
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
            LOG.printf("[ANIMATION] LED %d: unknown bit\n", led_position);
            return;
    }
    int ledsIdx[1] = { led_position };
    if (!animation.createAnimation(
        animType,
        strip_main,
        ledsIdx,
        1,
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

    bool animate = false;
    uint32_t color;
    uint32_t initialColor = 0x000000; // Початковий колір для анімації
    uint32_t period;
    uint32_t cycles;
    uint8_t startBrightness = 50;
    uint8_t endBrightness = 255;
    uint8_t ledCount;
    size_t bodyLen;

    if(type == TYPE_NOTIFICATIONS_BATCH) {
        LOG.printf("[WEBSOCKET] TYPE_NOTIFICATIONS_BATCH received\n");
        bodyLen = len - HEADER_SZ;

        // payloadLen має ділитися на RECORD_SZ
        if (bodyLen == 0 || (bodyLen % RECORD_SZ) != 0) {
            // некоректний фрейм — пропускаємо і реконнектимось
            LOG.printf("[ERROR] bodyLen == 0 || (bodyLen % RECORD_SZ\n");
            needToReconnectWebsocket = true;
            return;
        }

        // Обчислюємо кількість записів
        size_t count = bodyLen / RECORD_SZ;

        // Розбираємо count записів по RECORD_SZ
        const uint8_t* ptr = data + HEADER_SZ;

        LOG.printf("[WEBSOCKET] notification data processing\n");

        bool missiles, kab, drones, recon, explosion;
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

            drones          = flags16 & (1 << 5);
            missiles        = flags16 & (1 << 6);
            kab             = flags16 & (1 << 7);
            explosion       = flags16 & (1 << 8);
            recon           = flags16 & (1 << 9);

            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_EXPLOSION));
            endBrightness = 50;
            animType = AnimationParams::Type::FADE;
            period = 500;
            cycles = 360;

            if ((drones || recon) && settings.getBool(ENABLE_DRONES)) {
                animate = true;
                color = animation.colorFromHex(settings.getString(COLOR_DRONES));
            }
            if (missiles && settings.getBool(ENABLE_MISSILES)) {
                animate = true;
                color = animation.colorFromHex(settings.getString(COLOR_MISSILES));
            }
            if (kab && settings.getBool(ENABLE_KABS)) {
                animate = true;
                color = animation.colorFromHex(settings.getString(COLOR_KABS));
            }
            if (explosion && settings.getBool(ENABLE_EXPLOSIONS)) {
                animate = true;
                color = animation.colorFromHex(settings.getString(COLOR_EXPLOSION));
            }
            
            if(animate) {
                if (strip_main_initialized) {
                    for (int i = 0; i < ledCount; ++i) {
                        int ledsIdx[1] = { leds[i] };
                        if (!animation.createAnimation(
                            animType,
                            strip_main,
                            ledsIdx,
                            1,
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
                }
            }
        }
    }

    if(type == TYPE_ALERTS_BATCH) {
        //LOG.printf("[WEBSOCKET] TYPE_ALERTS_BATCH received\n");
        // Обчислюємо довжину payload після заголовка
        bodyLen = len - HEADER_SZ - HASH_SZ;

        // payloadLen має ділитися на RECORD_SZ
        if (bodyLen == 0 || (bodyLen % RECORD_SZ) != 0) {
            // некоректний фрейм — пропускаємо і реконнектимось
            LOG.printf("[ERROR] bodyLen == 0 || (bodyLen % RECORD_SZ\n");
            needToReconnectWebsocket = true;
            return;
        }

        // Перевіряємо  хеш
        uint16_t actualHash = (static_cast<uint16_t>(data[1]) << 8) | data[2];
        uint16_t prevHash = (static_cast<uint16_t>(data[3]) << 8) | data[4];
        //LOG.printf("[WEBSOCKET] hash check, local: [0x%04X] prev: [0x%04X], actual: [0x%04X]\n", alertsHash, prevHash, actualHash);

        if (prevHash != alertsHash) {
            // некоректний хеш — пропускаємо і реконнектимось
            LOG.printf("[ERROR] prevHash != alertsHash\n");
            needToReconnectWebsocket = true;
            return;
        }

        // Обчислюємо кількість записів
        size_t count = bodyLen / RECORD_SZ;

        // Розбираємо count записів по RECORD_SZ
        const uint8_t* ptr = data + HEADER_SZ + HASH_SZ;

        //LOG.printf("[WEBSOCKET] alerts data processing\n");

        std::vector<AlertDiff> diffs;
        // Створюємо тимчасову мапу для нових значень
        std::map<uint16_t, uint16_t> alertsMapActual;


        bool air, artillery, urban, chemical, nuclear, missiles, kab, drones, ballistic, explosion;
        bool airPrevious, artilleryPrevious, urbanPrevious, chemicalPrevious, nuclearPrevious, missilesPrevious, kabPrevious, dronesPrevious, ballisticPrevious, explosionPrevious ;
        for (size_t i = 0; i < count; ++i) {
            uint16_t region_id = uint16_t(ptr[0]) | (uint16_t(ptr[1]) << 8);
            uint16_t flags16   = uint16_t(ptr[2]) | (uint16_t(ptr[3]) << 8);
            ptr += RECORD_SZ;

            // Отримуємо попередній стан
            uint16_t previous_flags = alertsMap[region_id];
            
            // Розраховуємо diff
            AlertDiff diff = calculateAlertDiff(region_id, previous_flags, flags16);
            if (diff.has_changes) {
                diffs.push_back(diff);
            }
            
            alertsMapActual[region_id] = flags16;
        }

        LOG.printf("[DEBUG] analyzeAlertChanges called with %d diffs\n", (int)diffs.size());
        LOG.printf("------------------------------------------------------------\n");
        LOG.println();

        // Мапа для зберігання найвищих бітів для LED
        struct LedBit {
            int highest_bit;
            uint16_t region_id;
        };
        std::map<int, LedBit> led_bits_diff;
        std::map<int, LedBit> led_bits_alerts;

        // Проходимо по всіх змінах
        for (const auto& diff : diffs) {
            // Показуємо зміни для цього регіону
            LOG.printf("[DIFF] Region %d: flags changed from 0x%04X to 0x%04X\n", 
                diff.region_id, diff.previous_flags, diff.current_flags);
            
            // Показуємо які біти змінилися
            for (int bit = 0; bit < ALERT_TYPES_COUNT; bit++) {
                bool prev_state = diff.previous_flags & (1 << bit);
                bool curr_state = diff.current_flags & (1 << bit);
                
                if (prev_state != curr_state) {
                    LOG.printf("[DIFF]   Bit %d (%s): %s -> %s\n", 
                        bit,
                        ALERT_TYPES[bit],
                        prev_state ? "ON" : "OFF",
                        curr_state ? "ON" : "OFF"
                    );
                }
            }

            // Знаходимо найвищий біт для цього регіону
            int highest_bit_diff = findHighestBit16(diff.current_flags);
            LOG.printf("[DIFF]   Highest bit for region %d: %d\n", 
                diff.region_id, highest_bit_diff);

            // Шукаємо LED для цього регіону
            const RegionLedMapEntry* entry = getRegionEntry(diff.region_id);
            if (entry) {
                LOG.printf("[DIFF]   LEDs for region %d: ", diff.region_id);
                for (uint8_t i = 0; i < entry->led_count; ++i) {
                    int led_position = entry->led_positions[i];
                    LOG.printf("%d ", led_position);

                    // Оновлюємо найвищий біт для LED
                    auto it = led_bits_diff.find(led_position);
                    if (it == led_bits_diff.end() || highest_bit_diff > it->second.highest_bit) {
                        led_bits_diff[led_position] = {highest_bit_diff, diff.region_id};
                    }
                }
                // Шукаємо всі леди для цього регіону
                for (uint8_t i = 0; i < entry->led_count; ++i) {
                    int led_position = entry->led_positions[i];
                    LOG.printf("\n[LED_ANALYSIS] LED %d regions: ", led_position);

                    // Шукаємо всі регіони для цього LED
                    const std::vector<uint16_t>& regions = getRegionsForLed(led_position);
                    for (uint16_t region_id : regions) {
                        LOG.printf("%d ", region_id);

                        // Шукаємо найвищий біт для регіону в alertsMap
                        auto alerts_it = alertsMap.find(region_id);
                        if (alerts_it != alertsMap.end()) {
                            int highest_bit_region = findHighestBit16(alerts_it->second);
                            LOG.printf("[%d] ", highest_bit_region);

                            // Оновлюємо найвищий біт для LED
                            auto it = led_bits_alerts.find(led_position);
                            if (it == led_bits_alerts.end() || highest_bit_region > it->second.highest_bit) {
                                led_bits_alerts[led_position] = {highest_bit_region, region_id};
                            }
                        }
                    }
                    LOG.println();
                }  
                LOG.println();
            } else {
                LOG.printf("[DIFF]   No LEDs found for region %d\n", diff.region_id);
            }
        }

        // Виводимо фінальний список бітів для LED
        LOG.printf("\n[FINAL] LED bits summary:\n");
        for (const auto& led : led_bits_diff) {
            LOG.printf("[FINAL] LED diff %d: highest_bit=%d, region_id=%d\n",
                led.first,
                led.second.highest_bit,
                led.second.region_id
            );
        }
        for (const auto& led : led_bits_alerts) {
            LOG.printf("[FINAL] LED alerts %d: highest_bit=%d, region_id=%d\n",
                led.first,
                led.second.highest_bit,
                led.second.region_id
            );
        }

        // вмерджуємо alertsMapActual в основний alertsMap
        for (const auto& pair : alertsMapActual) {
            alertsMap[pair.first] = pair.second;
        }

        if (isFirstDataFetchCompleted) {
            // Виводимо фінальний список бітів для LED
            LOG.printf("\n[FINAL] LED bits summary:\n");
            for (const auto& led : led_bits_diff) {
                // Порівнюємо з led_bits_alerts
                auto alerts_it = led_bits_alerts.find(led.first);
                if (alerts_it != led_bits_alerts.end()) {
                    int diff_bit = led.second.highest_bit;
                    int alerts_bit = alerts_it->second.highest_bit;

                    if (diff_bit > alerts_bit) {
                        LOG.printf("[ANIMATION] LED %d: increasing bit from %d to %d\n",
                            led.first, alerts_bit, diff_bit);
                            animateLed(led.first, diff_bit, led.second.region_id, true);
                    } else if (diff_bit < alerts_bit) {
                        LOG.printf("[ANIMATION] LED %d: decreasing bit from %d to %d\n",
                            led.first, alerts_bit, diff_bit);
                            animateLed(led.first, diff_bit, led.second.region_id, false);
                    }
                }
            }
        }
        LOG.printf("------------------------------------------------------------\n");
        
        alertsHash = actualHash;
    }
    if (!isFirstDataFetchCompleted) {
        LOG.printf("[WEBSOCKET] init processing\n");
        if (strip_main_initialized) {
            animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
                for(uint16_t i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
        if (strip_bg_initialized) {
            animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
                uint32_t color = animation.regionActualColor(settings.getInt(HOME_DISTRICT));
                for(uint16_t i = 0; i < strip->numPixels(); i++) {
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
        LOG.println("[WEBSOCKET] connnection opened");
        //servicePin(DATA, HIGH, false);
        websocketLastPingTime = millis();
        //ha.setMapApiConnect(apiConnected);
    } else if (event == WebsocketsEvent::ConnectionClosed) {
        apiConnected = false;
        LOG.println("[WEBSOCKET] connnection closed");
        isFirstDataFetchCompleted = false;
        LOG.printf("[MEMORY] Heap before close: %u\n", ESP.getFreeHeap());
        //websocket.close();
        auto reason = websocket.getCloseReason();
        logWebsocketCloseReason(reason);
        delay(500);
        LOG.printf("[MEMORY] Heap after close: %u\n", ESP.getFreeHeap());
        //servicePin(DATA, LOW, false);
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
    //showServiceMessage("підключення...", "Сервер даних");
    alertsHash = 0;
    websocket.onMessage(onMessageCallback);
    websocket.onEvent(onEventsCallback);
    long startTime = millis();
    char webSocketUrl[100];
    sprintf(
        webSocketUrl,
        "ws://%s:%d/data_fusion_v1",
        settings.getString(WS_SERVER_HOST), //"10.2.0.156",
        settings.getInt(WS_SERVER_PORT)
    );
    LOG.printf("[WEBSOCKET] url:%s\n", webSocketUrl);
    websocket.connect(webSocketUrl);
    if (websocket.available()) {
        
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
    if (strip_main_initialized) {
        animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
            strip->clear();
            // Встановлюємо дефолтний колір
            for(int i = 0; i < strip->numPixels(); i++) {
                strip->setPixelColor(i, DefaultColors::OFF);
            }
            strip->show();
            delete strip;
            strip_main = nullptr;
            strip_main_initialized = false;
        });
    }
    
    if (strip_bg_initialized) {
        animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
            strip->clear();
            // Встановлюємо дефолтний колір
            for(int i = 0; i < settings.getInt(BG_LED_COUNT); i++) {
                strip->setPixelColor(i, DefaultColors::OFF);
            }
            strip->show();
            delete strip;
            strip_bg = nullptr;
            strip_bg_initialized = false;
        });
    }
    
    if (strip_service_initialized) {
        animation.safeStripOperation(strip_service, [](Adafruit_NeoPixel* strip) {
            strip->clear();
            // Встановлюємо дефолтний колір
            for(int i = 0; i < num_leds_service; i++) {
                strip->setPixelColor(i, DefaultColors::OFF);
            }
            strip->show();
            delete strip;
            strip_service = nullptr;
            strip_service_initialized = false;
        });
    }
}

void cleanupSingleStrip(Adafruit_NeoPixel*& strip, bool& initialized, uint32_t defaultColor) {
    if (initialized && strip != nullptr) {
        animation.safeStripOperation(strip, [&strip, defaultColor](Adafruit_NeoPixel* stripPtr) {
            stripPtr->clear();
            // Встановлюємо дефолтний колір
            for(uint16_t i = 0; i < strip->numPixels(); i++) {
                stripPtr->setPixelColor(i, defaultColor);
            }
            stripPtr->show();
            delete stripPtr;
        });
        strip = nullptr;
        initialized = false;
    }
}

void initStripMain() {
    StripStatus status;
    
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
            strip_main_initialized = true;
        }
    }
    else {
        LOG.println("[LED] SKIP: strip_main (pin <= 0)");
    }
    if (strip_main_initialized) {
        // Спочатку встановлюємо LEDs з мінімальною яскравістю
        animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
            strip->setBrightness(led.brightnessMapped(0));
            for(uint16_t i = 0; i < strip->numPixels(); i++) {
                uint32_t color = animation.ledActualColor(strip, i);
                strip->setPixelColor(i, color);
            }
            strip->show();
        });
        needAdaptStripMainBrightness = true;
    }
}

void initStripBg() {
    StripStatus status;
    
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
            strip_bg_initialized = true;
        }
    } else {
        LOG.println("[LED] SKIP: strip_bg (pin <= 0 or count <= 0)");
    }
    if (strip_bg_initialized) {
        // Спочатку встановлюємо LEDs з мінімальною яскравістю
        animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
            strip->setBrightness(led.brightnessMapped(0));
            uint32_t color = animation.stripDefaultColor(strip);
            for (int i = 0; i < strip->numPixels(); i++) {
                strip->setPixelColor(i, color);
            }
            strip->show();
        });
        needAdaptStripBgBrightness = true;      
    }
}

void initStripService() {
    StripStatus status;
    
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
            strip_service_initialized = true;
        }
    } else {
        LOG.println("[LED] SKIP: strip_service (pin <= 0)");
    }
    if (strip_service_initialized) {
        // Спочатку встановлюємо LEDs з мінімальною яскравістю
        animation.safeStripOperation(strip_service, [](Adafruit_NeoPixel* strip) {
            strip->setBrightness(led.brightnessMapped(0));
            for(int i = 0; i < strip->numPixels(); i++) {
                uint32_t color = animation.ledActualColor(strip, i);
                strip->setPixelColor(i, color);
            }
            strip->show();
        });
        needAdaptStripServiceBrightness = true;
    }
}

void reconnectStripMain() {
    LOG.println("[LED] Reconnecting strip_main with new pin configuration...");
    //analyzeMemoryFragmentation("before strip_main reconnection");
    animation.clearAllAnimations();
    logMemoryUsage("after clearing animations");
    //defragmentMemory("before strip_main cleanup");
    cleanupSingleStrip(strip_main, strip_main_initialized, DefaultColors::OFF);
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
    cleanupSingleStrip(strip_bg, strip_bg_initialized, DefaultColors::OFF);
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
    cleanupSingleStrip(strip_service, strip_service_initialized, DefaultColors::OFF);
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

    // Заповнюємо allLedsMain згідно з num_leds_main
    allLedsMain.clear();
    for (uint16_t i = 0; i < num_leds_main; ++i) {
        allLedsMain.push_back(i);
    }
    allLedsBg.clear();
    for (uint16_t i = 0; i < settings.getInt(BG_LED_COUNT); ++i) {
        allLedsBg.push_back(i);
    }
}


void initChipID() {
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(chipID, "%04x%04x", (uint32_t)(chipid >> 32), (uint32_t)chipid);
  LOG.printf("[INIT] ChipID Inited: '%s'\n", chipID);
}

void initWifi() {
    if (!WiFiConfig::ENABLED) {
        LOG.println("[WIFI] WiFi disabled in configuration");
        return;
    }

    LOG.println("[WIFI] Initializing WiFi...");
    
    // Встановлюємо режим станції
    WiFi.mode(WIFI_STA);
    
    // Спочатку спробуємо підключитися до збереженої мережі без WiFiManager
    WiFi.begin();
    
    // Чекаємо підключення 10 секунд
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
        LOG.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        lastWifiConnectTime = millis();
        LOG.println("\n[WIFI] Connected to saved WiFi");
        LOG.printf("[WIFI] IP address: %s\n", WiFi.localIP().toString().c_str());
        return;
    }
    
    LOG.println("\n[WIFI] Failed to connect to saved network");
    LOG.println("[WIFI] Starting WiFiManager...");
    
    // Create local WiFiManager to avoid global memory usage
    {
        WiFiManager wm_temp;
        
        // Мінімальні налаштування для економії пам'яті
        wm_temp.setHostname("jaam_fusion");
        wm_temp.setConfigPortalBlocking(true);
        wm_temp.setConnectTimeout(15);
        wm_temp.setConnectRetries(2);
        wm_temp.setConfigPortalTimeout(120);
        
        // Простий колбек без додаткових операцій
        wm_temp.setAPCallback([](WiFiManager* myWiFiManager) {
            LOG.printf("[WIFI] Connect to WiFi: %s\n", myWiFiManager->getConfigPortalSSID().c_str());
        });
        
        // Створюємо ім'я AP з chip ID
        char apName[32];
        snprintf(apName, sizeof(apName), "JAAM_FUSION_%s", chipID);
        
        // Спроба підключення
        if (!wm_temp.autoConnect(apName)) {
            LOG.println("[WIFI] Failed to connect. Rebooting...");
            rebootDevice(3000);
            return;
        }
        
        lastWifiConnectTime = millis();
        LOG.println("[WIFI] Connected to WiFi via WiFiManager");
        LOG.printf("[WIFI] IP address: %s\n", WiFi.localIP().toString().c_str());
        
    } // wm_temp destructor called here, freeing memory
    
    LOG.println("[WIFI] WiFi initialization completed");
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
}

void initTime() {
    LOG.println("[TIME] Init time");
    LOG.printf("[TIME] NTP host: %s\n", settings.getString(NTP_HOST));
    timeClient.setHost(settings.getString(NTP_HOST));
    timeClient.setTimeZone(settings.getInt(TIME_ZONE));
    timeClient.setDSTauto(&dst); // auto update on summer/winter time.
    timeClient.setTimeout(5000); // 5 seconds waiting for reply
    timeClient.begin();
    syncTime(7);
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
    loopCount++;
    int ledsIdx[1] = { currentIdx };
    uint8_t r = random(256), g = random(256), b = random(256);
    
    // Випадковий вибір стрічки
    Adafruit_NeoPixel* strip = nullptr;
    // int stripRand = random(0, 2);
    // switch(stripRand) {
    //     case 0:
    //         if (strip_main_initialized) strip = strip_main;
    //         break;
    //     case 1:
    //         if (strip_bg_initialized) strip = strip_bg;
    //         break;
    //     case 2:
    //         if (strip_service_initialized) strip = strip_service;
    //         break;
    //     default:
    //         if (strip_main_initialized) strip = strip_main;
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

void websocketProcess() {
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
        socketConnect();
    }
    if (websocketReconnect) {
        LOG.println("[WEBSOCKET] Reconnecting... websocketReconnect == true");
        isFirstDataFetchCompleted = false;
        socketConnect();
    }
}

void memoryProcess() {
    loopCount++;
    size_t freeHeap    = ESP.getFreeHeap();
    size_t usedHeap    = ESP.getHeapSize() - freeHeap;
    size_t maxAlloc    = ESP.getMaxAllocHeap();
    uint32_t uptimeMin = millis() / 60000;

    // WiFi status information
    bool wifiConnected = WiFi.status() == WL_CONNECTED;
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
}

// перевірка статусу wifi
// Якщо статус не WL_CONNECTED, то перепідключаємося
void wifiProcess() {
    if (WiFi.status() != WL_CONNECTED) {
        LOG.println("[WIFI] Reconnect");
        initWifi();
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
    if (!strip_main_initialized) {
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

    if (needToReconnectWebsocket) {
        LOG.println("[MAIN] Reconnecting WebSocket");
        isFirstDataFetchCompleted = false;
        needToReconnectWebsocket = false;
        socketConnect();
    }

    if (needReconnectMainStrip || needReconnectBgStrip || needReconnectServiceStrip) {
        LOG.println("[MAIN] Reconnecting LED strip");
        needReconnectStrips = false;
        reconnectStrips();
    }

    if (needAdaptColors) {
        if (strip_main_initialized) {
            LOG.printf("[WEB] Adjusting main colors\n");               
            animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
                for (int i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
        if (strip_bg_initialized) {
            LOG.printf("[WEB] Adjusting bg colors\n");
            animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
                uint32_t color = animation.stripDefaultColor(strip);
                for (int i = 0; i < strip->numPixels(); i++) {
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
    if (needAdaptStripMainBrightness) {
        needAdaptStripMainBrightness = false;
        if (!strip_main_initialized) {
            LOG.println("[LED] strip_bg not initialized, skipping brightness adaptation");
            return;
        }
        animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
            strip->setBrightness(led.brightnessMapped(settings.getInt(CURRENT_BRIGHTNESS)));
            for(uint16_t i = 0; i < strip->numPixels(); i++) {
                uint32_t color = animation.ledActualColor(strip, i);
                strip->setPixelColor(i, color);
            }
            strip->show();
        });
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
    if (needAdaptStripBgBrightness) {
        needAdaptStripBgBrightness = false;
        // Якщо strip_bg не ініціалізований, то нічого не робимо
        if (!strip_bg_initialized) {
            LOG.println("[LED] strip_bg not initialized, skipping brightness adaptation");
            return;
        }
        animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
            strip->setBrightness(led.brightnessMapped(settings.getInt(BRIGHTNESS_BG)));
            uint32_t color = animation.stripDefaultColor(strip);
            for (int i = 0; i < strip->numPixels(); i++) {
                strip->setPixelColor(i, color);
            }
            strip->show();
        });
    }
    if (needAdaptStripServiceBrightness) {
        needAdaptStripServiceBrightness = false;
        // Якщо strip_bg не ініціалізований, то нічого не робимо
        if (!strip_service_initialized) {
            LOG.println("[LED] strip_bg not initialized, skipping brightness adaptation");
            return;
        }
        animation.safeStripOperation(strip_service, [](Adafruit_NeoPixel* strip) {
            strip->setBrightness(led.brightnessMapped(settings.getInt(BRIGHTNESS_SERVICE)));
            for(int i = 0; i < strip->numPixels(); i++) {
                uint32_t color = animation.ledActualColor(strip, i);
                strip->setPixelColor(i, color);
            }
            strip->show();
        });
    }
}

void brightnessProcess() {
    // if auto brightness set to day/night mode, check current hour and choose brightness
    uint8_t currentBrightness = getCurrentBrightnes();
    if (settings.getInt(CURRENT_BRIGHTNESS) != currentBrightness) {
        settings.saveInt(CURRENT_BRIGHTNESS, currentBrightness);
        LOG.printf("[BRIGHTNESS] Setting current_brightness to %d\n", currentBrightness);
        animation.safeStripOperation(strip_main, [currentBrightness](Adafruit_NeoPixel* strip) {
            strip->setBrightness(led.brightnessMapped(currentBrightness));
            for(int i = 0; i < strip->numPixels(); i++) {
                uint32_t color = animation.ledActualColor(strip, i);
                strip->setPixelColor(i, color);
            }
            strip->show();
        });
    }
}

// --- SETUP ---
void setup() {
    LOG.begin(115200);
    checkFreeHeap("LOG initialization");

    initChipID();
    checkFreeHeap("chipID initialization");

    initSettings();
    checkFreeHeap("settings initialization");

    initStrip();
    checkFreeHeap("LED strips initialization");

    initWifi();
    checkFreeHeap("WiFi initialization");

    initWebsocket();
    checkFreeHeap("WebSocket initialization");

    initTime();
    checkFreeHeap("time initialization");

    // Ініціалізуємо генератор випадкових чисел
    randomSeed(esp_random());
    
    // Передаємо settings в AnimationManager
    animation.setSettings(&settings);
    led.setSettings(&settings);
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
    checkFreeHeap("async tasks configuration");

    // Ініціалізація веб-інтерфейсу
    web.begin(strip_main, strip_bg, strip_service);
    web.setSettings(&settings);
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