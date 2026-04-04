# Aquarium Controller

Sterownik akwarium oparty o `ESP32-S3`. Repozytorium zostalo uproszczone do jednego celu: utrzymania firmware, lokalnego UI OLED i panelu WWW serwowanego bezposrednio z urzadzenia.

## Zakres repo

- firmware embedded w `firmware/`
- panel WWW i REST API/SSE dostarczane przez firmware
- dokumentacja uruchomienia, testow i architektury firmware
- pomocnicze materialy testowe w `tools/testing/`

Usuniete z repo:

- BLE
- aplikacje mobilne i desktopowe
- emulatory i symulatory
- projekty `.NET` i shared contracts

## Najwazniejsze funkcje

- harmonogramy dla oswietlenia, filtra, napowietrzania i karmienia
- pomiar temperatury `DS18B20` i fail-safe dla grzalki
- lokalne UI na `OLED 128x32`
- panel WWW z podgladem statusu, logow i akcjami sterujacymi
- `HTTP OTA`
- tryb `STA/AP` z fallbackiem do `AP`
- automatyczne zamkniecie sesji `AP` po `90 s` bez klientow

## Uklad repo

```text
.
|-- firmware/
|   |-- src/
|   |-- web/
|   `-- platformio.ini
|-- docs/
|-- tools/
|   `-- testing/
|-- platformio.ini
`-- README.md
```

## Quick Start

1. Skopiuj `firmware/src/arduino_secrets.template.h` do `firmware/src/arduino_secrets.h`.
2. Uzupelnij `SECRET_SSID`, `SECRET_PASS`, `AP_SSID` i `AP_PASSWORD`.
3. Zbuduj firmware:

```powershell
python -m platformio run
```

4. Wgraj firmware:

```powershell
python -m platformio run -t upload
```

5. Otworz monitor portu szeregowego:

```powershell
python -m platformio device monitor -b 115200
```

## Dokumentacja

- [Setup](docs/setup.md)
- [Firmware](docs/firmware.md)
- [Testowanie](docs/testing.md)
- [Manual smoke test](docs/manual_smoke_test.md)
