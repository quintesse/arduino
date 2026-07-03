/*
 * This code takes a range measurement using a VL53L1X time-of-flight sensor
 * and if its significantly different than the previous measurement sends it
 * over LoRaWAN using a SX1262 module. Afterwards it goes into a deep sleep
 * for a set amount of time. On first boot or when the user presses a button
 * it will display some information, including the measured range, on an
 * SSD1306 OLED display.
 */

// LIB VL53L1X by Pololu (https://github.com/pololu/vl53l1x-arduino)
// LIB Adafruit_SSD1306 by Adafruit (https://github.com/adafruit/Adafruit_SSD1306)
// LIB RadioLib by Jan Gromes (https://github.com/jgromes/RadioLib)

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Preferences.h>
#include <RadioLib.h>
#include <Wire.h>
#include <math.h>
#include <secrets.h>

// Function declarations
void blink(int cnt, int time);
void ledOn();
void ledOff();
void goToDeepSleep();


void setup() {
    Serial.begin(115200);
    blink(30, 200);
}

void loop() {
    goToDeepSleep();
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
    //disableDisplay();
    //dsensor.disable();
    //radio.sleep();
    // Configure deep sleep wake-up timer
    esp_sleep_enable_timer_wakeup(10000LL * 1000LL);
    // Configure deep sleep wake-up button
    //esp_sleep_enable_ext1_wakeup(1ULL << DSLEEP_WAKEUP_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
    // Store non-volatile variables
    //preferences.end();
    // Close down Serial
    Serial.println(F("Sleeping..."));
    Serial.flush();
    Serial.end();
    // Enter deep sleep
    esp_deep_sleep_start();
    // Should never get here
    for (;;);
}
