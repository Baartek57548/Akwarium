# Firmware

## Zakres

Ten dokument opisuje aktywne firmware w katalogu `firmware/`: runtime urzadzenia, UI OLED, panel WWW, OTA i power management.

## Struktura katalogu `firmware/`

```text
firmware/
|-- boards/
|-- include/
|-- lib/
|-- scripts/
|-- src/
|-- web/
|-- Interfaces/
`-- platformio.ini
```

Aktywny kod runtime znajduje sie w `firmware/src/`. Zrodla panelu WWW sa w `firmware/web/`, a podczas builda trafiaja do wygenerowanego `WebAssets.h`.

## Runtime model

### Zadania

- `loop()` na `Core 1` uruchamia glowny runtime urzadzenia przez `SystemController::update()` oraz utrzymuje `OtaManager`
- `VideoTask` na `Core 0` obsluguje przyciski, UI OLED, logike menu i timeout sesji `AP`
- `WifiTask` zarzadza `STA/AP`, `WebServer`, captive portal, SSE i synchronizacja czasu

### Centralna orkiestracja

`SystemController` jest glownym orchestreratorem. Odpowiada za:

- inicjalizacje hardware
- odczyt sensorow
- logike harmonogramow
- stany wyjsc
- synchronizacje `SharedState`
- warunki przejscia do low power i light sleep

## Glowne moduly

### `AkwariumV4.ino`

Punkt wejscia firmware. Zawiera:

- setup runtime
- lokalna state machine OLED
- mapowanie przyciskow
- integracje menu `WiFi`, `Test`, `Data i czas`, `Harmonogramy`
- timeout `AP` po `90 s` bez klientow

### `SystemController.*`

Warstwa decyzyjna urzadzenia. Laczy:

- `ConfigManager`
- `ScheduleManager`
- `TemperatureController`
- `FeederController`
- `ServoController`
- `PowerManager`
- `SharedState`

### `AkwariumWifi.*`

Warstwa Wi-Fi i HTTP. Odpowiada za:

- probe `STA` przy starcie
- fallback do `AP`
- reczny start i stop sesji `AP`
- `WebServer`
- captive portal
- `GET /api/status`
- `GET /api/logs`
- `GET /api/events`
- `POST /api/action`
- `POST /update`

### `ApiHandlers.*` i `WebApiProtocol.*`

Warstwa API oraz skladanie odpowiedzi dla panelu WWW. Dba o spojnosc payloadow statusu, logow i wynikow akcji.

### `ConfigManager.*` i `ConfigValidation.*`

Model konfiguracji, walidacja, sanitizacja, migracje oraz zapis do `Preferences`.

### `SharedState.*`

Mutex-protected snapshot runtime wykorzystywany miedzy taskami i warstwami UI/API.

### `OtaManager.*` i `FirmwareInfo.*`

HTTP OTA i metadane builda firmware.

## UI OLED

Lokalna state machine zawiera ekrany:

- `HOME`
- `MENU`
- harmonogramy
- `LOGS`
- `SETTINGS_DATETIME`
- `TESTS`
- `FEEDING`
- `ACCESS_POINT`

Tryb `AP` moze byc uruchomiony z menu. Jezeli nikt nie jest polaczony, sesja zamknie sie automatycznie po `90 s`.

## Logika pracy sieci

- przy starcie firmware probuje polaczyc `STA` przez okolo `6 s`
- przy niepowodzeniu przechodzi do `AP`
- panel WWW dziala zarowno w `STA`, jak i `AP`
- logi i status sa odswiezane przez SSE na `GET /api/events`

## Power management

Wejscie w light sleep wymaga jednoczesnie:

- bezczynnosci powyzej progu nocnego
- braku aktywnego OTA
- braku aktywnego `AP`
- braku service mode i synchronizacji czasu
- wylaczonego `STA`
- braku aktywnego karmienia
- wylaczonych wyjsc swiatla i filtra

Jesli warunki do light sleep nie sa spelnione, firmware moze wygasic sam OLED i pozostac w `MODE_LOW_POWER`.

## Piny

| Funkcja | GPIO |
| --- | --- |
| `BUTTON_UP` | `15` |
| `BUTTON_SELECT` | `16` |
| `BUTTON_DOWN` | `14` |
| `DS18B20` | `1` |
| `FILTER relay` | `2` |
| `FEEDER relay` | `3` |
| `HEATER relay` | `4` |
| `LIGHT relay` | `5` |
| `SERVO` | `6` |
| `BAT_ADC` | `7` |
| `BAT_EN` | `10` |
| `FEEDER_SENSOR` | `12` |

## Ograniczenia

- `AkwariumV4.ino` nadal jest duzym plikiem laczacym UI i czesc integracji runtime
- `heaterMode=Off` dziala bardziej jak bezpiecznik sprzetowy niz klasyczny termostat
- `firmware/Interfaces/` zawiera archiwalne artefakty WWW i nie jest source of truth
