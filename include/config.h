#pragma once

#include <Arduino.h>

// Every tunable for the project lives here.
// Libraries under lib/ never include this file; main.cpp passes these values
// in through their Config structs, so the libraries stay reusable and you
// only ever edit this one file to change behavior.
namespace cfg {

// ---------------------------------------------------------------------------
// Pins: Feather ESP32-C6 -> TMC2209 breakout
// ---------------------------------------------------------------------------
constexpr uint8_t kPinStep = 5;
constexpr uint8_t kPinDir = 6;
constexpr uint8_t kPinEnable = 10;  // TMC2209 EN is active low
// Speed and acceleration numbers in Home Assistant (two extra endpoints).
// kMaxSpeed / kAcceleration above are only the defaults: once set from HA the
// saved value wins. Values from HA are clamped to these limits.
constexpr uint8_t kSpeedEndpoint = 11;
constexpr uint8_t kAccelEndpoint = 12;
constexpr float kSpeedMin = 200.0f;    // steps/s
constexpr float kSpeedMax = 6000.0f;
constexpr float kAccelMin = 200.0f;    // steps/s^2
constexpr float kAccelMax = 12000.0f;

// ---------------------------------------------------------------------------
// TMC2209 over UART (TMCStepper). STEP/DIR still move the motor; UART only
// configures the driver.
// Wiring: Feather TX (IO16) -> 1 kOhm -> breakout UART pin,
//         Feather RX (IO17) -> the same UART pin.
// MS1 and MS2 low = driver address 0.
// If the UART link is missing the firmware logs a warning and keeps running
// on the pot current and the MS1/MS2 microstep setting.
// ---------------------------------------------------------------------------
constexpr bool kUseTmcUart = true;
constexpr uint8_t kPinUartTx = 16;
constexpr uint8_t kPinUartRx = 17;
constexpr uint32_t kUartBaud = 115200;
constexpr uint8_t kTmcAddress = 0;
// VERIFY against the Adafruit schematic: the run current is calculated from
// this value, so a wrong number means a wrong current.
constexpr float kRSense = 0.05f;
constexpr uint16_t kRunCurrentMa = 800;  // RMS; start low, raise if the motor skips
constexpr uint16_t kMicrosteps = 8;

// ---------------------------------------------------------------------------
// Motion
// ---------------------------------------------------------------------------
// Calibrate kTravelSteps: steps from fully open (0) to fully closed.
// Formula: (blind height / tube circumference) * 200 * kMicrosteps
constexpr int32_t kTravelSteps = 3200;
constexpr float kMaxSpeed = 800.0f;      // steps/s
constexpr float kAcceleration = 400.0f;  // steps/s^2
constexpr bool kDirInverted = false;     // flip if the blind moves the wrong way
constexpr bool kHoldWhenIdle = false;    // true keeps coils energized (holding torque, more heat)

// ---------------------------------------------------------------------------
// Onboard NeoPixel (status LED)
// The data pin is IO9, shared with the BOOT button. IO20 powers the pixel.
// Idle is off to save battery.
// ---------------------------------------------------------------------------
constexpr uint8_t kPinNeoPixel = 9;
constexpr uint8_t kPinNeoPixelPower = 20;
constexpr uint8_t kLedBrightness = 32;         // 0-255
constexpr uint32_t kLedMovingColor = 0x0000FF;  // 0xRRGGBB, blue while the motor runs
constexpr uint32_t kLedIdleColor = 0x000000;    // off

// ---------------------------------------------------------------------------
// Persistence (position survives reboots)
// ---------------------------------------------------------------------------
constexpr char kNvsNamespace[] = "blind";

// ---------------------------------------------------------------------------
// Zigbee
// ---------------------------------------------------------------------------
constexpr uint8_t kEndpoint = 10;
constexpr char kManufacturer[] = "DIY";
constexpr char kModel[] = "RollerBlind";
// Wire a momentary button between this pin (A0) and GND. The BOOT button
// can't be used: IO9 is the NeoPixel data pin.
constexpr uint8_t kFactoryResetPin = 1;
constexpr uint32_t kFactoryResetHoldMs = 3000;  // hold this long to reset pairing
constexpr uint32_t kReportIntervalMs = 500;     // min gap between position reports while moving

}  // namespace cfg