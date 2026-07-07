#pragma once

#include <Arduino.h>

/**
 * @brief Initialize the sensor communication interface
 */
void initializeSensor();

/**
 * @brief Check whether sensor frames can be received within timeout window
 * @param timeoutMs Probe timeout in milliseconds
 * @return true when a valid frame is received, otherwise false
 */
bool isSensorConnected(uint32_t timeoutMs = 1000U);

/**
 * @brief Read distance from the ultrasonic sensor
 * @return Distance in meters, or -1.0 if read fails or timeout occurs
 */
float readSensorDistance();
