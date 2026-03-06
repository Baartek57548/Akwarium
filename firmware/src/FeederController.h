#ifndef FEEDER_CONTROLLER_H
#define FEEDER_CONTROLLER_H

#include <Arduino.h>

enum class Error { NONE, SENSOR_NOT_OK, TIMEOUT };

class FeederController {
private:
  enum class FeedPhase : uint8_t {
    WAIT_FOR_FIRST_ONE,
    WAIT_FOR_ZERO,
    WAIT_FOR_FINAL_ONE
  };

  int feederPin;
  int sensorPin;
  bool invertSensor;
  bool feeding;
  unsigned long feedStartTime;
  unsigned long feedDuration;
  unsigned long safetyTimeout;
  bool useSensorStop;
  FeedPhase feedPhase;
  unsigned long phaseStartTime;
  bool lastSensorState;
  const unsigned long EDGE_GUARD_MS = 100;
  const unsigned long RETURN_GUARD_MS = 20;
  const unsigned long MIN_CYCLE_MS = 300;
  Error lastError;
  bool isSensorConnected() const;

public:
  FeederController(int feederPin, int sensorPin, bool invertSensor = false);
  void begin();
  Error startFeed(unsigned long durationMs, bool useSensor = false);
  void update();
  bool isFeeding();
  void setSafetyTimeout(unsigned long timeoutMs);
  Error getLastError() const;
  void clearError();
};

#endif
