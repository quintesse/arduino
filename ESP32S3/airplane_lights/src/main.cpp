#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// WiFi Credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Mapping for XIAO ESP32-S3
const int pinA = D6; // Steady Group
const int pinB = D5; // Double Strobe Group
const int pinC = D4; // Long Pulse Group

WebServer server(80);

bool lightsActive;
bool startLightsActive = true;

unsigned long startMillis;

// HTML for the Web Interface
void handleRoot() {
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>button{width:100%;height:100px;font-size:24px;background:#007bff;color:white;border:none;border-radius:10px;}</style></head>";
  html += "<body><h1>Airplane Controller</h1>";
  html += "<button onclick=\"location.href='/toggle'\">TOGGLE LIGHTS</button>";
  html += "<p>Status: " + String(lightsActive ? "ANIMATED" : "OFF") + "</p></body></html>";
  server.send(200, "text/html", html);
}

void setLights(bool active) {
  lightsActive = active;
  int state = active ? HIGH : LOW;
  digitalWrite(pinA, state);
  digitalWrite(pinB, state);
  digitalWrite(pinC, state);
}

void handleToggle() {
  setLights(!lightsActive);
  // Redirect back to root after toggle
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  pinMode(pinA, OUTPUT);
  pinMode(pinB, OUTPUT);
  pinMode(pinC, OUTPUT);
  setLights(false);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(pinB, HIGH);
    delay(250);
    digitalWrite(pinB, LOW);
    delay(250);
    Serial.print(".");
  }

  if (MDNS.begin("airplane")) {
    Serial.println("MDNS responder started: http://airplane.local");
  }

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();

  setLights(false);
  lightsActive = startLightsActive;
  startMillis = millis();
}

void loop() {
  server.handleClient();

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