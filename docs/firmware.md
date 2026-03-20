# Firmware

## Zakres

Ten dokument opisuje firmware urządzenia z katalogu `firmware/`. Obejmuje strukturę projektu, główne moduły, scheduling logic, hardware interface, fail-safe oraz znane ograniczenia implementacji.

## Struktura katalogu `firmware/`

```text
firmware/
|-- boards/                 niestandardowa definicja płytki
|-- include/                nagłówki globalne PlatformIO
|-- lib/                    miejsce na biblioteki lokalne
|-- scripts/                hooki build metadata
|-- src/                    właściwy kod firmware
|-- Interfaces/             archiwalne paczki interfejsów WWW
`-- platformio.ini          konfiguracja builda firmware
```

Najważniejszy kod runtime znajduje się w `firmware/src/`.

## Runtime model

### Zadania i rdzenie

- `loop()` na `Core 1` wykonuje `SystemController::update()`, `OtaManager::update()`, `BleManager::update()` i zapis zmian z UI OLED.
- `VideoTask` na `Core 0` obsługuje lokalny state machine wyświetlacza, przyciski, render OLED i power management.
- `WifiTask` utrzymuje `WebServer`, captive portal i logikę `STA/AP`.

### Centralna orkiestracja

`SystemController` pełni rolę głównego orchestratora. Odpowiada za:

- inicjalizację hardware,
- odczyt sensorów,
- wykonywanie scheduling logic,
- wyznaczanie stanów wyjść,
- synchronizację z `SharedState`,
- zarządzanie energią i warunkami przejścia do light sleep.

## Moduły firmware

### `AkwariumV4.ino`

Punkt wejścia firmware. Zawiera:

- setup runtime,
- lokalny OLED state machine,
- mapowanie przycisków,
- obsługę zmian konfiguracji z ekranu urządzenia,
- integrację menu `WiFi`, `Bluetooth`, `Test`, `Data i czas`, `Harmonogramy`.

### `SystemController.*`

Warstwa decyzyjna urządzenia. Łączy:

- `ConfigManager`,
- `ScheduleManager`,
- `TemperatureController`,
- `FeederController`,
- `ServoController`,
- `PowerManager`,
- `SharedState`.

### `TemperatureController.*`

Odpowiada za:

- odczyt `DS18B20`,
- odrzucanie próbek nieprawidłowych (`DEVICE_DISCONNECTED_C`, `85.0 C`, zakres poza limitem),
- histerezę i minimalny interwał przełączeń,
- sterowanie grzałką jako bezpiecznikiem odcinającym, a nie pełnym termostatem.

### `ScheduleManager.*`

Implementuje scheduling logic dla:

- dnia i oświetlenia,
- filtra,
- napowietrzania,
- auto-karmienia.

Obsługuje okna przechodzące przez północ i separuje logikę czasu od warstwy UI.

### `FeederController.*`

Steruje karmnikiem:

- w trybie czasowym,
- w trybie sensorowym z cyklem `1 -> 0 -> 1`,
- z timeoutem bezpieczeństwa.

### `ServoController.*`

Steruje serwem napowietrzania. Implementacja attach/detach ogranicza niepotrzebne obciążenie sygnału PWM i zasilania.

### `AkwariumWifi.*`

Warstwa Wi-Fi i HTTP:

- start `STA` przy boot,
- ręczny start `AP`,
- `WebServer`,
- captive portal,
- endpoint `POST /update`,
- synchronizacja czasu przez `POST /settime`.

### `ApiHandlers.*`

Warstwa REST API. Udostępnia:

- `GET /api/status`,
- `GET /api/logs`,
- `POST /api/action`.

### `BleManager.*`

Warstwa BLE GATT. Udostępnia:

- status runtime,
- komendy sterujące,
- ustawienia,
- wynik `ACK/ERR`,
- informacje o urządzeniu,
- `BLE OTA`.

### `ConfigManager.*` i `ConfigValidation.*`

Odpowiadają za:

- model konfiguracji,
- walidację i sanitizację,
- migrację legacy,
- zapis do `Preferences`,
- CRC i wersjonowanie struktury.

### `SharedState.*`

Mutex-protected snapshot runtime używany między taskami. To lokalna abstraction layer dla danych odczytywanych przez UI, HTTP i BLE.

### `PowerManager.*` i `BatteryReader.*`

Obsługa bezczynności, poziomu baterii RTC i trybów zasilania. `PowerManager` publikuje telemetrię, ale właściwe wejście w sleep wykonuje `SystemController`.

### `OtaManager.*` i `FirmwareInfo.*`

Warstwa pomocnicza dla OTA oraz build metadata. `FirmwareInfo` osadza w obrazie marker `AQFWMETA`, który jest wykorzystywany przez aplikację MAUI przy walidacji pakietu `.bin`.

## Logika działania

### Start urządzenia

1. Inicjalizacja `SharedState`, konfiguracji, logów i hardware.
2. Uruchomienie kontrolerów temperatury, karmnika, serwa i baterii.
3. Próba przywrócenia poprawnego czasu z RTC lub backupu `NVS`.
4. Start `WiFi`, REST API, BLE i UI OLED.

### Cykl runtime

1. Odczyt temperatury i baterii.
2. Obliczenie aktywnych okien pracy z harmonogramów.
3. Wyznaczenie stanów światła, filtra, napowietrzania i grzałki.
4. Aktualizacja `SharedState`.
5. Zapis na fizyczne piny i aktualizacja actuatorów.

### Lokalna state machine

OLED UI działa jako osobna state machine z ekranami:

- `HOME`,
- `MENU`,
- harmonogramy,
- logi,
- `ACCESS_POINT`,
- `BLUETOOTH`,
- `TESTS`,
- `FEEDING`.

To nie jest główny model domenowy, ale interfejs lokalny urządzenia.

## Fail-safe i edge case'y

### Temperatura

- trzy nieprawidłowe próbki z rzędu oznaczają błąd sensora,
- próbka `85.0 C` jest traktowana jako artefakt startowy,
- odcięcie grzałki następuje po przekroczeniu `target + hysteresis`,
- ponowne podłączenie następuje po zejściu do `target`.

### Harmonogram filtra

Jeżeli `filter start == filter end`, firmware traktuje to jako błędne okno i stosuje fallback do rytmu dnia.

### RTC

- przy `lostPower()` najpierw zachowywany jest czas z układu, jeśli wygląda wiarygodnie,
- w przeciwnym razie firmware próbuje odzyskać epoch z backupu `NVS`,
- brak wiarygodnego czasu kończy się fallbackiem do wartości domyślnej.

### OTA

- podczas OTA firmware przechodzi w stan ograniczony,
- wyjścia są wymuszane do bezpiecznych wartości,
- normalny runtime jest wstrzymany do końca transferu.

### Power management

Wejście w light sleep wymaga spełnienia wszystkich warunków:

- brak aktywnego OTA,
- brak aktywnego `AP`,
- wyłączone radio `STA`,
- brak advertisingu i połączenia BLE,
- brak aktywnego karmienia,
- bezczynność powyżej progu nocnego.

## Piny i hardware interface

Aktualne mapowanie z kodu:

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

## Design Decisions

- `SystemController` jest pojedynczym punktem koordynacji, aby ograniczyć rozproszenie logiki krytycznej.
- `SharedState` stosuje model snapshot zamiast bezpośredniego współdzielenia obiektów między taskami.
- `ConfigValidation` jest jedyną akceptowaną ścieżką walidacji runtime patchy z UI, HTTP i BLE.
- OTA blokuje sterowanie runtime, ponieważ bezpieczeństwo spójności flash ma wyższy priorytet niż ciągłość działania wyjść.

## Known Limitations

- `AkwariumV4.ino` pozostaje dużym plikiem łączącym state machine, obsługę wejścia i część integracji runtime.
- `heaterMode=Off` nie działa jak klasyczny tryb wyłączenia grzałki; obecna implementacja zachowuje logikę bezpiecznika sprzętowego.
- `SharedState::minTempEpoch` nie jest aktualnie wypełniany pełnym timestampem minimalnej temperatury.
- Interfejsy WWW w `firmware/Interfaces/` są archiwalne i nie stanowią aktywnego source of truth.

## Future Improvements

- Rozbicie `AkwariumV4.ino` na dedykowane moduły UI i input controller.
- Dodanie testów jednostkowych dla `ConfigValidation` i `ScheduleManager`.
- Formalizacja hardware abstraction layer dla wyjść przekaźnikowych i sensorów.
- Ujednolicenie semantyki grzałki pomiędzy firmware, UI i dokumentacją domenową.
