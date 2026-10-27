// -----------------------------------------------------------------------------
// AccelerationTuning
//
// Demonstrates the acceleration-related additions to ClickEncoder:
//   - AccelConfig / setAccelConfig() / getAccelConfig(): tune the
//     acceleration curve at runtime instead of fixed compile-time constants
//   - getAcceleration(): read the current raw acceleration value
//   - isMoving(): cheap check for unconsumed motion without decoding a step
//   - reset(): clear button/click timing and pending delta state
//
// Double-click cycles through a few preset acceleration curves so you can
// feel the difference live. Releasing the button calls reset().
//
// This example polls service() from loop() for simplicity.
// For best acceleration and button timing accuracy, use a hardware timer and
// call service() approximately every 1 ms.
// -----------------------------------------------------------------------------

#include <ClickEncoder.h>

static constexpr uint8_t ENCODER_PIN_A = 2;
static constexpr uint8_t ENCODER_PIN_B = 3;
static constexpr int16_t ENCODER_PIN_BTN = 4;

ClickEncoder *encoder;
int16_t position = 0;

struct NamedAccelConfig {
  const char *name;
  ClickEncoder::AccelConfig cfg;
};

// A few presets to cycle through: gentle, default, and aggressive.
const NamedAccelConfig PRESETS[] = {
    {"Gentle", {2000, 20, 4}},
    {"Default", {4000, 45, 4}},
    {"Aggressive", {6000, 90, 6}},
};
constexpr size_t PRESET_COUNT = sizeof(PRESETS) / sizeof(PRESETS[0]);
int16_t presetIndex = 1; // start on "Default"

void applyPreset() {
  encoder->setAccelConfig(PRESETS[presetIndex].cfg);
  Serial.print("Acceleration preset: ");
  Serial.println(PRESETS[presetIndex].name);

  const auto &cfg = encoder->getAccelConfig();
  Serial.print("Config: top=");
  Serial.print(cfg.top);
  Serial.print(" inc=");
  Serial.print(cfg.inc);
  Serial.print(" dec=");
  Serial.println(cfg.dec);
}

void setup() {
  Serial.begin(115200);

  ClickEncoder::Config cfg{ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_BTN};
  encoder = new ClickEncoder(cfg);

  applyPreset();
  Serial.println("Turn fast vs. slow to feel the acceleration curve.");
  Serial.println("Double-click to cycle presets, hold and release to reset().");
}

void loop() {
  encoder->service();

  if (encoder->isMoving()) {
    int16_t delta = encoder->getValue();
    position += delta;
    Serial.print("Position: ");
    Serial.print(position);
    Serial.print("  Accel=");
    Serial.println(encoder->getAcceleration());
  }

  switch (encoder->getButton()) {
  case ClickEncoder::ButtonState::DoubleClicked:
    presetIndex = (presetIndex + 1) % PRESET_COUNT;
    applyPreset();
    break;

  case ClickEncoder::ButtonState::Released:
    encoder->reset();
    Serial.println("Encoder state reset.");
    break;

  default:
    break;
  }
}
