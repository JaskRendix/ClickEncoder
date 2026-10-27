// ----------------------------------------------------------------------------
// Rotary Encoder Driver with Acceleration
// Supports Click, Double Click, Hold, Release and Repeat events
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

enum class DecoderMode : uint8_t { Normal, Flaky, HalfStep };

class ClickEncoder {
public:
  // Select decoder mode (replaces invalid macro ENC_DECODER_MODE)
  static constexpr DecoderMode decoderMode = DecoderMode::Normal;

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
    int16_t pinButton = -1;
    uint8_t stepsPerNotch = 1;
    uint8_t encoderPinsActive = LOW;
    uint8_t buttonPinActive = LOW;
  };

  // Optional tuning knobs for the acceleration curve. Defaults reproduce
  // the original hard-coded behaviour, but can now be adjusted per-instance
  // instead of requiring a recompile.
  struct AccelConfig {
    static constexpr uint16_t DefaultTop = 4000;
    static constexpr uint16_t DefaultInc = 45;
    static constexpr uint16_t DefaultDec = 4;

    uint16_t top = DefaultTop;
    uint16_t inc = DefaultInc;
    uint16_t dec = DefaultDec;
  };

  explicit ClickEncoder(const Config &cfg);

  void service();

  // Rotation
  int16_t getValue();
  void setBounds(int16_t min, int16_t max, bool wrapAround = false);
  void clearBounds();
  void setCurrentValue(int16_t v);
  int16_t getTrackedValue() const;

  int16_t getMin() const { return minValue; }
  int16_t getMax() const { return maxValue; }

  // Button
  ButtonState getButton();
  bool hasButton() const { return cfg.pinButton >= 0; }
  void setDoubleClickEnabled(bool e) { doubleClickEnabled = e; }
  bool getDoubleClickEnabled() const { return doubleClickEnabled; }

  // Acceleration
  void setAccelerationEnabled(bool e) {
    accelerationEnabled = e;
    if (!e)
      acceleration = 0;
  }
  bool getAccelerationEnabled() const { return accelerationEnabled; }

  // Read the current raw acceleration value.
  uint16_t getAcceleration() const { return acceleration; }
  void setAccelConfig(const AccelConfig &cfg) { accelCfg = cfg; }
  const AccelConfig &getAccelConfig() const { return accelCfg; }

  // Utilities
  // Returns true while unconsumed rotation is pending.
  bool isMoving() const { return delta != 0; }

  // Reset rotation, acceleration and button timing state.
  void reset();

private:
  const Config cfg;
  AccelConfig accelCfg;

  // These fields are updated by service() and read by the public API.
  // If service() runs in an ISR, multi-byte values may require atomic
  // access protection depending on the target architecture.
  volatile int16_t delta = 0;
  volatile int16_t last = 0;
  volatile uint16_t acceleration = 0;
  volatile ButtonState button = ButtonState::Open;
  volatile int16_t trackedValue = 0;

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
