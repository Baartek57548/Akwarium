# AKWARIUM-1.2.1 - Pelna dokumentacja techniczna

Dokument opisuje aktualna implementacje firmware w katalogu `src/`.
Wersja dokumentu odpowiada stanowi kodu po ujednoliceniu logiki wyjsc:
`LIGHT`, `PUMP`, `HEATER` sa sterowane tym samym modelem pinowym (`ON => LOW`, `OFF => HIGH`).

## 1. Zakres systemu

Sterownik oparty o ESP32-S3 (Arduino + FreeRTOS) realizuje:

- harmonogram dnia (swiatlo), filtra (pompka), napowietrzania (serwo), karmienia
- pomiar temperatury DS18B20 i sterowanie grzalka z histereza i limitami bezpieczenstwa
- karmnik z kontrola cyklu czujnika i timeoutami
- panel WWW (SPA), REST API, OTA przez HTTP
- BLE GATT (status, komendy, ustawienia)
- OLED 128x32 i lokalna obsluga 3 przyciskami
- logi RAM + logi trwale (NVS)
- diagnostyke resetow i wakeupow (NVS)
- RTC DS3231 (UTC), logika harmonogramow w czasie lokalnym Europe/Warsaw

## 2. Platforma, build i target

## 2.1 Narzedzia i konfiguracja

- PlatformIO
- framework: Arduino (espressif32)
- board: `esp32s3_zero_4mb` (custom board json w `boards/`)
- flash: 4MB
- partitions: `min_spiffs.csv`
- monitor: `115200`, UTF-8
- build flag: `-DARDUINO_USB_CDC_ON_BOOT=1`

## 2.2 Kluczowe biblioteki

- U8g2 (OLED)
- RTClib (DS3231)
- ESP32Servo
- DallasTemperature + OneWire
- ArduinoJson
- ESP32 WebServer + Update
- ESP32 BLE Arduino
- Preferences (NVS)

## 2.3 Komendy

```bash
python -m platformio run
python -m platformio run -t upload
python -m platformio device monitor -b 115200
```

## 3. Architektura runtime

## 3.1 Kolejnosc `setup()`

1. `Serial.begin(115200)`
2. I2C: `Wire.begin(8, 9)`, `Wire.setClock(100000L)`
3. Start OLED i ekran "Wybudzanie..."
4. `SystemController::init()`
5. `animation = new AquariumAnimation(...)`
6. `LogManager::syncOledLogs()`
7. `setupApiEndpoints()`
8. `AkwariumWifi::begin()` (tworzy task WiFi)
9. `BleManager::init()`
10. Start `VideoTask` na core 0

Uwaga: metoda `runFeederCalibrationOnPowerUp()` istnieje, ale nie jest obecnie wywolywana w `setup()`.

## 3.2 Taski i rdzenie

- `loop()` (core 1):
  - `applyPendingUiChanges()`
  - `SystemController::update()`
  - `OtaManager::update()`
  - `BleManager::update()`
  - `vTaskDelay(10ms)`

- `VideoTask` (core 0, ~24 FPS):
  - obsluga przyciskow i state machine UI
  - synchronizacja UI z `SharedState`
  - render ekranow OLED
  - `SystemController::handlePowerManagement(...)`
  - `vTaskDelay(42ms)`

- `WifiTask` (core 1):
  - inicjalizacja STA i serwera HTTP
  - AP/DNS captive portal (na zadanie)
  - `server.handleClient()` + `dnsServer.processNextRequest()`

## 3.3 Synchronizacja danych miedzy taskami

- `SharedState` przechowuje snapshot stanu runtime (temp, bateria, przelaczniki, czas, aeracja).
- Dostep przez mutex (`SemaphoreHandle_t`) i kopie przez wartosc.
- `ConfigManager` ma osobny mutex oraz transakcyjny zapis NVS.
- Zmiany harmonogramu/czasu z UI sa kolejkowane (`PendingScheduleUpdate`, `PendingTimeUpdate`) i aplikowane w `loop()`.

## 4. Sprzet i mapowanie pinow

```text
BUTTON_UP_PIN      GPIO15
BUTTON_SELECT_PIN  GPIO16
BUTTON_DOWN_PIN    GPIO14

ONE_WIRE_BUS       GPIO1    (DS18B20)
HEATER_PIN         GPIO3
PUMP_PIN           GPIO4
FEEDER_PIN         GPIO5
SERVO_PIN          GPIO6

BAT_ADC_PIN        GPIO7
BAT_EN_PIN         GPIO10
FEEDER_SENSOR_PIN  GPIO12
LIGHT_PIN          GPIO17

I2C SDA            GPIO8
I2C SCL            GPIO9
```

## 5. Logika wyjsc wykonawczych

## 5.1 Model pinowy (aktualny)

Wszystkie trzy wyjscia glowne korzystaja z tego samego modelu:

- `ON => LOW`
- `OFF => HIGH`

Dotyczy:

- `LIGHT_PIN`
- `PUMP_PIN`
- `HEATER_PIN`

Stan po inicjalizacji: wszystkie trzy OFF.

## 5.2 Decyzje AUTO

W trybie automatycznym:

- `isLightOn = isDay`
- `isFilterOn = runFilter` (z harmonogramu filtra, ale noc wymusza OFF)
- `isHeaterOn = isDay && tempController.isHeaterOn()`

Nastepnie `applyOutputs()` zapisuje stan na piny.

## 5.3 Tryb TESTS (override)

Gdy UI jest w `UiState::TESTS`, aktywny jest test override:

- relaye sa ustawiane bezposrednio z wartosci testowych `light/heater/filter`
- sterowanie automatyczne jest pomijane
- test override jest kasowany po wyjsciu z ekranu TESTS

## 5.4 OTA a wyjscia

- `OtaManager::beginOtaUpdate()` ustawia `SharedState::updateRelays(false, false, false, false)`
- podczas OTA `SystemController::updateDecisions()` i `applyOutputs()` zwracaja od razu (`return`)

Efekt praktyczny: decyzje i zapisy na piny sa zatrzymane na czas OTA.

## 6. Temperatury i grzalka

## 6.1 Odczyt DS18B20

- odczyt co `2s` (`TEMP_READ_INTERVAL = 2000`)
- filtrowanie probek:
  - odrzucenie `DEVICE_DISCONNECTED_C`
  - odrzucenie stalej `85.0`
  - zakres poprawny `(-50, 50)C`

## 6.2 Warunki sterowania grzalka

Grzalka dziala tylko gdy:

- trwa dzien (`isDay == true`)
- temperatura snapshot nie jest `NaN`
- licznik blednych odczytow `< 3`

W przeciwnym razie `forceHeaterOff()`.

## 6.3 Parametry bezpieczenstwa

- setpoint: `cfg.targetTemp` (clamp w walidacji: `15.0..35.0`)
- histereza: `cfg.tempHysteresis` (clamp: `0.1..5.0`)
- minimalny odstep przelaczen: `120000 ms`
- twardy limit: przy `>= 28.0C` natychmiastowe OFF

## 7. Harmonogramy i automatyka

## 7.1 Semantyka okna czasowego

`ScheduleManager::isWithinWindow(now, start, end)`:

- `start == end` => okno nieaktywne
- `start < end` => zwykle okno dzienne
- `start > end` => okno przechodzace przez polnoc

## 7.2 Dzien/noc

- dzien z `dayStart/dayEnd`
- specjalne wartosci:
  - `dayStartHour == 24` => dzien zawsze ON
  - `dayEndHour == 24` => dzien zawsze OFF

## 7.3 Filtr i napowietrzanie

- `runFilter` i `runAeration` liczone z harmonogramow
- gdy noc, oba sa wymuszane na `false`

## 7.4 Karmienie automatyczne

Warunki startu auto-karmienia:

- `feedMode != 0`
- biezacy czas rowny `feedHour:feedMinute`, sekunda `< 5`
- odstep od `lastFeedEpoch`:
  - mode 1: >= 86000 s (~1 dzien)
  - mode 2: >= 172000 s (~2 dni)
  - mode 3: >= 258000 s (~3 dni)
- dodatkowy bezpiecznik: minimum 3h od ostatniego karmienia

Po sukcesie aktualizowany jest `lastFeedEpoch` i zapisywany config.

## 8. Serwo napowietrzania

## 8.1 Cele pozycji

Domyslne stale:

- `SERVO_OPEN_ANGLE = 0`
- `SERVO_PREOFF_ANGLE = 45`
- `SERVO_CLOSED_ANGLE = 90`

Priorytet logiki celu:

1. noc/idle => closed
2. aktywna aeracja => open
3. okno "preoff" przed koncem filtra => preoff
4. alarm temp (>=30.3 ON, <=29.7 OFF) => `cfg.servoAlarmAngle`
5. manual override (5 min) => zadany kat

## 8.2 Stabilizacja komend

- zmiana celu musi byc stabilna przez `SERVO_TARGET_STABLE_MS = 1200 ms`
- wyjatek: manual override idzie natychmiast
- ruch serwa trwa `MOVE_TIME = 2000 ms` (po detach pin ustawiany na LOW)

## 9. Karmnik

## 9.1 Interfejs sterowania

- `startFeed(durationMs, useSensor)`
- `update()`
- `isFeeding()`
- `setSafetyTimeout()`

## 9.2 Algorytm czujnika (useSensor=true)

Cykl zatrzymania oparty o krawedzie:

- oczekiwanie na `1`
- potem na `0`
- potem powrot na `1`
- dopiero wtedy stop silnika

Bezpieczniki:

- globalny timeout (`safetyTimeout`, domyslnie 15s)
- `EDGE_GUARD_MS = 100`
- `RETURN_GUARD_MS = 20`
- `MIN_CYCLE_MS = 300`

## 9.3 Wywolania karmienia

- REST: `action=feed_now`
- BLE command: `{"action":"feed_now"}`
- UI: przytrzymanie 3 przyciskow przez `1000 ms`
- harmonogram automatyczny (`ScheduleManager`)

## 10. Bateria RTC (CR2032/CR2025)

- ADC 12-bit, attenuation `ADC_11db`
- pomiar asynchroniczny: 30 probek
- trigger pomiaru co 10 minut (`SystemController::updateSensors`)
- mapowanie procentowe:
  - `2.8V => 0%`
  - `3.239V => 100%`
- log krytyczny przy `<=10%`, reset flagi po `>25%`

## 11. Czas i strefa

- RTC DS3231 przechowuje UTC
- strefa: `CET-1CEST,M3.5.0/2,M10.5.0/3` (Europe/Warsaw)
- `getCurrentDateTime()` zwraca czas lokalny do UI/harmonogramow
- `syncSystemTime(epochUtc)` ustawia:
  - system time (`settimeofday`)
  - RTC (`rtc.adjust`)
- endpoint `POST /settime?epoch=<unix_utc>`

## 12. Konfiguracja (NVS, CRC, walidacja)

## 12.1 Struktura i wersjonowanie

- struct `Config` w `ConfigData.h`
- `CONFIG_MAGIC = 0xCAFEBAC4`
- `CONFIG_VERSION = 5`
- CRC32 dla calej struktury (bez pola `crc32`)

## 12.2 Namespace i migracja

- namespace NVS: `"Akwarium"`
- przy braku/poprawce CRC:
  - ladowanie default
  - proba migracji ze struktury legacy (bez CRC)

## 12.3 Domyslne wartosci (loadDefaultConfig)

- dzien: `10:00-21:30`
- aeracja: `10:00-21:00`
- filtr: `10:30-20:30`
- `servoPreOffMins = 30`
- `targetTemp = 25.0`
- `tempHysteresis = 0.5`
- `feedMode = 1`, `feedHour = 18`, `feedMinute = 0`
- `alwaysScreenOn = false`

## 12.4 Zakresy walidacji (ConfigValidation)

- day start/end hour: `0..24`
- minuty: `0..59`
- aeration/filter hour: `0..23`
- `servoPreOffMins`: `0..255`
- `targetTemp`: `15.0..35.0`
- `tempHysteresis`: `0.1..5.0`
- `feedMode`: `0..3`
- `feedHour`: `0..23`
- `feedMinute`: `0..59`

## 13. REST API

## 13.1 HTTP zasoby GUI

- `GET /` - panel WWW
- `GET /style.css`
- `GET /script.js`

## 13.2 Czas

- `POST /settime?epoch=<unix_utc>`

## 13.3 OTA

- `POST /update` (upload firmware)
- po sukcesie restart urzadzenia

## 13.4 API status/logs/action

- `GET /api/status`
  - `temperature`, `battery`, `relays`, `servo`, `schedule`, `feeding`, `network`, `diag`
  - aktualnie `relays` zawiera `light` i `pump`

- `GET /api/logs`
  - `normal` (RAM)
  - `critical` (trwale)

- `POST /api/action`
  - `action=feed_now`
  - `action=set_servo&angle=<0..90>`
  - `action=clear_servo`
  - `action=clear_critical_logs`
  - `action=save_schedule` + pola:
    - `dayStart`, `dayEnd`
    - `airOn`, `airOff`
    - `filterOn`, `filterOff`
    - `targetTemp`, `tempHyst`
    - `feedTime`, `feedFreq`
    - `servoPreOffMins`

## 14. BLE GATT

## 14.1 Parametry

- nazwa: `Akwarium_BLE`
- passkey: `SECRET_BLE_PASSKEY` (6 cyfr)
- szyfrowanie: MITM + bond
- preferowany MTU: 185

## 14.2 UUID

- Service: `4fafc201-1fb5-459e-8bcc-c5c9c331914b`
- Status (read/notify): `beb5483e-36e1-4688-b7f5-ea07361b26a8`
- Command (write): `828917c1-ea55-4d4a-a66e-fd202cea0645`
- Settings (read/write): `d2912856-de63-11ed-b5ea-0242ac120002`
- Result (read/notify): `8e22cb9c-1728-45f9-8c50-2f7252f07379`

## 14.3 Command payload

- `{"action":"feed_now"}`
- `{"action":"set_servo","angle":45}`
- `{"action":"clear_servo"}`
- `{"action":"clear_critical_logs"}`

## 14.4 Settings payload (skrocone klucze)

- `tar`, `hys`
- `fdH`, `fdM`, `fdF`
- `lsH`, `lsM`, `leH`, `leM`
- `asH`, `asM`, `aeH`, `aeM`
- `fsH`, `fsM`, `feH`, `feM`
- `spO`

## 15. WiFi/AP i OTA

## 15.1 STA

- SSID/PASS z `SecretConfig` (przez `arduino_secrets.h`)
- timeout polaczenia: 6s
- po timeout urzadzenie dziala offline, bez autostartu AP

## 15.2 AP

- AP uruchamiany z menu
- DNS captive portal aktywny tylko w AP
- AP stop:
  - recznie (przycisk)
  - automatycznie po odlaczeniu klienta (po pierwszym polaczeniu)

## 15.3 Wylaczanie STA do deep sleep

- `requestStaOffForDeepSleep()` ustawia flage
- `WifiTask` wykonuje `WiFi.mode(WIFI_OFF)`

## 16. Logi i diagnostyka

## 16.1 LogManager

- dzienne logi RAM: do 80 wpisow, rotacja po zmianie dnia
- logi wazne trwale: do 40 wpisow w NVS
- poziomy:
  - `I` info
  - `W` warn (wazne)
  - `E` error (wazne)
- JSON endpoint: `GET /api/logs`

## 16.2 SystemDiagnostics

NVS namespace: `Diag`

Przechowywane pola:

- `bootCount`
- `brownoutCount`
- `wdtCount`
- `panicCount`
- `lastResetReason`
- `lastWakeupCause`

Dane sa publikowane w `GET /api/status.diag`.

## 17. Power management i deep sleep

## 17.1 Aktualny tryb runtime

Deep sleep runtime jest aktualnie wylaczony.
Aktywna jest tylko obsluga wygaszania OLED:

- po 4 minutach bezczynnosci: `MODE_LOW_POWER`, `display->setPowerSave(1)`
- po aktywnosci: `MODE_ACTIVE`, `display->setPowerSave(0)`
- `alwaysScreenOn=true` blokuje wygaszanie

Pierwsze nacisniecie przy wygaszonym ekranie tylko wybudza ekran (bez akcji UI).

## 17.2 Sciezka deep sleep (zaimplementowana, obecnie nieaktywna)

Istnieja metody:

- `canEnterDeepSleep()`
- `enterNightDeepSleep()`

Sciezka obejmuje:

- wymuszenie OFF na wyjsciach
- hold GPIO
- wakeup z GPIO14 i timera do startu dnia

Aktualnie nie jest wywolywana z runtime.

## 18. Kalibracja karmnika

Firmware zawiera dwa tryby:

- `runFeederCalibrationOnPowerUp()`:
  - serwisowy, warunkowany przytrzymaniem SELECT na zimnym starcie
  - nie jest aktualnie wywolywany w `setup()`

- `runFeederCalibrationFromMenu()`:
  - dostepny z menu
  - timeout kalibracji: 2 min
  - po kalibracji przywracany domyslny timeout karmnika 15s

## 19. Pliki i odpowiedzialnosci

- `src/src.ino` - orchestracja setup/loop, UI state machine, task display
- `src/SystemController.*` - runtime decisions, sensory, outputs, power
- `src/ScheduleManager.*` - logika okien czasowych + auto feed
- `src/TemperatureController.*` - DS18B20 + heater
- `src/FeederController.*` - silnik karmnika + czujnik cyklu
- `src/ServoController.*` - sterowanie serwem i detach
- `src/AkwariumWifi.*` - STA/AP, HTTP, OTA, WifiTask
- `src/ApiHandlers.*` - REST API
- `src/BleManager.*` - BLE GATT, security, payloady
- `src/ConfigManager.*` + `src/ConfigValidation.*` - NVS + walidacja
- `src/LogManager.*` - logi RAM/NVS
- `src/SystemDiagnostics.*` - metryki resetow
- `src/PowerManager.*` + `src/BatteryReader.*` - bateria RTC i tryby pracy
- `src/TimeUtils.*` - UTC/local, DST

## 20. Sekrety i konfiguracja lokalna

Plik `src/SecretConfig.h` wymaga definicji:

- `SECRET_SSID`
- `SECRET_PASS`
- `AP_SSID`
- `AP_PASSWORD`
- `SECRET_BLE_PASSKEY`

Praktyka:

1. Utworz `src/arduino_secrets.h` na podstawie `src/arduino_secrets.template.h`.
2. Ustaw realne dane.
3. Nie commituj pliku z sekretami.
