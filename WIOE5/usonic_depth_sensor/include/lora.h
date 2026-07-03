#pragma once

#include <Arduino.h>

enum class LoraTransmitResult : uint8_t {
	Success,
	RetryableFailure,
	FatalFailure,
};

/**
 * @brief Transmit distance reading over LoRaWAN.
 *
 * Uses OTAA join credentials from build flags and performs a single send attempt.
 * @param distance Distance in meters to transmit
 * @return Transmission outcome, including whether failure is retryable.
 */
LoraTransmitResult loraTransmit(float distance);
