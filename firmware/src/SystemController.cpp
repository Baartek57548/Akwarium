#include "SystemController.h"
#include "AkwariumWifi.h"
#include "AquariumAnimation.h"
#include "BleManager.h"
#include "ConfigManager.h"
#include "LogManager.h"
#include "OtaManager.h"
#include "PowerManager.h"
#include "ScheduleManager.h"
#include "SharedState.h"
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_sleep.h>
#include <esp_task_wdt.h>

#define HEATER_PIN 2
#define PUMP_PIN 4
#define FEEDER_PIN 3
#define SERVO_PIN 6
#define BUTTON_UP_PIN GPIO_NUM_15
#define BUTTON_SELECT_PIN GPIO_NUM_16
#define BUTTON_DOWN_PIN GPIO_NUM_14
#define ONE_WIRE_BUS 1
#define LIGHT_PIN 5
#define FEEDER_SENSOR_PIN 12
#define BAT_ADC_PIN 7
#define BAT_EN_PIN 10

// Relays for light/filter/feeder are active-low and wired on NC path in this
// build profile: LOW drives the relay and changes contact state.
static constexpr bool LIGHT_OUTPUT_ACTIVE_HIGH = false;
static constexpr bool PUMP_OUTPUT_ACTIVE_HIGH = false;
// Heater relay is wired on NO path with opposite behavior:
// HIGH keeps heater connected, LOW disconnects heater.
static constexpr bool HEATER_OUTPUT_ACTIVE_HIGH = true;
static constexpr bool FEEDER_OUTPUT_ACTIVE_HIGH = false;

static uint8_t outputLevelForState(bool enabled, bool activeHigh) {
  return enabled ? (activeHigh ? HIGH : LOW) : (activeHigh ? LOW : HIGH);
}

static void writeManagedOutput(uint8_t pin, bool enabled, bool activeHigh) {
  digitalWrite(pin, outputLevelForState(enabled, activeHigh));
}

static void logWakeupCauseOnBoot();
static bool isNightTimeNow();
static uint64_t computeSleepUsUntilDayStart(const DateTime &now);
static bool configureLightSleepWakeup();
static void syncBleWithOledState(bool oledShouldStayOn);

static const unsigned long LIGHT_SLEEP_IDLE_MS = 300000UL;
static const unsigned long NIGHT_INTERACTION_WINDOW_MS = 60000UL;
static const unsigned long OLED_IDLE_TIMEOUT_MS = 120000UL;
static bool wokeFromButtonThisBoot = false;

static void syncBleWithOledState(bool oledShouldStayOn) {
  const bool bleConnected = BleManager::isConnected();
  const bool bleAdvertising = BleManager::isAdvertising();

  // BLE jest sterowane przez UI, nie przez sam aktywny OLED.
  // Tutaj jedynie dopinamy awaryjne zatrzymanie reklamowania przy wygaszeniu
  // ekranu lub przejsciu do snu, o ile klient nie jest podlaczony.
  if (bleConnected) {
    return;
  }

  if (!oledShouldStayOn && bleAdvertising) {
    BleManager::stop();
  }
}

TemperatureController
    SystemController::tempController(ONE_WIRE_BUS, HEATER_PIN, 24.0f, 0.5f,
                                     HEATER_OUTPUT_ACTIVE_HIGH);
FeederController SystemController::feederController(FEEDER_PIN,
                                                    FEEDER_SENSOR_PIN, false,
                                                    FEEDER_OUTPUT_ACTIVE_HIGH);
ServoController SystemController::servoController(SERVO_PIN, 90);
BatteryReader SystemController::batteryReader(BAT_ADC_PIN);
RTC_DS3231 SystemController::rtc;

DateTime getCurrentDateTime() {
  if (SystemController::isRtcReady()) {
    return SystemController::rtc.now();
  }
  // Fallback bez RTC - monotoniczny czas oparty o millis, z neutralna data.
  return DateTime(2025, 1, 1, 0, 0, 0) +
         TimeSpan(static_cast<int32_t>(millis() / 1000UL));
}

void syncSystemTime(uint32_t epoch) {
  if (SystemController::isRtcReady()) {
    SystemController::rtc.adjust(DateTime(epoch));
  }
}

bool SystemController::manualServoOverride = false;
int SystemController::manualServoAngle = 90;
unsigned long SystemController::manualServoTimer = 0;

uint8_t SystemController::tempInvalidReadCount = 0;
bool SystemController::tempSensorErrorLogged = false;
bool SystemController::rtcReady = false;
int SystemController::lastResetReason = 0;

unsigned long SystemController::lastTempCheckMs = 0;
unsigned long SystemController::lastBatCheckMs =
    -600000UL; // Start pomiaru baterii natychmiast

bool SystemController::isRtcReady() { return rtcReady; }

int SystemController::getLastResetReason() { return lastResetReason; }

const char *SystemController::getLastResetLabel() {
  switch (static_cast<esp_reset_reason_t>(lastResetReason)) {
  case ESP_RST_TASK_WDT:
    return "watchdog";
  case ESP_RST_INT_WDT:
    return "watchdog";
  case ESP_RST_PANIC:
    return "panic";
  case ESP_RST_BROWNOUT:
    return "brownout";
  default:
    return nullptr;
  }
}

void SystemController::hardwareSetup() {
  pinMode(LIGHT_PIN, OUTPUT);
  writeManagedOutput(LIGHT_PIN, false, LIGHT_OUTPUT_ACTIVE_HIGH);
  pinMode(PUMP_PIN, OUTPUT);
  writeManagedOutput(PUMP_PIN, false, PUMP_OUTPUT_ACTIVE_HIGH);
  pinMode(HEATER_PIN, OUTPUT);
  writeManagedOutput(HEATER_PIN, true, HEATER_OUTPUT_ACTIVE_HIGH);

  pinMode(BAT_EN_PIN, OUTPUT);
  digitalWrite(BAT_EN_PIN, HIGH); // Załączenie dzielnika pomiarowego

  pinMode(static_cast<uint8_t>(BUTTON_UP_PIN), INPUT_PULLUP);
  pinMode(static_cast<uint8_t>(BUTTON_SELECT_PIN), INPUT_PULLUP);
  pinMode(static_cast<uint8_t>(BUTTON_DOWN_PIN), INPUT_PULLUP);

  tempController.begin();
  feederController.begin();
  servoController.begin();
  batteryReader.init();

  if (!rtc.begin()) {
    rtcReady = false;
    LogManager::logError("Nie znaleziono modulu RTC (DS3231)!");
  } else if (rtc.lostPower()) {
    rtcReady = true;
    LogManager::logWarn("RTC zresetowany, przywracanie domyslnego czasu...");
    rtc.adjust(DateTime(2025, 1, 1, 12, 0, 0));
  } else {
    rtcReady = true;
    // Sprawdz czy czas w RTC jest rozsądny (nie za stary)
    DateTime now = rtc.now();
    if (now.year() < 2024 || now.year() > 2030) {
      LogManager::logWarn("RTC ma niepoprawny czas, przywracanie domyslnego...");
      rtc.adjust(DateTime(2025, 1, 1, 12, 0, 0));
    }
  }
}

void SystemController::init() {
  SharedState::init();
  ConfigManager::init();
  LogManager::init();

  hardwareSetup();
  logWakeupCauseOnBoot();

  OtaManager::init();
  PowerManager::init(&batteryReader);
  ScheduleManager::init(&feederController);
  if (wokeFromButtonThisBoot) {
    PowerManager::registerActivity();
    LogManager::logInfo(
        "Wybudzenie przyciskiem GPIO14 - aktywne okno interakcji nocnej.");
  }

  // Rejestracja biezacego zadania (loop) do Watchdoga
  esp_task_wdt_add(NULL);

  lastResetReason = static_cast<int>(esp_reset_reason());
  const char *rstLabel = getLastResetLabel();
  if (rstLabel != nullptr) {
    char msg[80];
    snprintf(msg, sizeof(msg), "RESET: %s (code=%d)", rstLabel,
             lastResetReason);
    LogManager::logError(msg);
  } else {
    LogManager::logInfo("System zainicjalizowany poprawnie.");
  }
}

void SystemController::updateSensors() {
  unsigned long nowMs = millis();

  if (nowMs - lastTempCheckMs >= 2000) {
    lastTempCheckMs = nowMs;
    float tempVal = tempController.readTemperature();
    bool disconnected = (tempVal <= -127.0f);

    if (!disconnected && tempVal > -50.0f && tempVal < 100.0f) {
      tempInvalidReadCount = 0;
      // Aktualizacja mutex-protected
      SharedState::updateTemperature(tempVal, tempController.getDailyMin(), 0,
                                     tempController.getDailyMax());
      if (tempSensorErrorLogged) {
        LogManager::logInfo("Czujnik temp. powrocil.");
        tempSensorErrorLogged = false;
      }
    } else {
      if (tempInvalidReadCount < 255)
        tempInvalidReadCount++;
      if (tempInvalidReadCount >= 3 && !tempSensorErrorLogged) {
        LogManager::logError("Brak czujnika temperatury (DS18B20) lub awaria!");
        tempSensorErrorLogged = true;
      }
    }
  }

  if (nowMs - lastBatCheckMs >= 600000UL) { // 10 minut
    lastBatCheckMs = nowMs;
    batteryReader.startMeasurement();
  }

  PowerManager::update();
}

void SystemController::updateDecisions() {
  if (OtaManager::isOtaInProgress())
    return;

  DateTime now = getCurrentDateTime();
  SharedState::updateTime(now.hour(), now.minute(), now.second(), now.day(),
                          now.month(), now.year());

  ScheduleManager::update(now);

  const Config cfg = ConfigManager::getCopy();
  uint16_t nowMin = ScheduleManager::toMinutes(now.hour(), now.minute());

  bool isLightActive = ScheduleManager::isDayTime(nowMin);
  bool runFilter = ScheduleManager::isFilterActive(nowMin);
  bool runAeration = ScheduleManager::isAerationActive(nowMin);

  // Grzalka pracuje domyslnie w trybie "podlaczona", a sterownik realizuje
  // odciecie dopiero po przekroczeniu targetTemp + hysteresis. Ponowne
  // podlaczenie nastepuje przy temperaturze <= targetTemp.
  SharedStateData snap = SharedState::getSnapshot();
  if (cfg.heaterMode == static_cast<uint8_t>(HeaterMode::Threshold)) {
    tempController.setTargetTemperature(cfg.targetTemp);
    tempController.setHysteresis(cfg.tempHysteresis);
    if (!isnan(snap.temperature) && tempInvalidReadCount < 3) {
      tempController.controlHeater(snap.temperature);
    }
  } else if (tempController.isHeaterOn()) {
    tempController.forceHeaterOff();
  }

  // Decyzje dot. swiatla, filtra (Tymczasowe, tu wchodzi w gre UI zaleznie od
  // stanow. Tu zrobimy czyste auto) Ostateczna logika polaczy stany z
  // rendering.

  // Servo logic
  int servoTarget = SERVO_CLOSED_ANGLE;
  if (runAeration)
    servoTarget = SERVO_OPEN_ANGLE;
  int minsToOff = ScheduleManager::getMinutesUntilFilterOff(nowMin);
  if (minsToOff > 0 && minsToOff <= cfg.servoPreOffMins)
    servoTarget = SERVO_PREOFF_ANGLE;

  if (!isnan(snap.temperature) && snap.temperature > 30.0f) {
    servoTarget =
        constrain(cfg.servoAlarmAngle, SERVO_OPEN_ANGLE, SERVO_CLOSED_ANGLE);
  }

  if (manualServoOverride) {
    if (millis() - manualServoTimer > 300000UL) {
      manualServoOverride = false;
    } else {
      servoTarget =
          constrain(manualServoAngle, SERVO_OPEN_ANGLE, SERVO_CLOSED_ANGLE);
    }
  }

  // Aplikacja Serwa
  static int lastServoTarget = -1;
  if (servoTarget != lastServoTarget) {
    servoController.setPosition(servoTarget);
    lastServoTarget = servoTarget;
  }

  uint8_t aerationPct =
      map(servoTarget, SERVO_CLOSED_ANGLE, SERVO_OPEN_ANGLE, 0, 100);
  SharedState::updateAeration(aerationPct);

  bool isHeaterOn = tempController.isHeaterOn();

  SharedState::updateRelays(isHeaterOn, runFilter, isLightActive,
                            isLightActive);
}

void SystemController::applyOutputs() {
  if (OtaManager::isOtaInProgress())
    return;

  SharedStateData snap = SharedState::getSnapshot();
  writeManagedOutput(LIGHT_PIN, snap.isLightOn, LIGHT_OUTPUT_ACTIVE_HIGH);
  writeManagedOutput(PUMP_PIN, snap.isFilterOn, PUMP_OUTPUT_ACTIVE_HIGH);
  writeManagedOutput(HEATER_PIN, snap.isHeaterOn, HEATER_OUTPUT_ACTIVE_HIGH);

  servoController.update();
  feederController.update();
}

void SystemController::update() {
  esp_task_wdt_reset();

  updateSensors();
  updateDecisions();
  applyOutputs();
}

void SystemController::feedNow() {
  Error err = feederController.startFeed(1500, true);
  if (err == Error::NONE) {
    LogManager::logInfo("Reczne karmienie uruchomione.");
  }
}

bool SystemController::isFeedingNow() { return feederController.isFeeding(); }

void SystemController::setManualServo(int angle) {
  manualServoOverride = true;
  manualServoAngle = constrain(angle, SERVO_OPEN_ANGLE, SERVO_CLOSED_ANGLE);
  manualServoTimer = millis();
}

void SystemController::clearManualServo() { manualServoOverride = false; }

int SystemController::getServoPosition() {
  return servoController.getCurrentPosition();
}

static void logEspErr(const char *prefix, esp_err_t err) {
  char msg[96];
  snprintf(msg, sizeof(msg), "%s: %s (%d)", prefix, esp_err_to_name(err),
           static_cast<int>(err));
  LogManager::logError(msg);
}

static void logWakeupCauseOnBoot() {
  wokeFromButtonThisBoot = false;
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  switch (cause) {
  case ESP_SLEEP_WAKEUP_EXT1: {
    uint64_t mask = esp_sleep_get_ext1_wakeup_status();
    char msg[96];
    snprintf(msg, sizeof(msg), "Wakeup cause: EXT1 mask=0x%llX",
             static_cast<unsigned long long>(mask));
    LogManager::logInfo(msg);
    if ((mask & (1ULL << static_cast<uint64_t>(BUTTON_DOWN_PIN))) != 0ULL) {
      wokeFromButtonThisBoot = true;
    }
    break;
  }
  case ESP_SLEEP_WAKEUP_EXT0:
    LogManager::logInfo("Wakeup cause: EXT0");
    wokeFromButtonThisBoot = true;
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    LogManager::logInfo("Wakeup cause: TIMER");
    break;
  case ESP_SLEEP_WAKEUP_UNDEFINED:
    LogManager::logInfo("Wakeup cause: COLD BOOT");
    break;
  default: {
    char msg[64];
    snprintf(msg, sizeof(msg), "Wakeup cause: %d", static_cast<int>(cause));
    LogManager::logInfo(msg);
    break;
  }
  }
}

static bool isNightTimeNow() {
  if (!SystemController::isRtcReady()) {
    return false;
  }
  DateTime now = getCurrentDateTime();
  uint16_t nowMin = ScheduleManager::toMinutes(now.hour(), now.minute());
  return !ScheduleManager::isDayTime(nowMin);
}

bool SystemController::canEnterLightSleep(unsigned long nowMs,
                                          unsigned long lastActionMs) {
  if ((nowMs - lastActionMs) < LIGHT_SLEEP_IDLE_MS) {
    return false;
  }
  const SharedStateData snap = SharedState::getSnapshot();
  if (snap.isLightOn || snap.isFilterOn) {
    return false;
  }
  if (OtaManager::isOtaInProgress()) {
    return false;
  }
  if (AkwariumWifi::getIsAPMode()) {
    return false;
  }
  if (!AkwariumWifi::isStaOff()) {
    return false;
  }
  if (BleManager::isAdvertising() || BleManager::isConnected()) {
    return false;
  }
  if (SystemController::isFeedingNow()) {
    return false;
  }
  return true;
}

static uint64_t computeSleepUsUntilDayStart(const DateTime &now) {
  const Config cfg = ConfigManager::getCopy();
  if (cfg.dayStartHour > 23 || cfg.dayStartMinute > 59) {
    LogManager::logWarn(
        "Niepoprawny start dnia, timer sleep ustawiony na 30 min.");
    return 30ULL * 60ULL * 1000000ULL;
  }

  DateTime nextDayStart(now.year(), now.month(), now.day(), cfg.dayStartHour,
                        cfg.dayStartMinute, 0);
  if (nextDayStart <= now) {
    nextDayStart = nextDayStart + TimeSpan(1, 0, 0, 0);
  }

  uint32_t diffSec = nextDayStart.unixtime() - now.unixtime();
  if (diffSec == 0) {
    diffSec = 1;
  }

  char msg[96];
  snprintf(msg, sizeof(msg), "Timer wakeup za %lu s (start dnia %02u:%02u).",
           static_cast<unsigned long>(diffSec),
           static_cast<unsigned>(cfg.dayStartHour),
           static_cast<unsigned>(cfg.dayStartMinute));
  LogManager::logInfo(msg);

  return static_cast<uint64_t>(diffSec) * 1000000ULL;
}

static bool configureLightSleepWakeup() {
  const gpio_num_t wakePins[] = {BUTTON_UP_PIN, BUTTON_SELECT_PIN,
                                 BUTTON_DOWN_PIN};

  for (gpio_num_t wakePin : wakePins) {
    esp_err_t err = gpio_wakeup_enable(wakePin, GPIO_INTR_LOW_LEVEL);
    if (err != ESP_OK) {
      char label[48];
      snprintf(label, sizeof(label), "gpio_wakeup_enable(GPIO%d)",
               static_cast<int>(wakePin));
      logEspErr(label, err);
      return false;
    }
  }

  const esp_err_t err = esp_sleep_enable_gpio_wakeup();
  if (err != ESP_OK) {
    logEspErr("esp_sleep_enable_gpio_wakeup", err);
    return false;
  }

  return true;
}

static void drawCenteredLine(U8G2 *display, const char *text, int baselineY) {
  int16_t w = display->getStrWidth(text);
  int16_t x = (128 - w) / 2;
  if (x < 0)
    x = 0;
  display->drawStr(x, baselineY, text);
}

static void drawFeederCalibrationPrompt(U8G2 *display, const char *line1,
                                        const char *line2 = nullptr,
                                        const char *line3 = nullptr) {
  if (!display)
    return;
  display->clearBuffer();
  display->setFont(u8g2_font_6x10_tr);
  drawCenteredLine(display, line1, 10);
  if (line2 != nullptr)
    drawCenteredLine(display, line2, 21);
  if (line3 != nullptr)
    drawCenteredLine(display, line3, 31);
  display->sendBuffer();
}

static void drawFeederCalibrationAnimation(U8G2 *display,
                                           unsigned long elapsedMs,
                                           unsigned long expectedMs) {
  if (!display)
    return;

  static const int8_t spinnerX[8] = {0, 2, 3, 2, 0, -2, -3, -2};
  static const int8_t spinnerY[8] = {-3, -2, 0, 2, 3, 2, 0, -2};
  static const int8_t waveY[8] = {0, 1, 2, 1, 0, -1, -2, -1};

  unsigned long clampedElapsed =
      (expectedMs > 0) ? min(elapsedMs, expectedMs) : elapsedMs;
  uint8_t progressPct =
      (expectedMs > 0)
          ? static_cast<uint8_t>((clampedElapsed * 100UL) / expectedMs)
          : 0;
  uint8_t progressFill =
      (expectedMs > 0)
          ? static_cast<uint8_t>((clampedElapsed * 112UL) / expectedMs)
          : 0;
  if (progressFill > 112)
    progressFill = 112;

  uint8_t spinnerPhase = static_cast<uint8_t>((elapsedMs / 90UL) % 8UL);
  int pelletX = 30 + static_cast<int>((elapsedMs / 24UL) % 78UL);
  uint8_t wavePhase = static_cast<uint8_t>(((elapsedMs / 60UL) + pelletX) % 8);
  int pelletY = 18 + waveY[wavePhase];

  display->clearBuffer();
  display->setFont(u8g2_font_6x10_tr);
  drawCenteredLine(display, "Kalibracja karmnika", 9);

  display->drawFrame(3, 11, 122, 14);
  display->drawHLine(26, 18, 94);
  display->drawDisc(pelletX, pelletY, 2, U8G2_DRAW_ALL);

  for (uint8_t i = 0; i < 8; i++) {
    uint8_t idx = (spinnerPhase + i) % 8;
    int x = 15 + spinnerX[idx];
    int y = 18 + spinnerY[idx];
    if (i < 2) {
      display->drawDisc(x, y, 1, U8G2_DRAW_ALL);
    } else {
      display->drawPixel(x, y);
    }
  }

  display->drawFrame(8, 27, 114, 4);
  if (progressFill > 0)
    display->drawBox(9, 28, progressFill, 2);

  display->setFont(u8g2_font_5x7_tr);
  char percentText[8];
  snprintf(percentText, sizeof(percentText), "%3u%%",
           static_cast<unsigned>(progressPct));
  display->drawStr(97, 24, percentText);

  display->sendBuffer();
}

static void drawFeederCalibrationResult(U8G2 *display, bool ok) {
  if (!display)
    return;

  display->clearBuffer();
  display->setFont(u8g2_font_6x10_tr);
  drawCenteredLine(display, "Kalibracja karmnika", 9);

  if (ok) {
    drawCenteredLine(display, "zakonczona", 20);
    display->drawLine(49, 25, 57, 30);
    display->drawLine(57, 30, 77, 14);
  } else {
    drawCenteredLine(display, "BLAD", 20);
    display->drawLine(54, 14, 74, 30);
    display->drawLine(74, 14, 54, 30);
  }

  display->sendBuffer();
}

bool SystemController::runFeederCalibration(U8G2 *display) {
  constexpr unsigned long CALIBRATION_EXPECTED_MS = 7000UL;

  LogManager::logInfo("Kalibracja karmnika uruchomiona z menu.");
  drawFeederCalibrationPrompt(display, "Kalibracja karmnika",
                              "Przygotowanie...");
  delay(250);

  feederController.clearError();
  Error startErr = feederController.startFeed(CALIBRATION_EXPECTED_MS, true);
  if (startErr != Error::NONE) {
    LogManager::logError("Blad startu kalibracji karmnika.");
    drawFeederCalibrationPrompt(display, "Kalibracja karmnika", "BLAD STARTU");
    delay(1200);
    return false;
  }

  const unsigned long startMs = millis();
  while (feederController.isFeeding()) {
    drawFeederCalibrationAnimation(display, millis() - startMs,
                                   CALIBRATION_EXPECTED_MS);
    delay(16);
  }

  bool ok = (feederController.getLastError() == Error::NONE);
  if (ok) {
    LogManager::logInfo("Kalibracja karmnika zakonczona powodzeniem.");
  } else {
    LogManager::logError("Kalibracja karmnika zakonczona bledem.");
  }

  drawFeederCalibrationResult(display, ok);
  delay(1200);
  return ok;
}

void SystemController::enterNightLightSleep() {
  if (!rtcReady) {
    LogManager::logWarn("RTC niedostepny - pomijam light sleep.");
    return;
  }
  DateTime now = getCurrentDateTime();

  LogManager::logInfo("Noc: przechodze do light sleep.");

  SharedState::updateRelays(tempController.isHeaterOn(), false, false, false);

  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  if (!configureLightSleepWakeup()) {
    LogManager::logError(
        "Konfiguracja wakeup dla light sleep nieudana - pomijam light sleep.");
    return;
  }

  uint64_t timerWakeUs = computeSleepUsUntilDayStart(now);
  esp_err_t err = esp_sleep_enable_timer_wakeup(timerWakeUs);
  if (err != ESP_OK) {
    logEspErr("esp_sleep_enable_timer_wakeup", err);
    return;
  }

  Serial.flush();
  err = esp_light_sleep_start();
  if (err != ESP_OK) {
    logEspErr("esp_light_sleep_start", err);
    return;
  }

  const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause == ESP_SLEEP_WAKEUP_GPIO) {
    PowerManager::registerActivity();
    LogManager::logInfo("Wybudzenie z light sleep przez GPIO.");
  } else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
    LogManager::logInfo("Wybudzenie z light sleep przez timer.");
  }
}

void SystemController::handlePowerManagement(U8G2 *display,
                                             AquariumAnimation *anim) {
  unsigned long nowMs = millis();
  unsigned long lastAction = PowerManager::getLastActivityTime();
  const Config cfg = ConfigManager::getCopy();
  const SharedStateData snap = SharedState::getSnapshot();
  const bool bleConnected = BleManager::isConnected();

  auto handleOnlyOledTimeout = [&](unsigned long timeoutMs) {
    const bool shouldKeepOledOn = bleConnected || cfg.alwaysScreenOn ||
                                  ((nowMs - lastAction) <= timeoutMs);

    if (!shouldKeepOledOn) {
      if (PowerManager::getCurrentMode() == MODE_ACTIVE) {
        PowerManager::setMode(MODE_LOW_POWER);
      }
      if (display) {
        display->setPowerSave(1);
      }
      syncBleWithOledState(false);
    } else {
      if (PowerManager::getCurrentMode() != MODE_ACTIVE) {
        PowerManager::setMode(MODE_ACTIVE);
      }
      // OLED ma sie wybudzic po aktywnosci przyciskow nawet wtedy,
      // gdy registerActivity() juz przelaczyl mode na ACTIVE.
      if (display) {
        display->setPowerSave(0);
      }
      syncBleWithOledState(true);
    }
  };

  // Gdy swiatlo lub filtr sa wlaczone, nie usypiamy calego urzadzenia.
  // Wylaczamy tylko OLED po 2 minutach bez aktywnosci.
  if (snap.isLightOn || snap.isFilterOn) {
    handleOnlyOledTimeout(OLED_IDLE_TIMEOUT_MS);
    return;
  }

  if (!isNightTimeNow()) {
    if (!AkwariumWifi::getIsAPMode() && AkwariumWifi::isStaOff()) {
      AkwariumWifi::requestStaOn();
    }
    handleOnlyOledTimeout(OLED_IDLE_TIMEOUT_MS);
    return;
  }

  if (!AkwariumWifi::isStaOff()) {
    AkwariumWifi::requestStaOffForSleep();
  }

  if (!canEnterLightSleep(nowMs, lastAction)) {
    bool keepInteractive = bleConnected || cfg.alwaysScreenOn ||
                           ((nowMs - lastAction) < NIGHT_INTERACTION_WINDOW_MS);
    if (display) {
      display->setPowerSave(keepInteractive ? 0 : 1);
    }
    PowerManager::setMode(keepInteractive ? MODE_ACTIVE : MODE_LOW_POWER);
    syncBleWithOledState(keepInteractive);
    return;
  }

  if (display) {
    display->clearBuffer();
    display->sendBuffer();
    display->setPowerSave(1);
  }

  syncBleWithOledState(false);
  PowerManager::setMode(MODE_LIGHT_SLEEP);
  enterNightLightSleep();
}
