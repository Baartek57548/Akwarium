#include "Arduino.h"
#include "ConfigManager.h"
#include "ConfigValidation.h"
#include "DallasTemperature.h"
#include "FeederController.h"
#include "ScheduleManager.h"
#include "TemperatureController.h"

#include <exception>
#include <functional>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

Config makeBaseConfig() {
  Config cfg = {};
  cfg.lightMode = static_cast<uint8_t>(ScheduleMode::Schedule);
  cfg.dayStartHour = 9;
  cfg.dayStartMinute = 0;
  cfg.dayEndHour = 21;
  cfg.dayEndMinute = 0;
  cfg.aerationMode = static_cast<uint8_t>(ScheduleMode::Schedule);
  cfg.aerationHourOn = 8;
  cfg.aerationMinuteOn = 0;
  cfg.aerationHourOff = 22;
  cfg.aerationMinuteOff = 0;
  cfg.filterMode = static_cast<uint8_t>(ScheduleMode::Schedule);
  cfg.filterHourOn = 0;
  cfg.filterMinuteOn = 0;
  cfg.filterHourOff = 23;
  cfg.filterMinuteOff = 55;
  cfg.heaterMode = static_cast<uint8_t>(HeaterMode::Threshold);
  cfg.targetTemp = 25.0f;
  cfg.tempHysteresis = 0.5f;
  cfg.feedMode = 1;
  cfg.feedHour = 18;
  cfg.feedMinute = 0;
  std::snprintf(cfg.staSsid, sizeof(cfg.staSsid), "TestSSID");
  std::snprintf(cfg.staPassword, sizeof(cfg.staPassword), "TestPass123");
  std::snprintf(cfg.apSsid, sizeof(cfg.apSsid), "TestAP");
  std::snprintf(cfg.apPassword, sizeof(cfg.apPassword), "TestApPass123");
  cfg.version = CONFIG_VERSION;
  cfg.magic = CONFIG_MAGIC;
  return cfg;
}

void expect(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void expectEqual(long actual, long expected, const std::string &message) {
  if (actual != expected) {
    throw std::runtime_error(message + " (expected " + std::to_string(expected) +
                             ", got " + std::to_string(actual) + ")");
  }
}

void expectFloatNear(float actual, float expected, float epsilon,
                     const std::string &message) {
  if (std::fabs(actual - expected) > epsilon) {
    throw std::runtime_error(message + " (expected " +
                             std::to_string(expected) + ", got " +
                             std::to_string(actual) + ")");
  }
}

void testConfigValidationPatchRejectsInvalidMinute() {
  Config cfg = makeBaseConfig();
  ConfigPatch patch = {};
  patch.hasFeedMinute = true;
  patch.feedMinute = 17;

  ConfigValidationResult result = {};
  const bool applied = ConfigValidation::applyRuntimePatch(cfg, patch, result);

  expect(!applied, "ConfigValidation should reject invalid feed minute");
  expect(result.hasInvalidFields(), "ConfigValidation should mark invalid field");
  expect(std::string(result.errorCode) == "invalid_feed_time",
         "ConfigValidation should report invalid_feed_time");
  expectEqual(cfg.feedMinute, 0, "Rejected patch should leave config unchanged");
}

void testConfigValidationRejectsLightWindowOutsideScheduleMode() {
  Config cfg = makeBaseConfig();
  ConfigPatch patch = {};
  patch.hasLightMode = true;
  patch.lightMode = static_cast<int>(ScheduleMode::AlwaysOn);
  patch.hasDayStartHour = true;
  patch.dayStartHour = 7;

  ConfigValidationResult result = {};
  const bool applied = ConfigValidation::applyRuntimePatch(cfg, patch, result);

  expect(!applied, "ConfigValidation should reject light window outside schedule mode");
  expect(std::string(result.errorCode) == "light_time_requires_schedule",
         "ConfigValidation should expose light_time_requires_schedule");
}

void testConfigValidationAppliesValidTemperaturePatch() {
  Config cfg = makeBaseConfig();
  ConfigPatch patch = {};
  patch.hasHeaterMode = true;
  patch.heaterMode = static_cast<int>(HeaterMode::Off);
  patch.hasTargetTemp = true;
  patch.targetTemp = 27.0f;
  patch.hasTempHysteresis = true;
  patch.tempHysteresis = 0.7f;

  ConfigValidationResult result = {};
  const bool applied = ConfigValidation::applyRuntimePatch(cfg, patch, result);

  expect(applied, "ConfigValidation should accept valid temperature patch");
  expectEqual(cfg.heaterMode, static_cast<int>(HeaterMode::Off),
              "Heater mode should be updated");
  expectFloatNear(cfg.targetTemp, 27.0f, 0.001f, "Target temperature should change");
  expectFloatNear(cfg.tempHysteresis, 0.7f, 0.001f,
                  "Hysteresis should change");
}

void testScheduleManagerHandlesOvernightWindows() {
  expect(ScheduleManager::isWithinWindow(15, 1380, 120),
         "Overnight window should include 00:15");
  expect(!ScheduleManager::isWithinWindow(600, 1380, 120),
         "Overnight window should exclude midday");
}

void testScheduleManagerCalculatesFilterCountdown() {
  Config cfg = makeBaseConfig();
  cfg.filterMode = static_cast<uint8_t>(ScheduleMode::Schedule);
  cfg.filterHourOn = 22;
  cfg.filterMinuteOn = 0;
  cfg.filterHourOff = 2;
  cfg.filterMinuteOff = 0;
  ConfigManager::updateAndSave(cfg);

  expectEqual(ScheduleManager::getMinutesUntilFilterOff(23 * 60 + 30), 150,
              "Countdown should span midnight correctly");
  expectEqual(ScheduleManager::getMinutesUntilFilterOff(60), 60,
              "Countdown should work after midnight");
}

void testTemperatureControllerSwitchesHeaterWithThresholds() {
  ArduinoTest::reset();
  DallasTemperatureTest::reset();
  DallasTemperatureTest::setDeviceCount(1);

  TemperatureController controller(4, 5, 25.0f, 0.5f, true);
  controller.begin();
  expect(controller.isHeaterOn(), "Heater should start enabled in failsafe mode");

  controller.controlHeater(25.6f);
  expect(!controller.isHeaterOn(), "Heater should disconnect above threshold");
  expectEqual(ArduinoTest::getDigitalOutput(5), LOW,
              "Heater relay output should be driven low when off");

  ArduinoTest::advanceMillis(120000);
  controller.controlHeater(24.8f);
  expect(controller.isHeaterOn(), "Heater should reconnect at target temperature");
  expectEqual(ArduinoTest::getDigitalOutput(5), HIGH,
              "Heater relay output should be driven high when on");
}

void testTemperatureControllerKeepsLastValidReadingBeforeDisconnect() {
  ArduinoTest::reset();
  DallasTemperatureTest::reset();
  DallasTemperatureTest::setDeviceCount(1);
  TemperatureController controller(4, 5, 25.0f, 0.5f, true);
  controller.begin();

  DallasTemperatureTest::queueReading(24.75f);
  expectFloatNear(controller.readTemperature(), 24.75f, 0.001f,
                  "First valid reading should be returned");

  for (int attempt = 0; attempt < 2; ++attempt) {
    ArduinoTest::advanceMillis(2000);
    DallasTemperatureTest::queueReading(DEVICE_DISCONNECTED_C);
    const float value = controller.readTemperature();
    expectFloatNear(value, 24.75f, 0.001f,
                    "Controller should keep last valid reading across short glitches");
  }

  ArduinoTest::advanceMillis(2000);
  DallasTemperatureTest::queueReading(DEVICE_DISCONNECTED_C);
  expectFloatNear(controller.readTemperature(), DEVICE_DISCONNECTED_C, 0.001f,
                  "Controller should disconnect after repeated invalid samples");
}

void testFeederControllerRunsFullSensorCycle() {
  ArduinoTest::reset();
  ArduinoTest::setDigitalInput(9, LOW);

  FeederController feeder(8, 9, false, true);
  feeder.begin();

  expect(feeder.startFeed(1500, true) == Error::NONE,
         "Feeder should start when idle");
  expectEqual(ArduinoTest::getDigitalOutput(8), HIGH,
              "Feeder motor should start");

  feeder.update();
  ArduinoTest::advanceMillis(120);
  ArduinoTest::setDigitalInput(9, HIGH);
  feeder.update();

  ArduinoTest::advanceMillis(120);
  ArduinoTest::setDigitalInput(9, LOW);
  feeder.update();

  ArduinoTest::advanceMillis(120);
  ArduinoTest::setDigitalInput(9, HIGH);
  feeder.update();

  expect(!feeder.isFeeding(), "Feeder should stop after full sensor cycle");
  expectEqual(ArduinoTest::getDigitalOutput(8), LOW,
              "Feeder motor should stop after cycle");
  expect(feeder.getLastError() == Error::NONE,
         "Successful cycle should finish without error");
}

void testFeederControllerTimesOut() {
  ArduinoTest::reset();
  ArduinoTest::setDigitalInput(9, LOW);

  FeederController feeder(8, 9, false, true);
  feeder.begin();
  feeder.setSafetyTimeout(500);
  expect(feeder.startFeed(1500, true) == Error::NONE,
         "Feeder should start before timeout test");

  ArduinoTest::advanceMillis(600);
  feeder.update();

  expect(!feeder.isFeeding(), "Feeder should stop after timeout");
  expect(feeder.getLastError() == Error::TIMEOUT,
         "Timeout should be reported");
}

} // namespace

int main() {
  const std::vector<std::pair<std::string, std::function<void()>>> tests = {
      {"ConfigValidation rejects invalid minute",
       testConfigValidationPatchRejectsInvalidMinute},
      {"ConfigValidation rejects window outside schedule mode",
       testConfigValidationRejectsLightWindowOutsideScheduleMode},
      {"ConfigValidation applies valid temperature patch",
       testConfigValidationAppliesValidTemperaturePatch},
      {"ScheduleManager handles overnight windows",
       testScheduleManagerHandlesOvernightWindows},
      {"ScheduleManager calculates filter countdown",
       testScheduleManagerCalculatesFilterCountdown},
      {"TemperatureController switches heater with thresholds",
       testTemperatureControllerSwitchesHeaterWithThresholds},
      {"TemperatureController keeps last valid reading before disconnect",
       testTemperatureControllerKeepsLastValidReadingBeforeDisconnect},
      {"FeederController runs full sensor cycle",
       testFeederControllerRunsFullSensorCycle},
      {"FeederController times out", testFeederControllerTimesOut},
  };

  int failures = 0;
  for (const auto &test : tests) {
    try {
      test.second();
      std::cout << "[PASS] " << test.first << "\n";
    } catch (const std::exception &error) {
      failures += 1;
      std::cerr << "[FAIL] " << test.first << ": " << error.what() << "\n";
    }
  }

  if (failures > 0) {
    std::cerr << failures << " firmware unit test(s) failed.\n";
    return 1;
  }

  std::cout << tests.size() << " firmware unit test(s) passed.\n";
  return 0;
}
