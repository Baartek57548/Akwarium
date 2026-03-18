# FirmwareUI (native DLL)

Native Windows library that runs real UI rendering logic from `firmware/src/AquariumAnimation.cpp` using a fake U8g2 implementation and exposes a small C API for WinForms.

## Exported API

- `int initUI()`
- `void pressButtonUp()`
- `void pressButtonDown()`
- `void pressButtonSelect()`
- `const uint8_t* getFrameBuffer()` (returns `128*32` bytes, layout: `frame[x][y]`)

## Build (CMake)

```powershell
cd tools/simulator/FirmwareUI
cmake -S . -B build -A x64
cmake --build build --config Release
```

Expected output DLL:

- `tools/simulator/FirmwareUI/build/Release/FirmwareUI.dll`
