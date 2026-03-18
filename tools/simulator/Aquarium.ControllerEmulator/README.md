# Aquarium.ControllerEmulator (WinForms)

WinForms emulator OLED 128x32 sterowany przez natywna biblioteke `FirmwareUI.dll`.

## Uruchomienie

1. Zbuduj DLL:

```powershell
cd tools/simulator/FirmwareUI
cmake -S . -B build -A x64
cmake --build build --config Release
```

2. Uruchom emulator:

```powershell
dotnet run --project tools/simulator/Aquarium.ControllerEmulator/Aquarium.ControllerEmulator.csproj
```

Jesli jestes juz w katalogu `tools/simulator/FirmwareUI`, uruchom:

```powershell
dotnet run --project ..\Aquarium.ControllerEmulator\Aquarium.ControllerEmulator.csproj
```

## Sterowanie

- `ArrowUp` (lub `Esc`) / przycisk `BACK`
- `Enter` / przycisk `SELECT`
- `ArrowDown` / przycisk `DOWN`

## Reczne parametry

W prawym panelu mozna recznie ustawic:

- `Bateria [%]`
- `Napowietrzanie [%]`
- `Temperatura [C]`
- logi (`Tresc logu`, `Czas [HH:MM]`, `Dodaj log`, `Wyczysc logi`)
- `Uruchom kalibracje` (animacja kalibracji z firmware)

## Troubleshooting

- Blad `cmake is not recognized`:

```powershell
choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y
```

Po instalacji zamknij i otworz nowy terminal.

- Blad kompilacji C++ (`cl.exe not found`, `No CMAKE_CXX_COMPILER could be found`):

```powershell
choco install visualstudio2022buildtools --package-parameters "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --passive --locale en-US" -y
```

- Blad `Podana ścieżka pliku nie istnieje`:
1. Uzyj rozszerzenia `.csproj` (nie `.cspro`).
2. Uruchamiaj z poprawna sciezka wzgledem biezacego katalogu.
