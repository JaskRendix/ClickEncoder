#pragma once
#include "Arduino.h"

struct FakeTimer {
  unsigned long now_ms() const { return millis(); }
  void enterCritical() {}
  void exitCritical() {}
};
