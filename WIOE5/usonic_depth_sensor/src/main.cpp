#include <Arduino.h>
#include <HardwareSerial.h>

// Hardware Pin Definitions
#define SENSOR_PWR_PIN  PB9   // Managed 3.3V power line to the sensor

// Explicitly instantiate a dedicated Hardware Serial instance for the sensor
// Constructor syntax: HardwareSerial ObjectName(RX_PIN, TX_PIN);
// We pass NC (Not Connected) for TX since the sensor only transmits to us.
HardwareSerial sensorSerial(PC0, NC);

void setup() {
    // Initialize Debug Console (UART2 via PA2/PA3 to your ST-Link V3)
    Serial.begin(115200);
    delay(2000);
    Serial.println("--- Milestone 2: Custom Hardware Serial Sensor Test ---");

    // Configure the physical PB9 pin as our power switch
    pinMode(SENSOR_PWR_PIN, OUTPUT);
    digitalWrite(SENSOR_PWR_PIN, LOW); // Keep it off initially
}

void loop() {
    Serial.println("\n--- Starting Telemetry Cycle ---");
    
    // 1. Power on the sensor via PB9
    digitalWrite(SENSOR_PWR_PIN, HIGH);
    Serial.println("Sensor Power Rails: Energized (PB9 HIGH)");
    
    // 2. Settle Delay (Let the sensor boot up)
    delay(300); 
    
    // 3. Open our custom hardware serial instance at 9600 baud
    sensorSerial.begin(9600);
    
    // Flush out any power-on transition noise in the buffer
    while(sensorSerial.available()) { sensorSerial.read(); }
    delay(50); 
    
    uint16_t distance = 0;
    bool validRead = false;
    uint8_t buffer[4] = {0};
    uint32_t startTime = millis();

    // 4. Telemetry Capture Loop (1-second timeout safety)
    while ((millis() - startTime < 1000) && !validRead) {
        if (sensorSerial.available() >= 4) {
            if (sensorSerial.read() == 0xFF) {
                buffer[0] = 0xFF;
                buffer[1] = sensorSerial.read();
                buffer[2] = sensorSerial.read();
                buffer[3] = sensorSerial.read();

                // Compute Checksum Verification
                uint8_t checksum = (buffer[0] + buffer[1] + buffer[2]) & 0xFF;
                if (checksum == buffer[3]) {
                    distance = (buffer[1] << 8) + buffer[2];
                    validRead = true;
                }
            }
        }
    }

    // 5. Sensor Kill-Switch (Shut down hardware engine and drop line power)
    sensorSerial.end(); 
    digitalWrite(SENSOR_PWR_PIN, LOW);
    Serial.println("Sensor Power Rails: Isolated (PB9 LOW)");

    // 6. Business Logic Evaluation
    if (validRead) {
        Serial.print("Measured Distance: ");
        Serial.print(distance);
        Serial.println(" mm");

        if (distance >= 1050) {
            Serial.println("Decision: Distance >= 1050mm (Tank treated as Empty). Skip radio wake.");
        } else if (distance < 900) {
            Serial.println("Decision: Distance < 900mm (Filling Up). LoRaWAN Wake-up Required!");
        } else {
            Serial.println("Decision: Mid-zone. System will enter next deep sleep cycle.");
        }
    } else {
        Serial.println("Error: No valid hardware data frame received within timeout window.");
    }

    Serial.println("Waiting 10 seconds for next cycle...");
    delay(10000);
}