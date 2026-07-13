#include "JaamSound.h"
#include "JaamLogs.h"

#if DFPLAYER_ENABLED
// MINI: скільки часу тримати dfMiniPlaying=true без підтвердження, перш ніж примусово скинути
// (запобіжник на випадок, якщо і DFPlayerPlayFinished, і readState() мовчать) - з запасом більше за будь-який трек
static constexpr unsigned long DF_MINI_PLAY_TIMEOUT_MS = 3UL * 60UL * 1000UL;
// MINI: readState() - блокуючий запит-відповідь по UART, тому не питаємо частіше цього інтервалу
static constexpr unsigned long DF_MINI_STATE_CHECK_INTERVAL_MS = 2000;
// Верхня межа кількості файлів, якій довіряємо з getTotalFile()/readFileCounts() - захист від
// сміттєвого значення при глюку UART/пошкодженій таблиці SD-карти (звідти й будується tracks-dropdown у web UI)
static constexpr int DF_MAX_TRUSTED_FILES_COUNT = 50;
#endif

#if BUZZER_ENABLED || DFPLAYER_ENABLED
void JaamSound::init(int bPin, int rxPin, int txPin, int volCurrent, int volDay, int volNight) {
    volumeCurrent = volCurrent;
    volumeDay = volDay;
    volumeNight = volNight;
    buzzerPin = bPin;
    dfRxPin = rxPin;
    dfTxPin = txPin;
    LOG.printf("[SOUND] pins set: buzzerPin %d, rx %d, tx %d\n", bPin, rxPin, txPin);
}
#endif

void JaamSound::setVolumeCurrent(int volume) {
    volumeCurrent = volume;
}
void JaamSound::setVolumeDay(int volume) {
    volumeDay = volume;
}
void JaamSound::setVolumeNight(int volume) {
    volumeNight = volume;
}
void JaamSound::setSoundSource(int source) {
    soundSource = source;
}
void JaamSound::setBeepHour(int hour) {
    beepHour = hour;
}

#if BUZZER_ENABLED
void JaamSound::initBuzzer() {
    LOG.printf("[SOUND] Init Buzzer\n");
    if (!isBuzzerEnabled()) {
        LOG.printf("[SOUND] Buzzer pin is not set, skip init\n");
        return;
    }
    if (player) delete player;
    player = new MelodyPlayer(buzzerPin, 0, LOW);
    player->setVolume(expMap(volumeCurrent, 0, 100, 0, 255));
    LOG.printf("[SOUND] Set initial volume to: %d\n", volumeCurrent);

}

void JaamSound::playBuzzer(const char* melodyRtttl) {
    if (player == nullptr) {
        LOG.printf("[SOUND] Buzzer not initialised, cannot play melody\n");
        return;
    }
    Melody melody = MelodyFactory.loadRtttlString(melodyRtttl);
    player->playAsync(melody);
}

void JaamSound::setBuzzerVolume(int volume) {
    if (player == nullptr) {
        LOG.printf("[SOUND] Buzzer not initialised, cannot set volume\n");
        return;
    }
    player->setVolume(expMap(volume, 0, 100, 0, 255));
    LOG.printf("[SOUND] Set buzzer volume to: %d\n", volume);
}
#endif

bool JaamSound::isBuzzerEnabled() {
#if BUZZER_ENABLED
    if (buzzerPin > 0) {
        return true;
    }
#endif
    return false;
}

bool JaamSound::isBuzzerPlaying() {
#if BUZZER_ENABLED
    if (!isBuzzerEnabled()) {
        LOG.printf("[SOUND] Buzzer not enabled, cannot check if playing\n");
        return false;
    }
    if (player == nullptr) {
        LOG.printf("[SOUND] Buzzer not initialised, cannot check if playing\n");
        return false;
    }
    return player->isPlaying();
#else
    return false;
#endif
}

#if DFPLAYER_ENABLED
void JaamSound::setDFMaxVolume(int maxVolume) {
    dfPlayerMaxVolume = maxVolume;
}

bool JaamSound::retryDFBegin(std::function<bool()> begin, const char* name) {
    const int8_t attempts = 5;
    for (int8_t count = 1; !begin(); count++) {
        LOG.printf("[SOUND] Attempt #%d of %d\n", count, attempts);
        LOG.printf("[SOUND] %s not found...\n", name);
        delay(1000);
        if (count >= attempts) {
            LOG.printf("[SOUND] %s init failed: max attempts reached\n", name);
            return false;
        }
    }
    return true;
}

void JaamSound::resetDFPlayerState() {
    dfInitAttempted = false;
    dfBackend = DFBackend::NONE;
    dfMiniPlaying = false;
    dfTotalFiles = 0;
}

void JaamSound::initDFPlayer(int backend) {
    if (!isDFPlayerEnabled()) {
        LOG.printf("[SOUND] DFPlayer pins not set, skip init\n");
        return;
    }
    resetDFPlayerState();
    dfInitAttempted = true;
    dfSerial.end();
    delay(50);
    LOG.printf("[SOUND] rx, tx: %d, %d\n", dfRxPin, dfTxPin);

    if (backend == DFBackend::PRO) {
        LOG.printf("[SOUND] Init DFPlayer PRO\n");
        dfSerial.begin(115200, SERIAL_8N1, dfRxPin, dfTxPin); // RX, TX
        delay(500); // дати модулю час прокинутися після (ре)ініціалізації UART

        // Turn off the start prompt
        dfSerial.print("AT+PROMPT=OFF\r\n");
        delay(200);
        while (dfSerial.available()) {
            LOG.printf("%c", dfSerial.read());
        }

        if (!retryDFBegin([this]{ return dfplayerPro.begin(dfSerial); }, "DFPlayer PRO")) {
            return;
        }
        LOG.printf("[SOUND] DFPlayer PRO RX OK!\n");
        dfplayerPro.setVol(2);
        delay(500);
        if (dfplayerPro.getVol() != 2) {
            LOG.printf("[SOUND] DFPlayer PRO TX Fail!\n");
            return;
        }
        LOG.printf("[SOUND] DFPlayer PRO TX OK!\n");
        dfBackend = DFBackend::PRO;

        dfplayerPro.setVol(0);
        dfplayerPro.switchFunction(dfplayerPro.MUSIC);
        dfplayerPro.setVol(expMap(volumeCurrent, 0, 100, 0, dfPlayerMaxVolume));
        dfplayerPro.setPlayMode(dfplayerPro.SINGLE);
        delay(500);
        dfplayerPro.setLED(false);
    } else if (backend == DFBackend::MINI) {
        LOG.printf("[SOUND] Init DFPlayer Mini\n");
        dfSerial.begin(9600, SERIAL_8N1, dfRxPin, dfTxPin); // RX, TX
        delay(500); // дати модулю час прокинутися після (ре)ініціалізації UART

        if (!retryDFBegin([this]{ return dfplayerMini.begin(dfSerial, /*isACK=*/true, /*doReset=*/true); }, "DFPlayer Mini")) {
            return;
        }
        LOG.printf("[SOUND] DFPlayer Mini ready!\n");
        dfBackend = DFBackend::MINI;

        dfplayerMini.volume(expMap(volumeCurrent, 0, 100, 0, dfPlayerMaxVolume));
        delay(200);
    } else {
        LOG.printf("[SOUND] Unknown DFPlayer backend: %d\n", backend);
        return;
    }

    // ponytail: file names are not parsed, tracks are addressed purely by number (1..dfTotalFiles)
    dfTotalFiles = getDFPlayerFilesCount();
    if (dfTotalFiles <= 0) {
        LOG.printf("[SOUND] DFPlayer has no playable files\n");
    } else if (dfTotalFiles > DF_MAX_TRUSTED_FILES_COUNT) {
        // Захист від сміттєвого значення з getTotalFile()/readFileCounts() (глюк UART, пошкоджена
        // таблиця SD) - інакше tracks-dropdown у web UI намагається побудувати JsonArray на це число
        LOG.printf("[SOUND] DFPlayer files count %d exceeds trusted limit %d, clamping\n", dfTotalFiles, DF_MAX_TRUSTED_FILES_COUNT);
        dfTotalFiles = DF_MAX_TRUSTED_FILES_COUNT;
    }
    LOG.printf("[SOUND] DFPlayer files found: %d\n", dfTotalFiles);
}

void JaamSound::playDFPlayer(int trackNumber) {
    if (!isDFPlayerConnected()) {
        LOG.printf("[SOUND] DFPlayer not connected, cannot play track\n");
        return;
    }
    if (trackNumber < 1 || trackNumber > dfTotalFiles) {
        LOG.printf("[SOUND] Invalid track number: %d (available: 1..%d), not playing\n", trackNumber, dfTotalFiles);
        return;
    }
    if (dfBackend == DFBackend::PRO) {
        // playFileNum адресує по порядку копіювання на диск - та сама семантика, що й dfplayerMini.play(n),
        // щоб одне і те саме TRACK_ON_* число грало однаковий трек незалежно від backend
        dfplayerPro.playFileNum(trackNumber);
        LOG.printf("[SOUND] Track played: #%d (%s)\n", trackNumber, dfplayerPro.getFileName());
    } else if (dfBackend == DFBackend::MINI) {
        dfplayerMini.play(trackNumber);
        dfMiniPlaying = true;
        dfMiniPlayStartedAt = millis();
        // не 0 - інакше перший isDFPlayerPlaying() відразу після play() вже пройде поріг
        // DF_MINI_STATE_CHECK_INTERVAL_MS і зробить блокуючий readState() до того, як модуль
        // встигне повідомити "грає"; передчасний stopped-стан скине dfMiniPlaying занадто рано
        dfMiniLastStateCheckAt = dfMiniPlayStartedAt;
        LOG.printf("[SOUND] Track played: #%d\n", trackNumber);
    }
}

void JaamSound::setDFPlayerVolume(int volume) {
    if (!isDFPlayerConnected()) {
        LOG.printf("[SOUND] DFPlayer not connected, cannot set volume\n");
        return;
    }
    int mapped = expMap(volume, 0, 100, 0, dfPlayerMaxVolume);
    if (dfBackend == DFBackend::PRO) {
        dfplayerPro.setVol(mapped);
    } else if (dfBackend == DFBackend::MINI) {
        dfplayerMini.volume(mapped);
    }
    LOG.printf("[SOUND] Set DFPlayer volume to: %d\n", volume);
}

int JaamSound::getDFPlayerFilesCount() {
    if (!isDFPlayerConnected()) {
        LOG.printf("[SOUND] DFPlayer not connected, cannot get files count\n");
        return 0;
    }
    int filesCount = 0;
    if (dfBackend == DFBackend::PRO) {
        filesCount = dfplayerPro.getTotalFile();
    } else if (dfBackend == DFBackend::MINI) {
        filesCount = dfplayerMini.readFileCounts();
    }
    LOG.printf("[SOUND] DFPlayer files count: %d\n", filesCount);
    return filesCount;
}

#endif

bool JaamSound::isDFPlayerEnabled() {
    return dfRxPin > -1 && dfTxPin > -1;
}

bool JaamSound::isDFPlayerPlaying() {
#if DFPLAYER_ENABLED
    if (!isDFPlayerConnected()) {
        LOG.printf("[SOUND] DFPlayer not connected, cannot check if playing\n");
        return false;
    }
    if (dfBackend == DFBackend::PRO) {
        return dfplayerPro.isPlaying();
    }
    if (dfBackend == DFBackend::MINI) {
        // ponytail: no BUSY pin wired, relies on the module's unsolicited "play finished" notification;
        // a missed/garbled UART byte can leave this stuck true until the next play() call.
        if (dfplayerMini.available() && dfplayerMini.readType() == DFPlayerPlayFinished) {
            dfMiniPlaying = false;
        }
        if (dfMiniPlaying) {
            unsigned long now = millis();
            // fallback #1: якщо повідомлення DFPlayerPlayFinished втрачено, періодично питаємо стан
            // напряму командою readState() (0x42): 0 - зупинено, 1 - грає, 2 - на паузі.
            // Запит блокуючий (чекає відповідь по UART), тож троттлимо виклики
            if (now - dfMiniLastStateCheckAt >= DF_MINI_STATE_CHECK_INTERVAL_MS) {
                dfMiniLastStateCheckAt = now;
                int state = dfplayerMini.readState();
                if (state != -1 && state != 1) {
                    LOG.printf("[SOUND] DFPlayer Mini readState() fallback: state %d (not busy), clearing stuck playing flag\n", state);
                    dfMiniPlaying = false;
                }
            }
            // fallback #2: якщо навіть readState() не відповідає (обрив/шум на UART), не тримати
            // стан "грає" вічно - обмежуємо його жорстким тайм-аутом
            if (dfMiniPlaying && now - dfMiniPlayStartedAt >= DF_MINI_PLAY_TIMEOUT_MS) {
                LOG.printf("[SOUND] DFPlayer Mini playing state timed out, forcing reset\n");
                dfMiniPlaying = false;
            }
        }
        return dfMiniPlaying;
    }
    return false;
#else
    return false;
#endif
}

bool JaamSound::isDFPlayerConnected() {
#if DFPLAYER_ENABLED
    return dfBackend != DFBackend::NONE;
#else
    return false;
#endif
}

int JaamSound::getDFBackend() {
#if DFPLAYER_ENABLED
    return dfBackend;
#else
    return DFBackend::NONE;
#endif
}

bool JaamSound::wasDFPlayerInitAttempted() {
#if DFPLAYER_ENABLED
    return dfInitAttempted;
#else
    return false;
#endif
}

