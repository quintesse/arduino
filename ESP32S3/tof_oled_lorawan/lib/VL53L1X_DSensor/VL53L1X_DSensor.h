#pragma once

#include <DistanceSensor.h>
#include <VL53L1X.h>

class VL53L1X_DSensor : public IDistanceSensor {
   public:
    VL53L1X_DSensor();
    uint16_t read();
    bool init();
    void enable();
    void disable();

   private:
    VL53L1X sensor;
    VL53L1X::RangingData measure;
};
