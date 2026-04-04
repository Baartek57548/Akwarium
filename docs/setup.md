# Setup

## Zakres

Ten dokument opisuje przygotowanie srodowiska i uruchamianie firmware.

## Wymagania

| Obszar | Wymaganie |
| --- | --- |
| Firmware | `Python 3`, `PlatformIO Core`, sterownik USB dla plytki |
| Urzadzenie | `ESP32-S3`, OLED, RTC, DS18B20, przelazniki, serwo |
| Siec | dane `STA` i `AP` w `arduino_secrets.h` |

## Przygotowanie secrets

Skopiuj:

```text
firmware/src/arduino_secrets.template.h
```

do:

```text
firmware/src/arduino_secrets.h
```

Uzupelnij:

- `SECRET_SSID`
- `SECRET_PASS`
- `AP_SSID`
- `AP_PASSWORD`

## Build i upload

Root `platformio.ini` deleguje do `firmware/platformio.ini`, wiec komendy mozna uruchamiac z katalogu repozytorium.

Build:

```powershell
python -m platformio run
```

Upload:

```powershell
python -m platformio run -t upload
```

Monitor:

```powershell
python -m platformio device monitor -b 115200
```

Mozesz tez pracowac bezposrednio z katalogu `firmware/`:

```powershell
python -m platformio run -d firmware
```

## Panel WWW

- zrodla panelu znajduja sie w `firmware/web/`
- assety sa pakowane do `WebAssets.h` podczas builda
- po nieudanej probie `STA` firmware przechodzi do `AP`
- sesja `AP` zamyka sie automatycznie po `90 s` bez klientow

## Najczestsze problemy

### `PlatformIO` nie znajduje konfiguracji

Uruchamiaj komendy z katalogu repozytorium albo `firmware/`.

### Firmware nie laczy sie z `STA`

Sprawdz dane w `arduino_secrets.h` i monitor szeregowy. Po okolo `6 s` firmware powinno przejsc do `AP`.

### Panel WWW nie odswieza danych

Sprawdz, czy przegladarka ma polaczenie z `GET /api/events` i czy urzadzenie pozostaje w `STA` albo `AP`.
