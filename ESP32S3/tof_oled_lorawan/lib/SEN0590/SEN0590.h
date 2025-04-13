#pragma once

#include <Arduino.h>

class SEN0590 {
   public:
    SEN0590();
    uint16_t readDistance();

   private:
    const uint8_t SEN0590_ADDRESS = 0x74;  // I2C address of the SEN0590 sensor

    uint8_t readReg(uint8_t reg, const void *pBuf, size_t size);
    bool writeReg(uint8_t reg, const void *pBuf, size_t size);
};
