const int pinL9110S_B1A = 5; // D1
const int pinL9110S_B1B = 4; // D2
const int pinLED = 2; // D4

void setup() {
  Serial.begin(74880);
  Serial.println("Testing 1 ...");
  pinMode(pinL9110S_B1A, OUTPUT);
  pinMode(pinL9110S_B1B, OUTPUT);
  pinMode(pinLED, OUTPUT);
  digitalWrite(pinL9110S_B1A, LOW);
  digitalWrite(pinL9110S_B1B, LOW);
  digitalWrite(pinLED, HIGH); // Off
}

void loop() {
  Serial.println("CW");
  digitalWrite(pinL9110S_B1A, LOW);
  digitalWrite(pinL9110S_B1B, HIGH);
  digitalWrite(pinLED, LOW); // On
  delay(2000);
  Serial.println("CCW");
  digitalWrite(pinL9110S_B1A, HIGH);
  digitalWrite(pinL9110S_B1B, LOW);
  digitalWrite(pinLED, HIGH); // Off
  delay(2000);
}
