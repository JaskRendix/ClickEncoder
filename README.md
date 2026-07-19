# ClickEncoder

Arduino library for rotary encoders with an integrated push button. Provides quadrature decoding, optional rotational acceleration, and button state detection (click, double-click, hold, release).

This is a modernized fork of the original [0xPIT/encoder](https://github.com/0xPIT/encoder) library. The core decoding and timing logic is unchanged; the API has been restructured to use a `Config` struct and a scoped `enum class` instead of positional constructor arguments and preprocessor macros.

## Requirements

- C++11 or later.
- A periodic call to `service()`, ideally every ~1ms, from a hardware timer interrupt. `service()` can be called from any GPIO pin state, not just interrupt-capable pins — the timer drives polling, not pin-change interrupts.
- No timer library is bundled. Use whatever your platform provides: [TimerOne](http://playground.arduino.cc/Code/Timer1) on AVR, `Ticker` on ESP32 (included with the ESP32 Arduino core), or an equivalent for other platforms. See `examples/` for both.

## Installation

Copy this repository into your Arduino `libraries/` folder, or install via the Arduino Library Manager / PlatformIO if published there. The library layout follows the 1.5 format (`src/` contains the sources; `library.properties` at the root marks the format).

## Usage

```cpp
#include <ClickEncoder.h>
#include <TimerOne.h> // or Ticker on ESP32, or your platform's equivalent

ClickEncoder *encoder;

void timerIsr() {
  encoder->service();
}

void setup() {
  ClickEncoder::Config cfg{2, 3, 4}; // pinA, pinB, pinBTN
  encoder = new ClickEncoder(cfg);

  Timer1.initialize(1000); // 1ms
  Timer1.attachInterrupt(timerIsr);
}

void loop() {
  int16_t delta = encoder->getValue();
  if (delta != 0) {
    // use delta
  }

  ClickEncoder::ButtonState btn = encoder->getButton();
  if (btn == ClickEncoder::ButtonState::Clicked) {
    // handle click
  }
}
```

### Config

```cpp
struct Config {
  uint8_t pinA;
  uint8_t pinB;
  int16_t pinBTN = -1;         // -1 disables button handling
  uint8_t stepsPerNotch = 1;   // 1, 2, or 4 depending on encoder hardware
  uint8_t coderPinsActive = LOW;
  uint8_t btnPinActive = LOW;
};
```

Pins are configured as `INPUT_PULLUP` (or `INPUT` if the active level is `HIGH`) internally; no external pull resistors are required for the common case. Any GPIO pin can be used — no interrupt capability is needed, since decoding happens on the timer-driven `service()` call, not on pin-change interrupts.

If the encoder has no button, omit `pinBTN` (it defaults to `-1`) and don't call any of the button-related methods.

### Rotation

- `getValue()` — returns and clears the accumulated step delta since the last call, scaled by `stepsPerNotch` and by acceleration if enabled. If bounds are set, the return value is the actual (clamped or wrapped) change applied.
- `setBounds(min, max, wrapAround)` — constrains the tracked absolute value to `[min, max]`. With `wrapAround = true`, exceeding a bound wraps to the other end instead of clamping.
- `clearBounds()` — removes bounds tracking.
- `setCurrentValue(v)` / `getCurrentValue()` — set/read the tracked absolute value. `getCurrentValue()` reflects the state as of the last `getValue()` call, not the raw pin state — it does not update on its own between `getValue()` calls.
- `isMoving()` — returns true if there is unconsumed rotation delta pending.

### Acceleration

Acceleration increases the step size returned by `getValue()` when the encoder is rotated quickly. It is enabled by default.

- `setAccelerationEnabled(bool)` / `getAccelerationEnabled()`.
- `getAcceleration()` — reads the current raw internal acceleration value.
- `setAccelConfig(AccelConfig)` / `getAccelConfig()` — adjust the acceleration curve at runtime:

```cpp
struct AccelConfig {
  uint16_t top = 4000; // upper bound of the internal acceleration value
  uint16_t inc = 45;   // increment applied per detent while moving
  uint16_t dec = 4;    // decay applied per service() call while idle
};
```

The defaults assume `service()` is called every 1ms. If you use a different interval, `inc`/`dec` need to be re-tuned proportionally, or the acceleration curve will feel too fast or too slow.

Acceleration is commonly disabled while navigating a menu, where a fixed step size per detent is expected:

```cpp
encoder->setAccelerationEnabled(false);
```

### Decoder mode

The quadrature decoder strategy (`Normal`, `Flaky`, `HalfStep`) is a compile-time constant (`ClickEncoder::decoderMode` in `ClickEncoder.h`), not a runtime or macro-configurable option. `Normal` is the default and works for most encoders. If an encoder produces spurious or missed steps, edit `decoderMode` in the header directly and recompile.

### Button

`getButton()` returns a `ClickEncoder::ButtonState`:

```cpp
enum class ButtonState : uint8_t {
  Open, Pressed, Held, Released, Clicked, DoubleClicked, RepeatTicks
};
```

- `Pressed` — fires once on the initial press edge.
- `Held` — the button has been pressed longer than the hold threshold.
- `RepeatTicks` — fires repeatedly at a fixed interval while held, after the initial `Held` transition.
- `Released` — the button was released after a `Held`/`RepeatTicks` state.
- `Clicked` — a short press and release, with no second click following within the double-click window.
- `DoubleClicked` — two clicks within the double-click window.

Reading `getButton()` clears the state to `Open` except while `Held` or `RepeatTicks` is active.

`setDoubleClickEnabled(bool)` disables double-click detection; a short click is then reported as `Clicked` without waiting out the double-click window, at the cost of not being able to distinguish single from double clicks.

Timing thresholds (`ENC_BUTTONINTERVAL`, `ENC_DOUBLECLICKTIME`, `ENC_HOLDTIME`, `ENC_REPEAT_RATE`) are constants in `ClickEncoder.cpp`. Like the acceleration parameters, they assume `service()` is called every 1ms; both the button and double-click timing windows are counted in units of `service()` calls that pass the internal 10ms gate, not directly in elapsed wall-clock time.

### Other

- `reset()` — clears pending delta, acceleration, and button/click timing state.

## Examples

- `ClickEncoderTest` — full example with an LCD display and acceleration toggling.
- `BasicPolling` — minimal example, no timer library dependency, `service()` polled from `loop()`.
- `BoundedMenu` — encoder as a wrapping menu selector using `setBounds()`.
- `MultipleEncoders` — two encoders serviced from one shared timer interrupt.
- `AccelerationTuning` — runtime `AccelConfig` adjustment, `getAcceleration()`, `isMoving()`, `reset()`.

## Credits

Quadrature decoding and button timing logic originally by Peter Dannegger. Original Arduino port by karl@pitrich.com.
