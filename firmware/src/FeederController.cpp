#include "FeederController.h"

FeederController::FeederController(int feederPin, int sensorPin, bool invertSensor)
  : feederPin(feederPin), sensorPin(sensorPin), invertSensor(invertSensor),
    feeding(false), feedStartTime(0), feedDuration(0), safetyTimeout(15000), useSensorStop(false),
    feedPhase(FeedPhase::WAIT_FOR_FIRST_ONE), phaseStartTime(0), lastSensorState(false),
    lastError(Error::NONE) {}

void FeederController::begin() {
  pinMode(feederPin, OUTPUT);
  pinMode(sensorPin, INPUT_PULLDOWN);
  digitalWrite(feederPin, LOW);
}

Error FeederController::startFeed(unsigned long durationMs, bool useSensor) {
  if (feeding) return Error::NONE; // Already feeding

  lastSensorState = isSensorConnected();

  digitalWrite(feederPin, HIGH);
  feeding = true;
  feedStartTime = millis();
  feedDuration = durationMs;
  useSensorStop = useSensor;
  // If sensor is already at "1", we can immediately wait for transition to "0".
  // If not, first wait for it to become "1", then do full 1->0->1 cycle.
  feedPhase = lastSensorState ? FeedPhase::WAIT_FOR_ZERO : FeedPhase::WAIT_FOR_FIRST_ONE;
  phaseStartTime = feedStartTime;
  lastError = Error::NONE;
  return Error::NONE;
}

void FeederController::update() {
  if (!feeding) return;

  unsigned long now = millis();
  if (now - feedStartTime > safetyTimeout) {
    digitalWrite(feederPin, LOW);
    feeding = false;
    lastError = Error::TIMEOUT;
    return;
  }

  if (useSensorStop) {
    // Full cycle by edges: 1 -> 0 -> 1 -> stop.
    const bool sensorConnected = isSensorConnected();

    if (feedPhase == FeedPhase::WAIT_FOR_FIRST_ONE) {
      if (!lastSensorState && sensorConnected && (now - phaseStartTime >= EDGE_GUARD_MS)) {
        feedPhase = FeedPhase::WAIT_FOR_ZERO;
        phaseStartTime = now;
      }
      lastSensorState = sensorConnected;
      return;
    }

    if (feedPhase == FeedPhase::WAIT_FOR_ZERO) {
      if (lastSensorState && !sensorConnected && (now - phaseStartTime >= EDGE_GUARD_MS)) {
        feedPhase = FeedPhase::WAIT_FOR_FINAL_ONE;
        phaseStartTime = now;
      }
      lastSensorState = sensorConnected;
      return;
    }

    if (feedPhase == FeedPhase::WAIT_FOR_FINAL_ONE) {
      if (!lastSensorState && sensorConnected &&
          (now - phaseStartTime >= RETURN_GUARD_MS) &&
          (now - feedStartTime >= MIN_CYCLE_MS)) {
        digitalWrite(feederPin, LOW);
        feeding = false;
        lastError = Error::NONE;
      }
      lastSensorState = sensorConnected;
      return;
    }
  } else if (now - feedStartTime >= feedDuration) {
    digitalWrite(feederPin, LOW);
    feeding = false;
    lastError = Error::NONE;
  }
}

bool FeederController::isFeeding() {
  return feeding;
}

void FeederController::setSafetyTimeout(unsigned long timeoutMs) {
  safetyTimeout = timeoutMs;
}

Error FeederController::getLastError() const {
  return lastError;
}

void FeederController::clearError() {
  lastError = Error::NONE;
}

bool FeederController::isSensorConnected() const {
  return digitalRead(sensorPin) == (invertSensor ? LOW : HIGH);
}
