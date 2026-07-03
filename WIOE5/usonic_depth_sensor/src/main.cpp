#include <Arduino.h>
#include "low_power.h"
#include "sensor.h"
#include "lora.h"

// Telemetry Timing & Sensitivity Settings
#ifndef WAKE_INTERVAL_MS
#define WAKE_INTERVAL_MS 24 * 60 * 60 * 1000UL // 24 hours
#endif

#ifndef HEARTBEAT_INTERVAL_MS
#define HEARTBEAT_INTERVAL_MS 2592000000UL // 30 days
#endif

#ifndef SIGNIFICANT_CHANGE_THRESHOLD
#define SIGNIFICANT_CHANGE_THRESHOLD 0.05f // 5 cm
#endif

// Globals (Wiped on battery disconnect or physical RST press)
float lastSentDistance = -1.0;
uint32_t lastSentMillis = 0;

namespace {
bool wakeLedReady = false;

void setWakeLedState(bool isAwake) {
#if defined(LED_BUILTIN)
    if (!wakeLedReady) {
        pinMode(LED_BUILTIN, OUTPUT);
        wakeLedReady = true;
    }

    digitalWrite(LED_BUILTIN, isAwake ? LOW : HIGH);
#else
    (void)isAwake;
#endif
}
} // namespace

void setup() {
    setWakeLedState(true);
    Serial.begin(115200);
    initializeSensor();
    goToSleep(2000U);
    
    Serial.println("==============================================");
    Serial.println("   LORA ENABLED DEPTH SENSOR TELEMETRY UNIT   ");
    Serial.println("==============================================");
    Serial.println("Notice: Hitting the RST button forces an instant uplink.");
}

void loop() {
    setWakeLedState(true);
    Serial.println("\n--- Core Wake Cycle ---");

    float currentDistance = readSensorDistance();

    if (currentDistance < 0) {
        Serial.println("Telemetry Error: Sensor frame timeout.");
    } else {
        Serial.print("Current Reading: ");
        Serial.print(currentDistance, 3);
        Serial.println(" m");

        uint32_t now = millis();
        bool firstRun = (lastSentDistance < 0);
        float delta = abs(currentDistance - lastSentDistance);
        bool heartbeatDue = !firstRun && ((uint32_t)(now - lastSentMillis) >= HEARTBEAT_INTERVAL_MS);

        if (firstRun) {
            Serial.println("Status: Initial boot / Reset caught. Syncing baseline...");
            loraTransmit(currentDistance);
            lastSentDistance = currentDistance;
            lastSentMillis = now;
        } else if (delta >= SIGNIFICANT_CHANGE_THRESHOLD) {
            Serial.print("Status: Delta ("); 
            Serial.print(delta, 3); 
            Serial.println(" m) triggers update!");
            loraTransmit(currentDistance);
            lastSentDistance = currentDistance;
            lastSentMillis = now;
        } else if (heartbeatDue) {
            Serial.println("Status: Heartbeat interval elapsed. Sending keep-alive update.");
            loraTransmit(currentDistance);
            lastSentDistance = currentDistance;
            lastSentMillis = now;
        } else {
            Serial.print("Status: Stable. Delta ("); 
            Serial.print(delta, 3); 
            Serial.println(" m) stable. Skipping.");
        }
    }

    Serial.println("Cycle complete. Suspending core execution...");
    Serial.flush();
    
    // We did our job, we can go back to sleep
    setWakeLedState(false);
    goToSleep(WAKE_INTERVAL_MS);
}