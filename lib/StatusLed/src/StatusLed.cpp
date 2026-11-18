#include "StatusLed.h"

StatusLed::StatusLed(const Config& config)
    : _cfg(config), _pixel(1, config.dataPin, NEO_GRB + NEO_KHZ800) {}

void StatusLed::begin() {
  pinMode(_cfg.powerPin, OUTPUT);
  digitalWrite(_cfg.powerPin, HIGH);  // the pixel has no power without this

  _pixel.begin();
  _pixel.setBrightness(_cfg.brightness);
  _pixel.clear();
  _pixel.show();
}

void StatusLed::setColor(uint32_t rgb) {
  if (rgb == _color) return;
  _color = rgb;
  _pixel.setPixelColor(0, rgb);
  _pixel.show();
}