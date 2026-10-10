@echo off
setlocal
set PORT=%1
if "%PORT%"=="" set PORT=COM9
echo Flashing ESP32-R3X to %PORT%...
python -m esptool --chip esp32s3 --port %PORT% --baud 921600 write_flash 0x0 ESP32-R3X-v2.0-merged.bin
pause
