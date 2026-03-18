# Aquarium Controller Platform

Profesjonalna przebudowa systemu IoT do sterowania akwarium na ESP32-S3, z warstwami wspolnymi dla aplikacji MAUI, emulatora WinForms i firmware.

Cel refaktoryzacji:
- zachowac dotychczasowa logike dzialania,
- nie zmieniac protokolu JSON bez potrzeby,
- wydzielic wspolne modele i kontrakty,
- dodac emulator bez potrzeby posiadania ESP32,
- przygotowac repozytorium do dalszego skalowania.

## Repozytorium

Aktualny, fizyczny uklad repozytorium:

```text
repo/
  firmware/
  mobile-app/
  apps/
    Aquarium.Emulator/
    Aquarium.Mobile/
    Aquarium.Desktop/
  shared/
    Aquarium.Models/
    Aquarium.Protocol/
    Aquarium.EmulatorCore/
  docs/
  tools/
```

Uwagi:
- `mobile-app/` jest obecnie hostem MAUI dla Windows i Android.
- `Aquarium.Mobile` i `Aquarium.Desktop` sa logicznymi rolami w nowej architekturze, a nie osobnymi rozdzielonymi bazami kodu.
- `apps/Aquarium.Emulator/` to nowy projekt WinForms symulujacy urzadzenie ESP32.
- `AquariumController.slnx` jest glownym rozwiazaniem dla projektow .NET w repozytorium.

## Warstwy

```text
CORE
  logika urzadzenia i stan systemu
CONTROLLERS
  temperature, feeding, lighting, aeration
SERVICES
  wifi, ble, ota, storage
DRIVERS
  ds18b20, rtc, oled, servo, relay
COMMUNICATION
  json protocol, command parser, http api, ble protocol
UI
  OLED firmware UI, MAUI UI, WinForms emulator UI
```

## Glowny przeplyw

```text
MAUI UI / WinForms UI
  -> ViewModel / emulator UI
  -> IDeviceModeService
  -> SelectableBluetoothService
  -> BluetoothService albo EmulatorBluetoothService
  -> Aquarium.Protocol
  -> firmware albo EmulatedDeviceCore
  -> shared models
```

## Wspolne biblioteki

- `shared/Aquarium.Models/` - kanoniczne modele domenowe, np. `DeviceStatus`, `DeviceConfig`, `SystemInfo`.
- `shared/Aquarium.Protocol/` - kontrakty protokolu, UUID BLE, legacy DTO, mapowania i interfejsy transportu.
- `shared/Aquarium.EmulatorCore/` - silnik symulacji urzadzenia uzywany przez emulator i tryb emulatora w MAUI.

## Kontrakt komunikacji

Wspolny format logiczny wiadomosci:

```json
{
  "type": "command | status | event | response",
  "name": "string",
  "payload": {}
}
```

W praktyce projekt zachowuje zgodnosc z obecnym firmware przez warstwe mapujaca:
- `Aquarium.Protocol` zawiera kanoniczny envelope oraz kompatybilne DTO legacy.
- `LegacyModelMappers` przelacza miedzy starymi payloadami i nowymi modelami domenowymi.
- MAUI i emulator korzystaja z tych samych modeli, wiec UI nie zalezy od konkretnego transportu.

## Uruchamianie

### Firmware

```bash
python -m platformio run -e esp32-s3-devkitc-1
python -m platformio run -e esp32-s3-devkitc-1 -t upload
python -m platformio device monitor -b 115200
```

### MAUI Windows

```bash
dotnet build mobile-app/AquariumController.Mobile.csproj -f net10.0-windows10.0.19041.0 -p:TargetFrameworks=net10.0-windows10.0.19041.0
dotnet run --project mobile-app/AquariumController.Mobile.csproj -f net10.0-windows10.0.19041.0
```

### MAUI Android

```bash
dotnet build mobile-app/AquariumController.Mobile.csproj -f net10.0-android
```

### Emulator WinForms

```bash
dotnet run --project apps/Aquarium.Emulator/Aquarium.Emulator.csproj
```

Emulator wystawia lokalny HTTP API na `http://127.0.0.1:5080/` i generuje te same odpowiedzi JSON, ktore rozumie warstwa aplikacji.

## Dokumentacja

- [Architektura systemu](docs/architecture.md)
- [Mapowanie aplikacji](apps/README.md)
- [Weryfikacja techniczna](docs/technical_verification_report.md)
- [Testy manualne](docs/manual_smoke_test.md)

## Stan refaktoryzacji

Zbudowane lokalnie:
- firmware ESP32-S3,
- shared libraries,
- MAUI Windows,
- emulator WinForms.

Komunikacja i logika zostaly rozdzielone warstwowo, ale bez usuwania obecnych funkcji i bez zmiany znaczenia istniejacych payloadow.
