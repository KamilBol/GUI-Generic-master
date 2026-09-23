#ifndef SUPLA_SERVO_DIMMER_H
#define SUPLA_SERVO_DIMMER_H

#if defined(SUPLA_SERVO_CUSTOM)

#include <supla/control/dimmer_base.h>
#include <supla/action_handler.h>
#include <Servo.h>

class SuplaServoDimmer : public Supla::Control::DimmerBase {
public:
    SuplaServoDimmer(int servoIndex);
    
    void onInit() override;
    void setRGBWValueOnDevice(uint32_t red, uint32_t green, uint32_t blue, uint32_t colorBrightness, uint32_t brightness) override;
    void iterateAlways() override;
    void handleAction(int event, int action) override;

private:
    int _servoIndex;
    
    // Konfiguracja sprzętowa
    uint8_t _gpio;
    uint8_t _type; 
    uint8_t _chType; 
    bool _inverted;
    int _zero; 
    int _maxAngle; 
    int _speed; 
    int _speedDown;
    int _softStart; 
    bool _offPWM; 
    int _stopUs; 
    int _dir; 
    unsigned long _timeUpMs; 
    unsigned long _timeDownMs; 
    unsigned long _overdriveMs; 
    
    // Krańcówki
    uint8_t _lUpGpio;
    bool _lUpState;
    uint8_t _lDownGpio;
    bool _lDownState;

    Servo _servo;
    
    // Flagi operacyjne i bezpieczeństwo
    bool _isMoving;
    bool _isAttached;
    unsigned long _lastIterateTime;
    bool _isUp; 
    bool _firstCloudSync; // Zabezpieczenie przed gubieniem pamięci i szaleństwem po restarcie
    
    // Wewnętrzna reprezentacja położenia
    uint32_t _currentBrightness; 
    uint32_t _targetBrightness;  
    
    // Stan sprzętowy
    int _currentMicroSec;
    int _targetMicroSec;
    int _lastWrittenMicroSec; 
    
    // Maszyna Stanów i Timery
    unsigned long _motorStartTimeMs;
    unsigned long _motorTargetDurationMs;
    bool _isTimeLimitedRun;

    // Funkcje narzędziowe
    int calculateMicroSec180(uint32_t brightness);
    void detachServoSafe();
    void sendHardAck(uint32_t brightness);
    void limitSwitchCheck360();
};

#endif // SUPLA_SERVO_CUSTOM
#endif // SUPLA_SERVO_DIMMER_H