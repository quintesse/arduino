/*
 * This code takes a range measurement using a VL53L1X time-of-flight sensor
 * and if its significantly different than the previous measurement sends it
 * over LoRaWAN using a SX1262 module. Afterwards it goes into a deep sleep
 * for a set amount of time. On first boot or when the user presses a button
 * it will display some information, including the measured range, on an
 * SSD1306 OLED display.
 */

// LIB VL53L1X by Pololu (https://github.com/pololu/vl53l1x-arduino)
// LIB Adafruit_SSD1306 by Adafruit (https://github.com/adafruit/Adafruit_SSD1306)
// LIB RadioLib by Jan Gromes (https://github.com/jgromes/RadioLib)

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Preferences.h>
#include <RadioLib.h>
#include <Wire.h>
#include <math.h>
#include <secrets.h>

// Function declarations
void showAppInfo();
uint16_t readRange();
void showRange(uint16_t range);
void showSubtext(const __FlashStringHelper *msg);
void updateRange(uint16_t range);
bool sendRangeWithRetries(uint16_t range);
bool sendRange(uint16_t range);
bool joinNetwork();
void blink(int cnt, int time);
void ledOn();
void ledOff();
void initDisplay();
void enableDisplay();
void disableDisplay();
void clearDisplayBottom();
void initToFSensor();
void enableToFSensor();
void disableToFSensor();
uint16_t measureBatteryVoltage(int pin);
void goToDeepSleep();

const __FlashStringHelper *APP_NAME = F("Depth Sensor v0.4");

Preferences preferences;

Adafruit_SSD1306 display = Adafruit_SSD1306();
bool displayAvailable = false;

// #define USE_VL53L1X
#define USE_SEN0590
#if defined(USE_VL53L1X)

#include <VL53L1X_DSensor.h>
VL53L1X_DSensor dsensor;

#elif defined(USE_SEN0590)

#include <SEN0590_DSensor.h>
SEN0590_DSensor dsensor;

#else

#error "No distance sensor defined!"

#endif

const uint16_t RANGE_SIGNIFICANT_DELTA = 50;  // 5cm

const unsigned long DSLEEP_MAX_AWAKE_MS = 15000;
const unsigned long DSLEEP_TIME_MS = 24 * 60 * 60 * 1000; // Wake up every 24h
const int DSLEEP_WAKEUP_PIN = 21;  // User button on the Wio-SX1262 shield

const int VMON_PIN = A0;  // Pin we're measuring battery voltage on
uint16_t batterymv = 0;

#if (SSD1306_LCDHEIGHT != 32)
#error ("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

int tries = 5;

// LoRaWan
#define LORAWAN_UPLINK_USER_PORT 2

// regional choices: EU868, US915, AU915, AS923, IN865, KR920, CN780, CN500
const LoRaWANBand_t Region = EU868;
const uint8_t subBand = 0;  // For US915 and AU915

// SX1262 pin order: Module(NSS/CS, DIO1, RESET, BUSY);
SX1262 radio = new Module(41, 39, 42, 40);

// create the LoRaWAN node
LoRaWANNode node(&radio, &Region, subBand);

uint64_t joinEUI = RADIOLIB_LORAWAN_JOIN_EUI;
uint64_t devEUI = RADIOLIB_LORAWAN_DEV_EUI;
uint8_t appKey[] = {RADIOLIB_LORAWAN_APP_KEY};
uint8_t nwkKey[] = {RADIOLIB_LORAWAN_NWK_KEY};

const unsigned int JOIN_MAX_RETRIES = 3;
const unsigned int JOIN_RETRY_DELAY = 15000;

const unsigned int SEND_MAX_RETRIES = 3;
const unsigned int SEND_RETRY_DELAY = 5000;

const unsigned int PAYLOAD_VERSION = 2;

// Non-volatile variables
uint32_t bootCount;
uint16_t lastSharedRange;

void setup() {
    // Read non-volatile variables
    preferences.begin("depthsensor", false);
    bootCount = preferences.getUInt("bootcount", 0);
    lastSharedRange = preferences.getUInt("lastrange", IDistanceSensor::INVALID_RANGE);

    // Update boot count
    bootCount++;
    preferences.putUInt("bootcount", bootCount);

    esp_reset_reason_t reset_reason = esp_reset_reason();
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();

    blink(2, 300);

    Serial.begin(115200);
    delay(200);
    Serial.println(F("===================="));
    Serial.println(F("Starting..."));
    Serial.print(F("Reset reason: "));
    Serial.println(reset_reason);
    Serial.print(F("Wakeup cause: "));
    Serial.println(wakeup_cause);

    // Set pin for voltage monitor
    pinMode(VMON_PIN, INPUT);
    batterymv = measureBatteryVoltage(VMON_PIN);

    Wire.begin();

    if (reset_reason == ESP_RST_POWERON || (reset_reason == ESP_RST_DEEPSLEEP && wakeup_cause == ESP_SLEEP_WAKEUP_EXT1)) {
        initDisplay();
    }
    showAppInfo();
    initToFSensor();
}

void loop() {
    Serial.print(F("Tries: "));
    Serial.println(tries);
    if (tries > 0) {
        tries--;
        uint16_t range = readRange();
        showRange(range);
        if (range != IDistanceSensor::INVALID_RANGE) {
            updateRange(range);
            goToDeepSleep();
        }
    } else {
        // We were not able to get a good reading, going to sleep anyway
        blink(3, 150);
        goToDeepSleep();
    }
    delay(100);
}

void showAppInfo() {
    Serial.println(APP_NAME);
    Serial.print(F("Last depth: "));
    Serial.println((int)round(lastSharedRange / 10.0));
    Serial.print(F("Boot count: "));
    Serial.println(bootCount);
    Serial.print(F("Battery(mV):"));
    Serial.println(batterymv);
if (displayAvailable) {
        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(APP_NAME);
        display.print(F("Last depth: "));
        display.println(lastSharedRange);
        display.print(F("Boot count: "));
        display.println(bootCount);
        display.print(F("Battery(mV):"));
        display.println(batterymv);
        display.display();
        delay(3000);
    }
}

uint16_t readRange() {
    return dsensor.read();
}

void showRange(uint16_t range) {
    if (range != IDistanceSensor::INVALID_RANGE) {
        Serial.print(F("Measured range (mm): "));
        Serial.println(range);
    } else {
        Serial.print(F("Measured range: NO DATA"));
    }
    if (displayAvailable) {
        display.setTextColor(WHITE);
        display.setTextSize(2);
        display.clearDisplay();
        display.setCursor(0, 0);
        if (range != IDistanceSensor::INVALID_RANGE) {
            display.print((int)round(range / 10.0));
            display.print("cm");
            display.display();
            delay(3000);
        } else {
            display.print("no data");
            display.display();
            delay(500);
        }
    }
}

void showSubtext(const __FlashStringHelper *msg) {
    if (displayAvailable) {
        clearDisplayBottom();
        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setCursor(0, display.height() - 8);
        display.print(msg);
        display.display();
        delay(500);
    }
}

void updateRange(uint16_t range) {
    // check if the value actually changed enough
    if (lastSharedRange == IDistanceSensor::INVALID_RANGE || abs(lastSharedRange - range) >= RANGE_SIGNIFICANT_DELTA) {
        showSubtext(F("joining..."));
        if (joinNetwork()) {
            showSubtext(F("sending..."));
            if (sendRangeWithRetries(range)) {
                lastSharedRange = range;
                preferences.putUInt("lastrange", lastSharedRange);
                showSubtext(F("send ok"));
            } else {
                showSubtext(F("send fail"));
            }
        } else {
            showSubtext(F("join fail"));
        }
    } else {
        Serial.println(F("INF: Value too similar, not sending"));
        showSubtext(F("no change"));
        blink(2, 300);
    }
}

bool sendRangeWithRetries(uint16_t range) {
    for (int i = 0; i < SEND_MAX_RETRIES; i++) {
        if (i > 0) {
            Serial.println(F("ERR: Failed to send range value, retrying soon..."));
            blink(2, 150);
            delay(SEND_RETRY_DELAY);
        }
        if (sendRange(range)) {
            blink(3, 300);
            return true;
        };
    }
    Serial.println(F("ERR: All attemps to send range value failed, aborting"));
    blink(4, 150);
    return false;
}

bool sendRange(uint16_t range) {
    Serial.println(F("INF: Attempting to send range value..."));

    uint8_t uplinkPayload[5];
    uplinkPayload[0] = PAYLOAD_VERSION;
    uplinkPayload[1] = highByte(range);
    uplinkPayload[2] = lowByte(range);
    uplinkPayload[3] = highByte(batterymv);
    uplinkPayload[4] = lowByte(batterymv);

    int16_t state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload), LORAWAN_UPLINK_USER_PORT);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("ERR: Error sending range value: #"));
        Serial.println(state);
        return false;
    }

    Serial.println(F("INF: Range value sent successfully"));
    return true;
}

bool joinNetwork() {
    Serial.println(F("INF: Initialise the LoRaWan radio..."));
    blink(5, 50);
    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.println(F("ERR: Failed to initialise the LoRaWan radio"));
        blink(5, 150);
        return false;
    }

    // SX1262 rf switch order: setRfSwitchPins(rxEn, txEn);
    //  radio.setRfSwitchPins(38, RADIOLIB_NC);

    // Setup the OTAA session information
    node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    Serial.println(F("INF: Joining the LoRaWAN Network..."));

    state = node.activateOTAA();
    if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
        Serial.print(F("ERR: Failed to join LoRaWan network: #"));
        Serial.println(state);
        blink(6, 150);
        return false;
    }

    // Disable the ADR algorithm (on by default which is preferable)
    // node.setADR(false);

    // Set a fixed datarate
    //  node.setDatarate(LORAWAN_UPLINK_DATA_RATE);

    // Manages uplink intervals to the TTN Fair Use Policy
    //  node.setDutyCycle(false);

    Serial.println(F("INF: LoRaWan network joined successfully"));
    return true;
}

void blink(int cnt, int time) {
    for (int i = 0; i < cnt; i++) {
        ledOn();
        delay(time);
        ledOff();
        delay(time);
    }
}

void ledOn() {
    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
}

void ledOff() {
    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
}

void initDisplay() {
    // Initialize SSD1306 OLED display
    Serial.println(F("Display setup..."));
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        blink(7, 150);
        goToDeepSleep();
    }

    enableDisplay();
    // Momentarily display Adafruit logo
    display.display();

    display.setTextSize(2);
    display.clearDisplay();
    display.display();

    displayAvailable = true;
}

void enableDisplay() {
    if (displayAvailable) {
        // Turn display on
        display.ssd1306_command(SSD1306_DISPLAYON);
    }
}

void disableDisplay() {
    if (displayAvailable) {
        // Turn display off
        display.ssd1306_command(SSD1306_DISPLAYOFF);
    }
}

void clearDisplayBottom() {
    if (displayAvailable) {
        int16_t h = display.height();
        display.fillRect(0, h - 8, display.width(), 8, BLACK);
        display.display();
    }
}

void initToFSensor() {
    // Initialize VL53L1X time-of-flight sensor
    Serial.println(F("Time-of-flight sensor setup..."));
    if (!dsensor.init()) {
        Serial.println(F("Failed to detect/init VL53L1X"));
        if (displayAvailable) {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("No Sensor!");
            display.display();
        }
        blink(8, 150);
        goToDeepSleep();
    }
}

uint16_t measureBatteryVoltage(int pin) {
    uint32_t Vbatt = 0;
    for (int i = 0; i < 16; i++) {
        Vbatt = Vbatt + analogReadMilliVolts(pin);  // ADC with correction
    }
    // average of 16 readings, times 2 (we're measuring half the voltage)
    Vbatt = Vbatt / 8;
    return Vbatt;
}

void goToDeepSleep() {
    disableDisplay();
    dsensor.disable();
    // Configure deep sleep wake-up timer
    esp_sleep_enable_timer_wakeup(DSLEEP_TIME_MS * 1000);
    // Configure deep sleep wake-up button
    esp_sleep_enable_ext1_wakeup(1ULL << DSLEEP_WAKEUP_PIN, ESP_EXT1_WAKEUP_ANY_LOW);
    // Store non-volatile variables
    preferences.end();
    // Close down Serial
    Serial.println(F("Sleeping..."));
    Serial.flush();
    Serial.end();
    // Enter deep sleep
    esp_deep_sleep_start();
    // Should never get here
    for (;;);
}
