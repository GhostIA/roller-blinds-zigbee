#include "BlindMotor.h"

BlindMotor::BlindMotor(const Config& config)
    : _cfg(config),
      _stepper(AccelStepper::DRIVER, config.stepPin, config.dirPin),
      _tmc(config.uart, config.rSense, config.tmcAddress) {}

void BlindMotor::begin() {
  _stepper.setPinsInverted(_cfg.dirInverted, false, true);  // EN is active low
  _stepper.setEnablePin(_cfg.enablePin);
  _stepper.disableOutputs();

  // Speed and acceleration: the saved values win over the Config defaults.
  _prefs.begin(_cfg.nvsNamespace, false);
  _speed = _prefs.getFloat("speed", _cfg.maxSpeed);
  _accel = _prefs.getFloat("accel", _cfg.acceleration);
  _stepper.setMaxSpeed(_speed);
  _stepper.setAcceleration(_accel);

  if (_cfg.useUart) {
    _cfg.uart->begin(_cfg.uartBaud, SERIAL_8N1, _cfg.uartRx, _cfg.uartTx);
    setupTmc();
  }

  // Restore the last saved position.
  _stepper.setCurrentPosition(
      constrain(_prefs.getInt("pos", 0), 0, _cfg.travelSteps));
  Serial.printf("[motor] %u%% open\n", (unsigned)openPercent());
}

// Safe to call repeatedly. The driver forgets its settings whenever its motor
// supply drops out, so this also runs before every move.
void BlindMotor::setupTmc() {
  _tmc.begin();

  if (_tmc.test_connection() != 0) {
    Serial.println(
        "[tmc] no UART reply, using STEP/DIR only (pot current, MS1/MS2 "
        "microsteps)");
    return;
  }

  _tmc.I_scale_analog(false);  // take current from UART, not the pot
  _tmc.rms_current(_cfg.runCurrentMa);
  _tmc.microsteps(_cfg.microsteps);
  Serial.println("[tmc] configured over UART");
}

void BlindMotor::update() {
  if (_motionChanged.exchange(false)) applyMotion();

  if (_stop.exchange(false)) {
    _stepper.stop();  // decelerates
  }

  const int32_t target = _target.exchange(kNoRequest);
  if (target != kNoRequest) {
    _stepper.moveTo(constrain(target, 0, _cfg.travelSteps));
    if (_stepper.distanceToGo() != 0) {
      if (_cfg.useUart) setupTmc();  // motor power may have come up after boot
      _stepper.enableOutputs();
    }
  }

  _stepper.run();

  const bool moving = _stepper.distanceToGo() != 0;
  if (_wasMoving && !moving) finishMove();
  _wasMoving = moving;
}

void BlindMotor::requestOpenPercent(uint8_t openPercent) {
  _target = blindmath::percentToSteps(openPercent, _cfg.travelSteps);
}

void BlindMotor::setMaxSpeed(float stepsPerSecond) {
  if (stepsPerSecond <= 0.0f) return;
  _speed = stepsPerSecond;
  _motionChanged = true;
}

void BlindMotor::setAcceleration(float stepsPerSecondSquared) {
  if (stepsPerSecondSquared <= 0.0f) return;
  _accel = stepsPerSecondSquared;
  _motionChanged = true;
}

// Safe mid-move: AccelStepper recomputes its ramp from the new values.
void BlindMotor::applyMotion() {
  const float speed = _speed;
  const float accel = _accel;
  _stepper.setMaxSpeed(speed);
  _stepper.setAcceleration(accel);
  _prefs.putFloat("speed", speed);
  _prefs.putFloat("accel", accel);
  Serial.printf("[motor] speed %.0f steps/s, acceleration %.0f steps/s^2\n",
                speed, accel);
}

uint8_t BlindMotor::openPercent() {
  return blindmath::stepsToPercent(_stepper.currentPosition(), _cfg.travelSteps);
}

void BlindMotor::finishMove() {
  _prefs.putInt("pos", _stepper.currentPosition());  // flash write once per move
  if (!_cfg.holdWhenIdle) _stepper.disableOutputs();
  Serial.printf("[motor] stopped at %u%% open\n", (unsigned)openPercent());
}