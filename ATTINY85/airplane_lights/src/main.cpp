#include <Arduino.h>

// Mapping for TINY85 pins:
const int pinA = 0; // Steady Group
const int pinB = 1; // Double Strobe Group
const int pinC = 2; // Long Pulse Group

bool lightsActive;
bool startLightsActive = true;

unsigned long startMillis;

void setLights(bool active) {
  lightsActive = active;
  int state = active ? HIGH : LOW;
  digitalWrite(pinA, state);
  digitalWrite(pinB, state);
  digitalWrite(pinC, state);
}

void handleToggle() {
  setLights(!lightsActive);
}

void setup() {
  pinMode(pinA, OUTPUT);
  pinMode(pinB, OUTPUT);
  pinMode(pinC, OUTPUT);
  lightsActive = startLightsActive;
  startMillis = millis();
  setLights(true);
}

void loop() {
  unsigned long currentMillis = millis();
  if (lightsActive && (currentMillis - startMillis) >= 500) {
    unsigned long cycleTime = currentMillis % 1000;

    // Pin A: Steady On (Mixed Red, Green, White)
    digitalWrite(pinA, HIGH);

    // Pin B: Double Strobe (2 White)
    if (cycleTime < 50 || (cycleTime >= 150 && cycleTime < 200)) {
      digitalWrite(pinB, HIGH);
    } else {
      digitalWrite(pinB, LOW);
    }

    // Pin C: Long Pulse (2 Red)
    digitalWrite(pinC, (cycleTime >= 250 && cycleTime < 500) ? HIGH : LOW);
  }
}