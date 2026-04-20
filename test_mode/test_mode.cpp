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
    if (strlen(userSsid) == 0) return;
    WiFi.disconnect(false, true);
    WiFi.begin(userSsid, userPassword);
    WiFi.waitForConnectResult(10000);
    LOG.printf("[UPDATE] User WiFi saved: %s\n", userSsid);
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
        30, 30, 30
    );
    sound.initBuzzer();
    sound.setBuzzerVolume(30);
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
