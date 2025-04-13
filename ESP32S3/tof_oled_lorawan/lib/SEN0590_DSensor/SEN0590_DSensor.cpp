
#include "SEN0590_DSensor.h"

SEN0590_DSensor::SEN0590_DSensor() {
}

uint16_t SEN0590_DSensor::read() {
    uint16_t distance = sen0590.readDistance();
    if (distance > 10) {
        return distance;
    } else {
        return IDistanceSensor::INVALID_RANGE;
    }
}

bool SEN0590_DSensor::init() {
    return true;
}

void SEN0590_DSensor::enable() {
}

void SEN0590_DSensor::disable() {
}
