# Sterownik Akwarium PRO v5.1 - Info techniczne

Ten plik jest "source of truth" dla technicznych detali implementacji na podstawie aktualnego kodu `src/`.

## 1. Setup i kolejnosc startu

`setup()` (`src/AkwariumV4.ino`) wykonuje:

1. `Serial.begin(115200)`
2. I2C: `Wire.begin(8, 9)` + `Wire.setClock(100000L)`
3. szybki init OLED i ekran "Wybudzanie..."
4. `SystemController::init()`
5. `animation = new AquariumAnimation(&display)`
6. `setupApiEndpoints()`
7. `AkwariumWifi::begin()` (task WiFi)
8. `BleManager::init()`
9. `SystemController::runFeederCalibrationOnPowerUp(&display)`
10. start `VideoTask` na Core 0

## 2. Model taskow i rdzeni

- Core 0: `VideoTask`
  - obsluga UI/przyciskow
  - render OLED
  - `SystemController::handlePowerManagement(...)`
  - petla ~24 FPS (`vTaskDelay(42 ms)`)

- Core 1: `loop()`
  - `applyPendingUiChanges()`
  - `SystemController::update()`
  - `OtaManager::update()`
  - `BleManager::update()`
  - `vTaskDelay(10 ms)`

- Core 1: `WifiTask` (z `AkwariumWifi::begin()`)
  - laczenie STA (timeout 6 s)
  - start HTTP server
  - AP DNS captive portal (tylko gdy AP aktywny)
  - `server.handleClient()`

## 3. UI state machine

`UiState`:

- `HOME`, `MENU`
- `SCHEDULE_LIGHT`, `SCHEDULE_AERATION`, `SCHEDULE_FILTER`, `SCHEDULE_TEMP`, `SCHEDULE_FEEDING`
- `LOGS`, `SETTINGS_DATETIME`, `TESTS`
- `FEEDING`, `ACCESS_POINT`, `BLUETOOTH`

Cechy:

- reczne karmienie: wszystkie 3 przyciski przez `MANUAL_FEED_HOLD_MS = 1000`
- AP/BLE:
  - reczne wyjscie `UP`
  - auto-exit po pierwszym kliencie i rozlaczeniu
- zmiany harmonogramu/czasu z UI ida przez kolejke pending i sa zatwierdzane w `loop()`

## 4. Konfiguracja i persystencja

`Config` (`src/ConfigData.h`):

- harmonogram dnia, aeracji, filtra
- `servoPreOffMins`, `targetTemp`, `tempHysteresis`
- `feedMode`, `feedHour`, `feedMinute`, `lastFeedEpoch`
- `alwaysScreenOn`
- metadata: `version`, `magic`, `crc32`

Stale:

- `CONFIG_MAGIC = 0xCAFEBAC4`
- `CONFIG_VERSION = 5`
- `SERVO_OPEN_ANGLE = 0`
- `SERVO_PREOFF_ANGLE = 45`
- `SERVO_CLOSED_ANGLE = 90`

`ConfigManager`:

- namespace Preferences: `Akwarium`
- klucz: `sysConfig`
- CRC32 liczone po strukturze bez pola `crc32`
- fallback do default gdy CRC/magic/version sa niepoprawne
- migracja ze starszej struktury `ConfigV1Legacy`

Default (`loadDefaultConfig()`):

- dzien `10:00-21:30`
- aeracja `10:00-21:00`
- filtr `10:30-20:30`
- `targetTemp=25.0`
- `tempHysteresis=0.5`
- `feedMode=1`, `feedHour=18`, `feedMinute=0`
- `servoPreOffMins=30`
- `alwaysScreenOn=false`

## 5. Walidacja konfiguracji

`ConfigValidation::applyPatchAndClamp(...)`:

- day start/end hour: `0..24`, minute: `0..59`
- aeracja/filter hour: `0..23`, minute: `0..59`
- `servoPreOffMins`: `0..255`
- `targetTemp`: `15.0..35.0` (NaN odrzucane)
- `tempHysteresis`: `0.1..5.0` (NaN odrzucane)
- `feedMode`: `0..3`, `feedHour`: `0..23`, `feedMinute`: `0..59`

## 6. Harmonogram i auto-feeding

`ScheduleManager`:

- okna czasowe obsluguja przejscie przez polnoc
- `dayStartHour == 24` -> dzien zawsze ON
- `dayEndHour == 24` -> dzien zawsze OFF

Auto-feed (`checkAutoFeed`):

- trigger: zgodna godzina/minuta i `second < 5`
- odstepy:
  - `feedMode=1`: >= 86000 s
  - `feedMode=2`: >= 172000 s
  - `feedMode=3`: >= 258000 s
- dodatkowy bezpiecznik: minimum 3 h od ostatniego karmienia
- start: `startFeed(1500, true)`

## 7. HTTP API

### 7.1 `GET /api/status`

JSON zawiera sekcje:

- `temperature`: `current`, `target`, `hysteresis`, `min`, `minTimeEpoch`
- `battery`: `voltage`, `percent`
- `relays`: `light`, `pump`
- `servo`: `angle`
- `schedule`: day/air/filter + `servoPreOffMins`
- `feeding`: `hour`, `minute`, `freq`, `lastFeedEpoch`
- `network`: `ip`, `apMode`

### 7.2 `GET /api/logs`

Zwraca:

- `normal`: logi RAM
- `critical`: logi trwale z Preferences

### 7.3 `POST /api/action`

Akcje:

- `feed_now`
- `set_servo` (`angle`)
- `clear_servo`
- `clear_critical_logs`
- `save_schedule`

`save_schedule` obsluguje m.in. pola:

- `feedTime`, `feedFreq`
- `dayStart`, `dayEnd`
- `targetTemp`, `tempHyst`
- `airOn`, `airOff`
- `filterOn`, `filterOff`
- `servoPreOffMins`

Wyniki:

- `OK`
- `OK_PARTIAL` (czesc pol niepoprawna)
- `No valid fields` / `Save failed` / `Bad Request`

## 8. BLE GATT

### 8.1 Parametry BLE

- nazwa: `Akwarium_BLE`
- preferowane MTU: `185`
- security: encrypted + MITM + bonding + static PIN

### 8.2 UUID

- service: `4fafc201-1fb5-459e-8bcc-c5c9c331914b`
- status: `beb5483e-36e1-4688-b7f5-ea07361b26a8` (`READ + NOTIFY`)
- command: `828917c1-ea55-4d4a-a66e-fd202cea0645` (`WRITE`)
- settings: `d2912856-de63-11ed-b5ea-0242ac120002` (`READ + WRITE`)
- result: `8e22cb9c-1728-45f9-8c50-2f7252f07379` (`READ + NOTIFY`)

### 8.3 Payloady

`command` (`WRITE`):

- `{"action":"feed_now"}`
- `{"action":"set_servo","angle":45}`
- `{"action":"clear_servo"}`
- `{"action":"clear_critical_logs"}`

`result` kody (przyklady):

- `ack`: `connected`, `feed_now`, `set_servo`, `clear_servo`, `clear_logs`, `settings_saved`, `settings_partial`
- `err`: `empty_command`, `bad_json`, `missing_action`, `missing_angle`, `unknown_action`, `empty_settings`, `bad_payload`, `invalid_values`, `save_failed`

`settings` klucze:

- `tar`, `hys`
- `fdH`, `fdM`, `fdF`
- `lsH`, `lsM`, `leH`, `leM`
- `asH`, `asM`, `aeH`, `aeM`
- `fsH`, `fsM`, `feH`, `feM`
- `spO`

`status` (`buildStatusJson`):

- `tmp`, `tar`, `hys`, `mn`, `me`
- `bv`, `bp`
- `l`, `f`, `h`, `srv`
- `ip`, `ap`, `cli`

## 9. Sterowanie runtime

`SystemController::update()`:

1. `updateSensors()`
2. `updateDecisions()`
3. `applyOutputs()`

Wazne zasady:

- OTA (`OtaManager::isOtaInProgress`) wstrzymuje decyzje i output
- grzalka aktywna tylko w dzien i przy poprawnych odczytach temperatury
- alarm temperaturowy: przy `> 30.0 C` servo idzie w `servoAlarmAngle`
- reczny override serwa wygasa po 5 min

Outputy:

- `LIGHT_PIN`: `LOW=ON`, `HIGH=OFF`
- `PUMP_PIN`: `HIGH=ON`
- `HEATER_PIN`: `HIGH=ON`

## 10. Temperatura, karmnik, serwo

`TemperatureController`:

- odczyt co `2000 ms`
- odrzuca `DEVICE_DISCONNECTED_C`, `85.0 C`, wartosci poza zakresem
- min odstep przelaczen grzalki: `120000 ms`
- twarde odciecie grzalki: `>= 28.0 C`

`FeederController`:

- timeout bezpieczenstwa: `15000 ms`
- sekwencja czujnika przy `useSensor=true`: `1 -> 0 -> 1`
- zabezpieczenia faz: `EDGE_GUARD_MS=100`, `RETURN_GUARD_MS=20`, `MIN_CYCLE_MS=300`

`ServoController`:

- zakres `0..90`
- `MOVE_TIME=2000 ms`
- po ruchu `detach()` + wymuszenie `LOW` na pinie

## 11. Bateria i logi

`BatteryReader`:

- ADC: 12-bit + `ADC_11db`
- pomiar async: 30 probek `analogReadMilliVolts`

`PowerManager`:

- mapowanie CR2032: `2.8 V -> 0%`, `3.239 V -> 100%`
- alarm krytyczny <=10% (odblokowanie po >25%)

`LogManager`:

- `logInfo`/`logWarn`: RAM (max 20)
- `logError`: Serial + critical ring w Preferences (max 20)
- `GET /api/logs` zwraca: `normal[]` i `critical[]`

## 12. Zarzadzanie energia i deep sleep

W dzien (`isDayTime == true`):

- po 4 min bezczynnosci: `MODE_LOW_POWER` + `display.setPowerSave(1)`
- aktywnosc przywraca `MODE_ACTIVE`

W nocy (`isDayTime == false`):

- najpierw zadanie wygaszenia STA (`requestStaOffForDeepSleep()`)
- deep sleep tylko gdy:
  - idle >= `300000 ms`
  - OTA OFF
  - AP OFF
  - STA OFF
  - BLE OFF (brak advertising i klienta)
  - karmnik nie pracuje

Wakeup:

- EXT1 na `GPIO14` (`ESP_EXT1_WAKEUP_ANY_LOW`)
- timer do nastepnego `dayStart`

Przed snem:

- wymuszenie OFF dla LIGHT/PUMP/HEATER
- hold pinow przez `gpio_hold_en` + `gpio_deep_sleep_hold_en`

## 13. CI/CD

`.github/workflows/build.yml`:

- build na `pull_request -> main`
- build na `push -> main`
- release artifact (`firmware.bin`) na tagach `v*`

## 14. Smoke test

Manualna checklista:

- `docs/manual_smoke_test.md`
