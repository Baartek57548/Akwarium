# Sterownik Akwarium PRO v5.1

Dokumentacja tego pliku jest zaktualizowana pod aktualny kod w folderze `src/`.

## 1. Co robi projekt

Sterownik pracuje na ESP32-S3 (FreeRTOS, dual core) i obsluguje:

- harmonogram swiatla, filtra i napowietrzania
- pomiar temperatury DS18B20 + sterowanie grzalka (histereza + ograniczenie przelaczen)
- karmnik automatyczny i reczny (z czujnikiem i timeoutem)
- pomiar napiecia baterii RTC CR2032
- lokalny panel WWW + OTA przez upload pliku `.bin`
- lokalny interfejs OLED 128x32 + przyciski fizyczne
- logi systemowe (RAM + trwale logi krytyczne w Preferences)
- usypianie ekranu i Deep Sleep

## 2. Architektura (aktualna)

Najwazniejsze moduly:

- `Akwarium.ino` - setup, loop i UI state machine
- `SystemController.*` - glowna logika sterowania i decyzje runtime
- `AkwariumWifi.*` - WiFi STA/AP, HTTP server, captive portal DNS, OTA upload endpoint
- `BleManager.*` - serwer BLE GATT (status, komendy, ustawienia)
- `ApiHandlers.*` - endpointy `/api/status`, `/api/logs`, `/api/action`
- `ConfigManager.*` + `ConfigData.h` - konfiguracja w Preferences (CRC32)
- `ScheduleManager.*` - okna czasowe i auto-feeding
- `PowerManager.*` + `BatteryReader.*` - bateria, aktywnosc, tryby zasilania
- `TemperatureController.*` - DS18B20 i grzalka
- `FeederController.*` - silnik karmnika + logika czujnika
- `ServoController.*` - serwo napowietrzania (attach tylko na czas ruchu)
- `SharedState.*` - bezpieczny snapshot danych miedzy rdzeniami
- `AquariumAnimation.*` + `UIRenderers.*` - OLED/UI

## 3. Piny (z kodu)

```
BUTTON_UP_PIN      GPIO15
BUTTON_SELECT_PIN  GPIO16
BUTTON_DOWN_PIN    GPIO14

ONE_WIRE_BUS       GPIO1   (DS18B20)
HEATER_PIN         GPIO3
PUMP_PIN           GPIO4
FEEDER_PIN         GPIO5
SERVO_PIN          GPIO6

BAT_ADC_PIN        GPIO7
BAT_EN_PIN         GPIO10
FEEDER_SENSOR_PIN  GPIO12
LIGHT_PIN          GPIO17
```

Uwagi:

- `LIGHT_PIN` jest aktywny w logice odwrotnej (`LOW = ON`, `HIGH = OFF`).
- `PUMP_PIN` i `HEATER_PIN` dzialaja klasycznie (`HIGH = ON`).

## 4. Build i upload (PlatformIO)

Aktualne srodowisko w `platformio.ini`:

- `env:esp32-s3-devkitc-1`
- `platform = espressif32`
- `framework = arduino`

Przyklady:

```bash
pio run
pio run -t upload
pio device monitor
```

## 5. Konfiguracja WiFi/AP

W `src/arduino_secrets.h` ustaw:

- `SECRET_SSID`, `SECRET_PASS` - domowa siec WiFi (tryb STA)
- `AP_SSID`, `AP_PASSWORD` - dane Access Point urzadzenia

Start systemu:

- urzadzenie probuje polaczyc sie jako STA przez 6 s
- po timeout pracuje offline (bez automatycznego AP)
- AP uruchamia sie recznie z menu `Wifi`

## 5.1 Bluetooth BLE

- BLE uruchamiany recznie z menu `Bluetooth`
- usluga GATT zawiera:
  - status runtime (`READ + NOTIFY`)
  - komendy sterujace (`WRITE`)
  - ustawienia (`READ + WRITE`)
  - rezultat ACK/ERR (`READ + NOTIFY`)
- zapis/odczyt konfiguracji dziala analogicznie do AP/Web (`ConfigManager::save()`)
- parowanie BLE wymaga szyfrowania i bondingu (PIN: `260225`)

## 6. Web panel i API

HTTP:

- `GET /` - panel WWW
- `GET /style.css`, `GET /script.js` - assety
- `POST /settime?epoch=<unix>` - synchronizacja czasu RTC
- `POST /update` - OTA upload firmware

REST API:

- `GET /api/status` - status urzadzenia (temp, bateria, przekazniki, serwo, harmonogram, siec)
- `GET /api/logs` - logi normalne + krytyczne
- `POST /api/action` - akcje sterujace

Dozwolone akcje `action`:

- `feed_now`
- `set_servo` (wymaga `angle` 0..90)
- `clear_servo`
- `clear_critical_logs`
- `save_schedule` (parametry harmonogramu i temperatury)

## 7. UI i przyciski

Menu glowne (OLED): `Harmonogramy`, `Logi`, `Data i Czas`, `Test`, `Wifi`, `Bluetooth`.

Skrot zachowania:

- HOME: `SELECT` -> MENU
- MENU: `DOWN` nastepna pozycja, `SELECT` wejscie, `UP` powrot
- Ekrany harmonogramow / czasu: `SELECT` edycja/nastepne pole, `DOWN` zmiana wartosci
- LOGI: `DOWN` przewijanie
- Wifi/AP: `UP` reczne wylaczenie AP i powrot do menu
- Bluetooth: `UP` reczne wylaczenie BLE i powrot do menu; auto-exit po pierwszym polaczeniu i rozlaczeniu

## 8. Zasilanie i sleep

- po 4 min bez aktywnosci ekran przechodzi w power-save (jesli `alwaysScreenOn == false`)
- po 5 min bez aktywnosci i przy spelnieniu warunkow system wchodzi w Deep Sleep na 30 min
- wybudzanie: przyciski (`ESP_EXT1_WAKEUP_ANY_LOW`) lub timer RTC

Warunki Deep Sleep (w skrocie):

- grzalka wylaczona
- OTA nie jest w trakcie
- AP wylaczony i brak aktywnego polaczenia STA
- BLE nie jest aktywne (brak advertising/polaczonego klienta)

## 9. Domyslna konfiguracja

Domyslne wartosci z `ConfigManager::loadDefaultConfig()`:

- dzien: `10:00 -> 21:30`
- napowietrzanie: `10:00 -> 21:00`
- filtr: `10:30 -> 20:30`
- temperatura docelowa: `25.0 C`
- histereza temperatury: `0.5 C`
- karmienie: `18:00`, tryb `1` (codziennie)
- `servoPreOffMins = 30`
- `alwaysScreenOn = false`

## 10. Logi i diagnostyka

- logi normalne: ring buffer 20 wpisow (RAM)
- logi krytyczne: ring buffer 20 wpisow (trwale, Preferences)
- czyszczenie logow krytycznych: `POST /api/action` z `action=clear_critical_logs`

## 11. Bezpieczenstwo runtime

- brak poprawnego odczytu DS18B20 (seria bledow) -> fail-safe, grzalka OFF
- twardy limit temperatury: przy `>= 28.0 C` grzalka jest natychmiast odcinana
- manualny override serwa wygasa automatycznie po 5 minutach
- karmnik ma timeout bezpieczenstwa (`15 s`) i logike cyklu czujnika
