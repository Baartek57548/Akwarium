# Aquarium Controller

Aquarium Controller is a monorepo for an ESP32-S3 based aquarium controller.
It contains the device firmware, a .NET MAUI native application for Windows and Android, and the embedded web dashboard served by the controller.

## Repository Layout

```text
aquarium-controller/
|-- firmware/                 # PlatformIO + Arduino firmware for ESP32-S3
|   |-- src/
|   |-- include/
|   |-- lib/
|   |-- boards/
|   |-- platformio.ini
|   `-- README.md
|-- mobile-app/               # .NET MAUI app (Windows + Android)
|   |-- Platforms/
|   |-- Resources/
|   |-- Services/
|   |-- ViewModels/
|   |-- MainPage.xaml
|   `-- AquariumController.Mobile.csproj
|-- docs/                     # Supporting project documentation
|-- testy/                    # UI experiments and local prototypes
|-- .github/workflows/        # CI for firmware build and release
|-- platformio.ini            # Root PlatformIO compatibility entrypoint
`-- README.md
```

## Main Components

- `firmware/`
  ESP32-S3 firmware built with PlatformIO and the Arduino framework.
  It exposes BLE control, an embedded web panel, HTTP OTA, telemetry, logs, and scheduling.

- `mobile-app/`
  Native .NET MAUI application targeting:
  - `net10.0-windows10.0.19041.0`
  - `net10.0-android`

  The app uses `Plugin.BLE` and `CommunityToolkit.Mvvm`, and supports BLE control plus BLE OTA from the `SYSTEM / OTA` tab.

- Embedded web panel
  Served directly by the controller firmware. It shows device status, logs, schedules, and current firmware information. OTA in the browser stays on HTTP.

## Features

- BLE communication with the aquarium controller
- BLE OTA in the native Windows and Android app
- HTTP OTA in the embedded web panel
- Current firmware version, build metadata, and partition info exposed in app and web UI
- Device scheduling for lighting, filtration, aeration, temperature, and feeding
- Runtime logs and controller status overview

## Requirements

### General

- Git
- Windows PowerShell
- Python 3.x
- PlatformIO

### Native App

- .NET 10 SDK
- .NET MAUI workload
- Windows 10/11 for the desktop target
- Android SDK / emulator or physical Android device for the Android target

Install the MAUI workload if needed:

```powershell
dotnet workload install maui
```

## First-Time Setup

### 1. Configure firmware secrets

Create a local file:

```text
firmware/src/arduino_secrets.h
```

You can copy the template from:

```text
firmware/src/arduino_secrets.template.h
```

The file should define at least:

- `SECRET_SSID`
- `SECRET_PASS`
- `AP_SSID`
- `AP_PASSWORD`
- `SECRET_BLE_PASSKEY`

Do not commit real credentials to GitHub.

### 2. Restore and build the MAUI app

From the repository root:

```powershell
dotnet build mobile-app\AquariumController.Mobile.csproj -f net10.0-windows10.0.19041.0
dotnet build mobile-app\AquariumController.Mobile.csproj -f net10.0-android
```

### 3. Build the firmware

From the repository root:

```powershell
python -m platformio run -e esp32-s3-devkitc-1
```

## How To Run the Windows App

From the repository root:

```powershell
dotnet run --project mobile-app\AquariumController.Mobile.csproj -f net10.0-windows10.0.19041.0
```

Or from inside `mobile-app`:

```powershell
cd mobile-app
dotnet run -f net10.0-windows10.0.19041.0
```

If the app was already running and the build is blocked by a locked executable, close the window first or kill the process:

```powershell
taskkill /IM AquariumController.Mobile.exe /F
```

If you want to build first and run the generated executable manually:

```powershell
dotnet build mobile-app\AquariumController.Mobile.csproj -f net10.0-windows10.0.19041.0
.\mobile-app\bin\Debug\net10.0-windows10.0.19041.0\win-x64\AquariumController.Mobile.exe
```

## Android Build and Run

Build:

```powershell
dotnet build mobile-app\AquariumController.Mobile.csproj -f net10.0-android
```

Run on a connected emulator or device:

```powershell
dotnet build mobile-app\AquariumController.Mobile.csproj -f net10.0-android -t:Run
```

To verify that Android is connected:

```powershell
adb devices
```

## Firmware Build, Upload, and Monitor

Build:

```powershell
python -m platformio run -e esp32-s3-devkitc-1
```

Upload:

```powershell
python -m platformio run -e esp32-s3-devkitc-1 -t upload
```

Serial monitor:

```powershell
python -m platformio device monitor -b 115200
```

## OTA Behavior

- Native app: BLE OTA is available in the `SYSTEM / OTA` tab on Windows and Android.
- Web panel: HTTP OTA is available through `/update`.
- Firmware version, build date, build time, ESP-IDF version, and OTA partition information are shown in both the native app and the web UI.

## CI

GitHub Actions workflow:

- builds the firmware from `firmware/`
- uploads `firmware.bin` as an artifact
- creates a GitHub Release attachment when a `v*` tag is pushed

See:

- `.github/workflows/build.yml`

## Additional Documentation

- [Firmware README](firmware/README.md)
- [Smoke Test Notes](docs/manual_smoke_test.md)
