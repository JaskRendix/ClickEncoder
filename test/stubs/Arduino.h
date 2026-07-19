#pragma once
#include <atomic>
#include <cstdint>
#include <functional>

#define HIGH 1
#define LOW 0
#define INPUT 0
#define INPUT_PULLUP 1

inline std::atomic<unsigned long> fake_millis{0};

inline unsigned long millis() { return fake_millis.load(); }
inline void advance_time_ms(unsigned long ms) { fake_millis += ms; }

inline std::function<int(uint8_t)> digitalReadFn = [](uint8_t) { return HIGH; };
inline int digitalRead(uint8_t pin) { return digitalReadFn(pin); }

inline void pinMode(uint8_t, uint8_t) {}
inline void delay(unsigned long) {}
