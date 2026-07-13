#include "JaamSound.h"
#include "JaamLogs.h"

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

void JaamSound::initDFPlayer(int backend) {
    if (!isDFPlayerEnabled()) {
        LOG.printf("[SOUND] DFPlayer pins not set, skip init\n");
        return;
    }
    dfConnected = false;
    dfBackend = DFBackend::NONE;
    dfMiniPlaying = false;
    int8_t attempts = 5;
    int8_t count = 1;
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

        while (!dfplayerPro.begin(dfSerial)) {
            LOG.printf("[SOUND] Attempt #%d of %d\n", count, attempts);
            LOG.printf("[SOUND] DFPlayer PRO not found...\n");
            delay(1000);
            count++;
            if (count > attempts) {
                LOG.printf("[SOUND] DFPlayer PRO init failed: max attempts reached\n");
                return;
            }
        }
        LOG.printf("[SOUND] DFPlayer PRO RX OK!\n");
        dfplayerPro.setVol(2);
        delay(500);
        if (dfplayerPro.getVol() != 2) {
            LOG.printf("[SOUND] DFPlayer PRO TX Fail!\n");
            return;
        }
        LOG.printf("[SOUND] DFPlayer PRO TX OK!\n");
        dfConnected = true;
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

        while (!dfplayerMini.begin(dfSerial, /*isACK=*/true, /*doReset=*/true)) {
            LOG.printf("[SOUND] Attempt #%d of %d\n", count, attempts);
            LOG.printf("[SOUND] DFPlayer Mini not found...\n");
            delay(1000);
            count++;
            if (count > attempts) {
                LOG.printf("[SOUND] DFPlayer Mini init failed: max attempts reached\n");
                return;
            }
        }
        LOG.printf("[SOUND] DFPlayer Mini ready!\n");
        dfConnected = true;
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
    }
    LOG.printf("[SOUND] DFPlayer files found: %d\n", dfTotalFiles);
}

void JaamSound::playDFPlayer(int trackNumber) {
    if (!dfConnected) {
        LOG.printf("[SOUND] DFPlayer not connected, cannot play track\n");
        return;
    }
    if (dfBackend == DFBackend::PRO) {
        char path[16];
        snprintf(path, sizeof(path), "/%02d.mp3", trackNumber);
        dfplayerPro.playSpecFile(path);
        LOG.printf("[SOUND] Track played: %s (%s)\n", path, dfplayerPro.getFileName());
    } else if (dfBackend == DFBackend::MINI) {
        dfplayerMini.play(trackNumber);
        dfMiniPlaying = true;
        LOG.printf("[SOUND] Track played: #%d\n", trackNumber);
    }
}

void JaamSound::setDFPlayerVolume(int volume) {
    if (!dfConnected) {
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
    if (!dfConnected) {
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

int JaamSound::getDFBackend() {
    return dfConnected ? dfBackend : DFBackend::NONE;
}
#endif

bool JaamSound::isDFPlayerEnabled() {
    return dfRxPin > -1 && dfTxPin > -1;
}

bool JaamSound::isDFPlayerPlaying() {
#if DFPLAYER_ENABLED
    if (!dfConnected) {
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
        return dfMiniPlaying;
    }
    return false;
#else
    return false;
#endif
}

bool JaamSound::isDFPlayerConnected() {
#if DFPLAYER_ENABLED
    return dfConnected;
#else
    return false;
#endif
}

