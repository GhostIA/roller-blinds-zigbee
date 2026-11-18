#pragma once

#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is off. Build with -DZIGBEE_MODE_ED and the zigbee.csv partition table (see platformio.ini)."
#endif

#include <Arduino.h>

#include "BlindMotor.h"
#include "ZigbeeCore.h"
#include "ep/ZigbeeAnalog.h"
#include "ep/ZigbeeWindowCovering.h"

// Exposes a BlindMotor as a Zigbee Home Automation window covering, plus two
// analog outputs (shown as number entities in ZHA) for speed and acceleration.
// Only one instance may exist: the Zigbee callbacks are plain function
// pointers, so they reach the motor through file-level pointers.
class BlindZigbee {
 public:
  struct Config {
    uint8_t endpoint;
    const char* manufacturer;
    const char* model;
    uint8_t factoryResetPin;
    uint32_t factoryResetHoldMs;
    uint32_t reportIntervalMs;

    // Speed / acceleration numbers. Values from HA are clamped to the limits.
    uint8_t speedEndpoint;
    uint8_t accelEndpoint;
    float speedMin;
    float speedMax;
    float accelMin;
    float accelMax;
  };

  BlindZigbee(BlindMotor& motor, const Config& config);

  bool begin();   // registers the endpoint and starts the stack, does not block
  void update();  // call every loop iteration

 private:
  void pollFactoryReset();
  void reportPosition();
  void publishMotion();

  BlindMotor& _motor;
  Config _cfg;
  ZigbeeWindowCovering _cover;
  ZigbeeAnalog _speed;
  ZigbeeAnalog _accel;

  bool _motionPublished = false;
  int16_t _lastReportedPercent = -1;
  uint32_t _lastReportMs = 0;
  bool _resetPressed = false;
  uint32_t _resetPressStartMs = 0;
};