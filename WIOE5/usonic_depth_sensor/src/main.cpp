#include <Arduino.h>
#include "low_power.h"
#include "sensor.h"
#include "lora.h"

// 2. Telemetry Timing & Sensitivity Settings
const uint32_t TESTING_INTERVAL_MS = 15000;      // Wake interval in milliseconds
const float SIGNIFICANT_CHANGE_THRESHOLD = 0.05;   // 5 centimeters threshold

// 3. Volatile baseline (Wiped on battery disconnect or physical RST press)
float lastSentDistance = -1.0;

void setup() {
    Serial.begin(115200);
    initializeSensor();
    goToSleep(2000U);
    
    Serial.println("==========================================");
    Serial.println("   TELEMETRY NODE: VOLATILE RESET STATE   ");
    Serial.println("==========================================");
    Serial.println("Notice: Hitting the RST button forces an instant uplink.");
}

void loop() {
    Serial.println("\n--- Core Wake Cycle ---");

    float currentDistance = readSensorDistance();

    if (currentDistance < 0) {
        Serial.println("Telemetry Error: Sensor frame timeout.");
    } else {
        Serial.print("Current Reading: "); Serial.print(currentDistance, 3); Serial.println(" m");

        bool firstRun = (lastSentDistance < 0);
        float delta = abs(currentDistance - lastSentDistance);

        if (firstRun) {
            Serial.println("Status: Initial boot / Reset caught. Syncing baseline...");
            loraTransmit(currentDistance);
            lastSentDistance = currentDistance;
        } 
        else if (delta >= SIGNIFICANT_CHANGE_THRESHOLD) {
            Serial.print("Status: Delta ("); 
            Serial.print(delta, 3); 
            Serial.println(" m) triggers update!");
            loraTransmit(currentDistance);
            lastSentDistance = currentDistance;
        } 
        else {
            Serial.print("Status: Stable. Delta ("); 
            Serial.print(delta, 3); 
            Serial.println(" m) stable. Skipping.");
        }
    }

    Serial.println("Cycle complete. Suspending core execution...");
    Serial.flush();
    
    // Call the isolated low-power management function
    goToSleep(TESTING_INTERVAL_MS);
}