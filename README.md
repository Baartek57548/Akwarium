# Aquarium Controller Monorepo

Repozytorium zawiera firmware sterownika ESP32-S3 oraz aplikacjê mobiln¹/desktopow¹ .NET MAUI.

## Struktura

```text
aquarium-controller/
+¦¦ firmware/                  # PlatformIO (ESP32-S3)
-   +¦¦ src/
-   +¦¦ include/
-   +¦¦ lib/
-   +¦¦ boards/
-   +¦¦ platformio.ini
-   L¦¦ README.md
+¦¦ mobile-app/                # .NET MAUI (Android + Windows)
-   +¦¦ Platforms/
-   +¦¦ Resources/
-   +¦¦ App.xaml
-   +¦¦ MainPage.xaml
-   L¦¦ AquariumController.Mobile.csproj
+¦¦ docs/
+¦¦ .github/workflows/
L¦¦ README.md
```

## Firmware (PlatformIO)

Build z root repo:

```bash
python -m platformio run -d firmware -e esp32-s3-devkitc-1
```

Szczegó³y firmware: [firmware/README.md](firmware/README.md)

## Mobile App (.NET MAUI)

Aplikacja jest skonfigurowana dla:
- Android (`net10.0-android`)
- Windows (`net10.0-windows10.0.19041.0`, tylko na Windows)

Build (Windows target):

```bash
dotnet build mobile-app/AquariumController.Mobile.csproj -f net10.0-windows10.0.19041.0
```

Build (Android target):

```bash
dotnet build mobile-app/AquariumController.Mobile.csproj -f net10.0-android
```

## CI

Workflow GitHub Actions buduje firmware z katalogu `firmware/` i publikuje `firmware.bin` jako artifact.
