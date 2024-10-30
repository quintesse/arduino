/* This example shows how to take
range measurements with the VL53L0X and display on a SSD1306 OLED.

The range readings are in units of mm. */

// LIB Adafruit_VL53L0X by Adafruit (https://github.com/adafruit/Adafruit_VL53L0X)
// LIB Adafruit_SSD1306 by Adafruit (https://github.com/adafruit/Adafruit_SSD1306)

#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display = Adafruit_SSD1306();

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

#if (SSD1306_LCDHEIGHT != 32)
 #error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  delay(200);
  Serial.println(F("Starting..."));

  Wire.begin();

  // Initialize SSD1306 OLED display
  Serial.println(F("Display setup..."));
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  // Display Adafruit logo
  display.display();
  delay(1000);

  // text display big!
  display.setTextSize(2);
  display.setTextColor(WHITE);

  // Initialize VL53L0X time-of-flight sensor
  Serial.println(F("Time-of-flight sensor setup..."));
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("No Sensor!");
    display.display();
    for(;;);
  }
  lox.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE);
}

void loop() {
  VL53L0X_RangingMeasurementData_t measure;

  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  display.clearDisplay();
  display.setCursor(0,0);
  // check for phase failures and invalid values
  if (measure.RangeStatus != 4 && measure.RangeMilliMeter < 8000) {
    display.print(measure.RangeMilliMeter);
    display.print("mm");
    display.display();
    delay(50);
  } else {
    display.print("no data #");
    display.print(measure.RangeStatus);
    display.display();
  }
}
