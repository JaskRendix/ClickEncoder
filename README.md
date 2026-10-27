# ClickEncoder

A rotary encoder library with a built-in push button.

- Reads quadrature rotation
- Handles clicks, double-clicks, holds, releases, and repeats
- Supports rotation acceleration
- Works with regular GPIO pins—no pin-change interrupts needed
- Lets multiple encoders share one timer interrupt
- Lets you tune acceleration while the program runs

This fork updates the original [0xPIT/encoder](https://github.com/0xPIT/encoder) API with a `Config` struct and scoped `enum class` types. The encoder and button logic comes from the original library.

## What you need

- C++11 or later
- A call to `service()` on a timer, about every 1 ms
- A timer library for your board:
  - AVR: [TimerOne](http://playground.arduino.cc/Code/Timer1)
  - ESP32: `Ticker`, included with the ESP32 Arduino core
  - Other platforms: an equivalent timer

The library polls the pins on a timer, so you can use any digital pins.

## Install

Copy this repo into your Arduino `libraries/` folder. You can also install it through the Arduino Library Manager or PlatformIO if it’s published there.

```text
ClickEncoder/
├── library.properties
└── src/
    ├── ClickEncoder.h
    └── ClickEncoder.cpp
```

## Quick start

```cpp
#include <ClickEncoder.h>
#include <TimerOne.h>

ClickEncoder* encoder;

void timerIsr() {
  encoder->service();
}

void setup() {
  ClickEncoder::Config cfg{2, 3, 4};
  encoder = new ClickEncoder(cfg);

  Timer1.initialize(1000); // 1ms
  Timer1.attachInterrupt(timerIsr);
}

void loop() {
  int16_t delta = encoder->getValue();

  if (delta != 0) {
    // handle rotation
  }

  if (encoder->getButton() == ClickEncoder::ButtonState::Clicked) {
    // handle click
  }
}
```

## Configure pins

```cpp
struct Config {
  uint8_t pinA;
  uint8_t pinB;
  int16_t pinButton = -1;      // -1 disables button handling
  uint8_t stepsPerNotch = 1;   // 1, 2, or 4, depending on your encoder
  uint8_t encoderPinsActive = LOW;
  uint8_t buttonPinActive = LOW;
};
```

The library sets the pin mode based on the active level:

- `LOW` uses `INPUT_PULLUP`
- `HIGH` uses `INPUT`

Most encoders with a common pin connected to GND don’t need external pull-up resistors.

No push button? Leave out `pinButton` and don’t call the button methods.

## Read rotation

`getValue()` returns the movement since the last read, applies acceleration if it’s enabled, then clears the accumulated delta.

```cpp
int16_t delta = encoder->getValue();
position += delta;
```

### Track a value

Set bounds to have ClickEncoder track an absolute value:

```cpp
encoder->setBounds(0, 100);
encoder->setCurrentValue(50);
```

You can also use:

```cpp
setBounds(min, max, wrapAround);
clearBounds();

setCurrentValue(value);
getTrackedValue();

getMin();
getMax();
```

With wrap-around enabled, moving past either end jumps to the other:

```text
5 + 1 -> 0
0 - 1 -> 5
```

With wrap-around off, the value stops at the bounds:

```text
5 + 1 -> 5
0 - 1 -> 0
```

`getTrackedValue()` shows the value from the last `getValue()` call. Call `getValue()` first to process any pending movement:

```cpp
encoder->getValue();
int16_t value = encoder->getTrackedValue();
```

`isMoving()` returns `true` when there’s rotation waiting to be read.

### Menu example

```cpp
encoder->setAccelerationEnabled(false);
encoder->setBounds(0, 4, true);

encoder->getValue(); // update the tracked value

switch (encoder->getTrackedValue()) {
  case 0: /* Menu item 0 */ break;
  case 1: /* Menu item 1 */ break;
  case 2: /* Menu item 2 */ break;
  case 3: /* Menu item 3 */ break;
  case 4: /* Menu item 4 */ break;
}
```

## Acceleration

Acceleration is on by default. It increases the movement returned by `getValue()` when you turn the encoder quickly.

```cpp
encoder->setAccelerationEnabled(true);
encoder->setAccelerationEnabled(false);

bool enabled = encoder->getAccelerationEnabled();
uint16_t accel = encoder->getAcceleration();
```

To tune the acceleration while your program runs:

```cpp
struct AccelConfig {
  uint16_t top = 4000;
  uint16_t inc = 45;
  uint16_t dec = 4;
};

encoder->setAccelConfig(cfg);
auto cfg = encoder->getAccelConfig();
```

- `top`: maximum acceleration value
- `inc`: acceleration increase per movement
- `dec`: acceleration drop per `service()` call

These defaults assume `service()` runs every 1 ms. If your timer interval is different, adjust `inc` and `dec`.

For menus, you may want to turn acceleration off so each detent moves one item.

## Decoder mode

The library has three quadrature decoder modes:

```cpp
Normal
Flaky
HalfStep
```

Choose the mode at compile time in `ClickEncoder.h`:

```cpp
static constexpr DecoderMode decoderMode = DecoderMode::Normal;
```

`Normal` works for most encoders. If yours misses steps or reads noisy transitions, try `Flaky` or `HalfStep`, then recompile.

## Button events

```cpp
enum class ButtonState : uint8_t {
  Open,
  Pressed,
  Held,
  Released,
  Clicked,
  DoubleClicked,
  RepeatTicks
};
```

A short press goes through `Pressed`, then `Clicked`. A double-click ends with `DoubleClicked`. A long press goes through `Pressed`, then `Held`; while you keep holding, it can emit `RepeatTicks`. Releasing after a hold emits `Released`.

- `Pressed`: button just went down
- `Held`: hold threshold reached
- `RepeatTicks`: repeat event while held
- `Released`: held button let go
- `Clicked`: short press and release
- `DoubleClicked`: two clicks inside the double-click window
- `Open`: no pending event

Read an event with `getButton()`:

```cpp
auto state = encoder->getButton();
```

Reading it resets the state to `Open`, unless `Held` or `RepeatTicks` is active.

### Double-clicks

```cpp
encoder->setDoubleClickEnabled(false);
```

Turn off double-click detection to get `Clicked` right after a short press. You’ll get faster single-click response, but the library won’t distinguish single clicks from double-clicks.

### Timing

Button timing uses internal polling ticks. These constants in `ClickEncoder.cpp` assume `service()` runs every 1 ms:

```cpp
ENC_BUTTONINTERVAL
ENC_DOUBLECLICKTIME
ENC_HOLDTIME
ENC_REPEAT_RATE
```

## Reset

```cpp
encoder->reset();
```

`reset()` clears pending rotation, acceleration state, button state, and click, hold, and repeat timing.

It leaves your bounds, tracked value, and acceleration configuration alone.

## Examples

- `BasicPolling`: checks wiring by calling `service()` from `loop()`; no timer library needed
- `ClickEncoderTest`: shows button states and acceleration on an LCD
- `BoundedMenu`: shows tracked values, bounds, and wrap-around
- `MultipleEncoders`: shares one timer interrupt across several encoders
- `AccelerationTuning`: changes acceleration settings and uses `getAcceleration()`, `isMoving()`, and `reset()`

## Credits

Quadrature decoding and button timing by Peter Dannegger. The original Arduino port was by karl@pitrich.com.

The API update and library layout are based on the original 0xPIT/encoder project.
