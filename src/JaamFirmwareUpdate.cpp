#include "JaamFirmwareUpdate.h"
#include "JaamLogs.h"
#include <string>

void JaamFirmwareUpdate::setDisplay(JaamDisplay* display) {
    _display = display;
}

void JaamFirmwareUpdate::setApi(JaamApi* api) {
    _api = api;
}

void JaamFirmwareUpdate::setMapUpdateCallback(std::function<void(float)> cb) {
    _mapUpdateCb = cb;
}

void JaamFirmwareUpdate::setRebootCallback(std::function<void()> cb) {
    _rebootCb = cb;
}

void JaamFirmwareUpdate::setServicePinCallback(std::function<void()> cb) {
    _servicePinCb = cb;
}

void JaamFirmwareUpdate::init(const char* version) {
    _firmware = parseFirmwareVersion(version);
    LOG.printf("[INIT] major: %d, minor: %d, patch: %d, beta: %d\n",
               _firmware.major, _firmware.minor, _firmware.patch, _firmware.beta);
    fillFwVersion(_currentFwVersion, _firmware);
    LOG.printf("[INIT] Current firmware version: %s\n", _currentFwVersion);
    snprintf(_fwUpdateId, sizeof(_fwUpdateId), "%s", version);
    LOG.printf("[INIT] Firmware Update ID initialized: %s\n", _fwUpdateId);
}

void JaamFirmwareUpdate::initCallbacks() {
    httpUpdate.onStart([this]() {
        if (_api) _api->updateFirmwareProgress(0);
        if (_display) _display->showServiceMessage("Починаємо!", "Оновлення:");
        delay(1000);
    });
    httpUpdate.onEnd([this]() {
        if (_api) _api->updateFirmwareProgress(100);
        if (_display) _display->showServiceMessage("Перезавантаження..", "Оновлення:");
        delay(1000);
    });
    httpUpdate.onProgress([this](int progress, int total) {
        if (total == 0) return;
        uint8_t percent = static_cast<uint8_t>((progress * 100) / total);
        if (percent > 100) percent = 100;
        LOG.printf("[UPDATE] Progress: %u%%\n", percent);
        char progressText[5];
        snprintf(progressText, sizeof(progressText), "%u%%", percent);
        if (_api) _api->updateFirmwareProgress(percent);
        if (_display) _display->showServiceMessage(progressText, "Оновлення:");
        if (_mapUpdateCb) _mapUpdateCb(percent / 100.0f);
    });
    httpUpdate.onError([this](int error) {
        if (_api) _api->updateFirmwareProgress(-1);
        if (!_display) return;
        switch (error) {
            case HTTP_UE_TOO_LESS_SPACE:
                _display->showServiceMessage("Замало місця", "Помилка оновлення:", 5000);
                break;
            case HTTP_UE_SERVER_NOT_REPORT_SIZE:
                _display->showServiceMessage("Невідомий розмір файлу", "Помилка оновлення:", 5000);
                break;
            case HTTP_UE_BIN_FOR_WRONG_FLASH:
                _display->showServiceMessage("Неправильний тип пам'яті", "Помилка оновлення:", 5000);
                break;
            default:
                _display->showServiceMessage("Щось пішло не так", "Помилка оновлення:", 5000);
                break;
        }
    });
}

void JaamFirmwareUpdate::processBatch(const uint8_t* data, size_t bodyLen) {
    static constexpr size_t RECORD_FW = 5;
    size_t count = bodyLen / RECORD_FW;
    const uint8_t* ptr = data;

    LOG.printf("[WEBSOCKET] TYPE_FIRMWARE_UPDATE_BATCH data processing\n");

    for (size_t i = 0; i < count; ++i) {
        _firmwares[i].major = ptr[0];
        _firmwares[i].minor = ptr[1];
        _firmwares[i].patch = ptr[2];
        // Little-Endian: low byte first, high byte second
        _firmwares[i].beta = ptr[3] | (ptr[4] << 8);
        LOG.printf("Parsed FW: %d.%d.%d b%d\n",
                   _firmwares[i].major, _firmwares[i].minor,
                   _firmwares[i].patch, _firmwares[i].beta);
        ptr += RECORD_FW;
    }

    // Знаходимо найновішу версію серед отриманих
    JaamFirmware latestInBatch{};
    for (size_t i = 0; i < count; ++i) {
        if ((_firmwares[i].major | _firmwares[i].minor | _firmwares[i].patch | _firmwares[i].beta) == 0) continue;
        if (isNewerFirmware(_firmwares[i], latestInBatch)) {
            latestInBatch = _firmwares[i];
        }
    }

    // Порівнюємо найновішу з поточною прошивкою
    fillFwVersion(_newFwVersion, latestInBatch);
    _fwUpdateAvailable = isNewerFirmware(latestInBatch, _firmware);

    if (_fwUpdateAvailable) {
        LOG.printf("[FIRMWARE] Update available: %s -> %s\n", _currentFwVersion, _newFwVersion);
    } else {
        LOG.printf("[FIRMWARE] No updates available. Current version: %s, Latest version: %s\n",
                   _currentFwVersion, _newFwVersion);
    }

    if (_api) _api->updateNewFirmwareInfo(_newFwVersion);

    if (_servicePinCb) _servicePinCb();
}

bool JaamFirmwareUpdate::requestUpdate(const char* id) {
    if (!isValidFirmwareId(id)) {
        _display->showServiceMessage("Невідома версія", "Помилка оновлення:", 5000);
        LOG.printf("[FIRMWARE] Invalid firmware ID: %s\n", id);
        return false;
    }
    
    snprintf(_fwUpdateId, sizeof(_fwUpdateId), "%s", id);
    _needUpdate = true;
    LOG.printf("[FIRMWARE] Update requested for: %s\n", id);
    return true;
}

bool JaamFirmwareUpdate::isUpdateRequested() const {
    return _needUpdate;
}

void JaamFirmwareUpdate::clearUpdateRequest() {
    _needUpdate = false;
}

void JaamFirmwareUpdate::download() {
    disableLoopWDT();

    LOG.println("Starting firmware update...");
    char firmwareUrlChar[100];

    LOG.println("Building firmware url...");
    sprintf(firmwareUrlChar, "https://update.jaam.net.ua/%s", _fwUpdateId);

    LOG.printf("Firmware url: %s\n", firmwareUrlChar);
    _updateClient.setInsecure();
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    t_httpUpdate_return fwRet = httpUpdate.update(_updateClient, firmwareUrlChar, VERSION);
    handleUpdateStatus(fwRet, false);

    enableLoopWDT();
}

bool JaamFirmwareUpdate::isUpdateAvailable() const {
    return _fwUpdateAvailable;
}

const char* JaamFirmwareUpdate::getCurrentVersion() const {
    return _currentFwVersion;
}

const char* JaamFirmwareUpdate::getNewVersion() const {
    return _newFwVersion;
}

const char* JaamFirmwareUpdate::getUpdateId() const {
    return _fwUpdateId;
}

const JaamFirmware* JaamFirmwareUpdate::getFirmwares() const {
    return _firmwares;
}

const JaamFirmware& JaamFirmwareUpdate::getFirmware() const {
    return _firmware;
}

void JaamFirmwareUpdate::handleUpdateStatus(t_httpUpdate_return ret, bool isSpiffsUpdate) {
    LOG.printf("%s update status:\n", isSpiffsUpdate ? "Spiffs" : "Firmware");
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            LOG.printf("Error Occurred. Error (%d): %s\n",
                       httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            break;
        case HTTP_UPDATE_NO_UPDATES:
            LOG.println("HTTP_UPDATE_NO_UPDATES");
            break;
        case HTTP_UPDATE_OK:
            if (isSpiffsUpdate) {
                LOG.println("Spiffs update successfully completed. Starting firmware update...");
            } else {
                LOG.println("Firmware update successfully completed. Rebooting...");
                if (_rebootCb) _rebootCb();
            }
            break;
    }
}

// Повертає true якщо 'candidate' новіший за 'current'.
// Стабільний (beta==0) вважається новішим за будь-який beta з тим самим major.minor.patch.
bool JaamFirmwareUpdate::isNewerFirmware(const JaamFirmware& candidate, const JaamFirmware& current) {
    if (candidate.major != current.major) return candidate.major > current.major;
    if (candidate.minor != current.minor) return candidate.minor > current.minor;
    if (candidate.patch != current.patch) return candidate.patch > current.patch;
    // Однакові major.minor.patch: stable > beta
    if (candidate.beta == 0 && current.beta > 0) return true;
    if (candidate.beta > 0 && current.beta == 0) return false;
    // Обидва beta або обидва stable: більший номер beta → новіший
    return candidate.beta > current.beta;
}

JaamFirmware JaamFirmwareUpdate::parseFirmwareVersion(const char* version) {
    JaamFirmware fw{};
    if (version == nullptr) return fw;
    char* versionCopy = strdup(version);
    if (versionCopy == nullptr) return fw;
    char* token = strtok(versionCopy, ".-");
    int part = 0;
    while (token) {
        if (isdigit(token[0])) {
            if (part == 0) fw.major = atoi(token);
            else if (part == 1) fw.minor = atoi(token);
            else if (part == 2) fw.patch = atoi(token);
            part++;
        } else if (token[0] == 'b' && strcmp(token, "bin") != 0) {
            fw.beta = atoi(token + 1);
        }
        token = strtok(NULL, ".-");
    }
    free(versionCopy);
    return fw;
}

void JaamFirmwareUpdate::fillFwVersion(char* result, JaamFirmware fw) {
    std::string version = std::to_string(fw.major) + "." + std::to_string(fw.minor);

    if (fw.patch > 0) {
        version += "." + std::to_string(fw.patch);
    }

    if (fw.beta > 0) {
        version += "-b" + std::to_string(fw.beta);
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

    strcpy(result, version.c_str());
}

bool JaamFirmwareUpdate::isValidFirmwareId(const char* id) const {
    if (id == nullptr || strlen(id) == 0) {
        return false;
    }
    
    // Parse the provided ID
    JaamFirmware candidate = parseFirmwareVersion(id);
    
    // Check if parsed firmware is valid (at least major.minor should be set)
    if (candidate.major == 0 && candidate.minor == 0) {
        return false;
    }
    
    // Search for matching firmware in the list
    for (int i = 0; i < 10; ++i) {
        const JaamFirmware& fw = _firmwares[i];
        
        // Skip empty entries
        if ((fw.major | fw.minor | fw.patch | fw.beta) == 0) {
            continue;
        }
        
        // Check if all version components match
        if (fw.major == candidate.major && 
            fw.minor == candidate.minor && 
            fw.patch == candidate.patch && 
            fw.beta == candidate.beta) {
            return true;
        }
    }
    
    return false;
}
