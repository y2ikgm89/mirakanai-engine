#requires -Version 7.0
#requires -PSEdition Core

# Chapter 7.1 for check-json-contracts.ps1 2D Source Pulse runtime replacement manifest contracts.

$engine = Read-Json "engine/agent/manifest.json"
$sourcePulseRuntimeToolsModule = @($engine.modules | Where-Object { $_.name -eq "MK_tools" })
if ($sourcePulseRuntimeToolsModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_tools module for 2D Source Pulse runtime replacement"
}

if (@($sourcePulseRuntimeToolsModule[0].publicHeaders) -notcontains "engine/tools/include/mirakana/tools/asset_runtime_package_hot_reload_tool.hpp") {
    Write-Error "engine manifest MK_tools publicHeaders must include asset_runtime_package_hot_reload_tool.hpp"
}

$sourcePulseRuntimeToolsPurpose = [string]$sourcePulseRuntimeToolsModule[0].purpose
foreach ($needle in @(
        "execute_2d_source_pulse_runtime_replacement_safe_point",
        "TwoDSourcePulseRuntimeReplacementDesc",
        "TwoDSourcePulseRuntimeReplacementResult",
        "asset-aware Source Pulse rows",
        "runtime scene validation",
        "operator safe point review",
        "source_pulse.safe_point_required=true",
        "recook/runtime replacement diagnostics",
        "editor-core execution",
        "arbitrary shell execution",
        "native handle exposure",
        "active-session replacement without a safe point",
        "does not start editor-core execution",
        "native watchers",
        "broad active-session hot reload readiness"
    )) {
    if (-not $sourcePulseRuntimeToolsPurpose.Contains($needle)) {
        Write-Error "engine manifest MK_tools purpose must describe the 2D Source Pulse runtime replacement and non-claims: $needle"
    }
}
