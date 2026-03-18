#include "FakeU8g2.h"

#include "AquariumAnimation.h"
#include "Arduino.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <memory>
#include <mutex>

namespace {

constexpr unsigned long UiIdleReturnHomeMs = 30000UL;
constexpr unsigned long CalibrationPromptMs = 250UL;
constexpr unsigned long CalibrationExpectedMs = 7000UL;
constexpr unsigned long CalibrationResultMs = 1200UL;

enum class UiState {
  HOME,
  MENU,
  SCHEDULE_LIGHT,
  SCHEDULE_AERATION,
  SCHEDULE_FILTER,
  SCHEDULE_TEMP,
  SCHEDULE_FEEDING,
  LOGS,
  SETTINGS_DATETIME,
  TESTS,
  CALIBRATION,
  FEEDING,
  ACCESS_POINT,
  BLUETOOTH
};

struct SimulatedConfig {
  std::uint8_t lightMode = 0;
  std::uint8_t dayStartHour = 9;
  std::uint8_t dayStartMinute = 30;
  std::uint8_t dayEndHour = 23;
  std::uint8_t dayEndMinute = 0;

  std::uint8_t aerationMode = 0;
  std::uint8_t aerationHourOn = 8;
  std::uint8_t aerationMinuteOn = 0;
  std::uint8_t aerationHourOff = 20;
  std::uint8_t aerationMinuteOff = 0;

  std::uint8_t filterMode = 0;
  std::uint8_t filterHourOn = 8;
  std::uint8_t filterMinuteOn = 0;
  std::uint8_t filterHourOff = 18;
  std::uint8_t filterMinuteOff = 0;

  std::uint8_t heaterMode = 0;
  std::uint8_t targetTemp = 24;

  std::uint8_t feedHour = 15;
  std::uint8_t feedMinute = 0;
  std::uint8_t feedMode = 1;
};

bool shouldApplyUiIdleHomeTimeout(UiState state) {
  if (state == UiState::HOME)
    return false;
  if (state == UiState::ACCESS_POINT || state == UiState::BLUETOOTH)
    return false;
  if (state == UiState::FEEDING)
    return false;
  return true;
}

bool isScheduleActive(std::uint8_t mode, int nowMinutes, std::uint8_t onHour,
                      std::uint8_t onMinute, std::uint8_t offHour,
                      std::uint8_t offMinute) {
  if (mode == 1) {
    return true;
  }
  if (mode == 2) {
    return false;
  }

  const int on = (static_cast<int>(onHour) * 60) + static_cast<int>(onMinute);
  const int off = (static_cast<int>(offHour) * 60) + static_cast<int>(offMinute);

  if (on == off) {
    return true;
  }

  if (on < off) {
    return nowMinutes >= on && nowMinutes < off;
  }

  return nowMinutes >= on || nowMinutes < off;
}

class FirmwareUiRuntime {
public:
  bool initialize() {
    animation_ = std::make_unique<AquariumAnimation>(&display_);
    uiState_ = UiState::HOME;
    lastUpPressed_ = false;
    lastSelectPressed_ = false;
    lastDownPressed_ = false;
    pendingUp_ = false;
    pendingSelect_ = false;
    pendingDown_ = false;
    feedingUiActive_ = false;
    uiStateBeforeFeeding_ = UiState::HOME;
    feedingUntilMs_ = 0;
    lastUiInteractionMs_ = 0;
    lastTickMs_ = 0;
    temperaturePhase_ = 0.0;
    batteryPercent_ = 88;
    batteryVoltage_ = 4.0f;
    temperatureC_ = 24.0f;
    manualTemperatureEnabled_ = false;
    manualBatteryEnabled_ = false;
    manualAerationEnabled_ = false;
    manualTemperatureC_ = 24.0f;
    manualBatteryPercent_ = 88;
    manualAerationPercent_ = 30;
    calibrationPhase_ = CalibrationPhase::IDLE;
    calibrationPhaseStartMs_ = 0;
    calibrationOk_ = true;
    simulatedClock_ = std::chrono::system_clock::now();
    fake_set_millis(0);
    animation_->clearLogs();
    tick();
    return true;
  }

  void pressUp() {
    pendingUp_ = true;
    tick();
  }

  void pressDown() {
    pendingDown_ = true;
    tick();
  }

  void pressSelect() {
    pendingSelect_ = true;
    tick();
  }

  void setManualTemperature(float value) {
    if (!std::isfinite(value)) {
      manualTemperatureEnabled_ = false;
      return;
    }
    manualTemperatureEnabled_ = true;
    manualTemperatureC_ = std::clamp(value, 0.0f, 45.0f);
  }

  void setManualBatteryPercent(int value) {
    if (value < 0) {
      manualBatteryEnabled_ = false;
      return;
    }
    manualBatteryEnabled_ = true;
    manualBatteryPercent_ = static_cast<std::uint8_t>(std::clamp(value, 0, 100));
  }

  void setManualAerationPercent(int value) {
    if (value < 0) {
      manualAerationEnabled_ = false;
      return;
    }
    manualAerationEnabled_ = true;
    manualAerationPercent_ = static_cast<std::uint8_t>(std::clamp(value, 0, 100));
  }

  void addManualLog(const char *message, const char *timeText) {
    if (!animation_ || message == nullptr) {
      return;
    }

    char msgBuf[20] = {};
    std::strncpy(msgBuf, message, sizeof(msgBuf) - 1);
    if (msgBuf[0] == '\0') {
      return;
    }

    char timeBuf[6] = {};
    if (timeText != nullptr && std::strlen(timeText) >= 4) {
      std::strncpy(timeBuf, timeText, sizeof(timeBuf) - 1);
    } else {
      std::uint8_t hour = 0;
      std::uint8_t minute = 0;
      std::uint8_t second = 0;
      std::uint8_t day = 1;
      std::uint8_t month = 1;
      std::uint16_t year = 2026;
      getClockFields(hour, minute, second, day, month, year);
      std::snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u", hour, minute);
    }

    animation_->addLog(msgBuf, timeBuf);
  }

  void clearManualLogs() {
    if (animation_) {
      animation_->clearLogs();
    }
  }

  void startCalibrationAnimation() {
    calibrationPhase_ = CalibrationPhase::PROMPT;
    calibrationPhaseStartMs_ = lastTickMs_ == 0 ? millis() : lastTickMs_;
    calibrationOk_ = true;
    uiState_ = UiState::CALIBRATION;
  }

  const std::uint8_t *getFrameBuffer() {
    tick();
    return display_.getPublishedLinearFrameBuffer();
  }

private:
  enum class CalibrationPhase {
    IDLE,
    PROMPT,
    RUNNING,
    RESULT
  };

  void tick() {
    if (!animation_) {
      return;
    }

    const auto nowPoint = std::chrono::steady_clock::now();
    const auto nowMs = static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(nowPoint - processStart_).count());

    unsigned long deltaMs = 33;
    if (lastTickMs_ != 0 && nowMs > lastTickMs_) {
      deltaMs = nowMs - lastTickMs_;
      deltaMs = std::clamp(deltaMs, 1UL, 250UL);
    }
    lastTickMs_ = nowMs;
    fake_set_millis(nowMs);

    const bool isUpPressed = pendingUp_;
    const bool isSelectPressed = pendingSelect_;
    const bool isDownPressed = pendingDown_;
    pendingUp_ = false;
    pendingSelect_ = false;
    pendingDown_ = false;

    updateUiState(isUpPressed, isSelectPressed, isDownPressed, nowMs);
    updateSimulation(deltaMs);
    renderFrame(isUpPressed, isSelectPressed, isDownPressed);
    captureAnimationChanges();

    lastUpPressed_ = isUpPressed;
    lastSelectPressed_ = isSelectPressed;
    lastDownPressed_ = isDownPressed;
  }

  void updateUiState(bool isUpPressed, bool isSelectPressed, bool isDownPressed,
                     unsigned long nowMs) {
    if (!animation_) {
      return;
    }

    if (lastUiInteractionMs_ == 0) {
      lastUiInteractionMs_ = nowMs;
    }

    const bool upJustPressed = isUpPressed && !lastUpPressed_;
    const bool selectJustPressed = isSelectPressed && !lastSelectPressed_;
    const bool downJustPressed = isDownPressed && !lastDownPressed_;

    if (upJustPressed || selectJustPressed || downJustPressed) {
      lastUiInteractionMs_ = nowMs;
    }

    if (feedingUiActive_ && nowMs >= feedingUntilMs_) {
      feedingUiActive_ = false;
      if (uiState_ == UiState::FEEDING) {
        uiState_ = uiStateBeforeFeeding_;
      }
      animation_->setFeedingAnimation(false);
    }

    if (shouldApplyUiIdleHomeTimeout(uiState_) &&
        (nowMs - lastUiInteractionMs_ >= UiIdleReturnHomeMs)) {
      uiState_ = UiState::HOME;
    }

    switch (uiState_) {
    case UiState::HOME:
      if (selectJustPressed)
        uiState_ = UiState::MENU;
      break;

    case UiState::MENU:
      if (upJustPressed)
        uiState_ = UiState::HOME;
      if (downJustPressed)
        animation_->menuNext();
      if (selectJustPressed) {
        const std::uint8_t sel = animation_->getMenuSelection();
        if (sel == 0) {
          uiState_ = UiState::SCHEDULE_LIGHT;
          animation_->setActiveScheduleId(0);
        } else if (sel == 1) {
          uiState_ = UiState::LOGS;
        } else if (sel == 2) {
          uiState_ = UiState::SETTINGS_DATETIME;
          animation_->setActiveScheduleId(5);
        } else if (sel == 3) {
          uiState_ = UiState::TESTS;
          animation_->enterTestMode();
        } else if (sel == 4) {
          calibrationPhase_ = CalibrationPhase::PROMPT;
          calibrationPhaseStartMs_ = nowMs;
          calibrationOk_ = true;
          uiState_ = UiState::CALIBRATION;
        } else if (sel == 5) {
          uiState_ = UiState::ACCESS_POINT;
        } else if (sel == 6) {
          uiState_ = UiState::BLUETOOTH;
        }
      }
      break;

    case UiState::ACCESS_POINT:
      if (upJustPressed) {
        uiState_ = UiState::MENU;
      }
      break;

    case UiState::BLUETOOTH:
      if (upJustPressed) {
        uiState_ = UiState::MENU;
      }
      break;

    case UiState::LOGS:
      if (upJustPressed)
        uiState_ = UiState::MENU;
      if (downJustPressed)
        animation_->logScrollNext();
      break;

    case UiState::TESTS:
      if (upJustPressed)
        uiState_ = UiState::MENU;
      if (downJustPressed) {
        if (animation_->isEditingActive())
          animation_->incrementTestValue();
        else
          animation_->testNext();
      }
      if (selectJustPressed)
        animation_->toggleTestOption();
      break;

    case UiState::CALIBRATION:
      if (upJustPressed || selectJustPressed) {
        calibrationPhase_ = CalibrationPhase::IDLE;
        uiState_ = UiState::MENU;
        break;
      }

      if (calibrationPhase_ == CalibrationPhase::PROMPT &&
          (nowMs - calibrationPhaseStartMs_ >= CalibrationPromptMs)) {
        calibrationPhase_ = CalibrationPhase::RUNNING;
        calibrationPhaseStartMs_ = nowMs;
      } else if (calibrationPhase_ == CalibrationPhase::RUNNING &&
                 (nowMs - calibrationPhaseStartMs_ >= CalibrationExpectedMs)) {
        calibrationPhase_ = CalibrationPhase::RESULT;
        calibrationPhaseStartMs_ = nowMs;
      } else if (calibrationPhase_ == CalibrationPhase::RESULT &&
                 (nowMs - calibrationPhaseStartMs_ >= CalibrationResultMs)) {
        calibrationPhase_ = CalibrationPhase::IDLE;
        uiState_ = UiState::MENU;
      }
      break;

    case UiState::SETTINGS_DATETIME:
      if (selectJustPressed) {
        if (!animation_->isEditingActive())
          animation_->startEditing();
        else
          animation_->nextEditStep();
      }
      if (downJustPressed) {
        if (!animation_->isEditingActive())
          animation_->scheduleNext();
        else
          animation_->scheduleEditIncrement();
      }
      if (upJustPressed) {
        if (!animation_->isEditingActive())
          uiState_ = UiState::MENU;
      }
      break;

    case UiState::SCHEDULE_LIGHT:
      if (selectJustPressed) {
        if (!animation_->isEditingActive())
          animation_->startEditing();
        else
          animation_->nextEditStep();
      }
      if (downJustPressed) {
        if (!animation_->isEditingActive()) {
          if (animation_->getScheduleSelection() == 2) {
            uiState_ = UiState::SCHEDULE_AERATION;
            animation_->setActiveScheduleId(1);
          } else {
            animation_->scheduleNext();
          }
        } else {
          animation_->scheduleEditIncrement();
        }
      }
      if (upJustPressed) {
        if (!animation_->isEditingActive())
          uiState_ = UiState::MENU;
      }
      break;

    case UiState::SCHEDULE_AERATION:
      if (selectJustPressed) {
        if (!animation_->isEditingActive())
          animation_->startEditing();
        else
          animation_->nextEditStep();
      }
      if (downJustPressed) {
        if (!animation_->isEditingActive()) {
          if (animation_->getScheduleSelection() == 2) {
            uiState_ = UiState::SCHEDULE_FILTER;
            animation_->setActiveScheduleId(2);
          } else {
            animation_->scheduleNext();
          }
        } else {
          animation_->scheduleEditIncrement();
        }
      }
      if (upJustPressed) {
        if (!animation_->isEditingActive())
          uiState_ = UiState::MENU;
      }
      break;

    case UiState::SCHEDULE_FILTER:
      if (selectJustPressed) {
        if (!animation_->isEditingActive())
          animation_->startEditing();
        else
          animation_->nextEditStep();
      }
      if (downJustPressed) {
        if (!animation_->isEditingActive()) {
          if (animation_->getScheduleSelection() == 2) {
            uiState_ = UiState::SCHEDULE_TEMP;
            animation_->setActiveScheduleId(3);
          } else {
            animation_->scheduleNext();
          }
        } else {
          animation_->scheduleEditIncrement();
        }
      }
      if (upJustPressed) {
        if (!animation_->isEditingActive())
          uiState_ = UiState::MENU;
      }
      break;

    case UiState::SCHEDULE_TEMP:
      if (selectJustPressed) {
        if (!animation_->isEditingActive())
          animation_->startEditing();
        else
          animation_->nextEditStep();
      }
      if (downJustPressed) {
        if (!animation_->isEditingActive()) {
          uiState_ = UiState::SCHEDULE_FEEDING;
          animation_->setActiveScheduleId(4);
        } else {
          animation_->scheduleEditIncrement();
        }
      }
      if (upJustPressed) {
        if (!animation_->isEditingActive())
          uiState_ = UiState::MENU;
      }
      break;

    case UiState::SCHEDULE_FEEDING:
      if (selectJustPressed) {
        if (!animation_->isEditingActive())
          animation_->startEditing();
        else
          animation_->nextEditStep();
      }
      if (downJustPressed) {
        if (!animation_->isEditingActive()) {
          if (animation_->getScheduleSelection() == 0)
            animation_->scheduleNext();
          else {
            uiState_ = UiState::SCHEDULE_LIGHT;
            animation_->setActiveScheduleId(0);
          }
        } else {
          animation_->scheduleEditIncrement();
        }
      }
      if (upJustPressed) {
        if (!animation_->isEditingActive())
          uiState_ = UiState::MENU;
      }
      break;

    case UiState::FEEDING:
      if (upJustPressed || selectJustPressed) {
        feedingUiActive_ = false;
        animation_->setFeedingAnimation(false);
        uiState_ = uiStateBeforeFeeding_;
      }
      break;
    }
  }

  void updateSimulation(unsigned long deltaMs) {
    simulatedClock_ += std::chrono::milliseconds(deltaMs);

    const double deltaSeconds = static_cast<double>(deltaMs) / 1000.0;
    temperaturePhase_ += deltaSeconds * 0.25;
    if (temperaturePhase_ > 10000.0) {
      temperaturePhase_ = std::fmod(temperaturePhase_, 10000.0);
    }
    temperatureC_ = 24.0f + static_cast<float>(std::sin(temperaturePhase_) * 1.4);

    const double batteryWave = std::sin(temperaturePhase_ * 0.21);
    batteryPercent_ = static_cast<std::uint8_t>(std::clamp(78.0 + (batteryWave * 18.0), 5.0, 100.0));
    batteryVoltage_ = 3.6f + static_cast<float>(batteryPercent_) * 0.006f;
  }

  void getClockFields(std::uint8_t &hour, std::uint8_t &minute, std::uint8_t &second,
                      std::uint8_t &day, std::uint8_t &month, std::uint16_t &year) const {
    const std::time_t timeValue = std::chrono::system_clock::to_time_t(simulatedClock_);
    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &timeValue);
#else
    localTm = *std::localtime(&timeValue);
#endif

    hour = static_cast<std::uint8_t>(std::clamp(localTm.tm_hour, 0, 23));
    minute = static_cast<std::uint8_t>(std::clamp(localTm.tm_min, 0, 59));
    second = static_cast<std::uint8_t>(std::clamp(localTm.tm_sec, 0, 59));
    day = static_cast<std::uint8_t>(std::clamp(localTm.tm_mday, 1, 31));
    month = static_cast<std::uint8_t>(std::clamp(localTm.tm_mon + 1, 1, 12));
    year = static_cast<std::uint16_t>(std::clamp(localTm.tm_year + 1900, 2024, 2099));
  }

  int estimateTextWidth6x10(const char *text) const {
    if (text == nullptr) {
      return 0;
    }
    return static_cast<int>(std::strlen(text)) * 6;
  }

  int centeredX6x10(const char *text) const {
    const int width = estimateTextWidth6x10(text);
    return std::max(0, (128 - width) / 2);
  }

  void drawCalibrationPrompt() {
    display_.setFont(u8g2_font_6x10_tr);
    display_.drawStr(centeredX6x10("Kalibracja karmnika"), 10, "Kalibracja karmnika");
    display_.drawStr(centeredX6x10("Przygotowanie..."), 21, "Przygotowanie...");
  }

  void drawCalibrationAnimation(unsigned long elapsedMs) {
    static const int8_t spinnerX[8] = {0, 2, 3, 2, 0, -2, -3, -2};
    static const int8_t spinnerY[8] = {-3, -2, 0, 2, 3, 2, 0, -2};
    static const int8_t waveY[8] = {0, 1, 2, 1, 0, -1, -2, -1};

    const unsigned long clampedElapsed = std::min(elapsedMs, CalibrationExpectedMs);
    const std::uint8_t progressPct =
        static_cast<std::uint8_t>((clampedElapsed * 100UL) / CalibrationExpectedMs);
    std::uint8_t progressFill =
        static_cast<std::uint8_t>((clampedElapsed * 112UL) / CalibrationExpectedMs);
    progressFill = std::min<std::uint8_t>(progressFill, 112);

    const std::uint8_t spinnerPhase = static_cast<std::uint8_t>((elapsedMs / 90UL) % 8UL);
    const int pelletX = 30 + static_cast<int>((elapsedMs / 24UL) % 78UL);
    const std::uint8_t wavePhase = static_cast<std::uint8_t>(((elapsedMs / 60UL) + pelletX) % 8U);
    const int pelletY = 18 + waveY[wavePhase];

    display_.setFont(u8g2_font_6x10_tr);
    display_.drawStr(centeredX6x10("Kalibracja karmnika"), 9, "Kalibracja karmnika");

    display_.drawFrame(3, 11, 122, 14);
    display_.drawLine(26, 18, 119, 18);
    display_.drawDisc(pelletX, pelletY, 2, U8G2_DRAW_ALL);

    for (std::uint8_t i = 0; i < 8; i++) {
      const std::uint8_t idx = static_cast<std::uint8_t>((spinnerPhase + i) % 8U);
      const int x = 15 + spinnerX[idx];
      const int y = 18 + spinnerY[idx];
      if (i < 2) {
        display_.drawDisc(x, y, 1, U8G2_DRAW_ALL);
      } else {
        display_.drawPixel(x, y);
      }
    }

    display_.drawFrame(8, 27, 114, 4);
    if (progressFill > 0) {
      display_.drawBox(9, 28, progressFill, 2);
    }

    display_.setFont(u8g2_font_5x7_tr);
    char percentText[8] = {};
    std::snprintf(percentText, sizeof(percentText), "%3u%%",
                  static_cast<unsigned>(progressPct));
    display_.drawStr(97, 24, percentText);
  }

  void drawCalibrationResult() {
    display_.setFont(u8g2_font_6x10_tr);
    display_.drawStr(centeredX6x10("Kalibracja karmnika"), 9, "Kalibracja karmnika");
    if (calibrationOk_) {
      display_.drawStr(centeredX6x10("zakonczona"), 20, "zakonczona");
      display_.drawLine(49, 25, 57, 30);
      display_.drawLine(57, 30, 77, 14);
    } else {
      display_.drawStr(centeredX6x10("BLAD"), 20, "BLAD");
      display_.drawLine(54, 14, 74, 30);
      display_.drawLine(74, 14, 54, 30);
    }
  }

  void drawCalibrationFrame(unsigned long nowMs) {
    if (calibrationPhase_ == CalibrationPhase::PROMPT) {
      drawCalibrationPrompt();
      return;
    }

    if (calibrationPhase_ == CalibrationPhase::RUNNING) {
      drawCalibrationAnimation(nowMs - calibrationPhaseStartMs_);
      return;
    }

    if (calibrationPhase_ == CalibrationPhase::RESULT) {
      drawCalibrationResult();
      return;
    }

    drawCalibrationPrompt();
  }

  void renderFrame(bool isUpPressed, bool isSelectPressed, bool isDownPressed) {
    if (!animation_) {
      return;
    }

    std::uint8_t hour = 0;
    std::uint8_t minute = 0;
    std::uint8_t second = 0;
    std::uint8_t day = 1;
    std::uint8_t month = 1;
    std::uint16_t year = 2026;
    getClockFields(hour, minute, second, day, month, year);

    const int nowMinutes = (static_cast<int>(hour) * 60) + static_cast<int>(minute);
    const bool lightOn = isScheduleActive(config_.lightMode, nowMinutes, config_.dayStartHour,
                                          config_.dayStartMinute, config_.dayEndHour,
                                          config_.dayEndMinute);
    const bool aerationOn =
        isScheduleActive(config_.aerationMode, nowMinutes, config_.aerationHourOn,
                         config_.aerationMinuteOn, config_.aerationHourOff,
                         config_.aerationMinuteOff);
    const bool filterOn = isScheduleActive(config_.filterMode, nowMinutes, config_.filterHourOn,
                                           config_.filterMinuteOn, config_.filterHourOff,
                                           config_.filterMinuteOff);
    const bool heaterOn =
        (config_.heaterMode == 0) && (temperatureC_ < static_cast<float>(config_.targetTemp) - 0.3f);

    char feedTime[6] = {};
    std::snprintf(feedTime, sizeof(feedTime), "%02u:%02u", config_.feedHour, config_.feedMinute);

    const float displayTemperature =
        manualTemperatureEnabled_ ? manualTemperatureC_ : temperatureC_;
    const std::uint8_t displayAeration =
        manualAerationEnabled_ ? manualAerationPercent_ : static_cast<std::uint8_t>(aerationOn ? 80 : 30);
    const std::uint8_t displayBattery =
        manualBatteryEnabled_ ? manualBatteryPercent_ : batteryPercent_;
    const float displayBatteryVoltage =
        manualBatteryEnabled_ ? (3.3f + static_cast<float>(displayBattery) * 0.009f)
                              : batteryVoltage_;

    animation_->setTemperature(displayTemperature);
    animation_->setAeration(displayAeration);
    animation_->setFilterStatus(filterOn);
    animation_->setLightStatus(lightOn);
    animation_->setHeaterStatus(heaterOn);
    animation_->setTime(hour, minute, second);
    animation_->setDate(day, month, year);
    animation_->setBatteryVoltage(displayBatteryVoltage);
    animation_->setBattery(displayBattery);
    animation_->setLightSchedule(config_.dayStartHour, config_.dayStartMinute, config_.dayEndHour,
                                 config_.dayEndMinute);
    animation_->setLightMode(config_.lightMode);
    animation_->setAerationSchedule(config_.aerationHourOn, config_.aerationMinuteOn,
                                    config_.aerationHourOff, config_.aerationMinuteOff);
    animation_->setAerationMode(config_.aerationMode);
    animation_->setFilterSchedule(config_.filterHourOn, config_.filterMinuteOn, config_.filterHourOff,
                                  config_.filterMinuteOff);
    animation_->setFilterMode(config_.filterMode);
    animation_->setTargetTempSetting(config_.targetTemp);
    animation_->setHeaterMode(config_.heaterMode);
    animation_->setFeedingSchedule(feedTime, config_.feedMode, 0);
    animation_->setFeedingAnimation(uiState_ == UiState::FEEDING);

    display_.clearBuffer();

    const bool confirmFrameDrawn =
        (uiState_ != UiState::CALIBRATION) && animation_->drawConfirmAnimationFrame();
    if (!confirmFrameDrawn) {
      switch (uiState_) {
      case UiState::HOME:
        animation_->drawFrame();
        break;
      case UiState::MENU:
        animation_->drawMenu(isUpPressed, isSelectPressed, isDownPressed);
        break;
      case UiState::SCHEDULE_LIGHT:
        animation_->drawSchedule(isUpPressed, isSelectPressed, isDownPressed);
        break;
      case UiState::SCHEDULE_AERATION:
        animation_->drawScheduleAeration(isUpPressed, isSelectPressed, isDownPressed);
        break;
      case UiState::SCHEDULE_FILTER:
        animation_->drawScheduleFilter(isUpPressed, isSelectPressed, isDownPressed);
        break;
      case UiState::SCHEDULE_TEMP:
        animation_->drawScheduleTemp(isUpPressed, isSelectPressed, isDownPressed);
        break;
      case UiState::SCHEDULE_FEEDING:
        animation_->drawScheduleFeeding(isUpPressed, isSelectPressed, isDownPressed);
        break;
      case UiState::LOGS:
        animation_->drawLogs(isUpPressed, isSelectPressed, isDownPressed);
        break;
      case UiState::SETTINGS_DATETIME:
        animation_->drawSettingsDateTime(isUpPressed, isSelectPressed, isDownPressed);
        break;
      case UiState::TESTS:
        animation_->drawTests(isUpPressed, isSelectPressed, isDownPressed);
        break;
      case UiState::CALIBRATION:
        drawCalibrationFrame(lastTickMs_);
        break;
      case UiState::ACCESS_POINT:
        animation_->drawAccessPointScreen("Akwarium-AP", "12345678", "192.168.4.1", 1);
        break;
      case UiState::BLUETOOTH:
        animation_->drawBluetoothScreen("Akwarium-BLE", true, false, 0, 123456);
        break;
      case UiState::FEEDING:
        animation_->drawFeedingScreen();
        break;
      }
    }

    display_.sendBuffer();
  }

  void captureAnimationChanges() {
    if (!animation_) {
      return;
    }

    if (animation_->hasScheduleChanged()) {
      config_.lightMode = animation_->getLightMode();
      config_.dayStartHour = animation_->getScheduleHourOn();
      config_.dayStartMinute = animation_->getScheduleMinOn();
      config_.dayEndHour = animation_->getScheduleHourOff();
      config_.dayEndMinute = animation_->getScheduleMinOff();

      config_.aerationMode = animation_->getAerationMode();
      config_.aerationHourOn = animation_->getAerationHourOn();
      config_.aerationMinuteOn = animation_->getAerationMinOn();
      config_.aerationHourOff = animation_->getAerationHourOff();
      config_.aerationMinuteOff = animation_->getAerationMinOff();

      config_.filterMode = animation_->getFilterMode();
      config_.filterHourOn = animation_->getFilterHourOn();
      config_.filterMinuteOn = animation_->getFilterMinOn();
      config_.filterHourOff = animation_->getFilterHourOff();
      config_.filterMinuteOff = animation_->getFilterMinOff();

      config_.heaterMode = animation_->getHeaterMode();
      config_.targetTemp = animation_->getTargetTemp();
      config_.feedHour = animation_->getFeedHour();
      config_.feedMinute = animation_->getFeedMinute();
      config_.feedMode = animation_->getFeedFreq();
    }

    if (animation_->hasTimeChanged()) {
      std::tm customTm{};
      customTm.tm_hour = animation_->getNewHour();
      customTm.tm_min = animation_->getNewMinute();
      customTm.tm_sec = animation_->getNewSecond();
      customTm.tm_mday = animation_->getNewDay();
      customTm.tm_mon = animation_->getNewMonth() - 1;
      customTm.tm_year = animation_->getNewYear() - 1900;
      const std::time_t newTime = std::mktime(&customTm);
      if (newTime > 0) {
        simulatedClock_ = std::chrono::system_clock::from_time_t(newTime);
      }
    }
  }

  const std::chrono::steady_clock::time_point processStart_ = std::chrono::steady_clock::now();

  FakeU8g2 display_{};
  std::unique_ptr<AquariumAnimation> animation_{};
  SimulatedConfig config_{};

  UiState uiState_ = UiState::HOME;
  UiState uiStateBeforeFeeding_ = UiState::HOME;
  bool feedingUiActive_ = false;
  unsigned long feedingUntilMs_ = 0;

  bool lastUpPressed_ = false;
  bool lastSelectPressed_ = false;
  bool lastDownPressed_ = false;
  bool pendingUp_ = false;
  bool pendingSelect_ = false;
  bool pendingDown_ = false;

  unsigned long lastUiInteractionMs_ = 0;
  unsigned long lastTickMs_ = 0;

  std::chrono::system_clock::time_point simulatedClock_{};

  double temperaturePhase_ = 0.0;
  float temperatureC_ = 24.0f;
  float batteryVoltage_ = 4.0f;
  std::uint8_t batteryPercent_ = 88;

  bool manualTemperatureEnabled_ = false;
  bool manualBatteryEnabled_ = false;
  bool manualAerationEnabled_ = false;
  float manualTemperatureC_ = 24.0f;
  std::uint8_t manualBatteryPercent_ = 88;
  std::uint8_t manualAerationPercent_ = 30;

  CalibrationPhase calibrationPhase_ = CalibrationPhase::IDLE;
  unsigned long calibrationPhaseStartMs_ = 0;
  bool calibrationOk_ = true;
};

FirmwareUiRuntime g_runtime;
std::mutex g_mutex;

} // namespace

#if defined(_WIN32)
#define FIRMWARE_UI_EXPORT extern "C" __declspec(dllexport)
#else
#define FIRMWARE_UI_EXPORT extern "C"
#endif

FIRMWARE_UI_EXPORT int initUI() {
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_runtime.initialize() ? 1 : 0;
}

FIRMWARE_UI_EXPORT void pressButtonUp() {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_runtime.pressUp();
}

FIRMWARE_UI_EXPORT void pressButtonDown() {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_runtime.pressDown();
}

FIRMWARE_UI_EXPORT void pressButtonSelect() {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_runtime.pressSelect();
}

FIRMWARE_UI_EXPORT void setManualTemperature(float value) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_runtime.setManualTemperature(value);
}

FIRMWARE_UI_EXPORT void setManualBatteryPercent(int value) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_runtime.setManualBatteryPercent(value);
}

FIRMWARE_UI_EXPORT void setManualAerationPercent(int value) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_runtime.setManualAerationPercent(value);
}

FIRMWARE_UI_EXPORT void addManualLog(const char *message, const char *timeText) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_runtime.addManualLog(message, timeText);
}

FIRMWARE_UI_EXPORT void clearManualLogs() {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_runtime.clearManualLogs();
}

FIRMWARE_UI_EXPORT void startCalibrationAnimation() {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_runtime.startCalibrationAnimation();
}

FIRMWARE_UI_EXPORT const std::uint8_t *getFrameBuffer() {
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_runtime.getFrameBuffer();
}
