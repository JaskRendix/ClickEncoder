// -----------------------------------------------------------------------------
// BasicPolling
//
// The simplest possible ClickEncoder sketch: no TimerOne/Ticker, no LCD,
// no platform-specific pin juggling. service() is just called as often as
// possible from loop().
//
// Trade-off: click/hold/double-click timing is only as accurate as your
// loop() is fast and unblocked. Fine for a quick bring-up test on any
// board; for production use with meaningful hold/double-click detection,
// prefer driving service() from a real ~1ms hardware timer (see the
// MultipleEncoders example).
//
// Wiring: encoder A/B pins to ENCODER_PIN_A/B, encoder push-button to
// ENCODER_PIN_BTN, all with the encoder's common wired to GND (the
// library enables internal pull-ups, so no external resistors needed).
// -----------------------------------------------------------------------------

#include <ClickEncoder.h>

// Adjust these to match your wiring -- these are just safe, generally
// available GPIOs, not a claim about any specific board's silkscreen.
static constexpr uint8_t ENCODER_PIN_A = 2;
static constexpr uint8_t ENCODER_PIN_B = 3;
static constexpr int16_t ENCODER_PIN_BTN = 4;

ClickEncoder *encoder;
int16_t position = 0;

void setup() {
  Serial.begin(115200);

  ClickEncoder::Config cfg{ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_BTN};
  encoder = new ClickEncoder(cfg);

  Serial.println("BasicPolling ready. Turn the encoder or press the button.");
}

void loop() {
  encoder->service();

  int16_t delta = encoder->getValue();
  if (delta != 0) {
    position += delta;
    Serial.print("Position: ");
    Serial.println(position);
  }

  switch (encoder->getButton()) {
  case ClickEncoder::ButtonState::Clicked:
    Serial.println("Button: clicked");
    break;
  case ClickEncoder::ButtonState::DoubleClicked:
    Serial.println("Button: double-clicked");
    break;
  case ClickEncoder::ButtonState::Held:
    Serial.println("Button: held");
    break;
  default:
    break; // Open, Pressed, Released, RepeatTicks -- not interesting here
  }
}
