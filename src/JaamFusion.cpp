#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include <sys/time.h>
#include <algorithm>

#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <NTPtime.h>

#include <esp_system.h>
#include <async.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <HTTPUpdate.h>

#include "JaamAnimation.h"
#include "JaamBattery.h"
#include "JaamLogs.h"
#include "JaamConfig.h"
#include "JaamSettings.h"
#include "JaamWeb.h"
#include "JaamApi.h"
#include "JaamMDNS.h"
#include "JaamLed.h"
#include "JaamUtils.h"
#include "JaamStorage.h"
#include "JaamDisplay.h"
#include "JaamClimateSensor.h"
#include "JaamLightSensor.h"
#include "JaamSound.h"
#include "JaamButton.h"
#include "JaamHardware.h"
#include "JaamFirmwareUpdate.h"
#include "JaamSiren.h"

using namespace websockets;

// --- Forward Declarations ---
void adaptStripColorsAndBrightness();
void handleAdaptAnimationColors();
void handleAdaptAnimationBrightness();
void handleAdaptAnimationPeriod();
void handleAdaptAnimationType();
void handleRecalculateLeds();
void handleReconnectStrips(bool main, bool bg, bool service);
void handleUpdateBatteryPin();
void handleReconfigureDisplay();
void handleReconfigureSound();
void handleReconfigureSensors();
void handleReconfigureButtons();
void handleUpdateAnimationsMode();
void handleAdaptClimate();
void handleAdaptVolume();
void handleUpdateHomeAlertBit();
void handleUpdateTimezone();
void handleUpdateNtpHost();
void handleReconnectWebsocket();
void handleRegenerateBgColorMap();
void requestPlayTestMelody(int melodyId);
void requestPlayTestTrack(int trackId);
void requestReconfigureAll();
void requestWebsocketReconnect();
void requestFirmwareUpdate(const char* firmwareId = nullptr);
void requestRecalculateLeds();
void requestAdaptColors();
void requestRebootDevice();
void requestToRegenerateBgColorMap();
void displayProcess();
void brightnessProcess();
void rebuildSensorsListItems();
bool isButtonActivated();

// --- MAIN Configuration ---
char                chipID[13];

Async               async = Async(20);

NTPtime             timeClient(2);
DSTime*             currentDST = nullptr;  // Буде налаштовуватися для кожного поясу

JaamSettings        settings;
JaamFirmwareUpdate  fwUpdate;
JaamWeb             web;
JaamApi             api;
JaamMDNS            mdnsService;
JaamLed             led;
JaamBattery         battery;
JaamStorage         storage;
JaamDisplay         display;
JaamClimateSensor   climate;
JaamLightSensor     lightSensor;
JaamSound           sound;
JaamButton          buttons;
JaamHardware        hardwareConfig;
JaamSiren           siren;

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

// --- MAP Configuration ---                       // старий обробник (TYPE_RADIATION_BATCH)
std::map<uint16_t, uint8_t>     temperatureMap;
int                             weatherAutoMinTemp = 0;
int                             weatherAutoMaxTemp = 0;
bool                            weatherAutoBoundsValid = false;
// Current region map (flat array format) - dynamically allocated
CurrentRegionMap                currentMap;
uint32_t                        bgLedColors[MAX_LEDS_STRIP_BG];

// --- Alerts flat storage (новий обробник TYPE_ALERTS_BATCH) ---
// ~340 B BSS: region_id → flags16 (0 = немає тривоги)
uint16_t alertsFlat[MAX_REGIONS + 1] = {};
// ~500 B BSS: led_pos → поточний найвищий alert-bit (-1 = немає тривоги)
int8_t   ledBitCache[MAX_LEDS_STRIP_MAIN];
// ~63 B BSS: бітсет — які LED змінились у поточному пакеті
static uint8_t s_ledDirty[(MAX_LEDS_STRIP_MAIN + 7) / 8];

// --- WIFI Configuration ---
WiFiManager         wm;

time_t              lastWifiConnectTime = 0;  // Track when WiFi was last connected
uint16_t            alertsHash = 0;
bool                wifiConnected = false;


// --- WEBSOCKET Configuration ---
WebsocketsClient    websocket;
time_t              websocketLastPingTime = 0;
bool                isFirstDataFetchCompleted = false;
bool                startup = true;
bool                websocketConnected;
bool                websocketReconnect = false;
time_t              lastWebsocketConnectTime = 0;

// --- OTHER Configuration ---
bool                minuteOfSilence = false;
bool                uaAnthemPlaying = false;
short               clockBeepInterval = -1;
bool                isMapOff = false;
bool                isDisplayOff = false;
int                 prevMapMode = 1;
int                 alertBit = -1;
time_t              lastHomeAlertChangeTime = 0;
bool                nightMode = false;
uint16_t            homeAlertFlags = 0;

// --- CLOCK ---
bool needDivider = false;

// --- SYSTEM INFO ---
struct SystemInfo {
    size_t usedMemory;
    size_t freeMemory;
    uint32_t uptime;         // seconds
    uint32_t wifiUptime;     // seconds
    int8_t wifiSignal;
    bool websocketStatus;
    uint32_t websocketUptime; // seconds
    float cpuTemp;
};

SystemInfo getSystemInfo() {
    SystemInfo info;
    info.freeMemory = ESP.getFreeHeap();
    info.usedMemory = ESP.getHeapSize() - info.freeMemory;
    info.uptime = millis() / 1000;
    info.wifiUptime = wifiConnected ? (millis() - lastWifiConnectTime) / 1000 : 0;
    info.wifiSignal = wifiConnected ? WiFi.RSSI() : 0;
    info.websocketStatus = websocket.available();
    info.websocketUptime = websocket.available() ? (millis() - lastWebsocketConnectTime) / 1000 : 0;
    info.cpuTemp = temperatureRead();
    return info;
}

// --- FreeRTOS Task Handles ---
// TaskHandle_t memoryTaskHandle = nullptr;
// TaskHandle_t wifiReconnectTaskHandle = nullptr;
// TaskHandle_t websocketProcessTaskHandle = nullptr;


void clearAllAlertsMaps() {
    size_t memBefore = ESP.getFreeHeap();
    LOG.printf("[MEMORY] Free heap before clearing alert maps: %u bytes\n", memBefore);

    // Новий flat-сторедж
    memset(alertsFlat,   0,  sizeof(alertsFlat));
    memset(ledBitCache, -1,  sizeof(ledBitCache));
    memset(s_ledDirty,   0,  sizeof(s_ledDirty));

    size_t memAfter = ESP.getFreeHeap();
    LOG.printf("[MAIN] Clearing all alert maps completed\n");
    LOG.printf("[MEMORY] Memory reclaimed: %+d bytes (before: %u, after: %u)\n",
               (int)(memAfter - memBefore), memBefore, memAfter);
}

void clearAllWeatherMaps() {
    // Логування стану пам'яті перед очищенням
    size_t memBefore = ESP.getFreeHeap();
    LOG.printf("[MEMORY] Free heap before clearing weather maps: %u bytes\n", memBefore);
    
    // Очищаємо всі map'и
    temperatureMap.clear();

    // Скидаємо кеш авто-меж температури
    weatherAutoBoundsValid = false;
    
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

int getCurrentMapMode() {
  if (minuteOfSilence || uaAnthemPlaying || startup) return MapModes::FLAG; // ua flag

//   int homeRegionId = settings.getInt(HOME_DISTRICT);
//   int alarmMode = settings.getInt(ALARMS_AUTO_SWITCH);
//   if (alarmMode == 1 && isAlertInNeighboringDistricts()) {
//     return 1; // alerts mode
//   }
//   if (alarmMode >= 1 && id_to_alerts[homeRegionId].first != 0) {
//     return 1; // alerts mode
//   }
  return isMapOff ? 0 : settings.getInt(MAP_MODE);
}

void showMinOfSilenceScreen(int screen) {
  switch (screen) {
  case 0:
    display.drawIconWithText(JaamDisplayIcon::TRIDENT, "Шана Полеглим Героям!");
    break;
  case 1:
    display.drawIconWithText(JaamDisplayIcon::TRIDENT, "Слава Україні!");
    break;
  case 2:
    display.drawIconWithText(JaamDisplayIcon::TRIDENT, "Смерть ворогам!");
    break;
  default:
    break;
  }
}

void displayMinuteOfSilence() {
  // every 3 sec.
  int periodIndex = getCurrentPeriodIndex(3, 3, timeClient.second());
  showMinOfSilenceScreen(periodIndex);
}

// --- SOUND Functions ---
void playMelody(const char* melodyRtttl) {
#if BUZZER_ENABLED
  if (sound.isBuzzerEnabled() && (sound.soundSource == 0 || sound.soundSource == 2)) {
    sound.playBuzzer(melodyRtttl);
  } else {
    LOG.printf("Buzzer not enabled or sound source not valid (need 0 or 2): %d\n", sound.soundSource);
  }
#endif
}

void playTrack(String track) {
#if DFPLAYER_PRO_ENABLED
  if (track != "" && (sound.soundSource == 1 || sound.soundSource == 2)) {
    sound.playDFPlayer(track);
  } else {
    LOG.printf("DFPlayer not enabled or sound source not valid (need 1 or 2): %d\n", sound.soundSource);
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
  case DRONES:
    playMelody(MELODIES[settings.getInt(MELODY_ON_DRONES)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_DRONES)));
    break;
  case MISSILES:
    playMelody(MELODIES[settings.getInt(MELODY_ON_MISSILES)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_MISSILES)));
    break;
  case KABS:
    playMelody(MELODIES[settings.getInt(MELODY_ON_KABS)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_KABS)));
    break;
  case BALLISTIC:
    playMelody(MELODIES[settings.getInt(MELODY_ON_BALLISTIC)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_BALLISTIC)));
    break;
  case RECON_DRONES:
    playMelody(MELODIES[settings.getInt(MELODY_ON_RECON_DRONES)]);
    playTrack(sound.getTrackById(settings.getInt(TRACK_ON_RECON_DRONES)));
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
  // do not play any sound before websocket connection and startup
  if (startup || !isFirstDataFetchCompleted) return false;

  // ignore mute on alert
  //if (SoundType::ALERT_ON == type && settings.getBool(SOUND_ON_ALERT) && settings.getBool(IGNORE_MUTE_ON_ALERT)) return true;

  // disable sounds on night mode by time only
  if (settings.getBool(MUTE_SOUND_ON_NIGHT) && isItNightNow() && !settings.getBool(IGNORE_MUTE_ON_ALERT)) return false;

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
  case DRONES:
    return settings.getBool(SOUND_ON_DRONES);
  case MISSILES:
    return settings.getBool(SOUND_ON_MISSILES);
  case KABS:
    return settings.getBool(SOUND_ON_KABS);
  case BALLISTIC:
    return settings.getBool(SOUND_ON_BALLISTIC);
  case RECON_DRONES:
    return settings.getBool(SOUND_ON_RECON_DRONES);
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

// --- Firmware Update

void mapUpdate(float percents) {
    if (strip_main == nullptr || num_leds_main == 0) return;
    if (percents < 0.0f) percents = 0.0f;
    if (percents > 1.0f) percents = 1.0f;

    uint32_t litCount = static_cast<uint32_t>(num_leds_main * percents + 0.5f);
    const uint32_t green = animation.adaptColorBrightness(DefaultColors::MAIN_STRIP, led.brightnessRelative(100)); // Використовуємо 100% локальної яскравості для максимальної видимості прогресу
    const uint32_t off = DefaultColors::OFF;
    

    animation.safeStripOperation(strip_main, [&](Adafruit_NeoPixel* strip) {
        for (uint32_t i = 0; i < num_leds_main; ++i) {
            strip->setPixelColor(i, i < litCount ? green : off);
        }
        strip->show();
    });

    if (strip_bg != nullptr) {
        uint32_t bgCount = strip_bg->numPixels();
        uint32_t bgLitCount = static_cast<uint32_t>(bgCount * percents + 0.5f);
        animation.safeStripOperation(strip_bg, [&](Adafruit_NeoPixel* strip) {
            for (uint32_t i = 0; i < bgCount; ++i) {
                strip->setPixelColor(i, i < bgLitCount ? green : off);
            }
            strip->show();
        });
    }

    if (strip_service != nullptr) {
        static uint32_t serviceIndex = 0;
        uint32_t serviceCount = strip_service->numPixels();
        if (serviceCount > 0) {
            animation.safeStripOperation(strip_service, [&](Adafruit_NeoPixel* strip) {
                for (uint32_t i = 0; i < serviceCount; ++i) {
                    strip->setPixelColor(i, i == serviceIndex ? green : off);
                }
                strip->show();
            });
            serviceIndex = (serviceIndex + 1) % serviceCount;
        }
    }
}

bool saveNightMode(bool newState) {
    if (nightMode == newState) return false; // No change needed
    nightMode = newState;
  
    display.showServiceMessage(nightMode ? "Увімкнено" : "Вимкнено", "Нічна яскравість:");
    adaptStripColorsAndBrightness();
    handleAdaptAnimationBrightness();
    LOG.printf("[MAIN] Night mode %s\n", nightMode ? "enabled" : "disabled");
    api.setNightMode(nightMode);
    return true;
}

bool saveMapOff(bool newState, bool showMessage = true) {
    if (isMapOff == newState) return false; // No change needed
    isMapOff = newState;
  
    if (showMessage) {
        display.showServiceMessage(!isMapOff ? "Увімкнено" : "Вимкнено", "Мапу:");
    }
    adaptStripColorsAndBrightness();
    LOG.printf("[MAIN] Map %s\n", isMapOff ? "disabled" : "enabled");
    api.setMapEnabled(!isMapOff);
    return true;
}

bool saveDisplayOff(bool newState, bool showMessage = true) {
    if (isDisplayOff == newState) return false; // No change needed
    isDisplayOff = newState;
  
    if (showMessage) {
        display.showServiceMessage(!isDisplayOff ? "Увімкнено" : "Вимкнено", "Дисплей:");
    }
    LOG.printf("[MAIN] Display %s\n", isDisplayOff ? "disabled" : "enabled");
    api.setDisplayEnabled(!isDisplayOff);
    return true;
}

// --- Buttons Functions ---

static bool isFirmwareUpdatePriority(JaamButton::Action action) {
    return action == JaamButton::LONG_CLICK && fwUpdate.isUpdateAvailable() && settings.getBool(NEW_FW_NOTIFICATION) && isButtonActivated() && !isDisplayOff;
}

void handleClick(uint8_t buttonId, int event, JaamButton::Action action) {
  // Firmware update action has highest priority on long click.
    if (isFirmwareUpdatePriority(action)) {
      requestFirmwareUpdate();
      return;
  }

  SoundType soundType = action == JaamButton::SINGLE_CLICK ? SoundType::SINGLE_CLICK : SoundType::LONG_CLICK;
  if (event != 0 && needToPlaySound(soundType)) playMelody(soundType);

  switch (event) {
    // change map mode
    case 1:
      nextMapMode();
      break;
    // change display mode
    case 2:
      nextDisplayMode();
      break;
    // toggle map
    case 3:
      saveMapOff(!isMapOff);
      break;
    // toggle display
    case 4:
      saveDisplayOff(!isDisplayOff);
      break;
    // toggle display and map
    case 5:
      if (isDisplayOff != isMapOff) {
        saveMapOff(false, false);
        saveDisplayOff(false, false);
      } else {
        saveMapOff(!isMapOff, false);
        saveDisplayOff(!isDisplayOff, false);
      }
      display.showServiceMessage(!isMapOff ? "Увімкнено" : "Вимкнено", "Дисплей та мапу:");
      break;
    // toggle night mode
    case 6:
      saveNightMode(!nightMode);
      break;
    // reboot device (long click)
    case 10:
      rebootDevice();
      break;
    case 11: {
        JaamApiButtonEventType eventType = action == JaamButton::SINGLE_CLICK ? JAAM_BUTTON_EVENT_CLICK : JAAM_BUTTON_EVENT_LONG_CLICK;
        api.broadcastButtonEvent(buttonId, eventType);
        break;
    }
    default:
      // do nothing
      break;
  }
}

bool isButtonActivated() {
  return settings.getInt(BUTTON_1_MODE) != 0 || settings.getInt(BUTTON_1_MODE_LONG) != 0 || settings.getInt(BUTTON_2_MODE) != 0 || settings.getInt(BUTTON_2_MODE_LONG) != 0 || settings.getInt(BUTTON_3_MODE) != 0 || settings.getInt(BUTTON_3_MODE_LONG) != 0;
}

void buttonClick(uint8_t buttonId, int mode) {
#if TEST_MODE
    char buttonName[12];
    snprintf(buttonName, sizeof(buttonName), "Button %u", (unsigned)buttonId);
    displayMessage("Single click!", buttonName);
#else
    handleClick(buttonId, mode, JaamButton::SINGLE_CLICK);
#endif
}

void buttonLongClick(uint8_t buttonId, int modeLong) {
#if TEST_MODE
    char buttonName[12];
    snprintf(buttonName, sizeof(buttonName), "Button %u", (unsigned)buttonId);
    displayMessage("Long click!", buttonName);
#else
    handleClick(buttonId, modeLong, JaamButton::LONG_CLICK);
#endif
}

void buttonDuringLongClick(int modeLong, JaamButton::Action action) {
    if (isFirmwareUpdatePriority(JaamButton::LONG_CLICK)) {
    return;
  }
    if (action == JaamButton::DURING_LONG_CLICK) {
    switch (modeLong) {
    //   case 8:
    //     // if lamp mode is active, increase lamp brightness
    //     if (getCurrentMapMode() == 5) {
    //       int newBrightness = settings.getInt(BRIGHTNESS_LAMP) + 1;
    //       if (newBrightness > 100) {
    //         newBrightness = 100;
    //       }
    //       saveLampBrightness(newBrightness, false, true);
    //       sprintf(lampBrightnessMsg, "%d%%", newBrightness);
    //       showServiceMessage(lampBrightnessMsg, "Яскравість лампи:");
    //     } else {
    //       showServiceMessage("Лише для режиму лампи");
    //     }
    //     break;
    //   case 9:
    //     // if lamp mode is active, decrease lamp brightness
    //     if (getCurrentMapMode() == 5) {
    //       int newBrightness = settings.getInt(BRIGHTNESS_LAMP) - 1;
    //       if (newBrightness < 0) {
    //         newBrightness = 0;
    //       }
    //       saveLampBrightness(newBrightness, false, true);
    //       sprintf(lampBrightnessMsg, "%d%%", newBrightness);
    //       showServiceMessage(lampBrightnessMsg, "Яскравість лампи:");
    //     } else {
    //       showServiceMessage("Лише для режиму лампи");
    //     }
    //     break;
      default:
        // do nothing
        break;
  }
    } else if (action == JaamButton::LONG_CLICK_END) {
    switch (modeLong) {
      case 8:
      case 9:
        // if (getCurrentMapMode() == 5) {
        //   saveCurrentLampBrightness();
        // }
        break;
      default:
        // do nothing
        break;
    }
  }
}

void button1Click() {
  LOG.printf("Button 1 click\n");
    buttonClick(1, settings.getInt(BUTTON_1_MODE));
}

void button2Click() {
  LOG.printf("Button 2 click\n");
    buttonClick(2, settings.getInt(BUTTON_2_MODE));
}

void button3Click() {
  LOG.printf("Button 3 click\n");
    buttonClick(3, settings.getInt(BUTTON_3_MODE));
}

void button1LongClick() {
  LOG.printf("Button 1 long click\n");
    buttonLongClick(1, settings.getInt(BUTTON_1_MODE_LONG));
}

void button2LongClick() {
  LOG.printf("Button 2 long click\n");
    buttonLongClick(2, settings.getInt(BUTTON_2_MODE_LONG));
}

void button3LongClick() {
  LOG.printf("Button 3 long click\n");
    buttonLongClick(3, settings.getInt(BUTTON_3_MODE_LONG));
}

void button1DuringLongClick(JaamButton::Action action) {
  LOG.printf("Button 1 during long click\n");
    buttonDuringLongClick(settings.getInt(BUTTON_1_MODE_LONG), action);
}

void button2DuringLongClick(JaamButton::Action action) {
  LOG.printf("Button 2 during long click\n");
    buttonDuringLongClick(settings.getInt(BUTTON_2_MODE_LONG), action);
}

void button3DuringLongClick(JaamButton::Action action) {
  LOG.printf("Button 3 during long click\n");
    buttonDuringLongClick(settings.getInt(BUTTON_3_MODE_LONG), action);
}

void servicePin(ServiceLed type) {
    if (strip_service != nullptr) {
        uint32_t color = animation.ledActualColor(strip_service, type);
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

void alertAction(int bit, int districtId) {
    // int actualBit = getHighestActualBit(bit);
    // if (actualBit != bit) {
    //     LOG.printf("[ALERT] actualBit %d != %d. Animation aborted\n", actualBit, bit);
    //     return;
    // }
    // if (bit == alertBit ) {
    //     LOG.printf("[ALERT] No change in alert status (%d) <-> (%d)\n", bit, alertBit);
    //     return; // No change in alert status
    // }
    // if (!hasHigherPriority(bit, alertBit) && bit != -1) {
    //     LOG.printf("[ALERT] New alert status %d has lower priority than current %d. Ignored\n", bit, alertBit);
    //     return; // New alert has lower priority, ignore it
    // }
    // if (districtId != settings.getInt(HOME_DISTRICT)) {
    //     LOG.printf("[ALERT] Alert for district %d ignored, home district is %d\n", districtId, settings.getInt(HOME_DISTRICT));
    //     return; // Alert is not for home district
    // }
    const char* districtName = getNameById(DISTRICTS, districtId, MAX_REGIONS);
    LOG.printf("[ALERT] Home district %s status changed from %d to %d\n", districtName, alertBit, bit);
    
    // Звукові сповіщення (якщо увімкнено)
    if (settings.getBool(SOUND_ON_ALERT)) {
        switch (bit){
            case AlertModes::ALERT:
                if(needToPlaySound(SoundType::ALERT_ON)) playMelody(ALERT_ON);
                break;
            case AlertModes::DRONES:
                if(needToPlaySound(SoundType::DRONES)) playMelody(DRONES);
                break;
            case AlertModes::MISSILES:
                if(needToPlaySound(SoundType::MISSILES)) playMelody(MISSILES);
                break;
            case AlertModes::KABS:
                if(needToPlaySound(SoundType::KABS)) playMelody(KABS);
                break;
            case AlertModes::BALLISTIC:
                if(needToPlaySound(SoundType::BALLISTIC)) playMelody(BALLISTIC);
                break;
            case AlertModes::EXPLOSION:
                if(needToPlaySound(SoundType::EXPLOSIONS)) playMelody(EXPLOSIONS);
                break;
            case AlertModes::RECON_DRONES:
                if(needToPlaySound(SoundType::RECON_DRONES)) playMelody(RECON_DRONES);
                break;
            default:
                break;
        }
    }
    if (settings.getBool(SOUND_ON_ALERT_END) && bit == AlertModes::NO_ALERT) {
        if(needToPlaySound(SoundType::ALERT_OFF)) playMelody(ALERT_OFF);
    }
    
    // Показуємо повідомлення на дисплеї для будь-якого типу події
    display.showServiceMessage(getEventTypeName(bit), districtName, settings.getInt(DISPLAY_ALERT_MESSAGE_TIME) * 1000);
}

void animateLed(Adafruit_NeoPixel* strip, int map_mode, int led_position, int bit, int initialBit, uint16_t region_id, bool increase = true) {
    LOG.printf("[ANIMATION] LED %d: region %d to %d\n", led_position, region_id, bit); 

    if (strip == nullptr) {
        LOG.printf("[ANIMATION] LED %d: strip is nullptr\n", led_position);
        return;
    }

    if (strip == strip_bg && settings.getInt(BG_LED_MODE) != BgLedModes::HOME_REGION) {
        LOG.printf("[ANIMATION] SKIPPED %d: bg mode is not HOME_REGION, background animation skipped\n", led_position);
        return;
    }
    
    uint32_t color;
    uint32_t initialColor = animation.ledActualColor(strip, led_position);
    uint32_t period;
    uint32_t cycles;
    uint8_t startBrightness;
    uint8_t endBrightness = led.brightnessRelative(settings.getInt(BRIGHTNESS_ANIMATION_END));
    uint8_t ledCount;

    //int actualBit = getHighestActualBit(bit);

    // if (increase && actualBit != bit) {
    // //if (actualBit != bit) {
    //     LOG.printf("[ANIMATION] actualBit %d != %d. Animation aborted\n", actualBit, bit);
    //     return;
    // }

    switch (bit) {
        case -1: 
            color = animation.colorFromHex(settings.getString(COLOR_CLEAR));  
            startBrightness = led.brightnessRelative(settings.getInt(BRIGHTNESS_CLEAR));             
            animType = settings.getInt(ANIMATION_ALERT_OFF_TYPE);
            cycles = (settings.getInt(ALERT_OFF_TIME) * 1000)/settings.getInt(ANIMATION_ALERT_OFF_CYCLE_TIME);
            period = settings.getInt(ANIMATION_ALERT_OFF_CYCLE_TIME);
            break;
        case 0:
            color = animation.colorFromHex(settings.getString(COLOR_ALERT)); 
            animType = (increase) ? settings.getInt(ANIMATION_ALERT_ON_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            startBrightness = led.brightnessRelative(settings.getInt(BRIGHTNESS_ALERT));
            period = (increase) ? settings.getInt(ANIMATION_ALERT_ON_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(ALERT_ON_TIME) * 1000)/settings.getInt(ANIMATION_ALERT_ON_CYCLE_TIME) : 1;
            break;
        case 5: 
            color = animation.colorFromHex(settings.getString(COLOR_DRONES));
            startBrightness = led.brightnessRelative(settings.getInt(BRIGHTNESS_DRONES));
            animType = (increase) ? settings.getInt(ANIMATION_DRONE_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            period = (increase) ? settings.getInt(ANIMATION_DRONE_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(DRONE_TIME) * 1000)/settings.getInt(ANIMATION_DRONE_CYCLE_TIME) : 1; 
            break;
        case 6:
            color = animation.colorFromHex(settings.getString(COLOR_MISSILES));
            startBrightness = led.brightnessRelative(settings.getInt(BRIGHTNESS_MISSILES));
            animType = (increase) ? settings.getInt(ANIMATION_MISSILE_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            period = (increase) ? settings.getInt(ANIMATION_MISSILE_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(MISSILE_TIME) * 1000)/settings.getInt(ANIMATION_MISSILE_CYCLE_TIME) : 1; 
            break;
        case 7:
            color = animation.colorFromHex(settings.getString(COLOR_KABS));
            startBrightness = led.brightnessRelative(settings.getInt(BRIGHTNESS_KABS));
            animType = (increase) ? settings.getInt(ANIMATION_KAB_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            period = (increase) ? settings.getInt(ANIMATION_KAB_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(KAB_TIME) * 1000)/settings.getInt(ANIMATION_KAB_CYCLE_TIME) : 1;
            break;
        case 8:
            color = animation.colorFromHex(settings.getString(COLOR_BALLISTIC));
            startBrightness = led.brightnessRelative(settings.getInt(BRIGHTNESS_BALLISTIC));
            animType = (increase) ? settings.getInt(ANIMATION_BALLISTIC_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            period = (increase) ? settings.getInt(ANIMATION_BALLISTIC_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(BALLISTIC_TIME) * 1000)/settings.getInt(ANIMATION_BALLISTIC_CYCLE_TIME)  : 1;
            break;
        case 9:
            color = animation.colorFromHex(settings.getString(COLOR_EXPLOSION));
            startBrightness = led.brightnessRelative(settings.getInt(BRIGHTNESS_EXPLOSION));
            animType = (increase) ? settings.getInt(ANIMATION_EXPLOSION_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            period = (increase) ? settings.getInt(ANIMATION_EXPLOSION_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(EXPLOSION_TIME) * 1000)/settings.getInt(ANIMATION_EXPLOSION_CYCLE_TIME)  : 1;
            break;
        case 10:
            color = animation.colorFromHex(settings.getString(COLOR_RECON_DRONES));
            startBrightness = led.brightnessRelative(settings.getInt(BRIGHTNESS_RECON_DRONES));
            animType = (increase) ? settings.getInt(ANIMATION_RECON_DRONE_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            period = (increase) ? settings.getInt(ANIMATION_RECON_DRONE_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(RECON_DRONE_TIME) * 1000)/settings.getInt(ANIMATION_RECON_DRONE_CYCLE_TIME)  : 1;
            break;
        default:
            LOG.printf("[ANIMATION] LED %d: unknown bit\n", led_position);
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
        LOG.printf("[ERROR] Invalid strip for animation\n");
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
        bit,
        initialBit
    )) {
        LOG.printf("[ERROR] Failed to create animation\n");
        return;
    }
}

void updateSirenIfNeeded(int bit) {
    if (bit >= AlertModes::ALERT && homeAlertFlags == 0) {
        siren.setAlert();
    } else if (bit == AlertModes::NO_ALERT) {
        siren.clearAlert();
    }
}

void analyzeAlertChanges(const std::vector<AlertDiff>& diffs) {

}

void onMessageCallback(WebsocketsMessage msg) {
    checkFreeHeap("Websockets onMessageCallback");

    // Ігноруємо текстові повідомлення
    if (!msg.isBinary()) {
        LOG.printf("[WEBSOCKET] Message in not binary\n");
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
    if (type != TYPE_ALERTS_BATCH && type != TYPE_NOTIFICATIONS_BATCH && type != TYPE_WEATHER_BATCH && type != TYPE_FIRMWARE_UPDATE_BETA_BATCH && type != TYPE_FIRMWARE_UPDATE_PROD_BATCH) {
        LOG.printf("[ERROR] message type unknown\n");
        return;
    }

    // ітератор по пакету
    uint8_t ledCount;
    size_t bodyLen;

    if (type == TYPE_FIRMWARE_UPDATE_BETA_BATCH || type == TYPE_FIRMWARE_UPDATE_PROD_BATCH) {
        bool isBeta = (type == TYPE_FIRMWARE_UPDATE_BETA_BATCH);
        LOG.printf("[WEBSOCKET] TYPE_FIRMWARE_UPDATE_%s_BATCH received\n", isBeta ? "BETA" : "PROD");
        bodyLen = len - HEADER_SZ;

        // payloadLen має ділитися на RECORD_FW
        if (bodyLen == 0 || (bodyLen % RECORD_FW) != 0) {
            // некоректний фрейм — пропускаємо і реконнектимось
            LOG.printf("[ERROR] bodyLen == 0 || (bodyLen % RECORD_FW) != 0\n");
            requestWebsocketReconnect();
            return;
        }

        fwUpdate.processBatch(data + HEADER_SZ, bodyLen, isBeta);
        return;
    }
    // ─── Новий обробник нотифікацій: без heap, без персистентності ───────────────
    if (type == TYPE_NOTIFICATIONS_BATCH) {
        LOG.printf("[WEBSOCKET] TYPE_NOTIFICATIONS_BATCH received\n");

        if (len < HEADER_SZ) {
            LOG.printf("[ERROR] TYPE_NOTIFICATIONS_BATCH: frame too short (%u)\n", (unsigned)len);
            requestWebsocketReconnect();
            return;
        }
        bodyLen = len - HEADER_SZ;
        if (bodyLen == 0 || (bodyLen % RECORD_SZ) != 0) {
            LOG.printf("[ERROR] TYPE_NOTIFICATIONS_BATCH: bad bodyLen %u\n", (unsigned)bodyLen);
            requestWebsocketReconnect();
            return;
        }

        size_t count = bodyLen / RECORD_SZ;
        const uint8_t* ptr = data + HEADER_SZ;

        LOG.printf("[WEBSOCKET] TYPE_NOTIFICATIONS_BATCH data processing\n");

        for (size_t i = 0; i < count; ++i) {
            uint16_t region_id = uint16_t(ptr[0]) | (uint16_t(ptr[1]) << 8);
            uint16_t flags16   = uint16_t(ptr[2]) | (uint16_t(ptr[3]) << 8);
            ptr += RECORD_SZ;

            const RegionLedMapMeta* meta = getRegionEntry(region_id);
            if (!meta) {
                LOG.printf("[WEBSOCKET] notification region %d:\t\t0x%04X skipped - not in map\n", region_id, flags16);
                continue;
            }
            LOG.printf("[WEBSOCKET] notification region %d:\t\t0x%04X\n", region_id, flags16);

            // Знаходимо поточний найвищий alert-bit для LEDs цього регіону з кешу
            // (без heap — ledBitCache вже містить актуальний стан після ALERTS_BATCH)
            int highestBitRegion = -1;
            const uint16_t* leds = getRegionLeds(meta);
            for (uint8_t j = 0; j < meta->led_count; ++j) {
                int led = (int)leds[j];
                if (led < 0 || led >= MAX_LEDS_STRIP_MAIN) continue;
                int8_t bit = ledBitCache[led];
                if (bit != -1 && (highestBitRegion == -1 || hasHigherPriority((int)bit, highestBitRegion))) {
                    highestBitRegion = (int)bit;
                }
            }

            // Notification bit — false: не вимагати наявності bit 0 (air alert)
            int actualBitDiff = findHighestBit16(flags16, false);
            LOG.printf("[WEBSOCKET]   highestBitRegion %d, actualBitDiff %d\n", highestBitRegion, actualBitDiff);

            // Анімуємо тільки якщо нотифікація має вищий пріоритет, ніж поточна тривога
            if (hasHigherPriority(actualBitDiff, highestBitRegion)) {
                for (uint8_t j = 0; j < meta->led_count; ++j) {
                    int led = (int)leds[j];
                    if (led < 0 || led >= MAX_LEDS_STRIP_MAIN) continue;
                    animateLed(strip_main, MapModes::ALERT, led, actualBitDiff, highestBitRegion, region_id, true);
                }
                // Домашній регіон — окрема анімація strip_bg
                if ((uint16_t)settings.getInt(HOME_DISTRICT) == region_id) {
                    LOG.printf("[WEBSOCKET]   Animating home district LEDs: region %d, bit %d\n", region_id, actualBitDiff);
                    animateLed(strip_bg, MapModes::ALERT, 0, actualBitDiff, highestBitRegion, region_id, true);
                    alertAction(actualBitDiff, region_id);
                }
            }   
        }
    }


    // ─── Новий обробник тривог: без heap-алокацій ──────────────────────────────
    if (type == TYPE_ALERTS_BATCH) {
        LOG.printf("[WEBSOCKET] TYPE_ALERTS_BATCH received\n");

        if (len < HEADER_SZ + HASH_SZ) {
            LOG.printf("[ERROR] TYPE_ALERTS_BATCH: frame too short (%u)\n", (unsigned)len);
            requestWebsocketReconnect();
            return;
        }
        bodyLen = len - HEADER_SZ - HASH_SZ;
        if (bodyLen == 0 || (bodyLen % RECORD_SZ) != 0) {
            LOG.printf("[ERROR] TYPE_ALERTS_BATCH: bad bodyLen %u\n", (unsigned)bodyLen);
            requestWebsocketReconnect();
            return;
        }

        uint16_t actualHash = (static_cast<uint16_t>(data[1]) << 8) | data[2];
        uint16_t prevHash   = (static_cast<uint16_t>(data[3]) << 8) | data[4];
        LOG.printf("[WEBSOCKET] hash check, local: [0x%04X] prev: [0x%04X], actual: [0x%04X]\n",
                   alertsHash, prevHash, actualHash);

        if (prevHash != alertsHash && isFirstDataFetchCompleted) {
            LOG.printf("[ERROR] TYPE_ALERTS_BATCH: prevHash != alertsHash\n");
            requestWebsocketReconnect();
            return;
        }

        size_t count = bodyLen / RECORD_SZ;
        const uint8_t* ptr = data + HEADER_SZ + HASH_SZ;

        // ── Фаза 1: оновити alertsFlat, позначити dirty LED ──────────────────
        for (size_t i = 0; i < count; ++i) {
            uint16_t region_id = uint16_t(ptr[0]) | (uint16_t(ptr[1]) << 8);
            uint16_t flags16   = uint16_t(ptr[2]) | (uint16_t(ptr[3]) << 8);
            ptr += RECORD_SZ;

            // getRegionEntry перевіряє наявність у currentMap (region_id — довільний ID, не індекс)
            const RegionLedMapMeta* meta = getRegionEntry(region_id);
            if (!meta) {
                LOG.printf("[WEBSOCKET] alert region %d:\t\t0x%04X skipped - not in map\n", region_id, flags16);
                continue;
            }
            int flat_idx = getRegionFlatIdx(region_id);  // позиція в currentMap.meta (0..currentMap.size-1)
            if (flat_idx < 0 || flat_idx >= (int)(MAX_REGIONS + 1)) {
                LOG.printf("[WEBSOCKET] alert region %d:\t\t0x%04X skipped - invalid flat_idx %d\n",
                        region_id, flags16, flat_idx);
                continue;
            }

            if (alertsFlat[flat_idx] == flags16) {
                continue; // нічого не змінилось
            }
            LOG.printf("[WEBSOCKET] alert region %d:\t\t0x%04X -> 0x%04X\n",
                       region_id, alertsFlat[flat_idx], flags16);

            // Позначаємо dirty LED (бітсет: 1 bit на LED, без зайвого масиву s_ledOldBit)
            // ledBitCache[led] ще не змінюється до фази 2 — тому він є "старим" значенням
            const uint16_t* leds = getRegionLeds(meta);
            for (uint8_t j = 0; j < meta->led_count; ++j) {
                int led = (int)leds[j];
                if (led < 0 || led >= MAX_LEDS_STRIP_MAIN) continue;
                s_ledDirty[led >> 3] |= (1u << (led & 7));
            }

            alertsFlat[flat_idx] = flags16; // оновлюємо одразу
        }

        // ── Фаза 2: обчислити нові біти і запустити анімації ─────────────────
        if (isFirstDataFetchCompleted) {
            for (int led = 0; led < MAX_LEDS_STRIP_MAIN; ++led) {
                if (!(s_ledDirty[led >> 3] & (1u << (led & 7)))) continue;
                s_ledDirty[led >> 3] &= ~(1u << (led & 7)); // скидаємо прапор

                int8_t oldBit = ledBitCache[led]; // не змінювався в фазі 1
                uint16_t new_region = 0;
                int8_t newBit = (int8_t)findHighestBitForLedFlat(led, &new_region);

                if (newBit == oldBit) continue; // bit не змінився

                bool increase = hasHigherPriority((int)newBit, (int)oldBit);
                LOG.printf("[WEBSOCKET] LED %d: bit %d -> %d (%s), region %d\n",
                           led, oldBit, newBit, increase ? "increase" : "decrease", new_region);
                animateLed(strip_main, MapModes::ALERT, led, (int)newBit, (int)oldBit, new_region, increase);

                ledBitCache[led] = newBit;
            }

            // Домашній регіон (strip_bg + siren + API)
            int localAlertBit = findHighestBitForRegionFlat(settings.getInt(HOME_DISTRICT));
            if (localAlertBit != alertBit) {
                bool homeIncrease = hasHigherPriority(localAlertBit, alertBit);
                if (homeIncrease) {
                    LOG.printf("[WEBSOCKET] Home district: region %d bit %d increase\n",
                               settings.getInt(HOME_DISTRICT), localAlertBit);
                    alertAction(localAlertBit, settings.getInt(HOME_DISTRICT));
                } else {
                    LOG.printf("[WEBSOCKET] Home district: region %d bit %d decrease\n",
                               settings.getInt(HOME_DISTRICT), localAlertBit);
                    if (localAlertBit == -1) alertAction(localAlertBit, settings.getInt(HOME_DISTRICT));
                }
                animateLed(strip_bg, MapModes::ALERT, 0, localAlertBit, alertBit,
                           settings.getInt(HOME_DISTRICT), homeIncrease);
                updateSirenIfNeeded(localAlertBit);
            }
            alertBit = localAlertBit;
            int homeIdx = getRegionFlatIdx(settings.getInt(HOME_DISTRICT));
            uint16_t homeFlags = (homeIdx >= 0) ? alertsFlat[homeIdx] : 0;
            api.setHomeAlert(homeFlags);
            homeAlertFlags = homeFlags;
        }

        // ── Init-fetch: перший пакет після підключення ────────────────────────
        if (!isFirstDataFetchCompleted) {
            LOG.printf("[WEBSOCKET] TYPE_ALERTS_BATCH init processing\n");

            // Заповнюємо кеш після першого повного стану
            rebuildLedBitCache();

            if (strip_main != nullptr) {
                animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
                    for (uint16_t i = 0; i < strip->numPixels(); i++) {
                        strip->setPixelColor(i, animation.ledActualColor(strip, i));
                    }
                    strip->show();
                });
            }
            if (strip_bg != nullptr && settings.getInt(BG_LED_MODE) == BgLedModes::HOME_REGION) {
                animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
                    uint32_t color = animation.stripActualColor(strip);
                    for (uint16_t i = 0; i < strip->numPixels(); i++) {
                        strip->setPixelColor(i, color);
                    }
                    strip->show();
                });
            }
            alertBit = findHighestBitForRegionFlat(settings.getInt(HOME_DISTRICT));
            int homeInitIdx = getRegionFlatIdx(settings.getInt(HOME_DISTRICT));
            uint16_t homeFlags = (homeInitIdx >= 0) ? alertsFlat[homeInitIdx] : 0;
            api.setHomeAlert(homeFlags);
            homeAlertFlags = homeFlags;
            uint8_t encodedTemp = temperatureMap[settings.getInt(HOME_DISTRICT)];
            api.setHomeDistrictTemp(decodeTemperature(encodedTemp));
            updateSirenIfNeeded(alertBit);

            // скидаємо dirty-прапори (могли бути встановлені в фазі 1)
            memset(s_ledDirty, 0, sizeof(s_ledDirty));
        }

        isFirstDataFetchCompleted = true;
        startup    = false;
        alertsHash = actualHash;
    }

    if(type == TYPE_WEATHER_BATCH) {
        LOG.printf("[WEBSOCKET] TYPE_WEATHER_BATCH received\n");
        bodyLen = len - HEADER_SZ;

        // payloadLen має ділитися на RECORD_LZ
        if (bodyLen == 0 || (bodyLen % RECORD_LZ) != 0) {
            // некоректний фрейм — пропускаємо і реконнектимось
            LOG.printf("[ERROR] bodyLen == 0 || (bodyLen % RECORD_LZ) != 0\n");
            requestWebsocketReconnect();
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
            temperatureMap[region_id] = flags8; // Зберігаємо температуру для регіону
            LOG.printf("[WEBSOCKET] weather region %u:\tflags8=%u\n", region_id, flags8);
            ptr += RECORD_LZ; // перехід до наступного запису (2B region_id + 1B flags8)
        }

        // Оновлюємо кеш авто-меж температури (медіана + вікно 20°C з якорями)
        if (!temperatureMap.empty()) {
            size_t size = temperatureMap.size();
            if (size == 1) {
                // Один регіон — вікно ±10°C
                int t = decodeTemperature(temperatureMap.begin()->second);
                weatherAutoMinTemp = constrain(t - 10, -40, 40);
                weatherAutoMaxTemp = constrain(t + 10, -40, 40);
            } else {
                // 1. Копіюємо температури у тимчасовий буфер та знаходимо екстремуми
                int tempBuffer[size];
                int actualMin = 1000;
                int actualMax = -1000;
                size_t idx = 0;
                for (const auto& kv : temperatureMap) {
                    int t = decodeTemperature(kv.second);
                    tempBuffer[idx++] = t;
                    if (t < actualMin) actualMin = t;
                    if (t > actualMax) actualMax = t;
                }
                // 2. Сортуємо для знаходження медіани
                std::sort(tempBuffer, tempBuffer + size);
                int median = tempBuffer[size / 2];
                // 3. Формуємо асиметричне вікно залежно від діапазону медіани
                int windowDown, windowUp;
                if (median < 0) {
                    // Мороз: менше вниз, більше вгору → фактичні температури в холодному спектрі
                    windowDown = 5;
                    windowUp = 15;
                } else if (median > 20) {
                    // Спека: більше вниз, менше вгору → фактичні температури в теплому спектрі
                    windowDown = 15;
                    windowUp = 5;
                } else {
                    // Помірний діапазон: симетрично ±10°C
                    windowDown = 10;
                    windowUp = 10;
                }
                int targetMin = median - windowDown;
                int targetMax = median + windowUp;
                // 4. Якорі: розширюємо вікно, якщо реальні дані виходять за нього
                if (actualMin < targetMin) targetMin = actualMin;
                if (actualMax > targetMax) targetMax = actualMax;
                // 5. Запобіжник: якщо всі температури однакові
                if (targetMax == targetMin) {
                    targetMax += 1;
                    targetMin -= 1;
                }
                weatherAutoMinTemp = constrain(targetMin, -40, 40);
                weatherAutoMaxTemp = constrain(targetMax, -40, 40);
            }
            weatherAutoBoundsValid = true;
        } else {
            weatherAutoBoundsValid = false;
        }

        uint8_t encodedTemp = temperatureMap[settings.getInt(HOME_DISTRICT)];
        int homeTemp = decodeTemperature(encodedTemp);
        api.setHomeDistrictTemp(homeTemp);
        adaptStripColorsAndBrightness();
    }
    
    checkFreeHeap("Websockets data processing");
}

void logWebsocketCloseReason(websockets::CloseReason reason) {
    switch (reason) {
        case websockets::CloseReason_None:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: None (-1)\n");
            break;
        case websockets::CloseReason_NormalClosure:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: Normal Closure (1000)\n");
            break;
        case websockets::CloseReason_GoingAway:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: Going Away (1001)\n");
            break;
        case websockets::CloseReason_ProtocolError:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: Protocol Error (1002)\n");
            break;
        case websockets::CloseReason_UnsupportedData:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: Unsupported Data (1003)\n");
            break;
        case websockets::CloseReason_NoStatusRcvd:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: No Status Received (1005)\n");
            break;
        case websockets::CloseReason_AbnormalClosure:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: Abnormal Closure (1006)\n");
            break;
        case websockets::CloseReason_InvalidPayloadData:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: Invalid Payload Data (1007)\n");
            break;
        case websockets::CloseReason_PolicyViolation:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: Policy Violation (1008)\n");
            break;
        case websockets::CloseReason_MessageTooBig:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: Message Too Big (1009)\n");
            break;
        case websockets::CloseReason_InternalServerError:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: Internal Server Error (1011)\n");
            break;
        default:
            LOG.printf("[WEBSOCKET] WebSocket CloseReason: Unknown (%d)\n", static_cast<int>(reason));
            break;
    }
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if (event == WebsocketsEvent::ConnectionOpened) {
        websocketConnected = true;
        servicePin(DATA);
        LOG.printf("[WEBSOCKET] connection opened\n");
        
        websocketLastPingTime = millis();
    } else if (event == WebsocketsEvent::ConnectionClosed) {
        websocketConnected = false;
        servicePin(DATA);
        LOG.printf("[WEBSOCKET] connection closed\n");
        LOG.printf("[MEMORY] Heap before close: %u\n", ESP.getFreeHeap());
        //websocket.close();
        auto reason = websocket.getCloseReason();
        logWebsocketCloseReason(reason);
        delay(500);
        LOG.printf("[MEMORY] Heap after close: %u\n", ESP.getFreeHeap());
    } else if (event == WebsocketsEvent::GotPing) {
        LOG.printf("[WEBSOCKET] ping, payload: [%s], len: %d\n", data.c_str(), data.length());
        websocketLastPingTime = millis();
    } else if (event == WebsocketsEvent::GotPong) {
        LOG.printf("[WEBSOCKET] pong, payload: [%s], len: %d\n", data.c_str(), data.length());
    }
}

void socketConnect() {
    LOG.printf("[WEBSOCKET] connection start...\n");
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
        websocketConnected = true;
        servicePin(DATA);
        if (websocketReconnect) {
            animation.clearAllAnimations();
            websocketReconnect = false;
        }
        //clearAllAlertsMaps();
        //clearAllWeatherMaps();
        //animation.clearAllAnimations();
        LOG.printf("[WEBSOCKET] connection time - %d ms\n", millis() - startTime);
        char chipIdInfo[25];
        snprintf(chipIdInfo, sizeof(chipIdInfo), "chip_id:%s", chipID);
        LOG.printf("[WEBSOCKET] %s\n", chipIdInfo);
        websocket.send(chipIdInfo);
        char firmwareInfo[100];
        snprintf(firmwareInfo, sizeof(firmwareInfo), "firmware:%s_%s", fwUpdate.getCurrentVersion(), settings.getString(ID));
        LOG.printf("[WEBSOCKET] %s\n", firmwareInfo);
        websocket.send(firmwareInfo);
        char userInfo[250];
        JsonDocument userInfoJson;
        userInfoJson["legacy"] = settings.getInt(HARDWARE);
        snprintf(userInfo, sizeof(userInfo), "user_info:%s", userInfoJson.as<String>().c_str());
        LOG.printf("[WEBSOCKET] %s\n", userInfo);
        websocket.send(userInfo);
        websocket.ping("A");
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

bool isLedPinSupported(int pin) {
    bool supported = false;
    if (pin <= 0) {
        // Пін 0 або від'ємний вважаємо спеціальним випадком, який означає "без стрічки", тому підтримуємо його незалежно від того, чи є він у списку SUPPORTED_LEDS_PINS
        return true;
    }
    for (int supportedPin : SUPPORTED_LEDS_PINS) {
        if (pin == supportedPin) {
            supported = true;
            break;
        }
    }
    return supported;
}

void initStripMain() {
    StripStatus status;

    num_leds_main = hardwareConfig.getMainLedsCount();
    int mainLedPin = hardwareConfig.getMainLedPin();
    if (!isLedPinSupported(mainLedPin)) {
        LOG.printf("[LED] SKIP: strip_main (unsupported pin %d)\n", mainLedPin);
        return;
    }
    
    if (mainLedPin > 0) {
        int mainLedColorFormat = hardwareConfig.getMainLedColorFormat();
        uint8_t ledType = mainLedColorFormat + settings.getInt(MAIN_LED_FREQUENCY);
        LOG.printf("[LED] Initializing strip_main on pin %d with %d LEDs, type %d (format:%d + freq:%d)\n", 
                   mainLedPin, num_leds_main, ledType, 
                   mainLedColorFormat, settings.getInt(MAIN_LED_FREQUENCY));
        status = led.createStrip(strip_main, mainLedPin, num_leds_main, ledType);
        if (status != StripStatus::SUCCESS) {
            LOG.printf("[LED] ERROR: Failed to create strip_main: %d\n", status);
        } else {
            LOG.printf("[LED] SUCCESS: strip_main\n");
        }
    } else {
        LOG.printf("[LED] SKIP: strip_main (pin <= 0)\n");
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

    int bgLedPin = hardwareConfig.getBgLedPin();
    int bgLedCount = hardwareConfig.getBgLedsCount();

    if (!isLedPinSupported(bgLedPin)) {
        LOG.printf("[LED] SKIP: strip_bg (unsupported pin %d)\n", bgLedPin);
        return;
    }

    if (bgLedPin > 0 && bgLedCount > 0) {
        int bgLedColorFormat = hardwareConfig.getBgLedColorFormat();
        uint8_t ledType = bgLedColorFormat + settings.getInt(BG_LED_FREQUENCY);
        LOG.printf("[LED] Initializing strip_bg on pin %d with %d LEDs, type %d (format:%d + freq:%d)\n", 
                   bgLedPin, bgLedCount, ledType,
                   bgLedColorFormat, settings.getInt(BG_LED_FREQUENCY));
        status = led.createStrip(strip_bg, bgLedPin, bgLedCount, ledType);
        if (status != StripStatus::SUCCESS) {
            LOG.printf("[LED] ERROR: Failed to create strip_bg: %d\n", status);
        } else {
            LOG.printf("[LED] SUCCESS: strip_bg\n");
        }
    } else {
        LOG.printf("[LED] SKIP: strip_bg (pin <= 0 or count <= 0)\n");
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

    int serviceLedPin = hardwareConfig.getServiceLedPin();

    if (!isLedPinSupported(serviceLedPin)) {
        LOG.printf("[LED] SKIP: strip_service (unsupported pin %d)\n", serviceLedPin);
        return;
    }
    
    if (serviceLedPin > 0) {
        int serviceLedColorFormat = hardwareConfig.getServiceLedColorFormat();
        uint8_t ledType = serviceLedColorFormat + settings.getInt(SERVICE_LED_FREQUENCY);
        LOG.printf("[LED] Initializing strip_service on pin %d with %d LEDs, type %d (format:%d + freq:%d)\n", 
                   serviceLedPin, num_leds_service, ledType,
                   serviceLedColorFormat, settings.getInt(SERVICE_LED_FREQUENCY));
        status = led.createStrip(strip_service, serviceLedPin, num_leds_service, ledType);
        if (status != StripStatus::SUCCESS) {
            LOG.printf("[LED] ERROR: Failed to create strip_service: %d\n", status);
        } else {
            LOG.printf("[LED] SUCCESS: strip_service\n");
        }
    } else {
        LOG.printf("[LED] SKIP: strip_service (pin <= 0)\n");
    }
    servicePin(POWER);
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
    LOG.printf("[LED] Reconnecting strip_main with new pin configuration...\n");
    //analyzeMemoryFragmentation("before strip_main reconnection");
    animation.clearAllAnimations();
    logMemoryUsage("after clearing animations");
    //defragmentMemory("before strip_main cleanup");
    cleanupSingleStrip(strip_main, DefaultColors::OFF);
    //defragmentMemory("after strip_main cleanup");
    led.verifyStripCleanup(strip_main);
    //analyzeMemoryFragmentation("before strip_main recreation");
    initStripMain();
    LOG.printf("[LED] strip_main reconnection completed.\n");
    logMemoryUsage("after strip_main reconnection complete");
}

void reconnectStripBg() {
    LOG.printf("[LED] Reconnecting strip_bg with new pin configuration...\n");
    //analyzeMemoryFragmentation("before strip_bg reconnection");
    animation.clearAllAnimations();
    logMemoryUsage("after clearing animations");
    //defragmentMemory("before strip_bg cleanup");
    cleanupSingleStrip(strip_bg, DefaultColors::OFF);
    //defragmentMemory("after strip_bg cleanup");
    led.verifyStripCleanup(strip_bg);
    //analyzeMemoryFragmentation("before strip_bg recreation");
    initStripBg();
    LOG.printf("[LED] strip_bg reconnection completed.\n");
    logMemoryUsage("after strip_bg reconnection complete");
}

void reconnectStripService() {
    LOG.printf("[LED] Reconnecting strip_service with new pin configuration...\n");
    //analyzeMemoryFragmentation("before strip_service reconnection");
    animation.clearAllAnimations();
    logMemoryUsage("after clearing animations");
    //defragmentMemory("before strip_service cleanup");
    cleanupSingleStrip(strip_service, DefaultColors::OFF);
    //defragmentMemory("after strip_service cleanup");
    led.verifyStripCleanup(strip_service);
    //analyzeMemoryFragmentation("before strip_service recreation");
    initStripService();
    LOG.printf("[LED] strip_service reconnection completed.\n");
    logMemoryUsage("after strip_service reconnection complete");
}

// --- TIME Functions ---

static const TimezoneInfo* getTimezoneInfo(int timezoneId) {
    // Шукаємо timezone по ID в масиві TIMEZONE_OFFSETS
    for (int i = 0; i < TIMEZONES_COUNT; i++) {
        if (TIMEZONE_OFFSETS[i].id == timezoneId) {
            return &TIMEZONE_OFFSETS[i];
        }
    }
    // За замовчуванням повертаємо UTC+2 (Kyiv) - ID=0
    return &TIMEZONE_OFFSETS[0];
}

static void applyTimezoneSettings(int timezoneId) {
    const TimezoneInfo* tzInfo = getTimezoneInfo(timezoneId);
    LOG.printf("[TIME] Applying timezone ID %d (offset: %d hours %d minutes)\n", 
               timezoneId, tzInfo->offset, tzInfo->minutes);
    timeClient.setTimeZone(tzInfo->offset, tzInfo->minutes);
    
    // Налаштування DST
    if (tzInfo->hasDST) {
        if (currentDST != nullptr) {
            delete currentDST;
        }
        currentDST = new DSTime(
            tzInfo->dstStart[0], tzInfo->dstStart[1], tzInfo->dstStart[2], tzInfo->dstStart[3],
            tzInfo->dstEnd[0], tzInfo->dstEnd[1], tzInfo->dstEnd[2], tzInfo->dstEnd[3]
        );
        timeClient.setDSTauto(currentDST);
        LOG.printf("[TIME] DST enabled for this timezone\n");
    } else {
        if (currentDST != nullptr) {
            delete currentDST;
            currentDST = nullptr;
        }
        timeClient.setDSTauto(nullptr);
        LOG.printf("[TIME] DST not used for this timezone\n");
    }
}

static void printNtpStatus(NTPtime* timeClient) {
    switch (timeClient->NTPstatus()) {
        case 0:
            LOG.printf("[TIME] OK\n");
            LOG.printf("[TIME] Current date and time: %s\n", 
                timeClient->unixToString("DD.MM.YYYY hh:mm:ss").c_str());
            break;
        case 1:
            LOG.printf("[TIME] NOT_STARTED\n");
            break;
        case 2:
            LOG.printf("[TIME] NOT_CONNECTED_WIFI\n");
            break;
        case 3:
            LOG.printf("[TIME] NOT_CONNECTED_TO_SERVER\n");
            break;
        case 4:
            LOG.printf("[TIME] NOT_SENT_PACKET\n");
            break;
        case 5:
            LOG.printf("[TIME] WAITING_REPLY\n");
            break;
        case 6:
            LOG.printf("[TIME] TIMEOUT\n");
            break;
        case 7:
            LOG.printf("[TIME] REPLY_ERROR\n");
            break;
        case 8:
            LOG.printf("[TIME] NOT_CONNECTED_ETHERNET\n");
            break;
        default:
            LOG.printf("[TIME] UNKNOWN_STATUS\n");
            break;
    }
}

void syncTime(int8_t attempts) {
    timeClient.tick();
    if (timeClient.status() == UNIX_OK) return;
    LOG.printf("[TIME] Time not synced yet!\n");
    printNtpStatus(&timeClient);
    int8_t count = 1;
    while (timeClient.NTPstatus() != NTP_OK && count <= attempts) {
        LOG.printf("[TIME] Attempt #%d of %d\n", count, attempts);
        if (timeClient.NTPstatus() != NTP_WAITING_REPLY) {
        LOG.printf("[TIME] Force update!\n");
        timeClient.updateNow();
        }
        timeClient.tick();
        if (count < attempts) delay(1000);
        count++;
        printNtpStatus(&timeClient);
    }
    // Sync system clock after successful NTP sync
    if (timeClient.status() == UNIX_OK) {
        time_t ntpTime = timeClient.unixGMT();
        struct timeval tv = {ntpTime, 0};
        if (settimeofday(&tv, nullptr) == 0) {
            LOG.printf("[TIME] System clock synchronized with NTP\n");
        } else {
            LOG.printf("[TIME] Failed to sync system clock\n");
        }
    }
}

void adaptStripColorsAndBrightness() {
    if (strip_main != nullptr) {
        LOG.printf("[LED] Adjusting main strip colors and brightness\n");               
        animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
            for (uint16_t i = 0; i < strip->numPixels(); i++) {
                uint32_t color = animation.ledActualColor(strip, i);
                strip->setPixelColor(i, color);
            }
            strip->show();
        });
    }
    if (strip_bg != nullptr) {
        LOG.printf("[LED] Adjusting bg strip colors and brightness\n");
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
        LOG.printf("[LED] Adjusting service strip colors and brightness\n");
        animation.safeStripOperation(strip_service, [](Adafruit_NeoPixel* strip) {
            for(uint16_t i = 0; i < strip->numPixels(); i++) {
                uint32_t color = animation.ledActualColor(strip, i);
                strip->setPixelColor(i, color);
            }
            strip->show();
        });               
    }
}

// --- ALERT Functions ---


// --- BRIGHTNESS Functions ---
uint8_t getCurrentBrightnes() {

    // якщо нічний режим увімкнений, повертаємо нічну яскравість незалежно від інших налаштувань
    if (nightMode) {
        return settings.getInt(BRIGHTNESS_NIGHT);
    }
    // режим яскравості 0: завжди використовувати основну яскравість
    if (settings.getInt(BRIGHTNESS_MODE) == 0) {
        return settings.getInt(BRIGHTNESS);
    }
    // режим яскравості 1: використовувати денну або нічну яскравість залежно від часу доби
    if (settings.getInt(BRIGHTNESS_MODE) == 1) {
        return isItNightNow() ? settings.getInt(BRIGHTNESS_NIGHT) : settings.getInt(BRIGHTNESS_DAY);
    }
    // режим яскравості 2: використовувати денну або нічну яскравість залежно від рівня освітлення (якщо є датчик освітлення)
    if (settings.getInt(BRIGHTNESS_MODE) == 2 && lightSensor.isLightSensorAvailable()) {
        float lightLevel = lightSensor.getLightLevel();
        int threshold = settings.getInt(NIGHT_MODE_LIGHT_THRESHOLD);
        // Визначаємо день/ніч за рівнем освітлення
        if (!isnan(lightLevel)) {
            return lightLevel < threshold ? settings.getInt(BRIGHTNESS_NIGHT) : settings.getInt(BRIGHTNESS_DAY);
        }
    }
    // За замовчуванням повертаємо основну яскравість
    return settings.getInt(BRIGHTNESS);
}

void updateClimateData() {
    if (climate.isAnySensorAvailable()) {
        climate.read();
        float temp = NAN;
        float hum = NAN;
        float press = NAN;
        if (climate.isTemperatureAvailable()) {
            temp = climate.getTemperature();
        }
        if (climate.isHumidityAvailable()) {
            hum = climate.getHumidity();
        }
        if (climate.isPressureAvailable()) {
            press = climate.getPressure();
        }
        api.setClimateData(temp, hum, press);
    }
}

void updateLightLevelData() {
    if (lightSensor.isAnySensorAvailable()) {
        float lightLevel = lightSensor.getLightLevel();
        api.setLightLevel(lightLevel);
    }
}

void initSettings() {
    LOG.printf("[INIT] Init settings\n");
    settings.init();
    fwUpdate.init(VERSION);
    fwUpdate.applyActiveChannel(settings.getInt(FW_UPDATE_CHANNEL) == 1);
    
    // Перевіряємо чи є збережене значення hostname
    if (!settings.hasKey(BROADCAST_NAME)) {
        // Встановлюємо значення за замовчуванням: jaam_[chipId]
        char defaultHostname[20];
        snprintf(defaultHostname, sizeof(defaultHostname), "jaam_%s", chipID);
        settings.saveString(BROADCAST_NAME, defaultHostname);
        LOG.printf("[INIT] BROADCAST_NAME set to default: %s\n", defaultHostname);
    } else {
        LOG.printf("[INIT] BROADCAST_NAME: %s\n", settings.getString(BROADCAST_NAME));
    }
    
    // Реєструємо callback для автоматичного broadcast змін налаштувань до API
    settings.setChangeCallback([](Type type, int intValue, float fltValue, const char* strValue) {
      
        // Повідомляємо mDNS про зміни налаштувань
        mdnsService.onSettingsChange(type, intValue, fltValue, strValue);
        
        // Повідомляємо API про зміни налаштувань
        api.onSettingsChange(type, intValue, fltValue, strValue);

        // Повідомляємо Siren про зміни налаштувань
        siren.onSettingsChange(type, intValue, fltValue, strValue);
        
        // Обробляємо зміни налаштувань локально
        switch(type) {
            // Кольори та зовнішній вигляд (адаптація + анімації)
            case COLOR_CLEAR:
            case COLOR_ALERT:
            case COLOR_EXPLOSION:
            case COLOR_MISSILES:
            case COLOR_DRONES:
            case COLOR_RECON_DRONES:
            case COLOR_KABS:
            case COLOR_BALLISTIC:
            case COLOR_HOME_DISTRICT:
            case COLOR_BG:
            case COLOR_LAMP:
            case MAP_MODE:
            case BG_LED_MODE:
            case WEATHER_MIN_TEMP:
            case WEATHER_MAX_TEMP:
            case WEATHER_AUTO_BOUNDS:
            case ENABLE_KABS:
            case ENABLE_MISSILES:
            case ENABLE_DRONES:
            case ENABLE_RECON_DRONES:
            case ENABLE_BALLISTIC:
            case ENABLE_EXPLOSIONS:
                adaptStripColorsAndBrightness();
                handleAdaptAnimationColors();
                handleAdaptAnimationBrightness();
                break;
            
            // Домашній регіон (кольори + анімації + оновлення бітів)
            case HOME_DISTRICT:
                adaptStripColorsAndBrightness();
                handleAdaptAnimationColors();
                handleUpdateHomeAlertBit();
                handleAdaptAnimationBrightness();
                break;
            
            // Основна яскравість (загальна адаптація)
            // Яскравість для анімацій (кольори + яскравість анімацій)
            // Яскравість сервісних LED (тільки кольори)
            case BRIGHTNESS:
            case BRIGHTNESS_DAY:
            case BRIGHTNESS_NIGHT:
            case BRIGHTNESS_MODE:
            // Додатково перераховуємо CURRENT_BRIGHTNESS
                brightnessProcess();
            case DAY_START:
            case NIGHT_START:
            case NIGHT_MODE_LIGHT_THRESHOLD:
            case BRIGHTNESS_MIN:
            case BRIGHTNESS_MAX:
            case BRIGHTNESS_MAX_ACCEPT:
            case BRIGHTNESS_ALERT:
            case BRIGHTNESS_CLEAR:
            case BRIGHTNESS_EXPLOSION:
            case BRIGHTNESS_MISSILES:
            case BRIGHTNESS_DRONES:
            case BRIGHTNESS_RECON_DRONES:
            case BRIGHTNESS_KABS:
            case BRIGHTNESS_BALLISTIC:
            case BRIGHTNESS_HOME_DISTRICT:
            case BRIGHTNESS_BG:
            case BRIGHTNESS_ANIMATION_END:
            case BRIGHTNESS_LAMP:
            case BRIGHTNESS_SERVICE:
                adaptStripColorsAndBrightness();
                handleAdaptAnimationColors();
                handleAdaptAnimationBrightness();
                break;
            
            // Час анімацій
            case ANIMATION_ALERT_ON_CYCLE_TIME:
            case ANIMATION_ALERT_OFF_CYCLE_TIME:
            case ANIMATION_DRONE_CYCLE_TIME:
            case ANIMATION_RECON_DRONE_CYCLE_TIME:
            case ANIMATION_MISSILE_CYCLE_TIME:
            case ANIMATION_KAB_CYCLE_TIME:
            case ANIMATION_BALLISTIC_CYCLE_TIME:
            case ANIMATION_EXPLOSION_CYCLE_TIME:
                handleAdaptAnimationPeriod();
                break;
            
            // Тип анімацій
            case ANIMATION_ALERT_ON_TYPE:
            case ANIMATION_ALERT_OFF_TYPE:
            case ANIMATION_DRONE_TYPE:
            case ANIMATION_RECON_DRONE_TYPE:
            case ANIMATION_MISSILE_TYPE:
            case ANIMATION_KAB_TYPE:
            case ANIMATION_BALLISTIC_TYPE:
            case ANIMATION_EXPLOSION_TYPE:
                handleAdaptAnimationType();
                break;
            
            // Режим синхронізації анімацій
            case ENABLE_SYNC_ANIMATIONS:
                handleUpdateAnimationsMode();
                break;
            
            // LED MAIN конфігурація (формат/частота - тільки reconnect)
            case MAIN_LED_COLOR_FORMAT:
            case MAIN_LED_FREQUENCY:
                handleReconnectStrips(true, false, false);
                break;
            
            // LED MAIN пін/кількість (перерахунок + reconnect)
            case MAIN_LED_PIN:
            case MAIN_LED_COUNT:
                handleRecalculateLeds();
                handleReconnectStrips(true, false, false);
                break;
            
            // LED BG конфігурація (формат/частота - тільки reconnect)
            case BG_LED_COLOR_FORMAT:
            case BG_LED_FREQUENCY:
                handleReconnectStrips(false, true, false);
                break;
            
            // LED BG пін/кількість (перерахунок + reconnect)
            case BG_LED_PIN:
            case BG_LED_COUNT:
                handleRecalculateLeds();
                handleRegenerateBgColorMap();
                handleReconnectStrips(false, true, false);
                break;
            
            // LED SERVICE конфігурація (формат/частота/пін)
            case SERVICE_LED_COLOR_FORMAT:
            case SERVICE_LED_FREQUENCY:
            case SERVICE_LED_PIN:
                handleReconnectStrips(false, false, true);
                break;
            
            // Повна апаратна реконфігурація
            case HARDWARE:
                handleRecalculateLeds();
                handleReconnectStrips(true, true, true);
                handleReconfigureDisplay();
                adaptStripColorsAndBrightness();
                handleReconfigureButtons();
                handleReconfigureSound();
                handleReconfigureSensors();
                handleReconnectWebsocket();
                break;
            
            // Режими районів (перерахунок LED)
            case DISTRICT_MODE_KYIV:
            case DISTRICT_MODE_KHARKIV:
            case DISTRICT_MODE_ZP:
            case KYIV_LED:
                handleRecalculateLeds();
                break;
            
            // Налаштування дисплею
            case DISPLAY_MODEL:
            case DISPLAY_HEIGHT:
            case DISPLAY_ROTATION:
            case INVERT_DISPLAY:
                handleReconfigureDisplay();
                break;
            
            // Корекція сенсорів клімату
            case TEMP_CORRECTION:
            case HUM_CORRECTION:
            case PRESSURE_CORRECTION:
                handleAdaptClimate();
                break;
            
            // Звукова конфігурація (джерело + піни)
            case SOUND_SOURCE:
            case BUZZER_PIN:
            case DF_RX_PIN:
            case DF_TX_PIN:
                handleReconfigureSound();
                break;
            
            // Гучність мелодій
            case MELODY_VOLUME_DAY:
            case MELODY_VOLUME_NIGHT:
                handleAdaptVolume();
                break;
            
            // Моніторинг батареї
            case ENABLE_BATTERY_MONITORING:
            case BATTERY_PIN:
                handleUpdateBatteryPin();
                break;
            
            // Конфігурація кнопок
            case USE_TOUCH_BUTTON_1:
            case USE_TOUCH_BUTTON_2:
            case USE_TOUCH_BUTTON_3:
            case BUTTON_1_MODE:
            case BUTTON_2_MODE:
            case BUTTON_3_MODE:
            case BUTTON_1_MODE_LONG:
            case BUTTON_2_MODE_LONG:
            case BUTTON_3_MODE_LONG:
            case BUTTON_1_PIN:
            case BUTTON_2_PIN:
            case BUTTON_3_PIN:
                handleReconfigureButtons();
                break;
            
            // Часовий пояс
            case TIME_ZONE:
                handleUpdateTimezone();
                break;
            
            // Вивід логів у Serial/Telnet
            case LOGS_ENABLED:
                loggingStream.setLogsEnabled(intValue == 1);
                break;

            // NTP сервер
            case NTP_HOST:
                handleUpdateNtpHost();
                break;
            
            // WebSocket сервер
            case WS_SERVER_HOST:
            case WS_SERVER_PORT:
                handleReconnectWebsocket();
                break;

            case CLOCK_FONT:
            case DISPLAY_MODE:
            case DISPLAY_OFF_AT_NIGHT:
                displayProcess();
                break;
            
            // Увімкнення/вимкнення попереднього перегляду
            case ENABLE_ANIMATION_PREVIEW:
                if (intValue == 0) {
                    // Зупиняємо попередній перегляд при вимкненні
                    animation.stopPreview();
                }
                break;
    
            // Зміна каналу оновлення прошивки — перераховуємо стан з уже отриманих списків
            case NEW_FW_NOTIFICATION:
            case FW_UPDATE_CHANNEL:
                fwUpdate.applyActiveChannel(settings.getInt(FW_UPDATE_CHANNEL) == 1);
                break;

            default:
                // Інші налаштування не потребують додаткової обробки
                break;
        }
        
        // Попередній перегляд анімації (якщо увімкнено)
        if (settings.getBool(ENABLE_ANIMATION_PREVIEW)) {
            int8_t eventType = -2;  // -2 означає "не визначено"
            uint16_t animType = 0;
            uint32_t period = 1000;
            uint32_t color = 0xFF0000;
            uint8_t brightness = 255;
            bool isAnimationParam = false;
            
            // Визначаємо тип події та одразу заповнюємо параметри
            switch (type) {
                // Параметри для ALERT
                case ANIMATION_ALERT_ON_TYPE:
                case ANIMATION_ALERT_ON_CYCLE_TIME:
                case COLOR_ALERT:
                    eventType = AlertModes::ALERT;
                    animType = settings.getInt(ANIMATION_ALERT_ON_TYPE);
                    period = settings.getInt(ANIMATION_ALERT_ON_CYCLE_TIME);
                    color = animation.colorFromHex(settings.getString(COLOR_ALERT));
                    brightness = settings.getInt(BRIGHTNESS_ALERT);
                    isAnimationParam = true;
                    break;
                    
                // Параметри для NO_ALERT (скасування тривоги)
                case ANIMATION_ALERT_OFF_TYPE:
                case ANIMATION_ALERT_OFF_CYCLE_TIME:
                case COLOR_CLEAR:
                    eventType = AlertModes::NO_ALERT;
                    animType = settings.getInt(ANIMATION_ALERT_OFF_TYPE);
                    period = settings.getInt(ANIMATION_ALERT_OFF_CYCLE_TIME);
                    color = animation.colorFromHex(settings.getString(COLOR_CLEAR));
                    brightness = settings.getInt(BRIGHTNESS_CLEAR);
                    isAnimationParam = true;
                    break;
                    
                // Параметри для EXPLOSION
                case ANIMATION_EXPLOSION_TYPE:
                case ANIMATION_EXPLOSION_CYCLE_TIME:
                case COLOR_EXPLOSION:
                    eventType = AlertModes::EXPLOSION;
                    animType = settings.getInt(ANIMATION_EXPLOSION_TYPE);
                    period = settings.getInt(ANIMATION_EXPLOSION_CYCLE_TIME);
                    color = animation.colorFromHex(settings.getString(COLOR_EXPLOSION));
                    brightness = settings.getInt(BRIGHTNESS_EXPLOSION);
                    isAnimationParam = true;
                    break;
                    
                // Параметри для DRONES
                case ANIMATION_DRONE_TYPE:
                case ANIMATION_DRONE_CYCLE_TIME:
                case COLOR_DRONES:
                    eventType = AlertModes::DRONES;
                    animType = settings.getInt(ANIMATION_DRONE_TYPE);
                    period = settings.getInt(ANIMATION_DRONE_CYCLE_TIME);
                    color = animation.colorFromHex(settings.getString(COLOR_DRONES));
                    brightness = settings.getInt(BRIGHTNESS_DRONES);
                    isAnimationParam = true;
                    break;
                    
                // Параметри для RECON_DRONES
                case ANIMATION_RECON_DRONE_TYPE:
                case ANIMATION_RECON_DRONE_CYCLE_TIME:
                case COLOR_RECON_DRONES:
                    eventType = AlertModes::RECON_DRONES;
                    animType = settings.getInt(ANIMATION_RECON_DRONE_TYPE);
                    period = settings.getInt(ANIMATION_RECON_DRONE_CYCLE_TIME);
                    color = animation.colorFromHex(settings.getString(COLOR_RECON_DRONES));
                    brightness = settings.getInt(BRIGHTNESS_RECON_DRONES);
                    isAnimationParam = true;
                    break;
                    
                // Параметри для MISSILES
                case ANIMATION_MISSILE_TYPE:
                case ANIMATION_MISSILE_CYCLE_TIME:
                case COLOR_MISSILES:
                    eventType = AlertModes::MISSILES;
                    animType = settings.getInt(ANIMATION_MISSILE_TYPE);
                    period = settings.getInt(ANIMATION_MISSILE_CYCLE_TIME);
                    color = animation.colorFromHex(settings.getString(COLOR_MISSILES));
                    brightness = settings.getInt(BRIGHTNESS_MISSILES);
                    isAnimationParam = true;
                    break;
                    
                // Параметри для KABS
                case ANIMATION_KAB_TYPE:
                case ANIMATION_KAB_CYCLE_TIME:
                case COLOR_KABS:
                    eventType = AlertModes::KABS;
                    animType = settings.getInt(ANIMATION_KAB_TYPE);
                    period = settings.getInt(ANIMATION_KAB_CYCLE_TIME);
                    color = animation.colorFromHex(settings.getString(COLOR_KABS));
                    brightness = settings.getInt(BRIGHTNESS_KABS);
                    isAnimationParam = true;
                    break;
                    
                // Параметри для BALLISTIC
                case ANIMATION_BALLISTIC_TYPE:
                case ANIMATION_BALLISTIC_CYCLE_TIME:
                case COLOR_BALLISTIC:
                    eventType = AlertModes::BALLISTIC;
                    animType = settings.getInt(ANIMATION_BALLISTIC_TYPE);
                    period = settings.getInt(ANIMATION_BALLISTIC_CYCLE_TIME);
                    color = animation.colorFromHex(settings.getString(COLOR_BALLISTIC));
                    brightness = settings.getInt(BRIGHTNESS_BALLISTIC);
                    isAnimationParam = true;
                    break;
                    
                default:
                    break;
            }
            
            // Запуск попереднього перегляду
            if (isAnimationParam) {
                // Показуємо повідомлення на дисплеї
                display.showServiceMessage(getEventTypeName(eventType), "Анімація:", 5000);
                animation.startPreview(eventType, animType, color, period, brightness);
            }
        }
    });
}

void initDisplay() {
    LOG.printf("[INIT] Init display\n");
    int displayModel = hardwareConfig.getDisplayModel();
    int displayHeight = hardwareConfig.getDisplayHeight();
    int displayRotation = hardwareConfig.getDisplayRotation();
    display.begin(static_cast<JaamDisplayType>(displayModel), static_cast<JaamDisplayHeight>(displayHeight));
    display.invertDisplay(settings.getBool(INVERT_DISPLAY));
    display.rotateDisplay(static_cast<JaamDisplayRotation>(displayRotation));
}

void initSensors() {
     // init light sensor, additional initialization for JAAM 2.1 with light sensor
     lightSensor.begin(settings.getInt(HARDWARE) == HARDWARE::JAAM_2_1);
     if (lightSensor.isLightSensorAvailable()) {
        lightSensor.read();
        api.setLightLevel(lightSensor.getLightLevel());
     }

    // init climate sensor
    climate.begin();
    // try to get climate sensor data
    climate.read();
    float temp = NAN;
    float hum = NAN;
    float press = NAN;
    if (climate.isTemperatureAvailable()) {
        temp = climate.getTemperature();
    }
    if (climate.isHumidityAvailable()) {
        hum = climate.getHumidity();
    }
    if (climate.isPressureAvailable()) {
        press = climate.getPressure();
    }
    api.setClimateData(temp, hum, press);
    
    // Set sensor availability for API
    api.setSensorAvailability(
        climate.isTemperatureAvailable(),
        climate.isHumidityAvailable(),
        climate.isPressureAvailable(),
        lightSensor.isLightSensorAvailable()
    );

    rebuildSensorsListItems();

}

void initSound() {
#if BUZZER_ENABLED || DFPLAYER_PRO_ENABLED
  sound.init(
    hardwareConfig.getBuzzerPin(), 
    hardwareConfig.getDfRxPin(), 
    hardwareConfig.getDfTxPin(),
    settings.getInt(MELODY_VOLUME_CURRENT),
    settings.getInt(MELODY_VOLUME_DAY),
    settings.getInt(MELODY_VOLUME_NIGHT)
  );
#endif

  int soundSource = settings.getInt(SOUND_SOURCE);
  LOG.printf("[SOUND] source setting: %d\n", soundSource);

#if BUZZER_ENABLED
  // Only initialize buzzer if SOUND_SOURCE is 0 (Buzzer) and buzzer is enabled
  if (sound.isBuzzerEnabled() && soundSource == 0) {
    sound.initBuzzer();
  }
#endif
#if DFPLAYER_PRO_ENABLED
  // Only initialize DFPlayer if SOUND_SOURCE is 1 (DFPlayer) and DFPlayer is enabled
  if (sound.isDFPlayerEnabled() && soundSource == 1) {
    sound.initDFPlayer();
  }
#endif

  // Set the actual sound source based on what was initialized
  if (soundSource == 0 && sound.isBuzzerEnabled()) {
    sound.setSoundSource(0);
  } else if (soundSource == 1 && sound.isDFPlayerConnected()) {
    sound.setSoundSource(1);
  } else {
    sound.setSoundSource(-1);
  }
  LOG.printf("[SOUND] active source: %d\n", sound.soundSource);
}


void initMapping() {
    LOG.printf("[INIT] Init mapping\n");
    // Ініціалізуємо мапінг регіонів
    generateCurrentRegionMap(hardwareConfig);
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
        LOG.printf("[TIME] Time already initialized, skipping...\n");
        return;
    }
    LOG.printf("[TIME] Init time\n");
    LOG.printf("[TIME] NTP host: %s\n", settings.getString(NTP_HOST));
    timeClient.setHost(settings.getString(NTP_HOST));
    applyTimezoneSettings(settings.getInt(TIME_ZONE));
    timeClient.setTimeout(5000); // 5 seconds waiting for reply
    timeClient.begin();
    syncTime(7);
    timeInitialized = true;
}

void initMDNS() {
    LOG.printf("[INIT] Init mDNS\n");
    mdnsService.setSettings(&settings);
    mdnsService.setDeviceInfo(chipID, fwUpdate.getCurrentVersion());
    mdnsService.begin();
}

void initApi() {
    LOG.printf("[INIT] Init API\n");
    
    // Ініціалізуємо API
    api.setSettings(&settings);
    api.setDeviceInfo(chipID, fwUpdate.getCurrentVersion());
    api.setNightMode(nightMode);
    api.setMapEnabled(!isMapOff);
    api.setDisplayEnabled(!isDisplayOff);
    
    SystemInfo info = getSystemInfo();
    api.setSystemInfo(
        info.usedMemory,
        info.uptime,
        info.wifiUptime,
        info.wifiSignal,
        info.websocketStatus,
        info.websocketUptime,
        info.cpuTemp
    );
    
    // Стартуємо API якщо дозволено в налаштуваннях
    api.reconfigure();
}

void initWeb() {
    static bool webInitialized = false;
    if (webInitialized) {
        LOG.printf("[WEB] Web already initialized, skipping...\n");
        return;
    }
    web.setSettings(&settings);
    web.setStorage(&storage);
    web.setDeviceInfo(chipID, fwUpdate.getCurrentVersion());
    
    web.begin(strip_main, strip_bg, strip_service);
    webInitialized = true;
}

static void wifiEvents(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED: {
            char softApIp[16];
            strcpy(softApIp, WiFi.softAPIP().toString().c_str());
            display.showServiceMessage(softApIp, "Введіть у браузері:");
            WiFi.removeEvent(wifiEvents);
            break;
        }
        default:
            break;
    }
}

void apCallback(WiFiManager* wifiManager) {
    String ssid = wifiManager->getConfigPortalSSID();
    display.showServiceMessage(ssid, "Підключіться до WiFi:");
    WiFi.onEvent(wifiEvents);
}

void saveConfigCallback() {
    display.showServiceMessage(wm.getWiFiSSID(true), "Збережено AP:");
    delay(2000);
    rebootDevice();
}

void initWifi() {
    if (!WiFiConfig::ENABLED) {
        LOG.printf("[WIFI] WiFi disabled in configuration\n");
        return;
    }

    LOG.printf("[WIFI] Initializing WiFi...\n");
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
    snprintf(apssid, sizeof(apssid), "JAAM_%s", chipID);
    if (!wm.autoConnect(apssid)) {
        LOG.printf("[WIFI] Reboot\n");
        rebootDevice(5000);
        return;
    }
    // Connected to WiFi
    LOG.printf("[WIFI] connected...yeey :)\n");
    lastWifiConnectTime = millis();
    wifiConnected = true;
    servicePin(WIFI);
    display.showServiceMessage("підключено!", "WiFi", 3000);
    wm.setHttpPort(WiFiConfig::WEB_PORT);
    wm.startWebPortal();
    initTime();
    initWeb();
    initApi();
    initMDNS();
    
    // // Спочатку спробуємо підключитися до збереженої мережі без WiFiManager
    // WiFi.begin();
    
    // // Чекаємо підключення 10 секунд
    // int attempts = 0;
    // while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    //     delay(500);
    //     attempts++;
    //     LOG.printf(".");
    // }
    
    // if (WiFi.status() == WL_CONNECTED) {
    //     lastWifiConnectTime = millis();
    //     wifiConnected = true;
    //     servicePin(WIFI);
    //     initTime();
    //     initWeb();
    //     LOG.printf("[WIFI] Connected to saved WiFi\n");
    //     LOG.printf("[WIFI] IP address: %s\n", WiFi.localIP().toString().c_str());
    //     return;
    // }

    // LOG.printf("[WIFI] Failed to connect to saved network\n");
    // LOG.printf("[WIFI] Starting WiFiManager...\n");
    
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
    //     snprintf(apName, sizeof(apName), "JAAM_%s", chipID);
        
    //     // Спроба підключення
    //     if (!wm_temp.autoConnect(apName)) {
    //         LOG.printf("[WIFI] Failed to connect. Rebooting...\n");
    //         rebootDevice(3000);
    //         return;
    //     }
        
    //     lastWifiConnectTime = millis();
    //     LOG.printf("[WIFI] Connected to WiFi via WiFiManager\n");
    //     LOG.printf("[WIFI] IP address: %s\n", WiFi.localIP().toString().c_str());
        
    // } // wm_temp destructor called here, freeing memory
    
    LOG.printf("[WIFI] initialization completed\n");
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
        LOG.printf("[ERROR] Failed to create stripMutex semaphore\n");
        return;
    }

    // Ініціалізуємо стрічки з бажаними пінами
    initStripMain();
    initStripBg();
    initStripService();   
    adaptStripColorsAndBrightness();

    // Тепер заповнюємо allLedsMain і allLedsBg після ініціалізації стрічок
    allLedsMain.clear();
    for (uint32_t i = 0; i < num_leds_main; ++i) {
        allLedsMain.push_back(i);
    }
    
    allLedsBg.clear();
    int bgCount = hardwareConfig.getBgLedsCount();
    if (bgCount > 0) {
        for (uint32_t i = 0; i < (uint32_t)bgCount; ++i) {
            allLedsBg.push_back(i);
        }
    }
}

void initBattery() {
    LOG.printf("[BATTERY] Initializing battery monitoring...\n");
    // Ініціалізація моніторингу батареї
    battery.begin();
}

void initStorage() {
    LOG.printf("[STORAGE] Initializing storage...\n");
    // Ініціалізація файлової системи
    if (storage.begin()) {
        storage.getStorageInfo();
        storage.getFilesInfo();
    }
}

void initSiren() {
    LOG.printf("[SIREN] Initializing siren...\n");
    siren.setSettings(&settings);
    siren.init();
}


void initButtons() {
  LOG.printf("[BUTTON] Init buttons\n");

  int button1Pin = hardwareConfig.getButton1Pin();
  int button2Pin = hardwareConfig.getButton2Pin();
  int button3Pin = hardwareConfig.getButton3Pin();

  LOG.printf("[BUTTON] button1 pin: %d\n", button1Pin);
  LOG.printf("[BUTTON] button1 touch: %d\n", settings.getBool(USE_TOUCH_BUTTON_1));
  buttons.setButton1Pin(button1Pin, !settings.getBool(USE_TOUCH_BUTTON_1));
  buttons.setButton1ClickListener(button1Click);
  buttons.setButton1LongClickListener(button1LongClick);
  buttons.setButton1DuringLongClickListener(button1DuringLongClick);

  LOG.printf("[BUTTON] button2 pin: %d\n", button2Pin);
  LOG.printf("[BUTTON] button2 touch: %d\n", settings.getBool(USE_TOUCH_BUTTON_2));
  buttons.setButton2Pin(button2Pin, !settings.getBool(USE_TOUCH_BUTTON_2));
  buttons.setButton2ClickListener(button2Click);
  buttons.setButton2LongClickListener(button2LongClick);
  buttons.setButton2DuringLongClickListener(button2DuringLongClick);

  LOG.printf("[BUTTON] button3 pin: %d\n", button3Pin);
  LOG.printf("[BUTTON] button3 touch: %d\n", settings.getBool(USE_TOUCH_BUTTON_3));
  buttons.setButton3Pin(button3Pin, !settings.getBool(USE_TOUCH_BUTTON_3));
  buttons.setButton3ClickListener(button3Click);
  buttons.setButton3LongClickListener(button3LongClick);
  buttons.setButton3DuringLongClickListener(button3DuringLongClick);
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
        LOG.printf("[ERROR] No available initialized strips\n");
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
    uint8_t endBrightness = led.brightnessRelative(settings.getInt(BRIGHTNESS_ANIMATION_END)); //   random(AnimationConfig::MIN_END_BRIGHTNESS, AnimationConfig::MAX_END_BRIGHTNESS + 1);
    
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
        LOG.printf("ERROR: Failed to create animation\n");
        return;
    }

    currentIdx++;
    if (currentIdx >= num_leds_main) {
        currentIdx = 0;
    }
}

void showClock() {
    if (timeClient.status() != UNIX_OK) {
        LOG.printf("[TIME] Clock not synced yet!\n");
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

void showNewFirmwareNotification() {
    int periodIndex = getCurrentPeriodIndex(settings.getInt(DISPLAY_MODE_TIME), 2, timeClient.second());
    char title[50];
    char message[50];
    if (periodIndex) {
        strcpy(title, "Доступне оновлення:");
        strncpy(message, fwUpdate.getNewVersion(), sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
    } else if (!isButtonActivated()) {
        strcpy(title, "Введіть у браузері:");
        strncpy(message, WiFi.localIP().toString().c_str(), sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
    } else {
        strcpy(title, "Для оновл. натисніть");
        snprintf(message, sizeof(message), "та тримайте кнопку %c", (char)24);
    }
    display.printMessage(message, title);
}

void playMinOfSilenceSound() {
  playMelody(MIN_OF_SILINCE);
}

void checkMinuteOfSilence()
{
    bool localMinOfSilence;

#if TEST_MIN_OF_SILENCE
    // Test mode: activate every 5 minutes (at 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 0 minute mark)
    localMinOfSilence = (settings.getBool(MIN_OF_SILENCE) == 1 && timeClient.minute() % 5 == 0);
    LOG.printf("[TEST MODE] localMinOfSilence: %d (minute: %d)\n", localMinOfSilence, timeClient.minute());
#else
    // Normal mode: activate at 9:00
    localMinOfSilence = (settings.getBool(MIN_OF_SILENCE) == 1 && timeClient.hour() == 9 && timeClient.minute() == 0);
#endif

    if (localMinOfSilence != minuteOfSilence) {
        minuteOfSilence = localMinOfSilence;

        if (minuteOfSilence) {
            // play mos beep every 2 sec during min of silence
            if (needToPlaySound(MIN_OF_SILINCE)) {
                clockBeepInterval = async.setInterval(playMinOfSilenceSound, 2000); // every 2 sec
            }
            // adapt colors on min of silence start
            adaptStripColorsAndBrightness();
            handleAdaptAnimationColors();
        } else {
            // turn off mos beep
            if (clockBeepInterval >= 0) {
                async.clearInterval(clockBeepInterval);
                clockBeepInterval = -1;
            }
            if (needToPlaySound(MIN_OF_SILINCE_END)) {
                playMelody(MIN_OF_SILINCE_END);
                uaAnthemPlaying = true;
            } else {
                // adapt colors on min of silence end
                adaptStripColorsAndBrightness();
                handleAdaptAnimationColors();
            }
        }
    }
}

void websocketProcess() {
    if (!wifiConnected) {
        LOG.printf("[WEBSOCKET] Reconnecting... wifiConnected == false\n");
        return;
    }
    //if (millis() - websocketLastPingTime > 30000 && !websocketReconnect) {
    if (millis() - websocketLastPingTime > settings.getInt(WS_ALERT_TIME) && !websocketReconnect) {
        LOG.printf("[WEBSOCKET] websocketReconnect = true; Reason: no ping/pong from server (WS_ALERT_TIME)\n");
        websocketReconnect = true;
        clearAllAlertsMaps();
        clearAllWeatherMaps();
        isFirstDataFetchCompleted = false;
        animation.clearAllAnimations();
        //int positions[] = {}; // not used in RUNNING_LIGHT
        animation.createAnimation(
            AnimationTypes::RUNNING_LIGHT,
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
        LOG.printf("[WEBSOCKET] websocketReconnect = true; Reason: WS_REBOOT_TIME exceeded, will reboot\n");
        rebootDevice(3000, true);
    }
    if (!websocket.available()) {
        LOG.printf("[WEBSOCKET] Reconnecting... websocket.available() == false\n");
        websocketConnected = false;
        socketConnect();
    }
    if (websocketReconnect) {
        LOG.printf("[WEBSOCKET] Reconnecting... websocketReconnect == true\n");
        websocketConnected = false;
        socketConnect();
    }
}

void memoryProcess() {
    static uint32_t loopCount = 1;
    SystemInfo info = getSystemInfo();
    String wifiStatus = wifiConnected ? "+" : "-";
    String websocketStatus = info.websocketStatus ? "+" : "-";

    LOG.printf(
        "[DEBUG] Loop %u: used heap %u B, free heap %u B, uptime %u min. WiFi: %s%u min. WebSocket: %s%u min\n",
        loopCount,
        info.usedMemory,
        info.freeMemory,
        info.uptime / 60,
        wifiStatus.c_str(),
        info.wifiUptime / 60,
        websocketStatus.c_str(),
        info.websocketUptime / 60
    );
    
    api.setSystemInfo(
        info.usedMemory,
        info.uptime,
        info.wifiUptime,
        info.wifiSignal,
        info.websocketStatus,
        info.websocketUptime,
        info.cpuTemp
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
            LOG.printf("[WIFI] Reconnect\n");
            wifiConnected = false;
            initWifi();
            reconnectAttempts++;
        } else {
            LOG.printf("[WIFI] Max reconnect attempts reached, rebooting...\n");
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
        LOG.printf("[LED] strip_main not initialized\n");
        return;
    }
    auto freeLeds = animation.getFreeLeds(strip_main, num_leds_main);
    LOG.printf("[LED] Free LEDs on strip_main: %d\n", (int)freeLeds.size());
    LOG.printf("[LED] List: ");
    for (const auto& led : freeLeds) {
        LOG.printf("%d ", led.ledIdx);
    }
    LOG.printf("\n");
}

// --- Climate Process ---
void climateProcess() {
  if (!climate.isAnySensorAvailable()) return;
  updateClimateData();
}

// --- Light Sensor Process ---
void lightSensorProcess() {
  if (!lightSensor.isAnySensorAvailable()) return;
  // read current light level
  lightSensor.read();
  updateLightLevelData();
}

// --- Settings Change Handlers ---

void handleAdaptAnimationColors() {
    LOG.printf("[SETTINGS] Adjusting animation colors\n");
    animation.adaptAllAnimationColors();
}

void handleAdaptAnimationBrightness() {
    LOG.printf("[SETTINGS] Adjusting animation brightness\n");
    animation.adaptAllAnimationBrightness();
}

void handleAdaptAnimationPeriod() {
    LOG.printf("[SETTINGS] Adjusting animation period\n");
    animation.adaptAllAnimationPeriod();
}

void handleAdaptAnimationType() {
    LOG.printf("[SETTINGS] Adjusting animation type\n");
    animation.adaptAllAnimationType();
}



void handleRecalculateLeds() {
    LOG.printf("[SETTINGS] Recalculating LEDs\n");
    generateCurrentRegionMap(hardwareConfig);
}

void handleReconnectStrips(bool main = false, bool bg = false, bool service = false) {
    static bool needReconnectMain = false;
    static bool needReconnectBg = false;
    static bool needReconnectService = false;
    
    if (main) needReconnectMain = true;
    if (bg) needReconnectBg = true;
    if (service) needReconnectService = true;
    
    // Виконуємо переподключення, якщо є якісь флаги
    if (needReconnectMain || needReconnectBg || needReconnectService) {
        LOG.printf("[SETTINGS] Reconnecting LED strips (main:%d bg:%d svc:%d)\n", 
                   needReconnectMain, needReconnectBg, needReconnectService);
        
        if (needReconnectMain) {
            reconnectStripMain();
            // Перезбираємо список індексів для Main після reconnect
            allLedsMain.clear();
            for (uint32_t i = 0; i < num_leds_main; ++i) {
                allLedsMain.push_back(i);
            }
            needReconnectMain = false;
        }
        if (needReconnectBg) {
            reconnectStripBg();
            // Перезбираємо список індексів для BG після reconnect
            allLedsBg.clear();
            int bgCount = hardwareConfig.getBgLedsCount();
            if (bgCount > 0) {
                for (uint32_t i = 0; i < (uint32_t)bgCount; ++i) {
                    allLedsBg.push_back(i);
                }
            }
            needReconnectBg = false;
        }
        if (needReconnectService) {
            reconnectStripService();
            needReconnectService = false;
        }
        
        adaptStripColorsAndBrightness();
        
        // Оновлюємо посилання в веб-інтерфейсі
        web.setStrips(strip_main, strip_bg, strip_service);
        
        LOG.printf("[LED] Strip reconnection completed.\n");
        logMemoryUsage("after strip reconnection complete");
    }
}

void handleUpdateBatteryPin() {
    LOG.printf("[SETTINGS] Updating battery pin state\n");
    if (settings.getInt(BATTERY_PIN) > 0) {
        battery.setPin(settings.getInt(BATTERY_PIN));
    } else {
        LOG.printf("[SETTINGS] Battery pin not configured, skipping update\n");
    }
}

void handleReconfigureDisplay() {
    LOG.printf("[SETTINGS] Reconfiguring display\n");
    initDisplay();
}

void handleReconfigureSound() {
    LOG.printf("[SETTINGS] Reconfiguring sound\n");
    initSound();
}

void handleReconfigureSensors() {
    LOG.printf("[SETTINGS] Reconfiguring sensors\n");
    initSensors();
}

void handleReconfigureButtons() {
    LOG.printf("[SETTINGS] Reconfiguring buttons\n");
    initButtons();
}

void handleUpdateAnimationsMode() {
    LOG.printf("[SETTINGS] Animations sync mode %s\n", settings.getBool(ENABLE_SYNC_ANIMATIONS) ? "ENABLED" : "DISABLED");
    animation.setSynchronizedMode(settings.getBool(ENABLE_SYNC_ANIMATIONS));
}

void requestRebootDevice() {
    LOG.printf("[SETTINGS] Reboot requested\n");
    rebootDevice(1000);
}

void reconfigureAll() {
    
    // LED та кольори
    handleRecalculateLeds();
    handleReconnectStrips(true, true, true);
    adaptStripColorsAndBrightness();
    handleRegenerateBgColorMap();
    
    // Апаратні компоненти
    handleReconfigureDisplay();
    handleReconfigureSound();
    handleReconfigureSensors();
    handleReconfigureButtons();
    handleUpdateBatteryPin();
    
    // Мережеві налаштування
    handleReconnectWebsocket();
    handleUpdateNtpHost();
    
    // Інші налаштування
    handleUpdateTimezone();
    handleAdaptClimate();
    handleAdaptVolume();
    handleUpdateAnimationsMode();
    handleUpdateHomeAlertBit();
    
    // Оновлення каналу прошивки
    fwUpdate.applyActiveChannel(settings.getInt(FW_UPDATE_CHANNEL) == 1);
    
    // Оновлення режиму логування
    loggingStream.setLogsEnabled(settings.getBool(LOGS_ENABLED));
    
    // Оновлення API
    api.reconfigure();
    
    LOG.println("[SETTINGS] Full system reconfiguration completed");
}

void requestReconfigureAll() {
    LOG.println("[SETTINGS] Requesting full system reconfiguration");
    async.setTimeout(reconfigureAll, 500); // Додаємо невелику затримку перед початком повної переконфігурації
}

void handleAdaptClimate() {
    LOG.printf("[SETTINGS] Adapting climate settings\n");
    climateProcess();
}

void handleAdaptVolume() {
    LOG.printf("[SETTINGS] Adapting sound volume\n");
    sound.setVolumeNight(settings.getInt(MELODY_VOLUME_NIGHT));
    sound.setVolumeDay(settings.getInt(MELODY_VOLUME_DAY));
}

void handleUpdateHomeAlertBit() {
    LOG.printf("[SETTINGS] Updating home alert bit\n");
    int localAlertBit = findHighestBitForRegionFlat(settings.getInt(HOME_DISTRICT));
    if (localAlertBit != alertBit) {
        alertAction(localAlertBit, settings.getInt(HOME_DISTRICT));
        updateSirenIfNeeded(localAlertBit);
    }
    alertBit = localAlertBit;
    int homeIdx = getRegionFlatIdx(settings.getInt(HOME_DISTRICT));
    homeAlertFlags = (homeIdx >= 0) ? alertsFlat[homeIdx] : 0;
    LOG.printf("[SETTINGS] homeAlertFlags: %u\n", homeAlertFlags);
    api.setHomeAlert(homeAlertFlags);
    uint8_t encodedTemp = temperatureMap[settings.getInt(HOME_DISTRICT)];
    int homeTemp = decodeTemperature(encodedTemp);
    api.setHomeDistrictTemp(homeTemp);
}

void handleUpdateTimezone() {
    LOG.printf("[SETTINGS] Updating timezone\n");
    applyTimezoneSettings(settings.getInt(TIME_ZONE));
}

void handleUpdateNtpHost() {
    LOG.printf("[SETTINGS] Updating NTP host to: %s\n", settings.getString(NTP_HOST));
    timeClient.setHost(settings.getString(NTP_HOST));
    syncTime(3);
}

void handleReconnectWebsocket() {
    LOG.printf("[SETTINGS] WebSocket server settings changed, reconnecting...\n");
    websocketConnected = false;
    websocketReconnect = true;
    clearAllAlertsMaps();
    clearAllWeatherMaps();
    isFirstDataFetchCompleted = false;
    alertsHash = 0;
    if (websocket.available()) {
        websocket.close();
    }
}

void handleRegenerateBgColorMap() {
    LOG.printf("[SETTINGS] Regenerating BG LED color map\n");
    generateBgLedColorsMap();
}

// --- Test Melody/Track Functions ---

void requestPlayTestMelody(int melodyId) {
    LOG.printf("[TEST] Playing test melody %d\n", melodyId);
    if (melodyId >= 0 && melodyId < (int)(sizeof(MELODIES) / sizeof(MELODIES[0]))) {
        playMelody(MELODIES[melodyId]);
    }
}

void requestPlayTestTrack(int trackId) {
    LOG.printf("[TEST] Playing test track %d\n", trackId);
    // TODO: Реалізувати відтворення треків, якщо потрібно
}

void requestWebsocketReconnect() {
    LOG.printf("[WEBSOCKET] Reconnection requested\n");
    handleReconnectWebsocket();
}

void updateFirmware() {
    LOG.printf("[FIRMWARE] Update started\n");
    display.showServiceMessage("Виконано", "Запит оновлення:", 3000);
    animation.clearAllAnimations();
    fwUpdate.download();
    fwUpdate.clearUpdateRequest();
}

void requestFirmwareUpdate(const char* firmwareId) {
    const char* versionToUse = firmwareId ? firmwareId : fwUpdate.getUpdateId();
    
    if (versionToUse == nullptr || strlen(versionToUse) == 0) {
        LOG.printf("[FIRMWARE] No firmware version specified\n");
        return;
    }
    
    // Встановлюємо ID прошивки для завантаження
    bool isBeta = settings.getInt(FW_UPDATE_CHANNEL) == 1;
    if (!fwUpdate.requestUpdate(versionToUse, isBeta)) {
        LOG.printf("[FIRMWARE] Invalid firmware ID: %s\n", versionToUse);
        return;
    }
    
    LOG.printf("[FIRMWARE] Update requested for version: %s\n", versionToUse);
    async.setTimeout(updateFirmware, 500); // Додаємо невелику затримку перед початком оновлення
}

void requestRecalculateLeds() {
    LOG.printf("[REQUEST] Recalculating LEDs\n");
    handleRecalculateLeds();
}

void requestAdaptColors() {
    LOG.printf("[REQUEST] Adapting colors\n");
    adaptStripColorsAndBrightness();
}

void requestToRegenerateBgColorMap() {
    LOG.printf("[REQUEST] Regenerating BG LED color map\n");
    handleRegenerateBgColorMap();
}

void brightnessProcess() {
    // if auto brightness set to day/night mode, check current hour and choose brightness
    uint8_t currentBrightness = getCurrentBrightnes();
    if (settings.getInt(CURRENT_BRIGHTNESS) != currentBrightness) {
        settings.saveInt(CURRENT_BRIGHTNESS, currentBrightness);
        LOG.printf("[BRIGHTNESS] Setting current_brightness to %d\n", currentBrightness);
        adaptStripColorsAndBrightness();     
        handleAdaptAnimationColors();
        handleAdaptAnimationBrightness();
    }
}

// --- Battery Monitor Process ---
void batteryProcess() {
    battery.logVoltage();
}

// --- Display Show Methods ---
void showWeather() {
    int homeDistrict = settings.getInt(HOME_DISTRICT);
    auto it = temperatureMap.find(homeDistrict);
    const char* regionName = getNameById(DISTRICTS, homeDistrict, MAX_REGIONS);
    char weatherInfo[50];
    
    if (it != temperatureMap.end()) {
        int homeTemp = decodeTemperature(it->second);
        snprintf(weatherInfo, sizeof(weatherInfo), "%d℃", homeTemp);
    } else {
        snprintf(weatherInfo, sizeof(weatherInfo), "--℃");
    }
    
    display.printMessage(weatherInfo, regionName);
}

void showTechnicalInfo() {
    SystemInfo info = getSystemInfo();
    const char* version = fwUpdate.getCurrentVersion();
    uint32_t totalMemory = info.usedMemory + info.freeMemory;
    
    String versionStr = String("v") + version;
    String ipAddrStr = wifiConnected ? WiFi.localIP().toString() : "Н/А";
    
    char memBuf[30];
    snprintf(memBuf, sizeof(memBuf), "%u/%u KB", info.usedMemory / 1024, totalMemory / 1024);
    String memoryStr = String(memBuf);
    
    display.printMultilineMessage(versionStr, ipAddrStr, memoryStr, "", "Тех. інформація:");
}

void showMicroclimate() {
    // Check sensor availability first
    bool hasTemp = climate.isTemperatureAvailable();
    bool hasHum = climate.isHumidityAvailable();
    bool hasPressure = climate.isPressureAvailable();
    
    // Only read values if sensors are available
    float temp = hasTemp ? climate.getTemperature() : NAN;
    float hum = hasHum ? climate.getHumidity() : NAN;
    float pressure = hasPressure ? climate.getPressure() : NAN;
    
    String line1 = "";  // Temperature
    String line2 = "";  // Humidity + Pressure
    
    // First line - Temperature
    if (hasTemp) {
        char tempBuf[30];
        snprintf(tempBuf, sizeof(tempBuf), "%.1f℃", temp);
        line1 = String(tempBuf);
    }
    
    // Second line - Humidity and Pressure
    if (hasHum && hasPressure) {
        char climBuf[50];
        snprintf(climBuf, sizeof(climBuf), "%.1f%% %.0fмм", hum, pressure);
        line2 = String(climBuf);
    } else if (hasHum) {
        char humBuf[30];
        snprintf(humBuf, sizeof(humBuf), "%.1f%%", hum);
        line2 = String(humBuf);
    } else if (hasPressure) {
        char pressBuf[30];
        snprintf(pressBuf, sizeof(pressBuf), "%.0fмм", pressure);
        line2 = String(pressBuf);
    }
    
    // If no data available
    if (line1.isEmpty() && line2.isEmpty()) {
        line1 = "Немає даних";
    }
    
    display.printMultilineMessage(line1, line2, "", "", "Мікроклімат");
}

void showCombined() {
    // Check what modes are enabled
    bool displayWeather = settings.getBool(TOGGLE_MODE_WEATHER);
    bool displayMicroclimate = climate.isAnySensorAvailable() && settings.getBool(TOGGLE_MODE_MICROCLIMATE);
    
    // Calculate number of periods: Clock (always) + Weather (optional) + Microclimate (optional)
    int numPeriods = 1; // Always show clock
    if (displayWeather) numPeriods++;
    if (displayMicroclimate) numPeriods++;
    
    int periodIndex = getCurrentPeriodIndex(settings.getInt(DISPLAY_MODE_TIME), numPeriods, timeClient.second());
    
    // Build dynamic display sequence
    int currentPeriod = 0;
    
    // Period 0: Always Clock
    if (periodIndex == currentPeriod) {
        showClock();
        return;
    }
    currentPeriod++;
    
    // Next period: Weather (if enabled)
    if (displayWeather) {
        if (periodIndex == currentPeriod) {
            showWeather();
            return;
        }
        currentPeriod++;
    }
    
    // Next period: Microclimate (if enabled and available)
    if (displayMicroclimate) {
        if (periodIndex == currentPeriod) {
            showMicroclimate();
            return;
        }
        currentPeriod++;
    }
    
    // Fallback to clock
    showClock();
}

// --- Display Process ---
void displayProcess()
{
    // Remove UA Anthem playing flag if anthem stopped
    if (uaAnthemPlaying && (!sound.isBuzzerPlaying() && !sound.isDFPlayerPlaying())) {
        uaAnthemPlaying = false;
        // adapt colors on min of silence end
        adaptStripColorsAndBrightness();
        handleAdaptAnimationColors();
    }

    // Turn off display at night if setting enabled (Priority - highest)
    if (settings.getBool(DISPLAY_OFF_AT_NIGHT) && isItNightNow() && !display.isServiceMessageActive()) {
        display.clear();
        return;
    }

    // Show Minute of silence mode if activated. (Priority - 0)
    if (minuteOfSilence) {
        displayMinuteOfSilence();
        return;
    }
    // Show Glory To Ukraine if anthem playing. (Priority - 1)
    if (uaAnthemPlaying) {
        showMinOfSilenceScreen(1);
        return;
    }
    // Show firmware update notification if available (Priority - 2)
    if (fwUpdate.isUpdateAvailable() && settings.getBool(NEW_FW_NOTIFICATION)) {
        showNewFirmwareNotification();
        return;
    }
    
    // Check if display is manually turned off
    if (isDisplayOff && !display.isServiceMessageActive()) {
        display.clear();
        return;
    }
    
    // Handle display modes
    int displayMode = settings.getInt(DISPLAY_MODE);
    
    switch(displayMode) {
        case 0: // Вимкнено (Disabled)
        if (!display.isServiceMessageActive()) {
            display.clear();
        }
            break;
        
        case 1: // Годинник (Clock)
            showClock();
            break;
        
        case 2: // Погода (Weather)
            showWeather();
            break;
        
        case 3: // Технічна інформація (Technical Information)
            showTechnicalInfo();
            break;
        
        case 4: // Мікроклімат (Microclimate)
            showMicroclimate();
            break;
        
        case 9: // Комбінований (Combined)
            showCombined();
            break;
        
        default:
            showClock();
            break;
    }
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
    checkMinuteOfSilence();
    if (needToPlaySound(REGULAR) && !minuteOfSilence && sound.beepHour != timeClient.hour() && timeClient.minute() == 0 && timeClient.second() == 0) {
        sound.setBeepHour(timeClient.hour());
        playMelody(REGULAR);
    }
}

void rebuildSensorsListItems() {
    SettingListItem* lightSensorOption = getSettingItemById(AUTO_BRIGHTNESS_MODES, AUTO_BRIGHTNESS_OPTIONS_COUNT, 2); // Сенсор освітлення
    if (lightSensorOption != nullptr) {
        bool sensorAvailable = lightSensor.isAnySensorAvailable();
        lightSensorOption->ignore = !sensorAvailable;
        lightSensorOption->showDisabled = !sensorAvailable;
    }
    SettingListItem* climateSensorOption = getSettingItemById(DISPLAY_MODES, DISPLAY_MODES_COUNT, 4); // Мікроклімат
    if (climateSensorOption != nullptr) {
        bool sensorAvailable = climate.isAnySensorAvailable();
        climateSensorOption->ignore = !sensorAvailable;
        climateSensorOption->showDisabled = !sensorAvailable;
    }
}

// --- SETUP ---
void setup() {
    memset(ledBitCache, -1, sizeof(ledBitCache));
    memset(s_ledDirty,   0, sizeof(s_ledDirty));

    LOG.begin(115200);
    logsManager.begin();  // Initialize logs capture system

    checkFreeHeap("LOG initialization");

    initChipID();
    checkFreeHeap("chipID initialization");

    initSettings();
    checkFreeHeap("settings initialization");

    fwUpdate.setDisplay(&display);
    fwUpdate.setApi(&api);
    fwUpdate.setMapUpdateCallback([](float p) { mapUpdate(p); });
    fwUpdate.setRebootCallback([]() { rebootDevice(); });
    fwUpdate.setServicePinCallback([]() { servicePin(UPD_AVAILABLE); });
    fwUpdate.initCallbacks();
    checkFreeHeap("updates initialization");

    // Передаємо settings в AnimationManager
    animation.setSettings(&settings);

    // Ініціалізуємо storage перед strip, щоб мати можливість завантажити кастомну мапу для стрічок з пам'яті
    initStorage();
    checkFreeHeap("SPIFFS initialization");
    
    initStrip();
    checkFreeHeap("LED strips initialization");
    brightnessProcess();

    initMapping();
    checkFreeHeap("LED mapping initialization");

    // Адаптація led для відображення прапору України
    adaptStripColorsAndBrightness();
    checkFreeHeap("colors adaptation");

    initSiren();
    checkFreeHeap("siren initialization");

    initButtons();
    checkFreeHeap("buttons initialization");

    initDisplay();
    display.drawIconWithText(JaamDisplayIcon::TRIDENT, "JAAM " + String(VERSION) + "");
    checkFreeHeap("display initialization");

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
    
    // Встановлюємо режим роботи анімацій
    animation.setSynchronizedMode(settings.getBool(ENABLE_SYNC_ANIMATIONS));
    checkFreeHeap("animation settings");
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
    async.setInterval(batteryProcess, BATTERY_CHECK_INTERVAL);
    async.setInterval(displayProcess, DISPLAY_CHECK_INTERVAL);
    async.setInterval(climateProcess, CLIMATE_CHECK_INTERVAL);
    async.setInterval(lightSensorProcess, LIGHT_SENSOR_CHECK_INTERVAL);
    async.setInterval(volumeProcess, VOLUME_CHECK_INTERVAL);
    async.setInterval(beepHourProcess, BEEP_HOUR_CHECK_INTERVAL);

    checkFreeHeap("async tasks configuration");
    
    LOG.printf("[SETUP] Initialization complete\n");
    checkFreeHeap("full setup");
    loggingStream.setLogsEnabled(settings.getBool(LOGS_ENABLED));
}

void loop() {
    wm.process();
#if !defined(TEST_ANIMATION)
    websocket.poll();
#endif
    animation.update();
    async.run();
    web.handleClient();
    api.handleWebSocketClients();
    buttons.tick();
}
