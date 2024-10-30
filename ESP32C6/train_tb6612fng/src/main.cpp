/* This code manages the motor of a simple toy train */

#include <Arduino.h>
#include <Wire.h>
#include "Motor.h"

// Function declarations
void blink(int cnt, int time);
void ledOn();
void ledOff();
void goToDeepSleep();

#define AIN1 D3
#define AIN2 D7
#define PWMA A1
#define BIN1 D5
#define BIN2 D6
#define PWMB A2
#define STBY D4

const int offsetA = -1;
const int offsetB = 1;

Motor motor = Motor(AIN1, AIN2, PWMA, offsetA, STBY);
Motor leds = Motor(BIN1, BIN2, PWMB, offsetB, STBY);

#define ANALOG_SENSOR1 A0
#define DIGITAL_SENSOR1 D8

int senseCount = 0;

void IRAM_ATTR onMagnetSensed() {
  if (millis() < 5000) {
    Serial.println(F("Magnet sensed? Probably phantom, ignoring"));
  } else {
    Serial.println(F("Magnet sensed, breaking!"));
    Serial.println(millis());
    motor.brake();
  }
}

void setup() {
  blink(1, 300);
  ledOn();

  Serial.begin(9600);
  delay(200);
  Serial.println(F("===================="));
  Serial.println(F("Kid Train Conductor"));

  Wire.begin();

  motor.drive(255);
  //motor.brake();
  Serial.println(F("Turned on motor full speed"));

  leds.drive(25);
  Serial.println(F("Turned on LEDs full brightness"));

  pinMode(ANALOG_SENSOR1, INPUT);
  //attachInterrupt(DIGITAL_SENSOR1, onMagnetSensed, RISING);
}

void loop() {
  int v = analogRead(ANALOG_SENSOR1);
  if (v <= 1650 || v >= 1680) {
    Serial.println(v);
    senseCount++;
    if (senseCount == 3) {
      Serial.println(F("Breaking!"));
      motor.brake();
      delay(5);
      motor.drive(-255);
      delay(100);
      motor.brake();
    }
    delay(5);
  } else {
    senseCount = 0;
    delay(50);
  }
//  delay(100);
}

void blink(int cnt, int time) {
  for (int i = 0; i < cnt; i++) {
    ledOn();
    delay(time);
    ledOff();
    delay(time);
  }
}

void ledOn() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void ledOff() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void goToDeepSleep() {
  // Close down Serial
  Serial.println(F("Sleeping..."));
  Serial.flush();
  Serial.end();
  // Enter deep sleep
  esp_deep_sleep_start();
  // Should never get here
  for(;;);
}