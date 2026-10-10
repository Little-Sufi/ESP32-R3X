@echo off
title ESP32-R3X v3.0 Firmware Flasher
cd /d "%~dp0"

echo ===================================================
echo     ESP32-R3X v3.0 Firmware Auto-Flasher
echo ===================================================
echo.

REM --- 1. Check Python ---
where python >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python is not installed or not in PATH!
    echo Please install Python 3.8+ from https://www.python.org/downloads/
    echo Make sure to check "Add Python to PATH" during installation.
    echo.
    goto :done
)
echo [OK] Python found.

REM --- 2. Check esptool ---
python -m esptool version >nul 2>&1
if errorlevel 1 (
    echo [INFO] esptool not found. Installing via pip...
    python -m pip install --upgrade esptool
    if errorlevel 1 (
        echo [ERROR] Failed to install esptool. Check internet connection.
        goto :done
    )
)
echo [OK] esptool ready.
echo.

REM --- 3. Port Detection ---
set "PORT=%~1"
if "%PORT%"=="" (
    echo [INFO] Auto-detecting COM port...
    for /f "tokens=*" %%P in ('python -c "import serial.tools.list_ports; ports=[p.device for p in serial.tools.list_ports.comports()]; print(ports[0] if ports else '')" 2^>nul') do set "PORT=%%P"
)
if "%PORT%"=="" (
    for /f "tokens=*" %%P in ('powershell -NoProfile -Command "[System.IO.Ports.SerialPort]::GetPortNames() | Select-Object -First 1" 2^>nul') do set "PORT=%%P"
)
if "%PORT%"=="" (
    echo [ERROR] No COM port detected! Please connect your ESP32-S3 and try again.
    echo         Or specify manually: flash_r3x.bat COM3
    goto :done
)

echo [INFO] Using port: %PORT%
echo.

REM --- 4. Flash ---
if exist "ESP32-R3X-v3.0-merged.bin" (
    echo [INFO] Flashing merged firmware image at offset 0x0...
    echo.
    python -m esptool --chip esp32s3 --port %PORT% --baud 921600 write-flash 0x0 ESP32-R3X-v3.0-merged.bin
    goto :check_result
)

if exist "ESP32-R3X.ino.merged.bin" (
    echo [INFO] Flashing merged firmware image at offset 0x0...
    echo.
    python -m esptool --chip esp32s3 --port %PORT% --baud 921600 write-flash 0x0 ESP32-R3X.ino.merged.bin
    goto :check_result
)

if exist "ESP32-R3X.ino.bin" (
    if exist "ESP32-R3X.ino.bootloader.bin" (
        echo [INFO] Flashing segmented binaries...
        echo.
        python -m esptool --chip esp32s3 --port %PORT% --baud 921600 write-flash 0x0 ESP32-R3X.ino.bootloader.bin 0x8000 ESP32-R3X.ino.partitions.bin 0xe000 boot_app0.bin 0x10000 ESP32-R3X.ino.bin
        goto :check_result
    )
)

echo [ERROR] No firmware binaries found in this directory!
echo         Place flash_r3x.bat alongside the .bin files.
goto :done

:check_result
echo.
if errorlevel 1 (
    echo ===================================================
    echo [FAILED] Flashing failed!
    echo ===================================================
    echo Check: USB cable, COM port, and boot mode.
    echo Try holding BOOT button while pressing RESET.
) else (
    echo ===================================================
    echo [SUCCESS] ESP32-R3X v3.0 flashed successfully!
    echo ===================================================
    echo You can now press RESET on your board.
)

:done
echo.
echo Press any key to close...
pause >nul
