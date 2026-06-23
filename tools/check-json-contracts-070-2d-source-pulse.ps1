#requires -Version 7.0
#requires -PSEdition Core

# Chapter 7.0 for check-json-contracts.ps1 2D Source Pulse watch bridge static contracts.

$engine = Read-Json "engine/agent/manifest.json"
$sourcePulseToolsModule = @($engine.modules | Where-Object { $_.name -eq "MK_tools" })
if ($sourcePulseToolsModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_tools module for 2D Source Pulse"
}

if (@($sourcePulseToolsModule[0].publicHeaders) -notcontains "engine/tools/include/mirakana/tools/2d_source_pulse.hpp") {
    Write-Error "engine manifest MK_tools publicHeaders must include 2d_source_pulse.hpp"
}

$sourcePulseToolsPurpose = [string]$sourcePulseToolsModule[0].purpose
foreach ($needle in @(
        "plan_2d_source_pulse_events",
        "TwoDSourcePulseDesc",
        "TwoDSourcePulseEventRow",
        "TwoDSourcePulsePlan",
        "FileWatchEventKind added/modified/removed",
        "AssetHotReloadEventKind",
        "path-derived AssetId",
        "watcher ownership stays in MK_platform",
        "does not construct platform watchers",
        "active-session hot reload readiness"
    )) {
    if (-not $sourcePulseToolsPurpose.Contains($needle)) {
        Write-Error "engine manifest MK_tools purpose must describe the 2D Source Pulse watch bridge and non-claims: $needle"
    }
}
