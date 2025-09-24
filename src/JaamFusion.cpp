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
#include "JaamClimateSensor.h"
#include <JaamSound.h>

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
JaamClimateSensor   climate;
JaamSound           sound;

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
uint16_t                animType;

// --- MAP Configuration ---
std::map<uint16_t, uint16_t>    alertsMap;
std::map<uint16_t, uint8_t>     temperatureMap;
RegionLedMapEntry               customMap[MAX_REGIONS];
uint32_t                        bgLedColors[MAX_BG_LEDS];

// --- TASKS Configuration ---
volatile bool needAdaptAnimationColors = false;
volatile bool needAdaptStripBrightness = false;
volatile bool needReconnectWebsocket = false;
volatile bool needAdaptColors = false;
volatile bool needAdaptAnimationBrightness = false;
volatile bool needAdaptAnimationPeriod = false;
volatile bool needAdaptAnimationType = false;
volatile bool needAdaptClimate = false;
volatile bool needRecalculateLeds = false;
volatile bool needReconnectMainStrip = false;
volatile bool needReconnectBgStrip = false;
volatile bool needReconnectServiceStrip = false;
volatile bool needUpdateBatteryPin = false;
volatile bool needReconfigureDisplay = false;
volatile bool needReconfigureSound = false;
volatile bool needUpdateAnimationsMode = false;
volatile bool needToRegenerateBgColorMap = false;
volatile bool needAdaptVolume = false;
volatile bool needUpdateHomeAlertBit = false;


// --- WIFI Configuration ---
WiFiManager         wm;

time_t              lastWifiConnectTime = 0;  // Track when WiFi was last connected
uint16_t            alertsHash = 0;
bool                wifiConnected = false;


// --- WEBSOCKET Configuration ---
WebsocketsClient    websocket;
time_t              websocketLastPingTime = 0;
bool                isFirstDataFetchCompleted = false;
bool                apiConnected;
bool                websocketReconnect = false;
time_t              lastWebsocketConnectTime = 0;

// --- OTHER Configuration ---
uint8_t             legacy = 0;
int                 alertBit = -1;
time_t              lastHomeAlertChangeTime = 0;

// --- CLOCK ---
bool needDivider = false;


// --- FreeRTOS Task Handles ---
// TaskHandle_t memoryTaskHandle = nullptr;
// TaskHandle_t wifiReconnectTaskHandle = nullptr;
// TaskHandle_t websocketProcessTaskHandle = nullptr;


void clearAllAlertsMaps() {
    // Логування стану пам'яті перед очищенням
    size_t memBefore = ESP.getFreeHeap();
    LOG.printf("[MEMORY] Free heap before clearing alert maps: %u bytes\n", memBefore);
    
    // Очищаємо всі map'и
    alertsMap.clear();
    
    // Додаткове очищення пам'яті після clear()
    // Для std::map викликаємо shrink_to_fit через swap з пустими контейнерами
    std::map<uint16_t, uint16_t>().swap(alertsMap);

    // Принудове очищення пам'яті
    forceMemoryCleanup("after alert maps clearing");
    
    // Дефрагментація пам'яті
    defragmentMemory("after alert maps clearing");
    
    // Логування результату
    size_t memAfter = ESP.getFreeHeap();
    int memReclaimed = (int)(memAfter - memBefore);
    
    LOG.printf("[MAIN] Clearing all alert alerts maps completed\n");
    LOG.printf("[MEMORY] Memory reclaimed: %+d bytes (before: %u, after: %u)\n", 
               memReclaimed, memBefore, memAfter);
}

void clearAllWeatherMaps() {
    // Логування стану пам'яті перед очищенням
    size_t memBefore = ESP.getFreeHeap();
    LOG.printf("[MEMORY] Free heap before clearing weather maps: %u bytes\n", memBefore);
    
    // Очищаємо всі map'и
    temperatureMap.clear();
    
    // Додаткове очищення пам'яті після clear()
    // Для std::map викликаємо shrink_to_fit через swap з пустими контейнерами
    std::map<uint16_t, uint8_t>().swap(temperatureMap);

    // Принудове очищення пам'яті
    forceMemoryCleanup("after weather maps clearing");
    
    // Дефрагментація пам'яті
    defragmentMemory("after weather maps clearing");
    
    // Логування результату
    size_t memAfter = ESP.getFreeHeap();
    int memReclaimed = (int)(memAfter - memBefore);
    
    LOG.printf("[MAIN] Clearing all weather maps completed\n");
    LOG.printf("[MEMORY] Memory reclaimed: %+d bytes (before: %u, after: %u)\n", 
               memReclaimed, memBefore, memAfter);
}

void rebootDevice(int time = 2000, bool async = false) {
    LOG.printf("[MAIN] Rebooting in %d ms...\n", time);
    display.showServiceMessage("Перезавантаження...", "", time);
    
    // Clean up all resources before reboot
    clearAllAlertsMaps();
    clearAllWeatherMaps();
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

// --- SOUND Functions ---
void playMelody(const char* melodyRtttl) {
#if BUZZER_ENABLED
  if (sound.isBuzzerEnabled() && (sound.soundSource == 0 || sound.soundSource == 2)) {
    sound.playBuzzer(melodyRtttl);
  } else {
    LOG.println("Buzzer not enabled or sound source not valid (need 0 or 2): " + String(sound.soundSource));
  }
#endif
}

void playTrack(String track) {
#if DFPLAYER_PRO_ENABLED
  if (track != "" && (sound.soundSource == 1 || sound.soundSource == 2)) {
    sound.playDFPlayer(track);
  } else {
    LOG.println("DFPlayer not enabled or sound source not valid (need 1 or 2): " + String(sound.soundSource));
  }
#endif
}

void playMelody(SoundType type) {
#if BUZZER_ENABLED || DFPLAYER_PRO_ENABLED
  switch (type) {
  case MIN_OF_SILINCE:
    playMelody(MOS_BEEP);
    playTrack(DF_CLOCK_TICK);
    break;
  case MIN_OF_SILINCE_END:
    playMelody(UA_ANTHEM);
    playTrack(DF_UA_ANTHEM);
    break;
  case ALERT_ON:
    playMelody(MELODIES[settings.getInt(MELODY_ON_ALERT)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_ALERT)));
    break;
  case ALERT_OFF:
    playMelody(MELODIES[settings.getInt(MELODY_ON_ALERT_END)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_ALERT_END)));
    break;
  case EXPLOSIONS:
    playMelody(MELODIES[settings.getInt(MELODY_ON_EXPLOSION)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_EXPLOSION)));
    break;
  case CRITICAL_MIG:
    playMelody(MELODIES[settings.getInt(MELODY_ON_CRITICAL_MIG)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_CRITICAL_MIG)));
    break; 
  case CRITICAL_STRATEGIC:
    playMelody(MELODIES[settings.getInt(MELODY_ON_CRITICAL_STRATEGIC)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_CRITICAL_STRATEGIC)));
    break;
  case CRITICAL_MIG_MISSILES:
    playMelody(MELODIES[settings.getInt(MELODY_ON_CRITICAL_MIG_MISSILES)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_CRITICAL_MIG_MISSILES)));
    break;
  case CRITICAL_BALLISTIC_MISSILES:
    playMelody(MELODIES[settings.getInt(MELODY_ON_CRITICAL_BALLISTIC_MISSILES)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_CRITICAL_BALLISTIC_MISSILES)));
    break;
  case CRITICAL_STRATEGIC_MISSILES:
    playMelody(MELODIES[settings.getInt(MELODY_ON_CRITICAL_STRATEGIC_MISSILES)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_CRITICAL_STRATEGIC_MISSILES)));
    break;
  case REGULAR:
    playMelody(CLOCK_BEEP);
    playTrack(DF_CLOCK_BEEP);
    break;
  case SINGLE_CLICK:
    playMelody(SINGLE_CLICK_SOUND);
    playTrack(DF_CLOCK_TICK);
    break;
  case LONG_CLICK:
    playMelody(LONG_CLICK_SOUND);
    playTrack(DF_CLOCK_TICK);
    break;
  }
#endif
}

bool needToPlaySound(SoundType type) {
#if BUZZER_ENABLED || DFPLAYER_PRO_ENABLED
  // do not play any sound before websocket connection
  if (!isFirstDataFetchCompleted) return false;

  // ignore mute on alert
  if (SoundType::ALERT_ON == type && settings.getBool(SOUND_ON_ALERT) && settings.getBool(IGNORE_MUTE_ON_ALERT)) return true;

  // disable sounds on night mode by time only
  if (settings.getBool(MUTE_SOUND_ON_NIGHT) && isItNightNow()) return false;

  switch (type) {
  case MIN_OF_SILINCE:
    return settings.getBool(SOUND_ON_MIN_OF_SL);
  case MIN_OF_SILINCE_END:
    return settings.getBool(SOUND_ON_MIN_OF_SL);
  case ALERT_ON:
    return settings.getBool(SOUND_ON_ALERT);
  case ALERT_OFF:
    return settings.getBool(SOUND_ON_ALERT_END);
  case EXPLOSIONS:
    return settings.getBool(SOUND_ON_EXPLOSION);
  case CRITICAL_MIG:
    return settings.getBool(SOUND_ON_CRITICAL_MIG);
  case CRITICAL_STRATEGIC:
    return settings.getBool(SOUND_ON_CRITICAL_STRATEGIC);
  case CRITICAL_MIG_MISSILES:
    return settings.getBool(SOUND_ON_CRITICAL_MIG_MISSILES);
  case CRITICAL_BALLISTIC_MISSILES:
    return settings.getBool(SOUND_ON_CRITICAL_BALLISTIC_MISSILES);
  case CRITICAL_STRATEGIC_MISSILES:
    return settings.getBool(SOUND_ON_CRITICAL_STRATEGIC_MISSILES);
  case REGULAR:
    return settings.getBool(SOUND_ON_EVERY_HOUR);
  case SINGLE_CLICK:
  case LONG_CLICK:
    return settings.getBool(SOUND_ON_BUTTON_CLICK);
  }
#endif
  return false;
}

// --- WEBSOCKET Functions ---

void printHex(const String& data) {
    LOG.print("[WEBSOCKET] HEX: ");
    for (size_t i = 0; i < data.length(); ++i) {
        LOG.printf("%02X ", static_cast<uint8_t>(data[i]));
    }
    LOG.println();
}

void servicePin(ServiceLed type) {
    if (strip_service != nullptr) {
        uint32_t color = getServicePinColor(type);
        animation.safeStripOperation(strip_service, [type, color](Adafruit_NeoPixel* strip) {
            strip->setPixelColor(type, color);
            strip->show();
        }); 
    }
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

void alertAction(int localAlertBit) {
    if (localAlertBit != alertBit) {
        LOG.printf("[ALERT] Home district alert status changed from %d to %d\n", alertBit, localAlertBit);
        if (settings.getBool(SOUND_ON_ALERT) && hasHigherPriority(localAlertBit, alertBit)) {
            switch (localAlertBit){
                case AlertModes::ALERT:
                    if(needToPlaySound(SoundType::ALERT_ON)) playMelody(ALERT_ON);
                    display.showServiceMessage("ТРИВОГА", "", 5000);
                    break;
                case AlertModes::DRONES:
                    if(needToPlaySound(SoundType::EXPLOSIONS)) playMelody(EXPLOSIONS);
                    display.showServiceMessage("БПЛА", "", 5000);
                    break;
                case AlertModes::MISSILES:
                    if(needToPlaySound(SoundType::EXPLOSIONS)) playMelody(EXPLOSIONS);
                    display.showServiceMessage("РАКЕТИ", "", 5000);
                    break;
                case AlertModes::KABS:
                    if(needToPlaySound(SoundType::EXPLOSIONS)) playMelody(EXPLOSIONS);
                    display.showServiceMessage("КАБ", "", 5000);
                    break;
                case AlertModes::BALLISTIC:
                    if(needToPlaySound(SoundType::EXPLOSIONS)) playMelody(EXPLOSIONS);
                    display.showServiceMessage("БАЛЛІСТИКА", "", 5000);
                    break;
                case AlertModes::EXPLOSION:
                    if(needToPlaySound(SoundType::EXPLOSIONS)) playMelody(EXPLOSIONS);
                    display.showServiceMessage("ВИБУХИ", "", 5000);
                    break;
                case AlertModes::RECON_DRONES:
                    if(needToPlaySound(SoundType::EXPLOSIONS)) playMelody(EXPLOSIONS);
                    display.showServiceMessage("РОЗВІДКА БПЛА", "", 5000);
                    break;
                default:
                    break;
            }
        }
        if (settings.getBool(SOUND_ON_ALERT_END) && localAlertBit == AlertModes::NO_ALERT) {
            if(needToPlaySound(SoundType::ALERT_OFF)) playMelody(ALERT_OFF);
            display.showServiceMessage("ВІДБІЙ", "", 5000);
        }
    }
}

void animateLed(Adafruit_NeoPixel* strip, int map_mode, int led_position, int bit, uint16_t region_id, bool increase = true) {
    //LOG.printf("[ANIMATION] LED %d: region %d to %d\n", led_position, region_id, bit); 

    if (strip == nullptr) {
        LOG.printf("[ANIMATION] LED %d: strip is nullptr\n", led_position);
        return;
    }
    
    uint32_t color;
    uint32_t initialColor = 0x000000; // Початковий колір для анімації
    uint32_t period;
    uint32_t cycles;
    uint8_t startBrightness;
    uint8_t endBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_ANIMATION_END));
    uint8_t ledCount;

    int actualBit = getHighestActualBit(bit);

    if (increase && actualBit != bit) {
        LOG.printf("[ANIMATION] actualBit %d != bit. Animation aborted %d\n", actualBit, bit);
        return;
    }

    switch (actualBit) {
        case -1: 
            color = animation.colorFromHex(settings.getString(COLOR_CLEAR));  
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_CLEAR));             
            animType = settings.getInt(ANIMATION_ALERT_OFF_TYPE);
            cycles = (settings.getInt(ALERT_OFF_TIME) * 1000)/settings.getInt(ANIMATION_ALERT_OFF_CYCLE_TIME);
            period = settings.getInt(ANIMATION_ALERT_OFF_CYCLE_TIME);
            break;
        case 0:
            color = animation.colorFromHex(settings.getString(COLOR_ALERT)); 
            animType = (increase) ? settings.getInt(ANIMATION_ALERT_ON_TYPE) : 4;
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_ALERT));
            period = (increase) ? settings.getInt(ANIMATION_ALERT_ON_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(ALERT_ON_TIME) * 1000)/settings.getInt(ANIMATION_ALERT_ON_CYCLE_TIME) : 1;
            break;
        case 5: 
            color = animation.colorFromHex(settings.getString(COLOR_DRONES));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_DRONES));
            animType = (increase) ? settings.getInt(ANIMATION_DRONE_TYPE) : 4;
            period = (increase) ? settings.getInt(ANIMATION_DRONE_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(DRONE_TIME) * 1000)/settings.getInt(ANIMATION_DRONE_CYCLE_TIME) : 1; 
            break;
        case 6:
            color = animation.colorFromHex(settings.getString(COLOR_MISSILES));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_MISSILES));
            animType = (increase) ? settings.getInt(ANIMATION_MISSILE_TYPE) : 4;
            period = (increase) ? settings.getInt(ANIMATION_MISSILE_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(MISSILE_TIME) * 1000)/settings.getInt(ANIMATION_MISSILE_CYCLE_TIME) : 1; 
            break;
        case 7:
            color = animation.colorFromHex(settings.getString(COLOR_KABS));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_KABS));
            animType = (increase) ? settings.getInt(ANIMATION_KAB_TYPE) : 4;
            period = (increase) ? settings.getInt(ANIMATION_KAB_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(KAB_TIME) * 1000)/settings.getInt(ANIMATION_KAB_CYCLE_TIME) : 1;
            break;
        case 8:
            color = animation.colorFromHex(settings.getString(COLOR_BALLISTIC));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_BALLISTIC));
            animType = (increase) ? settings.getInt(ANIMATION_BALLISTIC_TYPE) : 4;
            period = (increase) ? settings.getInt(ANIMATION_BALLISTIC_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(BALLISTIC_TIME) * 1000)/settings.getInt(ANIMATION_BALLISTIC_CYCLE_TIME)  : 1;
            break;
        case 9:
            color = animation.colorFromHex(settings.getString(COLOR_EXPLOSION));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_EXPLOSION));
            animType = (increase) ? settings.getInt(ANIMATION_EXPLOSION_TYPE) : 4;
            period = (increase) ? settings.getInt(ANIMATION_EXPLOSION_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(EXPLOSION_TIME) * 1000)/settings.getInt(ANIMATION_EXPLOSION_CYCLE_TIME)  : 1;
            break;
        case 10:
            color = animation.colorFromHex(settings.getString(COLOR_RECON_DRONES));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_RECON_DRONES));
            animType = (increase) ? settings.getInt(ANIMATION_RECON_DRONE_TYPE) : 4;
            period = (increase) ? settings.getInt(ANIMATION_RECON_DRONE_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(RECON_DRONE_TIME) * 1000)/settings.getInt(ANIMATION_RECON_DRONE_CYCLE_TIME)  : 1;
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
        map_mode,
        ledsPtr,
        ledCount,
        color,
        initialColor,
        period,
        cycles,
        startBrightness,
        endBrightness,
        region_id,
        actualBit
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
    if (type != TYPE_ALERTS_BATCH && type != TYPE_NOTIFICATIONS_BATCH && type != TYPE_WEATHER_BATCH) {
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

            int highestBitRegion = -1;
            for (uint8_t i = 0; i < ledCount; ++i) {
                int led_position = leds[i];
                //LOG.printf("[WEBSOCKET] LED %d: ", led_position);
                // Шукаємо всі регіони для цього LED
                const std::vector<uint16_t>& regions = getRegionsForLed(led_position);
                for (uint16_t rid : regions) {
                    LOG.printf(" %d ", rid);
                    // Шукаємо найвищий біт для регіону в alertsMap
                    auto alerts_it = alertsMap.find(rid);
                    if (alerts_it != alertsMap.end()) {
                        int highestBitSearch = findHighestBit16(alerts_it->second);
                        LOG.printf("[%d] ", highestBitSearch);

                        if (hasHigherPriority(highestBitSearch, highestBitRegion)) {
                            highestBitRegion = highestBitSearch;
                        }
                    }
                }
            }  
            LOG.println();
    
            int actualBitRegion = getHighestActualBit(highestBitRegion);
            int actualBitDiff = getHighestActualBit(findHighestBit16(flags16, false));

            LOG.printf("[WEBSOCKET] actualBitRegion %d, actualBitDiff %d\n", actualBitRegion, actualBitDiff);

            if (hasHigherPriority(actualBitDiff,actualBitRegion)) {
                for (int i = 0; i < ledCount; ++i) {
                    animateLed(strip_main, MapModes::ALERT, leds[i], actualBitDiff, region_id, true);
                    if (isLedInHomeRegion(leds[i])) {
                        needToAnimateBgHomeRegion = true;
                        homeRegionBit = actualBitDiff;
                    }
                }
            }
        }
        if (needToAnimateBgHomeRegion && settings.getInt(BG_LED_MODE) == BgLedModes::HOME_REGION && strip_bg != nullptr) {
            LOG.printf("[WEBSOCKET] Animating home region LEDs: region %d, bit %d\n",
                settings.getInt(HOME_DISTRICT), homeRegionBit);
            animateLed(strip_bg, MapModes::ALERT, 0, homeRegionBit, settings.getInt(HOME_DISTRICT), true);
            alertAction(homeRegionBit);
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
        
        std::map<int, LedBit> led_bits_actual;
        std::map<int, LedBit> led_bits_old;

        LOG.printf("[WEBSOCKET] processing %d diffs\n", (int)diffs.size());

        // Проходимо по всіх змінах
        bool needToAnimateBgHomeRegion = false;
        bool homeRegionIncrease = false;
        int homeRegionBit = 0;
        for (const auto& diff : diffs) {
            //Показуємо зміни для цього регіону
            LOG.printf("[DIFF] Region %d: flags changed from 0x%04X to 0x%04X\n", 
                diff.region_id, diff.previous_flags, diff.current_flags);
            
            //Показуємо які біти змінилися
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
            int highest_bit_current = findHighestBit16(diff.current_flags);
            LOG.printf("[DIFF]   Highest bit for region %d: %d\n", 
                diff.region_id, highest_bit_current);

            // Шукаємо LED для цього регіону
            const RegionLedMapEntry* entry = getRegionEntry(diff.region_id);
            if (entry) {
                
                for (uint8_t i = 0; i < entry->led_count; ++i) {
                    int led_position = entry->led_positions[i];
                    LOG.printf("[DIFF]   LED for region %d: %d\n", diff.region_id, led_position);

                    led_bits_old[led_position] = {findHighestBitForLed(led_position), diff.region_id};
                    //led_bits_actual[led_position] = {highest_bit_current, diff.region_id};
                    // Оновлюємо найвищий біт для LED
                    // if (hasHigherPriority(findHighestBitForLed(led_position), highest_bit_current)) {
                    //     led_bits_actual[led_position] = {findHighestBitForLed(led_position), diff.region_id};
                    // }
                    //LOG.printf("[DIFF] LED %d highest bit actual: %d\n", led_position, led_bits_actual[led_position].highest_bit);
                    LOG.printf("[DIFF] LED %d highest bit alerts: %d\n", led_position, led_bits_old[led_position].highest_bit);
                }
                // Шукаємо всі леди для цього регіону
                // int highest_bit_region = -1;
                // for (uint8_t i = 0; i < entry->led_count; ++i) {
                //     int led_position = entry->led_positions[i];
                //     LOG.printf("[DIFF] LED %d regions: ", led_position);

                //     // Шукаємо всі регіони для цього LED
                //     const std::vector<uint16_t>& regions = getRegionsForLed(led_position);
                //     for (uint16_t region_id : regions) {
                //         LOG.printf("%d ", region_id);

                //         // Шукаємо найвищий біт для регіону в alertsMap
                //         int highest_bit_region = findHighestBit16(alertsMap[region_id]);
                //         if (diff.region_id == region_id) {
                //             int highest_bit_region_diff = findHighestBit16(diff.current_flags);
                //             if (hasHigherPriority(highest_bit_region, highest_bit_region_diff)) {
                //                 highest_bit_region = highest_bit_region_diff;
                //                 //led_bits_actual[led_position] = {highest_bit_region, region_id};
                //                 LOG.printf("!");
                //             }
                //         } 
                //         LOG.printf("[%d] ", highest_bit_region);
                //         // Оновлюємо найвищий біт для LED
                //         // led_bits_actual[led_position] = {highest_bit_region, region_id};
                //         // if (hasHigherPriority(highest_bit_current, highest_bit_region)) {
                //         //     LOG.printf("! ", highest_bit_region);
                //         //     led_bits_actual[led_position] = {highest_bit_current, region_id};
                //         // }
                //         LOG.printf(", ");
                //     }
                //     LOG.println();
                //     // LOG.printf("[DIFF] LED %d highest bit actual: %d\n", led_position, led_bits_actual[led_position].highest_bit);
                //     // LOG.printf("[DIFF] LED %d highest bit alerts: %d\n", led_position, led_bits_old[led_position].highest_bit);
                // }  
            } else {
                LOG.printf("[DIFF]   No LEDs found for region %d\n", diff.region_id);
            }
        }

        // Виводимо фінальний список бітів для LED
        // LOG.printf("[DIFF] LED bits summary:\n");
        // for (const auto& led : led_bits_actual) {
        //     LOG.printf("[DIFF] LED diff %d: highest_bit=%d, region_id=%d\n",
        //         led.first,
        //         led.second.highest_bit,
        //         led.second.region_id
        //     );
        // }
        // for (const auto& led : led_bits_old) {
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
            for (const auto& led : led_bits_old) {
                // Порівнюємо з led_bits_old
                
                int diff_bit = findHighestBitForLed(led.first);
                int alerts_bit = led.second.highest_bit;

                LOG.printf("[DIFF] LED %d: region %d  actual bit %d\n",
                    led.first, led.second.region_id, diff_bit);

                LOG.printf("[DIFF] LED %d: region %d old bit %d\n",
                    led.first, led.second.region_id, alerts_bit);

                if (diff_bit == alerts_bit) {
                    LOG.printf("[WEBSOCKET] LED %d: region %d no changes in bit %d\n",
                        led.first, led.second.region_id, diff_bit);
                    continue; // Немає змін, пропускаємо
                }

                if (hasHigherPriority(diff_bit, alerts_bit)) {
                    LOG.printf("[WEBSOCKET] LED %d: region %d increasing bit from %d to %d\n",
                        led.first, led.second.region_id, alerts_bit, diff_bit);
                        animateLed(strip_main, MapModes::ALERT, led.first, diff_bit, led.second.region_id, true);
                        if (isLedInHomeRegion(led.first)) {
                            needToAnimateBgHomeRegion = true;
                            homeRegionBit = diff_bit;
                            homeRegionIncrease = true;
                        }
                } 
                if (hasHigherPriority(alerts_bit, diff_bit)) {
                    LOG.printf("[WEBSOCKET] LED %d: region %d decreasing bit from %d to %d\n",
                        led.first, led.second.region_id, alerts_bit, diff_bit);
                        animateLed(strip_main, MapModes::ALERT, led.first, diff_bit, led.second.region_id, false);
                        if (isLedInHomeRegion(led.first)) {
                            needToAnimateBgHomeRegion = true;
                            homeRegionBit = diff_bit;
                            homeRegionIncrease = false;
                        }
                }
            }
            if (needToAnimateBgHomeRegion && settings.getInt(BG_LED_MODE) == BgLedModes::HOME_REGION && strip_bg != nullptr) {
                LOG.printf("[WEBSOCKET] Animating home region LEDs: region %d, bit %d, increase %d\n",
                    settings.getInt(HOME_DISTRICT), homeRegionBit, homeRegionIncrease);
                animateLed(strip_bg, MapModes::ALERT, 0, homeRegionBit, settings.getInt(HOME_DISTRICT), homeRegionIncrease);
                alertAction(homeRegionBit);
                alertBit = homeRegionBit;
            }else {
                LOG.println("[WEBSOCKET] No changes in home region LEDs");
            }
        }
        //LOG.println();
        alertsHash = actualHash;
    }
    if(type == TYPE_WEATHER_BATCH) {
        LOG.printf("[WEBSOCKET] TYPE_WEATHER_BATCH received\n");
        bodyLen = len - HEADER_SZ;

        // payloadLen має ділитися на RECORD_LZ
        if (bodyLen == 0 || (bodyLen % RECORD_LZ) != 0) {
            // некоректний фрейм — пропускаємо і реконнектимось
            LOG.printf("[ERROR] bodyLen == 0 || (bodyLen % RECORD_LZ\n");
            needReconnectWebsocket = true;
            return;
        }

        // Обчислюємо кількість записів
        size_t count = bodyLen / RECORD_LZ;

        // Розбираємо count записів по RECORD_LZ
        const uint8_t* ptr = data + HEADER_SZ;

        LOG.printf("[WEBSOCKET] TYPE_WEATHER_BATCH data processing\n");

        clearAllWeatherMaps(); // очищаємо попередні дані

        for (size_t i = 0; i < count; ++i) {
            uint16_t region_id = uint16_t(ptr[0]) | (uint16_t(ptr[1]) << 8);
            uint8_t flags8 = ptr[2]; // flags8 займає 1 байт
            temperatureMap[region_id] = static_cast<int8_t>(flags8);; // Зберігаємо температуру для регіону
            LOG.printf("[WEBSOCKET] weather region %u: flags8=%u\n", region_id, flags8);
            ptr += RECORD_LZ; // перехід до наступного запису (2B region_id + 1B flags8)
        }
        needAdaptColors = true;
    }
    if (!isFirstDataFetchCompleted) {
        LOG.printf("[WEBSOCKET] init processing\n");
        if (strip_main != nullptr) {
            animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
                for(uint16_t i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
        if (strip_bg != nullptr && settings.getInt(BG_LED_MODE) == BgLedModes::HOME_REGION) {
            animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
                uint32_t color = animation.stripActualColor(strip);
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
    display.showServiceMessage("підключення...", "Сервер даних");
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
        clearAllWeatherMaps();
        //animation.clearAllAnimations();
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

        display.showServiceMessage("підключено!", "Сервер даних", 3000);
    } else {
        display.showServiceMessage("недоступний", "Сервер даних", 3000);
    }
}

// --- LED Functions ---

// функція очищення
void cleanup() {
    if (strip_main != nullptr) {
        animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
            strip->clear();
            // Встановлюємо дефолтний колір
            for(uint16_t i = 0; i < strip->numPixels(); i++) {
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
            for(uint16_t i = 0; i < strip->numPixels(); i++) {
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
            for(uint16_t i = 0; i < strip->numPixels(); i++) {
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
            for(uint16_t i = 0; i < strip->numPixels(); i++) {
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

    if (legacy == LEGACY::MAKET) {
        num_leds_main = 273;
    } else {
        num_leds_main = 26;
    }
    
    if (settings.getInt(MAIN_LED_PIN) > 0) {
        uint8_t ledType = settings.getInt(MAIN_LED_COLOR_FORMAT) + settings.getInt(MAIN_LED_FREQUENCY);
        LOG.printf("[LED] Initializing strip_main on pin %d with %d LEDs, type %d (format:%d + freq:%d)\n", 
                   settings.getInt(MAIN_LED_PIN), num_leds_main, ledType, 
                   settings.getInt(MAIN_LED_COLOR_FORMAT), settings.getInt(MAIN_LED_FREQUENCY));
        status = led.createStrip(strip_main, settings.getInt(MAIN_LED_PIN), num_leds_main, 10, DefaultColors::MAIN_STRIP, ledType);
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
        status = led.createStrip(strip_bg, settings.getInt(BG_LED_PIN), settings.getInt(BG_LED_COUNT), 10, DefaultColors::BG_STRIP, ledType);
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
    //         for (uint16_t i = 0; i < strip->numPixels(); i++) {
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
        status = led.createStrip(strip_service, settings.getInt(SERVICE_LED_PIN), num_leds_service, 10, DefaultColors::SERVICE_STRIP, ledType);
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
        // Перезбираємо список індексів для BG
        allLedsBg.clear();
        for (uint32_t i = 0; i < (uint32_t)settings.getInt(BG_LED_COUNT); ++i) {
            allLedsBg.push_back(i);
        }
        reconnectStripBg();
        needReconnectBgStrip = false;
    }
    if (needReconnectServiceStrip) {
        reconnectStripService();
        needReconnectServiceStrip = false;
    }
    needAdaptStripBrightness = true;
    needAdaptColors = true;
    
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

// --- ALERT Functions ---
int getAlertBit(uint16_t region_id) {
    int highestBit = findHighestBitForRegion(region_id);
    return highestBit;
}


// --- BRIGHTNESS Functions ---
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
void initLegacy() {
    LOG.println("[INIT] Init legacy");
    legacy = settings.getInt(LEGACY);
    num_leds_main = 26; // Default value

    if (legacy == LEGACY::MAKET) {
        num_leds_main = 273;
        settings.saveInt(DISPLAY_MODEL, static_cast<int>(JaamDisplayType::SH1106G));
        settings.saveInt(DISPLAY_HEIGHT, static_cast<int>(JaamDisplayHeight::HEIGHT_64));
        settings.saveInt(DISPLAY_ROTATION, static_cast<int>(JaamDisplayRotation::ROTATION_0));
    } else if (legacy == LEGACY::JAAM_2_1) {
        settings.saveInt(DISPLAY_MODEL, static_cast<int>(JaamDisplayType::SH1106G));
        settings.saveInt(DISPLAY_HEIGHT, static_cast<int>(JaamDisplayHeight::HEIGHT_64));
        settings.saveInt(DISPLAY_ROTATION, static_cast<int>(JaamDisplayRotation::ROTATION_0));
    } else if (legacy == LEGACY::JAAM_1_3) {
        settings.saveInt(DISPLAY_MODEL, static_cast<int>(JaamDisplayType::SSD1306));
        settings.saveInt(DISPLAY_HEIGHT, static_cast<int>(JaamDisplayHeight::HEIGHT_64));
        settings.saveInt(DISPLAY_ROTATION, static_cast<int>(JaamDisplayRotation::ROTATION_0));
    }
    LOG.printf("[INIT] Legacy set to %d\n", legacy);
}

void initSettings() {
    LOG.println("[INIT] Init settings");
    settings.init();
    firmware = parseFirmwareVersion(VERSION);
    LOG.printf("[INIT] major: %d, minor: %d, patch: %d, isBeta: %d, betaBuild: %d\n",
            firmware.major, firmware.minor, firmware.patch, firmware.isBeta, firmware.betaBuild);
    fillFwVersion(currentFwVersion, firmware);
    LOG.printf("[INIT] Current firmware version: %s\n", currentFwVersion);

    initLegacy();

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
    display.invertDisplay(settings.getBool(INVERT_DISPLAY));
    display.rotateDisplay(static_cast<JaamDisplayRotation>(settings.getInt(DISPLAY_ROTATION)));
    display.drawIconWithText(JaamDisplayIcon::TRIDENT, "Jaam Fusion v" + String(VERSION) + " Слава Україні!");
}

void initSensors() {
//  lightSensor.begin(legacy);
//   if (lightSensor.isLightSensorAvailable()) {
//     lightSensorCycle();
//   }
//   if (isAnalogLightSensorEnabled()) {
//     lightSensor.setPhotoresistorPin(settings.getInt(LIGHT_SENSOR_PIN));
//   }

  // init climate sensor
  climate.begin();
  // try to get climate sensor data
  climate.read();

  //initDisplayModes();
}

void initSound() {
#if BUZZER_ENABLED || DFPLAYER_PRO_ENABLED
  sound.init(
    settings.getInt(BUZZER_PIN), 
    settings.getInt(DF_RX_PIN), 
    settings.getInt(DF_TX_PIN),
    settings.getInt(MELODY_VOLUME_CURRENT),
    settings.getInt(MELODY_VOLUME_DAY),
    settings.getInt(MELODY_VOLUME_NIGHT)
  );
#endif
#if BUZZER_ENABLED
  if (sound.isBuzzerEnabled()) {  
    sound.initBuzzer();
  }
#endif
#if DFPLAYER_PRO_ENABLED
  if (sound.isDFPlayerEnabled()) {
    sound.initDFPlayer();
  }
#endif

  if (sound.isBuzzerEnabled() && sound.isDFPlayerConnected()) {
    sound.setSoundSource(settings.getInt(SOUND_SOURCE));
  } else if (sound.isBuzzerEnabled()) {
    sound.setSoundSource(0);
  } else if (sound.isDFPlayerConnected()) {
    sound.setSoundSource(1);
  } else {
    sound.setSoundSource(-1);
  }
  LOG.printf("Sound source: %d\n", sound.soundSource);
}


void initMapping() {
    LOG.println("[INIT] Init mapping");
    // Ініціалізуємо мапінг регіонів
    generateCustomRegionMap();
    // Ініціалізуємо кольори задніх LED
    generateBgLedColorsMap();
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
    web.setStorage(&storage);
    webInitialized = true;
}

static void wifiEvents(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED: {
            char softApIp[16];
            strcpy(softApIp, WiFi.softAPIP().toString().c_str());
            display.printMessage(softApIp, "Введіть у браузері:");
            WiFi.removeEvent(wifiEvents);
            break;
        }
        default:
            break;
    }
}

void apCallback(WiFiManager* wifiManager) {
    const char* message = wifiManager->getConfigPortalSSID().c_str();
    display.printMessage(message, "Підключіться до WiFi:");
    WiFi.onEvent(wifiEvents);
}

void saveConfigCallback() {
    display.showServiceMessage(wm.getWiFiSSID(true).c_str(), "Збережено AP:");
    delay(2000);
    rebootDevice();
    }

void initWifi() {
    if (!WiFiConfig::ENABLED) {
        LOG.println("[WIFI] WiFi disabled in configuration");
        return;
    }

    LOG.println("[WIFI] Initializing WiFi...");
    display.showServiceMessage("підключенння...", "WiFi");
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
    String wifiSSID = wm.getWiFiSSID(true);
    if (wifiSSID.length() > 0) {
        display.showServiceMessage(wifiSSID, "Підключення до:");
    }
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
    display.showServiceMessage("підключено!", "WiFi", 3000);
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
            animType = 0; // FADE
            break;
        case 1:
            animType = 1; // BLINK
            break;
        case 2:
            animType = 2; // BLEND_FADE
            break;
        case 3:
            animType = 3; // PULSE
            break;
        default:
            animType = 0; // FADE
    }
    //animType = 2; // BLEND_FADE

    // Випадкові параметри для анімації з використанням конфігурації
    uint32_t period = random(AnimationConfig::MIN_PERIOD, AnimationConfig::MAX_PERIOD + 1);
    uint32_t cycles = 30; // random(AnimationConfig::MIN_CYCLES, AnimationConfig::MAX_CYCLES + 1);
    uint8_t startBrightness = 50; // random(AnimationConfig::MIN_START_BRIGHTNESS, AnimationConfig::MAX_START_BRIGHTNESS + 1);
    uint8_t endBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_ANIMATION_END)); //   random(AnimationConfig::MIN_END_BRIGHTNESS, AnimationConfig::MAX_END_BRIGHTNESS + 1);
    
    // Створення анімації з обробкою помилок
    if (!animation.createAnimation(
        animType,
        strip,
        MapModes::ALERT,
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
        clearAllWeatherMaps();
        animation.clearAllAnimations();
        //int positions[] = {}; // not used in RUNNING_LIGHT
        animation.createAnimation(
            5, // RUNNING_LIGHT
            strip_main,  
            MapModes::ALERT,       
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

// --- Climate Process ---
void climateProcess() {
  if (!climate.isAnySensorAvailable()) return;
  climate.read();
  //updateHaTempSensors();
  //updateHaHumSensors();
  //updateHaPressureSensors();
}

void mainThreadProcess() {
    // Ця функція виконується в основному циклі
    // Вона потрібна для асинхронного менеджера, щоб мати можливість виконувати інші задачі

    if (needToRegenerateBgColorMap) {
        LOG.println("[MAIN] Regenerating BG LED color map");
        generateBgLedColorsMap();
        needToRegenerateBgColorMap = false;
    }

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
                for (uint16_t i = 0; i < strip->numPixels(); i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });
        }
        if (strip_bg != nullptr) {
            LOG.printf("[WEB] Adjusting bg colors\n");
            animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
                switch (settings.getInt(BG_LED_MODE)) {
                    case BgLedModes::COLOR_MAP: {
                        for(uint16_t i = 0; i < strip->numPixels(); i++) {
                            uint32_t color = animation.ledActualColor(strip, i);
                            strip->setPixelColor(i, color);
                        }
                        break;
                    }
                    default: {
                        uint32_t color = animation.stripActualColor(strip);
                        for(uint16_t i = 0; i < strip->numPixels(); i++) {
                            strip->setPixelColor(i, color);
                        }
                        break;
                    }
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
        LOG.printf("[WEB] Adjusting animation colors\n");
        animation.adaptAllAnimationColors();
        needAdaptAnimationColors = false;
    }
    if (needAdaptAnimationBrightness) {
        LOG.printf("[WEB] Adjusting animation brightness\n");
        animation.adaptAllAnimationBrightness();
        needAdaptAnimationBrightness = false;
    }
    if (needAdaptAnimationPeriod) {
        LOG.printf("[WEB] Adjusting animation period\n");
        animation.adaptAllAnimationPeriod();
        needAdaptAnimationPeriod = false;
    }
    if (needAdaptAnimationType) {
        LOG.printf("[WEB] Adjusting animation type\n");
        animation.adaptAllAnimationType();
        needAdaptAnimationType = false;
    }
    if (needAdaptStripBrightness) {
        LOG.printf("[WEB] Adjusting strip brightness\n");
        needAdaptStripBrightness = false;
        if (strip_main != nullptr) {
            animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
                strip->setBrightness(led.brightnessMapped(settings.getInt(CURRENT_BRIGHTNESS)));
                // for(uint16_t i = 0; i < strip->numPixels(); i++) {
                //     uint32_t color = animation.ledActualColor(strip, i);
                //     strip->setPixelColor(i, color);
                // }
                strip->show();
            });
        }
        if (strip_bg != nullptr) {
            animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
                strip->setBrightness(led.brightnessMapped(settings.getInt(CURRENT_BRIGHTNESS)));
                // uint32_t color = animation.stripActualColor(strip);
                // for (uint16_t i = 0; i < strip->numPixels(); i++) {
                //     strip->setPixelColor(i, color);
                // }
                strip->show();
            });
        }
        if (strip_service != nullptr) {
            animation.safeStripOperation(strip_service, [](Adafruit_NeoPixel* strip) {
                strip->setBrightness(led.brightnessMapped(settings.getInt(CURRENT_BRIGHTNESS)));
                // for(uint16_t i = 0; i < strip->numPixels(); i++) {
                //     uint32_t color = animation.ledActualColor(strip, i);
                //     strip->setPixelColor(i, color);
                // }
                strip->show();
            });
        }
        // int ledsIdx[1] = { 0 };
        // if (!animation.createAnimation(
        //     6, // SET_BRIGHTNESS
        //     strip_main,
        //     MapModes::ALERT,
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
        initLegacy(); // reinitialize legacy settings
        display.begin(static_cast<JaamDisplayType>(settings.getInt(DISPLAY_MODEL)), static_cast<JaamDisplayHeight>(settings.getInt(DISPLAY_HEIGHT)));
        display.invertDisplay(settings.getBool(INVERT_DISPLAY));
        display.rotateDisplay(static_cast<JaamDisplayRotation>(settings.getInt(DISPLAY_ROTATION)));
        needReconfigureDisplay = false;
    }

    if (needReconfigureSound) {
        LOG.println("[MAIN] Reconfiguring sound");
        initSound();
        needReconfigureSound = false;
    }

    if (needUpdateAnimationsMode) {
        LOG.printf("[MAIN] Animations sync mode %s\n", settings.getInt(ENABLE_SYNC_ANIMATIONS) ? "ENABLED" : "DISABLED");
        animation.setSynchronizedMode(settings.getInt(ENABLE_SYNC_ANIMATIONS));
        needUpdateAnimationsMode = false;
    }

    if (needAdaptClimate) {
        LOG.println("[MAIN] Adapting climate settings");
        climateProcess();
        needAdaptClimate = false;
    }

    if (needAdaptVolume) {
        LOG.println("[MAIN] Adapting sound settings");
        sound.setVolumeNight(settings.getInt(MELODY_VOLUME_NIGHT));
        sound.setVolumeDay(settings.getInt(MELODY_VOLUME_DAY));

        needAdaptVolume = false;
    }

    if (needUpdateHomeAlertBit) {
        LOG.println("[MAIN] Updating home alert bit");
        int localAlertBit = getAlertBit(settings.getInt(HOME_DISTRICT));
        alertAction(localAlertBit);
        alertBit = localAlertBit;
        needUpdateHomeAlertBit = false;
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
                strip->show();
            });
        }
        if (strip_bg != nullptr) {
            animation.safeStripOperation(strip_bg, [currentBrightness](Adafruit_NeoPixel* strip) {
                strip->setBrightness(led.brightnessMapped(currentBrightness));
                strip->show();
            });
        }
        if (strip_service != nullptr) {
            animation.safeStripOperation(strip_service, [currentBrightness](Adafruit_NeoPixel* strip) {
                strip->setBrightness(led.brightnessMapped(currentBrightness));
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

// --- Sound Process ---
void volumeProcess() {
  int volumeLocal = isItNightNow() ? sound.volumeNight : sound.volumeDay;
  if (volumeLocal != sound.volumeCurrent) {
    sound.setVolumeCurrent(volumeLocal);
    settings.saveInt(MELODY_VOLUME_CURRENT, volumeLocal);
    #if BUZZER_ENABLED
    if (sound.isBuzzerEnabled()) {
      sound.setBuzzerVolume(volumeLocal); 
    }
    #endif
    #if DFPLAYER_PRO_ENABLED
    if (sound.isDFPlayerConnected()) {
      sound.setDFPlayerVolume(volumeLocal); 
    }
    #endif
    LOG.printf("Set volume to: %d\n", volumeLocal);
  }
}

void beepHourProcess() {
  if (needToPlaySound(REGULAR) && sound.beepHour != timeClient.hour() && timeClient.minute() == 0 && timeClient.second() == 0) {
    sound.setBeepHour(timeClient.hour());
    playMelody(REGULAR);
  }
}

// --- SETUP ---
void setup() {
    LOG.begin(115200);

    checkFreeHeap("LOG initialization");

    initSettings();
    checkFreeHeap("settings initialization");
    
    initStrip();
    checkFreeHeap("LED strips initialization");
    brightnessProcess();

    initDisplay();
    checkFreeHeap("display initialization");

    initStorage();
    checkFreeHeap("SPIFFS initialization");

    initChipID();
    checkFreeHeap("chipID initialization");

    initMapping();
    checkFreeHeap("LED mapping initialization");

    initBattery();
    checkFreeHeap("battery initialization");

    initSensors();
    checkFreeHeap("sensors initialization");

    initSound();
    checkFreeHeap("sound initialization");

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
    // Встановлюємо режим роботи анімацій
    animation.setSynchronizedMode(settings.getBool(ENABLE_SYNC_ANIMATIONS));
    checkFreeHeap("animation settings");
    led.setSettings(&settings);
    checkFreeHeap("LED settings");
    battery.setSettings(&settings);
    checkFreeHeap("battery settings");

    // Налаштовуємо асинхронні задачі
#if defined(TEST_ANIMATION)
    async.setInterval(animations, ANIMATION_INTERVAL);
#endif
    //async.setInterval(animationsLog, 1000);
    async.setInterval(brightnessProcess, MAIN_THREAD_CHECK_INTERVAL);
    async.setInterval(memoryProcess, MEMORY_CHECK_INTERVAL);
    async.setInterval(wifiProcess, WIFI_CHECK_INTERVAL);
#if !defined(TEST_ANIMATION)
    async.setInterval(websocketProcess, WEBSOCKET_CHECK_INTERVAL);;
#endif
    async.setInterval(timeProcess, TIME_CHECK_INTERVAL);
    async.setInterval(mainThreadProcess, MAIN_THREAD_CHECK_INTERVAL);
    
    async.setInterval(batteryProcess, 10000); // кожні 10 секунд
    async.setInterval(displayProcess, 1000); // кожну секунду
    async.setInterval(climateProcess, 10000); // кожні 10 секунд
    async.setInterval(volumeProcess, 1000); // кожну секунду
    async.setInterval(beepHourProcess, 1000); // кожну секунду

    checkFreeHeap("async tasks configuration");
    
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