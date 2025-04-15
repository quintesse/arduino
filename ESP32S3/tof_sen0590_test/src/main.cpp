#include <Arduino.h>
#include "Wire.h"

// Example code using the SEN0590_DSensor library
#include "SEN0590_DSensor.h"

void setup() {
  Serial.begin(115200);
  Wire.begin();
}

IDistanceSensor *sensor = new SEN0590_DSensor();

void loop() {
  uint16_t distance = sensor->read();
  Serial.print("distance=");
  Serial.print(distance);
  Serial.print("mm");
  Serial.println("\t");
  delay(100);
}

/*
// Example code using the SEN0590 library

#include "SEN0590.h"

void setup() {
  Serial.begin(115200);
  Wire.begin();
}

SEN0590 sensor;

void loop() {
  uint16_t distance = sensor.readDistance();
  Serial.print("distance=");
  Serial.print(distance);
  Serial.print("mm");
  Serial.println("\t");
  delay(100);
}
*/

/*
// Basic example code

#define address 0x74

uint8_t buf[2] = { 0 };

uint8_t dat = 0xB0;
int distance = 0;

uint8_t readReg(uint8_t reg, const void* pBuf, size_t size);
bool writeReg(uint8_t reg, const void* pBuf, size_t size);

void loop() {
  writeReg(0x10, &dat, 1);
  delay(50);
  readReg(0x02, buf, 2);
  distance = buf[0] * 0x100 + buf[1] + 10;
  Serial.print("distance=");
  Serial.print(distance);
  Serial.print("mm");
  Serial.println("\t");
  delay(100);
}

uint8_t readReg(uint8_t reg, const void* pBuf, size_t size) {
  if (pBuf == NULL) {
    Serial.println("pBuf ERROR!! : null pointer");
  }
  uint8_t* _pBuf = (uint8_t*)pBuf;
  Wire.beginTransmission(address);
  Wire.write(&reg, 1);
  if (Wire.endTransmission() != 0) {
    Serial.println("Read fail!!");
    return 0;
  }
  delay(20);
  Wire.requestFrom(address, (uint8_t)size);
  for (uint16_t i = 0; i < size; i++) {
    _pBuf[i] = Wire.read();
  }
  return size;
}

bool writeReg(uint8_t reg, const void* pBuf, size_t size) {
  if (pBuf == NULL) {
    Serial.println("pBuf ERROR!! : null pointer");
  }
  uint8_t* _pBuf = (uint8_t*)pBuf;
  Wire.beginTransmission(address);
  Wire.write(&reg, 1);

  for (uint16_t i = 0; i < size; i++) {
    Wire.write(_pBuf[i]);
  }
  if (Wire.endTransmission() != 0) {
    Serial.println("Write fail!!");
    return 0;
  } else {
    return 1;
  }
}
*/