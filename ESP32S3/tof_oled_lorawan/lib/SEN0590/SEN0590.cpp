
#include "SEN0590.h"
#include <Wire.h>

SEN0590::SEN0590() {
    Wire.begin();
}

uint16_t SEN0590::readDistance() {
    uint8_t dat = 0xB0;
    uint8_t buf[2] = {0};

    writeReg(0x10, &dat, 1);
    delay(50);
    readReg(0x02, buf, 2);
    uint16_t distance = buf[0] * 0x100 + buf[1] + 10;

    return distance;
}

uint8_t SEN0590::readReg(uint8_t reg, const void *pBuf, size_t size) {
    if (pBuf == NULL) {
        Serial.println("pBuf ERROR!! : null pointer");
    }
    uint8_t *_pBuf = (uint8_t *)pBuf;
    Wire.beginTransmission(SEN0590_ADDRESS);
    Wire.write(&reg, 1);
    if (Wire.endTransmission() != 0) {
        Serial.println("ERROR: Sensor read failed!");
        return 0;
    }
    delay(20);
    Wire.requestFrom(SEN0590_ADDRESS, (uint8_t)size);
    for (uint16_t i = 0; i < size; i++) {
        _pBuf[i] = Wire.read();
    }
    return size;
}

bool SEN0590::writeReg(uint8_t reg, const void *pBuf, size_t size) {
    if (pBuf == NULL) {
        Serial.println("pBuf ERROR!! : null pointer");
    }
    uint8_t *_pBuf = (uint8_t *)pBuf;
    Wire.beginTransmission(SEN0590_ADDRESS);
    Wire.write(&reg, 1);

    for (uint16_t i = 0; i < size; i++) {
        Wire.write(_pBuf[i]);
    }
    if (Wire.endTransmission() != 0) {
        Serial.println("ERROR: Sensor write failed!");
        return 0;
    } else {
        return 1;
    }
}
