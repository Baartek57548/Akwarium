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
#include "SystemDiagnostics.h"
#include "TimeUtils.h"
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_err.h>
#include <esp_sleep.h>
#include <esp_task_wdt.h>
#include <sys/time.h>

#define HEATER_PIN 3
#define PUMP_PIN 4
#define FEEDER_PIN 5
#define SERVO_PIN 6
#define BUTTON_UP_PIN GPIO_NUM_15
#define BUTTON_SELECT_PIN GPIO_NUM_16
#define BUTTON_DOWN_PIN GPIO_NUM_14
#define ONE_WIRE_BUS 1
#define LIGHT_PIN 17
#define FEEDER_SENSOR_PIN 12
#define BAT_ADC_PIN 7
#define BAT_EN_PIN 10

static void logWakeupCauseOnBoot();
static void releaseDeepSleepHolds();
static bool isNightTimeNow();
static uint64_t computeSleepUsUntilDayStart(const DateTime &now);
static bool configureButtonWakeup();
static bool holdOutputsInOffState();
static int clampServoAngleByLimits(int angle);
static uint8_t outputLevelFromState(bool isOn);

static const unsigned long DEEP_SLEEP_IDLE_MS = 300000UL;
static const unsigned long NIGHT_INTERACTION_WINDOW_MS = 60000UL;
static const unsigned long SERVO_TARGET_STABLE_MS = 1200UL;
static bool brownoutMitigationActive = false;
static unsigned long brownoutMitigationUntilMs = 0;
static bool wokeFromButtonThisBoot = false;
static int servoLastTarget = -1;
static int servoPendingTarget = -1;
static unsigned long servoPendingSinceMs = 0;
static unsigned long servoLastCmdMs = 0;
static portMUX_TYPE testOverrideMux = portMUX_INITIALIZER_UNLOCKED;

static int clampServoAngleByLimits(int angle) {
  const int servoMin = min(SERVO_OPEN_ANGLE, SERVO_CLOSED_ANGLE);
  const int servoMax = max(SERVO_OPEN_ANGLE, SERVO_CLOSED_ANGLE);
  return constrain(angle, servoMin, servoMax);
}

static uint8_t outputLevelFromState(bool isOn) {
  // Relays are active-low: ON -> LOW, OFF -> HIGH.
  return isOn ? LOW : HIGH;
}

TemperatureController SystemController::tempController(ONE_WIRE_BUS,
                                                       HEATER_PIN);
FeederController SystemController::feederController(FEEDER_PIN,
                                                    FEEDER_SENSOR_PIN);
ServoController SystemController::servoController(SERVO_PIN, 90);
BatteryReader SystemController::batteryReader(BAT_ADC_PIN);
RTC_DS3231 SystemController::rtc;

static DateTime getCurrentUtcDateTime() {
  if (SystemController::isRtcReady()) {
    return SystemController::rtc.now();
  }
  return DateTime(2025, 1, 1, 0, 0, 0) +
         TimeSpan(static_cast<int32_t>(millis() / 1000UL));
}

DateTime getCurrentDateTime() {
  if (SystemController::isRtcReady()) {
    return TimeUtils::utcDateTimeToLocal(getCurrentUtcDateTime());
  }
  // Fallback bez RTC - monotoniczny czas oparty o millis, z neutralna data.
  return DateTime(2025, 1, 1, 0, 0, 0) +
         TimeSpan(static_cast<int32_t>(millis() / 1000UL));
}

void syncSystemTime(uint32_t epoch) {
  struct timeval tv;
  tv.tv_sec = static_cast<time_t>(epoch);
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);

  if (SystemController::isRtcReady()) {
    SystemController::rtc.adjust(DateTime(epoch));
  }
}

bool SystemController::manualServoOverride = false;
int SystemController::manualServoAngle = 90;
unsigned long SystemController::manualServoTimer = 0;
bool SystemController::testOverrideActive = false;
bool SystemController::testLightOn = false;
bool SystemController::testHeaterOn = false;
bool SystemController::testFilterOn = false;
uint8_t SystemController::testAerationPercent = 0;

uint8_t SystemController::tempInvalidReadCount = 0;
bool SystemController::tempSensorErrorLogged = false;
bool SystemController::rtcReady = false;

unsigned long SystemController::lastTempCheckMs = 0;
unsigned long SystemController::lastBatCheckMs =
    -600000UL; // Start pomiaru baterii natychmiast

bool SystemController::isRtcReady() { return rtcReady; }

void SystemController::hardwareSetup() {
  releaseDeepSleepHolds();
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, outputLevelFromState(false)); // OFF
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, outputLevelFromState(false)); // OFF
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, outputLevelFromState(false)); // OFF

  pinMode(BAT_EN_PIN, OUTPUT);
  digitalWrite(BAT_EN_PIN, HIGH); // Załączenie dzielnika pomiarowego

  // Po wybudzeniu pin wake moze pozostac w trybie RTC GPIO.
  rtc_gpio_deinit(BUTTON_DOWN_PIN);
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
    DateTime fallbackLocal(2025, 1, 1, 12, 0, 0);
    rtc.adjust(DateTime(TimeUtils::localDateTimeToUtcEpoch(fallbackLocal)));
  } else {
    rtcReady = true;
  }
}

void SystemController::init() {
  TimeUtils::initTimezone();

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

  esp_reset_reason_t reason = esp_reset_reason();
  esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
  SystemDiagnostics::init(reason, wakeCause);
  const SystemDiagSnapshot diag = SystemDiagnostics::getSnapshot();

  char diagMsg[200];
  snprintf(diagMsg, sizeof(diagMsg),
           "Diag boot=%lu reset=%s wake=%s brownout=%lu wdt=%lu panic=%lu",
           static_cast<unsigned long>(diag.bootCount), diag.lastResetReason,
           diag.lastWakeupCause, static_cast<unsigned long>(diag.brownoutCount),
           static_cast<unsigned long>(diag.wdtCount),
           static_cast<unsigned long>(diag.panicCount));
  LogManager::logInfo(diagMsg);

  if (reason == ESP_RST_BROWNOUT) {
    brownoutMitigationActive = true;
    brownoutMitigationUntilMs = millis() + (10UL * 60UL * 1000UL);
    LogManager::logWarn(
        "Brownout mitigation: ograniczam czestotliwosc ruchu serwa na 10 min.");
    LogManager::logError("System zresetowany przez Brownout (spadek zasilania)!");
  } else if (reason == ESP_RST_TASK_WDT || reason == ESP_RST_INT_WDT ||
             reason == ESP_RST_WDT) {
    LogManager::logError(
        "System zresetowany przez Watchdog (Deadlock/Soft Lock)!");
  } else if (reason == ESP_RST_PANIC) {
    LogManager::logError("System zresetowany przez Panic/Exception!");
  } else if (reason == ESP_RST_DEEPSLEEP &&
             wakeCause != ESP_SLEEP_WAKEUP_UNDEFINED) {
    LogManager::logInfo("DEEP_SLEEP_WAKE: wybudzenie z deep sleep (to nie crash).");
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

  if (brownoutMitigationActive &&
      static_cast<long>(millis() - brownoutMitigationUntilMs) >= 0) {
    brownoutMitigationActive = false;
    LogManager::logInfo("Brownout mitigation: wygaszony.");
  }

  DateTime now = getCurrentDateTime();
  SharedState::updateTime(now.hour(), now.minute(), now.second(), now.day(),
                          now.month(), now.year());

  ScheduleManager::update(now);

  const Config cfg = ConfigManager::getCopy();
  uint16_t nowMin = ScheduleManager::toMinutes(now.hour(), now.minute());

  bool isDay = ScheduleManager::isDayTime(nowMin);
  bool runFilter = ScheduleManager::isFilterActive(nowMin);
  bool runAeration = ScheduleManager::isAerationActive(nowMin);
  if (!isDay) {
    runFilter = false;
    runAeration = false;
  }

  bool testOverrideActiveSnap = false;
  bool testLightOnSnap = false;
  bool testHeaterOnSnap = false;
  bool testFilterOnSnap = false;
  uint8_t testAerationPercentSnap = 0;

  portENTER_CRITICAL(&testOverrideMux);
  testOverrideActiveSnap = testOverrideActive;
  testLightOnSnap = testLightOn;
  testHeaterOnSnap = testHeaterOn;
  testFilterOnSnap = testFilterOn;
  testAerationPercentSnap = testAerationPercent;
  portEXIT_CRITICAL(&testOverrideMux);

  if (testOverrideActiveSnap) {
    tempController.forceHeaterOff();

    const uint8_t aerationPercent = constrain(testAerationPercentSnap, 0, 100);
    const int servoTarget =
        map(aerationPercent, 0, 100, SERVO_CLOSED_ANGLE, SERVO_OPEN_ANGLE);

    const unsigned long nowMs = millis();
    if (servoTarget != servoPendingTarget) {
      servoPendingTarget = servoTarget;
      servoPendingSinceMs = nowMs;
    }

    const bool pendingChanged = (servoPendingTarget != servoLastTarget);
    if (pendingChanged && !servoController.isMoving()) {
      servoController.setPosition(servoPendingTarget);
      servoLastCmdMs = nowMs;
      servoLastTarget = servoPendingTarget;
    }

    SharedState::updateAeration(aerationPercent);
    SharedState::updateRelays(testHeaterOnSnap, testFilterOnSnap, testLightOnSnap,
                              isDay);
    return;
  }

  // Sterowanie grzalka (Tylko jesli odczyt temp jest wzglednie swiezy)
  SharedStateData snap = SharedState::getSnapshot();
  if (isDay && !isnan(snap.temperature) && tempInvalidReadCount < 3) {
    tempController.setTargetTemperature(cfg.targetTemp);
    tempController.setHysteresis(cfg.tempHysteresis);
    tempController.controlHeater(snap.temperature);
  } else {
    tempController.forceHeaterOff(); // Noc i awarie: grzalka zawsze OFF
  }

  // Decyzje dot. swiatla, filtra (Tymczasowe, tu wchodzi w gre UI zaleznie od
  // stanow. Tu zrobimy czyste auto) Ostateczna logika polaczy stany z
  // rendering.

  // Servo logic
  int servoTarget = SERVO_CLOSED_ANGLE;
  const char *servoReason = "night_or_idle";
  if (runAeration) {
    servoTarget = SERVO_OPEN_ANGLE;
    servoReason = "aeration_window";
  }
  int minsToOff = ScheduleManager::getMinutesUntilFilterOff(nowMin);
  if (minsToOff > 0 && minsToOff <= cfg.servoPreOffMins) {
    servoTarget = SERVO_PREOFF_ANGLE;
    servoReason = "filter_preoff_window";
  }

  static bool highTempServoAlarm = false;
  if (!isnan(snap.temperature)) {
    if (!highTempServoAlarm && snap.temperature >= 30.3f) {
      highTempServoAlarm = true;
    } else if (highTempServoAlarm && snap.temperature <= 29.7f) {
      highTempServoAlarm = false;
    }
  }

  if (highTempServoAlarm) {
    servoTarget = clampServoAngleByLimits(cfg.servoAlarmAngle);
    servoReason = "temp_alarm";
  }

  if (manualServoOverride) {
    if (millis() - manualServoTimer > 300000UL) {
      manualServoOverride = false;
    } else {
      servoTarget = clampServoAngleByLimits(manualServoAngle);
      servoReason = "manual_override";
    }
  }

  // Aplikacja Serwa
  const unsigned long nowMs = millis();
  if (servoTarget != servoPendingTarget) {
    servoPendingTarget = servoTarget;
    servoPendingSinceMs = nowMs;
  }

  const bool pendingChanged = (servoPendingTarget != servoLastTarget);
  const bool pendingStable =
      (nowMs - servoPendingSinceMs) >= SERVO_TARGET_STABLE_MS;
  const bool manualImmediate = manualServoOverride;

  if (pendingChanged && !servoController.isMoving() &&
      (manualImmediate || pendingStable)) {
    servoController.setPosition(servoPendingTarget);
    servoLastCmdMs = nowMs;

    char msg[170];
    snprintf(msg, sizeof(msg),
             "Servo -> %d (reason=%s, day=%d, aer=%d, filt=%d, minsToOff=%d, temp=%.2f)",
             servoPendingTarget, servoReason, isDay ? 1 : 0, runAeration ? 1 : 0,
             runFilter ? 1 : 0, minsToOff,
             isnan(snap.temperature) ? -99.0f : snap.temperature);
    LogManager::logInfo(msg);

    servoLastTarget = servoPendingTarget;
  }

  uint8_t aerationPct =
      map(servoTarget, SERVO_CLOSED_ANGLE, SERVO_OPEN_ANGLE, 0, 100);
  SharedState::updateAeration(aerationPct);

  bool isHeaterOn = isDay && tempController.isHeaterOn();

  // Aplikacja relejow na zewnatrz (Light i Filter). TBD: Powiazanie z
  // globalnymi stanowiskami z UI. Dla stabilnosci na czas przejscia UI
  // zostawimy to do wcisniecia w applyOutputs.
  SharedState::updateRelays(isHeaterOn, runFilter, isDay, isDay);
}

void SystemController::applyOutputs() {
  if (OtaManager::isOtaInProgress())
    return;

  SharedStateData snap = SharedState::getSnapshot();
  digitalWrite(LIGHT_PIN, outputLevelFromState(snap.isLightOn));
  digitalWrite(PUMP_PIN, outputLevelFromState(snap.isFilterOn));
  digitalWrite(HEATER_PIN, outputLevelFromState(snap.isHeaterOn));

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
  manualServoAngle = clampServoAngleByLimits(angle);
  manualServoTimer = millis();
}

void SystemController::clearManualServo() { manualServoOverride = false; }

int SystemController::getServoPosition() {
  return servoController.getCurrentPosition();
}

void SystemController::setTestOverride(bool lightOn, bool heaterOn,
                                       bool filterOn, uint8_t aerationPercent) {
  const uint8_t clampedAeration = constrain(aerationPercent, 0, 100);
  bool changed = false;

  portENTER_CRITICAL(&testOverrideMux);
  changed = (!testOverrideActive) || (testLightOn != lightOn) ||
            (testHeaterOn != heaterOn) || (testFilterOn != filterOn) ||
            (testAerationPercent != clampedAeration);

  testOverrideActive = true;
  testLightOn = lightOn;
  testHeaterOn = heaterOn;
  testFilterOn = filterOn;
  testAerationPercent = clampedAeration;
  portEXIT_CRITICAL(&testOverrideMux);

  if (changed) {
    char msg[128];
    snprintf(msg, sizeof(msg),
             "TEST override: light=%d heater=%d filter=%d servo=%u%%",
             testLightOn ? 1 : 0, testHeaterOn ? 1 : 0, testFilterOn ? 1 : 0,
             static_cast<unsigned>(testAerationPercent));
    LogManager::logInfo(msg);
  }
}

void SystemController::clearTestOverride() {
  bool wasActive = false;
  portENTER_CRITICAL(&testOverrideMux);
  wasActive = testOverrideActive;
  testOverrideActive = false;
  portEXIT_CRITICAL(&testOverrideMux);

  if (!wasActive) {
    return;
  }
  servoLastTarget = -1;
  servoPendingTarget = -1;
  servoPendingSinceMs = 0;
  LogManager::logInfo("TEST override: OFF (powrot do harmonogramu).");
}

bool SystemController::isTestOverrideActive() {
  bool active = false;
  portENTER_CRITICAL(&testOverrideMux);
  active = testOverrideActive;
  portEXIT_CRITICAL(&testOverrideMux);
  return active;
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

static void releaseDeepSleepHolds() {
  gpio_deep_sleep_hold_dis();
  gpio_hold_dis(static_cast<gpio_num_t>(LIGHT_PIN));
  gpio_hold_dis(static_cast<gpio_num_t>(PUMP_PIN));
  gpio_hold_dis(static_cast<gpio_num_t>(HEATER_PIN));
}

static bool isNightTimeNow() {
  if (!SystemController::isRtcReady()) {
    return false;
  }
  DateTime now = getCurrentDateTime();
  uint16_t nowMin = ScheduleManager::toMinutes(now.hour(), now.minute());
  return !ScheduleManager::isDayTime(nowMin);
}

bool SystemController::canEnterDeepSleep(unsigned long nowMs,
                                         unsigned long lastActionMs) {
  if ((nowMs - lastActionMs) < DEEP_SLEEP_IDLE_MS) {
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
  if (digitalRead(static_cast<uint8_t>(BUTTON_DOWN_PIN)) == LOW) {
    return false;
  }
  return true;
}

static uint64_t computeSleepUsUntilDayStart(const DateTime &now) {
  const Config cfg = ConfigManager::getCopy();
  if (cfg.dayStartHour > 23 || cfg.dayStartMinute > 59) {
    LogManager::logWarn("Niepoprawny start dnia, timer sleep ustawiony na 30 min.");
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
  snprintf(msg, sizeof(msg),
           "Timer wakeup za %lu s (start dnia %02u:%02u).",
           static_cast<unsigned long>(diffSec), static_cast<unsigned>(cfg.dayStartHour),
           static_cast<unsigned>(cfg.dayStartMinute));
  LogManager::logInfo(msg);

  return static_cast<uint64_t>(diffSec) * 1000000ULL;
}

static bool configureButtonWakeup() {
  if (!esp_sleep_is_valid_wakeup_gpio(BUTTON_DOWN_PIN) ||
      !rtc_gpio_is_valid_gpio(BUTTON_DOWN_PIN)) {
    LogManager::logError("GPIO14 nie jest poprawnym RTC wake pin.");
    return false;
  }

  esp_err_t err = rtc_gpio_deinit(BUTTON_DOWN_PIN);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    logEspErr("rtc_gpio_deinit(BUTTON_DOWN_PIN)", err);
    return false;
  }

  err = rtc_gpio_init(BUTTON_DOWN_PIN);
  if (err != ESP_OK) {
    logEspErr("rtc_gpio_init(BUTTON_DOWN_PIN)", err);
    return false;
  }

  err = rtc_gpio_set_direction(BUTTON_DOWN_PIN, RTC_GPIO_MODE_INPUT_ONLY);
  if (err != ESP_OK) {
    logEspErr("rtc_gpio_set_direction(BUTTON_DOWN_PIN)", err);
    return false;
  }

  err = rtc_gpio_pullup_en(BUTTON_DOWN_PIN);
  if (err != ESP_OK) {
    logEspErr("rtc_gpio_pullup_en(BUTTON_DOWN_PIN)", err);
    return false;
  }

  err = rtc_gpio_pulldown_dis(BUTTON_DOWN_PIN);
  if (err != ESP_OK) {
    logEspErr("rtc_gpio_pulldown_dis(BUTTON_DOWN_PIN)", err);
    return false;
  }

  // Dla pojedynczego przycisku EXT0 bywa stabilniejsze od EXT1.
  err = esp_sleep_enable_ext0_wakeup(BUTTON_DOWN_PIN, 0);
  if (err == ESP_OK) {
    LogManager::logInfo("Wake GPIO14: EXT0 (LOW) aktywny.");
    return true;
  }

  logEspErr("esp_sleep_enable_ext0_wakeup", err);
  uint64_t wakeMask = (1ULL << static_cast<uint64_t>(BUTTON_DOWN_PIN));
  err = esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_LOW);
  if (err != ESP_OK) {
    logEspErr("esp_sleep_enable_ext1_wakeup", err);
    return false;
  }

  LogManager::logWarn("Wake GPIO14: fallback do EXT1 (ANY_LOW).");
  return true;
}

static bool holdOutputsInOffState() {
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, outputLevelFromState(false)); // OFF
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, outputLevelFromState(false)); // OFF
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, outputLevelFromState(false)); // OFF

  esp_err_t err = gpio_hold_en(static_cast<gpio_num_t>(LIGHT_PIN));
  if (err != ESP_OK) {
    logEspErr("gpio_hold_en(LIGHT_PIN)", err);
    return false;
  }

  err = gpio_hold_en(static_cast<gpio_num_t>(PUMP_PIN));
  if (err != ESP_OK) {
    logEspErr("gpio_hold_en(PUMP_PIN)", err);
    return false;
  }

  err = gpio_hold_en(static_cast<gpio_num_t>(HEATER_PIN));
  if (err != ESP_OK) {
    logEspErr("gpio_hold_en(HEATER_PIN)", err);
    return false;
  }

  gpio_deep_sleep_hold_en();

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

static void drawFeederCalibrationCountdown(U8G2 *display,
                                           unsigned long remainingMs) {
  if (!display)
    return;

  const unsigned long remainingSec = remainingMs / 1000UL;
  const unsigned long minutes = remainingSec / 60UL;
  const unsigned long seconds = remainingSec % 60UL;

  char timerLine[24];
  snprintf(timerLine, sizeof(timerLine), "Pozostalo %02lu:%02lu", minutes,
           seconds);

  display->clearBuffer();
  display->setFont(u8g2_font_6x10_tr);
  drawCenteredLine(display, "KALIBRACJA KARMNIKA", 10);
  drawCenteredLine(display, timerLine, 22);
  drawCenteredLine(display, "Prosze czekac...", 31);
  display->sendBuffer();
}

static bool waitForCalibrationDecision(unsigned long timeoutMs) {
  const unsigned long startMs = millis();

  while (true) {
    if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
      while (digitalRead(BUTTON_SELECT_PIN) == LOW) {
        esp_task_wdt_reset();
        delay(5);
      }
      return true; // confirmed
    }

    if (digitalRead(BUTTON_UP_PIN) == LOW) {
      while (digitalRead(BUTTON_UP_PIN) == LOW) {
        esp_task_wdt_reset();
        delay(5);
      }
      return false; // canceled by user
    }

    if (millis() - startMs >= timeoutMs) {
      LogManager::logInfo("Kalibracja timeout -> autostart");
      return true; // auto-start calibration on timeout
    }

    esp_task_wdt_reset();
    delay(10);
  }
}

void SystemController::runFeederCalibrationOnPowerUp(U8G2 *display) {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  bool isPowerOnBoot = (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED);
  if (!isPowerOnBoot)
    return;

  const unsigned long HOLD_TO_CALIBRATE_MS = 1200UL;
  if (digitalRead(BUTTON_SELECT_PIN) != LOW) {
    LogManager::logInfo(
        "Kalibracja karmnika pominieta (przytrzymaj SELECT przy starcie, aby uruchomic).");
    return;
  }

  unsigned long holdStartMs = millis();
  while (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    esp_task_wdt_reset();
    if (millis() - holdStartMs >= HOLD_TO_CALIBRATE_MS) {
      break;
    }
    delay(10);
  }

  if (millis() - holdStartMs < HOLD_TO_CALIBRATE_MS) {
    LogManager::logInfo("Kalibracja pominieta (zbyt krotki hold SELECT).");
    return;
  }

  LogManager::logInfo("Tryb serwisowy: uruchamiam kalibracje karmnika.");
  drawFeederCalibrationPrompt(display, "KALIBRACJA...", "Trwa ustawianie",
                              "pozycji start");
  feederController.clearError();

  Error startErr = feederController.startFeed(1500, true);
  if (startErr != Error::NONE) {
    LogManager::logError("Blad startu kalibracji!");
    drawFeederCalibrationPrompt(display, "KALIBRACJA BLAD");
    delay(1200);
    return;
  }

  while (feederController.isFeeding()) {
    feederController.update();
    esp_task_wdt_reset();
    delay(2);
  }

  Error endErr = feederController.getLastError();
  if (endErr == Error::NONE) {
    LogManager::logInfo("Kalibracja OK.");
    drawFeederCalibrationPrompt(display, "KALIBRACJA OK");
  } else {
    LogManager::logError("Kalibracja FAILED!");
    drawFeederCalibrationPrompt(display, "KALIBRACJA BLAD");
  }
  delay(1200);
}

void SystemController::runFeederCalibrationFromMenu(U8G2 *display) {
  const unsigned long CALIBRATION_TIMEOUT_MS = 120000UL; // 2 min
  const unsigned long UI_REFRESH_MS = 100UL;
  const unsigned long FEEDER_DEFAULT_TIMEOUT_MS = 15000UL;

  LogManager::logInfo("Kalibracja karmnika uruchomiona z menu.");
  feederController.clearError();
  feederController.setSafetyTimeout(CALIBRATION_TIMEOUT_MS);

  Error startErr = feederController.startFeed(1500, true);
  if (startErr != Error::NONE) {
    LogManager::logError("Blad startu kalibracji z menu!");
    feederController.setSafetyTimeout(FEEDER_DEFAULT_TIMEOUT_MS);
    drawFeederCalibrationPrompt(display, "KALIBRACJA BLAD");
    delay(1200);
    return;
  }

  unsigned long startMs = millis();
  unsigned long lastUiMs = 0;

  while (feederController.isFeeding()) {
    feederController.update();
    esp_task_wdt_reset();

    const unsigned long nowMs = millis();
    if (nowMs - lastUiMs >= UI_REFRESH_MS) {
      lastUiMs = nowMs;
      const unsigned long elapsedMs = nowMs - startMs;
      const unsigned long remainingMs =
          (elapsedMs >= CALIBRATION_TIMEOUT_MS) ? 0UL
                                                : (CALIBRATION_TIMEOUT_MS -
                                                   elapsedMs);
      drawFeederCalibrationCountdown(display, remainingMs);
    }

    delay(5);
  }

  feederController.setSafetyTimeout(FEEDER_DEFAULT_TIMEOUT_MS);

  Error endErr = feederController.getLastError();
  if (endErr == Error::NONE) {
    LogManager::logInfo("Kalibracja z menu zakonczona poprawnie.");
    drawFeederCalibrationPrompt(display, "KALIBRACJA OK");
  } else if (endErr == Error::TIMEOUT) {
    LogManager::logError("Kalibracja z menu: timeout.");
    drawFeederCalibrationPrompt(display, "KALIBRACJA TIMEOUT");
  } else {
    LogManager::logError("Kalibracja z menu zakonczona bledem.");
    drawFeederCalibrationPrompt(display, "KALIBRACJA BLAD");
  }

  delay(1200);
}

void SystemController::enterNightDeepSleep() {
  if (!rtcReady) {
    LogManager::logWarn("RTC niedostepny - pomijam deep sleep.");
    return;
  }
  DateTime nowLocal = getCurrentDateTime();
  DateTime nowUtc = getCurrentUtcDateTime();

  LogManager::logInfo("Noc: wylaczam urzadzenia i przechodze do deep sleep.");
  char gateMsg[180];
  snprintf(gateMsg, sizeof(gateMsg),
           "Sleep gate ota=%d ap=%d staOff=%d bleAdv=%d bleConn=%d feeding=%d",
           OtaManager::isOtaInProgress() ? 1 : 0,
           AkwariumWifi::getIsAPMode() ? 1 : 0,
           AkwariumWifi::isStaOff() ? 1 : 0, BleManager::isAdvertising() ? 1 : 0,
           BleManager::isConnected() ? 1 : 0,
           SystemController::isFeedingNow() ? 1 : 0);
  LogManager::logInfo(gateMsg);

  char tsMsg[180];
  snprintf(tsMsg, sizeof(tsMsg),
           "Sleep start local=%04d-%02d-%02d %02d:%02d:%02d | utc=%04d-%02d-%02d %02d:%02d:%02d",
           nowLocal.year(), nowLocal.month(), nowLocal.day(), nowLocal.hour(),
           nowLocal.minute(), nowLocal.second(), nowUtc.year(), nowUtc.month(),
           nowUtc.day(), nowUtc.hour(), nowUtc.minute(), nowUtc.second());
  LogManager::logInfo(tsMsg);

  tempController.forceHeaterOff();
  SharedState::updateRelays(false, false, false, false);

  if (!holdOutputsInOffState()) {
    LogManager::logError("Nie udalo sie zablokowac wyjsc OFF przed deep sleep.");
    return;
  }

  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  esp_err_t err =
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  if (err != ESP_OK) {
    logEspErr("esp_sleep_pd_config", err);
    return;
  }

  if (!configureButtonWakeup()) {
    LogManager::logError("Konfiguracja wakeup GPIO14 nieudana - pomijam deep sleep.");
    return;
  }

  uint64_t timerWakeUs = computeSleepUsUntilDayStart(nowLocal);
  err = esp_sleep_enable_timer_wakeup(timerWakeUs);
  if (err != ESP_OK) {
    logEspErr("esp_sleep_enable_timer_wakeup", err);
    return;
  }

  Serial.flush();
  esp_deep_sleep_start();
}

void SystemController::handlePowerManagement(U8G2 *display,
                                             AquariumAnimation *anim) {
  (void)anim;
  unsigned long nowMs = millis();
  unsigned long lastAction = PowerManager::getLastActivityTime();
  const Config cfg = ConfigManager::getCopy();
  static bool deepSleepDisabledLogged = false;
  static bool screenPowerSave = false;

  // Tryb pracy bez deep sleep: wygaszamy tylko wyswietlacz po bezczynnosci.
  if (!deepSleepDisabledLogged) {
    LogManager::logInfo(
        "Deep sleep wylaczony: aktywne tylko wygaszanie wyswietlacza.");
    deepSleepDisabledLogged = true;
  }

  if ((nowMs - lastAction) > 240000UL &&
      !cfg.alwaysScreenOn) { // 4 minuty
    if (PowerManager::getCurrentMode() == MODE_ACTIVE) {
      PowerManager::setMode(MODE_LOW_POWER);
    }
    if (!screenPowerSave && display) {
        display->setPowerSave(1);
        screenPowerSave = true;
        LogManager::logInfo("Display: power-save ON (bezczynnosc).");
    }
  } else {
    if (PowerManager::getCurrentMode() != MODE_ACTIVE) {
      PowerManager::setMode(MODE_ACTIVE);
    }
    if (screenPowerSave && display) {
        display->setPowerSave(0);
        screenPowerSave = false;
        LogManager::logInfo("Display: power-save OFF (aktywność).");
    }
  }
}
