#include <Arduino.h>
#include <HardwareSerial.h>
#include "sensor.h"

#ifndef SENSOR_FAKE_MODE
#define SENSOR_FAKE_MODE 0
#endif

// Hardware Serial instance for sensor communication
HardwareSerial sensorSerial(PC0, PC1);

namespace {
float readSensorDistanceInternal(uint32_t timeoutMs) {
    uint8_t buffer[4];
    uint8_t bufferIndex = 0;
    uint32_t startTime = millis();

    // Clear anything that arrived while waiting
    while (sensorSerial.available()) {
        sensorSerial.read();
    }

    while ((millis() - startTime) < timeoutMs) {
        if (sensorSerial.available()) {
            uint8_t incomingByte = sensorSerial.read();
            if (bufferIndex == 0 && incomingByte != 0xFF) {
                continue;
            }
            buffer[bufferIndex++] = incomingByte;

            if (bufferIndex == 4) {
                uint8_t calculatedSum = (buffer[0] + buffer[1] + buffer[2]) & 0xFF;
                if (calculatedSum == buffer[3]) {
                    uint16_t distanceMm = (static_cast<uint16_t>(buffer[1]) << 8) | buffer[2];
                    return distanceMm / 1000.0f; // Conversion to meters
                }
                bufferIndex = 0;
            }
        }
    }

    return -1.0f; // Return error code on timeout
}
} // namespace

void initializeSensor() {
#if SENSOR_FAKE_MODE
    Serial.println("[Sensor] Fake mode active; skipping hardware UART init.");
    return;
#else
    Serial.println("[Sensor] Initializing UART at 9600 baud.");
    sensorSerial.begin(9600);
    Serial.println("[Sensor] UART init complete.");
#endif
}

bool isSensorConnected(uint32_t timeoutMs) {
#if SENSOR_FAKE_MODE
    (void)timeoutMs;
    return true;
#else
    if (timeoutMs == 0U) {
        timeoutMs = 1000U;
    }
    return readSensorDistanceInternal(timeoutMs) >= 0.0f;
#endif
}

float readSensorDistance() {
#if SENSOR_FAKE_MODE
    return 1.0f;
#else
    return readSensorDistanceInternal(1000U);
#endif
}
