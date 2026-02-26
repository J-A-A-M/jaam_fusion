#include "JaamSiren.h"
#include "JaamConfig.h"
#include "JaamLogs.h"
#include <async.h>

extern Async async;

// Глобальний вказівник для callback функцій
static JaamSiren* g_sirenInstance = nullptr;

// Глобальні callback функції для async.setTimeout
void sirenAlertCallback() {
    if (g_sirenInstance) {
        g_sirenInstance->onAlertTimeout();
    }
}

void sirenClearCallback() {
    if (g_sirenInstance) {
        g_sirenInstance->onClearTimeout();
    }
}

void sirenAlert2Callback() {
    if (g_sirenInstance) {
        g_sirenInstance->onAlertTimeout2();
    }
}

void sirenClear2Callback() {
    if (g_sirenInstance) {
        g_sirenInstance->onClearTimeout2();
    }
}

JaamSiren::JaamSiren()
    : settings(nullptr),
      alertPin(-1),
      clearPin(-1),
      pinModeConfig(0),
      activeLevel(HIGH),
      pinTime(1000),
      alertTimer(-1),
      clearTimer(-1),
      alertActive(false),
      clearActive(false),
      alertInHomeRegion(false),
      alertPin2(-1),
      clearPin2(-1),
      pinModeConfig2(0),
      activeLevel2(HIGH),
      pinTime2(1000),
      alertTimer2(-1),
      clearTimer2(-1),
      alertActive2(false),
      clearActive2(false) {
    g_sirenInstance = this; // Встановлюємо глобальний вказівник
}

void JaamSiren::setSettings(JaamSettings* settings) {
    this->settings = settings;
}

void JaamSiren::onSettingsChange(Type type, int intValue, const char* strValue) {
    if (!settings) {
        return;
    }
    
    // Відстежуємо зміни критичних налаштувань для сирени
    switch (type) {
        case ALERT_PIN:
        case CLEAR_PIN:
        case ALERT_CLEAR_PIN_MODE:
        case ALERT_CLEAR_PIN_TIME:
        case ALERT_PIN_ACTIVE_LEVEL:
        case ALERT_PIN_2:
        case CLEAR_PIN_2:
        case ALERT_CLEAR_PIN_MODE_2:
        case ALERT_CLEAR_PIN_TIME_2:
        case ALERT_PIN_ACTIVE_LEVEL_2:
            // При зміні налаштувань пінів та режимів перезапускаємо конфігурацію
            LOG.printf("[SIREN] Settings changed, reconfiguring\n");
            resetPins();
            init();
            break;
            
        default:
            break;
    }
}

void JaamSiren::init() {
    if (!settings) {
        LOG.printf("[SIREN] Cannot initialize: settings is null\n");
        return;
    }
    
    LOG.printf("[SIREN] Initializing siren\n");
    updateConfiguration();
    setupPins();
}

void JaamSiren::updateConfiguration() {
    if (!settings) {
        return;
    }
    
    // Перший комплект
    alertPin = settings->getInt(ALERT_PIN);
    clearPin = settings->getInt(CLEAR_PIN);
    pinModeConfig = settings->getInt(ALERT_CLEAR_PIN_MODE);
    activeLevel = settings->getInt(ALERT_PIN_ACTIVE_LEVEL) == 0 ? LOW : HIGH;
    pinTime = settings->getInt(ALERT_CLEAR_PIN_TIME);
    
    LOG.printf("[SIREN] Configuration 1: alertPin=%d, clearPin=%d, mode=%d, activeLevel=%s, time=%dms\n", 
               alertPin, clearPin, pinModeConfig, activeLevel == HIGH ? "HIGH" : "LOW", pinTime);
    
    // Другий комплект
    alertPin2 = settings->getInt(ALERT_PIN_2);
    clearPin2 = settings->getInt(CLEAR_PIN_2);
    pinModeConfig2 = settings->getInt(ALERT_CLEAR_PIN_MODE_2);
    activeLevel2 = settings->getInt(ALERT_PIN_ACTIVE_LEVEL_2) == 0 ? LOW : HIGH;
    pinTime2 = settings->getInt(ALERT_CLEAR_PIN_TIME_2);
    
    LOG.printf("[SIREN] Configuration 2: alertPin=%d, clearPin=%d, mode=%d, activeLevel=%s, time=%dms\n", 
               alertPin2, clearPin2, pinModeConfig2, activeLevel2 == HIGH ? "HIGH" : "LOW", pinTime2);
}

void JaamSiren::setupPins() {
    // Перший комплект
    int inactiveLevel = (activeLevel == HIGH) ? LOW : HIGH;
    
    if (alertPin > 0) {
        pinMode(alertPin, OUTPUT);
        digitalWrite(alertPin, inactiveLevel);
        LOG.printf("[SIREN 1] Alert pin %d configured as OUTPUT (inactive=%s)\n", 
                   alertPin, inactiveLevel == HIGH ? "HIGH" : "LOW");
    }
    
    // CLEAR_PIN налаштовуємо тільки в імпульсному режимі (mode=1)
    if (pinModeConfig == 1 && clearPin > 0) {
        pinMode(clearPin, OUTPUT);
        digitalWrite(clearPin, inactiveLevel);
        LOG.printf("[SIREN 1] Clear pin %d configured as OUTPUT (inactive=%s)\n", 
                   clearPin, inactiveLevel == HIGH ? "HIGH" : "LOW");
    }
    
    alertActive = false;
    clearActive = false;
    
    // Другий комплект
    int inactiveLevel2 = (activeLevel2 == HIGH) ? LOW : HIGH;
    
    if (alertPin2 > 0) {
        pinMode(alertPin2, OUTPUT);
        digitalWrite(alertPin2, inactiveLevel2);
        LOG.printf("[SIREN 2] Alert pin %d configured as OUTPUT (inactive=%s)\n", 
                   alertPin2, inactiveLevel2 == HIGH ? "HIGH" : "LOW");
    }
    
    if (pinModeConfig2 == 1 && clearPin2 > 0) {
        pinMode(clearPin2, OUTPUT);
        digitalWrite(clearPin2, inactiveLevel2);
        LOG.printf("[SIREN 2] Clear pin %d configured as OUTPUT (inactive=%s)\n", 
                   clearPin2, inactiveLevel2 == HIGH ? "HIGH" : "LOW");
    }
    
    alertActive2 = false;
    clearActive2 = false;
    
    if (alertInHomeRegion) {
        LOG.printf("[SIREN] Alert in home region, activating alert pins\n");
        setAlert();
    }
}

void JaamSiren::setAlert() {
    alertInHomeRegion = true; // Встановлюємо прапорець, що в домашньому регіоні тривога
    
    // Перший комплект
    if (alertPin > 0) {
        digitalWrite(alertPin, activeLevel);
        alertActive = true;
        
        if (pinModeConfig == 1) {
            // Імпульсний режим - встановлюємо таймер для автоматичного вимкнення
            if (alertTimer >= 0) {
                async.clearInterval(alertTimer);
            }
            alertTimer = async.setTimeout(sirenAlertCallback, pinTime);
            LOG.printf("[SIREN 1] Alert activated on pin %d (pulse mode, %dms)\n", alertPin, pinTime);
        } else {
            LOG.printf("[SIREN 1] Alert activated on pin %d (bistable mode)\n", alertPin);
        }
    }
    
    // Другий комплект
    if (alertPin2 > 0) {
        digitalWrite(alertPin2, activeLevel2);
        alertActive2 = true;
        
        if (pinModeConfig2 == 1) {
            if (alertTimer2 >= 0) {
                async.clearInterval(alertTimer2);
            }
            alertTimer2 = async.setTimeout(sirenAlert2Callback, pinTime2);
            LOG.printf("[SIREN 2] Alert activated on pin %d (pulse mode, %dms)\n", alertPin2, pinTime2);
        } else {
            LOG.printf("[SIREN 2] Alert activated on pin %d (bistable mode)\n", alertPin2);
        }
    }
}

void JaamSiren::clearAlert() {
    alertInHomeRegion = false; // Скидаємо прапорець, в домашньому регіоні безпечно
    
    // Перший комплект
    if (pinModeConfig == 0) {
        // Бістабільний режим: деактивуємо ALERT_PIN
        if (alertPin > 0) {
            deactivatePin(alertPin, activeLevel);
            alertActive = false;
            LOG.printf("[SIREN 1] Alert deactivated on pin %d (bistable mode)\n", alertPin);
        }
    } else {
        // Імпульсний режим: активуємо CLEAR_PIN
        if (clearPin > 0) {
            activatePin(clearPin);
            clearActive = true;
            
                if (clearTimer >= 0) {
                    async.clearInterval(clearTimer);
                }
                clearTimer = async.setTimeout(sirenClearCallback, pinTime);
            LOG.printf("[SIREN 1] Clear activated on pin %d (pulse mode, %dms)\n", clearPin, pinTime);
        }
    }
    
    // Другий комплект
    if (pinModeConfig2 == 0) {
        if (alertPin2 > 0) {
            deactivatePin(alertPin2, activeLevel2);
            alertActive2 = false;
            LOG.printf("[SIREN 2] Alert deactivated on pin %d (bistable mode)\n", alertPin2);
        }
    } else {
        if (clearPin2 > 0) {
            digitalWrite(clearPin2, activeLevel2);
            clearActive2 = true;
            
                if (clearTimer2 >= 0) {
                    async.clearInterval(clearTimer2);
                }
                clearTimer2 = async.setTimeout(sirenClear2Callback, pinTime2);
            LOG.printf("[SIREN 2] Clear activated on pin %d (pulse mode, %dms)\n", clearPin2, pinTime2);
        }
    }
}

void JaamSiren::resetPins() {
    // Скасування таймерів
    if (alertTimer >= 0) {
        async.clearInterval(alertTimer);
        alertTimer = -1;
    }
    if (clearTimer >= 0)
    {
        async.clearInterval(clearTimer);
        clearTimer = -1;
    }
    if (alertTimer2 >= 0)
    {
        async.clearInterval(alertTimer2);
        alertTimer2 = -1;
    }
    if (clearTimer2 >= 0)
    {
        async.clearInterval(clearTimer2);
        clearTimer2 = -1;
    }

    // Перший комплект
    if (alertPin > 0) {
        deactivatePin(alertPin, activeLevel);
        alertActive = false;
    }
    
    if (clearPin > 0) {
        deactivatePin(clearPin, activeLevel);
        clearActive = false;
    }
    
    // Другий комплект
    if (alertPin2 > 0) {
        deactivatePin(alertPin2, activeLevel2);
        alertActive2 = false;
    }
    
    if (clearPin2 > 0) {
        deactivatePin(clearPin2, activeLevel2);
        clearActive2 = false;
    }
    
    LOG.printf("[SIREN] All pins reset to inactive level\n");
}

void JaamSiren::activatePin(int pin) {
    if (pin > 0) {
        digitalWrite(pin, activeLevel);
    }
}

void JaamSiren::deactivatePin(int pin, int activeLevel) {
    int inactiveLevel = (activeLevel == HIGH) ? LOW : HIGH;
    if (pin > 0) {
        digitalWrite(pin, inactiveLevel);
    }
}

bool JaamSiren::isAlertActive() const {
    return alertActive;
}

bool JaamSiren::isClearActive() const {
    return clearActive;
}

// Callback методи для таймерів
void JaamSiren::onAlertTimeout() {
    if (alertTimer >= 0) {
        async.clearInterval(alertTimer);
        alertTimer = -1;
    }
    deactivatePin(alertPin, activeLevel);
    alertActive = false;
    LOG.printf("[SIREN 1] Alert pin %d deactivated after timeout\n", alertPin);
}

void JaamSiren::onClearTimeout() {
    if (clearTimer >= 0) {
        async.clearInterval(clearTimer);
        clearTimer = -1;
    }
    deactivatePin(clearPin, activeLevel);
    clearActive = false;
    LOG.printf("[SIREN 1] Clear pin %d deactivated after timeout\n", clearPin);
}

void JaamSiren::onAlertTimeout2() {
    if (alertTimer2 >= 0) {
        async.clearInterval(alertTimer2);
        alertTimer2 = -1;
    }
    deactivatePin(alertPin2, activeLevel2);
    alertActive2 = false;
    LOG.printf("[SIREN 2] Alert pin %d deactivated after timeout\n", alertPin2);
}

void JaamSiren::onClearTimeout2() {
    if (clearTimer2 >= 0) {
        async.clearInterval(clearTimer2);
        clearTimer2 = -1;
    }
    deactivatePin(clearPin2, activeLevel2);
    clearActive2 = false;
    LOG.printf("[SIREN 2] Clear pin %d deactivated after timeout\n", clearPin2);
}
