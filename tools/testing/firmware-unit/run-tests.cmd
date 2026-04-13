@echo off
setlocal

set ROOT=%~dp0..\..\..
set OUTDIR=%~dp0bin
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

set CXX=%FIRMWARE_UNIT_CXX%
if "%CXX%"=="" (
  where clang++ >nul 2>nul
  if not errorlevel 1 set CXX=clang++
)
if "%CXX%"=="" (
  where g++ >nul 2>nul
  if not errorlevel 1 set CXX=g++
)
if "%CXX%"=="" (
  echo No host C++ compiler found. Set FIRMWARE_UNIT_CXX to clang++ or g++ path.
  exit /b 1
)

"%CXX%" -std=c++17 -Wall -Wextra ^
  -I"%~dp0include" ^
  -I"%ROOT%\firmware\src" ^
  "%~dp0src\arduino_stubs.cpp" ^
  "%~dp0src\firmware_test_doubles.cpp" ^
  "%~dp0tests_main.cpp" ^
  "%ROOT%\firmware\src\ConfigValidation.cpp" ^
  "%ROOT%\firmware\src\ScheduleManager.cpp" ^
  "%ROOT%\firmware\src\TemperatureController.cpp" ^
  "%ROOT%\firmware\src\FeederController.cpp" ^
  -o "%OUTDIR%\firmware-unit-tests.exe"
if errorlevel 1 exit /b 1

"%OUTDIR%\firmware-unit-tests.exe"
