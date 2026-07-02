#include <Arduino.h>

// The Wio-E5 mini has an onboard user LED usually mapped to LED_BUILTIN (Pin PA0 or PB5 depending on variant)
// Let's explicitly define it or use the macro to be safe.
#ifndef LED_BUILTIN
#define LED_BUILTIN PA0 
#endif

void setup() {
    // Initialize the onboard LED pin as an output
    pinMode(LED_BUILTIN, OUTPUT);

    // Initialize Hardware Serial 1 (Usually mapped to the USB-C bridge or specific TX/RX pins)
    // For Wio-E5 mini, Serial is mapped to UART2 (PA2=TX, PA3=RX) which connects to the USB-C chip,
    // but if you are spying on a specific pin with your CP2102, make sure your wire is on PA2/TX.
    Serial.begin(115200);
    
    // Wait for serial port to connect (only needed for native USB, but good practice)
    delay(2000); 
    Serial.println("--- Wio-E5 Wastewater Telemetry Initialized ---");
}

void loop() {
    // Turn the LED on
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Status: LED ON - System Alive");
    delay(1000);

    // Turn the LED off
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Status: LED OFF - Deep Sleep Simulation");
    delay(1000);
}