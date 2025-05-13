#include <Arduino.h>

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

char        chipID[13];
char        currentFwVersion[25];
int         currentIdx = 0;
uint32_t    loopCount = 0;
int         needRebootWithDelay = -1;

WiFiManager wm;
WiFiClient  client;

Async       asyncEngine = Async(20);

JaamSettings settings;
Firmware currentFirmware;
JaamWeb webInterface;

// Глобальні змінні для стріпок
Adafruit_NeoPixel* strip_main = nullptr;
Adafruit_NeoPixel* strip_bg = nullptr;
Adafruit_NeoPixel* strip_service = nullptr;
SemaphoreHandle_t stripMutex = nullptr;

// Глобальні змінні для стану стріпок
bool strip_main_initialized = false;
bool strip_bg_initialized = false;
bool strip_service_initialized = false;

// Глобальний менеджер анімацій
AnimationManager animManager;

// Зберігаємо тестові кольори
uint32_t testColor1 = 0;
uint32_t testColor2 = 0;
uint32_t testColor3 = 0;

uint32_t lastWifiConnectTime = 0;  // Track when WiFi was last connected

// Функція для створення стріпки з обробкою помилок
ErrorCode createStrip(Adafruit_NeoPixel*& strip, int pin, uint8_t count, uint8_t brightness, uint8_t type) {
    if (pin <= 0) {
        LOG.printf("ERROR: пін не вказано: %d\n", pin);
        return ErrorCode::STRIP_PIN_NOT_SET;
    }
    
    strip = new Adafruit_NeoPixel(count, pin, type);
    if (strip == nullptr) {
        LOG.println("ERROR: Не вдалося створити стріпку");
        return ErrorCode::STRIP_CREATION_FAILED;
    }
    
    strip->begin();
    strip->setBrightness(brightnessVal(brightness));
    strip->clear();
    
    // Встановлюємо дефолтний колір для всіх LED
    uint32_t defaultColor;
    if (pin == settings.getInt(MAIN_LED_PIN)) {
        defaultColor = DefaultColors::MAIN_STRIP;
    } else if (pin == settings.getInt(BG_LED_PIN)) {
        defaultColor = DefaultColors::BG_STRIP;
    } else {
        defaultColor = DefaultColors::SERVICE_STRIP;
    }
    
    for(int i = 0; i < count; i++) {
        strip->setPixelColor(i, defaultColor);
    }
    strip->show();
    return ErrorCode::SUCCESS;
}

void animations() {
    loopCount++;
    int ledsIdx[1] = { currentIdx };
    uint8_t r = random(256), g = random(256), b = random(256);
    
    // Випадковий вибір стріпки
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
        LOG.println("ERROR: Немає доступних ініціалізованих стріпок");
        return;
    }
    
    uint32_t color = strip->Color(r, g, b);

    // Випадковий вибір типу анімації
    AnimationParams::Type animType;
    int typeRand = random(0, 3); // 0, 1 або 2 для FADE, BLINK або BLEND_BLINK
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
        default:
            animType = AnimationParams::Type::FADE;
    }

    // Випадкові параметри для анімації з використанням конфігурації
    uint32_t period = random(AnimationConfig::MIN_PERIOD, AnimationConfig::MAX_PERIOD + 1);
    uint8_t cycles = random(AnimationConfig::MIN_CYCLES, AnimationConfig::MAX_CYCLES + 1);
    uint8_t startBrightness = random(AnimationConfig::MIN_START_BRIGHTNESS, AnimationConfig::MAX_START_BRIGHTNESS + 1);
    uint8_t endBrightness = random(AnimationConfig::MIN_END_BRIGHTNESS, AnimationConfig::MAX_END_BRIGHTNESS + 1);
    
    // Створення анімації з обробкою помилок
    if (!animManager.createAnimation(
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
    if (currentIdx >= 26) {
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
  uint32_t wifiUptime = wifiConnected ? (millis() - lastWifiConnectTime) / 60000 : 0;  // in seconds
  String wifiStatus = wifiConnected ? "Connected" : "Disconnected";
  String wifiIP = wifiConnected ? WiFi.localIP().toString() : "N/A";

  LOG.printf(
    "Loop %u: LED %d, used heap %u B, free heap %u B, uptime %u min. WiFi: %s, uptime: %u min\n",
    loopCount,
    currentIdx,
    usedHeap,
    freeHeap,
    uptimeMin,
    wifiStatus.c_str(),
    wifiUptime
  );
}

void initSettings() {
  LOG.println("Init settings");
  settings.init();
  currentFirmware = parseFirmwareVersion(VERSION);
  fillFwVersion(currentFwVersion, currentFirmware);
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
    delay(5000);
}

void wifiReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    LOG.println("WiFI Reconnect");
    initWifi();
  }
}

void setup() {
    LOG.begin(115200);

    initChipID();
    initSettings();
    initWifi();
    
    // Створюємо м'ютекс для захисту доступу до стріпів
    stripMutex = xSemaphoreCreateMutex();
    if (stripMutex == NULL) {
        LOG.println("ERROR: Не вдалося створити семафор stripMutex");
        return;
    }

    // Ініціалізуємо стріпки з бажаними пінами
    ErrorCode error;
    
    error = createStrip(strip_main, settings.getInt(MAIN_LED_PIN), 26, settings.getInt(BRIGHTNESS), NEO_GRB + NEO_KHZ800);
    if (error != ErrorCode::SUCCESS) {
        LOG.println("ERROR: Не вдалося створити strip_main");
    } else {
        LOG.println("SUCCESS: strip_main");
        strip_main_initialized = true;
    }
    
    error = createStrip(strip_bg, settings.getInt(BG_LED_PIN), settings.getInt(BG_LED_COUNT), settings.getInt(BRIGHTNESS), NEO_GRB + NEO_KHZ800);
    if (error != ErrorCode::SUCCESS) {
        LOG.println("ERROR: Не вдалося створити strip_bg");
    } else {
        LOG.println("SUCCESS: strip_bg");
        strip_bg_initialized = true;
    }

    
    error = createStrip(strip_service, settings.getInt(SERVICE_LED_PIN), 5, settings.getInt(BRIGHTNESS), NEO_GRB + NEO_KHZ800);
    if (error != ErrorCode::SUCCESS) {
        LOG.println("ERROR: Не вдалося створити strip_service");
    } else {
        LOG.println("SUCCESS: strip_service");
        strip_service_initialized = true;
    }
    // Тестовий патерн
    if (strip_main_initialized) {
        safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
            testColor1 = strip->Color(0, 255, 0);    // Зелений
            for(int i = 0; i < 26; i++) {
                strip->setPixelColor(i, testColor1);
            }
            strip->show();
        });
    }
    
    if (strip_bg_initialized) {
        safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
            testColor2 = strip->Color(255, 0, 0);    // Червоний
            for(int i = 0; i < settings.getInt(BG_LED_COUNT); i++) {
                strip->setPixelColor(i, testColor2);
            }
            strip->show();
        });
    }
    
    if (strip_service_initialized) {
        safeStripOperation(strip_service, [](Adafruit_NeoPixel* strip) {
            testColor3 = strip->Color(0, 0, 255);    // Синій
            for(int i = 0; i < 5; i++) {
                strip->setPixelColor(i, testColor3);
            }
            strip->show();
        });
    }

    // Ініціалізуємо генератор випадкових чисел
    randomSeed(esp_random());

    // Налаштовуємо асинхронні задачі
    asyncEngine.setInterval(animations, ANIMATION_INTERVAL);
    asyncEngine.setInterval(memory, MEMORY_CHECK_INTERVAL);
    asyncEngine.setInterval(wifiReconnect, WIFI_CHECK_INTERVAL);

    // Ініціалізація веб-інтерфейсу
    webInterface.begin(&settings, strip_main, strip_bg, strip_service);
}

void loop() {
    wm.process();
    animManager.update();
    asyncEngine.run();

    // Обробка веб-клієнтів
    webInterface.handleClient();
}

// Оновлена функція очищення
void cleanup() {
    if (strip_main_initialized) {
        safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
            strip->clear();
            // Встановлюємо дефолтний колір
            for(int i = 0; i < NUM_LEDS_MAIN; i++) {
                strip->setPixelColor(i, DefaultColors::MAIN_STRIP);
            }
            strip->show();
            delete strip;
            strip_main = nullptr;
            strip_main_initialized = false;
        });
    }
    
    if (strip_bg_initialized) {
        safeStripOperation(strip_bg, [](Adafruit_NeoPixel* strip) {
            strip->clear();
            // Встановлюємо дефолтний колір
            for(int i = 0; i < NUM_LEDS_BG; i++) {
                strip->setPixelColor(i, DefaultColors::BG_STRIP);
            }
            strip->show();
            delete strip;
            strip_bg = nullptr;
            strip_bg_initialized = false;
        });
    }
    
    if (strip_service_initialized) {
        safeStripOperation(strip_service, [](Adafruit_NeoPixel* strip) {
            strip->clear();
            // Встановлюємо дефолтний колір
            for(int i = 0; i < NUM_LEDS_SERVICE; i++) {
                strip->setPixelColor(i, DefaultColors::SERVICE_STRIP);
            }
            strip->show();
            delete strip;
            strip_service = nullptr;
            strip_service_initialized = false;
        });
    }
}