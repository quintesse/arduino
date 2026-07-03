#pragma once

#include <Arduino.h>

/**
 * @brief Transmit distance reading over LoRa radio
 * @param distance Distance in meters to transmit
 */
void loraTransmit(float distance);
