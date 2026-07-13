#pragma once
#if BUZZER_ENABLED
#include <melody_player.h>
#include <melody_factory.h>
#endif
#if DFPLAYER_ENABLED
#include <DFRobot_DF1201S.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>
#endif
#include "JaamConfig.h"

// DF Player backend, selected at runtime by SOUND_SOURCE (only one physical module is ever wired up)
namespace DFBackend {
    constexpr int NONE = 0;
    constexpr int PRO = 1;
    constexpr int MINI = 2;
}

class JaamSound {
    private:
        // Перцептивна (степенева, gamma≈2) шкала гучності: людський слух сприймає гучність
        // логарифмічно, лінійний map() на низьких % дає завелику гучність
        int expMap(int x, int in_min, int in_max, int out_min, int out_max) {
            if (in_max == in_min) return out_min;
            float normalized = (float)(x - in_min) / (float)(in_max - in_min);
            if (normalized < 0.0f) normalized = 0.0f;
            if (normalized > 1.0f) normalized = 1.0f;
            float scaled = normalized * normalized; // gamma≈2
            return (int)(scaled * (out_max - out_min) + out_min);
        }
    #if BUZZER_ENABLED
        MelodyPlayer* player;
    #endif
    #if DFPLAYER_ENABLED
        HardwareSerial dfSerial;
        DFRobot_DF1201S dfplayerPro;
        DFRobotDFPlayerMini dfplayerMini;
        int dfPlayerMaxVolume;
        int dfBackend; // DFBackend::NONE / PRO / MINI - which module is currently initialised
        bool dfMiniPlaying;
    #endif
        int buzzerPin;
        bool dfConnected;
        int dfRxPin;
        int dfTxPin;

    public:
        int dfTotalFiles;
        int volumeCurrent;
        int volumeDay;
        int volumeNight;
        int soundSource;// -1 Вимкнено, 0 Buzzer, 1 DF Player PRO, 2 DF Player Mini
        int beepHour;

        JaamSound() :
        #if DFPLAYER_ENABLED
            dfSerial(2),
            dfPlayerMaxVolume(15),
            dfBackend(DFBackend::NONE),
            dfMiniPlaying(false),
        #endif
            dfConnected(false),
            dfTotalFiles(0),
            buzzerPin(-1),
            dfRxPin(-1),
            dfTxPin(-1),
            beepHour(-1),
            soundSource(-1)
        {};
        void init(int buzzerPin, int rxPin, int txPin, int volCurrent, int volDay, int volNight);
        void setVolumeCurrent(int volume);
        void setVolumeDay(int volume);
        void setVolumeNight(int volume);
        void setSoundSource(int source);
        void setBeepHour(int hour);
    #if DFPLAYER_ENABLED
        void setDFMaxVolume(int maxVolume);
    #endif
    #if BUZZER_ENABLED
        void initBuzzer();
        void playBuzzer(const char* melodyRtttl);
        void setBuzzerVolume(int volume);
    #endif
    #if DFPLAYER_ENABLED
        void initDFPlayer(int backend);
        void playDFPlayer(int trackNumber);
        void setDFPlayerVolume(int volume);
        int getDFPlayerFilesCount();
        int getDFBackend(); // DFBackend::NONE / PRO / MINI - what is currently connected
    #endif
        bool isBuzzerEnabled();
        bool isBuzzerPlaying();
        bool isDFPlayerEnabled();
        bool isDFPlayerPlaying();
        bool isDFPlayerConnected();
};