#include "SuplaServoDimmer.h"
#if defined(SUPLA_SERVO_CUSTOM)
#include "SuplaConfigManager.h"
#include "SuplaDeviceGUI.h"
#include <Arduino.h>

#ifndef SUPLA_LOG_DEBUG
#define SUPLA_LOG_DEBUG(fmt, ...) do { Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } while(0)
#endif

extern SuplaConfigManager *ConfigManager;
extern SuplaConfigESP *ConfigESP;

#ifndef OFF_GPIO
#define OFF_GPIO 255
#endif

SuplaServoDimmer::SuplaServoDimmer(int servoIndex) : _servoIndex(servoIndex) {
    setDefaultStateRestore();
    setFadeEffectTime(0); 
    
    _isMoving = false;
    _isAttached = false;
    _currentBrightness = 0;
    _targetBrightness = 0;
    _lastIterateTime = 0;
    _lastLogTime = 0;
    _currentMicroSec = 1500;
    _targetMicroSec = 1500;
    _lastWrittenMicroSec = 0;
    _isUp = true;
    _firstCloudSync = true;
    _isWaitingToDetach = false;

    auto elType = ConfigManager->get(KEY_SERVO_TYPE);
    _type = elType ? elType->getElement(_servoIndex).toInt() : 0;
    
    auto elChType = ConfigManager->get(KEY_SERVO_CH_TYPE);
    _chType = elChType ? elChType->getElement(_servoIndex).toInt() : 0;
    
    if (getChannel()) {
        switch(getMode()) {
            case MODE_RELAY:
                getChannel()->setType(SUPLA_CHANNELTYPE_RELAY);
                getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);
                break;
            case MODE_ROLLER_SHUTTER:
                getChannel()->setType(SUPLA_CHANNELTYPE_DIMMER);
                getChannel()->setDefault(SUPLA_CHANNELFNC_CONTROLLINGTHEROLLERSHUTTER);
                break;
            case MODE_DIMMER:
            default:
                getChannel()->setType(SUPLA_CHANNELTYPE_DIMMER);
                getChannel()->setDefault(SUPLA_CHANNELFNC_DIMMER);
                break;
        }
    }

    auto elInv = ConfigManager->get(KEY_SERVO_INVERTED);
    _inverted = elInv ? (elInv->getElement(_servoIndex).toInt() == 1) : false;
    
    auto elMin = ConfigManager->get(KEY_SERVO_ZERO);
    _minUs = elMin ? elMin->getElement(_servoIndex).toInt() : 500;
    if(_minUs < 100) _minUs = 500;
    
    auto elMid = ConfigManager->get(KEY_SERVO_STOP_US);
    _midUs = elMid ? elMid->getElement(_servoIndex).toInt() : 1500;
    if(_midUs < 100) _midUs = 1500;
    
    auto elMax = ConfigManager->get(KEY_SERVO_MAX_ANGLE);
    _maxUs = elMax ? elMax->getElement(_servoIndex).toInt() : 2500;
    if(_maxUs < 100) _maxUs = 2500;
    
    auto elTrans = ConfigManager->get(KEY_SERVO_SPEED);
    _transitionTimeMs = elTrans ? (unsigned long)elTrans->getElement(_servoIndex).toInt() : 500;
    if(_transitionTimeMs == 0) _transitionTimeMs = 500;
    
    auto elDet = ConfigManager->get(KEY_SERVO_TIME);
    _detachDelayMs = elDet ? (unsigned long)elDet->getElement(_servoIndex).toInt() : 0;
    
    auto elDb = ConfigManager->get(KEY_SERVO_SOFT_START);
    _deadbandPercent = elDb ? elDb->getElement(_servoIndex).toInt() : 3;

    auto elLUp = ConfigManager->get(KEY_SERVO_LIMIT_UP_STATE);
    _lUpState = elLUp ? (elLUp->getElement(_servoIndex).toInt() == 1) : false;

    auto elLDn = ConfigManager->get(KEY_SERVO_LIMIT_DOWN_STATE);
    _lDownState = elLDn ? (elLDn->getElement(_servoIndex).toInt() == 1) : false;
}

ServoMode SuplaServoDimmer::getMode() const {
    if (_type == 0) { 
        if (_chType == 2) return MODE_RELAY;
        if (_chType == 0) return MODE_ROLLER_SHUTTER;
        return MODE_DIMMER;
    } else { 
        if (_chType == 0) return MODE_RELAY;
        return MODE_ROLLER_SHUTTER;
    }
}

void SuplaServoDimmer::onInit() {
    Supla::Control::DimmerBase::onInit(); 

    _gpio = ConfigESP->getGpio(_servoIndex, FUNCTION_SERVO);
    _lUpGpio = ConfigESP->getGpio(_servoIndex, FUNCTION_LIMIT_SWITCH);
    _lDownGpio = ConfigESP->getGpio(_servoIndex + 5, FUNCTION_LIMIT_SWITCH);

    SUPLA_LOG_DEBUG("[SERVO_INIT %d] PIN: %d | Type: %d | Mode: %d | Min: %d | Mid: %d | Max: %d | Trans: %lu ms", 
                    _servoIndex, _gpio, _type, getMode(), _minUs, _midUs, _maxUs, _transitionTimeMs);

    if (_lUpGpio != OFF_GPIO) pinMode(_lUpGpio, ConfigESP->getPullUp(_lUpGpio) ? INPUT_PULLUP : INPUT);
    if (_lDownGpio != OFF_GPIO) pinMode(_lDownGpio, ConfigESP->getPullUp(_lDownGpio) ? INPUT_PULLUP : INPUT);

    if (_gpio != OFF_GPIO && _gpio != 0) {
        // Twarde zabicie systemowego PWM, aby nie gryzl sie z Servo.h
        #ifdef ARDUINO_ARCH_ESP8266
        analogWrite(_gpio, 0); 
        #endif
        
        pinMode(_gpio, OUTPUT);
        digitalWrite(_gpio, LOW);

        _currentBrightness = 0;
        if (getChannel()) {
            _currentBrightness = constrain(getChannel()->getValueBrightness(), 0, 255);
            SUPLA_LOG_DEBUG("[SERVO_INIT %d] Odczytano poczatkowa jasnosc z kanalu: %u", _servoIndex, _currentBrightness);
        }
        
        if(_type == 1 && getMode() == MODE_DIMMER) _currentBrightness = 127;

        _targetBrightness = _currentBrightness;
        _targetMicroSec = calculateMicroSec(_currentBrightness);
        _currentMicroSec = _targetMicroSec;
        _startMicroSec = _currentMicroSec;
    }
}

int SuplaServoDimmer::calculateMicroSec(uint32_t brightness255) {
    uint32_t safeBrightness = constrain(brightness255, 0, 255);
    if (_inverted) { safeBrightness = 255 - safeBrightness; }

    // Logika dla trybu Relay
    if (getMode() == MODE_RELAY) {
        if (brightness255 == 0) {
            return (_type == 1) ? _midUs : _minUs; 
        } else {
            return _maxUs;
        }
    }

    if (_type == 1) { 
        float percent = (safeBrightness / 255.0) * 100.0;
        float dev = abs(percent - 50.0);
        float dbHalf = _deadbandPercent / 2.0;

        if (dev <= dbHalf) return _midUs; 

        if (percent > 50.0) {
            int offset = (_deadbandPercent * 255) / 200;
            return map(safeBrightness, 128 + offset, 255, _midUs + 40, _maxUs);
        } else {
            int offset = (_deadbandPercent * 255) / 200;
            return map(safeBrightness, 0, 127 - offset, _minUs, _midUs - 40);
        }
    } else { 
        // Zero-float matematyka 32-bitowa dla serw 180
        uint32_t range = _maxUs - _minUs;
        return ((safeBrightness * range) / 255) + _minUs;
    }
}

uint32_t SuplaServoDimmer::calculateBrightnessFromMicroSec(int microSec) {
    if (_maxUs == _minUs) return 0;
    int minUs = _inverted ? _maxUs : _minUs;
    int maxUs = _inverted ? _minUs : _maxUs;
    long b = map(microSec, minUs, maxUs, 0, 255);
    return (uint32_t)constrain(b, 0, 255);
}

void SuplaServoDimmer::sendAckToCloud(uint32_t brightness255) {
    auto ch = getChannel();
    if (!ch) return;

    uint32_t safeBrightness = constrain(brightness255, 0, 255);
    SUPLA_LOG_DEBUG("[ACK_OUT %d] Wysylam status do chmury. Base val: %u", _servoIndex, safeBrightness);
    
    switch (getMode()) {
        case MODE_RELAY:
            // GŁÓWNY FIX RELAY (UI STATUS): Kanał przekaźnika w Supli V3 operuje logicznie (Boolean).
            // Wymuszamy RED = 1 (zamiast 255), aby serwer na value[0] odczytał poprawne "1" (ON).
            ch->setNewValue(safeBrightness > 0 ? 1 : 0, 0, 0, 0, safeBrightness > 0 ? 255 : 0);
            SUPLA_LOG_DEBUG("[ACK_OUT %d] Format Relay Wymuszony (value[0]=%d)", _servoIndex, safeBrightness > 0 ? 1 : 0);
            break;
            
        case MODE_ROLLER_SHUTTER: {
            double percentage = map(safeBrightness, 0, 255, 0, 100);
            ch->setNewValue(percentage, 0.0);
            SUPLA_LOG_DEBUG("[ACK_OUT %d] Format Rolety: %.1f %%", _servoIndex, percentage);
            break;
        }
            
        case MODE_DIMMER:
        default:
            ch->setNewValue(0, 0, 0, 0, safeBrightness);
            SUPLA_LOG_DEBUG("[ACK_OUT %d] Format Dimmer: %u", _servoIndex, safeBrightness);
            break;
    }
}

void SuplaServoDimmer::detachServoSafe() {
    if (_isAttached) {
        _servo.detach();
        digitalWrite(_gpio, LOW);
        _isAttached = false;
        _lastWrittenMicroSec = 0; 
        SUPLA_LOG_DEBUG("[SERVO %d] PWM Odciete (Detach)", _servoIndex);
    }
}

void SuplaServoDimmer::forceStopAndSync(uint8_t targetBrightness255) {
    SUPLA_LOG_DEBUG("[FORCE_STOP %d] Wywolano zatrzymanie! Target jasnosc: %u", _servoIndex, targetBrightness255);
    _targetMicroSec = (_type == 1) ? _midUs : _currentMicroSec;
    if (_isAttached) {
        _servo.writeMicroseconds(_targetMicroSec);
    }
    
    _currentBrightness = targetBrightness255;
    _targetBrightness = targetBrightness255;
    _isMoving = false;
    
    _isWaitingToDetach = true; 
    _targetReachedTime = millis();
    
    sendAckToCloud(_currentBrightness);
}

void SuplaServoDimmer::limitSwitchCheck360() {
    if (!_isAttached || _type == 0) return;

    bool hitUp = (_lUpGpio != OFF_GPIO && digitalRead(_lUpGpio) == _lUpState);
    bool hitDown = (_lDownGpio != OFF_GPIO && digitalRead(_lDownGpio) == _lDownState);

    if (hitUp && _targetMicroSec > _midUs) forceStopAndSync(255);
    else if (hitDown && _targetMicroSec < _midUs) forceStopAndSync(0);
}

void SuplaServoDimmer::handleAction(int event, int action) {
    SUPLA_LOG_DEBUG("[ACTION %d] Odebrano zdarzenie! Event: %d | Action: %d", _servoIndex, event, action);
    if (_gpio == OFF_GPIO || _gpio == 0) return;

    if (getMode() == MODE_ROLLER_SHUTTER) {
        if (action == 12 || action == 3) { 
            forceStopAndSync((_type == 1) ? 127 : calculateBrightnessFromMicroSec(_currentMicroSec));
            return;
        }
    }

    // Pozwalamy klasie bazowej DimmerBase zająć się wszystkimi akcjami przycisków fizycznych i chmury (ON/OFF/TOGGLE). 
    // Ona sama wywoła nasz 'setRGBWValueOnDevice' z odpowiednią wartością (1023 lub 0), a nasza matematyka zrobi resztę!
    Supla::Control::DimmerBase::handleAction(event, action);
}

void SuplaServoDimmer::setRGBWValueOnDevice(uint32_t r, uint32_t g, uint32_t b, uint32_t cb, uint32_t brightness) {
    SUPLA_LOG_DEBUG("[CLOUD_IN %d] Ramka wpadla z Supli! Brightness: %u (cb: %u)", _servoIndex, brightness, cb);
    if (_gpio == OFF_GPIO || _gpio == 0) return;

    // GŁÓWNY FIX ARCHITEKTONICZNY: Zbijamy sprzętowe 10-bitowe wymuszenie DimmerBase (0-1023) 
    // z powrotem na naszą twardą, ujednoliconą skalę 8-bitową (0-255).
    uint32_t target255 = map(constrain(brightness, 0, 1023), 0, 1023, 0, 255);

    if (_firstCloudSync) {
        _currentBrightness = target255;
        _targetMicroSec = calculateMicroSec(_currentBrightness);
        _currentMicroSec = _targetMicroSec;
        _startMicroSec = _currentMicroSec;
        _firstCloudSync = false;
        SUPLA_LOG_DEBUG("[CLOUD_IN %d] Pierwsza synchronizacja pominieta", _servoIndex);
        return; 
    }
    
    _targetBrightness = target255;
    _isUp = (_targetBrightness > _currentBrightness);
    
    _startMicroSec = _currentMicroSec;

    switch (getMode()) {
        case MODE_DIMMER:
        case MODE_ROLLER_SHUTTER:
            _targetMicroSec = calculateMicroSec(_targetBrightness);
            break;

        case MODE_RELAY:
            if (_targetBrightness > 0) {
                _targetMicroSec = _inverted ? _minUs : _maxUs;
            } else {
                _targetMicroSec = (_type == 1) ? _midUs : (_inverted ? _maxUs : _minUs);
            }
            break;
    }

    SUPLA_LOG_DEBUG("[MATH %d] Target %u -> TargetPulse: %d us (Start: %d us)", _servoIndex, _targetBrightness, _targetMicroSec, _startMicroSec);

    if (_type == 0) {
        long deltaUs = abs(_targetMicroSec - _startMicroSec);
        long spanUs = abs(_maxUs - _minUs);
        _moveDurationMs = (spanUs > 0) ? (deltaUs * _transitionTimeMs) / spanUs : 0;
    } else {
        bool isRoller = (getMode() == MODE_ROLLER_SHUTTER);
        if (isRoller && _targetBrightness != 127) { 
            float ratio = abs((int)_targetBrightness - (int)_currentBrightness) / 255.0;
            _moveDurationMs = (unsigned long)(_transitionTimeMs * ratio);
        } else {
            _moveDurationMs = 0; 
        }
    }
    SUPLA_LOG_DEBUG("[MATH %d] Wyliczony czas ruchu: %lu ms", _servoIndex, _moveDurationMs);

    _moveStartTimeMs = millis();
    _isMoving = true;
    _isWaitingToDetach = false;

    if (!_isAttached) {
        _servo.attach(_gpio, _minUs, _maxUs);
        _isAttached = true;
        SUPLA_LOG_DEBUG("[SERVO %d] PWM Podpiete (Attach) Pin: %d", _servoIndex, _gpio);
    }
    
    sendAckToCloud(_targetBrightness);
}

void SuplaServoDimmer::iterateAlways() {
    if (!_isAttached) return;

    limitSwitchCheck360();

    unsigned long now = millis();
    if (now - _lastIterateTime < 20) return; 
    _lastIterateTime = now;
    
    bool reached = false;

    if (_isMoving) {
        if (_type == 0) { 
            unsigned long elapsed = now - _moveStartTimeMs;
            if (elapsed < _moveDurationMs) {
                _currentMicroSec = map(elapsed, 0, _moveDurationMs, _startMicroSec, _targetMicroSec);
            } else {
                _currentMicroSec = _targetMicroSec;
                reached = true;
            }
        } else { 
            _currentMicroSec = _targetMicroSec;
            bool isRoller = (getMode() == MODE_ROLLER_SHUTTER);
            if (isRoller && _targetMicroSec != _midUs) {
                if (now - _moveStartTimeMs >= _moveDurationMs) {
                    forceStopAndSync(127);
                    reached = true;
                }
            } else if (_targetMicroSec == _midUs) {
                reached = true;
            }
        }

        if (_currentMicroSec != _lastWrittenMicroSec) {
            _servo.writeMicroseconds(_currentMicroSec);
            _lastWrittenMicroSec = _currentMicroSec;
        }

        // Telemetria: zrzucamy pozycje co 500ms
        if (now - _lastLogTime >= 500) {
            SUPLA_LOG_DEBUG("[MOVE %d] Pozycja: %d us | Cel: %d us", _servoIndex, _currentMicroSec, _targetMicroSec);
            _lastLogTime = now;
        }
    }

    if (reached) {
        _isMoving = false;
        if (!_isWaitingToDetach) {
            SUPLA_LOG_DEBUG("[MOVE %d] Cel osiagniety (%d us). Rozpoczecie odliczania odciecia.", _servoIndex, _currentMicroSec);
            _targetReachedTime = now;
            _isWaitingToDetach = true;
        }
    }

    if (_isWaitingToDetach && _detachDelayMs > 0) {
        if (now - _targetReachedTime >= _detachDelayMs) {
            if (_type == 0 || (_type == 1 && _currentMicroSec == _midUs)) {
                detachServoSafe();
            }
            _isWaitingToDetach = false;
        }
    }
}
#endif // SUPLA_SERVO_CUSTOM