#pragma once
#include "JaamLogs.h"
#include <string>

struct Firmware {
    int major = 0;
    int minor = 0;
    int patch = 0;
    int betaBuild = 0;
    bool isBeta = false;
  };
  
static Firmware parseFirmwareVersion(const char* version) {

    Firmware firmware;

    char* versionCopy = strdup(version);
    char* token = strtok(versionCopy, ".-");

    int part = 0;
    while (token) {
        if (isdigit(token[0])) {
        if (part == 0)
            firmware.major = atoi(token);
        else if (part == 1)
            firmware.minor = atoi(token);
        else if (part == 2)
            firmware.patch = atoi(token);
        part++;
        } else if (token[0] == 'b' && strcmp(token, "bin") != 0) {
        firmware.isBeta = true;
        firmware.betaBuild = atoi(token + 1); // Skip the 'b' character
        }
        token = strtok(NULL, ".-");
    }

    free(versionCopy);

    return firmware;
}

static void fillFwVersion(char* result, Firmware firmware) {
    std::string version = std::to_string(firmware.major) + "." + std::to_string(firmware.minor);
  
    if (firmware.patch > 0) {
      version += "." + std::to_string(firmware.patch);
    }
  
    if (firmware.isBeta) {
      version += "-b" + std::to_string(firmware.betaBuild);
    }
    #if ARDUINO_ESP32S3_DEV
    version += "-s3";
    #elif ARDUINO_ESP32C3_DEV
    version += "-c3";
    #endif

    #if LITE
    version += "-lite";
    #endif

    #if TEST_MODE
    version += "-test";
    #endif

    #if FUSION
    version += "-fusion";
    #endif

    strcpy(result, version.c_str());
}