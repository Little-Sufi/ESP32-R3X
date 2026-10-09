@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo          ESP32-R3X HIGH-SPEED DIRECT FLASHER
echo =======================================================

set PORT=%1
if "%PORT%"=="" set PORT=COM9

set ESPTOOL="C:\Users\assua\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\5.3.1\esptool.exe"
set BUILD_DIR=D:\ESP32-R3X\build

if not exist "%BUILD_DIR%\ESP32-R3X.ino.bin" (
    echo [ERROR] %BUILD_DIR%\ESP32-R3X.ino.bin not found!
    echo Please run fast_build.bat first.
    exit /b 1
)

echo Flashing firmware to %PORT% at 921600 baud...
%ESPTOOL% --chip esp32s3 --port %PORT% --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 "%BUILD_DIR%\ESP32-R3X.ino.bootloader.bin" 0x8000 "%BUILD_DIR%\ESP32-R3X.ino.partitions.bin" 0x10000 "%BUILD_DIR%\ESP32-R3X.ino.bin"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] ESP32-R3X successfully flashed to %PORT%!
) else (
    echo.
    echo [ERROR] Flash failed! Verify port is connected and not locked by Serial Monitor.
)
