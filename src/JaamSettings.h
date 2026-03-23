#pragma once
#include <Print.h>
#include <Preferences.h>
#include <functional>
#include "JaamConfig.h"

// Callback для повідомлення про зміни налаштувань
typedef std::function<void(Type type, int intValue, float fltValue, const char* strValue)> SettingsChangeCallback;

class JaamSettings {

public:
    JaamSettings();
    void init();
    const char* getKey(Type type);
    int getInt(Type type);
    bool saveInt(Type type, int value, bool saveToPrefs = true);
    const char* getString(Type type);
    bool saveString(Type type, const char* value, bool saveToPrefs = true);
    float getFloat(Type type);
    bool saveFloat(Type type, float value, bool saveToPrefs = true);
    bool getBool(Type type);
    bool saveBool(Type type, bool value, bool saveToPrefs = true);
    bool hasKey(Type type);
    void getSettingsBackup(Print* stream, const char* fwVersion, const char* chipID, const char* time);
    bool restoreSettingsBackup(const char* settings);

    // Реєстрація callback для відстеження змін
    void setChangeCallback(SettingsChangeCallback callback);

private:
    Preferences preferences;
    SettingsChangeCallback changeCallback;

    // Валідація значень перед збереженням
    bool validateIntSetting(Type type, int value);
};
