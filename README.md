# Sterownik Akwarium PRO v5.1

Kompletny sterownik akwarium dla ESP32-S3 (PlatformIO + Arduino) z lokalnym UI OLED, Web API, OTA oraz BLE.

## 1. Najwazniejsze funkcje

- harmonogram dnia, filtra i napowietrzania
- sterowanie grzalka z histereza i zabezpieczeniem temperatury
- automatyczne oraz reczne karmienie (z czujnikiem i timeoutem)
- panel WWW (`/`) i REST API (`/api/status`, `/api/logs`, `/api/action`)
- OTA przez HTTP upload (`POST /update`)
- BLE GATT z szyfrowaniem, bondowaniem i PIN
- logi runtime (RAM) + logi krytyczne trwale (Preferences)
- zarzadzanie energia: wygaszanie ekranu + deep sleep nocny

## 2. Wymagania

- Python 3.10+
- PlatformIO (CLI lub rozszerzenie VS Code)
- ESP32-S3 Zero 4MB (`board = esp32s3_zero_4mb`)
- czujnik DS18B20, OLED SSD1306 128x32 (I2C), RTC DS3231

## 3. Szybki start

### 3.1 Konfiguracja sekretow

Projekt ma fallback do `src/arduino_secrets.template.h`, ale do normalnej pracy ustaw swoje dane w `src/arduino_secrets.h`.

PowerShell:

```powershell
Copy-Item src/arduino_secrets.template.h src/arduino_secrets.h
```

Nastepnie edytuj `src/arduino_secrets.h`:

- `SECRET_SSID`, `SECRET_PASS`
- `AP_SSID`, `AP_PASSWORD`
- `SECRET_BLE_PASSKEY`

Plik `src/arduino_secrets.h` jest ignorowany przez git.

### 3.2 Build

```bash
python -m platformio run
```

Alternatywnie (jesli `pio` jest w PATH):

```bash
pio run
```

### 3.3 Flash

```bash
python -m platformio run -t upload
```

Jesli plytka nie wchodzi w upload: przytrzymaj `BOOT`, podlacz zasilanie/USB i ponow upload.

### 3.4 Serial monitor

```bash
python -m platformio device monitor -b 115200
```

W `platformio.ini`:

- `monitor_speed = 115200`
- `monitor_encoding = UTF-8`

## 4. Piny

```text
BUTTON_UP_PIN      GPIO15
BUTTON_SELECT_PIN  GPIO16
BUTTON_DOWN_PIN    GPIO14

ONE_WIRE_BUS       GPIO1
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

- `LIGHT_PIN`: logika odwrotna (`LOW = ON`, `HIGH = OFF`)
- `PUMP_PIN` i `HEATER_PIN`: `HIGH = ON`

## 5. Interfejsy sterowania

### 5.1 OLED + przyciski

Menu glowne:

- Harmonogramy
- Logi
- Data i czas
- Test
- Wifi
- Bluetooth

Dodatkowo:

- reczne karmienie: przytrzymaj wszystkie 3 przyciski przez 1 s
- AP i BLE maja auto-exit po sesji klienta + reczne wyjscie `UP`

### 5.2 HTTP API

Podstawowe endpointy:

- `GET /api/status`
- `GET /api/logs`
- `POST /api/action`
- `POST /settime?epoch=<unix>`
- `POST /update` (OTA)

Dostepne akcje `action`:

- `feed_now`
- `set_servo` (wymaga `angle` 0..90)
- `clear_servo`
- `clear_critical_logs`
- `save_schedule`

Przyklady:

```bash
curl http://<IP>/api/status
curl http://<IP>/api/logs
curl -X POST http://<IP>/api/action -d "action=feed_now"
curl -X POST http://<IP>/api/action -d "action=set_servo&angle=35"
curl -X POST http://<IP>/api/action -d "action=clear_servo"
```

### 5.3 BLE

- nazwa urzadzenia: `Akwarium_BLE`
- wymagane parowanie z PIN (`SECRET_BLE_PASSKEY`)
- szyfrowanie: MITM + bonding

Akcje (`command`):

- `{"action":"feed_now"}`
- `{"action":"set_servo","angle":45}`
- `{"action":"clear_servo"}`
- `{"action":"clear_critical_logs"}`

Szczegolowy opis BLE (UUID, payload `settings`) jest w `Info.md`.

## 6. Zasilanie i deep sleep

- po 4 min bezczynnosci: ekran przechodzi w power-save (jesli `alwaysScreenOn=false`)
- nocny deep sleep po 5 min bezczynnosci, tylko gdy:
  - OTA wylaczone
  - AP wylaczony
  - radio STA wylaczone
  - BLE nie reklamuje i brak klienta
  - karmnik nie pracuje
- wakeup:
  - przycisk `GPIO14` (EXT1)
  - timer do startu dnia wg harmonogramu

## 7. Build/CI i release

Workflow: `.github/workflows/build.yml`

- uruchamia build dla PR do `main`
- uruchamia build dla push do `main`
- dla tagow `v*` publikuje `firmware.bin` jako GitHub Release

Przyklad release:

```bash
git switch main
git pull --ff-only
git tag -a v1.2.0 -m "Release v1.2.0"
git push origin v1.2.0
```

## 8. Struktura projektu

Najwazniejsze pliki/moduly:

- `src/AkwariumV4.ino` - setup, loop, UI state machine
- `src/SystemController.*` - glowna logika runtime
- `src/AkwariumWifi.*` - WiFi/AP, HTTP, OTA upload
- `src/ApiHandlers.*` - REST API
- `src/BleManager.*` - BLE GATT
- `src/ConfigManager.*`, `src/ConfigValidation.*` - konfiguracja i walidacja
- `src/ScheduleManager.*` - okna czasowe i auto-feed
- `src/PowerManager.*`, `src/BatteryReader.*` - bateria i aktywnosc
- `src/TemperatureController.*` - DS18B20 + grzalka
- `src/FeederController.*`, `src/ServoController.*` - wykonawcze
- `src/SharedState.*` - snapshot miedzy rdzeniami

## 9. Testy manualne

Checklista smoke test:

- `docs/manual_smoke_test.md`

## 10. Najczestsze problemy

### `pio` / `platformio` command not found

Uzywaj komend przez modul Pythona:

```bash
python -m platformio run
```

### Push do `main` odrzucany

Repo ma reguly ochrony `main` (PR + wymagany check build). Wypychaj zmiany na branch i merguj przez PR.

### Brak logow w monitorze

- sprawdz baud: `115200`
- sprawdz czy monitor jest otwarty na poprawnym porcie
- po OTA odczekaj restart urzadzenia
