#pragma once

#include <Arduino.h>

enum class LoraTransmitResult : uint8_t {
	Success,
	RetryableFailure,
	FatalFailure,
};

/**
 * @brief Transmit an uplink payload over LoRaWAN.
 *
 * Uses OTAA join credentials from build flags and performs a single send attempt.
 * @param payload Pointer to payload bytes
 * @param payloadLen Number of bytes to transmit
 * @return Transmission outcome, including whether failure is retryable.
 */
LoraTransmitResult loraTransmit(const uint8_t *payload, size_t payloadLen);
