#define WITH_LCD

#include <ClickEncoder.h>

// -----------------------------------------------------------------------------
// Portable periodic service() driver.
//
// ClickEncoder's click/hold/double-click timing is driven by how often
// service() is called (ideally every ~1ms from a hardware timer), so we
// pick the best mechanism available per platform:
//   - AVR: TimerOne (hardware Timer1 interrupt)
//   - ESP32: Ticker (ships with the esp32 Arduino core, no extra install)
//   - anything else: fall back to polling from loop() -- less precise
//     timing for held/double-click detection, but keeps the example
//     buildable everywhere.
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Pin assignments.
//
// AVR boards (Uno/Mega/etc.) define A0-A5 as analog-pin aliases usable as
// plain digital I/O; ESP32 dev-board variants often only alias A0, so we
// use explicit GPIO numbers there instead. The ESP32 numbers below also
// specifically avoid: GPIO6-11 (wired to the onboard SPI flash -- never
// usable as general I/O), GPIO0/2/12/15 (boot-strapping pins), and
// GPIO34-39 (input-only, no internal pull-up, unsuitable for the encoder
// pins which rely on INPUT_PULLUP). Adjust to match your actual wiring.
// -----------------------------------------------------------------------------
#if defined(ARDUINO_ARCH_AVR)
#define ENCODER_PIN_A A1
#define ENCODER_PIN_B A0
#define ENCODER_PIN_BTN A2
#else
#define ENCODER_PIN_A 32
#define ENCODER_PIN_B 33
#define ENCODER_PIN_BTN 25
#endif

#ifdef WITH_LCD
#include <LiquidCrystal.h>

#if defined(ARDUINO_ARCH_AVR)
#define LCD_RS 8
#define LCD_RW 9
#define LCD_EN 10
#define LCD_D4 4
#define LCD_D5 5
#define LCD_D6 6
#define LCD_D7 7
#else
#define LCD_RS 19
#define LCD_RW 23
#define LCD_EN 18
#define LCD_D4 17
#define LCD_D5 16
#define LCD_D6 4
#define LCD_D7 5
#endif

#define LCD_CHARS 20
#define LCD_LINES 4

LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
#endif

ClickEncoder *encoder;
int16_t last = -1;
int16_t value = 0;

void timerIsr() { encoder->service(); }

#ifdef WITH_LCD
void displayAccelerationStatus() {
  lcd.setCursor(0, 1);
  lcd.print("Acceleration ");
  lcd.print(encoder->getAccelerationEnabled() ? "on " : "off");
}
#endif

void setup() {
  Serial.begin(9600);

  ClickEncoder::Config cfg{ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_BTN};
  encoder = new ClickEncoder(cfg);

#ifdef WITH_LCD
  lcd.begin(LCD_CHARS, LCD_LINES);
  lcd.clear();
  displayAccelerationStatus();
#endif

#if defined(ARDUINO_ARCH_AVR)
  Timer1.initialize(1000); // 1ms
  Timer1.attachInterrupt(timerIsr);
#elif defined(ARDUINO_ARCH_ESP32)
  encoderTicker.attach_ms(1, timerIsr);
#else
#warning                                                                       \
    "No hardware timer configured for this platform; polling service() from loop() instead."
#endif
}

void loop() {
#if !ENCODER_HAS_HW_TIMER
  // No hardware timer available on this platform: poll as a fallback.
  // Less precise than a 1ms ISR, so click/hold/double-click timing may
  // be less reliable if loop() does other slow work.
  encoder->service();
#endif

  value += encoder->getValue();

  if (value != last) {
    last = value;
    Serial.print("Encoder Value: ");
    Serial.println(value);
#ifdef WITH_LCD
    lcd.setCursor(0, 0);
    lcd.print("                    "); // 20 spaces
    lcd.setCursor(0, 0);
    lcd.print(value);
#endif
  }

  ClickEncoder::ButtonState b = encoder->getButton();
  if (b != ClickEncoder::ButtonState::Open) {
    Serial.print("Button: ");

#define VERBOSECASE(label)                                                     \
  case label:                                                                  \
    Serial.println(#label);                                                    \
    break;

    switch (b) {
      VERBOSECASE(ClickEncoder::ButtonState::Pressed);
      VERBOSECASE(ClickEncoder::ButtonState::Held);
      VERBOSECASE(ClickEncoder::ButtonState::Released);
      VERBOSECASE(ClickEncoder::ButtonState::Clicked);
      VERBOSECASE(ClickEncoder::ButtonState::RepeatTicks);

    case ClickEncoder::ButtonState::DoubleClicked:
      Serial.println("ClickEncoder::ButtonState::DoubleClicked");
      encoder->setAccelerationEnabled(!encoder->getAccelerationEnabled());
      Serial.print("  Acceleration is ");
      Serial.println(encoder->getAccelerationEnabled() ? "enabled"
                                                       : "disabled");
#ifdef WITH_LCD
      displayAccelerationStatus();
#endif
      break;

    default:
      break;
    }

#undef VERBOSECASE
  }
}
