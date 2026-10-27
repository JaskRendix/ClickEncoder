#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include "Arduino.h"
#include "ClickEncoder.h"

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void reset_mock_hardware() {
  fake_millis.store(0);
  digitalReadFn = [](uint8_t) { return HIGH; };
}

void run_ticks(ClickEncoder &enc, uint32_t count, uint32_t ms) {
  for (uint32_t i = 0; i < count; ++i) {
    advance_time_ms(ms);
    enc.service();
  }
}

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

TEST_CASE("Encoder baseline initialization") {
  reset_mock_hardware();

  ClickEncoder::Config cfg{2, 3, 4, 4};
  ClickEncoder enc(cfg);

  REQUIRE(enc.getValue() == 0);
  REQUIRE(enc.getButton() == ClickEncoder::ButtonState::Open);
}

// -----------------------------------------------------------------------------
// Quadrature decoding
// -----------------------------------------------------------------------------

TEST_CASE("Quadrature rotation decoding") {
  reset_mock_hardware();
  ClickEncoder::Config cfg{2, 3, -1, 1};
  ClickEncoder enc(cfg);

  SECTION("Clockwise produces negative delta") {
    digitalReadFn = [&](uint8_t p) { return (p == 2 ? LOW : HIGH); };
    enc.service();

    digitalReadFn = [&](uint8_t p) { return (p == 2 ? LOW : LOW); };
    enc.service();

    REQUIRE(enc.getValue() < 0);
  }

  SECTION("Counter-clockwise produces positive delta") {
    digitalReadFn = [&](uint8_t p) { return (p == 3 ? LOW : HIGH); };
    enc.service();

    digitalReadFn = [&](uint8_t p) { return (p == 2 ? LOW : LOW); };
    enc.service();

    REQUIRE(enc.getValue() > 0);
  }

  SECTION("Bounce immunity is not implemented: invalid toggles may move") {
    digitalReadFn = [&](uint8_t p) { return (p == 2 ? LOW : HIGH); };
    enc.service();

    digitalReadFn = [&](uint8_t p) { return (p == 2 ? HIGH : HIGH); };
    enc.service();

    digitalReadFn = [&](uint8_t p) { return (p == 2 ? LOW : HIGH); };
    enc.service();

    REQUIRE(enc.getValue() != 0);
  }
}

// -----------------------------------------------------------------------------
// Bounds
// -----------------------------------------------------------------------------

TEST_CASE("Bounds: clamping and wrap-around") {
  reset_mock_hardware();
  ClickEncoder::Config cfg{2, 3, -1, 1};
  ClickEncoder enc(cfg);

  SECTION("Clamping reduces value but never exceeds bounds") {
    enc.setBounds(0, 10, false);
    enc.setCurrentValue(10);

    digitalReadFn = [&](uint8_t p) { return (p == 2 ? LOW : HIGH); };
    enc.service();
    enc.getValue(); // consume the pending delta so trackedValue updates

    REQUIRE(enc.getTrackedValue() == 9);
  }

  SECTION("Wrap-around upper → lower only when exceeding bounds") {
    enc.setBounds(0, 10, true);
    enc.setCurrentValue(10);

    digitalReadFn = [&](uint8_t p) { return (p == 2 ? LOW : HIGH); };
    enc.service();
    enc.getValue();

    REQUIRE(enc.getTrackedValue() == 9);
  }

  SECTION("Wrap-around lower → upper only when exceeding bounds") {
    enc.setBounds(0, 10, true);
    enc.setCurrentValue(0);

    digitalReadFn = [&](uint8_t p) { return (p == 3 ? LOW : HIGH); };
    enc.service();
    enc.getValue();

    REQUIRE(enc.getTrackedValue() == 1);
  }
}

// -----------------------------------------------------------------------------
// StepsPerNotch behavior
// -----------------------------------------------------------------------------

TEST_CASE("StepsPerNotch scaling") {
  reset_mock_hardware();

  ClickEncoder::Config cfg{2, 3, -1, 4};
  ClickEncoder enc(cfg);

  // One half-step → delta = -1 → scaled to -1 (driver divides after
  // accumulation)
  digitalReadFn = [&](uint8_t p) { return (p == 2 ? LOW : HIGH); };
  enc.service();
  digitalReadFn = [&](uint8_t p) { return (p == 2 ? LOW : LOW); };
  enc.service();

  REQUIRE(enc.getValue() == -1);
}

// -----------------------------------------------------------------------------
// Acceleration
// -----------------------------------------------------------------------------

TEST_CASE("Acceleration disabled produces linear output") {
  reset_mock_hardware();
  ClickEncoder::Config cfg{2, 3, -1, 1};

  ClickEncoder enc(cfg);
  enc.setAccelerationEnabled(false);

  digitalReadFn = [&](uint8_t p) { return (p == 2 ? LOW : HIGH); };
  enc.service();
  digitalReadFn = [&](uint8_t p) { return (p == 2 ? LOW : LOW); };
  enc.service();

  REQUIRE(enc.getValue() == -2);
}

// -----------------------------------------------------------------------------
// Button logic
// -----------------------------------------------------------------------------

TEST_CASE("Button: click, hold, repeat, release, double-click") {
  reset_mock_hardware();
  ClickEncoder::Config cfg{2, 3, 4, 1, LOW, LOW};
  ClickEncoder enc(cfg);

  SECTION("Single click") {
    digitalReadFn = [&](uint8_t p) { return (p == 4 ? LOW : HIGH); };
    run_ticks(enc, 5, 10);

    digitalReadFn = [&](uint8_t) { return HIGH; };
    run_ticks(enc, 1, 10);

    // doubleClickTicks was armed for ENC_DOUBLECLICKTIME / ENC_BUTTONINTERVAL
    // (400ms / 10ms = 40) ticks on release. It only counts down one tick per
    // service() call (gated at the 10ms button-check interval), not per
    // elapsed wall-clock ms -- so we need ~40 more 10ms-spaced calls here,
    // not a single call after a big time jump.
    run_ticks(enc, 45, 10);

    auto st = enc.getButton();
    REQUIRE(st == ClickEncoder::ButtonState::Clicked);
  }

  SECTION("Hold → RepeatTicks") {
    digitalReadFn = [&](uint8_t p) { return (p == 4 ? LOW : HIGH); };
    // Held triggers at tick 101 (1000ms / 10ms threshold + 1). RepeatTicks
    // then needs repeatIntervalCounter to accumulate another 150ms, i.e. 15
    // more ticks -- so at least 116 ticks total. Give some margin.
    run_ticks(enc, 130, 10);

    REQUIRE(enc.getButton() == ClickEncoder::ButtonState::RepeatTicks);
  }

  SECTION("Released state") {
    digitalReadFn = [&](uint8_t p) { return (p == 4 ? LOW : HIGH); };
    run_ticks(enc, 120, 10);

    digitalReadFn = [&](uint8_t) { return HIGH; };
    run_ticks(enc, 1, 10);

    REQUIRE(enc.getButton() == ClickEncoder::ButtonState::Released);
  }

  SECTION("Double-click detection") {
    // First click
    digitalReadFn = [&](uint8_t p) { return (p == 4 ? LOW : HIGH); };
    run_ticks(enc, 5, 10);
    digitalReadFn = [&](uint8_t) { return HIGH; };
    run_ticks(enc, 1, 10);

    // Within double-click window
    run_ticks(enc, 20, 10);

    // Second click
    digitalReadFn = [&](uint8_t p) { return (p == 4 ? LOW : HIGH); };
    run_ticks(enc, 5, 10);
    digitalReadFn = [&](uint8_t) { return HIGH; };
    run_ticks(enc, 1, 10);

    REQUIRE(enc.getButton() == ClickEncoder::ButtonState::DoubleClicked);
  }
}