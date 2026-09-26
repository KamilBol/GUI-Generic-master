#ifndef SUPLA_SERVO_DIMMER_H
#define SUPLA_SERVO_DIMMER_H

#if defined(SUPLA_SERVO_CUSTOM)

#include <supla/control/dimmer_base.h>
#include <supla/action_handler.h>
#include <Servo.h>

enum ServoMode {
    MODE_DIMMER,
    MODE_RELAY,
    MODE_ROLLER_SHUTTER
};

class SuplaServoDimmer : public Supla::Control::DimmerBase {
public:
    SuplaServoDimmer(int servoIndex);
    
    void onInit() override;
    void setRGBWValueOnDevice(uint32_t red, uint32_t green, uint32_t blue, uint32_t colorBrightness, uint32_t brightness) override;
    void iterateAlways() override;
    void handleAction(int event, int action) override;

private:
    int _servoIndex;
    uint8_t _gpio;
    uint8_t _type; // 0: 180 st., 1: 360 st.
    uint8_t _chType; 
    bool _inverted;
    
    // Zmienne Sprzętowe
    int _minUs;
    int _midUs; // Uzywane tez jako STOP dla 360
    int _maxUs;
    unsigned long _transitionTimeMs;
    unsigned long _detachDelayMs;
    int _deadbandPercent;
    
    // Krancowki
    uint8_t _lUpGpio;
    bool _lUpState;
    uint8_t _lDownGpio;
    bool _lDownState;

    Servo _servo;
    
    // Flagi Stanow
    bool _isMoving;
    bool _isAttached;
    bool _firstCloudSync;
    bool _isWaitingToDetach;
    bool _isUp;

    uint32_t _currentBrightness;
    uint32_t _targetBrightness;
    
    int _currentMicroSec;
    int _targetMicroSec;
    int _lastWrittenMicroSec;
    
    // Timery i Interpolacja (millis)
    unsigned long _moveStartTimeMs;
    unsigned long _moveDurationMs;
    int _startMicroSec;
    unsigned long _targetReachedTime;
    unsigned long _lastIterateTime;
    unsigned long _lastLogTime; // Telemetria: dlawik logow

    ServoMode getMode() const;
    int calculateMicroSec(uint32_t brightness);
    uint32_t calculateBrightnessFromMicroSec(int microSec);
    void detachServoSafe();
    void sendAckToCloud(uint32_t currentPercentage);
    void limitSwitchCheck360();
    void forceStopAndSync(uint8_t targetBrightness);
};

#endif // SUPLA_SERVO_CUSTOM
#endif // SUPLA_SERVO_DIMMER_H