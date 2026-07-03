#pragma once

#include <Arduino.h>

/**
 * @brief Initialize the sensor communication interface
 */
void initializeSensor();

/**
 * @brief Read distance from the ultrasonic sensor
 * @return Distance in meters, or -1.0 if read fails or timeout occurs
 */
float readSensorDistance();
