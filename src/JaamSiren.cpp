#include "JaamSiren.h"
#include "JaamConfig.h"
#include "JaamLogs.h"

JaamSiren::JaamSiren() 
    : settings(nullptr), 
      alertPin(-1), 
      clearPin(-1), 
      pinModeConfig(0), 
      activeLevel(HIGH),
      pinTime(1000),
      alertActivationTime(0),
      clearActivationTime(0),
      alertActive(false),
      clearActive(false),
      alertInHomeRegion(false) {
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
    
    alertPin = settings->getInt(ALERT_PIN);
    clearPin = settings->getInt(CLEAR_PIN);
    pinModeConfig = settings->getInt(ALERT_CLEAR_PIN_MODE);
    activeLevel = settings->getInt(ALERT_PIN_ACTIVE_LEVEL) == 0 ? LOW : HIGH;
    pinTime = settings->getInt(ALERT_CLEAR_PIN_TIME);
    
    LOG.printf("[SIREN] Configuration: alertPin=%d, clearPin=%d, mode=%d, activeLevel=%s, time=%dms\n", 
               alertPin, clearPin, pinModeConfig, activeLevel == HIGH ? "HIGH" : "LOW", pinTime);
}

void JaamSiren::setupPins() {
    // Налаштовуємо піни як OUTPUT якщо вони встановлені (значення > 0)
    int inactiveLevel = (activeLevel == HIGH) ? LOW : HIGH;
    
    if (alertPin > 0) {
        pinMode(alertPin, OUTPUT);
        digitalWrite(alertPin, inactiveLevel);
        LOG.printf("[SIREN] Alert pin %d configured as OUTPUT (inactive=%s)\n", 
                   alertPin, inactiveLevel == HIGH ? "HIGH" : "LOW");
    }
    
    // CLEAR_PIN налаштовуємо тільки в імпульсному режимі (mode=1)
    if (pinModeConfig == 1 && clearPin > 0) {
        pinMode(clearPin, OUTPUT);
        digitalWrite(clearPin, inactiveLevel);
        LOG.printf("[SIREN] Clear pin %d configured as OUTPUT (inactive=%s)\n", 
                   clearPin, inactiveLevel == HIGH ? "HIGH" : "LOW");
    }
    
    alertActive = false;
    clearActive = false;
    if (alertInHomeRegion) {
        LOG.printf("[SIREN] Alert in home region, activating alert pin\n");
        setAlert();
    }
}

void JaamSiren::setAlert() {
    alertInHomeRegion = true; // Встановлюємо прапорець, що в домашньому регіоні тривога
    if (alertPin <= 0) {
        LOG.printf("[SIREN] Alert pin not configured\n");
        return;
    }
    
    activatePin(alertPin);
    alertActive = true;
    
    if (pinModeConfig == 1) {
        // Імпульсний режим: запускаємо таймер для автоматичної деактивації
        alertActivationTime = millis();
        LOG.printf("[SIREN] Alert activated on pin %d (pulse mode, %dms)\n", alertPin, pinTime);
    } else {
        // Бістабільний режим: залишаємо активним до явної деактивації
        LOG.printf("[SIREN] Alert activated on pin %d (bistable mode)\n", alertPin);
    }
}

void JaamSiren::clearAlert() {
    alertInHomeRegion = false; // Скидаємо прапорець, в домашньому регіоні безпечно
    if (pinModeConfig == 0) {
        // Бістабільний режим: деактивуємо ALERT_PIN
        if (alertPin > 0) {
            deactivatePin(alertPin);
            alertActive = false;
            LOG.printf("[SIREN] Alert deactivated on pin %d (bistable mode)\n", alertPin);
        }
    } else {
        // Імпульсний режим: активуємо CLEAR_PIN на час pinTime
        if (clearPin <= 0) {
            LOG.printf("[SIREN] Clear pin not configured\n");
            return;
        }
        
        activatePin(clearPin);
        clearActive = true;
        clearActivationTime = millis();
        
        LOG.printf("[SIREN] Clear activated on pin %d (pulse mode, %dms)\n", clearPin, pinTime);
    }
}

void JaamSiren::resetPins() {
    if (alertPin > 0) {
        deactivatePin(alertPin);
        alertActive = false;
    }
    
    if (clearPin > 0) {
        deactivatePin(clearPin);
        clearActive = false;
    }
    
    LOG.printf("[SIREN] All pins reset to inactive level\n");
}

void JaamSiren::activatePin(int pin) {
    if (pin > 0) {
        digitalWrite(pin, activeLevel);
    }
}

void JaamSiren::deactivatePin(int pin) {
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

void JaamSiren::tick() {
    // Таймери працюють тільки в імпульсному режимі
    if (pinModeConfig != 1) {
        return;
    }
    
    // Перевіряємо чи потрібно деактивувати піни після заданого часу
    unsigned long currentTime = millis();
    unsigned long activationDuration = (unsigned long)pinTime;
    
    // Перевіряємо alert pin
    if (alertActive && alertPin > 0) {
        if (currentTime - alertActivationTime >= activationDuration) {
            deactivatePin(alertPin);
            alertActive = false;
            LOG.printf("[SIREN] Alert pin %d deactivated after %dms\n", alertPin, pinTime);
        }
    }
    
    // Перевіряємо clear pin
    if (clearActive && clearPin > 0) {
        if (currentTime - clearActivationTime >= activationDuration) {
            deactivatePin(clearPin);
            clearActive = false;
            LOG.printf("[SIREN] Clear pin %d deactivated after %dms\n", clearPin, pinTime);
        }
    }
}
