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
    LOG.println("[SOUND] Init Buzzer");
    if (!isBuzzerEnabled()) {
        LOG.println("[SOUND] Buzzer pin is not set, skip init");
        return;
    }
    if (player) delete player;
    player = new MelodyPlayer(buzzerPin, 0, LOW);
    player->setVolume(expMap(volumeCurrent, 0, 100, 0, 255));
    LOG.printf("[SOUND] Set initial volume to: %d\n", volumeCurrent);

}

void JaamSound::playBuzzer(const char* melodyRtttl) {
    if (player == nullptr) {
        LOG.println("[SOUND] Buzzer not initialised, cannot play melody");
        return;
    }
    Melody melody = MelodyFactory.loadRtttlString(melodyRtttl);
    player->playAsync(melody);
}

void JaamSound::setBuzzerVolume(int volume) {
    if (player == nullptr) {
        LOG.println("[SOUND] Buzzer not initialised, cannot set volume");
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
        LOG.println("[SOUND] Buzzer not enabled, cannot check if playing");
        return false;
    }
    if (player == nullptr) {
        LOG.println("[SOUND] Buzzer not initialised, cannot check if playing");
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
        LOG.println("[SOUND] DFPlayer not connected, cannot check files limit");
        return false;
    }
    if (dfTotalFiles > maxFilesCount) {
        LOG.printf("[SOUND] DFPlayer files limit reached: %d/%d\n", dfTotalFiles, maxFilesCount);
        return true;
    }
    return false;
}
void JaamSound::initDFPlayer() {
    LOG.println("[SOUND] Init DFPlayer");
    if (!isDFPlayerEnabled()) {
        LOG.println("[SOUND] DFPlayer pins not set, skip init");
        return;
    }
    int8_t attempts = 5;
    int8_t count = 1;
    dfSerial.begin(115200, SERIAL_8N1, dfRxPin, dfTxPin); // RX, TX

    LOG.printf("[SOUND] rx, tx: %d, %d\n", dfRxPin, dfTxPin);

    // Turn off the start prompt
    dfSerial.print("AT+PROMPT=OFF\r\n");
    delay(100);
    while (dfSerial.available()) {
        LOG.println(dfSerial.read());
    }

    while (!dfplayer.begin(dfSerial)) {
      LOG.printf("[SOUND] Attempt #%d of %d\n", count, attempts);
      LOG.println("[SOUND] DFPlayer not found...");
      delay(1000);
      count++;
      if (count > attempts) {
        LOG.println("[SOUND] DFPlayer init failed: max attempts reached");
        return;
      }
      
    }
    LOG.println("[SOUND] DFPlayer RX OK!");
    dfplayer.setVol(2);
    delay(500); 
    if (dfplayer.getVol() != 2) {
      LOG.println("[SOUND] DFPlayer TX Fail!");
      return;
    }
    LOG.println("[SOUND] DFPlayer TX OK!");
    dfConnected = true;
    LOG.println("[SOUND] DFPlayer ready!");

    dfplayer.setVol(0); 
    dfplayer.switchFunction(dfplayer.MUSIC);
    dfplayer.setVol(map(volumeCurrent, 0, 100, 0, dfPlayerMaxVolume));
    LOG.print("[SOUND] Volume: ");
    LOG.println(dfplayer.getVol());

    dfplayer.setPlayMode(dfplayer.SINGLECYCLE);
    LOG.print("[SOUND] PlayMode: ");
    LOG.println(dfplayer.getPlayMode());
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
      LOG.print(dynamicTrackNames[i].id);
      LOG.print(": ");
      LOG.print(dynamicTracks[i]);
      LOG.print(" - ");
      LOG.println(dynamicTrackNames[i].name);
    }
}

void JaamSound::playDFPlayer(String trackPath) {
    if (!dfConnected) {
        LOG.println("[SOUND] DFPlayer not connected, cannot play track");
        return;
    }
    dfplayer.playSpecFile(trackPath);
    LOG.printf("[SOUND] Track played: %s (%s)\n", trackPath.c_str(), dfplayer.getFileName());
}


void JaamSound::setDFPlayerVolume(int volume) {
    if (!dfConnected) {
        LOG.println("[SOUND] DFPlayer not connected, cannot set volume");
        return;
    }
    dfplayer.setVol(map(volume, 0, 100, 0, dfPlayerMaxVolume));
    LOG.printf("[SOUND] Set DFPlayer volume to: %d\n", volume);
}



int JaamSound::getDFPlayerFilesCount() {
    if (!dfConnected) {
        LOG.println("[SOUND] DFPlayer not connected, cannot get files count");
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
        LOG.println("[SOUND] DFPlayer not connected, cannot check if playing");
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

