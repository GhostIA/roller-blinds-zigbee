#include <Arduino.h>

#include "BlindMotor.h"
#include "BlindZigbee.h"
#include "StatusLed.h"
#include "config.h"

namespace {

// Field order matches the Config structs in each library.
BlindMotor motor(BlindMotor::Config{
    cfg::kPinStep,
    cfg::kPinDir,
    cfg::kPinEnable,
    cfg::kTravelSteps,
    cfg::kMaxSpeed,
    cfg::kAcceleration,
    cfg::kDirInverted,
    cfg::kHoldWhenIdle,
    cfg::kNvsNamespace,
    cfg::kUseTmcUart,
    &Serial1,
    cfg::kPinUartRx,
    cfg::kPinUartTx,
    cfg::kUartBaud,
    cfg::kTmcAddress,
    cfg::kRSense,
    cfg::kRunCurrentMa,
    cfg::kMicrosteps,
});

BlindZigbee zigbee(motor, BlindZigbee::Config{
                              cfg::kEndpoint,
                              cfg::kManufacturer,
                              cfg::kModel,
                              cfg::kFactoryResetPin,
                              cfg::kFactoryResetHoldMs,
                              cfg::kReportIntervalMs,
                              cfg::kSpeedEndpoint,
                              cfg::kAccelEndpoint,
                              cfg::kSpeedMin,
                              cfg::kSpeedMax,
                              cfg::kAccelMin,
                              cfg::kAccelMax,
                          });

StatusLed led(StatusLed::Config{
    cfg::kPinNeoPixel,
    cfg::kPinNeoPixelPower,
    cfg::kLedBrightness,
});

}  // namespace

void setup() {
  Serial.begin(115200);

  led.begin();
  motor.begin();

  if (!zigbee.begin()) {
    Serial.println("[zigbee] failed to start, rebooting");
    delay(1000);
    ESP.restart();
  }
}

void loop() {
  // No delay() here: AccelStepper needs run() called as often as possible.
  motor.update();
  zigbee.update();
  led.setColor(motor.isMoving() ? cfg::kLedMovingColor : cfg::kLedIdleColor);
}