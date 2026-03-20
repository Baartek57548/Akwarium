# Setup

## Zakres

Ten dokument opisuje wymagania środowiskowe oraz uruchamianie głównych części systemu: firmware, emulatorów i aplikacji UI.

## Wymagania środowiskowe

| Obszar | Wymaganie |
| --- | --- |
| Firmware | `Python 3`, `PlatformIO Core`, sterowniki USB dla płytki |
| MAUI | `.NET 10 SDK`, workload `MAUI`, narzędzia Android i/lub Windows |
| Emulator funkcjonalny | `.NET 10 SDK`, Windows |
| Emulator UI OLED | `CMake`, kompilator `MSVC`, `.NET 8 SDK`, Windows |
| BLE | adapter Bluetooth LE przy pracy z urządzeniem fizycznym |

## Przygotowanie repozytorium

### 1. Firmware secrets

Skopiuj plik:

```text
firmware/src/arduino_secrets.template.h
```

do:

```text
firmware/src/arduino_secrets.h
```

Uzupełnij:

- `SECRET_SSID`
- `SECRET_PASS`
- `AP_SSID`
- `AP_PASSWORD`
- `SECRET_BLE_PASSKEY`

## Firmware

Root `platformio.ini` deleguje build do `firmware/platformio.ini`, więc komendy można wykonywać z katalogu repozytorium.

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

## Emulator funkcjonalny

Uruchomienie:

```powershell
dotnet run --project apps/Aquarium.Emulator/Aquarium.Emulator.csproj
```

Po starcie emulator wystawia lokalne API:

```text
http://127.0.0.1:5080/
```

## Aplikacja MAUI

### Windows

```powershell
dotnet run --project mobile-app/AquariumController.Mobile.csproj -f net10.0-windows10.0.19041.0
```

### Android

```powershell
dotnet build mobile-app/AquariumController.Mobile.csproj -f net10.0-android
```

Jeżeli chcesz używać emulatora zamiast fizycznego urządzenia, przełącz tryb połączenia w UI na `Emulator`.

## Emulator UI OLED

### Build biblioteki natywnej

```powershell
cmake -S tools/simulator/FirmwareUI -B tools/simulator/FirmwareUI/build -A x64
cmake --build tools/simulator/FirmwareUI/build --config Release
```

### Uruchomienie hosta WinForms

```powershell
dotnet run --project tools/simulator/Aquarium.ControllerEmulator/Aquarium.ControllerEmulator.csproj
```

## Rozwiązanie .NET

Główne rozwiązanie repozytorium:

```text
AquariumController.slnx
```

Build całej części `.NET`:

```powershell
dotnet build AquariumController.slnx
```

## Najczęstsze problemy

### `PlatformIO` nie znajduje konfiguracji

Uruchamiaj komendy z katalogu repozytorium lub `firmware/`. Root `platformio.ini` jest kompatybilnym shimem do fizycznej konfiguracji firmware.

### `FirmwareUI.dll` nie ładuje się

Sprawdź, czy istnieje:

```text
tools/simulator/FirmwareUI/build/Release/FirmwareUI.dll
```

### MAUI nie buduje targetu Windows

Sprawdź instalację `.NET 10 SDK`, workload `MAUI` i komponentów Windows/Android wymaganych przez środowisko.

## Design Decisions

- Root repo utrzymuje kompatybilny `platformio.ini`, aby nie wymuszać pracy z podkatalogu `firmware/`.
- OTA metadata są weryfikowane po stronie UI jeszcze przed rozpoczęciem transferu.
- Ścieżki uruchomieniowe są rozdzielone według rzeczywistych przypadków użycia: firmware, emulator funkcjonalny, emulator OLED, MAUI.

## Known Limitations

- Konfiguracja środowiska dla MAUI i natywnego emulatora OLED jest Windows-centric.
- Repozytorium nie dostarcza jeszcze jednego, automatycznego bootstrap script dla wszystkich narzędzi.

## Future Improvements

- Skrypt bootstrap dla środowiska developerskiego.
- Zautomatyzowana walidacja wymaganych SDK i workloadów.
- Kontener lub preset developerski dla części .NET i narzędzi pomocniczych.
