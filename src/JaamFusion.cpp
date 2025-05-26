#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>

#include <WiFiManager.h>
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

using namespace websockets;

char                chipID[13];
char                currentFwVersion[25];
int                 currentIdx = 0;
uint32_t            loopCount = 0;
int                 needRebootWithDelay = -1;

Async               async = Async(20);

JaamSettings        settings;
Firmware            firmware;
JaamWeb             web;
JaamLed             led;

// --- LED Configuration ---
Adafruit_NeoPixel*  strip_main = nullptr;
Adafruit_NeoPixel*  strip_bg = nullptr;
Adafruit_NeoPixel*  strip_service = nullptr;
SemaphoreHandle_t   stripMutex = nullptr;
uint16_t            num_leds_main = 26;
uint16_t            num_leds_service = 5;
uint8_t             homeDistrict = 25;
std::vector<int>    allLedsMain;
std::vector<int>    allLedsBg;

bool                strip_main_initialized = false;
bool                strip_bg_initialized = false;
bool                strip_service_initialized = false;

AnimationManager    animation;
AnimationParams::Type animType;

// --- WIFI Configuration ---
WiFiManager         wm;
WebsocketsClient    websocket;
uint32_t            lastWifiConnectTime = 0;  // Track when WiFi was last connected

time_t              websocketLastPingTime = 0;
bool                isFirstDataFetchCompleted = false;
bool                apiConnected;
uint8_t             legacy = 4;
bool                websocketReconnect = false;
uint32_t            lastWebsocketConnectTime = 0;


std::map<uint16_t, uint16_t>    alertsMap;
std::map<uint16_t, bool>        airAlertsMap;
std::map<uint16_t, bool>        artilleryAlertsMap;
std::map<uint16_t, bool>        urbanFightsAlertsMap;
std::map<uint16_t, bool>        chemicalAlertsMap;
std::map<uint16_t, bool>        nuclearAlertsMap;
std::map<uint16_t, bool>        missilesAlertsMap;
std::map<uint16_t, bool>        kabAlertsMap;
std::map<uint16_t, bool>        dronesAlertsMap;
std::map<uint16_t, bool>        ballisticAlertsMap;


void clearAllAlertsMaps() {
    alertsMap.clear();
    airAlertsMap.clear();
    artilleryAlertsMap.clear();
    urbanFightsAlertsMap.clear();
    chemicalAlertsMap.clear();
    nuclearAlertsMap.clear();
    missilesAlertsMap.clear();
    kabAlertsMap.clear();
    dronesAlertsMap.clear();
    ballisticAlertsMap.clear();
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
void printHex(const String& data) {
    LOG.print("HEX: ");
    for (size_t i = 0; i < data.length(); ++i) {
        LOG.printf("%02X ", static_cast<uint8_t>(data[i]));
    }
    LOG.println();
}

void onMessageCallback(WebsocketsMessage msg) {
    LOG.print("Got Message: ");

    

    // Ігноруємо текстові повідомлення
    if (!msg.isBinary()) {
        LOG.println("message in not binary");
        LOG.println(msg.data());
        JsonDocument data = parseJson(msg.data().c_str());
        String payload = data["payload"];
        if (!payload.isEmpty()) {
            if (payload == "ping") {
                LOG.println("Heartbeat from server");
            }
        }
        return;
    } else {
        LOG.printf("len: %d processing\n", msg.length());
    }
    websocketLastPingTime = millis();

    WSString payload = msg.rawData();
    const uint8_t* data = reinterpret_cast<const uint8_t*>(payload.data());
    size_t len = payload.size();

    // 3) Мінімальна довжина — хоча б 1B на type
    if (len < HEADER_SZ) {
        LOG.printf("len < HEADER_SZ\n");
        return;
    }

    // 4) Перевіряємо тип пакета
    uint8_t type = data[0];
    if (type != TYPE_ALERTS_BATCH) {
        LOG.printf("type != TYPE_ALERTS_BATCH\n");
        return;
    }

    // 5) Обчислюємо довжину payload після заголовка
    size_t bodyLen = len - HEADER_SZ;

    // 6) payloadLen має ділитися на RECORD_SZ
    if (bodyLen == 0 || (bodyLen % RECORD_SZ) != 0) {
        // некоректний фрейм — пропускаємо
        LOG.printf("bodyLen == 0 || (bodyLen % RECORD_SZ\n");
        return;
    }

    // 7) Обчислюємо кількість записів
    size_t count = bodyLen / RECORD_SZ;
    //LOG.printf("count %d\n", count);

    // 8) Розбираємо count записів по RECORD_SZ
    const uint8_t* ptr = data + HEADER_SZ;
    uint32_t color;
    uint32_t period;
    uint8_t cycles;
    uint8_t startBrightness = 50;
    uint8_t endBrightness = 255;
    uint8_t ledCount;
    std::vector<int> ledsIdx;
    bool air, artillery, urban, chemical, nuclear, missiles, kab, drones, ballistic;
    for (size_t i = 0; i < count; ++i) {
        uint16_t region_id = uint16_t(ptr[0]) | (uint16_t(ptr[1]) << 8);
        uint16_t flags16   = uint16_t(ptr[2]) | (uint16_t(ptr[3]) << 8);
        ptr += RECORD_SZ;

        bool animate = false;

        bool airStarted = false;
        bool airCompleted = false;
        bool dronesStarted = false;
        bool dronesCompleted = false;
        bool missilesStarted = false;
        bool missilesCompleted = false;
        bool kabStarted = false;
        bool kabCompleted = false;
        

        // Зберігаємо
        alertsMap[region_id] = flags16;
        // Розкладаємо по окремих тривогах
        air         = flags16 & (1 << 0);
        artillery   = flags16 & (1 << 1);
        urban       = flags16 & (1 << 2);
        chemical    = flags16 & (1 << 3);
        nuclear     = flags16 & (1 << 4);
        drones      = flags16 & (1 << 5);
        missiles    = flags16 & (1 << 6);
        kab         = flags16 & (1 << 7);
        ballistic   = flags16 & (1 << 8);

        if (!airAlertsMap[region_id]    &&   air) { airStarted = true; }
        if (airAlertsMap[region_id]    &&   !air) { airCompleted = true; }
        if (!dronesAlertsMap[region_id]    &&   drones) { dronesStarted = true; }
        if (dronesAlertsMap[region_id]    &&   !drones) { dronesCompleted = true; }
        if (!missilesAlertsMap[region_id]    &&   missiles) { missilesStarted = true; }
        if (missilesAlertsMap[region_id]    &&   !missiles) { missilesCompleted = true; }
        if (!kabAlertsMap[region_id]    &&   kab) { kabStarted = true; }
        if (kabAlertsMap[region_id]    &&   !kab) { kabCompleted = true; }

        // LOG.printf("Region %d airAlertsMap %d air %d aS %d aC %d dS %d dC %d mS %d mC %d kS %d kC %d\n", 
        //     region_id, airAlertsMap[region_id], air, 
        //     airStarted, airCompleted,
        //     dronesStarted, dronesCompleted,
        //     missilesStarted, missilesCompleted, 
        //     kabStarted,kabCompleted);

        // Розкладаємо по окремих тривогах
        airAlertsMap[region_id]                   = air;
        artilleryAlertsMap[region_id]             = artillery;
        urbanFightsAlertsMap[region_id]           = urban;
        chemicalAlertsMap[region_id]              = chemical;
        nuclearAlertsMap[region_id]               = nuclear;
        missilesAlertsMap[region_id]              = missiles;
        kabAlertsMap[region_id]                   = kab;
        dronesAlertsMap[region_id]                = drones;
        ballisticAlertsMap[region_id]             = ballistic;

        const int* leds = getLedsForRegion(region_id, ledCount);
        if (leds == nullptr) {
            // Якщо такого регіону немає — пропускаємо цей запис
            continue;
        }
        if (isFirstDataFetchCompleted) {
            if (airCompleted || dronesCompleted || missilesCompleted || kabCompleted) {
                animate = true;
                color = animation.ledActualColor(strip_main, leds[0]);
                period = 1000;
                
                if (airCompleted) {
                    cycles = 1;
                    startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_ALERT));
                    endBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_CLEAR));
                    animType = AnimationParams::Type::ONE_WAY_BLEND;
                }
                if (dronesCompleted || missilesCompleted || kabCompleted) {
                    cycles = 1;
                    period = 3000;
                    startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_EXPLOSION));
                    endBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_ALERT));
                    animType = AnimationParams::Type::ONE_WAY_BLEND;
                }
                
            }
            if (airStarted) {
                animate = true;
                color = strip_main->Color(255, 128, 0); //animation.ledActualColor(strip_main, leds[0], false);
                period = 1000;
                cycles = 5;
                endBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_NEW_ALERT));
                animType = AnimationParams::Type::FADE;
            }
            if (dronesStarted || kabStarted || missilesStarted) {
                animate = true;
                color = animation.ledActualColor(strip_main, leds[0], false);
                period = 1000;
                cycles = 5;
                startBrightness = led.brightnessAbsolute(settings.getInt(BRIGHTNESS_EXPLOSION));
                endBrightness = 50;
                animType = AnimationParams::Type::PULSE;
            }
            
            if(animate) {
                for (int i = 0; i < ledCount; ++i) {
                    int ledsIdx[1] = { leds[i] };
                    if (!animation.createAnimation(
                        animType,
                        strip_main,
                        ledsIdx,
                        1,
                        color,
                        period,
                        cycles,
                        startBrightness,
                        endBrightness,
                        region_id
                    )) {
                        LOG.println("ERROR: Не вдалося створити анімацію");
                        return;
                    }
                }
            }
        } else {
            animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
                for(uint16_t i = 0; i < num_leds_main; i++) {
                    uint32_t color = animation.ledActualColor(strip, i);
                    strip->setPixelColor(i, color);
                }
                strip->show();
            });

        }
    }

    isFirstDataFetchCompleted = true;

    // Викликаємо createAnimation один раз для всіх активних region_id
    // if (!ledsIdx.empty()) {
    //     if (!animation.createAnimation(
    //         animType,
    //         strip_main,
    //         ledsIdx.data(),
    //         ledsIdx.size(),
    //         color,
    //         period,
    //         cycles,
    //         startBrightness,
    //         endBrightness,
    //         1
    //     )) {
    //         LOG.println("ERROR: Не вдалося створити анімацію");
    //         return;
    //     }
    // }
}

void logWebsocketCloseReason(websockets::CloseReason reason) {
    switch (reason) {
        case websockets::CloseReason_None:
            LOG.println("WebSocket CloseReason: None (-1)");
            break;
        case websockets::CloseReason_NormalClosure:
            LOG.println("WebSocket CloseReason: Normal Closure (1000)");
            break;
        case websockets::CloseReason_GoingAway:
            LOG.println("WebSocket CloseReason: Going Away (1001)");
            break;
        case websockets::CloseReason_ProtocolError:
            LOG.println("WebSocket CloseReason: Protocol Error (1002)");
            break;
        case websockets::CloseReason_UnsupportedData:
            LOG.println("WebSocket CloseReason: Unsupported Data (1003)");
            break;
        case websockets::CloseReason_NoStatusRcvd:
            LOG.println("WebSocket CloseReason: No Status Received (1005)");
            break;
        case websockets::CloseReason_AbnormalClosure:
            LOG.println("WebSocket CloseReason: Abnormal Closure (1006)");
            break;
        case websockets::CloseReason_InvalidPayloadData:
            LOG.println("WebSocket CloseReason: Invalid Payload Data (1007)");
            break;
        case websockets::CloseReason_PolicyViolation:
            LOG.println("WebSocket CloseReason: Policy Violation (1008)");
            break;
        case websockets::CloseReason_MessageTooBig:
            LOG.println("WebSocket CloseReason: Message Too Big (1009)");
            break;
        case websockets::CloseReason_InternalServerError:
            LOG.println("WebSocket CloseReason: Internal Server Error (1011)");
            break;
        default:
            LOG.printf("WebSocket CloseReason: Unknown (%d)\n", static_cast<int>(reason));
            break;
    }
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
    isFirstDataFetchCompleted = false;
    LOG.printf("Heap before close: %u\n", ESP.getFreeHeap());
    //websocket.close();
    auto reason = websocket.getCloseReason();
    logWebsocketCloseReason(reason);
    delay(500);
    LOG.printf("Heap after close: %u\n", ESP.getFreeHeap());
    //servicePin(DATA, LOW, false);
    //ha.setMapApiConnect(apiConnected);
  } else if (event == WebsocketsEvent::GotPing) {
    LOG.printf("websocket ping, payload: [%s], len: %d\n", data.c_str(), data.length());
    printHex(data);
    websocketLastPingTime = millis();
  } else if (event == WebsocketsEvent::GotPong) {
    LOG.printf("websocket pong, payload: [%s], len: %d\n", data.c_str(), data.length());
    printHex(data);
  }
}

void socketConnect() {
  LOG.println("connection start...");
  //showServiceMessage("підключення...", "Сервер даних");
  
  websocket.onMessage(onMessageCallback);
  websocket.onEvent(onEventsCallback);
  long startTime = millis();
  char webSocketUrl[100];
  sprintf(
    webSocketUrl,
    "ws://%s:%d/data_fusion_v1",
    "10.2.0.156",
    settings.getInt(WS_SERVER_PORT)
  );
  LOG.println(webSocketUrl);
  websocket.connect(webSocketUrl);
  if (websocket.available()) {
    clearAllAlertsMaps();
    animation.clearAllAnimations();
    animation.paintStripDefault(strip_main, num_leds_main);
    LOG.print("connection time - ");
    LOG.print(millis() - startTime);
    LOG.println("ms");
    char chipIdInfo[25];
    sprintf(chipIdInfo, "chip_id:%s", chipID);
    LOG.println(chipIdInfo);
    websocket.send(chipIdInfo);
    char firmwareInfo[100];
    sprintf(firmwareInfo, "firmware:%s_%s", currentFwVersion, settings.getString(ID));
    LOG.println(firmwareInfo);
    websocket.send(firmwareInfo);

    char userInfo[250];
    JsonDocument userInfoJson;
    userInfoJson["legacy"] = legacy;
    sprintf(userInfo, "user_info:%s", userInfoJson.as<String>().c_str());
    LOG.println(userInfo);
    websocket.send(userInfo);
    websocket.ping("A");
    websocketReconnect = false;
    lastWebsocketConnectTime  = millis();
    //showServiceMessage("підключено!", "Сервер даних", 3000);
  } else {
    //showServiceMessage("недоступний", "Сервер даних", 3000);
  }
}

void websocketProcess() {
  if (millis() - websocketLastPingTime > settings.getInt(WS_ALERT_TIME)) {
    LOG.println("websocketReconnect = true; Причина: не було ping/pong від сервера (WS_ALERT_TIME)");
    websocketReconnect = true;
  }
  if (millis() - websocketLastPingTime > settings.getInt(WS_REBOOT_TIME)) {
    LOG.println("websocketReconnect = true; Причина: перевищено WS_REBOOT_TIME, буде перезавантаження");
    rebootDevice(3000, true);
  }
  if (!websocket.available()) {
    LOG.println("Reconnecting... websocket.available() == false");
    socketConnect();
  }
  if (websocketReconnect) {
    LOG.println("Reconnecting... websocketReconnect == true");
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
    uint32_t period = 1000; //random(AnimationConfig::MIN_PERIOD, AnimationConfig::MAX_PERIOD + 1);
    uint8_t cycles = 12; // random(AnimationConfig::MIN_CYCLES, AnimationConfig::MAX_CYCLES + 1);
    uint8_t startBrightness = 50; // random(AnimationConfig::MIN_START_BRIGHTNESS, AnimationConfig::MAX_START_BRIGHTNESS + 1);
    uint8_t endBrightness = 255; //   random(AnimationConfig::MIN_END_BRIGHTNESS, AnimationConfig::MAX_END_BRIGHTNESS + 1);
    
    // Створення анімації з обробкою помилок
    // if (!animation.createAnimation(
    //     animType,
    //     strip,
    //     ledsIdx,
    //     1,
    //     color,
    //     period,
    //     cycles,
    //     startBrightness,
    //     endBrightness
    // )) {
    //     LOG.println("ERROR: Не вдалося створити анімацію");
    //     return;
    // }

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
  uint32_t websocketUptime = websocket.available() ? (millis() - lastWebsocketConnectTime) / 60000 : 0; // in seconds
  String wifiStatus = wifiConnected ? "Connected" : "Disconnected";
  String websocketStatus = websocket.available() ? "Connected" : "Disconnected";
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
    delay(1000);
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
        animation.createAnimation(
            AnimationParams::Type::RUNNING_LIGHT, 
            strip_main, 
            allLedsMain.data(), 
            num_leds_main,
            strip_main->Color(255, 128, 0),
            2000,
            90,
            50,
            255
        );
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
//   if (strip_main_initialized) {
//     animation.safeStripOperation(strip_main, [](Adafruit_NeoPixel* strip) {
//       uint32_t color = strip->getPixelColor(currentIdx);
//       uint8_t r = (color >> 16) & 0xFF;
//       uint8_t g = (color >> 8)  & 0xFF;
//       uint8_t b =  color        & 0xFF;
//       LOG.printf("Pixel %d color: R=%d, G=%d, B=%d (0x%06X)\n", 
//                 currentIdx, r, g, b, color);
//     });
//   }

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
    initStrip();
    initWifi();

    // Ініціалізуємо генератор випадкових чисел
    randomSeed(esp_random());
    
    // Передаємо settings в AnimationManager
    animation.setSettings(&settings);
    led.setSettings(&settings);
    // Налаштовуємо асинхронні задачі
    //async.setInterval(animations, ANIMATION_INTERVAL);
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
    websocket.poll();
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