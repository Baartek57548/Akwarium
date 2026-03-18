# Simulator Tools

Folder zawiera emulator UI oparty o realny kod firmware:

- `tools/simulator/FirmwareUI/` - natywna biblioteka C++ (`FirmwareUI.dll`) z emulacja U8g2 (`FakeU8g2`) i wykonaniem logiki rysowania z `firmware/src/AquariumAnimation.cpp`.
- `tools/simulator/Aquarium.ControllerEmulator/` - aplikacja WinForms, ktora laduje `FirmwareUI.dll`, pobiera framebuffer OLED 128x32 i renderuje go pixel-perfect.

## Build native DLL (Windows)

```powershell
cd tools/simulator/FirmwareUI
cmake -S . -B build -A x64
cmake --build build --config Release
```

## Run WinForms emulator

```powershell
dotnet run --project tools/simulator/Aquarium.ControllerEmulator/Aquarium.ControllerEmulator.csproj
```

Klawiatura:

- `ArrowUp` -> `UP`
- `Enter` -> `SELECT`
- `ArrowDown` -> `DOWN`
