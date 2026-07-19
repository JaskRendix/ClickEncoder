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
// Modernized fork: portable C++, strategy-based decoders
// ----------------------------------------------------------------------------

#pragma once

#include <Arduino.h>
#include <stdint.h>

enum class DecoderType : uint8_t { Normal, Flaky, HalfStep };

class ClickEncoder {
public:
  // Select decoder mode (replaces invalid macro ENC_DECODER_MODE)
  static constexpr DecoderType decoderMode = DecoderType::Normal;

  enum class ButtonState : uint8_t {
    Open = 0,
    Pressed,
    Held,
    Released,
    Clicked,
    DoubleClicked,
    RepeatTicks
  };

  struct Config {
    uint8_t pinA;
    uint8_t pinB;
    int16_t pinBTN = -1;
    uint8_t stepsPerNotch = 1;
    uint8_t coderPinsActive = LOW;
    uint8_t btnPinActive = LOW;
  };

  // Optional tuning knobs for the acceleration curve. Defaults reproduce
  // the original hard-coded behaviour, but can now be adjusted per-instance
  // instead of requiring a recompile.
  struct AccelConfig {
    uint16_t top = 4000;
    uint16_t inc = 45;
    uint16_t dec = 4;
  };

public:
  explicit ClickEncoder(const Config &cfg);

  void service();

  int16_t getValue();
  ButtonState getButton();

  void setBounds(int16_t min, int16_t max, bool wrapAround = false);
  void clearBounds();
  void setCurrentValue(int16_t v);
  int16_t getCurrentValue() const;
  int16_t getMin() const { return minValue; }
  int16_t getMax() const { return maxValue; }

  void setDoubleClickEnabled(bool e) { doubleClickEnabled = e; }
  bool getDoubleClickEnabled() const { return doubleClickEnabled; }

  void setAccelerationEnabled(bool e) {
    accelerationEnabled = e;
    if (!e)
      acceleration = 0;
  }
  bool getAccelerationEnabled() const { return accelerationEnabled; }

  // NEW: expose raw acceleration + allow tuning the curve at runtime.
  uint16_t getAcceleration() const { return acceleration; }
  void setAccelConfig(const AccelConfig &cfg) { accelCfg = cfg; }
  const AccelConfig &getAccelConfig() const { return accelCfg; }

  // NEW: true if the encoder has generated any delta since the last
  // getValue() call was consumed to zero -- lets callers cheaply poll
  // "is anything happening" without decoding a full step.
  bool isMoving() const { return delta != 0; }

  // NEW: force-reset all button/click timing state, e.g. after a UI
  // context switch where a stale long-press shouldn't carry over.
  void reset();

private:
  const Config cfg;
  AccelConfig accelCfg;

  volatile int16_t delta = 0;
  volatile int16_t last = 0;
  volatile uint16_t acceleration = 0;
  volatile ButtonState button = ButtonState::Open;
  volatile int16_t trackAbsoluteValue = 0;

  bool doubleClickEnabled = true;
  bool accelerationEnabled = true;

  bool hasBounds = false;
  bool wrapBounds = false;
  int16_t minValue = 0;
  int16_t maxValue = 0;

  uint16_t keyDownTicks = 0;
  int16_t doubleClickTicks = 0;
  uint32_t lastButtonCheck = 0;
  uint16_t repeatIntervalCounter = 0;

  static constexpr int8_t tableNormal[16] = {0, 1, -1, 0,  -1, 0,  0, 1,
                                             1, 0, 0,  -1, 0,  -1, 1, 0};

  static constexpr int8_t tableHalfStep[16] = {0, 0, -1, 0, 0, 0,  0, 1,
                                               1, 0, 0,  0, 0, -1, 0, 0};
};