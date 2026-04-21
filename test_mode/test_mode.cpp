// test_mode/test_mode.cpp
//
// Two-phase firmware:
//   PHASE_TEST   — hardware diagnostics: LED snake, sensor readings, button events
//   PHASE_UPDATE — write NVS defaults + OTA flash (triggered by BTN1 long press)
//
// Build: pio run -e test_mode
// Set JAAM_VERSION in platformio.ini: 1 = JAAM 1.3, 2 = JAAM 2.1, 3 = JAAM 3.2 (default)

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

#include "../src/JaamConfig.h"
#include "../src/JaamHardware.h"
#include "../src/JaamSettings.h"
#include "../src/JaamLogs.h"
#include "../src/JaamDisplay.h"
#include "../src/JaamButton.h"
#include "../src/JaamSound.h"
#include "../src/JaamLed.h"
#include "../src/JaamClimateSensor.h"
#include "../src/JaamLightSensor.h"

// ─── Globals required by JaamLed.h (extern declarations) ──────────────────────
Adafruit_NeoPixel* strip_main    = nullptr;
Adafruit_NeoPixel* strip_bg      = nullptr;
Adafruit_NeoPixel* strip_service = nullptr;
SemaphoreHandle_t  stripMutex    = nullptr;

// Required by JaamHardware.cpp, JaamLed.cpp, JaamDisplay.cpp, JaamClimateSensor.cpp
JaamSettings settings;
JaamHardware hardwareConfig;

// ─── Hardware config (JAAM_VERSION drives both LED config and NVS defaults) ───
#if JAAM_VERSION == 3
  static const int     MAIN_COUNT  = JaamHardwareLed::MAIN_LED_COUNT_JAAM_3_2;  // 405
  static const int     BG_COUNT    = JaamHardwareLed::BG_LED_COUNT_JAAM_3;       // 39
  static const int     SVC_COUNT   = JaamHardwareLed::SERVICE_LED_COUNT_DEFAULT; // 5
  static const uint8_t MAX_BR      = JaamHardwareLed::BRIGHTNESS_JAAM_3_2_MAX;  // 35
  static const int     BTN1_PIN    = JaamHardwarePins::BUTTON_1_PIN_JAAM_3;      // 5
  static const int     BTN2_PIN    = JaamHardwarePins::BUTTON_2_PIN_JAAM;        // 2
  static const int     BTN3_PIN    = JaamHardwarePins::BUTTON_3_PIN_JAAM_3;      // 4
  static const auto    DISP_TYPE   = JaamDisplayType::SH1106G;
#elif JAAM_VERSION == 2
  static const int     MAIN_COUNT  = JaamHardwareLed::MAIN_LED_COUNT_JAAM_2_1;  // 26
  static const int     BG_COUNT    = JaamHardwareLed::BG_LED_COUNT_JAAM_2;       // 44
  static const int     SVC_COUNT   = JaamHardwareLed::SERVICE_LED_COUNT_DEFAULT; // 5
  static const uint8_t MAX_BR      = JaamHardwareLed::BRIGHTNESS_JAAM_2_1_MAX;  // 65
  static const int     BTN1_PIN    = JaamHardwarePins::BUTTON_1_PIN_JAAM_2;      // 4
  static const int     BTN2_PIN    = JaamHardwarePins::BUTTON_2_PIN_JAAM;        // 2
  static const int     BTN3_PIN    = JaamHardwarePins::BUTTON_3_PIN_DISABLED;    // -1
  static const auto    DISP_TYPE   = JaamDisplayType::SH1106G;
#else  // JAAM_VERSION == 1
  static const int     MAIN_COUNT  = JaamHardwareLed::MAIN_LED_COUNT_JAAM_1_3;  // 26
  static const int     BG_COUNT    = JaamHardwareLed::BG_LED_COUNT_DISABLED;     // -1
  static const int     SVC_COUNT   = JaamHardwareLed::SERVICE_LED_COUNT_DISABLED;// -1
  static const uint8_t MAX_BR      = JaamHardwareLed::BRIGHTNESS_JAAM_1_3_MAX;  // 127
  static const int     BTN1_PIN    = JaamHardwarePins::BUTTON_1_PIN_JAAM_1;      // 35
  static const int     BTN2_PIN    = JaamHardwarePins::BUTTON_2_PIN_DISABLED;    // -1
  static const int     BTN3_PIN    = JaamHardwarePins::BUTTON_3_PIN_DISABLED;    // -1
  static const auto    DISP_TYPE   = JaamDisplayType::SSD1306;
#endif

// ─── Updater config (edit before flashing) ────────────────────────────────────
static const int   orderNumber   = 0;    // Order number → id = "JAAMx-yyyy"
static const char* ssid          = "";   // WiFi SSID for OTA
static const char* password      = "";   // WiFi password
static const char* userSsid      = "";   // User WiFi SSID (saved to device)
static const char* userPassword  = "";   // User WiFi password
static const char* firmwareUrl   = "https://update.jaam.net.ua/5.0";
static const int   homeDistrict  = 31;   // Default home region ID (31 = Kyiv)

// ─── Module instances ──────────────────────────────────────────────────────────
JaamDisplay       display;
JaamButton        buttons;
JaamSound         sound;
JaamClimateSensor climate;
JaamLightSensor   lightSensor;

// ─── State ─────────────────────────────────────────────────────────────────────
enum Phase { PHASE_TEST, PHASE_UPDATING };
static Phase         currentPhase    = PHASE_TEST;
static bool          btnEventPending = false;
static unsigned long btnEventTime    = 0;
static unsigned long lastDisplayUpd  = 0;

// ─── Snake animation state ─────────────────────────────────────────────────────
static const int     SNAKE_LEN   = 9;
static const uint32_t SNAKE_STEP = 25;   // ms per step (~40 fps)
static float         snakePos    = 0.0f;
static unsigned long lastSnakeMs = 0;

// ─── Bg breathing state ────────────────────────────────────────────────────────
static uint8_t       bgBr    = 0;
static int8_t        bgDir   = 1;
static unsigned long lastBgMs = 0;

// ─── Service snake state ───────────────────────────────────────────────────────
static const int     SNAKE_LEN_SVC  = 3;   // shorter trail for 5-LED strip
static const uint32_t SNAKE_STEP_SVC = 80;  // slower to be visually distinct
static float         svcPos    = 0.0f;
static unsigned long lastSvcMs = 0;

static const char* STARTUP_MELODY = "Boot:d=16,o=5,b=220:c,e,g,c6";

// ─── Forward declarations ──────────────────────────────────────────────────────
void startUpdating();
void updateIdleDisplay();

// ─── Button callbacks ──────────────────────────────────────────────────────────
static void showBtnEvent(const char* btn, const char* action) {
    display.printMessage(btn, action);
    LOG.printf("[TEST] %s %s\n", btn, action);
    btnEventPending = true;
    btnEventTime    = millis();
}

static void onBtn1Click()     { showBtnEvent("BTN 1", "Click"); }
static void onBtn1LongClick() { startUpdating(); }
static void onBtn2Click()     { showBtnEvent("BTN 2", "Click"); }
static void onBtn2LongClick() { showBtnEvent("BTN 2", "Long Click"); }
static void onBtn3Click()     { showBtnEvent("BTN 3", "Click"); }
static void onBtn3LongClick() { showBtnEvent("BTN 3", "Long Click"); }

// ─── LED: snake on strip_main ──────────────────────────────────────────────────
static void updateSnake() {
    if (!strip_main) return;
    if (millis() - lastSnakeMs < SNAKE_STEP) return;
    lastSnakeMs = millis();

    const int n = strip_main->numPixels();
    const int head = (int)snakePos % n;

    strip_main->clear();
    for (int i = 0; i < SNAKE_LEN; i++) {
        int pos = (head - i + n) % n;
        // Brightness fades from head to tail: 255 → ~28
        uint8_t br = (uint8_t)((SNAKE_LEN - i) * 255 / SNAKE_LEN);
        // Scale by hardware max brightness
        br = (uint8_t)((uint32_t)br * MAX_BR / 255);
        strip_main->setPixelColor(pos, 0, 0, br);  // blue
    }
    strip_main->show();

    snakePos += 0.5f;
    if (snakePos >= (float)n) snakePos -= (float)n;
}

// ─── LED: green breathing on strip_bg ─────────────────────────────────────────
static void updateBgBreathing() {
    if (!strip_bg) return;
    if (millis() - lastBgMs < 20) return;
    lastBgMs = millis();

    bgBr = (uint8_t)constrain((int)bgBr + bgDir * 2, 0, MAX_BR);
    if (bgBr >= MAX_BR) bgDir = -1;
    if (bgBr == 0)      bgDir =  1;

    for (int i = 0; i < strip_bg->numPixels(); i++) {
        strip_bg->setPixelColor(i, 0, bgBr, 0);  // green
    }
    strip_bg->show();
}

// ─── LED: red snake on strip_service ──────────────────────────────────────────
static void updateServiceSnake() {
    if (!strip_service) return;
    if (millis() - lastSvcMs < SNAKE_STEP_SVC) return;
    lastSvcMs = millis();

    const int n = strip_service->numPixels();
    const int head = (int)svcPos % n;

    strip_service->clear();
    for (int i = 0; i < SNAKE_LEN_SVC; i++) {
        int pos = (head - i + n) % n;
        uint8_t br = (uint8_t)((SNAKE_LEN_SVC - i) * MAX_BR / SNAKE_LEN_SVC);
        strip_service->setPixelColor(pos, br, 0, 0);  // red
    }
    strip_service->show();

    svcPos += 1.0f;
    if (svcPos >= (float)n) svcPos -= (float)n;
}

// ─── Idle display (sensor readings) ───────────────────────────────────────────
void updateIdleDisplay() {
    climate.read();
    lightSensor.read();

    String l1 = climate.isTemperatureAvailable()
        ? ("T: " + String(climate.getTemperature(), 1) + " C")
        : "T: N/A";
    String l2 = climate.isHumidityAvailable()
        ? ("H: " + String(climate.getHumidity(), 1) + " %")
        : "H: N/A";
    String l3 = lightSensor.isLightSensorAvailable()
        ? ("L: " + String(lightSensor.getLightLevel(), 0) + " lux")
        : "L: N/A";

    display.printMultilineMessage(l1, l2, l3, "Btn1L=Update", "TEST MODE");
}

// ─── Updater phase ─────────────────────────────────────────────────────────────
static void writeNvsDefaults() {
    char deviceId[12];  // "JAAM3-9999\0"
    snprintf(deviceId, sizeof(deviceId), "JAAM%d-%04d", JAAM_VERSION, orderNumber);

    Preferences prefs;
    prefs.begin("storage", false);
    prefs.clear();
    prefs.putString("id", deviceId);    // device id
    prefs.putInt("hmd", homeDistrict);  // home district

#if JAAM_VERSION == 1
    prefs.putInt("legacy", 0);          // JAAM 1
    prefs.putInt("bm",   1);            // button mode (1 - map mode change)
    prefs.putInt("bml",  0);            // button long mode (0 - toggle display and map)
    prefs.putInt("brightness", 100);    // display brightness (0-100)
    prefs.putInt("brd", 100);           // auto brightness for day (0-100)
    LOG.printf("[UPDATE] NVS: id=%s, JAAM 1 defaults applied\n", deviceId);
#elif JAAM_VERSION == 2
    prefs.putInt("legacy", 3);          // JAAM 2.1
    prefs.putInt("bm",   1);            // button mode (1 - map mode change)
    prefs.putInt("b2m",  2);            // button 2 mode (2 - display mode change)
    prefs.putInt("bml",  0);            // button long mode (0 - toggle display and map)
    prefs.putInt("b2ml", 6);            // button 2 long mode (6 - toggle night brightness)
    prefs.putInt("sobc", 1);            // sound of button click (0 - off, 1 - on)
    prefs.putInt("brightness", 100);    // display brightness (0-100)
    prefs.putInt("brd", 100);           // auto brightness for day (0-100)
    LOG.printf("[UPDATE] NVS: id=%s, JAAM 2 defaults applied\n", deviceId);
#else  // JAAM_VERSION == 3
    prefs.putInt("legacy", 6);          // JAAM 3.2
    prefs.putInt("bm",   1);            // button mode (1 - map mode change)
    prefs.putInt("b2m",  2);            // button 2 mode (2 - display mode change)
    prefs.putInt("b3m",  5);            // button 3 mode (5 - toggle map and display)
    prefs.putInt("bml",  0);            // button long mode (0 - toggle display and map)
    prefs.putInt("b2ml", 6);            // button 2 long mode (6 - toggle night brightness)
    prefs.putInt("b3ml", 10);           // button 3 long mode (10 - reboot device)
    prefs.putInt("sobc", 1);            // sound of button click (0 - off, 1 - on)
    prefs.putInt("brightness", 100);    // display brightness (0-100)
    prefs.putInt("brd", 100);           // auto brightness for day (0-100)
    LOG.printf("[UPDATE] NVS: id=%s, JAAM 3 defaults applied\n", deviceId);
#endif

    prefs.end();
}

static void saveUserWifi() {
  Serial.println("[UPDATE] Disconnecting from WiFi...");
  bool disconnect = WiFi.disconnect(false, true);
  Serial.println(disconnect ? "[UPDATE] Disconnected from WiFi" : "[UPDATE] Disconnect from WiFi failed");
  if (userSsid != "" && userPassword != "") {
    WiFi.begin(userSsid, userPassword);
    WiFi.waitForConnectResult(10000);
    Serial.printf("[UPDATE] Saved User WiFi creds. SSID: %s\n", userSsid);
  } else {
    Serial.println("[UPDATE] No User WiFi creds, erased..."); }
}

void startUpdating() {
    currentPhase = PHASE_UPDATING;

    if (strip_main)    { strip_main->clear();    strip_main->show(); }
    if (strip_bg)      { strip_bg->clear();      strip_bg->show(); }
    if (strip_service) { strip_service->clear(); strip_service->show(); }

    LOG.println("[UPDATE] Phase: UPDATING");
    display.printMessage("UPDATE", "Writing NVS...");
    writeNvsDefaults();

    if (strlen(ssid) == 0) {
        display.printMessage("UPDATE", "No WiFi config");
        LOG.println("[UPDATE] ssid is empty — NVS written, OTA skipped");
        return;
    }

    display.printMessage("UPDATE", "WiFi...");
    WiFi.begin(ssid, password);

    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - t0 > 20000) {
            display.printMessage("UPDATE", "WiFi timeout");
            LOG.println("[UPDATE] WiFi connect timeout");
            return;
        }
        delay(500);
        LOG.printf("[UPDATE] Connecting to %s...\n", ssid);
    }
    LOG.println("[UPDATE] WiFi connected");
    display.printMessage("UPDATE", "Downloading...");

    WiFiClientSecure client;
    client.setInsecure();
    httpUpdate.rebootOnUpdate(false);
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    httpUpdate.onStart([]() {
        display.printMessage("UPDATE", "Started...");
        LOG.println("[UPDATE] OTA started");
    });

    httpUpdate.onProgress([](int cur, int total) {
        if (total == 0) return;
        int pct = cur / (total / 100);
        display.printMessage("UPDATE", String(pct) + "%");
        LOG.printf("[UPDATE] %d%%\n", pct);
    });

    httpUpdate.onEnd([]() {
        display.printMessage("UPDATE", "Done! Rebooting");
        LOG.println("[UPDATE] OTA complete");
        saveUserWifi();
        delay(1000);
        ESP.restart();
    });

    httpUpdate.onError([](int err) {
        display.printMessage("UPDATE ERR", String(err));
        LOG.printf("[UPDATE] OTA error %d: %s\n", err, httpUpdate.getLastErrorString().c_str());
    });

    httpUpdate.update(client, firmwareUrl);
}

// ─── Arduino entry points ──────────────────────────────────────────────────────
void setup() {
    LOG.begin(115200);
    delay(200);
    LOG.println("\n=== TEST MODE ===");

    stripMutex = xSemaphoreCreateMutex();

    // 1. Display
    display.begin(DISP_TYPE, JaamDisplayHeight::HEIGHT_64);
    display.printMessage("TEST MODE", "Init...");

    // 2. Settings (provides default values for sensor corrections, display font, etc.)
    settings.init();

    // 3. Main LED strip
    JaamLed::createStrip(strip_main,
                         JaamHardwarePins::MAIN_LED_PIN_JAAM,
                         MAIN_COUNT,
                         NEO_GRB + NEO_KHZ800);
    if (strip_main) {
        strip_main->setBrightness(255);  // direct color values control brightness in snake
        strip_main->clear();
        strip_main->show();
    }

    // 4. Background LED strip (skip for JAAM 1.3)
    if (BG_COUNT > 0) {
        JaamLed::createStrip(strip_bg,
                             JaamHardwarePins::BG_LED_PIN_JAAM,
                             BG_COUNT,
                             NEO_GRB + NEO_KHZ800);
        if (strip_bg) {
            strip_bg->setBrightness(255);
            strip_bg->clear();
            strip_bg->show();
        }
    }

    // 5. Service LED strip (skip for JAAM 1.3)
    if (SVC_COUNT > 0) {
        JaamLed::createStrip(strip_service,
                             JaamHardwarePins::SERVICE_LED_PIN_JAAM,
                             SVC_COUNT,
                             NEO_GRB + NEO_KHZ800);
        if (strip_service) {
            strip_service->setBrightness(255);
            strip_service->clear();
            strip_service->show();
        }
    }

    // 6. Sensors
    climate.begin();
    lightSensor.begin();

    // 7. Buttons
    buttons.setButton1Pin(BTN1_PIN, true);
    buttons.setButton1ClickListener(onBtn1Click);
    buttons.setButton1LongClickListener(onBtn1LongClick);

    if (BTN2_PIN > 0) {
        buttons.setButton2Pin(BTN2_PIN, true);
        buttons.setButton2ClickListener(onBtn2Click);
        buttons.setButton2LongClickListener(onBtn2LongClick);
    }

    if (BTN3_PIN > 0) {
        buttons.setButton3Pin(BTN3_PIN, true);
        buttons.setButton3ClickListener(onBtn3Click);
        buttons.setButton3LongClickListener(onBtn3LongClick);
    }

    // 8. Sound — play startup melody via buzzer
    sound.init(
        JaamHardwarePins::BUZZER_PIN_JAAM,
        JaamHardwarePins::DF_RX_PIN_JAAM,
        JaamHardwarePins::DF_TX_PIN_JAAM,
        50, 50, 50
    );
    sound.initBuzzer();
    sound.setBuzzerVolume(50);
    sound.playBuzzer(STARTUP_MELODY);

    display.printMessage("TEST MODE", "Ready");

    uint64_t efuseMac = ESP.getEfuseMac();
    char chipID[13];
    sprintf(chipID, "%04x%04x", (uint32_t)(efuseMac >> 32), (uint32_t)efuseMac);
    char deviceId[12];
    snprintf(deviceId, sizeof(deviceId), "JAAM%d-%04d", JAAM_VERSION, orderNumber);
    LOG.println("[TEST] Setup complete");
    LOG.printf("[TEST] Chip ID: %s\n", chipID);
    LOG.printf("[TEST] Device ID: %s\n", deviceId);
}

void loop() {
    if (currentPhase != PHASE_TEST) return;

    buttons.tick();
    updateSnake();
    updateBgBreathing();
    updateServiceSnake();

    // Clear button event label after 2 s, then refresh sensor display
    if (btnEventPending && millis() - btnEventTime > 2000) {
        btnEventPending = false;
        lastDisplayUpd  = 0;
    }

    // Refresh sensor readings on idle display every 2 s
    if (!btnEventPending && millis() - lastDisplayUpd > 2000) {
        updateIdleDisplay();
        lastDisplayUpd = millis();
    }
}


//     // АР Крим
//     {9999, "АР Крим", false, false},
    
//     // Вінницька область та її райони
//     {4, "Вінницька обл.", false, false},
//     {36, "Вінницький район", false, true},
//     {37, "Гайсинський район", false, true},
//     {35, "Жмеринський район", false, true},
//     {33, "Могилів-Подільський район", false, true},
//     {32, "Тульчинський район", false, true},
//     {34, "Хмільницький район", false, true},
//     {155, "м. Вінниця та Вінницька територіальна громада", true, true, true},
    
//     // Волинська область та її райони
//     {8, "Волинська обл.", false, false},
//     {38, "Володимирський район", false, true},
//     {41, "Камінь-Каширський район", false, true},
//     {40, "Ковельський район", false, true},
//     {39, "Луцький район", false, true},
//     {225, "м. Луцьк та Луцька територіальна громада", true, true, true},
    
    
//     // Дніпропетровська область та її райони
//     {9, "Дніпропетровська обл.", false, false},
//     {44, "Дніпровський район", false, true},
//     {42, "Кам'янський район", false, true},
//     {46, "Криворізький район", false, true},
//     {47, "Нікопольський район", false, true},
//     {43, "Самарівський район", false, true},
//     {45, "Павлоградський район", false, true},
//     {48, "Синельниківський район", false, true},
//     {332, "м. Дніпро та Дніпровська територіальна громада", true, true, true},
    
//     // Донецька область та її райони
//     {28, "Донецька обл.", false, false},
//     {54, "Бахмутський район", false, true},
//     {55, "Волноваський район", false, true},
//     {51, "Горлівський район", false, true},
//     {53, "Донецький район", false, true},
//     {49, "Кальміуський район", false, true},
//     {50, "Краматорський район", false, true},
//     {52, "Маріупольський район", false, true},
//     {56, "Покровський район", false, true},
    
//     // Житомирська область та її райони
//     {10, "Житомирська обл.", false, false},
//     {57, "Бердичівський район", false, true},
//     {59, "Житомирський район", false, true},
//     {60, "Звягельський район", false, true},
//     {58, "Коростенський район", false, true},
//     {442, "м. Житомир та Житомирська територіальна громада", true, true, true},
    
//     // Закарпатська область та її райони
//     {11, "Закарпатська обл.", false, false},
//     {61, "Берегівський район", false, true},
//     {62, "Хустський район", false, true},
//     {65, "Мукачівський район", false, true},
//     {63, "Рахівський район", false, true},
//     {64, "Тячівський район", false, true},
//     {66, "Ужгородський район", false, true},
//     {500, "м. Ужгород та Ужгородська територіальна громада", true, true, true},
    
//     // Запорізька область та її райони
//     {12, "Запорізька обл.", false, false},
//     {146, "Василівський район", false, true},
//     {147, "Бердянський район", false, true},
//     {149, "Запорізький район", false, true},
//     {148, "Мелітопольський район", false, true},
//     {145, "Пологівський район", false, true},
//     {564, "м. Запоріжжя та Запорізька територіальна громада", false, true},
    
//     // Івано-Франківська область та її райони
//     {13, "Ів.-Франківська обл.", false, false},
//     {67, "Верховинський район", false, true},
//     {68, "Івано-Франківський район", false, true},
//     {71, "Калуський район", false, true},
//     {70, "Коломийський район", false, true},
//     {69, "Косівський район", false, true},
//     {72, "Надвірнянський район", false, true},
//     {632, "м. Івано-Франківськ та Івано-Франківська територіальна громада", true, true, true},

//     // м. Київ 
//     {31, "м. Київ", false, false},
    
//     // Київська область та її райони
//     {14, "Київська обл.", false, false},
//     {73, "Білоцерківський район", false, true},
//     {78, "Бориспільський район", false, true},
//     {79, "Броварський район", false, true},
//     {75, "Бучанський район", false, true},
//     {74, "Вишгородський район", false, true},
//     {76, "Обухівський район", false, true},
//     {77, "Фастівський район", false, true},
  
//     // Кіровоградська область та її райони
//     {15, "Кіровоградська обл.", false, false},
//     {82, "Голованівський район", false, true},
//     {81, "Кропивницький район", false, true},
//     {83, "Новоукраїнський район", false, true},
//     {80, "Олександрійський район", false, true},
//     {761, "м. Кропивницький та Кропивницька територіальна громада", true, true, true},
    
//     // Луганська область та її райони
//     {16, "Луганська обл.", false, false},
//     {85, "Сватівський район", true, true, true},
//     {84, "Сєвєродонецький район", true, true, true},
//     {86, "Старобільський район", true, true, true},
//     {87, "Щастинський район", true, true, true},
    
//     // Львівська область та її райони
//     {27, "Львівська обл.", false, false},
//     {91, "Дрогобицький район", false, true},
//     {94, "Золочівський район", false, true},
//     {90, "Львівський район", false, true},
//     {88, "Самбірський район", false, true},
//     {89, "Стрийський район", false, true},
//     {92, "Шептицький район", false, true},
//     {93, "Яворівський район", false, true},
//     {845, "м. Львів та Львівська територіальна громада", true, true, true},
    
//     // Миколаївська область та її райони
//     {17, "Миколаївська обл.", false, false},
//     {96, "Баштанський район", false, true},
//     {95, "Вознесенський район", false, true},
//     {98, "Миколаївський район", false, true},
//     {97, "Первомайський район", false, true},
//     {926, "м. Миколаїв та Миколаївська територіальна громада", true, true, true},
    
//     // Одеська область та її райони
//     {18, "Одеська обл.", false, false},
//     {100, "Березівський район", false, true},
//     {102, "Білгород-Дністровський район", false, true},
//     {105, "Болградський район", false, true},
//     {101, "Ізмаїльський район", false, true},
//     {104, "Одеський район", false, true},
//     {99, "Подільський район", false, true},
//     {103, "Роздільнянський район", false, true},
//     {964, "м. Одеса та Одеська територіальна громада", true, true, true},
    
//     // Полтавська область та її райони
//     {19, "Полтавська обл.", false, false},
//     {107, "Кременчуцький район", false, true},
//     {106, "Лубенський район", false, true},
//     {108, "Миргородський район", false, true},
//     {109, "Полтавський район", false, true},
//     {1060, "м. Полтава та Полтавська територіальна громада", true, true, true},
    
//     // Рівненська область та її райони
//     {5, "Рівненська обл.", false, false},
//     {110, "Вараський район", false, true},
//     {111, "Дубенський район", false, true},
//     {112, "Рівненський район", false, true},
//     {113, "Сарненський район", false, true},
//     {1133, "м. Рівне та Рівненська територіальна громада", true, true, true},
    
//     // Сумська область та її райони
//     {20, "Сумська обл.", false, false},
//     {117, "Конотопський район", false, true},
//     {118, "Охтирський район", false, true},
//     {116, "Роменський район", false, true},
//     {114, "Сумський район", false, true},
//     {115, "Шосткинський район", false, true},
//     {1187, "м. Суми та Сумська територіальна громада", true, true, true},
    
//     // Тернопільська область та її райони
//     {21, "Тернопільська обл.", false, false},
//     {120, "Кременецький район", false, true},
//     {119, "Тернопільський район", false, true},
//     {121, "Чортківський район", false, true},
//     {1241, "м. Тернопіль та Тернопільська територіальна громада", true, true, true},
    
//     // Харківська область та її райони
//     {22, "Харківська обл.", false, false},
//     {126, "Богодухівський район", false, true},
//     {125, "Ізюмський район", false, true},
//     {127, "Берестинський район", false, true},
//     {123, "Куп'янський район", false, true},
//     {128, "Лозівський район", false, true},
//     {124, "Харківський район", false, true},
//     {122, "Чугуївський район", false, true},
//     {1293, "м. Харків та Харківська територіальна громада", false, true},
    
//     // Херсонська область та її райони
//     {23, "Херсонська обл.", false, false},
//     {129, "Бериславський район", false, true},
//     {133, "Генічеський район", false, true},
//     {131, "Каховський район", false, true},
//     {130, "Скадовський район", false, true},
//     {132, "Херсонський район", false, true},
//     {1370, "м. Херсон та Херсонська територіальна громада", true, true, true},
    
//     // Хмельницька область та її райони
//     {3, "Хмельницька обл.", false, false},
//     {135, "Кам'янець-Подільський район", false, true},
//     {134, "Хмельницький район", false, true},
//     {136, "Шепетівський район", false, true},
//     {1400, "м. Хмельницький та Хмельницька територіальна громада", true, true, true},
    
//     // Черкаська область та її райони
//     {24, "Черкаська обл.", false, false},
//     {150, "Звенигородський район", false, true},
//     {153, "Золотоніський район", false, true},
//     {151, "Уманський район", false, true},
//     {152, "Черкаський район", false, true},
//     {1473, "м. Черкаси та Черкаська територіальна громада", true, true, true},
    
//     // Чернівецька область та її райони
//     {26, "Чернівецька обл.", false, false},
//     {138, "Вижницький район", false, true},
//     {139, "Дністровський район", false, true},
//     {137, "Чернівецький район", false, true},
//     {1542, "м. Чернівці та Чернівецька територіальна громада", true, true, true},
    
//     // Чернігівська область та її райони
//     {25, "Чернігівська обл.", false, false},
//     {144, "Корюківський район", false, true},
//     {142, "Ніжинський район", false, true},
//     {141, "Новгород-Сіверський район", false, true},
//     {143, "Прилуцький район", false, true},
//     {140, "Чернігівський район", false, true},
//     {1591, "м. Чернігів та Чернігівська територіальна громада", true, true, true},
