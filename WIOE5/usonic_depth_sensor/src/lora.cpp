#include <Arduino.h>
#include "lora.h"
#include "low_power.h"

void loraTransmit(float distance) {
    // TODO: This is fake for now, must be replaced with actual LoRa transmission code
    Serial.println("\n>>>>> [RADIO] UPLINK TRANSMISSION IN PROGRESS... <<<<<");
    Serial.print(">>>>> Payload Data: "); 
    Serial.print(distance, 3); 
    Serial.println(" m");
    goToSleep(500U);
    Serial.println(">>>>> [RADIO] Uplink successfully sent.");
}
