<#
.SYNOPSIS
    ESP32-R3X Automatic Library Installer for Arduino IDE
.DESCRIPTION
    Installs all required dependencies (PCF8574, TFT_eSPI, CC1101, RF24, etc.)
    into your Arduino sketchbook libraries directory to ensure error-free compilation.
.AUTHOR
    AMKC
.LICENSE
    MIT License
#>

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "    ESP32-R3X Library Installer (Firmware for ESP32-S3)    " -ForegroundColor Cyan
Write-Host "                     Created by AMKC                      " -ForegroundColor Yellow
Write-Host "==========================================================" -ForegroundColor Cyan

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$projectRoot = Split-Path -Parent $scriptDir
$sourceLibs = Join-Path $projectRoot "libraries"

if (-not (Test-Path $sourceLibs)) {
    Write-Error "Libraries folder not found at: $sourceLibs"
    exit 1
}

$destTargets = @(
    "$HOME\Documents\Arduino\libraries",
    "$HOME\OneDrive\Documents\Arduino\libraries",
    "$env:LOCALAPPDATA\Arduino15\libraries"
)

$installedCount = 0

foreach ($target in $destTargets) {
    try {
        if (-not (Test-Path $target)) {
            New-Item -ItemType Directory -Force -Path $target | Out-Null
        }
        Write-Host "[*] Copying bundled libraries to: $target" -ForegroundColor Green
        Copy-Item -Path "$sourceLibs\*" -Destination $target -Recurse -Force
        $installedCount++
    }
    catch {
        Write-Warning "Could not copy to $target ($($_.Exception.Message)) - Continuing..."
    }
}

if ($installedCount -gt 0) {
    Write-Host "`n[SUCCESS] All ESP32-R3X libraries successfully installed!" -ForegroundColor Green
    Write-Host "[NOTE] PCF8574, TFT_eSPI, SmartRC-CC1101, and other dependencies are ready." -ForegroundColor Cyan
    Write-Host "[NOTE] You can now compile ESP32-R3X.ino in Arduino IDE without any missing library errors.`n" -ForegroundColor Yellow
} else {
    Write-Error "Failed to install libraries to any target directory."
}
