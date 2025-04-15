#pragma once

#include <Arduino.h>

class IDistanceSensor {
   public:
    static const uint16_t INVALID_RANGE = 0xffff;

    virtual uint16_t read() { return 0xffff; }
    virtual bool init() { return true; }
    virtual void enable() {}
    virtual void disable() {}
};
