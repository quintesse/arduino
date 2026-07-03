#include <Arduino.h>
#include <STM32LoRaWAN.h>

#include "lora.h"

#ifndef LORAWAN_REGION
#define LORAWAN_REGION EU868
#endif

#ifndef LORAWAN_UPLINK_PORT
#define LORAWAN_UPLINK_PORT 10
#endif

#ifndef LORAWAN_CONFIRMED_UPLINK
#define LORAWAN_CONFIRMED_UPLINK 1
#endif

#ifndef TTN_APP_EUI
#define TTN_APP_EUI "0000000000000000"
#endif

#ifndef TTN_APP_KEY
#define TTN_APP_KEY "00000000000000000000000000000000"
#endif

#ifndef TTN_DEV_EUI
#define TTN_DEV_EUI ""
#endif

namespace {
LoRaModem modem;
bool modemInitialized = false;

bool isAllZeroHex(const char *value) {
    if (value == nullptr || value[0] == '\0') {
        return true;
    }

    for (size_t i = 0; value[i] != '\0'; ++i) {
        if (value[i] != '0') {
            return false;
        }
    }

    return true;
}

bool hasValidTtnCredentials() {
    return !isAllZeroHex(TTN_APP_EUI) && !isAllZeroHex(TTN_APP_KEY);
}

bool ensureModemInitialized() {
    if (modemInitialized) {
        return true;
    }

    Serial.println("[LoRaWAN] Initializing modem...");
    if (!modem.begin(LORAWAN_REGION)) {
        Serial.println("[LoRaWAN] Modem initialization failed.");
        return false;
    }

    modem.setPort(LORAWAN_UPLINK_PORT);
    modemInitialized = true;
    return true;
}

bool ensureJoinedToNetwork() {
    if (modem.connected()) {
        return true;
    }

    if (!hasValidTtnCredentials()) {
        Serial.println("[LoRaWAN] Missing TTN OTAA credentials. Set TTN_APP_EUI and TTN_APP_KEY build flags.");
        return false;
    }

    Serial.println("[LoRaWAN] Joining network (OTAA)...");

    bool joined = false;
    if (TTN_DEV_EUI[0] != '\0' && !isAllZeroHex(TTN_DEV_EUI)) {
        joined = modem.joinOTAA(TTN_APP_EUI, TTN_APP_KEY, TTN_DEV_EUI);
    } else {
        joined = modem.joinOTAA(TTN_APP_EUI, TTN_APP_KEY);
    }

    if (!joined) {
        Serial.println("[LoRaWAN] OTAA join failed.");
        return false;
    }

    Serial.print("[LoRaWAN] Joined. DevEUI: ");
    Serial.println(modem.deviceEUI());
    return true;
}
} // namespace

LoraTransmitResult loraTransmit(float distance) {
    char payload[16];
    const int payloadLen = snprintf(payload, sizeof(payload), "%.3f", distance);
    if (payloadLen <= 0) {
        Serial.println("[LoRaWAN] Failed to build payload.");
        return LoraTransmitResult::FatalFailure;
    }

    if (!ensureModemInitialized()) {
        return LoraTransmitResult::FatalFailure;
    }

    if (!ensureJoinedToNetwork()) {
        if (!hasValidTtnCredentials()) {
            return LoraTransmitResult::FatalFailure;
        }
        return LoraTransmitResult::RetryableFailure;
    }

    modem.beginPacket();
    modem.write(reinterpret_cast<const uint8_t *>(payload), static_cast<size_t>(payloadLen));
    const int sent = modem.endPacket(LORAWAN_CONFIRMED_UPLINK != 0);

    if (sent == payloadLen) {
        Serial.print("[LoRaWAN] Uplink sent successfully: ");
        Serial.print(payload);
        Serial.println(" m");
        return LoraTransmitResult::Success;
    }

    Serial.println("[LoRaWAN] Uplink failed.");
    return LoraTransmitResult::RetryableFailure;
}
