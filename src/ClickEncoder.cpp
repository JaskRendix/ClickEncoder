// ----------------------------------------------------------------------------
// Rotary Encoder Driver with Acceleration
// Supports Click, DoubleClick, Long Click
//
// (c) 2010 karl@pitrich.com
// (c) 2014 karl@pitrich.com
//
// Timer-based rotary encoder logic by Peter Dannegger
// http://www.mikrocontroller.net/articles/Drehgeber
//
// Modernized fork: portable C++, compile-time decoder selection
// ----------------------------------------------------------------------------

#include "ClickEncoder.h"

// Arduino core already defines noInterrupts()/interrupts() portably across
// AVR, SAMD, ESP32, RP2040, etc. Use those in Arduino builds. Keep the AVR
// fallback for AVR-specific builds that do not define ARDUINO.
#if defined(ARDUINO_ARCH_AVR)
#include <avr/interrupt.h>
#define ENC_ENTER_CRITICAL() cli()
#define ENC_EXIT_CRITICAL() sei()
#elif defined(ARDUINO)
#define ENC_ENTER_CRITICAL() noInterrupts()
#define ENC_EXIT_CRITICAL() interrupts()
#else
#define ENC_ENTER_CRITICAL()
#define ENC_EXIT_CRITICAL()
#endif

static constexpr uint32_t ENC_BUTTONINTERVAL = 10;
static constexpr uint32_t ENC_DOUBLECLICKTIME = 400;
static constexpr uint32_t ENC_HOLDTIME = 1000;
static constexpr uint16_t ENC_REPEAT_RATE = 150;

ClickEncoder::ClickEncoder(const Config &cfg) : cfg(cfg) {
  uint8_t coderType = (cfg.encoderPinsActive == LOW) ? INPUT_PULLUP : INPUT;
  pinMode(cfg.pinA, coderType);
  pinMode(cfg.pinB, coderType);

  if (cfg.pinButton >= 0) {
    uint8_t btnType = (cfg.buttonPinActive == LOW) ? INPUT_PULLUP : INPUT;
    pinMode(cfg.pinButton, btnType);
  }

  int16_t init = 0;
  if (digitalRead(cfg.pinA) == cfg.encoderPinsActive)
    init = 3;
  if (digitalRead(cfg.pinB) == cfg.encoderPinsActive)
    init ^= 1;
  last = init;
}

void ClickEncoder::setBounds(int16_t min, int16_t max, bool wrapAround) {
  minValue = min;
  maxValue = max;
  wrapBounds = wrapAround;
  hasBounds = true;

  if (trackedValue < minValue)
    trackedValue = minValue;
  if (trackedValue > maxValue)
    trackedValue = maxValue;
}

void ClickEncoder::clearBounds() { hasBounds = false; }

void ClickEncoder::setCurrentValue(int16_t v) { trackedValue = v; }

int16_t ClickEncoder::getTrackedValue() const { return trackedValue; }

void ClickEncoder::reset() {
  // Reset transient state; preserve trackedValue and configured bounds.
  ENC_ENTER_CRITICAL();
  delta = 0;
  acceleration = 0;
  button = ButtonState::Open;
  ENC_EXIT_CRITICAL();

  keyDownTicks = 0;
  doubleClickTicks = 0;
  repeatIntervalCounter = 0;
}

void ClickEncoder::service() {
  bool moved = false;
  uint32_t now = millis();

  if (accelerationEnabled && acceleration >= accelCfg.dec)
    acceleration -= accelCfg.dec;

  int16_t localLast = last;

  // -------------------------------------------------------------------------
  // Compile-time decoder selection
  // -------------------------------------------------------------------------
  if (decoderMode == DecoderMode::Flaky ||
      decoderMode == DecoderMode::HalfStep) {

    localLast = (localLast << 2) & 0x0F;

    if (digitalRead(cfg.pinA) == cfg.encoderPinsActive)
      localLast |= 2;
    if (digitalRead(cfg.pinB) == cfg.encoderPinsActive)
      localLast |= 1;

    int8_t tbl = (decoderMode == DecoderMode::HalfStep)
                     ? tableHalfStep[localLast]
                     : tableNormal[localLast];

    if (tbl) {
      delta += tbl;
      moved = true;
    }

  } else {
    int8_t curr = 0;
    if (digitalRead(cfg.pinA) == cfg.encoderPinsActive)
      curr = 3;
    if (digitalRead(cfg.pinB) == cfg.encoderPinsActive)
      curr ^= 1;

    int8_t diff = localLast - curr;
    if (diff & 1) {
      localLast = curr;
      delta += (diff & 2) - 1;
      moved = true;
    }
  }

  last = localLast;

  if (accelerationEnabled && moved) {
    if (acceleration <= (accelCfg.top - accelCfg.inc))
      acceleration += accelCfg.inc;
  }

  // -------------------------------------------------------------------------
  // Button processing
  // -------------------------------------------------------------------------
  if (cfg.pinButton >= 0 && (now - lastButtonCheck >= ENC_BUTTONINTERVAL)) {
    lastButtonCheck = now;

    ButtonState st = button;

    if (digitalRead(cfg.pinButton) == cfg.buttonPinActive) {
      // Rising edge of the press: report Pressed once, before Held kicks
      // in. Previously ButtonState::Pressed was declared but never
      // assigned anywhere, so callers switching on it would never see it.
      if (keyDownTicks == 0)
        st = ButtonState::Pressed;

      keyDownTicks++;

      if (keyDownTicks > (ENC_HOLDTIME / ENC_BUTTONINTERVAL)) {
        if (st != ButtonState::Held && st != ButtonState::RepeatTicks) {
          st = ButtonState::Held;
          repeatIntervalCounter = 0;
        } else {
          repeatIntervalCounter += ENC_BUTTONINTERVAL;
          if (repeatIntervalCounter >= ENC_REPEAT_RATE) {
            repeatIntervalCounter = 0;
            st = ButtonState::RepeatTicks;
          }
        }
      }

    } else {
      if (keyDownTicks > 0) {
        if (st == ButtonState::Held || st == ButtonState::RepeatTicks) {
          st = ButtonState::Released;
          doubleClickTicks = 0;
        } else {
          if (doubleClickTicks > 1) {
            if (doubleClickTicks < (ENC_DOUBLECLICKTIME / ENC_BUTTONINTERVAL)) {
              st = ButtonState::DoubleClicked;
              doubleClickTicks = 0;
            }
          } else {
            doubleClickTicks = doubleClickEnabled
                                   ? (ENC_DOUBLECLICKTIME / ENC_BUTTONINTERVAL)
                                   : 1;
          }
        }
      }
      keyDownTicks = 0;
    }

    if (doubleClickTicks > 0) {
      doubleClickTicks--;
      if (doubleClickTicks == 0)
        st = ButtonState::Clicked;
    }

    button = st;
  }
}

int16_t ClickEncoder::getValue() {
  int16_t val;
  uint16_t rawAccel;

  // delta and acceleration are updated by service(), which may run in an ISR.
  // Snapshot both atomically so the calculation uses a consistent value.
  ENC_ENTER_CRITICAL();
  val = delta;
  delta = 0;
  rawAccel = acceleration;
  ENC_EXIT_CRITICAL();

  if (cfg.stepsPerNotch == 4)
    val >>= 2;
  else if (cfg.stepsPerNotch == 2)
    val >>= 1;

  if (val == 0)
    return 0;

  int16_t accelMul = 1 + ((uint32_t)rawAccel * rawAccel >> 18);

  int16_t step = val * accelMul;

  if (hasBounds) {
    int32_t target = (int32_t)trackedValue + step;

    if (wrapBounds) {
      int32_t range = (maxValue - minValue + 1);
      if (target > maxValue)
        target = minValue + ((target - maxValue - 1) % range);
      else if (target < minValue)
        target = maxValue - ((minValue - target - 1) % range);
    } else {
      if (target < minValue)
        target = minValue;
      if (target > maxValue)
        target = maxValue;
    }

    step = (int16_t)(target - trackedValue);
    trackedValue = (int16_t)target;
  }

  return step;
}

ClickEncoder::ButtonState ClickEncoder::getButton() {
  ButtonState st = button;
  if (st != ButtonState::Held && st != ButtonState::RepeatTicks)
    button = ButtonState::Open;
  return st;
}
