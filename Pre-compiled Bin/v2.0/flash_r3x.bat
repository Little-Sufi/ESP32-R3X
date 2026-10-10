@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo     ESP32-R3X v2.0 Firmware Auto-Flasher
echo ===================================================
echo.

:: 1. Check Python Prerequisite
python --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Python is not installed or not in PATH!
    echo Please install Python 3.8+ from https://www.python.org/downloads/
    echo Make sure to check "Add Python to PATH" during installation.
    echo.
    pause
    exit /b 1
)

:: 2. Check esptool Prerequisite & Auto-Install if missing
python -m esptool version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] esptool is missing. Installing prerequisites via pip...
    python -m pip install --upgrade esptool
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to install esptool via pip. Please check your internet connection.
        pause
        exit /b 1
    )
)

:: 3. Port Configuration & Auto-Detection
set PORT=%1
if "%PORT%"=="" (
    for /f "tokens=*" %%P in ('powershell -NoProfile -Command "[System.IO.Ports.SerialPort]::GetPortNames() | Select-Object -First 1"') do set PORT=%%P
)
if "%PORT%"=="" set PORT=COM9

echo Target Port: %PORT%
echo (To specify another port: flash_r3x.bat COMx)
echo.

:: 4. Locate Binaries and Flash
if exist "ESP32-R3X-v2.0-merged.bin" (
    echo [INFO] Flashing complete single-image: ESP32-R3X-v2.0-merged.bin at 0x0...
    python -m esptool --chip esp32s3 --port %PORT% --baud 921600 write_flash 0x0 ESP32-R3X-v2.0-merged.bin
) else if exist "ESP32-R3X.ino.merged.bin" (
    echo [INFO] Flashing complete single-image: ESP32-R3X.ino.merged.bin at 0x0...
    python -m esptool --chip esp32s3 --port %PORT% --baud 921600 write_flash 0x0 ESP32-R3X.ino.merged.bin
) else if exist "ESP32-R3X-v2.0-app.bin" (
    echo [INFO] Flashing segmented binaries (bootloader, partitions, app)...
    python -m esptool --chip esp32s3 --port %PORT% --baud 921600 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 ESP32-R3X-v2.0-app.bin
) else if exist "ESP32-R3X.ino.bin" (
    echo [INFO] Flashing segmented binaries (bootloader, partitions, app)...
    python -m esptool --chip esp32s3 --port %PORT% --baud 921600 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 ESP32-R3X.ino.bin
) else (
    echo [ERROR] Could not find firmware binaries in current directory!
    echo Ensure flash_r3x.bat is located alongside the .bin files.
    pause
    exit /b 1
)

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ===================================================
    echo [SUCCESS] ESP32-R3X v2.0 successfully flashed!
    echo ===================================================
) else (
    echo.
    echo [ERROR] Flashing failed! Check cable connection and COM port.
)

pause
