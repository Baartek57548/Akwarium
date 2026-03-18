# Apps

This folder contains application hosts in the refactored architecture.

- `Aquarium.Emulator/` - WinForms emulator of the ESP32 device.
- `Aquarium.Mobile/` - logical MAUI mobile host; the current source tree still lives in `mobile-app/`.
- `Aquarium.Desktop/` - logical MAUI Windows host; it shares the same codebase as the mobile host.

The shared runtime logic is factored into:

- `shared/Aquarium.Models/`
- `shared/Aquarium.Protocol/`
- `shared/Aquarium.EmulatorCore/`
