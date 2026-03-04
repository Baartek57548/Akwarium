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
#include <Preferences.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_err.h>
#include <esp_sleep.h>
#include <esp_task_wdt.h>

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
static bool isPlausibleEpoch(uint32_t epoch);
static bool restoreRtcFromBackup();
static void persistRtcBackupIfNeeded(uint32_t epoch, bool force = false);

static const unsigned long DEEP_SLEEP_IDLE_MS = 300000UL;
static const unsigned long NIGHT_INTERACTION_WINDOW_MS = 60000UL;
static const unsigned long RTC_BACKUP_WRITE_INTERVAL_MS = 600000UL;
static const float SERVO_ALARM_TEMP_ON_C = 30.0f;
static const float SERVO_ALARM_TEMP_OFF_C = 29.5f;
static const unsigned long SERVO_TARGET_STABLE_MS = 1200UL;
static bool wokeFromButtonThisBoot = false;
static Preferences rtcPrefs;
static bool rtcPrefsReady = false;
static uint32_t rtcBackupEpochCache = 0;
static unsigned long lastRtcBackupWriteMs = 0;
static bool servoTempAlarmActive = false;

TemperatureController SystemController::tempController(ONE_WIRE_BUS,
                                                       HEATER_PIN);
FeederController SystemController::feederController(FEEDER_PIN,
                                                    FEEDER_SENSOR_PIN);
ServoController SystemController::servoController(SERVO_PIN, 90);
BatteryReader SystemController::batteryReader(BAT_ADC_PIN);
RTC_DS3231 SystemController::rtc;

static bool ensureRtcPrefsReady() {
  if (rtcPrefsReady) {
    return true;
  }

  if (!rtcPrefs.begin("Akwarium", false)) {
    return false;
  }

  rtcPrefsReady = true;
  rtcBackupEpochCache = rtcPrefs.getULong("rtcEpoch", 0);
  return true;
}

static bool isPlausibleEpoch(uint32_t epoch) {
  // 2024-01-01 00:00:00 .. 2100-01-01 00:00:00
  return epoch >= 1704067200UL && epoch <= 4102444800UL;
}

static bool restoreRtcFromBackup() {
  if (!ensureRtcPrefsReady()) {
    return false;
  }

  uint32_t backupEpoch = rtcPrefs.getULong("rtcEpoch", 0);
  if (!isPlausibleEpoch(backupEpoch)) {
    return false;
  }

  SystemController::rtc.adjust(DateTime(backupEpoch));
  rtcBackupEpochCache = backupEpoch;
  return true;
}

static void persistRtcBackupIfNeeded(uint32_t epoch, bool force) {
  if (!isPlausibleEpoch(epoch)) {
    return;
  }

  if (!ensureRtcPrefsReady()) {
    return;
  }

  const unsigned long nowMs = millis();
  if (!force) {
    if (rtcBackupEpochCache != 0 && epoch <= rtcBackupEpochCache) {
      return;
    }
    if ((nowMs - lastRtcBackupWriteMs) < RTC_BACKUP_WRITE_INTERVAL_MS) {
      return;
    }
  }

  rtcPrefs.putULong("rtcEpoch", epoch);
  rtcBackupEpochCache = epoch;
  lastRtcBackupWriteMs = nowMs;
}

DateTime getCurrentDateTime() {
  if (SystemController::isRtcReady()) {
    return SystemController::rtc.now();
  }
  // Fallback bez RTC - monotoniczny czas oparty o millis, z neutralna data.
  return DateTime(2025, 1, 1, 0, 0, 0) +
         TimeSpan(static_cast<int32_t>(millis() / 1000UL));
}

void syncSystemTime(uint32_t epoch) {
  if (!isPlausibleEpoch(epoch)) {
    return;
  }

  if (SystemController::isRtcReady()) {
    SystemController::rtc.adjust(DateTime(epoch));
  }

  persistRtcBackupIfNeeded(epoch, true);
}

bool SystemController::manualServoOverride = false;
int SystemController::manualServoAngle = 90;
unsigned long SystemController::manualServoTimer = 0;

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
  digitalWrite(LIGHT_PIN, HIGH); // OFF (aktywny stan LOW)
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW); // OFF
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW); // OFF

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
    if (restoreRtcFromBackup()) {
      LogManager::logWarn(
          "RTC zresetowany - czas przywrocony z backupu NVS.");
    } else {
      LogManager::logWarn(
          "RTC zresetowany, brak backupu. Ustawiam domyslny czas.");
      rtc.adjust(DateTime(2025, 1, 1, 12, 0, 0));
      persistRtcBackupIfNeeded(rtc.now().unixtime(), true);
    }
  } else {
    rtcReady = true;
    persistRtcBackupIfNeeded(rtc.now().unixtime(), true);
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

  esp_reset_reason_t reason = esp_reset_reason();
  if (reason == ESP_RST_TASK_WDT) {
    LogManager::logError(
        "System zresetowany przez Task Watchdog (Deadlock/Soft Lock)!");
  } else if (reason == ESP_RST_PANIC) {
    LogManager::logError("System zresetowany przez Panic/Exception!");
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
  if (rtcReady) {
    persistRtcBackupIfNeeded(now.unixtime(), false);
  }

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
  if (runAeration)
    servoTarget = SERVO_OPEN_ANGLE;
  int minsToOff = ScheduleManager::getMinutesUntilFilterOff(nowMin);
  if (minsToOff > 0 && minsToOff <= cfg.servoPreOffMins)
    servoTarget = SERVO_PREOFF_ANGLE;

  if (!isnan(snap.temperature)) {
    if (servoTempAlarmActive) {
      if (snap.temperature <= SERVO_ALARM_TEMP_OFF_C) {
        servoTempAlarmActive = false;
      }
    } else if (snap.temperature >= SERVO_ALARM_TEMP_ON_C) {
      servoTempAlarmActive = true;
    }
  }

  if (servoTempAlarmActive) {
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

  // Aplikacja Serwa z filtrem stabilnosci celu, aby uniknac losowych
  // "szarpniec" przy chwilowych zmianach warunkow.
  static int lastServoTarget = -1;
  static int pendingServoTarget = -1;
  static unsigned long pendingServoSinceMs = 0;

  const unsigned long nowMs = millis();
  if (servoTarget != pendingServoTarget) {
    pendingServoTarget = servoTarget;
    pendingServoSinceMs = nowMs;
  }

  const bool manualImmediate = manualServoOverride;
  const bool pendingChanged = (pendingServoTarget != lastServoTarget);
  const bool pendingStable =
      (nowMs - pendingServoSinceMs) >= SERVO_TARGET_STABLE_MS;

  if (pendingChanged &&
      ((manualImmediate && !servoController.isMoving()) ||
       (pendingStable && !servoController.isMoving()))) {
    servoController.setPosition(pendingServoTarget);
    lastServoTarget = pendingServoTarget;
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
  digitalWrite(LIGHT_PIN,
               snap.isLightOn
                   ? LOW
                   : HIGH); // Zakladajac LIGHT_ACTIVE_HIGH = false w legacy
  digitalWrite(PUMP_PIN, snap.isFilterOn ? HIGH : LOW);
  digitalWrite(HEATER_PIN, snap.isHeaterOn ? HIGH : LOW);

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
  manualServoAngle =
      constrain(angle, SERVO_OPEN_ANGLE, SERVO_CLOSED_ANGLE);
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

  uint64_t wakeMask = (1ULL << static_cast<uint64_t>(BUTTON_DOWN_PIN));
  err = esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_LOW);
  if (err != ESP_OK) {
    logEspErr("esp_sleep_enable_ext1_wakeup", err);
    return false;
  }

  return true;
}

static bool holdOutputsInOffState() {
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, HIGH); // OFF
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW); // OFF
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW); // OFF

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

  const unsigned long CALIBRATION_TIMEOUT_MS = 10UL * 60UL * 1000UL; // 10 min

  LogManager::logInfo("Wymagana kalibracja karmnika.");
  drawFeederCalibrationPrompt(display, "KALIBRACJA KARMNIKA", "SELECT=start",
                              "BACK=anuluj (10m)");

  bool shouldCalibrate = waitForCalibrationDecision(CALIBRATION_TIMEOUT_MS);
  if (!shouldCalibrate) {
    LogManager::logInfo("Kalibracja pominieta.");
    drawFeederCalibrationPrompt(display, "KALIBRACJA POMINIETA");
    delay(1200);
    return;
  }

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

void SystemController::enterNightDeepSleep() {
  if (!rtcReady) {
    LogManager::logWarn("RTC niedostepny - pomijam deep sleep.");
    return;
  }
  DateTime now = getCurrentDateTime();

  LogManager::logInfo("Noc: wylaczam urzadzenia i przechodze do deep sleep.");

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

  uint64_t timerWakeUs = computeSleepUsUntilDayStart(now);
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

  if (!isNightTimeNow()) {
    bool keepScreenOn = cfg.alwaysScreenOn || ((nowMs - lastAction) <= 240000UL);
    PowerManager::setMode(keepScreenOn ? MODE_ACTIVE : MODE_LOW_POWER);
    if (display) {
      display->setPowerSave(keepScreenOn ? 0 : 1);
      if (keepScreenOn) {
        display->setContrast(255);
      }
    }
    return;
  }

  if (!AkwariumWifi::isStaOff()) {
    AkwariumWifi::requestStaOffForDeepSleep();
  }

  if (!canEnterDeepSleep(nowMs, lastAction)) {
    bool keepInteractive = ((nowMs - lastAction) < NIGHT_INTERACTION_WINDOW_MS);
    PowerManager::setMode(keepInteractive ? MODE_ACTIVE : MODE_LOW_POWER);
    if (display) {
      display->setPowerSave(keepInteractive ? 0 : 1);
      if (keepInteractive) {
        display->setContrast(255);
      }
    }
    return;
  }

  if (display) {
    display->clearBuffer();
    display->sendBuffer();
    display->setPowerSave(1);
  }

  PowerManager::setMode(MODE_DEEP_SLEEP);
  enterNightDeepSleep();
}
