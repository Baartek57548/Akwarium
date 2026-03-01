# Sterownik Akwarium PRO v5.1 - dokumentacja techniczna

Plik opisuje aktualna architekture kodu i API modulow na podstawie `src/`.

## 1. Przeplyw startu

Kolejnosc startu (`setup()` w `Akwarium.ino`):

1. `Wire.begin()` + I2C 400 kHz.
2. `SystemController::init()`:
   - `SharedState::init()`
   - `ConfigManager::init()` (Preferences + CRC32 + migracja legacy)
   - `LogManager::init()`
   - setup pinow i kontrolerow (temp, feeder, servo, battery, RTC)
   - `OtaManager::init()`
   - `PowerManager::init(...)`
   - `ScheduleManager::init(...)`
3. Inicjalizacja OLED i `AquariumAnimation`.
4. `setupApiEndpoints()` (endpointy REST).
5. `AkwariumWifi::begin()` (osobny task WiFi/WebServer na Core 1).
6. `BleManager::init()`.
7. `SystemController::runFeederCalibrationOnPowerUp(...)`.
8. Start `VideoTask` na Core 0.

## 2. Runtime i rdzenie

Core 1:

- `loop()`:
  - `applyPendingUiChanges()` (zapis zmian UI -> Config/RTC)
  - `SystemController::update()` (sensory, decyzje, wyjscia)
  - `OtaManager::update()`
  - `BleManager::update()`

Core 0:

- `VideoTask`:
  - odczyt przyciskow i state machine UI
  - snapshot danych (`SharedState::getSnapshot()`)
  - render OLED (`AquariumAnimation`)
  - `SystemController::handlePowerManagement(...)`
  - odswiezanie co ~42 ms (~24 FPS)

Task WiFi (rowniez Core 1):

- `AkwariumWifi::begin()` uruchamia `WifiTask`:
  - laczenie STA (timeout 6 s)
  - start HTTP servera
  - obsluga DNS captive portal (gdy AP aktywny)
  - `server.handleClient()` w petli

## 3. SharedState

`SharedStateData` przenosi miedzy taskami:

- temperatura (`temperature`, `minTemp`, `minTempEpoch`, `maxTemp`)
- bateria (`batteryVoltage`, `batteryPercent`)
- stany wykonawcze (`isHeaterOn`, `isFilterOn`, `isLightOn`, `isDay`)
- napowietrzanie (`aerationPercent`)
- czas (`hour`, `minute`, `second`, `day`, `month`, `year`)

Synchronizacja:

- mutex FreeRTOS (`xSemaphoreCreateMutex`)
- zapis przez update*()
- odczyt atomowym snapshotem przez `getSnapshot()`

## 4. ConfigManager i ConfigData

`Config` (w `ConfigData.h`) zawiera harmonogramy, temperature, serwo, karmienie i flagi systemowe.

Wazne stale:

- `CONFIG_MAGIC = 0xCAFEBAC4`
- `CONFIG_VERSION = 5`
- katy serwa: `OPEN=0`, `PREOFF=45`, `CLOSED=90`

Persystencja:

- namespace Preferences: `"Akwarium"`
- klucz: `"sysConfig"`
- CRC32 liczone po calej strukturze bez pola `crc32`
- niezgodnosc CRC/magic/version -> fallback do default
- migracja ze starego `ConfigV1Legacy` (bez CRC)

## 5. SystemController

Publiczne API:

- `init()`
- `update()`
- `feedNow()`
- `setManualServo(int angle)`
- `clearManualServo()`
- `getServoPosition()`
- `runFeederCalibrationOnPowerUp(U8G2 *display)`
- `handlePowerManagement(U8G2 *display, AquariumAnimation *anim)`
- `RTC_DS3231 rtc` (publiczny)

### 5.1 updateSensors()

- temperatura: odczyt co 2 s (`TemperatureController`)
- bateria: start pomiaru co 10 min (`BatteryReader::startMeasurement()`)
- `PowerManager::update()` finalizuje i mapuje procent
- seria >=3 blednych odczytow temp -> log bledu

### 5.2 updateDecisions()

- `ScheduleManager::update(now)` (auto feed)
- wyliczenie:
  - `isDayTime`
  - `isFilterActive`
  - `isAerationActive`
- sterowanie grzalka:
  - target/histereza z config
  - fail-safe OFF przy bledach sensora
- target serwa:
  - domyslnie CLOSED
  - OPEN w oknie napowietrzania
  - PREOFF gdy zbliza sie koniec okna filtra (`servoPreOffMins`)
  - alarmowy kat przy temp > 30 C (`servoAlarmAngle`)
  - reczny override z WebUI (wygasa po 5 min)
- aktualizacja `SharedState`

### 5.3 applyOutputs()

Bezposredni zapis na piny:

- `LIGHT_PIN`: `LOW=ON`, `HIGH=OFF`
- `PUMP_PIN`: `HIGH=ON`
- `HEATER_PIN`: `HIGH=ON`

oraz `servoController.update()` i `feederController.update()`.

### 5.4 Zarzadzanie energia

`handlePowerManagement()`:

- po 4 min bezczynnosci -> OLED power-save (jesli `alwaysScreenOn=false`)
- Deep Sleep po 5 min bezczynnosci i warunkach bezpieczenstwa:
  - grzalka OFF
  - OTA OFF
  - AP OFF
  - brak aktywnego WiFi STA
  - BLE OFF (brak advertising i brak polaczonego klienta)
- usypianie na 30 min
- wakeup:
  - `ESP_EXT1_WAKEUP_ANY_LOW` (przyciski)
  - timer RTC

### 5.5 Kalibracja karmnika

`runFeederCalibrationOnPowerUp()`:

- uruchamia sie tylko po zimnym starcie (nie po wakeup ze sleep)
- prompt na OLED:
  - `SELECT` start
  - `UP` anuluj
  - timeout 10 min -> autostart
- wykonuje pojedynczy cykl `startFeed(1500, true)`

## 6. ScheduleManager

Publiczne API:

- `init(FeederController *feeder)`
- `update(const DateTime &now)`
- `isDayTime(...)`
- `isAerationActive(...)`
- `isFilterActive(...)`
- `getMinutesUntilFilterOff(...)`
- `toMinutes(...)`
- `isWithinWindow(...)`

Reguly:

- obsluga okien przechodzacych przez polnoc
- `dayStartHour == 24` -> dzien zawsze ON
- `dayEndHour == 24` -> dzien zawsze OFF

Auto karmienie:

- aktywne gdy `feedMode != 0`
- okno wyzwolenia: `hour/minute` zgodne i `second < 5`
- odstepy:
  - `feedMode=1` codziennie (~86000 s)
  - `feedMode=2` co 2 dni (~172000 s)
  - `feedMode=3` co 3 dni (~258000 s)
- dodatkowy bezpiecznik: minimum 3 h od ostatniego karmienia

## 7. TemperatureController

Publiczne API:

- `begin()`
- `readTemperature()`
- `controlHeater(float currentTemp)`
- `setTargetTemperature(float)`
- `setHysteresis(float)`
- `isHeaterOn()`
- `resetDailyStats(float)`
- `getDailyMin()`
- `getDailyMax()`

Szczegoly:

- odczyt DS18B20 z cache co 2 s
- odrzucane probki:
  - `DEVICE_DISCONNECTED_C`
  - `85.0 C` (typowy artefakt startowy)
  - wartosci poza zakresem
- histereza + min interwal przelaczen grzalki `120000 ms`
- twarde odciecie grzalki przy `>= 28.0 C`

## 8. FeederController

Publiczne API:

- `begin()`
- `startFeed(durationMs, useSensor)`
- `update()`
- `isFeeding()`
- `setSafetyTimeout(timeoutMs)`
- `getLastError()`
- `clearError()`

`enum class Error`:

- `NONE`
- `SENSOR_NOT_OK` (zdefiniowane w API, aktualnie nieuzyte do walidacji pre-start)
- `TIMEOUT`

Logika cyklu czujnika (gdy `useSensor=true`):

- fazy: `WAIT_FOR_FIRST_ONE -> WAIT_FOR_ZERO -> WAIT_FOR_FINAL_ONE`
- silnik zatrzymuje sie po pelnym cyklu 1->0->1
- timeout bezpieczenstwa domyslnie 15 s

## 9. ServoController

Publiczne API:

- `begin()`
- `setPosition(int position)`
- `update()`
- `isMoving()`
- `getCurrentPosition()`

Szczegoly:

- zakres pozycji obcinany do `0..90`
- attach tylko na czas ruchu
- po `MOVE_TIME = 2000 ms` detach i wymuszenie `LOW` na pinie

## 10. PowerManager i BatteryReader

`BatteryReader`:

- ADC 12-bit, attenuation 11 dB
- asynchroniczny pomiar: 30 probek
- konwersja: `V = raw * 3.3 / 4095 * 1.57769`

`PowerManager`:

- trzyma `lastActivityTime` (reset przez przyciski i akcje API)
- mapuje napiecie CR2032:
  - `2.8 V -> 0%`
  - `3.239 V -> 100%`
- przy `<=10%` loguje krytyczny alarm baterii (z histereza odblokowania >25%)
- publikuje wynik do `SharedState`

## 11. AkwariumWifi i OTA HTTP

Publiczne API:

- `begin()`
- `getIsAPMode()`
- `startAP()`
- `stopAP()`
- `getAPName()`
- `getAPPassword()`
- `getIP()`
- `getConnectedClients()`
- `getServer()`

Tryby:

- STA na starcie (timeout 6 s)
- AP uruchamiany recznie z UI
- DNS captive portal aktywny tylko w AP

HTTP OTA:

- endpoint `POST /update`
- upload obslugiwany przez `Update.h`
- podczas uploadu:
  - `OtaManager::beginOtaUpdate()`
  - po zakonczeniu `OtaManager::endOtaUpdate(success)`
- po sukcesie restart ESP

## 12. BleManager (Bluetooth LE)

Publiczne API:

- `init()`
- `start()`
- `stop()`
- `update()`
- `notifyStatus()`
- `isConnected()`
- `isAdvertising()`
- `getConnectedClients()`
- `getDeviceName()`
- `getPasskey()`

Charakterystyki GATT:

- `status` (`READ + NOTIFY`) - zwięzly status runtime do cyklicznych powiadomien
- `command` (`WRITE`) - akcje sterujace analogiczne do Web API
- `settings` (`READ + WRITE`) - odczyt/zapis konfiguracji analogiczny do AP/Web
- `result` (`READ + NOTIFY`) - ACK/ERR dla komend i zapisu ustawien

Bezpieczenstwo:

- wymagane szyfrowanie i bonding
- parowanie z PIN (`getPasskey()`, domyslnie `260225`)
- write/read na charakterystykach chronione uprawnieniami encrypted

## 13. ApiHandlers - endpointy REST

### 13.1 GET /api/status

Zwraca JSON:

- `temperature`: `current`, `target`, `hysteresis`, `min`, `minTimeEpoch`
- `battery`: `voltage`, `percent`
- `relays`: `light`, `pump`
- `servo`: `angle`
- `schedule`:
  - `dayStartHour`, `dayStartMin`, `dayEndHour`, `dayEndMin`
  - `airStartHour`, `airStartMin`, `airEndHour`, `airEndMin`
  - `filterStartHour`, `filterStartMin`, `filterEndHour`, `filterEndMin`
  - `servoPreOffMins`
- `feeding`: `hour`, `minute`, `freq`, `lastFeedEpoch`
- `network`: `ip`, `apMode`

### 13.2 GET /api/logs

Zwraca:

- `normal`: logi z RAM
- `critical`: logi trwale z Preferences

### 13.3 POST /api/action

Obslugiwane akcje:

- `feed_now`
- `set_servo` (`angle`)
- `clear_servo`
- `clear_critical_logs`
- `save_schedule`

Parametry `save_schedule`:

- `feedTime`, `feedFreq`
- `dayStart`, `dayEnd`
- `targetTemp`, `tempHyst`
- `airOn`, `airOff`
- `filterOn`, `filterOff`
- `servoPreOffMins`

Kazda akcja rejestruje aktywnosc (`PowerManager::registerActivity()`).

## 13. LogManager

Typy logow:

- `logInfo()`, `logWarn()` -> RAM (`webLogs`, max 20)
- `logError()` -> RAM + trwale (`criticalLogs`, max 20, Preferences)

`getLogsAsJson()`:

- serializuje oba ring buffery
- sanitizuje znaki specjalne JSON (`escapeJsonString`)

`clearCriticalLogs()`:

- zeruje licznik i head pointer krytycznych logow

## 14. UI (AquariumAnimation + UIRenderers)

Glowne ekrany:

- HOME
- MENU
- harmonogramy: swiatlo, napowietrzanie, filtr, temperatura, karmienie
- logi
- data i czas
- testy
- ekran Access Point
- ekran Bluetooth
- animacja karmienia

Menu glowne ma 6 pozycji:

- `Harmonogramy`
- `Logi`
- `Data i Czas`
- `Test`
- `Wifi`
- `Bluetooth`

`Akwarium.ino` mapuje zapis zmian UI do:

- `ConfigManager::getConfig()` + `ConfigManager::save()` (harmonogramy)
- `SystemController::rtc.adjust(...)` (data/czas)
