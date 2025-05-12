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


// Глобальний менеджер анімацій
AnimationManager animManager;

// Зберігаємо тестові кольори
uint32_t testColor1 = 0;
uint32_t testColor2 = 0;
uint32_t testColor3 = 0;

uint32_t lastWifiConnectTime = 0;  // Track when WiFi was last connected

// Функція для створення стріпки з обробкою помилок
ErrorCode createStrip(Adafruit_NeoPixel*& strip, uint8_t pin, uint8_t count, uint8_t type) {
    strip = new Adafruit_NeoPixel(count, pin, type);
    if (strip == nullptr) {
        LOG.println("ERROR: Не вдалося створити стріпку");
        return ErrorCode::STRIP_CREATION_FAILED;
    }
    
    strip->begin();
    strip->setBrightness(brightnessVal(DEFAULT_BRIGHTNESS_PERCENT));
    strip->clear();
    
    // Встановлюємо дефолтний колір для всіх LED
    uint32_t defaultColor;
    if (pin == settings.getInt(MAIN_LED_PIN)) {
        defaultColor = DefaultColors::STRIP1;
    } else if (pin == settings.getInt(BG_LED_PIN)) {
        defaultColor = DefaultColors::STRIP2;
    } else {
        defaultColor = DefaultColors::STRIP3;
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
    int stripRand = random(0, 3);
    switch(stripRand) {
        case 0:
            strip = strip1;
            break;
        case 1:
            strip = strip2;
            break;
        case 2:
            strip = strip3;
            break;
        default:
            strip = strip1;
    }

    strip = strip1;
    
    // Перевірка на nullptr
    if (strip == nullptr) {
        LOG.println("ERROR: Спроба використання неініціалізованої стріпки");
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

    animType = AnimationParams::Type::BLEND_BLINK;
    
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
    
    error = createStrip(strip1, settings.getInt(MAIN_LED_PIN), 26, NEO_GRB + NEO_KHZ800);
    if (error != ErrorCode::SUCCESS) {
        LOG.println("ERROR: Не вдалося створити strip1");
        return;
    }
    
    error = createStrip(strip2, settings.getInt(BG_LED_PIN), settings.getInt(BG_LED_COUNT), NEO_GRB + NEO_KHZ800);
    if (error != ErrorCode::SUCCESS) {
        LOG.println("ERROR: Не вдалося створити strip2");
        return;
    }
    
    error = createStrip(strip3, settings.getInt(SERVICE_LED_PIN), 5, NEO_GRB + NEO_KHZ800);
    if (error != ErrorCode::SUCCESS) {
        LOG.println("ERROR: Не вдалося створити strip3");
        return;
    }

    // Тестовий патерн
    if (xSemaphoreTake(stripMutex, portMAX_DELAY) == pdTRUE) {
        // Зберігаємо тестові кольори
        testColor1 = strip1->Color(0, 255, 0);    // Зелений
        testColor2 = strip2->Color(255, 0, 0);    // Червоний
        testColor3 = strip3->Color(0, 0, 255);    // Синій

        for(int i = 0; i < 26; i++) {
            strip1->setPixelColor(i, testColor1);
            strip2->setPixelColor(i, testColor2);
            strip3->setPixelColor(i, testColor3);
        }
        strip1->show();
        strip2->show();
        strip3->show();
        xSemaphoreGive(stripMutex);
    }

    // Ініціалізуємо генератор випадкових чисел
    randomSeed(esp_random());

    // Налаштовуємо асинхронні задачі
    asyncEngine.setInterval(animations, ANIMATION_INTERVAL);
    asyncEngine.setInterval(memory, MEMORY_CHECK_INTERVAL);
    asyncEngine.setInterval(wifiReconnect, WIFI_CHECK_INTERVAL);
}

void loop() {
    wm.process();
    animManager.update();
    asyncEngine.run();
}

// Оновлена функція очищення
void cleanup() {
    if (strip1 != nullptr) {
        strip1->clear();
        // Встановлюємо дефолтний колір
        for(int i = 0; i < 26; i++) {
            strip1->setPixelColor(i, DefaultColors::STRIP1);
        }
        strip1->show();
        delete strip1;
        strip1 = nullptr;
    }
    if (strip2 != nullptr) {
        strip2->clear();
        // Встановлюємо дефолтний колір
        for(int i = 0; i < settings.getInt(BG_LED_COUNT); i++) {
            strip2->setPixelColor(i, DefaultColors::STRIP2);
        }
        strip2->show();
        delete strip2;
        strip2 = nullptr;
    }
    if (strip3 != nullptr) {
        strip3->clear();
        // Встановлюємо дефолтний колір
        for(int i = 0; i < 5; i++) {
            strip3->setPixelColor(i, DefaultColors::STRIP3);
        }
        strip3->show();
        delete strip3;
        strip3 = nullptr;
    }
}