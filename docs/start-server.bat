@echo off
cd /d "%~dp0"
echo ======================================================================
echo    ESP32-R3X Web Flasher & Docs Server
echo ======================================================================
echo Opening Web Flasher at: http://localhost:5500/flasher.html
start http://localhost:5500/flasher.html
where python >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Serving via Python at http://localhost:5500 ...
    python -m http.server 5500
) else (
    echo Serving via npx at http://localhost:5500 ...
    npx --yes serve . -l 5500
)
