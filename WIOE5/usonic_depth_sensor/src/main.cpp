#include <Arduino.h>
#include <HardwareSerial.h>
#include "low_power.h"

// 1. Hardware Pin Configurations
HardwareSerial sensorSerial(PC0, PC1);

// 2. Telemetry Timing & Sensitivity Settings
const uint32_t TESTING_INTERVAL_MS = 15000;      // Wake interval in milliseconds
const float SIGNIFICANT_CHANGE_THRESHOLD = 0.05;   // 5 centimeters threshold

// 3. Volatile baseline (Wiped on battery disconnect or physical RST press)
float lastSentDistance = -1.0; 

float readSensorDistance() {
    uint8_t buffer[4];
    uint8_t bufferIndex = 0;
    uint32_t startTime = millis();

    // Clear anything that arrived while waiting
    while(sensorSerial.available()) sensorSerial.read();

    while (millis() - startTime < 1000) {
        if (sensorSerial.available()) {
            uint8_t incomingByte = sensorSerial.read();
            if (bufferIndex == 0 && incomingByte != 0xFF) continue;
            buffer[bufferIndex++] = incomingByte;

            if (bufferIndex == 4) {
                uint8_t calculatedSum = (buffer[0] + buffer[1] + buffer[2]) & 0xFF;
                if (calculatedSum == buffer[3]) {
                    uint16_t distanceMm = (buffer[1] << 8) | buffer[2];
                    return distanceMm / 1000.0; // Conversion to meters
                }
                bufferIndex = 0;
            }
        }
    }
    return -1.0; // Return error code on timeout
}

void fakeLoraTransmit(float distance) {
    Serial.println("\n>>>>> [RADIO] UPLINK TRANSMISSION IN PROGRESS... <<<<<");
    Serial.print(">>>>> Payload Data: "); 
    Serial.print(distance, 3); 
    Serial.println(" m");
    goToSleep(500U);
    Serial.println(">>>>> [RADIO] Uplink successfully sent.");
}

void setup() {
    Serial.begin(115200);
    sensorSerial.begin(9600);
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
            fakeLoraTransmit(currentDistance);
            lastSentDistance = currentDistance;
        } 
        else if (delta >= SIGNIFICANT_CHANGE_THRESHOLD) {
            Serial.print("Status: Delta ("); 
            Serial.print(delta, 3); 
            Serial.println(" m) triggers update!");
            fakeLoraTransmit(currentDistance);
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