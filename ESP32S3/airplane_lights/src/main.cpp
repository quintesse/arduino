#include <Arduino.h>

// Define your GPIO pins
const int pinA = D6; // Steady Group
const int pinB = D5; // Double Strobe Group
const int pinC = D4; // Long Pulse Group

void setup() {
  pinMode(pinA, OUTPUT);
  pinMode(pinB, OUTPUT);
  pinMode(pinC, OUTPUT);

  // Pin A is always on
  digitalWrite(pinA, HIGH);
}

void loop() {
  unsigned long currentMillis = millis();
  unsigned long cycleTime = currentMillis % 1000; // 1-second (1000ms) cycle

  // Pin B: 2 short flashes (50ms on, 100ms off, 50ms on, rest off)
  if (cycleTime < 50) {
    digitalWrite(pinB, HIGH);
  } else if (cycleTime >= 50 && cycleTime < 150) {
    digitalWrite(pinB, LOW);
  } else if (cycleTime >= 150 && cycleTime < 200) {
    digitalWrite(pinB, HIGH);
  } else {
    digitalWrite(pinB, LOW);
  }

  // Pin C: 1 slightly longer pulse (250ms)
  if (cycleTime >= 250 && cycleTime < 500) {
    digitalWrite(pinC, HIGH);
  } else {
    digitalWrite(pinC, LOW);
  }
}
