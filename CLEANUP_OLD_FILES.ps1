$ErrorActionPreference = "Stop"

$legacySourceFiles = @(
    "src\can_monitor.cpp",
    "src\home_assistant_discovery.cpp",
    "src\mqtt_manager.cpp",
    "src\ota_manager.cpp",
    "src\zehnder_decoder.cpp",
    "src\zehnder_poller.cpp"
)

$legacyHeaderFiles = @(
    "include\can_monitor.h",
    "include\home_assistant_discovery.h",
    "include\mqtt_manager.h",
    "include\ota_manager.h",
    "include\zehnder_decoder.h",
    "include\zehnder_poller.h"
)

foreach ($file in ($legacySourceFiles + $legacyHeaderFiles))
{
    if (Test-Path $file)
    {
        Remove-Item $file -Force
        Write-Host "Entfernt: $file"
    }
}

if (Test-Path ".pio")
{
    Remove-Item ".pio" -Recurse -Force
    Write-Host "Build-Cache .pio entfernt"
}

Write-Host ""
Write-Host "Bereinigung abgeschlossen."
Write-Host "Jetzt bauen mit:"
Write-Host "pio run -e atoms3-zehnder"
