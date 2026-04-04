# Збірка проєкту

Як зібрати прошивку JAAM Fusion з вихідного коду на **Windows / macOS / Linux**.

Проєкт збирається через **PlatformIO** (Arduino framework). Середовища збірки (env) описані у файлі `platformio.ini`.

## Який env обрати

У репозиторії є кілька середовищ збірки:

- `firmware` — базовий таргет (ESP32 / `esp32dev`)
- `firmware_esp32s3` — таргет під ESP32‑S3 (`esp32-s3-devkitc-1`)
- `firmware_esp32c3` — таргет під ESP32‑C3 (`esp32-c3-devkitc-02`)

Є також допоміжні env’и:

- `telnet` — збірка з Telnet‑логами (якщо увімкнено)
- `test_animation`, `test_min_of_silence` — тестові збірки
- `updater` — окремий модуль оновлювача

## Варіант 1 (рекомендовано): VS Code + PlatformIO

Це найпростіший шлях, особливо якщо ви хочете одразу мати автодоповнення, навігацію по коду і інтегрований upload/monitor.

1. Встановіть **Visual Studio Code**.
2. Встановіть розширення **PlatformIO IDE**.
3. Відкрийте репозиторій у VS Code.
4. Дочекайтесь, поки PlatformIO підтягне toolchain та залежності.
5. Зберіть прошивку:
   - PlatformIO sidebar → **Build**
   - або через команду `PlatformIO: Build`

## Варіант 2: PlatformIO Core (CLI)

Це зручно для CI/скриптів або якщо ви не користуєтесь VS Code.

Після встановлення PlatformIO Core у терміналі будуть доступні команди `pio ...`.

### Збірка

```bash
pio run -e firmware
```

Для інших чипів:

```bash
pio run -e firmware_esp32s3
pio run -e firmware_esp32c3
```

Результат збірки зʼявиться в `.pio/build/<env>/`.

### Прошивка (upload)

Підʼєднайте плату по USB і виконайте:

```bash
pio run -e firmware -t upload
```

Якщо порт не визначився автоматично, вкажіть його явно:

```bash
pio run -e firmware -t upload --upload-port /dev/ttyUSB0
```

На Windows це зазвичай `COM3`, `COM4`, …

### Serial Monitor

```bash
pio device monitor -e firmware
```

Швидкість порту в проєкті: `115200`.

## Інструкції за ОС

Нижче — практичні кроки з урахуванням типових проблем (драйвери/права/usb‑порти).

### Windows

1. Встановіть:

   - Git (Git for Windows)
   - VS Code
   - PlatformIO IDE extension
2. Підʼєднайте плату по USB.
3. Якщо плата не зʼявляється як COM‑порт — встановіть драйвер під USB‑UART вашої плати (часто це **CP210x** або **CH340**).
4. Збірка/прошивка:

   - VS Code → PlatformIO → Build / Upload
   - або CLI:

   ```bat
   pio run -e firmware
   pio run -e firmware -t upload
   pio device monitor -e firmware
   ```

### macOS

1. Встановіть Xcode Command Line Tools:

   ```bash
   xcode-select --install
   ```

2. Далі — або VS Code + PlatformIO (рекомендовано), або CLI.
3. Порт зазвичай виглядає як `/dev/cu.usbserial-...` або `/dev/cu.usbmodem-...`.

Приклад upload із явним портом:

```bash
pio run -e firmware -t upload --upload-port /dev/cu.usbserial-XXXX
```

### Linux

1. Встановіть VS Code + PlatformIO або PlatformIO Core.
2. Перевірте, що користувач має доступ до serial‑пристроїв:
   - додайте користувача в групу `dialout` (або відповідну у вашій дистрибуції)
   - перепідʼєднайтеся до сесії (logout/login)
3. Порт зазвичай `/dev/ttyUSB0` або `/dev/ttyACM0`.

Приклад:

```bash
pio run -e firmware
pio run -e firmware -t upload --upload-port /dev/ttyUSB0
pio device monitor -e firmware
```

## Часті проблеми

### PlatformIO не бачить порт

- Перевірте кабель (часто проблема в “charge‑only” кабелі).
- Перевірте драйвер USB‑UART (Windows).
- На Linux перевірте права доступу до `/dev/tty*`.

### Зібрав не той бінарник (не той чип)

ESP32 / ESP32‑S3 / ESP32‑C3 мають різні env’и. Переконайтесь, що ви збираєте саме:

- `firmware` для ESP32
- `firmware_esp32s3` для ESP32‑S3
- `firmware_esp32c3` для ESP32‑C3

### Де подивитись точні параметри збірки

Джерело істини — `platformio.ini` у корені репозиторію.

Також корисно: [Web Assets](web-assets.md) (як у проєкті збираються/стискаються файли веб‑інтерфейсу).
