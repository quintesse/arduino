
#include "VL53L1X_DSensor.h"

VL53L1X_DSensor::VL53L1X_DSensor() {
    measure.range_status = VL53L1X::RangeStatus::None;  // Set to invalid value
}

uint16_t VL53L1X_DSensor::read() {
    sensor.readSingle(true);
    measure = sensor.ranging_data;
    // check for phase failures and invalid values
    if (measure.range_status == VL53L1X::RangeStatus::RangeValid) {
        return measure.range_mm;
    } else {
        return INVALID_RANGE;
    }
}

bool VL53L1X_DSensor::init() {
    enable();
    delay(50);
    if (!sensor.init()) {
        return false;
    }
    sensor.setDistanceMode(VL53L1X::Long);
    sensor.setMeasurementTimingBudget(75000);
    return true;
}

void VL53L1X_DSensor::enable() {
    // Pin D7 should be connected to the sensor's XSHUT pin
    pinMode(D7, OUTPUT);
    // Pulling the pin high will enable the sensor
    digitalWrite(D7, HIGH);
}

void VL53L1X_DSensor::disable() {
    // Pin D7 should be connected to the sensor's XSHUT pin
    pinMode(D7, OUTPUT);
    // Pulling the pin low will put the sensor in sleep mode
    digitalWrite(D7, LOW);
}
