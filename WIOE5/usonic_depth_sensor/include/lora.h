#pragma once

#include <Arduino.h>

/**
 * @brief Transmit an uplink payload over LoRaWAN.
 *
 * Uses OTAA join credentials from build flags and handles join/transmit retries
 * according to the configured build flags.
 * @param payload Pointer to payload bytes
 * @param payloadLen Number of bytes to transmit
 * @return true when payload was sent successfully, false otherwise.
 */
bool loraTransmit(const uint8_t *payload, size_t payloadLen);
