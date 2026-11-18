#pragma once

#include <AccelStepper.h>
#include <Arduino.h>
#include <Preferences.h>
#include <TMCStepper.h>

#include <atomic>

#include "BlindMath.h"
#include <climits>

// Percent-open blind driven by AccelStepper, with the TMC2209 configured over
// UART by TMCStepper. If the UART link fails, STEP/DIR still works.
//
// Steps:   0 = fully open, travelSteps = fully closed.
// Percent: 100 = fully open, 0 = fully closed.
//
// The request*() methods can be called from other tasks (the Zigbee
// callbacks). They only record the request; update() acts on it from the
// loop task, so AccelStepper is only touched by one task.
class BlindMotor {
 public:
  struct Config {
    uint8_t stepPin;
    uint8_t dirPin;
    uint8_t enablePin;  // active low
    int32_t travelSteps;
    float maxSpeed;
    float acceleration;
    bool dirInverted;
    bool holdWhenIdle;
    const char* nvsNamespace;

    // TMC2209 over UART
    bool useUart;
    HardwareSerial* uart;
    uint8_t uartRx;
    uint8_t uartTx;
    uint32_t uartBaud;
    uint8_t tmcAddress;
    float rSense;
    uint16_t runCurrentMa;
    uint16_t microsteps;
  };

  explicit BlindMotor(const Config& config);

  void begin();
  void update();  // call every loop iteration, never block between calls

  // Safe from any task
  void requestOpenPercent(uint8_t openPercent);
  void requestOpen() { requestOpenPercent(100); }
  void requestClose() { requestOpenPercent(0); }
  void requestStop() { _stop = true; }

  // Safe from any task. update() applies the new value and saves it, so it
  // survives a reboot and overrides the defaults from the Config.
  void setMaxSpeed(float stepsPerSecond);
  void setAcceleration(float stepsPerSecondSquared);
  float maxSpeed() const { return _speed; }
  float acceleration() const { return _accel; }

  // Loop task only
  uint8_t openPercent();
  bool isMoving() const { return _wasMoving; }

 private:
  static constexpr int32_t kNoRequest = INT32_MIN;

  void setupTmc();
  void applyMotion();
  void finishMove();

  Config _cfg;
  AccelStepper _stepper;
  TMC2209Stepper _tmc;
  Preferences _prefs;
  std::atomic<int32_t> _target{kNoRequest};
  std::atomic<bool> _stop{false};
  std::atomic<float> _speed{0.0f};
  std::atomic<float> _accel{0.0f};
  std::atomic<bool> _motionChanged{false};
  bool _wasMoving = false;
};