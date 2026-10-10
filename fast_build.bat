@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo          ESP32-R3X ULTRA-FAST COMPILER (CCACHE + 8-CORE)
echo =======================================================

set ARDUINO_CLI="C:\Users\assua\AppData\Local\Programs\arduino-ide\resources\app\lib\backend\resources\arduino-cli.exe"
set SKETCH_DIR="D:\ESP32-R3X\ESP32-R3X"
set LIBS_DIR="D:\ESP32-R3X\Libraries"
set BUILD_DIR="D:\ESP32-R3X\build"
set FQBN=esp32:esp32:esp32s3:CDCOnBoot=default,FlashSize=4M,PartitionScheme=huge_app

if not exist %BUILD_DIR% mkdir %BUILD_DIR%

set START_TIME=%TIME%
echo [1/2] Compiling with multi-core jobs (-j 8) and persistent cache...

%ARDUINO_CLI% compile --fqbn %FQBN% --libraries %LIBS_DIR% --build-path %BUILD_DIR% -j 8 --export-binaries %SKETCH_DIR%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Compilation failed!
    exit /b %ERRORLEVEL%
)

echo.
echo [2/2] Compilation Succeeded!
echo Start Time : %START_TIME%
echo Finish Time: %TIME%
echo Output Binary: %BUILD_DIR%\ESP32-R3X.ino.bin
echo.
echo =======================================================
echo Cache Statistics:
"C:\Users\assua\AppData\Local\Arduino15\ccache\ccache.exe" -s
echo =======================================================
echo.
echo Press any key to close...
pause >nul
