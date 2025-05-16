#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>

#include <WiFiManager.h>
#include <math.h>
#include <esp_system.h>
#include <async.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "JaamAnimation.h"
#include "JaamLogs.h"
#include "JaamConfig.h"
#include "JaamUtils.h"
#include "JaamSettings.h"
#include "JaamWeb.h"
#include "JaamLed.h"

using namespace websockets;

char            chipID[13];
char            currentFwVersion[25];
int             currentIdx = 0;
uint32_t        loopCount = 0;
int             needRebootWithDelay = -1;

Async           async = Async(20);

JaamSettings    settings;
Firmware        firmware;
JaamWeb         web;
JaamLed         led;

// --- LED Configuration ---
Adafruit_NeoPixel*  strip_main = nullptr;
Adafruit_NeoPixel*  strip_bg = nullptr;
Adafruit_NeoPixel*  strip_service = nullptr;
SemaphoreHandle_t   stripMutex = nullptr;
uint16_t            num_leds_main = 26;
uint16_t            num_leds_service = 5;
uint8_t             homeDistrict = 18;

bool                strip_main_initialized = false;
bool                strip_bg_initialized = false;
bool                strip_service_initialized = false;

AnimationManager    animation;

// --- WIFI Configuration ---
WiFiManager         wm;
WiFiClient          client;
WebsocketsClient    client_websocket;
uint32_t            lastWifiConnectTime = 0;  // Track when WiFi was last connected

time_t  websocketLastPingTime = 0;
bool    isFirstDataFetchCompleted = false;
bool    apiConnected;
uint8_t     legacy = 4;
bool    websocketReconnect = false;
uint32_t     lastWebsocketConnectTime = 0;


void rebootDevice(int time = 2000, bool async = false) {
  if (async) {
    needRebootWithDelay = time;
    return;
  }
  //showServiceMessage("Перезавантаження..", "", time);
  delay(time);
  //display.clearDisplay();
  //display.display();
  ESP.restart();
}


static JsonDocument parseJson(const char* payload) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    LOG.printf("Deserialization error: $s\n", error.f_str());
    return doc;
  } else {
    return doc;
  }
}


//--Websocket process start

void onMessageCallback(WebsocketsMessage message) {
  LOG.print("Got Message: ");
  LOG.println(message.data());
  JsonDocument data = parseJson(message.data().c_str());
  String payload = data["payload"];
  if (!payload.isEmpty()) {
    if (payload == "ping") {
      LOG.println("Heartbeat from server");
      websocketLastPingTime = millis();
    }
  }
  isFirstDataFetchCompleted = true;
}

void onEventsCallback(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    apiConnected = true;
    LOG.println("connnection opened");
    //servicePin(DATA, HIGH, false);
    websocketLastPingTime = millis();
    //ha.setMapApiConnect(apiConnected);
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    apiConnected = false;
    LOG.println("connnection closed");
    //servicePin(DATA, LOW, false);
    //ha.setMapApiConnect(apiConnected);
  } else if (event == WebsocketsEvent::GotPing) {
    LOG.println("websocket ping");
    websocketLastPingTime = millis();
    client_websocket.pong();
  } else if (event == WebsocketsEvent::GotPong) {
    LOG.println("websocket pong");
  }
}

void socketConnect() {
  LOG.println("connection start...");
  //showServiceMessage("підключення...", "Сервер даних");
  client_websocket.onMessage(onMessageCallback);
  client_websocket.onEvent(onEventsCallback);
  long startTime = millis();
  char webSocketUrl[100];
  sprintf(
    webSocketUrl,
    "ws://%s:%d/data_v4",
    settings.getString(WS_SERVER_HOST),
    settings.getInt(WS_SERVER_PORT)
  );
  LOG.println(webSocketUrl);
  client_websocket.connect(webSocketUrl);
  if (client_websocket.available()) {
    LOG.print("connection time - ");
    LOG.print(millis() - startTime);
    LOG.println("ms");
    char chipIdInfo[25];
    sprintf(chipIdInfo, "chip_id:%s", chipID);
    LOG.println(chipIdInfo);
    client_websocket.send(chipIdInfo);
    char firmwareInfo[100];
    sprintf(firmwareInfo, "firmware:%s_%s", currentFwVersion, settings.getString(ID));
    LOG.println(firmwareInfo);
    client_websocket.send(firmwareInfo);

    char userInfo[250];
    JsonDocument userInfoJson;
    userInfoJson["legacy"] = legacy;
    sprintf(userInfo, "user_info:%s", userInfoJson.as<String>().c_str());
    LOG.println(userInfo);
    client_websocket.send(userInfo);
    client_websocket.ping();
    websocketReconnect = false;
    lastWebsocketConnectTime  = millis();
    //showServiceMessage("підключено!", "Сервер даних", 3000);
  } else {
    //showServiceMessage("недоступний", "Сервер даних", 3000);
  }
}

void websocketProcess() {
  if (millis() - websocketLastPingTime > settings.getInt(WS_ALERT_TIME)) {
    websocketReconnect = true;
  }
  if (millis() - websocketLastPingTime > settings.getInt(WS_REBOOT_TIME)) {
    rebootDevice(3000, true);
  }
  if (!client_websocket.available() or websocketReconnect) {
    LOG.println("Reconnecting...");
    socketConnect();
  }
}
//--Websocket process end








void animations() {
    loopCount++;
    int ledsIdx[1] = { currentIdx };
    uint8_t r = random(256), g = random(256), b = random(256);
    
    // Випадковий вибір стрічки
    Adafruit_NeoPixel* strip = nullptr;
    int stripRand = random(0, 2);
    switch(stripRand) {
        case 0:
            if (strip_main_initialized) strip = strip_main;
            break;
        case 1:
            if (strip_bg_initialized) strip = strip_bg;
            break;
        case 2:
            if (strip_service_initialized) strip = strip_service;
            break;
        default:
            if (strip_main_initialized) strip = strip_main;
    }

    strip = strip_main;

    if (!strip) {
        LOG.println("ERROR: Немає доступних ініціалізованих стрічок");
        return;
    }
    // r = 255;
    // g = 0;
    // b = 0;
    
    uint32_t color = strip->Color(r, g, b);

    // Випадковий вибір типу анімації
    AnimationParams::Type animType;
    int typeRand = random(0, 4); // 0, 1 або 2 для FADE, BLINK або BLEND_BLINK
    switch(typeRand) {
        case 0:
            animType = AnimationParams::Type::FADE;
            break;
        case 1:
            animType = AnimationParams::Type::BLINK;
            break;
        case 2:
            animType = AnimationParams::Type::BLEND_BLINK;
            break;
        case 3:
            animType = AnimationParams::Type::PULSE;
            break;
        default:
            animType = AnimationParams::Type::FADE;
    }
    animType = AnimationParams::Type::BLEND_BLINK;

    // Випадкові параметри для анімації з використанням конфігурації
    uint32_t period = 2000; //random(AnimationConfig::MIN_PERIOD, AnimationConfig::MAX_PERIOD + 1);
    uint8_t cycles = 3; // random(AnimationConfig::MIN_CYCLES, AnimationConfig::MAX_CYCLES + 1);
    uint8_t startBrightness = 50; // random(AnimationConfig::MIN_START_BRIGHTNESS, AnimationConfig::MAX_START_BRIGHTNESS + 1);
    uint8_t endBrightness = 255; //   random(AnimationConfig::MIN_END_BRIGHTNESS, AnimationConfig::MAX_END_BRIGHTNESS + 1);
    
    // Створення анімації з обробкою помилок
    if (!animation.createAnimation(
        animType,
        strip,
        ledsIdx,
        1,
        color,
        period,
        cycles,
        startBrightness,
        endBrightness
    )) {
        LOG.println("ERROR: Не вдалося створити анімацію");
        return;
    }

    currentIdx++;
    if (currentIdx >= num_leds_main) {
        currentIdx = 0;
    }
}

void memory() {
  size_t freeHeap    = ESP.getFreeHeap();
  size_t usedHeap    = ESP.getHeapSize() - freeHeap;
  size_t maxAlloc    = ESP.getMaxAllocHeap();
  uint32_t uptimeMin = millis() / 60000;
  
  // WiFi status information
  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  uint32_t wifiUptime = wifiConnected ? (millis() - lastWifiConnectTime) / 60000 : 0; 
  uint32_t websocketUptime = client_websocket.available() ? (millis() - lastWebsocketConnectTime) / 60000 : 0; // in seconds
  String wifiStatus = wifiConnected ? "Connected" : "Disconnected";
  String websocketStatus = client_websocket.available() ? "Connected" : "Disconnected";
  String wifiIP = wifiConnected ? WiFi.localIP().toString() : "N/A";

  LOG.printf(
    "Loop %u: LED %d, used heap %u B, free heap %u B, uptime %u min. WiFi: %s, %u min. WebSocket: %s, %u min\n",
    loopCount,
    currentIdx,
    usedHeap,
    freeHeap,
    uptimeMin,
    wifiStatus.c_str(),
    wifiUptime,
    websocketStatus.c_str(),
    websocketUptime
  );
}

void initSettings() {
  LOG.println("Init settings");
  settings.init();
  firmware = parseFirmwareVersion(VERSION);
  LOG.printf("major: %d, minor: %d, patch: %d, isBeta: %d, betaBuild: %d\n",
           firmware.major, firmware.minor, firmware.patch, firmware.isBeta, firmware.betaBuild);
  fillFwVersion(currentFwVersion, firmware);
  LOG.printf("Current firmware version: %s\n", currentFwVersion);
}


void initChipID() {
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(chipID, "%04x%04x", (uint32_t)(chipid >> 32), (uint32_t)chipid);
  LOG.printf("ChipID Inited: '%s'\n", chipID);
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
        LOG.println("WiFi is disabled in configuration");
        return;
    }

    LOG.println("Initializing WiFi...");
    
    // Set WiFi to station mode
    WiFi.mode(WIFI_STA);
    
    // Configure WiFiManager
    wm.setHostname("jaam_fusion");
    wm.setTitle("JAAM Fusion WiFi Setup");
    wm.setConfigPortalBlocking(true);
    wm.setConnectTimeout(WiFiConfig::CONNECT_TIMEOUT / 1000);
    wm.setConnectRetries(WiFiConfig::CONNECT_RETRIES);
    wm.setAPCallback(apCallback);
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setConfigPortalTimeout(WiFiConfig::PORTAL_TIMEOUT / 1000); 
    
    // Create AP name with chip ID
    char apName[32];
    snprintf(apName, sizeof(apName), "JAAM_FUSION_%s", chipID);
    
    // Try to connect to saved WiFi
    LOG.println("Attempting to connect to saved WiFi...");
    if (!wm.autoConnect(apName)) {
        LOG.println("Reboot");
        rebootDevice(5000);
        return;
    }
    
    lastWifiConnectTime = millis();  // Record connection time
    LOG.println("Connected to WiFi");
    LOG.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    
    // Start web portal
    wm.setHttpPort(WiFiConfig::WEB_PORT);
    wm.startWebPortal();
    LOG.println("Web portal started on port 8080");
    socketConnect();
    delay(5000);
}

void initStrip() {
    // Створюємо м'ютекс для захисту доступу до стрічок
    stripMutex = xSemaphoreCreateMutex();
    if (stripMutex == NULL) {
        LOG.println("ERROR: Не вдалося створити семафор stripMutex");
        return;
    }

    // Створюємо екземпляр JaamLed
    

    // Ініціалізуємо стрічки з бажаними пінами
    StripStatus status;
    
    status = led.createStrip(strip_main, settings.getInt(MAIN_LED_PIN), num_leds_main, settings.getInt(BRIGHTNESS), DefaultColors::MAIN_STRIP, NEO_GRB + NEO_KHZ800);
    if (status != StripStatus::SUCCESS) {
        LOG.printf("ERROR: Не вдалося створити strip_main: %d\n", status);
    } else {
        LOG.println("SUCCESS: strip_main");
        strip_main_initialized = true;
    }
    
    status = led.createStrip(strip_bg, settings.getInt(BG_LED_PIN), settings.getInt(BG_LED_COUNT), settings.getInt(BRIGHTNESS_BG), DefaultColors::BG_STRIP, NEO_GRB + NEO_KHZ800);
    if (status != StripStatus::SUCCESS) {
        LOG.printf("ERROR: Не вдалося створити strip_bg: %d\n", status);
    } else {
        LOG.println("SUCCESS: strip_bg");
        strip_bg_initialized = true;
    }

    status = led.createStrip(strip_service, settings.getInt(SERVICE_LED_PIN), num_leds_service, settings.getInt(BRIGHTNESS_SERVICE), DefaultColors::SERVICE_STRIP, NEO_GRB + NEO_KHZ800);
    if (status != StripStatus::SUCCESS) {
        LOG.printf("ERROR: Не вдалося створити strip_service: %d\n", status);
    } else {
        LOG.println("SUCCESS: strip_service");
        strip_service_initialized = true;
    }

    if (strip_main_initialized) {
        animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
            for(uint16_t i = 0; i < num_leds_main; i++) {
                uint32_t color = animation.ledActualColor(strip, i);
                strip->setPixelColor(i, color);
            }
            strip->show();
        });
    }
    
    if (strip_bg_initialized) {
        animation.safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
            for(int i = 0; i < settings.getInt(BG_LED_COUNT); i++) {
                uint32_t color = animation.ledActualColor(strip, i);
                strip->setPixelColor(i, color);
            }
            strip->show();
        });
    }
    
    if (strip_service_initialized) {
        animation.safeStripOperation(strip_service, [](Adafruit_NeoPixel* strip) {
            for(int i = 0; i < num_leds_service; i++) {
                uint32_t color = animation.ledActualColor(strip, i);
                strip->setPixelColor(i, color);
            }
            strip->show();
        });
    }
}

void wifiReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    LOG.println("WiFI Reconnect");
    initWifi();
  }
}

void get_pixel_color() {
  if (strip_main_initialized) {
    animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
      uint32_t color = strip->getPixelColor(currentIdx);
      uint8_t r = (color >> 16) & 0xFF;
      uint8_t g = (color >> 8) & 0xFF;
      uint8_t b = color & 0xFF;
      LOG.printf("Pixel %d color: R=%d, G=%d, B=%d (0x%06X)\n", 
                currentIdx, r, g, b, color);
    });
  }

  // Виводимо інформацію про активні анімації
  animation.logActiveAnimations();
}

void logFreeMainLeds() {
    if (!strip_main_initialized) {
        LOG.println("strip_main не ініціалізовано");
        return;
    }
    auto freeLeds = animation.getFreeLeds(strip_main, num_leds_main);
    LOG.printf("Вільних LED на strip_main: %d\n", (int)freeLeds.size());
    LOG.print("Список: ");
    for (const auto& led : freeLeds) {
        LOG.printf("%d ", led.ledIdx);
    }
    LOG.println();
}

void setup() {
    LOG.begin(115200);

    initChipID();
    initSettings();
    initWifi();
    initStrip();

    // Ініціалізуємо генератор випадкових чисел
    randomSeed(esp_random());
    
    // Передаємо settings в AnimationManager
    animation.setSettings(&settings);
    led.setSettings(&settings);
    // Налаштовуємо асинхронні задачі
    async.setInterval(animations, ANIMATION_INTERVAL);
    //async.setInterval(get_pixel_color, ANIMATION_INTERVAL);
    async.setInterval(memory, MEMORY_CHECK_INTERVAL);
    async.setInterval(wifiReconnect, WIFI_CHECK_INTERVAL);
    async.setInterval(websocketProcess, WEBSOCKET_CHECK_INTERVAL);
    //async.setInterval(logFreeMainLeds, 5000); // 5000 мс = 5 секунд, змініть інтервал за потреби

    // Ініціалізація веб-інтерфейсу
    web.begin(strip_main, strip_bg, strip_service);
    web.setSettings(&settings);
}

void loop() {
    wm.process();
    client_websocket.poll();
    animation.update();
    async.run();
    web.handleClient();
}

// функція очищення
void cleanup() {
    if (strip_main_initialized) {
        animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
            strip->clear();
            // Встановлюємо дефолтний колір
            for(int i = 0; i < num_leds_main; i++) {
                strip->setPixelColor(i, DefaultColors::MAIN_STRIP);
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
                strip->setPixelColor(i, DefaultColors::BG_STRIP);
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
                strip->setPixelColor(i, DefaultColors::SERVICE_STRIP);
            }
            strip->show();
            delete strip;
            strip_service = nullptr;
            strip_service_initialized = false;
        });
    }
}