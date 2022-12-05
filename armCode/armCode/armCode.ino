#include "Servo.h"
Servo arm_servo;

#define SERVO_ARM_PIN 6

void setup() {
  // put your setup code here, to run once:
  arm_servo.attach(SERVO_ARM_PIN);

}

void loop() {
  for(int i = 0; i < 10; i++)
  {
    arm_servo.write(0);
    delay(1000);
    arm_servo.write(180);
    delay(1000);
  }

    delay(100);
}
