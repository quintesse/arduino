/* This code manages the motor of a simple toy train */

#include <Arduino.h>
#include <Wire.h>
#include "Motor.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>

const int maxSpeed = 200;
const int backupSpeed = -64;
const int backupTime = 3000;
const int sensorReps = 4;

const int baseMagnetValue = 1648;
const int magnetThreshold = baseMagnetValue * 0.015;

enum State { STOPPED, STARTING, MOVING, BREAKING, BACKINGUP, ERROR };

// Function declarations
void handleRoot(AsyncWebServerRequest *request);
void handleSensor(AsyncWebServerRequest *request);
void handleSpeed(AsyncWebServerRequest *request);
void handleLights(AsyncWebServerRequest *request);
bool magnetSensed(bool sensed);
bool checkSensor();
void toState(State newState);
const char* stateName(State state);
unsigned long elapsed();
void blink(int cnt, int time);
void ledOn();
void ledOff();
void goToDeepSleep();

WiFiManager wm;
AsyncWebServer server(80);

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

State state = STOPPED;
unsigned long lastStateChange = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("===================="));
  Serial.println(F("Toy Train Controller"));

  Wire.begin();

  motor.drive(maxSpeed);
  Serial.println(F("Turned on motor at full speed"));
  toState(STARTING);

  leds.drive(25);
  Serial.println(F("Turned on LEDs"));

  pinMode(ANALOG_SENSOR1, INPUT);

  if (!SPIFFS.begin(true)){
    Serial.println("SPIFFS not found, no network");
    return;
  }
  
  WiFi.mode(WIFI_STA);
  wm.setConfigPortalBlocking(false);
  bool res = wm.autoConnect("IoT-Train-Setup", "iottrain"); // password protected ap
  if (!res) {
    Serial.println("Failed to connect to WiFi network");
    return;
  }

  Serial.println("Connected to WiFi network");

  if (!MDNS.begin("iottrain")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    MDNS.setInstanceName("IoT Toy Train Controller");
    Serial.println("Train available at http://iottrain.local");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/index.html", HTTP_GET, handleRoot);
  server.on("/sensor", HTTP_GET, handleSensor);
  server.on("/speed", HTTP_GET, handleSpeed);
  server.on("/lights", HTTP_GET, handleLights);
  
  server.begin();
}

void handleRoot(AsyncWebServerRequest *request) {
  Serial.println("GET /");
  request->send(SPIFFS, "/index.html", "text/html");
}

void handleSensor(AsyncWebServerRequest *request) {
  Serial.println("GET /sensor");
  String ptr = "Sensor <i class='fa fa-magnet' style='font-size:24px";
  ptr += checkSensor() ? ";color:red" : "";
  ptr += "'></i>";
  ptr += analogRead(ANALOG_SENSOR1);
  request->send(200, "text/html", ptr);
}

void handleSpeed(AsyncWebServerRequest *request) {
  Serial.print("GET /speed");
  if (request->hasParam("action")) {
    String action = request->getParam("action")->value();
    Serial.print("?action=");
    Serial.println(action);
    if (action == "faster") {
      motor.drive(motor.speed() + 40);
    } else if (action == "slower") {
      motor.drive(motor.speed() - 40);
    } else if (action == "stop") {
      motor.brake();
    }
  } else {
    Serial.println();
  }
  int speed = motor.speed();
  String ptr = "";
  if (speed == 0) {
    ptr += "<i class='fa-regular fa-hand'></i>";
  } else {
    for (int i = 0; i < abs(speed); i += 40) {
      if (speed < 0) {
        ptr += "<i class='fa-solid fa-caret-left' style='font-size:24px;color:red'></i>";
      } else {
        ptr += "<i class='fa-solid fa-caret-right' style='font-size:24px;color:green'></i>";
      }
    }
  }
  request->send(200, "text/html", ptr);
}

void handleLights(AsyncWebServerRequest *request) {
  Serial.print("GET /lights");
  if (request->hasParam("action")) {
    String action = request->getParam("action")->value();
    Serial.print("?action=");
    Serial.println(action);
    if (action == "full") {
      leds.drive(25);
    } else if (action == "low") {
      leds.drive(5);
    } else if (action == "off") {
      leds.brake();
    }
  } else {
    Serial.println();
  }
  int brightness = leds.speed();
  String ptr = "";
  ptr += brightness;
  request->send(200, "text/plain", ptr);
}

void loop() {
  State newState = state;
  if (state == STARTING) {
    if (elapsed() > 500) {
      Serial.println(F("Starting to look for magnets..."));
      newState = MOVING;
    }
  } else if (state == MOVING) {
    if (magnetSensed(true)) {
      Serial.println(F("Magnet sensed, breaking!"));
      motor.brake();
      newState = BREAKING;
    }
  } else if (state == BREAKING) {
    if (magnetSensed(false)) {
      // Magnet not sensed anymore, so we overshot, let's back up
      Serial.println(F("Backing up"));
      motor.drive(backupSpeed);
      newState = BACKINGUP;
    } else if (elapsed() > 500) {
      // We've been breaking for a while and are still sensing the magnet,
      // so let's assume we stopped on top of it
      Serial.println(F("Stopped"));
      newState = STOPPED;      
    }
  } else if (state == BACKINGUP) {
    if (magnetSensed(true)) {
      Serial.println(F("Magnet sensed, stopping"));
      motor.brake();
      newState = STOPPED;
    } else if (elapsed() > backupTime) {
      Serial.println(F("Magnet not sensed, stopping"));
      motor.brake();
      newState = ERROR;
    }
  } else if (state == STOPPED) {
    if (magnetSensed(false)) {
      Serial.println(F("No magnet sensed, let's start moving again"));
      motor.drive(maxSpeed);
      newState = STARTING;
    }
  } else if (state == ERROR) {
    if (elapsed() / 300 % 2 == 0) {
      leds.drive(25);
    } else {
      leds.drive(0);
    }
  }
  toState(newState);
  delay(10);
  wm.process();
}

bool magnetSensed(bool sensed) {
  for (int i = 0; i < sensorReps; i++) {
    if (sensed == !checkSensor()) {
      return false;
    }
  }
  return true;
}

bool checkSensor() {
  int sensorValue = analogRead(ANALOG_SENSOR1);
  if (sensorValue <= (baseMagnetValue - magnetThreshold) || sensorValue >= (baseMagnetValue + magnetThreshold)) {
    Serial.print(F("Sensor value: "));
    Serial.println(sensorValue);
    return true;
  }
  return false;
}

void toState(State newState) {
  if (newState == state) {
    return;
  }
  Serial.print(F("New state: "));
  Serial.println(stateName(newState));
  state = newState;
  lastStateChange = millis();
}

const char* stateName(State state) {
  switch (state) {
    case STOPPED:
      return "STOPPED";
    case STARTING:
      return "STARTING";
    case MOVING:
      return "MOVING";
    case BREAKING:
      return "BREAKING";
    case BACKINGUP:
      return "BACKINGUP";
    case ERROR:
      return "ERROR";
  }
  return "UNKNOWN";
}

// Returns the number of milliseconds since the last state change
unsigned long elapsed() {
  return millis() - lastStateChange;
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