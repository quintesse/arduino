#pragma once

#include <IDistanceSensor.h>
#include <SEN0590.h>

class SEN0590_DSensor : public IDistanceSensor {
   public:
    SEN0590_DSensor();
    uint16_t read();
    bool init();
    void enable();
    void disable();

   private:
    SEN0590 sen0590;
};
