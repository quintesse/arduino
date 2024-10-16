const int pinL9110S_B1A = 5; // D1
const int pinL9110S_B1B = 4; // D2
const int pinLED = 2; // D4
const int maxSpeed = 255;
const int speedStep = 48;
int curSpeed = maxSpeed;
int dir = 0;

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
  Serial.println(curSpeed);
  if (dir == 0) {
    analogWrite(pinL9110S_B1A, 0);
    analogWrite(pinL9110S_B1B, curSpeed);
  } else {
    analogWrite(pinL9110S_B1A, curSpeed);
    analogWrite(pinL9110S_B1B, 0);    
  }
  digitalWrite(pinLED, LOW); // On
  delay(2000);
  curSpeed = curSpeed - speedStep;
  if (curSpeed < 0) {
    curSpeed = maxSpeed;
    dir = !dir;
  }
}
