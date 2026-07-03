#include <Arduino.h>
#include "low_power.h"
#include "sensor.h"
#include "lora.h"
#include "battery.h"

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

#ifndef LOW_VOLTAGE_TRIGGER_MV
#define LOW_VOLTAGE_TRIGGER_MV 3200U
#endif

#ifndef LOW_VOLTAGE_RECOVERY_HYSTERESIS_MV
#define LOW_VOLTAGE_RECOVERY_HYSTERESIS_MV 100U
#endif

#ifndef PAYLOAD_VERSION
#define PAYLOAD_VERSION 3
#endif

// Globals (Wiped on battery disconnect or physical RST press)
float lastSentDistance = -1.0;
uint32_t lastSentMillis = 0;
bool lowVoltageAlertLatched = false;

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

bool loraTransmitWithRetries(float distance, uint16_t voltageMv) {
    const uint16_t rangeMm = static_cast<uint16_t>((distance * 1000.0f) + 0.5f);
    const uint16_t bootCount = 0;

    uint8_t payload[7];
    payload[0] = PAYLOAD_VERSION;
    payload[1] = highByte(rangeMm);
    payload[2] = lowByte(rangeMm);
    payload[3] = highByte(voltageMv);
    payload[4] = lowByte(voltageMv);
    payload[5] = highByte(bootCount);
    payload[6] = lowByte(bootCount);

    const bool sent = loraTransmit(payload, sizeof(payload));
    if (!sent) {
        Serial.println("[LoRaWAN] Send failed; retries exhausted or join not available in this cycle.");
    }
    return sent;
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

    const uint16_t batteryMv = measureBatteryVoltageMv();
    Serial.print("[Power] Measured voltage: ");
    Serial.print(batteryMv);
    Serial.println(" mV");

    const bool lowVoltageFeatureEnabled = (LOW_VOLTAGE_TRIGGER_MV > 0U);
    if (lowVoltageFeatureEnabled && lowVoltageAlertLatched) {
        const uint32_t recoveryMv = static_cast<uint32_t>(LOW_VOLTAGE_TRIGGER_MV) +
                                    static_cast<uint32_t>(LOW_VOLTAGE_RECOVERY_HYSTERESIS_MV);
        if (batteryMv >= recoveryMv) {
            lowVoltageAlertLatched = false;
            Serial.print("[Power] Low-voltage alert re-armed after recovery to ");
            Serial.print(batteryMv);
            Serial.println(" mV.");
        }
    }

    const bool lowVoltageTrigger = lowVoltageFeatureEnabled &&
                                   !lowVoltageAlertLatched &&
                                   (batteryMv > 0U) &&
                                   (batteryMv <= LOW_VOLTAGE_TRIGGER_MV);

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
        bool shouldSend = firstRun ||
                  (delta >= SIGNIFICANT_CHANGE_THRESHOLD) ||
                  heartbeatDue ||
                  lowVoltageTrigger;

        if (!shouldSend) {
            Serial.print("Status: Stable. Delta (");
            Serial.print(delta, 3);
            Serial.println(" m) stable. Skipping.");
        } else {
            if (firstRun) {
                Serial.println("Status: Initial boot / Reset caught. Syncing baseline...");
            } else if (delta >= SIGNIFICANT_CHANGE_THRESHOLD) {
                Serial.print("Status: Delta (");
                Serial.print(delta, 3);
                Serial.println(" m) triggers update!");
            } else if (lowVoltageTrigger) {
                Serial.print("Status: Low voltage alert threshold crossed (");
                Serial.print(batteryMv);
                Serial.print(" mV <= ");
                Serial.print(LOW_VOLTAGE_TRIGGER_MV);
                Serial.println(" mV). Sending one-shot alert.");
            } else {
                Serial.println("Status: Heartbeat interval elapsed. Sending keep-alive update.");
            }

            const bool sent = loraTransmitWithRetries(currentDistance, batteryMv);
            if (sent) {
                lastSentDistance = currentDistance;
                lastSentMillis = now;
                if (lowVoltageTrigger) {
                    lowVoltageAlertLatched = true;
                }
            } else {
                Serial.println("Status: Transmission failed after retries. Baseline preserved for next wake cycle.");
            }
        }
    }

    Serial.println("Cycle complete. Suspending core execution...");
    Serial.flush();
    
    // We did our job, we can go back to sleep
    setWakeLedState(false);
    goToSleep(WAKE_INTERVAL_MS);
}