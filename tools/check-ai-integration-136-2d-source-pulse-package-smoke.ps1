#requires -Version 7.0
#requires -PSEdition Core

# Chapter 13.6 for check-ai-integration.ps1 2D Source Pulse package smoke contracts.

$sourcePulsePackageMainText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sourcePulsePackageManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sourcePulsePackageValidatorText = Get-AgentSurfaceText "tools/validate-2d-package-playtest-productization.ps1"
$sourcePulseInstalledValidatorText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
$sourcePulseEngineManifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$sourcePulseCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$sourcePulsePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-24-original-2d-commercial-authoring-live-iteration-v1.md"

foreach ($needle in @(
        "SourcePulsePackageSmokeProbeResult",
        "validate_2d_source_pulse_package_evidence",
        "source_pulse_texture_source_document",
        "source_pulse_cooked_texture_document",
        "make_source_pulse_watch_tick_desc",
        "execute_2d_source_pulse_runtime_replacement_safe_point",
        "--require-2d-source-pulse",
        "2d_source_pulse_status",
        "2d_source_pulse_external_engine_code_use"
    )) {
    Assert-ContainsText $sourcePulsePackageMainText $needle "sample 2D Source Pulse package smoke"
}

foreach ($forbiddenNeedle in @(
        "std::system",
        "CreateProcess",
        "ShellExecute",
        "ReadDirectoryChangesW",
        "Unity",
        "Unreal",
        "Godot"
    )) {
    Assert-DoesNotContainText $sourcePulsePackageMainText $forbiddenNeedle "sample 2D Source Pulse package smoke execution and clean-room boundary"
}

foreach ($needle in @(
        "installed-2d-source-pulse-smoke",
        "installed-2d-source-pulse-smoke-playtest",
        "source-pulse-package-smoke",
        "2d_source_pulse_status=ready",
        "2d_source_pulse_event_rows=3",
        "2d_source_pulse_native_backend_rows=1",
        "2d_source_pulse_polling_fallback_rows=1",
        "2d_source_pulse_runtime_replacement_committed_rows=1",
        "2d_source_pulse_external_engine_schema_import=0",
        "2d_source_pulse_external_engine_asset_use=0",
        "2d_source_pulse_external_engine_code_use=0"
    )) {
    Assert-ContainsText $sourcePulsePackageManifestText $needle "sample 2D Source Pulse game manifest"
}

foreach ($needle in @(
        "`$sourcePulseRecipeId = `"installed-2d-source-pulse-smoke`"",
        "`$sourcePulsePlaytestRecipeId = `"installed-2d-source-pulse-smoke-playtest`"",
        "`$sourcePulseSmokeExpectations",
        "--require-2d-source-pulse",
        "2d_source_pulse_runtime_replacement_committed_rows",
        "2d_source_pulse_external_engine_code_use"
    )) {
    Assert-ContainsText $sourcePulsePackageValidatorText $needle "2D package playtest productization validator Source Pulse counters"
}

foreach ($needle in @(
        "`$requires2dSourcePulse",
        "2d_source_pulse_status",
        "2d_source_pulse_native_handle_exposure",
        "2d_source_pulse_external_engine_schema_import",
        "2d_source_pulse_external_engine_asset_use",
        "2d_source_pulse_external_engine_code_use"
    )) {
    Assert-ContainsText $sourcePulseInstalledValidatorText $needle "installed desktop runtime validator Source Pulse counters"
}

foreach ($needle in @(
        "original2dCommercialAuthoringLiveIterationEvidence",
        "current2dSourcePulsePackageSmoke",
        "installed-2d-source-pulse-smoke",
        "--require-2d-source-pulse",
        "Source Pulse is a first-party MIRAIKANAI 2D authoring/live-iteration workflow",
        "does not import Unity, Unreal Engine, or Godot projects",
        "Legal readiness is an engineering evidence gate"
    )) {
    Assert-ContainsText $sourcePulseEngineManifestText $needle "engine manifest Source Pulse package smoke"
}

foreach ($needle in @(
        "2D Source Pulse Package Smoke v1",
        "installed-2d-source-pulse-smoke",
        "2d_source_pulse_status=ready",
        "does not import Unity, Unreal Engine, or Godot projects",
        "legal readiness remains an engineering evidence gate"
    )) {
    Assert-ContainsText $sourcePulseCapabilitiesText $needle "docs/current-capabilities.md Source Pulse package smoke"
}

Assert-ContainsText $sourcePulsePlanText "Phase 5 validation evidence" "Original 2D authoring plan Phase 5 evidence"
Assert-ContainsText $sourcePulsePlanText "Phase 6 validation evidence" "Original 2D authoring plan Phase 6 evidence"
