#include "ServoController.h"
#include <Arduino.h>

ServoController::ServoController(int pin, int startAngle)
    : servoPin(pin), currentPosition(startAngle), lastPosition(-1),
      moving(false), moveStartTime(0), initialAngle(startAngle) {
  if (initialAngle < 0)
    initialAngle = 0;
  if (initialAngle > 90)
    initialAngle = 90;
}

void ServoController::begin() {
  // Przywrocenie zachowania ze stabilnej, starszej wersji:
  // inicjalne ustawienie pozycji startowej po boot.
  servoMotor.attach(servoPin);
  servoMotor.write(initialAngle);
  delay(MOVE_TIME);
  servoMotor.detach();
  // Wymus LOW na pinie po odlaczeniu - zapobiega grzaniu serwomechanizmu
  // (plywajacy stan PWM po detach() jest interpretowany przez servo jako ruch)
  pinMode(servoPin, OUTPUT);
  digitalWrite(servoPin, LOW);
  currentPosition = initialAngle;
  lastPosition = initialAngle;
  moving = false;
}

void ServoController::setPosition(int position) {
  if (position < 0)
    position = 0;
  if (position > 90)
    position = 90;

  // Zawsze podlacz i wyslij pozycje - stroz jest po stronie wywolujacego
  // (updateSystemState sprawdza czy cel sie zmienil, API/testy wywoluja wprost)
  servoMotor.attach(servoPin);
  servoMotor.write(position);
  moveStartTime = millis();
  moving = true;

  lastPosition = position;
  currentPosition = position;
}

void ServoController::update() {
  if (moving && millis() - moveStartTime >= MOVE_TIME) {
    servoMotor.detach();
    // Wymus LOW na pinie po odlaczeniu - zapobiega grzaniu serwomechanizmu
    pinMode(servoPin, OUTPUT);
    digitalWrite(servoPin, LOW);
    moving = false;
  }
}

bool ServoController::isMoving() { return moving; }

int ServoController::getCurrentPosition() { return currentPosition; }
