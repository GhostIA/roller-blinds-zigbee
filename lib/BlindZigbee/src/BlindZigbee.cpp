#include "BlindZigbee.h"

namespace {

BlindMotor* gMotor = nullptr;
const BlindZigbee::Config* gCfg = nullptr;

void onSpeedChange(float stepsPerSecond) {
  if (gMotor && gCfg) {
    gMotor->setMaxSpeed(constrain(stepsPerSecond, gCfg->speedMin, gCfg->speedMax));
  }
}

void onAccelChange(float stepsPerSecondSquared) {
  if (gMotor && gCfg) {
    gMotor->setAcceleration(
        constrain(stepsPerSecondSquared, gCfg->accelMin, gCfg->accelMax));
  }
}

void onOpen() {
  if (gMotor) gMotor->requestOpen();
}

void onClose() {
  if (gMotor) gMotor->requestClose();
}

void onGoToLiftPercentage(uint8_t openPercent) {
  if (gMotor) gMotor->requestOpenPercent(openPercent);
}

void onStop() {
  if (gMotor) gMotor->requestStop();
}

}  // namespace

BlindZigbee::BlindZigbee(BlindMotor& motor, const Config& config)
    : _motor(motor),
      _cfg(config),
      _cover(config.endpoint),
      _speed(config.speedEndpoint),
      _accel(config.accelEndpoint) {}

bool BlindZigbee::begin() {
  gMotor = &_motor;
  gCfg = &_cfg;
  pinMode(_cfg.factoryResetPin, INPUT_PULLUP);

  _cover.setManufacturerAndModel(_cfg.manufacturer, _cfg.model);
  _cover.setCoveringType(ROLLERSHADE);
  // operational, online, not reversed, open-loop lift/tilt (no encoder)
  _cover.setConfigStatus(true, true, false, false, false, false, false);
  _cover.setMode(false, false, false, false);
  // Lift is reported as a percentage, no tilt on a roller blind
  _cover.setLimits(0, 100, 0, 0);

  _cover.onOpen(onOpen);
  _cover.onClose(onClose);
  _cover.onGoToLiftPercentage(onGoToLiftPercentage);
  _cover.onStop(onStop);

  // Speed and acceleration as analog outputs
  _speed.setManufacturerAndModel(_cfg.manufacturer, _cfg.model);
  _speed.addAnalogOutput();
  _speed.setAnalogOutputDescription("Blind speed (steps/s)");
  _speed.setAnalogOutputMinMax(_cfg.speedMin, _cfg.speedMax);
  _speed.setAnalogOutputResolution(1.0f);
  _speed.onAnalogOutputChange(onSpeedChange);

  _accel.setManufacturerAndModel(_cfg.manufacturer, _cfg.model);
  _accel.addAnalogOutput();
  _accel.setAnalogOutputDescription("Blind acceleration (steps/s^2)");
  _accel.setAnalogOutputMinMax(_cfg.accelMin, _cfg.accelMax);
  _accel.setAnalogOutputResolution(1.0f);
  _accel.onAnalogOutputChange(onAccelChange);

  Zigbee.addEndpoint(&_cover);
  Zigbee.addEndpoint(&_speed);
  Zigbee.addEndpoint(&_accel);
  return Zigbee.begin();  // end device by default
}

void BlindZigbee::update() {
  pollFactoryReset();
  if (Zigbee.connected()) {
    if (!_motionPublished) publishMotion();
    reportPosition();
  }
}

// Tell HA the current values once the stack is up, so the numbers show what
// is actually saved on the device (not the endpoint's blank default).
void BlindZigbee::publishMotion() {
  _speed.setAnalogOutput(_motor.maxSpeed());
  _accel.setAnalogOutput(_motor.acceleration());
  _motionPublished = true;
}

void BlindZigbee::reportPosition() {
  const uint8_t percent = _motor.openPercent();
  if ((int16_t)percent == _lastReportedPercent) return;

  const uint32_t now = millis();
  const bool firstReport = _lastReportedPercent < 0;
  if (!firstReport && now - _lastReportMs < _cfg.reportIntervalMs) return;

  _cover.setLiftPercentage(percent);
  _lastReportedPercent = percent;
  _lastReportMs = now;
}

void BlindZigbee::pollFactoryReset() {
  if (digitalRead(_cfg.factoryResetPin) != LOW) {
    _resetPressed = false;
    return;
  }

  const uint32_t now = millis();
  if (!_resetPressed) {
    _resetPressed = true;
    _resetPressStartMs = now;
    return;
  }

  if (now - _resetPressStartMs >= _cfg.factoryResetHoldMs) {
    Serial.println("[zigbee] factory reset, rebooting");
    Zigbee.factoryReset();
    _resetPressed = false;
  }
}