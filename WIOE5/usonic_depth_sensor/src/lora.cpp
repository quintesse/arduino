#include <Arduino.h>
#include <STM32LoRaWAN.h>

#include "lora.h"

#ifndef LORAWAN_REGION
#define LORAWAN_REGION EU868
#endif

#ifndef LORAWAN_UPLINK_PORT
#define LORAWAN_UPLINK_PORT 2
#endif

#ifndef LORAWAN_CONFIRMED_UPLINK
#define LORAWAN_CONFIRMED_UPLINK 0
#endif

#ifndef LORAWAN_JOIN_MAX_RETRIES
#define LORAWAN_JOIN_MAX_RETRIES 3
#endif

#ifndef LORAWAN_JOIN_RETRY_DELAY_MS
#define LORAWAN_JOIN_RETRY_DELAY_MS 2000UL
#endif

#ifndef LORAWAN_TX_MAX_RETRIES
#define LORAWAN_TX_MAX_RETRIES 3
#endif

#ifndef LORAWAN_TX_RETRY_DELAY_MS
#define LORAWAN_TX_RETRY_DELAY_MS 2000UL
#endif

#ifndef LORAWAN_FAKE_TRANSMIT_SUCCESS
#define LORAWAN_FAKE_TRANSMIT_SUCCESS 0
#endif

#ifndef TTN_APP_EUI
#define TTN_APP_EUI ""
#endif

#ifndef TTN_APP_KEY
#define TTN_APP_KEY "00000000000000000000000000000000"
#endif

#ifndef TTN_NWK_KEY
#define TTN_NWK_KEY ""
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
    // JoinEUI can legitimately be all zeros on some networks (e.g. TTN examples).
    // Require it to be configured (non-empty), but only enforce non-zero for AppKey.
    return TTN_APP_EUI[0] != '\0' && !isAllZeroHex(TTN_APP_KEY);
}

bool hasExplicitTtnNetworkKey() {
    return !isAllZeroHex(TTN_NWK_KEY);
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

    Serial.println("[LoRaWAN] Joining network (OTAA)...");

    if (hasExplicitTtnNetworkKey() && !modem.setNwkKey(TTN_NWK_KEY)) {
        Serial.println("[LoRaWAN] Failed to configure TTN_NWK_KEY.");
        return false;
    }

    if (!modem.setAppEui(TTN_APP_EUI) || !modem.setAppKey(TTN_APP_KEY)) {
        Serial.println("[LoRaWAN] Failed to configure OTAA credentials.");
        return false;
    }

    bool joinStarted = false;
    if (TTN_DEV_EUI[0] != '\0' && !isAllZeroHex(TTN_DEV_EUI)) {
        if (!modem.setDevEui(TTN_DEV_EUI)) {
            Serial.println("[LoRaWAN] Failed to configure TTN_DEV_EUI.");
            return false;
        }
        joinStarted = modem.joinOTAAAsync();
    } else {
        joinStarted = modem.joinOTAAAsync();
    }

    if (!joinStarted) {
        Serial.println("[LoRaWAN] Failed to start OTAA join attempt.");
        return false;
    }

    // Complete a single join attempt only (avoid the library's 60s multi-retry join loop).
    modem.maintainUntilIdle();

    const bool joined = modem.connected();

    if (!joined) {
        Serial.println("[LoRaWAN] OTAA join failed.");
        return false;
    }

    Serial.print("[LoRaWAN] Joined. DevEUI: ");
    Serial.println(modem.deviceEUI());
    return true;
}
} // namespace

bool loraTransmit(const uint8_t *payload, size_t payloadLen) {
    if (payload == nullptr || payloadLen == 0U) {
        Serial.println("[LoRaWAN] Payload is empty.");
        return false;
    }

#if (LORAWAN_FAKE_TRANSMIT_SUCCESS != 0)
    Serial.print("[LoRaWAN] FAKE mode enabled. Pretending uplink success (");
    Serial.print(payloadLen);
    Serial.println(" bytes).");
    delay(2000U);
    return true;
#endif

    if (!hasValidTtnCredentials()) {
        Serial.println("[LoRaWAN] Missing TTN OTAA credentials. Set TTN_APP_EUI and TTN_APP_KEY build flags.");
        return false;
    }

    if (!ensureModemInitialized()) {
        return false;
    }

    bool joined = false;
    for (uint8_t joinAttempt = 1; joinAttempt <= LORAWAN_JOIN_MAX_RETRIES; ++joinAttempt) {
        Serial.print("[LoRaWAN] Join attempt ");
        Serial.print(joinAttempt);
        Serial.print("/");
        Serial.println(LORAWAN_JOIN_MAX_RETRIES);

        joined = ensureJoinedToNetwork();
        if (joined) {
            break;
        }

        if (joinAttempt < LORAWAN_JOIN_MAX_RETRIES) {
            delay(LORAWAN_JOIN_RETRY_DELAY_MS);
        }
    }

    if (!joined) {
        Serial.println("[LoRaWAN] Join retries exhausted for this wake cycle.");
        return false;
    }

    for (uint8_t txAttempt = 1; txAttempt <= LORAWAN_TX_MAX_RETRIES; ++txAttempt) {
        Serial.print("[LoRaWAN] Uplink attempt ");
        Serial.print(txAttempt);
        Serial.print("/");
        Serial.println(LORAWAN_TX_MAX_RETRIES);

        modem.beginPacket();
        modem.write(payload, payloadLen);
        const int sent = modem.endPacket(LORAWAN_CONFIRMED_UPLINK != 0);

        if (sent == static_cast<int>(payloadLen)) {
            Serial.print("[LoRaWAN] Uplink sent successfully (");
            Serial.print(payloadLen);
            Serial.println(" bytes).");
            return true;
        }

        Serial.println("[LoRaWAN] Uplink failed.");
        if (txAttempt < LORAWAN_TX_MAX_RETRIES) {
            delay(LORAWAN_TX_RETRY_DELAY_MS);
        }
    }

    Serial.println("[LoRaWAN] Uplink retries exhausted for this wake cycle.");
    return false;
}
