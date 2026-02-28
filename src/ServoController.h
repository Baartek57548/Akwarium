#ifndef SERVO_CONTROLLER_H
#define SERVO_CONTROLLER_H

#include <ESP32Servo.h>

class ServoController {
private:
  Servo servoMotor;
  int servoPin;
  int currentPosition;
  int lastPosition;
  bool moving;
  unsigned long moveStartTime;
  const unsigned long MOVE_TIME = 2000;
  int initialAngle;

public:
  ServoController(int pin, int startAngle = 90);
  void begin();
  void setPosition(int position);
  void update();
  bool isMoving();
  int getCurrentPosition();
};

#endif
