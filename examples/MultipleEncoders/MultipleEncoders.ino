// -----------------------------------------------------------------------------
// MultipleEncoders
//
// Drives two independent ClickEncoder instances (e.g. a "volume" knob and
// a "brightness" knob) from a single shared ~1ms hardware timer interrupt.
// Portable across AVR (TimerOne) and ESP32 (Ticker); falls back to
// loop()-polling on anything else.
//
// This is the pattern to copy for any project needing more than one
// encoder: one ISR calling service() on every instance is simpler and
// cheaper than one timer per encoder.
// -----------------------------------------------------------------------------

#include <ClickEncoder.h>

#if defined(ARDUINO_ARCH_AVR)
#include <TimerOne.h>
#define ENCODER_HAS_HW_TIMER 1
#elif defined(ARDUINO_ARCH_ESP32)
#include <Ticker.h>
Ticker encoderTicker;
#define ENCODER_HAS_HW_TIMER 1
#else
#define ENCODER_HAS_HW_TIMER 0
#endif

// Adjust to match your wiring; avoid ESP32 flash pins (6-11), strapping
// pins (0/2/12/15), and input-only pins (34-39) if targeting ESP32.
#if defined(ARDUINO_ARCH_AVR)
static constexpr uint8_t VOLUME_PIN_A = A0, VOLUME_PIN_B = A1;
static constexpr int16_t VOLUME_PIN_BTN = A2;
static constexpr uint8_t BRIGHT_PIN_A = A3, BRIGHT_PIN_B = A4;
static constexpr int16_t BRIGHT_PIN_BTN = A5;
#else
static constexpr uint8_t VOLUME_PIN_A = 32, VOLUME_PIN_B = 33;
static constexpr int16_t VOLUME_PIN_BTN = 25;
static constexpr uint8_t BRIGHT_PIN_A = 26, BRIGHT_PIN_B = 27;
static constexpr int16_t BRIGHT_PIN_BTN = 14;
#endif

ClickEncoder *volumeEncoder;
ClickEncoder *brightnessEncoder;

void timerIsr() {
  volumeEncoder->service();
  brightnessEncoder->service();
}

void setup() {
  Serial.begin(115200);

  ClickEncoder::Config volCfg{VOLUME_PIN_A, VOLUME_PIN_B, VOLUME_PIN_BTN};
  ClickEncoder::Config brightCfg{BRIGHT_PIN_A, BRIGHT_PIN_B, BRIGHT_PIN_BTN};
  volumeEncoder = new ClickEncoder(volCfg);
  brightnessEncoder = new ClickEncoder(brightCfg);

  volumeEncoder->setBounds(0, 100, false);
  volumeEncoder->setCurrentValue(50);
  brightnessEncoder->setBounds(0, 100, false);
  brightnessEncoder->setCurrentValue(80);

#if defined(ARDUINO_ARCH_AVR)
  Timer1.initialize(1000); // 1ms
  Timer1.attachInterrupt(timerIsr);
#elif defined(ARDUINO_ARCH_ESP32)
  encoderTicker.attach_ms(1, timerIsr);
#else
#warning                                                                       \
    "No hardware timer configured for this platform; polling service() from loop() instead."
#endif

  Serial.println("MultipleEncoders ready.");
}

void loop() {
#if !ENCODER_HAS_HW_TIMER
  volumeEncoder->service();
  brightnessEncoder->service();
#endif

  if (volumeEncoder->getValue() != 0) {
    Serial.print("Volume: ");
    Serial.println(volumeEncoder->getCurrentValue());
  }

  if (brightnessEncoder->getValue() != 0) {
    Serial.print("Brightness: ");
    Serial.println(brightnessEncoder->getCurrentValue());
  }

  if (volumeEncoder->getButton() == ClickEncoder::ButtonState::Clicked) {
    Serial.println("Volume knob clicked (e.g. toggle mute)");
  }

  if (brightnessEncoder->getButton() == ClickEncoder::ButtonState::Clicked) {
    Serial.println("Brightness knob clicked (e.g. toggle auto-brightness)");
  }
}
