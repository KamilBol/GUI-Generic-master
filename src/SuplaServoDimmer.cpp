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

#define B_ACTION_TURN_ON 1
#define B_ACTION_TURN_OFF 2
#define B_ACTION_TOGGLE 3
#define B_ACTION_REVEAL 10
#define B_ACTION_SHUT 11
#define B_ACTION_STOP 12
#define B_ACTION_OPEN 20
#define B_ACTION_CLOSE 21

SuplaServoDimmer::SuplaServoDimmer(int servoIndex) : _servoIndex(servoIndex) {
    setDefaultStateRestore();
    setFadeEffectTime(0); 
    
    _isMoving = false;
    _isAttached = false;
    _currentBrightness = 0;
    _targetBrightness = 0;
    _lastIterateTime = 0;
    _currentMicroSec = 1500;
    _targetMicroSec = 1500;
    _lastWrittenMicroSec = 0;
    _isTimeLimitedRun = false;
    _isUp = true;
    _firstCloudSync = true; // Aktywacja pancerza restartowego

    _type = ConfigManager->get(KEY_SERVO_TYPE)->getElement(_servoIndex).toInt();
    _chType = ConfigManager->get(KEY_SERVO_CH_TYPE)->getElement(_servoIndex).toInt();
    
    if (_type == 0) { 
        if (_chType == 2) {
            getChannel()->setType(SUPLA_CHANNELTYPE_RELAY);
            getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);
        } else if (_chType == 0) {
            getChannel()->setType(SUPLA_CHANNELTYPE_DIMMER);
            getChannel()->setDefault(SUPLA_CHANNELFNC_CONTROLLINGTHEROLLERSHUTTER);
        } else {
            getChannel()->setType(SUPLA_CHANNELTYPE_DIMMER);
            getChannel()->setDefault(SUPLA_CHANNELFNC_DIMMER);
        }
    } else { 
        if (_chType == 0) {
            getChannel()->setType(SUPLA_CHANNELTYPE_RELAY);
            getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);
        } else {
            getChannel()->setType(SUPLA_CHANNELTYPE_DIMMER);
            getChannel()->setDefault(SUPLA_CHANNELFNC_CONTROLLINGTHEROLLERSHUTTER);
        }
    }
}

void SuplaServoDimmer::onInit() {
    Supla::Control::DimmerBase::onInit(); 

    _gpio = ConfigESP->getGpio(_servoIndex, FUNCTION_SERVO);

    _inverted = ConfigManager->get(KEY_SERVO_INVERTED)->getElement(_servoIndex).toInt() == 1;
    _zero = ConfigManager->get(KEY_SERVO_ZERO)->getElement(_servoIndex).toInt();
    _maxAngle = ConfigManager->get(KEY_SERVO_MAX_ANGLE)->getElement(_servoIndex).toInt();
    _speed = ConfigManager->get(KEY_SERVO_SPEED)->getElement(_servoIndex).toInt();
    _softStart = ConfigManager->get(KEY_SERVO_SOFT_START)->getElement(_servoIndex).toInt();
    _offPWM = ConfigManager->get(KEY_SERVO_OFF_PWM)->getElement(_servoIndex).toInt() == 1;
    
    _stopUs = ConfigManager->get(KEY_SERVO_STOP_US)->getElement(_servoIndex).toInt();
    _dir = ConfigManager->get(KEY_SERVO_DIR)->getElement(_servoIndex).toInt();
    
    _timeUpMs = (unsigned long)(ConfigManager->get(KEY_SERVO_TIME)->getElement(_servoIndex).toFloat() * 1000);
    _timeDownMs = (unsigned long)(ConfigManager->get(KEY_SERVO_TIME_DOWN)->getElement(_servoIndex).toFloat() * 1000);
    if (_timeUpMs == 0) _timeUpMs = 60000; 
    if (_timeDownMs == 0) _timeDownMs = 60000;

    _speedDown = ConfigManager->get(KEY_SERVO_SPEED_DOWN)->getElement(_servoIndex).toInt();
    _overdriveMs = (unsigned long)(ConfigManager->get(KEY_SERVO_OVERDRIVE)->getElement(_servoIndex).toFloat() * 1000);
    
    _lUpGpio = ConfigESP->getGpio(_servoIndex, FUNCTION_LIMIT_SWITCH);
    _lUpState = ConfigManager->get(KEY_SERVO_LIMIT_UP_STATE)->getElement(_servoIndex).toInt() == 1;
    _lDownGpio = ConfigESP->getGpio(_servoIndex + 5, FUNCTION_LIMIT_SWITCH);
    _lDownState = ConfigManager->get(KEY_SERVO_LIMIT_DOWN_STATE)->getElement(_servoIndex).toInt() == 1;

    if (_lUpGpio != OFF_GPIO) {
        bool pullUp = ConfigESP->getPullUp(_lUpGpio);
        pinMode(_lUpGpio, pullUp ? INPUT_PULLUP : INPUT);
    }
    if (_lDownGpio != OFF_GPIO) {
        bool pullUp = ConfigESP->getPullUp(_lDownGpio);
        pinMode(_lDownGpio, pullUp ? INPUT_PULLUP : INPUT);
    }

    if (_gpio != OFF_GPIO && _gpio != 0) {
        if (getChannel()) {
            _currentBrightness = constrain(getChannel()->getValueBrightness(), 0, 255);
        }
        _targetBrightness = _currentBrightness;

        if (_type == 0) { 
            _currentMicroSec = calculateMicroSec180(_currentBrightness);
            _targetMicroSec = _currentMicroSec;
        } else { 
            _currentMicroSec = _stopUs;
            _targetMicroSec = _stopUs;
        }
        
        // ZABEZPIECZENIE (Rozwiązane szaleństwo po resecie):
        // Całkowicie wyrzucone inicjowanie attach(). Po resecie serwo śpi martwym bykiem.
    }
}

void SuplaServoDimmer::handleAction(int event, int action) {
    if (_gpio == OFF_GPIO || _gpio == 0) return;

    uint8_t parkingBrightness = map(constrain(_zero, 0, 100), 0, 100, 0, 255);
    uint8_t activeBrightness = (_zero <= 50) ? 255 : 0;

    if (action == B_ACTION_TURN_ON) {
        setRGBWValueOnDevice(0, 0, 0, 0, activeBrightness);
        return;
    } else if (action == B_ACTION_TURN_OFF) {
        setRGBWValueOnDevice(0, 0, 0, 0, parkingBrightness);
        return;
    } else if (action == B_ACTION_TOGGLE) {
        if (abs((int)_currentBrightness - (int)parkingBrightness) < 5) {
            setRGBWValueOnDevice(0, 0, 0, 0, activeBrightness);
        } else {
            setRGBWValueOnDevice(0, 0, 0, 0, parkingBrightness);
        }
        return;
    } else if (action == B_ACTION_OPEN || action == B_ACTION_REVEAL) {
        setRGBWValueOnDevice(0, 0, 0, 0, 255); 
        return;
    } else if (action == B_ACTION_CLOSE || action == B_ACTION_SHUT) {
        setRGBWValueOnDevice(0, 0, 0, 0, 0); 
        return;
    } else if (action == B_ACTION_STOP) {
        if (_isMoving && _type == 1) { 
            unsigned long elapsed = millis() - _motorStartTimeMs;
            if (_motorTargetDurationMs > 0) {
                float movedRatio = (float)elapsed / (float)_motorTargetDurationMs;
                if (movedRatio > 1.0) movedRatio = 1.0;
                int brightnessMoved = abs((int)_targetBrightness - (int)_currentBrightness) * movedRatio;
                if (_isUp) {
                    _currentBrightness += brightnessMoved;
                    if (_currentBrightness > 255) _currentBrightness = 255;
                } else {
                    if (_currentBrightness > (uint32_t)brightnessMoved) _currentBrightness -= brightnessMoved;
                    else _currentBrightness = 0;
                }
            }
            _servo.writeMicroseconds(_stopUs);
            if (_offPWM) { delay(50); detachServoSafe(); }
        }
        _isMoving = false;
        _targetBrightness = _currentBrightness;
        sendHardAck(_currentBrightness);
        return;
    }

    Supla::Control::DimmerBase::handleAction(event, action);
}

int SuplaServoDimmer::calculateMicroSec180(uint32_t brightness) {
    uint32_t safeBrightness = constrain(brightness, 0, 255);
    if (_inverted) { safeBrightness = 255 - safeBrightness; }
    
    int clampedAngle = constrain(_maxAngle, 0, 180);
    int clampedZero = constrain(_zero, 0, 100);
    
    // Rozkładanie na cały suwak: 1 stopień = 10us
    long centerUs = map(clampedZero, 0, 100, 544, 2400); 
    long minUs = constrain(centerUs - (clampedAngle * 10), 544, 2400);
    long maxUs = constrain(centerUs + (clampedAngle * 10), 544, 2400);
    
    return map(safeBrightness, 0, 255, minUs, maxUs);
}

void SuplaServoDimmer::detachServoSafe() {
    if (_isAttached) {
        _servo.detach();
        _isAttached = false;
        _lastWrittenMicroSec = 0; 
        SUPLA_LOG_DEBUG("[SERVO %d] PWM odciete", _servoIndex);
    }
}

void SuplaServoDimmer::sendHardAck(uint32_t brightness) {
    if (getChannel()) {
        uint8_t valueToSend = static_cast<uint8_t>(constrain(brightness, 0, 255));
        
        // POPRAWKA JEDNOSTEK: Ściemniacz vs Roleta (Skoki %)
        bool isRollerShutter = (_type == 0 && _chType == 0) || (_type == 1 && _chType == 1);
        if (isRollerShutter) {
            valueToSend = map(brightness, 0, 255, 0, 100);
        }

        getChannel()->setNewValue(0, 0, 0, 0, valueToSend);
    }
}

void SuplaServoDimmer::setRGBWValueOnDevice(uint32_t red, uint32_t green, uint32_t blue, uint32_t colorBrightness, uint32_t brightness) {
    // BLOKADA: Ignorujemy pierwszy fałszywy skok wymuszony z chmury Supli przy resecie prądowym!
    if (_firstCloudSync) {
        _currentBrightness = constrain(brightness, 0, 255);
        _targetMicroSec = (_type == 0) ? calculateMicroSec180(_currentBrightness) : _stopUs;
        _currentMicroSec = _targetMicroSec;
        _firstCloudSync = false;
        SUPLA_LOG_DEBUG("[SERVO %d] Zsynchronizowano z chmura po resecie. Czekam w ciszy.", _servoIndex);
        return; 
    }

    if (_gpio == OFF_GPIO || _gpio == 0) return;
    
    uint32_t safeBrightness = constrain(brightness, 0, 255);
    _isUp = (safeBrightness > _currentBrightness);
    _targetBrightness = safeBrightness;

    if (_type == 0) {
        _targetMicroSec = calculateMicroSec180(_targetBrightness);
        SUPLA_LOG_DEBUG("[SERVO %d 180] Cel: %d us. Suwak jasnosci (0-255): Z %d -> DO %d", _servoIndex, _targetMicroSec, _currentBrightness, _targetBrightness);
    } else {
        if (_chType == 0) {
            _motorTargetDurationMs = _timeUpMs; 
            _isTimeLimitedRun = (_motorTargetDurationMs > 0);
        } else {
            float diff = abs((int)_targetBrightness - (int)_currentBrightness);
            float ratio = diff / 255.0;
            if (_isUp) {
                _motorTargetDurationMs = (unsigned long)(_timeUpMs * ratio);
            } else {
                _motorTargetDurationMs = (unsigned long)(_timeDownMs * ratio);
            }
            _isTimeLimitedRun = true;
            SUPLA_LOG_DEBUG("[SERVO %d 360] Cel (%d%%). Czas: %ld ms", _servoIndex, (int)(ratio*100), _motorTargetDurationMs);
        }
    }

    _motorStartTimeMs = millis();
    
    if (!_isAttached) {
        _servo.writeMicroseconds(_currentMicroSec);
        _servo.attach(_gpio);
        _isAttached = true;
    }
    _isMoving = true;
    
    sendHardAck(_targetBrightness);
}

void SuplaServoDimmer::limitSwitchCheck360() {
    if (_lDownGpio != OFF_GPIO) {
        bool limitTriggered = (digitalRead(_lDownGpio) == _lDownState);
        if (limitTriggered && !_isUp) { 
            SUPLA_LOG_DEBUG("[SERVO %d 360] KRANCOWKA DOLNA!", _servoIndex);
            _motorTargetDurationMs = 0; 
            _currentBrightness = 0;
            _targetBrightness = 0;
            detachServoSafe();
            _isMoving = false;
            _isTimeLimitedRun = false;
            sendHardAck(0); 
            return;
        }
    }
    if (_lUpGpio != OFF_GPIO) {
        bool limitTriggered = (digitalRead(_lUpGpio) == _lUpState);
        if (limitTriggered && _isUp) { 
            SUPLA_LOG_DEBUG("[SERVO %d 360] KRANCOWKA GORNA!", _servoIndex);
            _motorTargetDurationMs = 0; 
            _currentBrightness = 255;
            _targetBrightness = 255;
            detachServoSafe();
            _isMoving = false;
            _isTimeLimitedRun = false;
            sendHardAck(255); 
            return;
        }
    }
}

void SuplaServoDimmer::iterateAlways() {
    if (!_isMoving || !_isAttached) return;

    unsigned long currentTime = millis();
    if (currentTime - _lastIterateTime < 20) return; 
    _lastIterateTime = currentTime;

    if (_type == 0) {
        if (_currentMicroSec == _targetMicroSec) {
            _isMoving = false;
            _currentBrightness = _targetBrightness;
            sendHardAck(_currentBrightness); 
            if (_offPWM) detachServoSafe();
        } else {
            int step = map(_speed, 0, 100, 1, 30); 
            if (_softStart > 0 && abs(_currentMicroSec - _targetMicroSec) < _softStart * 10) {
                step = (step > 1) ? step / 2 : 1; 
            }

            if (_currentMicroSec < _targetMicroSec) {
                _currentMicroSec += step;
                if (_currentMicroSec > _targetMicroSec) _currentMicroSec = _targetMicroSec; 
            } else {
                _currentMicroSec -= step;
                if (_currentMicroSec < _targetMicroSec) _currentMicroSec = _targetMicroSec; 
            }
            
            if (_currentMicroSec != _lastWrittenMicroSec) {
                _servo.writeMicroseconds(_currentMicroSec);
                _lastWrittenMicroSec = _currentMicroSec;
            }
        }
    } else {
        limitSwitchCheck360();
        if (!_isMoving) return; 

        unsigned long elapsed = currentTime - _motorStartTimeMs;
        
        if (_isTimeLimitedRun && elapsed >= (_motorTargetDurationMs + _overdriveMs)) {
            _servo.writeMicroseconds(_stopUs);
            _isMoving = false;
            _currentBrightness = _targetBrightness;
            if (_offPWM) { delay(50); detachServoSafe(); } 
            sendHardAck(_currentBrightness); 
            return;
        }

        // TARCIE STATYCZNE (DEADBAND) - Rozwiązanie problemu "Buczenia bez ruchu"
        int deadband = 45; // Moc minimalna by przełamać opór przekładni

        if (_chType == 0) { 
            if (_targetBrightness > 0) {
                int motorPwm = map(_speed, 0, 100, deadband, 500); // Wskakuje od razu na próg użyteczny
                int outPwm = (_dir == 0) ? _stopUs - motorPwm : _stopUs + motorPwm;
                if (outPwm != _lastWrittenMicroSec) {
                    _servo.writeMicroseconds(outPwm);
                    _lastWrittenMicroSec = outPwm;
                }
            } else {
                _servo.writeMicroseconds(_stopUs);
                _isMoving = false;
                _currentBrightness = 0;
                if (_offPWM) { delay(50); detachServoSafe(); } 
                sendHardAck(0);
            }
        } else { 
            if (_targetBrightness == _currentBrightness) {
                _servo.writeMicroseconds(_stopUs);
                _isMoving = false;
                if (_offPWM) { delay(50); detachServoSafe(); } 
                return;
            }
            
            int activeSpeed = _isUp ? _speed : _speedDown;
            int motorPwm = map(activeSpeed, 0, 100, deadband, 500); // Stiction bypass
            
            if (_softStart > 0) {
                unsigned long softStartMs = _softStart * 1000UL;
                if (elapsed < softStartMs) {
                    motorPwm = map(elapsed, 0, softStartMs, deadband, motorPwm); 
                } else if (_isTimeLimitedRun && elapsed > _motorTargetDurationMs - softStartMs) {
                    motorPwm = map(_motorTargetDurationMs - elapsed, 0, softStartMs, deadband, motorPwm); 
                }
            }

            bool driveLeft = _isUp ? (_dir == 0) : (_dir == 1);
            int outPwm = driveLeft ? _stopUs - motorPwm : _stopUs + motorPwm;
            
            if (outPwm != _lastWrittenMicroSec) {
                _servo.writeMicroseconds(outPwm);
                _lastWrittenMicroSec = outPwm;
            }
        }
    }
}

#endif // SUPLA_SERVO_CUSTOM