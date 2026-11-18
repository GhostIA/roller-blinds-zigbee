# Zigbee Roller Blind

Firmware for a motorized roller blind controlled over Zigbee. It runs on an
Adafruit Feather ESP32-C6, drives a stepper motor through a TMC2209 driver,
and pairs with Home Assistant (ZHA) as a standard window covering.

- Open, close, stop and go to any position (0–100 %) from Home Assistant
- Motor speed and acceleration can be adjusted from Home Assistant as number entities
- Position, speed and acceleration are saved to flash, so they survive a reboot
- The TMC2209 is set up over UART (run current, microsteps). If the UART link is missing, it falls back to plain STEP/DIR
- The onboard NeoPixel shows blue while the motor runs
- A button on A0 resets Zigbee pairing

## Hardware

| Part | Notes |
| --- | --- |
| Adafruit Feather ESP32-C6 | Zigbee radio + MCU |
| TMC2209 stepper driver breakout | STEP/DIR for motion, single-wire UART for configuration |
| Bipolar stepper motor (200 steps/rev) | Turns the blind tube |
| Motor power supply | Matches your motor/driver (VM on the TMC2209) |
| Momentary push button | Factory reset, optional but recommended |
| 1 kΩ resistor | For the UART TX line |

### Wiring

| Feather ESP32-C6 | TMC2209 / other |
| --- | --- |
| IO5 | STEP |
| IO6 | DIR |
| IO10 | EN (active low) |
| IO16 (TX) → 1 kΩ → | PDN/UART |
| IO17 (RX) → | PDN/UART (same pin, no resistor) |
| GND | GND, MS1, MS2 (both low = UART address 0) |
| IO1 (A0) | Button to GND (factory reset) |

The NeoPixel uses IO9 (data, shared with the BOOT button) and IO20 (power
enable), both on the board already. Because IO9 is the NeoPixel data line,
the BOOT button can't be used for factory reset, which is why it uses A0.

All pins are set in [include/config.h](include/config.h).

## Building and flashing

This project uses [PlatformIO](https://platformio.org/). Zigbee on the
ESP32-C6 needs Arduino-ESP32 3.x, which the official `espressif32` platform
does not ship yet, so [platformio.ini](platformio.ini) uses the
[pioarduino](https://github.com/pioarduino/platform-espressif32) fork. It is
downloaded automatically on the first build.

```sh
pio run -e feather_c6                 # build
pio run -e feather_c6 -t upload       # build and flash
pio device monitor                    # serial log, 115200 baud
```

`upload_port` and `debug_port` in `platformio.ini` are set to
`/dev/cu.usbmodem1101`. Change or remove them to match your machine.

The build uses the `zigbee.csv` partition table (included with the Arduino
core) and `-DZIGBEE_MODE_ED` to run as a Zigbee end device. The firmware
won't compile without that flag.

## Pairing with Home Assistant (ZHA)

1. In Home Assistant, go to **Settings → Devices & services → Zigbee Home Automation → Add device**.
2. Power on the Feather. A new device does not have a network yet, so it starts looking for one right away.
3. The device appears as `DIY RollerBlind` with three entities:
   - a **cover** (roller shade) for open, close, stop and position
   - **Blind speed (steps/s)**, a number from 200 to 6000
   - **Blind acceleration (steps/s^2)**, a number from 200 to 12000

To pair with a different coordinator, hold the factory reset button for 3
seconds. The device clears its Zigbee network settings and reboots. The
blind's saved position and motion settings are kept.

## Configuration

Every setting is in [include/config.h](include/config.h). The libraries
under `lib/` don't read this file. Instead, `main.cpp` passes the values in
through each library's `Config` struct.

| Setting | Default | What it does |
| --- | --- | --- |
| `kTravelSteps` | 3200 | Steps from fully open to fully closed. **You must calibrate this.** |
| `kMaxSpeed` | 800 | Default speed in steps/s (Home Assistant overrides it once set) |
| `kAcceleration` | 400 | Default acceleration in steps/s² (Home Assistant overrides it once set) |
| `kDirInverted` | false | Flip this if the blind moves the wrong way |
| `kHoldWhenIdle` | false | Keep the coils powered when stopped (more holding torque, more heat) |
| `kUseTmcUart` | true | Configure the driver over UART |
| `kRunCurrentMa` | 800 | RMS motor current in mA. Start low and raise it if the motor skips steps. |
| `kMicrosteps` | 8 | Microstepping, set over UART |
| `kRSense` | 0.05 | Sense resistor on your TMC2209 board in Ω. Check your board's schematic, because the run current is calculated from this value. |
| `kReportIntervalMs` | 500 | Shortest time between position reports while moving |
| `kFactoryResetHoldMs` | 3000 | How long to hold the reset button |
| `kLedBrightness` | 32 | NeoPixel brightness (0–255) |

### Calibrating travel

The blind has no limit switches or encoder. It counts steps from the last
position it saved, so `kTravelSteps` has to match your blind:

```
kTravelSteps = (blind drop / tube circumference) × 200 × kMicrosteps
```

For example, a 1.5 m drop on a 38 mm tube (≈ 119 mm circumference) at 8
microsteps comes to about 1500 / 119.4 × 1600 ≈ 20,100 steps.

Position 0 steps means fully open. A device with no saved position assumes
the blind is fully open when it first boots, so roll the blind fully up
before you power it on for the first time.

## Position conventions

| | Fully open | Fully closed |
| --- | --- | --- |
| Steps (internal) | 0 | `kTravelSteps` |
| Percent (Zigbee / HA) | 100 | 0 |

The conversion is in [lib/BlindMath/src/BlindMath.h](lib/BlindMath/src/BlindMath.h).

## Project layout

```
include/config.h        All settings
src/main.cpp            Wires the libraries together; setup() and loop()
lib/
  BlindMath/            Percent ↔ step conversion (no Arduino headers, host-testable)
  BlindMotor/           AccelStepper motion, TMC2209 UART setup, saving to flash
  BlindZigbee/          Zigbee window covering + speed/acceleration endpoints, factory reset
  StatusLed/            Onboard NeoPixel
test/
  test_blind_math/      Host unit tests
  test_blind_motor/     On-device tests
```

### How it fits together

- `loop()` never blocks. `BlindMotor::update()` has to run as often as possible so AccelStepper can produce step pulses.
- Zigbee callbacks run on the Zigbee task, not the loop task. They only store the request (target, stop, new speed) in atomics. `BlindMotor::update()` then acts on it from the loop, so only one task ever touches AccelStepper.
- The position is written to flash once per move, when the motor stops, not on every step.
- The TMC2209 loses its settings whenever its motor supply drops out, so the firmware sends the UART configuration again before every move.
- The device reports its position to the coordinator while moving, at most once every `kReportIntervalMs`, and only when the percentage changes.

Zigbee endpoints:

| Endpoint | Type | Purpose |
| --- | --- | --- |
| 10 | Window covering (roller shade) | Position and commands |
| 11 | Analog output | Speed (steps/s) |
| 12 | Analog output | Acceleration (steps/s²) |

## Tests

**Host tests** cover the percent/step math and need no hardware:

```sh
pio test -e native
```

**On-device tests** cover `BlindMotor` (moves, stop, retargeting
mid-move, saving across a restart). Plug in the Feather but **leave motor
power off**. The tests only toggle the STEP/DIR/EN pins and turn off the
UART setup. They save to their own `blindtest` namespace, so the real saved
position is untouched.

```sh
pio test -e feather_c6
```

## Limitations

- **Open loop.** There is no homing or end-stop. If the motor skips steps, or power is lost mid-move (the position is only saved when a move ends), the firmware's idea of the position drifts. To fix it, move the blind to fully open by hand and erase the saved position. Changing `kNvsNamespace` or erasing flash with `pio run -t erase` both work. Erasing flash also clears the Zigbee pairing.
- **Lift only.** Tilt is not supported.
- Only one `BlindZigbee` instance can exist, because the Zigbee callbacks are plain function pointers.
