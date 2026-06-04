#include "JaamSound.h"
#include "JaamLogs.h"

#if BUZZER_ENABLED || DFPLAYER_PRO_ENABLED
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

int JaamSound::findTrackIndex(int number) {
    #if DFPLAYER_PRO_ENABLED
      String trackName = String("/") + (number < 10 ? "0" : "") + String(number) + ".mp3";
    
      for (int i = 0; i < TRACKS_COUNT; i++) {
        if (TRACKS[i] == trackName) {
          return i;
        }
      }
    #endif
      return -1;
    }
String JaamSound::getTrackById(int id) {
    if (dfConnected) {
        for (int i = 0; i < dfTotalFiles; i++) {
            if (dynamicTrackNames[i].id == id) {
            return dynamicTracks[i];
            }
        }
    }
    return "";
}

#if DFPLAYER_PRO_ENABLED
bool JaamSound::isDFPlayerFilesLimitReached(int dfTotalFiles) {
    if (!dfConnected) {
        LOG.printf("[SOUND] DFPlayer not connected, cannot check files limit\n");
        return false;
    }
    if (dfTotalFiles > maxFilesCount) {
        LOG.printf("[SOUND] DFPlayer files limit reached: %d/%d\n", dfTotalFiles, maxFilesCount);
        return true;
    }
    return false;
}
void JaamSound::initDFPlayer() {
    LOG.printf("[SOUND] Init DFPlayer\n");
    if (!isDFPlayerEnabled()) {
        LOG.printf("[SOUND] DFPlayer pins not set, skip init\n");
        return;
    }
    dfConnected = false;
    int8_t attempts = 5;
    int8_t count = 1;
    dfSerial.end();
    delay(50);
    dfSerial.begin(115200, SERIAL_8N1, dfRxPin, dfTxPin); // RX, TX
    delay(500); // дати модулю час прокинутися після (ре)ініціалізації UART

    LOG.printf("[SOUND] rx, tx: %d, %d\n", dfRxPin, dfTxPin);

    // Turn off the start prompt
    dfSerial.print("AT+PROMPT=OFF\r\n");
    delay(200);
    while (dfSerial.available()) {
        LOG.printf("%c", dfSerial.read());
    }

    while (!dfplayer.begin(dfSerial)) {
      LOG.printf("[SOUND] Attempt #%d of %d\n", count, attempts);
      LOG.printf("[SOUND] DFPlayer not found...\n");
      delay(1000);
      count++;
      if (count > attempts) {
        LOG.printf("[SOUND] DFPlayer init failed: max attempts reached\n");
        return;
      }
      
    }
    LOG.printf("[SOUND] DFPlayer RX OK!\n");
    dfplayer.setVol(2);
    delay(500); 
    if (dfplayer.getVol() != 2) {
      LOG.printf("[SOUND] DFPlayer TX Fail!\n");
      return;
    }
    LOG.printf("[SOUND] DFPlayer TX OK!\n");
    dfConnected = true;
    LOG.printf("[SOUND] DFPlayer ready!\n");

    dfplayer.setVol(0); 
    dfplayer.switchFunction(dfplayer.MUSIC);
    dfplayer.setVol(map(volumeCurrent, 0, 100, 0, dfPlayerMaxVolume));
    LOG.printf("[SOUND] Volume: %d\n", dfplayer.getVol());

    dfplayer.setPlayMode(dfplayer.SINGLE);
    LOG.printf("[SOUND] PlayMode: %d\n", dfplayer.getPlayMode());
    delay(500);

    dfplayer.setLED(false);

    dfTotalFiles = getDFPlayerFilesCount();
    if (dfTotalFiles <= 0) {
    LOG.printf("[SOUND] DFPlayer has no playable files\n");
      return;
    }
    if (isDFPlayerFilesLimitReached(dfTotalFiles)) {
      LOG.printf("[SOUND] DFPlayer files limit reached: (%d/%d)\n", dfTotalFiles, maxFilesCount);
      return;
    }

    dynamicTracks = new String[dfTotalFiles];
    dynamicTrackNames = new SettingListItem[dfTotalFiles];

    for (int i = 0; i < dfTotalFiles; i++) {
      int fileNumber = i + 1;

      int foundIndex = findTrackIndex(fileNumber);

      if (foundIndex >= 0) {
        dynamicTracks[i] = TRACKS[foundIndex];
        dynamicTrackNames[i] = TRACK_NAMES[foundIndex];
      } else {
        String trackPath = String("/") + (fileNumber < 10 ? "0" : "") + String(fileNumber) + ".mp3";
        dynamicTracks[i] = trackPath;

        char buf[16];   
        snprintf(buf, sizeof(buf), "%d", fileNumber);

        dynamicTrackNames[i].id = i;
        dynamicTrackNames[i].name = strdup(buf);
        dynamicTrackNames[i].ignore = false;
      }
    }

    for (int i = 0; i < dfTotalFiles; i++) {
      LOG.printf("%d: %s - %s\n", dynamicTrackNames[i].id, dynamicTracks[i].c_str(), dynamicTrackNames[i].name);
    }
}

void JaamSound::playDFPlayer(String trackPath) {
    if (!dfConnected) {
        LOG.printf("[SOUND] DFPlayer not connected, cannot play track\n");
        return;
    }
    dfplayer.playSpecFile(trackPath);
    LOG.printf("[SOUND] Track played: %s (%s)\n", trackPath.c_str(), dfplayer.getFileName());
}


void JaamSound::setDFPlayerVolume(int volume) {
    if (!dfConnected) {
        LOG.printf("[SOUND] DFPlayer not connected, cannot set volume\n");
        return;
    }
    dfplayer.setVol(map(volume, 0, 100, 0, dfPlayerMaxVolume));
    LOG.printf("[SOUND] Set DFPlayer volume to: %d\n", volume);
}



int JaamSound::getDFPlayerFilesCount() {
    if (!dfConnected) {
        LOG.printf("[SOUND] DFPlayer not connected, cannot get files count\n");
        return 0;
    }
    int filesCount = dfplayer.getTotalFile();
    LOG.printf("[SOUND] DFPlayer files count: %d\n", filesCount);
    return filesCount;
}
#endif

bool JaamSound::isDFPlayerEnabled() {
    return dfRxPin > -1 && dfTxPin > -1;
}

bool JaamSound::isDFPlayerPlaying() {
#if DFPLAYER_PRO_ENABLED
    if (!dfConnected) {
        LOG.printf("[SOUND] DFPlayer not connected, cannot check if playing\n");
        return false;
    }
    return dfplayer.isPlaying();
#else
    return false;
#endif
}

bool JaamSound::isDFPlayerConnected() {
#if DFPLAYER_PRO_ENABLED
    return dfConnected;
#else
    return false;
#endif
}

