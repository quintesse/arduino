#include <Arduino.h>
#include <HardwareSerial.h>
#include "sensor.h"

// Hardware Serial instance for sensor communication
HardwareSerial sensorSerial(PC0, PC1);

void initializeSensor() {
    sensorSerial.begin(9600);
}

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
