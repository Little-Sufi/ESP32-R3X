<#
.SYNOPSIS
    ESP32-R3X Flash Firmware Utility
.DESCRIPTION
    Compiles and flashes the ESP32-R3X firmware to an ESP32-S3 board.
.AUTHOR
    AMKC
.LICENSE
    MIT License
#>

param (
    [string]$Port = "COM9"
)

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "       ESP32-R3X Flasher Utility (ESP32-S3 ONLY)          " -ForegroundColor Cyan
Write-Host "                     Created by AMKC                      " -ForegroundColor Yellow
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "[*] Target Port: $Port" -ForegroundColor Yellow

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$projectRoot = Split-Path -Parent $scriptDir
$sketchPath = Join-Path $projectRoot "ESP32-R3X\ESP32-R3X.ino"
$libPath = Join-Path $projectRoot "libraries"
$buildPath = Join-Path $projectRoot "build"

# Locate arduino-cli
$cliPaths = @(
    "$env:LOCALAPPDATA\Programs\arduino-ide\resources\app\lib\backend\resources\arduino-cli.exe",
    "arduino-cli.exe"
)

$cliExe = $null
foreach ($path in $cliPaths) {
    if (Test-Path $path) {
        $cliExe = $path
        break
    }
}

if (-not $cliExe) {
    Write-Error "arduino-cli executable not found. Please install Arduino IDE or add arduino-cli to PATH."
    exit 1
}

$fqbn = "esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=8M,PSRAM=opi,PartitionScheme=huge_app"

Write-Host "[*] Compiling firmware for ESP32-S3..." -ForegroundColor Green
& $cliExe compile --fqbn $fqbn --libraries $libPath --output-dir $buildPath $sketchPath

if ($LASTEXITCODE -ne 0) {
    Write-Error "Compilation failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

Write-Host "[*] Uploading firmware to $Port..." -ForegroundColor Green
& $cliExe upload --fqbn $fqbn -p $Port --input-dir $buildPath

if ($LASTEXITCODE -ne 0) {
    Write-Error "Upload failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

Write-Host "`n[SUCCESS] ESP32-R3X successfully flashed to $Port!" -ForegroundColor Green
