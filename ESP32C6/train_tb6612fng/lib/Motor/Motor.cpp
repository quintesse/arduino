#include "Motor.h"
#include <Arduino.h>

Motor::Motor(int In1pin, int In2pin, int PWMpin, int offset, int STBYpin) {
  In1 = In1pin;
  In2 = In2pin;
  PWM = PWMpin;
  Standby = STBYpin;
  Offset = offset;  
}

void Motor::drive(int speed) {
  pinMode(In1, OUTPUT);
  pinMode(In2, OUTPUT);
  pinMode(PWM, OUTPUT);
  pinMode(Standby, OUTPUT);
  digitalWrite(Standby, HIGH);
  speed = speed * Offset;
  if (speed>=0) {
    digitalWrite(In1, HIGH);
    digitalWrite(In2, LOW);
    analogWrite(PWM, speed);
  } else {
    digitalWrite(In1, LOW);
    digitalWrite(In2, HIGH);
    analogWrite(PWM, -speed);
  }
}

void Motor::brake() {
  digitalWrite(In1, HIGH);
  digitalWrite(In2, HIGH);
  analogWrite(PWM,0);
}

void Motor::standby() {
  digitalWrite(Standby, LOW);
}
