#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>


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
#include "JaamSound.h"
#include "JaamButton.h"
#include "JaamHardware.h"
#include "JaamFirmwareUpdate.h"

using namespace websockets;

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
JaamSound           sound;
JaamButton          buttons;
JaamHardware        hardwareConfig;

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
volatile bool needReconfigureButtons = false;
volatile bool needUpdateAnimationsMode = false;
volatile bool needToRegenerateBgColorMap = false;
volatile bool needAdaptVolume = false;
volatile bool needUpdateHomeAlertBit = false;
volatile bool needUpdateTimezone = false;
volatile bool needPlayTestMelody = false;
volatile bool needPlayTestTrack = false;


// --- WIFI Configuration ---
WiFiManager         wm;

time_t              lastWifiConnectTime = 0;  // Track when WiFi was last connected
uint16_t            alertsHash = 0;
bool                wifiConnected = false;


// --- WEBSOCKET Configuration ---
WebsocketsClient    websocket;
time_t              websocketLastPingTime = 0;
bool                isFirstDataFetchCompleted = false;
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
int                 testMelodyId = -1;
int                 testTrackId = -1;

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

int getCurrentMapMode() {
  if (minuteOfSilence || uaAnthemPlaying || !isFirstDataFetchCompleted) return MapModes::FLAG; // ua flag

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
  // do not play any sound before websocket connection
  if (!isFirstDataFetchCompleted) return false;

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

//--Update
// void saveLatestFirmware() {
//   int fwUpdateChannel = settings.getInt(FW_UPDATE_CHANNEL);
//   const int *count = fwUpdateChannel ? &testBinsCount : &binsCount;
//   Firmware firmware = currentFirmware;
//   for (int i = 0; i < *count; i++) {
//     const char* filename = fwUpdateChannel ? test_bin_list[i] : bin_list[i];
//     if (prefix("latest", filename)) continue;
//     Firmware parsedFirmware = parseFirmwareVersion(filename);
//     if (firstIsNewer(parsedFirmware, firmware)) {
//       firmware = parsedFirmware;
//     }
//   }
//   latestFirmware = firmware;
//   fwUpdate.isUpdateAvailable() = firstIsNewer(latestFirmware, currentFirmware);
//   servicePin(UPD_AVAILABLE, fwUpdate.isUpdateAvailable() ? HIGH : LOW, false);
//   fillFwVersion(newFwVersion, latestFirmware);
//   LOG.printf("Latest firmware version: %s\n", newFwVersion);
//   LOG.println(fwUpdate.isUpdateAvailable() ? "New fw available!" : "No new firmware available");
// }

void mapUpdate(float percents) {
    if (strip_main == nullptr || num_leds_main == 0) return;
    if (percents < 0.0f) percents = 0.0f;
    if (percents > 1.0f) percents = 1.0f;

    uint32_t litCount = static_cast<uint32_t>(num_leds_main * percents + 0.5f);
    const uint32_t green = strip_main->Color(0, 255, 0);
    const uint32_t off = strip_main->Color(0, 0, 0);

    animation.safeStripOperation(strip_main, [&](Adafruit_NeoPixel* strip) {
        for (uint32_t i = 0; i < num_leds_main; ++i) {
            strip->setPixelColor(i, i < litCount ? green : off);
        }
        strip->show();
    });

    if (strip_bg != nullptr) {
        uint32_t bgCount = strip_bg->numPixels();
        uint32_t bgLitCount = static_cast<uint32_t>(bgCount * percents + 0.5f);
        const uint32_t bgGreen = strip_bg->Color(0, 255, 0);
        const uint32_t bgOff = strip_bg->Color(0, 0, 0);
        animation.safeStripOperation(strip_bg, [&](Adafruit_NeoPixel* strip) {
            for (uint32_t i = 0; i < bgCount; ++i) {
                strip->setPixelColor(i, i < bgLitCount ? bgGreen : bgOff);
            }
            strip->show();
        });
    }

    if (strip_service != nullptr) {
        static uint32_t serviceIndex = 0;
        uint32_t serviceCount = strip_service->numPixels();
        if (serviceCount > 0) {
            const uint32_t serviceGreen = strip_service->Color(0, 255, 0);
            const uint32_t serviceOff = strip_service->Color(0, 0, 0);
            animation.safeStripOperation(strip_service, [&](Adafruit_NeoPixel* strip) {
                for (uint32_t i = 0; i < serviceCount; ++i) {
                    strip->setPixelColor(i, i == serviceIndex ? serviceGreen : serviceOff);
                }
                strip->show();
            });
            serviceIndex = (serviceIndex + 1) % serviceCount;
        }
    }
    

}



// --- Buttons Functions ---

void handleClick(int event, JaamButton::Action action) {
  SoundType soundType = action == JaamButton::Action::SINGLE_CLICK ? SoundType::SINGLE_CLICK : SoundType::LONG_CLICK;
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
      isMapOff = !isMapOff;
      display.showServiceMessage(!isMapOff ? "Увімкнено" : "Вимкнено", "Мапу:");
      needAdaptColors = true;
      break;
    // toggle display
    case 4:
      isDisplayOff = !isDisplayOff;
      display.showServiceMessage(!isDisplayOff ? "Увімкнено" : "Вимкнено", "Дисплей:");
      break;
    // toggle display and map
    case 5:
      if (isDisplayOff != isMapOff) {
        isDisplayOff = false;
        isMapOff = false;
      } else {
        isMapOff = !isMapOff;
        isDisplayOff = !isDisplayOff;
      }
      display.showServiceMessage(!isMapOff ? "Увімкнено" : "Вимкнено", "Дисплей та мапу:");
      //mapCycle();
      break;
    // toggle night mode
    case 6:
      //saveNightMode(!nightMode);
      break;
    // toggle lamp (singl click) or reboot device (long click)
    case 10:
      rebootDevice();
      break;
    case 100:
      fwUpdate.requestUpdate(fwUpdate.getNewVersion());
      break;
    default:
      // do nothing
      break;
  }
}

bool isButtonActivated() {
  return settings.getInt(BUTTON_1_MODE) != 0 || settings.getInt(BUTTON_1_MODE_LONG) != 0 || settings.getInt(BUTTON_2_MODE) != 0 || settings.getInt(BUTTON_2_MODE_LONG) != 0 || settings.getInt(BUTTON_3_MODE) != 0 || settings.getInt(BUTTON_3_MODE_LONG) != 0;
}

void singleClick(int mode) {
  handleClick(mode, JaamButton::SINGLE_CLICK);
}

void longClick(int modeLong) {
  if (fwUpdate.isUpdateAvailable()) {
    handleClick(100, JaamButton::LONG_CLICK);
    return;
  }
  handleClick(modeLong, JaamButton::LONG_CLICK);
}

void buttonClick(const char* buttonName, int mode) {
#if TEST_MODE
  displayMessage("Single click!", buttonName);
#else
  singleClick(mode);
#endif
}

void buttonLongClick(const char* buttonName, int modeLong) {
#if TEST_MODE
  displayMessage("Long click!", buttonName);
#else
  longClick(modeLong);
#endif
}

void buttonDuringLongClick(const char* buttonName, int modeLong, JaamButton::Action action) {
#if FW_UPDATE_ENABLED
  if (settings.getInt(NEW_FW_NOTIFICATION) == 1 && fwUpdate.isUpdateAvailable() && isButtonActivated() && !isDisplayOff) {
    return;
  }
#endif
  if (action == JaamButton::Action::DURING_LONG_CLICK) {
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
  } else if (action == JaamButton::Action::LONG_CLICK_END) {
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
  buttonClick("Button 1", settings.getInt(BUTTON_1_MODE));
}

void button2Click() {
  LOG.printf("Button 2 click\n");
  buttonClick("Button 2", settings.getInt(BUTTON_2_MODE));
}

void button3Click() {
  LOG.printf("Button 3 click\n");
  buttonClick("Button 3", settings.getInt(BUTTON_3_MODE));
}

void button1LongClick() {
  LOG.printf("Button 1 long click\n");
  buttonLongClick("Button 1", settings.getInt(BUTTON_1_MODE_LONG));
}

void button2LongClick() {
  LOG.printf("Button 2 long click\n");
  buttonLongClick("Button 2", settings.getInt(BUTTON_2_MODE_LONG));
}

void button3LongClick() {
  LOG.printf("Button 3 long click\n");
  buttonLongClick("Button 3", settings.getInt(BUTTON_3_MODE_LONG));
}

void button1DuringLongClick(JaamButton::Action action) {
  LOG.printf("Button 1 during long click\n");
  buttonDuringLongClick("Button 1", settings.getInt(BUTTON_1_MODE_LONG), action);
}

void button2DuringLongClick(JaamButton::Action action) {
  LOG.printf("Button 2 during long click\n");
  buttonDuringLongClick("Button 2", settings.getInt(BUTTON_2_MODE_LONG), action);
}

void button3DuringLongClick(JaamButton::Action action) {
  LOG.printf("Button 3 during long click\n");
  buttonDuringLongClick("Button 3", settings.getInt(BUTTON_3_MODE_LONG), action);
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
    if (settings.getBool(SOUND_ON_ALERT)) {
        switch (bit){
            case AlertModes::ALERT:
                if(needToPlaySound(SoundType::ALERT_ON)) playMelody(ALERT_ON);
                display.showServiceMessage("ТРИВОГА", districtName, settings.getInt(DISPLAY_ALERT_MESSAGE_TIME) * 1000);
                break;
            case AlertModes::DRONES:
                if(needToPlaySound(SoundType::DRONES)) playMelody(DRONES);
                display.showServiceMessage("БПЛА", districtName, settings.getInt(DISPLAY_ALERT_MESSAGE_TIME) * 1000);
                break;
            case AlertModes::MISSILES:
                if(needToPlaySound(SoundType::MISSILES)) playMelody(MISSILES);
                display.showServiceMessage("РАКЕТИ", districtName, settings.getInt(DISPLAY_ALERT_MESSAGE_TIME) * 1000);
                break;
            case AlertModes::KABS:
                if(needToPlaySound(SoundType::KABS)) playMelody(KABS);
                display.showServiceMessage("КАБ", districtName, settings.getInt(DISPLAY_ALERT_MESSAGE_TIME) * 1000);
                break;
            case AlertModes::BALLISTIC:
                if(needToPlaySound(SoundType::BALLISTIC)) playMelody(BALLISTIC);
                display.showServiceMessage("БАЛЛІСТИКА", districtName, settings.getInt(DISPLAY_ALERT_MESSAGE_TIME) * 1000);
                break;
            case AlertModes::EXPLOSION:
                if(needToPlaySound(SoundType::EXPLOSIONS)) playMelody(EXPLOSIONS);
                display.showServiceMessage("ВИБУХИ", districtName, settings.getInt(DISPLAY_ALERT_MESSAGE_TIME) * 1000);
                break;
            case AlertModes::RECON_DRONES:
                if(needToPlaySound(SoundType::RECON_DRONES)) playMelody(RECON_DRONES);
                display.showServiceMessage("РОЗВІДКА БПЛА", districtName, settings.getInt(DISPLAY_ALERT_MESSAGE_TIME) * 1000);
                break;
            default:
                break;
        }
    }
    if (settings.getBool(SOUND_ON_ALERT_END) && bit == AlertModes::NO_ALERT) {
        if(needToPlaySound(SoundType::ALERT_OFF)) playMelody(ALERT_OFF);
        display.showServiceMessage("ВІДБІЙ", districtName, settings.getInt(DISPLAY_ALERT_MESSAGE_TIME) * 1000);
    }
}

void animateLed(Adafruit_NeoPixel* strip, int map_mode, int led_position, int bit, uint16_t region_id, bool increase = true) {
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
    uint8_t endBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_ANIMATION_END));
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
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_CLEAR));             
            animType = settings.getInt(ANIMATION_ALERT_OFF_TYPE);
            cycles = (settings.getInt(ALERT_OFF_TIME) * 1000)/settings.getInt(ANIMATION_ALERT_OFF_CYCLE_TIME);
            period = settings.getInt(ANIMATION_ALERT_OFF_CYCLE_TIME);
            break;
        case 0:
            color = animation.colorFromHex(settings.getString(COLOR_ALERT)); 
            animType = (increase) ? settings.getInt(ANIMATION_ALERT_ON_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_ALERT));
            period = (increase) ? settings.getInt(ANIMATION_ALERT_ON_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(ALERT_ON_TIME) * 1000)/settings.getInt(ANIMATION_ALERT_ON_CYCLE_TIME) : 1;
            break;
        case 5: 
            color = animation.colorFromHex(settings.getString(COLOR_DRONES));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_DRONES));
            animType = (increase) ? settings.getInt(ANIMATION_DRONE_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            period = (increase) ? settings.getInt(ANIMATION_DRONE_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(DRONE_TIME) * 1000)/settings.getInt(ANIMATION_DRONE_CYCLE_TIME) : 1; 
            break;
        case 6:
            color = animation.colorFromHex(settings.getString(COLOR_MISSILES));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_MISSILES));
            animType = (increase) ? settings.getInt(ANIMATION_MISSILE_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            period = (increase) ? settings.getInt(ANIMATION_MISSILE_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(MISSILE_TIME) * 1000)/settings.getInt(ANIMATION_MISSILE_CYCLE_TIME) : 1; 
            break;
        case 7:
            color = animation.colorFromHex(settings.getString(COLOR_KABS));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_KABS));
            animType = (increase) ? settings.getInt(ANIMATION_KAB_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            period = (increase) ? settings.getInt(ANIMATION_KAB_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(KAB_TIME) * 1000)/settings.getInt(ANIMATION_KAB_CYCLE_TIME) : 1;
            break;
        case 8:
            color = animation.colorFromHex(settings.getString(COLOR_BALLISTIC));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_BALLISTIC));
            animType = (increase) ? settings.getInt(ANIMATION_BALLISTIC_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            period = (increase) ? settings.getInt(ANIMATION_BALLISTIC_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(BALLISTIC_TIME) * 1000)/settings.getInt(ANIMATION_BALLISTIC_CYCLE_TIME)  : 1;
            break;
        case 9:
            color = animation.colorFromHex(settings.getString(COLOR_EXPLOSION));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_EXPLOSION));
            animType = (increase) ? settings.getInt(ANIMATION_EXPLOSION_TYPE) : AnimationTypes::ONE_WAY_BLEND_FADE;
            period = (increase) ? settings.getInt(ANIMATION_EXPLOSION_CYCLE_TIME) : 3000;
            cycles = (increase) ? (settings.getInt(EXPLOSION_TIME) * 1000)/settings.getInt(ANIMATION_EXPLOSION_CYCLE_TIME)  : 1;
            break;
        case 10:
            color = animation.colorFromHex(settings.getString(COLOR_RECON_DRONES));
            startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_RECON_DRONES));
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
        bit
    )) {
        LOG.printf("[ERROR] Failed to create animation\n");
        return;
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
    if (type != TYPE_ALERTS_BATCH && type != TYPE_NOTIFICATIONS_BATCH && type != TYPE_WEATHER_BATCH && type != TYPE_FIRMWARE_UPDATE_BATCH) {
        LOG.printf("[ERROR] message type unknown\n");
        return;
    }

    // ітератор по пакету
    uint8_t ledCount;
    size_t bodyLen;

    if(type == TYPE_FIRMWARE_UPDATE_BATCH) {
        LOG.printf("[WEBSOCKET] TYPE_FIRMWARE_UPDATE_BATCH received\n");
        bodyLen = len - HEADER_SZ;

        // payloadLen має ділитися на RECORD_FW
        if (bodyLen == 0 || (bodyLen % RECORD_FW) != 0) {
            // некоректний фрейм — пропускаємо і реконнектимось
            LOG.printf("[ERROR] bodyLen == 0 || (bodyLen % RECORD_FW) != 0\n");
            needReconnectWebsocket = true;
            return;
        }

        fwUpdate.processBatch(data + HEADER_SZ, bodyLen);
        return;
    }

    if(type == TYPE_NOTIFICATIONS_BATCH) {
        LOG.printf("[WEBSOCKET] TYPE_NOTIFICATIONS_BATCH received\n");
        bodyLen = len - HEADER_SZ;

        // payloadLen має ділитися на RECORD_SZ
        if (bodyLen == 0 || (bodyLen % RECORD_SZ) != 0) {
            // некоректний фрейм — пропускаємо і реконнектимось
            LOG.printf("[ERROR] bodyLen == 0 || (bodyLen % RECORD_SZ) != 0\n");
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
                LOG.printf("[WEBSOCKET] notification region %d:\t\t%d skipped - no leds associated\n", region_id, flags16);
                continue;
            } else {
                LOG.printf("[WEBSOCKET] notification region %d:\t\t%d\n", region_id, flags16);
            }

            int highestBitRegion = -1;
            LOG.printf("[WEBSOCKET]   highest bit from regions: ");
            for (uint8_t i = 0; i < ledCount; ++i) {
                int led_position = leds[i];
                //LOG.printf("[WEBSOCKET] LED %d: ", led_position);
                // Шукаємо всі регіони для цього LED
                const std::vector<uint16_t>& regions = getRegionsForLed(led_position);
                for (uint16_t rid : regions) {
                    // Шукаємо найвищий біт для регіону в alertsMap
                    auto alerts_it = alertsMap.find(rid);
                    if (alerts_it != alertsMap.end()) {
                        int highestBitSearch = findHighestBit16(alerts_it->second);
                        LOG.printf("%d:%d ", rid, highestBitSearch);

                        if (hasHigherPriority(highestBitSearch, highestBitRegion)) {
                            highestBitRegion = highestBitSearch;
                        }
                    }
                }
            }  
            LOG.printf("\n");

            // Тепер highestBitRegion містить найвищий пріоритет серед усіх регіонів, пов'язаних з цим LED
            int actualBitDiff = findHighestBit16(flags16, false);

            LOG.printf("[WEBSOCKET]   highestBitRegion %d, actualBitDiff %d\n", highestBitRegion, actualBitDiff);

            if (hasHigherPriority(actualBitDiff,highestBitRegion)) {
                for (int i = 0; i < ledCount; ++i) {
                    animateLed(strip_main, MapModes::ALERT, leds[i], actualBitDiff, region_id, true);
                    if (isLedInHomeRegion(leds[i])) {
                        needToAnimateBgHomeRegion = true;
                        homeRegionBit = actualBitDiff;
                    }
                }
            }
            if (region_id == settings.getInt(HOME_DISTRICT)) {
                LOG.printf("[WEBSOCKET]   Animating home district LEDs: region %d, bit %d\n", settings.getInt(HOME_DISTRICT), homeRegionBit);
                animateLed(strip_bg, MapModes::ALERT, 0, homeRegionBit, settings.getInt(HOME_DISTRICT), true);
                alertAction(actualBitDiff, settings.getInt(HOME_DISTRICT));
            }
        }
    }

    if(type == TYPE_ALERTS_BATCH) {
        LOG.printf("[WEBSOCKET] TYPE_ALERTS_BATCH received\n");
        // Обчислюємо довжину payload після заголовка
        bodyLen = len - HEADER_SZ - HASH_SZ;

        // payloadLen має ділитися на RECORD_SZ
        if (bodyLen == 0 || (bodyLen % RECORD_SZ) != 0) {
            // некоректний фрейм — пропускаємо і реконнектимось
            LOG.printf("[ERROR] bodyLen == 0 || (bodyLen % RECORD_SZ) != 0\n");
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
                LOG.printf("[WEBSOCKET] alert region %d:\t\t%d skipped - no leds associated\n", region_id, flags16);
                continue;
            } else {
                LOG.printf("[WEBSOCKET] alert region %d:\t\t%d\n", region_id, flags16);
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
                    //LOG.printf("[DIFF]   LED for region %d: %d\n", diff.region_id, led_position);

                    led_bits_old[led_position] = {findHighestBitForLed(led_position), diff.region_id};
                    //led_bits_actual[led_position] = {highest_bit_current, diff.region_id};
                    // Оновлюємо найвищий біт для LED
                    // if (hasHigherPriority(findHighestBitForLed(led_position), highest_bit_current)) {
                    //     led_bits_actual[led_position] = {findHighestBitForLed(led_position), diff.region_id};
                    // }
                    //LOG.printf("[DIFF] LED %d highest bit actual: %d\n", led_position, led_bits_actual[led_position].highest_bit);
                    //LOG.printf("[DIFF]   LED %d highest bit alerts: %d\n", led_position, led_bits_old[led_position].highest_bit);
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
                //     LOG.printf("\n");
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

                // LOG.printf("[WEBSOCKET] LED %d: region %d actual bit %d\n",
                //     led.first, led.second.region_id, diff_bit);

                // LOG.printf("[WEBSOCKET] LED %d: region %d old bit %d\n",
                //     led.first, led.second.region_id, alerts_bit);

                if (diff_bit == alerts_bit) {
                    // LOG.printf("[WEBSOCKET] LED %d: region %d no changes in bit %d\n",
                    //     led.first, led.second.region_id, diff_bit);
                    continue; // Немає змін, пропускаємо
                }

                if (hasHigherPriority(diff_bit, alerts_bit)) {
                    LOG.printf("[WEBSOCKET] LED %d: region %d increasing bit from %d to %d\n", led.first, led.second.region_id, alerts_bit, diff_bit);
                    animateLed(strip_main, MapModes::ALERT, led.first, diff_bit, led.second.region_id, true);
                } 
                if (hasHigherPriority(alerts_bit, diff_bit)) {
                    LOG.printf("[WEBSOCKET] LED %d: region %d decreasing bit from %d to %d\n", led.first, led.second.region_id, alerts_bit, diff_bit);
                        animateLed(strip_main, MapModes::ALERT, led.first, diff_bit, led.second.region_id, false);
                }
            }
            int localAlertBit = findHighestBitForRegionDirect(settings.getInt(HOME_DISTRICT));
            if (localAlertBit != alertBit){
                if (hasHigherPriority(localAlertBit, alertBit)) {
                    LOG.printf("[WEBSOCKET] Animating home district LEDs: region %d, bit %d, increase\n",settings.getInt(HOME_DISTRICT), localAlertBit);
                    homeRegionIncrease = true;
                    alertAction(localAlertBit, settings.getInt(HOME_DISTRICT));
                }
                if (hasHigherPriority(alertBit, localAlertBit)) {
                    LOG.printf("[WEBSOCKET] Animating home district LEDs: region %d, bit %d, decrease\n",settings.getInt(HOME_DISTRICT), localAlertBit);
                    homeRegionIncrease = false;
                    if (localAlertBit == -1) {
                        alertAction(localAlertBit, settings.getInt(HOME_DISTRICT));
                    }
                }
                animateLed(strip_bg, MapModes::ALERT, 0, localAlertBit, settings.getInt(HOME_DISTRICT), homeRegionIncrease);
            }
            alertBit = localAlertBit;
            uint16_t homeAlertFlags = alertsMap[settings.getInt(HOME_DISTRICT)];
            api.updateHomeAlert(homeAlertFlags);
        }
        alertsHash = actualHash;
    }

    if(type == TYPE_WEATHER_BATCH) {
        LOG.printf("[WEBSOCKET] TYPE_WEATHER_BATCH received\n");
        bodyLen = len - HEADER_SZ;

        // payloadLen має ділитися на RECORD_LZ
        if (bodyLen == 0 || (bodyLen % RECORD_LZ) != 0) {
            // некоректний фрейм — пропускаємо і реконнектимось
            LOG.printf("[ERROR] bodyLen == 0 || (bodyLen % RECORD_LZ) != 0\n");
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
            temperatureMap[region_id] = flags8; // Зберігаємо температуру для регіону
            LOG.printf("[WEBSOCKET] weather region %u:\tflags8=%u\n", region_id, flags8);
            ptr += RECORD_LZ; // перехід до наступного запису (2B region_id + 1B flags8)
        }
        uint8_t encodedTemp = temperatureMap[settings.getInt(HOME_DISTRICT)];
        int homeTemp = decodeTemperature(encodedTemp);
        api.updateHomeDistrictTemp(homeTemp);
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
        alertBit = findHighestBitForRegionDirect(settings.getInt(HOME_DISTRICT));
        uint16_t homeAlertFlags = alertsMap[settings.getInt(HOME_DISTRICT)];
        api.setHomeAlert(homeAlertFlags);
        uint8_t encodedTemp = temperatureMap[settings.getInt(HOME_DISTRICT)];
        int homeTemp = decodeTemperature(encodedTemp);
        api.setHomeDistrictTemp(homeTemp);
    }

    isFirstDataFetchCompleted = true;
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
        clearAllAlertsMaps();
        clearAllWeatherMaps();
        //animation.clearAllAnimations();
        LOG.printf("[WEBSOCKET] connection time - %d ms\n", millis() - startTime);
        char chipIdInfo[25];
        sprintf(chipIdInfo, "chip_id:%s", chipID);
        LOG.printf("[WEBSOCKET] %s\n", chipIdInfo);
        websocket.send(chipIdInfo);
        char firmwareInfo[100];
        sprintf(firmwareInfo, "firmware:%s_%s", fwUpdate.getCurrentVersion(), settings.getString(ID));
        LOG.printf("[WEBSOCKET] %s\n", firmwareInfo);
        websocket.send(firmwareInfo);
        char userInfo[250];
        JsonDocument userInfoJson;
        userInfoJson["legacy"] = settings.getInt(HARDWARE);
        sprintf(userInfo, "user_info:%s", userInfoJson.as<String>().c_str());
        LOG.printf("[WEBSOCKET] %s\n", userInfo);
        websocket.send(userInfo);
        websocket.ping("A");
        if (websocketReconnect) {
            animation.clearAllAnimations();
            websocketReconnect = false;
        }
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

    num_leds_main = hardwareConfig.getMainLedsCount();
    int mainLedPin = hardwareConfig.getMainLedPin();
    
    if (mainLedPin > 0) {
        int mainLedColorFormat = hardwareConfig.getMainLedColorFormat();
        uint8_t ledType = mainLedColorFormat + settings.getInt(MAIN_LED_FREQUENCY);
        LOG.printf("[LED] Initializing strip_main on pin %d with %d LEDs, type %d (format:%d + freq:%d)\n", 
                   mainLedPin, num_leds_main, ledType, 
                   mainLedColorFormat, settings.getInt(MAIN_LED_FREQUENCY));
        status = led.createStrip(strip_main, mainLedPin, num_leds_main, 10, DefaultColors::MAIN_STRIP, ledType);
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

    if (bgLedPin > 0 && bgLedCount > 0) {
        int bgLedColorFormat = hardwareConfig.getBgLedColorFormat();
        uint8_t ledType = bgLedColorFormat + settings.getInt(BG_LED_FREQUENCY);
        LOG.printf("[LED] Initializing strip_bg on pin %d with %d LEDs, type %d (format:%d + freq:%d)\n", 
                   bgLedPin, bgLedCount, ledType,
                   bgLedColorFormat, settings.getInt(BG_LED_FREQUENCY));
        status = led.createStrip(strip_bg, bgLedPin, bgLedCount, 10, DefaultColors::BG_STRIP, ledType);
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

    
    if (serviceLedPin > 0) {
        int serviceLedColorFormat = hardwareConfig.getServiceLedColorFormat();
        uint8_t ledType = serviceLedColorFormat + settings.getInt(SERVICE_LED_FREQUENCY);
        LOG.printf("[LED] Initializing strip_service on pin %d with %d LEDs, type %d (format:%d + freq:%d)\n", 
                   serviceLedPin, num_leds_service, ledType,
                   serviceLedColorFormat, settings.getInt(SERVICE_LED_FREQUENCY));
        status = led.createStrip(strip_service, serviceLedPin, num_leds_service, 10, DefaultColors::SERVICE_STRIP, ledType);
        if (status != StripStatus::SUCCESS) {
            LOG.printf("[LED] ERROR: Failed to create strip_service: %d\n", status);
        } else {
            LOG.printf("[LED] SUCCESS: strip_service\n");
        }
    } else {
        LOG.printf("[LED] SKIP: strip_service (pin <= 0)\n");
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

void reconnectStrips() {
    LOG.printf("[LED] Reconnecting strips...\n");
    
    if (needReconnectMainStrip) {
        // Перезбираємо список індексів для Main
        allLedsMain.clear();
        for (uint32_t i = 0; i < num_leds_main; ++i) {
            allLedsMain.push_back(i);
        }
        reconnectStripMain();
        needReconnectMainStrip = false;
    }
    if (needReconnectBgStrip) {
        // Перезбираємо список індексів для BG
        allLedsBg.clear();
        int bgCount = hardwareConfig.getBgLedsCount();
        if (bgCount > 0) {
            for (uint32_t i = 0; i < (uint32_t)bgCount; ++i) {
                allLedsBg.push_back(i);
            }
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
    
    LOG.printf("[LED] Strip reconnection completed.\n");
    logMemoryUsage("after strip reconnection complete");
}

// --- TIME Functions ---

static TimezoneInfo* getTimezoneInfo(int timezoneId) {
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
    TimezoneInfo* tzInfo = getTimezoneInfo(timezoneId);
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
}

void adaptColors() {
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
}

// --- ALERT Functions ---


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
        api.updateClimateData(temp, hum, press);
    }
}

void initSettings() {
    LOG.printf("[INIT] Init settings\n");
    settings.init();
    fwUpdate.init(VERSION);
    
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
    settings.setChangeCallback([](Type type, int intValue, const char* strValue) {
        // Повідомляємо mDNS про зміни налаштувань
        mdnsService.onSettingsChange(type, intValue, strValue);
        
        // Повідомляємо API про зміни налаштувань
        api.onSettingsChange(type, intValue, strValue);
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
    //  lightSensor.begin(hardware);
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

    // initDisplayModes();
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
    generateCustomRegionMap(hardwareConfig);
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
    servicePin(POWER);
    stripMutex = xSemaphoreCreateMutex();
    if (stripMutex == NULL) {
        LOG.printf("[ERROR] Failed to create stripMutex semaphore\n");
        return;
    }

    // Ініціалізуємо стрічки з бажаними пінами
    initStripMain();
    initStripBg();
    initStripService();   
    needAdaptStripBrightness = true;

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
            // set flags to adapt colors on min of silence start
            needAdaptColors = true;
            needAdaptAnimationColors = true;
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
                // set flags to adapt colors on min of silence end
                needAdaptColors = true;
                needAdaptAnimationColors = true;
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
    
    api.updateSystemInfo(
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
  //updateHaTempSensors();
  //updateHaHumSensors();
  //updateHaPressureSensors();
}

void mainThreadProcess() {
    // Ця функція виконується в основному циклі
    // Вона потрібна для асинхронного менеджера, щоб мати можливість виконувати інші задачі

    if (needToRegenerateBgColorMap) {
        LOG.printf("[MAIN] Regenerating BG LED color map\n");
        generateBgLedColorsMap();
        needToRegenerateBgColorMap = false;
    }

    if (needReconnectMainStrip || needReconnectBgStrip || needReconnectServiceStrip) {
        LOG.printf("[MAIN] Reconnecting LED strip\n");
        reconnectStrips();
    }

    if (needReconnectWebsocket && !needReconnectMainStrip) {
        LOG.printf("[MAIN] Reconnecting WebSocket\n");
        needReconnectWebsocket = false;
        websocketConnected = false;
        socketConnect();
    }

    if (needRecalculateLeds) {
        generateCustomRegionMap(hardwareConfig);
        LOG.printf("[MAIN] Recalculating LEDs\n");
        needRecalculateLeds = false;
        //needReconnectWebsocket = true;
    }

    if (needAdaptColors) {
        adaptColors();
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
        //     LOG.printf("[ERROR] Failed to create animation\n");
        //     return;
        // }
    }
    if (needUpdateBatteryPin) {
        LOG.printf("[MAIN] Updating battery pin state\n");
        if (settings.getInt(BATTERY_PIN) > 0) {
            battery.setPin(settings.getInt(BATTERY_PIN));
        } else {
            LOG.printf("[MAIN] Battery pin not configured, skipping update\n");
        }
        needUpdateBatteryPin = false;
    }

    if (needReconfigureDisplay) {
        LOG.printf("[MAIN] Reconfiguring display\n");
        initDisplay();
        needReconfigureDisplay = false;
    }

    if (needReconfigureSound) {
        LOG.printf("[MAIN] Reconfiguring sound\n");
        initSound();
        needReconfigureSound = false;
    }

    if (needReconfigureButtons) {
        LOG.printf("[MAIN] Reconfiguring buttons\n");
        initButtons();
        needReconfigureButtons = false;
    }

    if (needUpdateAnimationsMode) {
        LOG.printf("[MAIN] Animations sync mode %s\n", settings.getInt(ENABLE_SYNC_ANIMATIONS) ? "ENABLED" : "DISABLED");
        animation.setSynchronizedMode(settings.getInt(ENABLE_SYNC_ANIMATIONS));
        needUpdateAnimationsMode = false;
    }

    if (needAdaptClimate) {
        LOG.printf("[MAIN] Adapting climate settings\n");
        climateProcess();
        needAdaptClimate = false;
    }

    if (needAdaptVolume) {
        LOG.printf("[MAIN] Adapting sound settings\n");
        sound.setVolumeNight(settings.getInt(MELODY_VOLUME_NIGHT));
        sound.setVolumeDay(settings.getInt(MELODY_VOLUME_DAY));
        needAdaptVolume = false;
    }

    if (needUpdateHomeAlertBit) {
        LOG.printf("[MAIN] Updating home alert bit\n");
        int localAlertBit = findHighestBitForRegionDirect(settings.getInt(HOME_DISTRICT));
        if (localAlertBit != alertBit) alertAction(localAlertBit, settings.getInt(HOME_DISTRICT));
        alertBit = localAlertBit;
        uint16_t homeAlertFlags = alertsMap[settings.getInt(HOME_DISTRICT)];
        LOG.printf("[MAIN] homeAlertFlags: %u\n", homeAlertFlags);
        api.updateHomeAlert(homeAlertFlags);
        uint8_t encodedTemp = temperatureMap[settings.getInt(HOME_DISTRICT)];
        int homeTemp = decodeTemperature(encodedTemp);
        api.updateHomeDistrictTemp(homeTemp);
        needUpdateHomeAlertBit = false;
    }

    if (needUpdateTimezone) {
        LOG.printf("[MAIN] Updating timezone\n");
        applyTimezoneSettings(settings.getInt(TIME_ZONE));
        needUpdateTimezone = false;
    }

    if (needPlayTestMelody) {
        LOG.printf("[MAIN] Playing test melody\n");
        playMelody(MELODIES[testMelodyId]);
        needPlayTestMelody = false;
    }
    if (fwUpdate.isUpdateRequested()) {
        LOG.printf("[MAIN] Updating firmware\n");
        display.showServiceMessage("Виконано", "Запит оновлення:", 3000);
        animation.clearAllAnimations();
        fwUpdate.download();
        fwUpdate.clearUpdateRequest();
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
void displayProcess()
{
    // Remove UA Anthem playing flag if anthem stopped
    if (uaAnthemPlaying && (!sound.isBuzzerPlaying() && !sound.isDFPlayerPlaying())) {
        uaAnthemPlaying = false;
        // set flags to adapt colors on min of silence end
        needAdaptColors = true;
        needAdaptAnimationColors = true;
    }
    // Show Minute of silence mode if activated. (Priority - 0)
    if (minuteOfSilence) {
        displayMinuteOfSilence();
        return;
    }
    // Show Glory To Ukraine if athema playing. (Priority - 1)
    if (uaAnthemPlaying) {
        showMinOfSilenceScreen(1);
        return;
    }
    if (fwUpdate.isUpdateAvailable() && settings.getBool(NEW_FW_NOTIFICATION)) {
        showNewFirmwareNotification();
        return;
    }
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
    checkMinuteOfSilence();
    if (needToPlaySound(REGULAR) && !minuteOfSilence && sound.beepHour != timeClient.hour() && timeClient.minute() == 0 && timeClient.second() == 0) {
        sound.setBeepHour(timeClient.hour());
        playMelody(REGULAR);
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

    fwUpdate.setDisplay(&display);
    fwUpdate.setMapUpdateCallback([](float p) { mapUpdate(p); });
    fwUpdate.setRebootCallback([]() { rebootDevice(); });
    fwUpdate.setServicePinCallback([]() { servicePin(UPD_AVAILABLE); });
    fwUpdate.initCallbacks();
    checkFreeHeap("updates initialization");

    // Передаємо settings в AnimationManager
    animation.setSettings(&settings);
    
    initStrip();
    checkFreeHeap("LED strips initialization");
    brightnessProcess();

    initMapping();
    checkFreeHeap("LED mapping initialization");

    // Адаптація led для відображення парпору України
    adaptColors();
    checkFreeHeap("colors adaptation");

    initButtons();
    checkFreeHeap("buttons initialization");

    initDisplay();
    display.drawIconWithText(JaamDisplayIcon::TRIDENT, "JAAM " + String(VERSION) + "");
    checkFreeHeap("display initialization");

    initStorage();
    checkFreeHeap("SPIFFS initialization");

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
    async.setInterval(batteryProcess, BATTERY_CHECK_INTERVAL);
    async.setInterval(displayProcess, DISPLAY_CHECK_INTERVAL);
    async.setInterval(climateProcess, CLIMATE_CHECK_INTERVAL);
    async.setInterval(volumeProcess, VOLUME_CHECK_INTERVAL);
    async.setInterval(beepHourProcess, BEEP_HOUR_CHECK_INTERVAL);

    checkFreeHeap("async tasks configuration");
    
    LOG.printf("[SETUP] Initialization complete\n");
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
    api.handleWebSocketClients();
    buttons.tick();
}
