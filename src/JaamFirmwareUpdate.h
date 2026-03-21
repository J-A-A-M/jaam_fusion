#pragma once

#include <Arduino.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <functional>
#include "JaamDisplay.h"
#include "JaamApi.h"

struct JaamFirmware {
    int major = 0;
    int minor = 0;
    int patch = 0;
    int beta = 0;
};

class JaamFirmwareUpdate {
public:
    void setDisplay(JaamDisplay* display);
    void setApi(JaamApi* api);
    void setMapUpdateCallback(std::function<void(float)> cb);
    void setRebootCallback(std::function<void()> cb);
    void setServicePinCallback(std::function<void()> cb);

    void init(const char* version);
    void initCallbacks();

    // Parses websocket TYPE_FIRMWARE_UPDATE_BETA/PROD_BATCH payload (without header byte).
    // isActiveChannel=true — оновлює спільний стан (_newFwVersion, _fwUpdateAvailable) і сповіщає API.
    void processBatch(const uint8_t* data, size_t bodyLen, bool isBeta, bool isActiveChannel);

    bool requestUpdate(const char* id, bool isBeta);
    void applyActiveChannel(bool isBeta);
    bool isUpdateRequested() const;
    void clearUpdateRequest();
    void download();
    bool isValidFirmwareId(const char* id, bool isBeta) const;

    bool isUpdateAvailable() const;
    const char* getCurrentVersion() const;
    const char* getNewVersion() const;
    const char* getUpdateId() const;
    const JaamFirmware* getFirmwaresBeta() const;
    const JaamFirmware* getFirmwaresProd() const;
    const JaamFirmware& getFirmware() const;

private:
    char _currentFwVersion[25] = {};
    char _newFwVersion[25] = {};
    char _fwUpdateId[25] = {};
    bool _fwUpdateAvailable = false;
    volatile bool _needUpdate = false;
    static constexpr size_t MAX_FW = 10;
    JaamFirmware _firmwares_beta[MAX_FW] = {};
    JaamFirmware _firmwares_prod[MAX_FW] = {};
    JaamFirmware _firmware = {};
    WiFiClientSecure _updateClient;

    JaamDisplay* _display = nullptr;
    JaamApi* _api = nullptr;
    std::function<void(float)> _mapUpdateCb;
    std::function<void()> _rebootCb;
    std::function<void()> _servicePinCb;

    void handleUpdateStatus(t_httpUpdate_return ret, bool isSpiffsUpdate);

    static bool isNewerFirmware(const JaamFirmware& candidate, const JaamFirmware& current);
    static JaamFirmware parseFirmwareVersion(const char* version);
    static void fillFwVersion(char* result, JaamFirmware fw);
};
