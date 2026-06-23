#requires -Version 7.0
#requires -PSEdition Core

# Chapter 13.4 for check-ai-integration.ps1 2D Source Pulse runtime replacement contracts.

$sourcePulseRuntimeHeaderText = Get-AgentSurfaceText "engine/tools/include/mirakana/tools/asset_runtime_package_hot_reload_tool.hpp"
$sourcePulseRuntimeSourceText = Get-AgentSurfaceText "engine/tools/asset/asset_runtime_package_hot_reload_tool.cpp"
$sourcePulseRuntimeTestsText = Get-AgentSurfaceText "tests/unit/tools_runtime_hot_reload_package_tests.cpp"
$sourcePulseRuntimeManifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$sourcePulseRuntimeCurrentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$sourcePulseRuntimePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-24-original-2d-commercial-authoring-live-iteration-v1.md"

foreach ($needle in @(
        "TwoDSourcePulseRuntimeReplacementDesc",
        "TwoDSourcePulseRuntimeReplacementResult",
        "execute_2d_source_pulse_runtime_replacement_safe_point",
        "runtime_scene_validation_succeeded",
        "operator_reviewed_safe_point",
        "request_editor_core_execution",
        "request_arbitrary_shell_execution",
        "request_active_session_without_safe_point"
    )) {
    Assert-ContainsText $sourcePulseRuntimeHeaderText $needle "2D Source Pulse runtime replacement public header"
}

foreach ($needle in @(
        "source_pulse.safe_point_required",
        "runtime_scene_validation_required",
        "operator_reviewed_safe_point_required",
        "source_pulse_safe_point_required",
        "editor_core_execution_requested",
        "arbitrary_shell_execution_requested",
        "active_session_without_safe_point_requested",
        "source_pulse_events_from_rows",
        "tick_state.scheduler.enqueue",
        "execute_asset_runtime_package_hot_reload_registered_asset_watch_tick_safe_point",
        "source_pulse_status_for_watch_tick"
    )) {
    Assert-ContainsText $sourcePulseRuntimeSourceText $needle "2D Source Pulse runtime replacement source"
}

foreach ($forbiddenNeedle in @(
        "std::system",
        "CreateProcess",
        "ShellExecute",
        "WindowsFileWatcher",
        "PollingFileWatcher",
        "ReadDirectoryChangesW"
    )) {
    Assert-DoesNotContainText $sourcePulseRuntimeSourceText $forbiddenNeedle "2D Source Pulse runtime replacement execution boundary"
}

foreach ($needle in @(
        "2d source pulse runtime replacement commits after scene validation and operator review",
        "2d source pulse runtime replacement blocks without runtime scene validation",
        "2d source pulse runtime replacement blocks without operator reviewed safe point",
        "2d source pulse runtime replacement blocks active session without safe point",
        "2d source pulse runtime replacement rejects editor core and arbitrary shell execution",
        "2d source pulse runtime replacement preserves recook and runtime replacement failure diagnostics"
    )) {
    Assert-ContainsText $sourcePulseRuntimeTestsText $needle "2D Source Pulse runtime replacement tests"
}

Assert-ContainsText $sourcePulseRuntimeManifestText "execute_2d_source_pulse_runtime_replacement_safe_point" "engine manifest 2D Source Pulse runtime replacement purpose"
Assert-ContainsText $sourcePulseRuntimeManifestText "runtime scene validation, operator safe point review" "engine manifest 2D Source Pulse runtime replacement gates"
Assert-ContainsText $sourcePulseRuntimeManifestText "active-session replacement without a safe point" "engine manifest 2D Source Pulse runtime replacement active-session boundary"
Assert-ContainsText $sourcePulseRuntimeCurrentCapabilitiesText "2D Source Pulse Runtime Replacement v1" "docs/current-capabilities.md 2D Source Pulse runtime replacement"
Assert-ContainsText $sourcePulseRuntimeCurrentCapabilitiesText "does not construct platform watchers" "docs/current-capabilities.md 2D Source Pulse runtime replacement non-claim"
Assert-ContainsText $sourcePulseRuntimePlanText "Phase 3 validation evidence" "Original 2D authoring plan Phase 3 evidence"
