#pragma once

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

// The Feather ESP32-C6's onboard NeoPixel, driven by the Adafruit NeoPixel
// library. The data pin is shared with the BOOT button and the pixel is
// powered through a separate enable pin that must be held high.
class StatusLed {
 public:
  struct Config {
    uint8_t dataPin;
    uint8_t powerPin;
    uint8_t brightness;  // 0-255
  };

  explicit StatusLed(const Config& config);

  void begin();

  // 0xRRGGBB, 0 = off. Only talks to the LED when the color changes, so it is
  // cheap to call every loop iteration.
  void setColor(uint32_t rgb);

 private:
  Config _cfg;
  Adafruit_NeoPixel _pixel;
  uint32_t _color = 0;
};