#include <WiFi.h>
#include <HTTPUpdate.h>
#include <Preferences.h>

WiFiClientSecure client;

Preferences preferences;

const char* ssid = ""; // WIFI-мережа для оновлення прошивки
const char* password = ""; // Пароль до WIFI мережі для оновлення прошивки
const char* userSsid = ""; // WIFI-мережа замовника
const char* userPassword = ""; // Пароль до WIFI мережі замовника


// const char* firmwareUrl = "https://update.jaam.net.ua/5.0"; // production
const char* firmwareUrl = "https://update.jaam.net.ua/5.0-b11"; // beta

// ID регіонів нижче в коментарі
const int home_district = 31; // Київ (за замовчуванням)

void updateUserWifiCreds() {
  Serial.println("Disconnecting from WiFi...");
  bool disconnect = WiFi.disconnect(false, true);
  Serial.println(disconnect ? "Disconnected from WiFi" : "Disconnect from WiFi failed");
  if (userSsid != "" && userPassword != "") {
    WiFi.begin(userSsid, userPassword);
    WiFi.waitForConnectResult(10000);
    Serial.printf("Saved User WiFi creds. SSID: %s\n", userSsid);
  } else {
    Serial.println("No User WiFi creds, erased..."); }
}

void updateFirmware() {
  preferences.begin("storage", false);

  // clear all previous settings
  preferences.clear();

  // set ID and home district for the device
  preferences.putInt("hmd", home_district); // home district

#if JAAM_VERSION == 1

  // Default value for JAAM 1
  preferences.putString("id", "JAAM");
  preferences.putInt("legacy", 0); // JAAM 1

  Serial.println("Default JAAM 1 settings applied...");

#elif JAAM_VERSION == 2

  // Default value for JAAM 2
  preferences.putString("id", "JAAM2");
  preferences.putInt("legacy", 3); // JAAM 2.1
  
  Serial.println("Default JAAM 2 settings applied...");

#elif JAAM_VERSION == 3

  // Default value for JAAM 3
  preferences.putString("id", "JAAM3");
  preferences.putInt("legacy", 6); // JAAM 3.2
  preferences.putInt("bm", 1); // button mode (1 - map mode change)
  preferences.putInt("b2m", 2); // button mode (2 - display mode change)
  preferences.putInt("b3m", 5); // button mode (7 - lamp mode)
  preferences.putInt("bml", 0); // toggle display and map
  preferences.putInt("b2ml", 6); // toggle night brightness
  preferences.putInt("b3ml", 10); // reboot device
  preferences.putInt("dsmd", 2); // display SH1106
  preferences.putInt("dh", 64); // display SH1106
  preferences.putInt("sobc", 1); // sound of button click (0 - off, 1 - on)
  preferences.putInt("brightness", 100); // display brightness (0-100)
  preferences.putInt("brd", 100); // auto brightness for day (0-100)
  
  Serial.println("Default JAAM 3 settings applied...");
#endif

  preferences.end();

  if (firmwareUrl == "") {
    Serial.println("No firmware URL provided, skipping firmware update...");
    updateUserWifiCreds();
    Serial.println("Now you can flash the firmware manually!");
    return;
  }
  Serial.printf("Starting firmware update from URL: %s\n", firmwareUrl);
  client.setInsecure();
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  t_httpUpdate_return fwRet = httpUpdate.update(client, firmwareUrl);
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  uint64_t chipid = ESP.getEfuseMac();
  Serial.printf("Chip ID: %04x%04x\n", (uint32_t)(chipid >> 32), (uint32_t)chipid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf("Connecting to %s...\n", ssid);
    httpUpdate.rebootOnUpdate(false);
    httpUpdate.onStart([]() {
      Serial.println("Firmware update started!");
    });
    httpUpdate.onProgress([](int progress, int total) {
      if (total == 0) return;
      char progressText[5];
      Serial.printf("Progress: %d%%\n", progress / (total / 100));
    });
    httpUpdate.onEnd([]() {
      Serial.println("Firmware update successfully completed!");
      updateUserWifiCreds();
      delay(1000);
      ESP.restart();
    });
    httpUpdate.onError([](int error) {
      Serial.printf("Firmware update error occurred. Error (%d): %s\n", error, httpUpdate.getLastErrorString().c_str());
    });
  }
  Serial.println("Connected to WiFi");

  updateFirmware();
}

void loop() {
  // Do nothing
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
