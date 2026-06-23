#requires -Version 7.0
#requires -PSEdition Core

# Chapter 13.3 for check-ai-integration.ps1 2D Source Pulse watch bridge contracts.

$twoDSourcePulseHeaderText = Get-AgentSurfaceText "engine/tools/include/mirakana/tools/2d_source_pulse.hpp"
$twoDSourcePulseSourceText = Get-AgentSurfaceText "engine/tools/asset/2d_source_pulse.cpp"
$twoDSourcePulseTestsText = Get-AgentSurfaceText "tests/unit/tools_2d_source_pulse_tests.cpp"
$twoDSourcePulseRootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$twoDSourcePulseToolsCMakeText = Get-AgentSurfaceText "engine/tools/asset/CMakeLists.txt"
$twoDSourcePulseManifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$twoDSourcePulseCurrentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$twoDSourcePulsePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-24-original-2d-commercial-authoring-live-iteration-v1.md"

foreach ($needle in @(
        "TwoDSourcePulseStatus",
        "TwoDSourcePulseEventRow",
        "TwoDSourcePulseDesc",
        "TwoDSourcePulsePlan",
        "plan_2d_source_pulse_events"
    )) {
    Assert-ContainsText $twoDSourcePulseHeaderText $needle "2D Source Pulse public header"
}

foreach ($needle in @(
        "FileWatchEventKind::added",
        "FileWatchEventKind::modified",
        "FileWatchEventKind::removed",
        "AssetHotReloadEventKind::added",
        "AssetHotReloadEventKind::modified",
        "AssetHotReloadEventKind::removed",
        "AssetId::from_name",
        "unknown_file_watch_event_kind",
        "native_handle_exposed",
        "autonomous_background_commit_requested",
        "package_script_execution_requested",
        "renderer_rhi_handles_requested",
        "parent_segment_path"
    )) {
    Assert-ContainsText $twoDSourcePulseSourceText $needle "2D Source Pulse source"
}

foreach ($forbiddenNeedle in @(
        "PollingFileWatcher",
        "WindowsFileWatcher",
        "ReadDirectoryChangesW",
        "inotify",
        "FSEvents"
    )) {
    Assert-DoesNotContainText $twoDSourcePulseSourceText $forbiddenNeedle "2D Source Pulse source watcher ownership boundary"
}

foreach ($needle in @(
        "2d source pulse maps native or polling file events to source pulse rows",
        "2d source pulse rejects invalid paths and unknown event kinds",
        "2d source pulse rejects native handle exposure",
        "2d source pulse rejects autonomous background commit",
        "2d source pulse rejects package script and renderer rhi requests",
        "2d source pulse does not construct or own platform watchers"
    )) {
    Assert-ContainsText $twoDSourcePulseTestsText $needle "2D Source Pulse tests"
}

Assert-ContainsText $twoDSourcePulseRootCMakeText "MK_tools_2d_source_pulse_tests" "root CMake 2D Source Pulse test target"
Assert-ContainsText $twoDSourcePulseToolsCMakeText "2d_source_pulse.cpp" "MK_tools asset CMake 2D Source Pulse source"
Assert-ContainsText $twoDSourcePulseManifestText "engine/tools/include/mirakana/tools/2d_source_pulse.hpp" "engine manifest 2D Source Pulse public header"
Assert-ContainsText $twoDSourcePulseManifestText "plan_2d_source_pulse_events" "engine manifest 2D Source Pulse purpose"
Assert-ContainsText $twoDSourcePulseManifestText "watcher ownership stays in MK_platform" "engine manifest 2D Source Pulse watcher ownership"
Assert-ContainsText $twoDSourcePulseCurrentCapabilitiesText "2D Source Pulse Watch Bridge v1" "docs/current-capabilities.md 2D Source Pulse"
Assert-ContainsText $twoDSourcePulseCurrentCapabilitiesText "does not construct or own platform watchers" "docs/current-capabilities.md 2D Source Pulse non-claim"
Assert-ContainsText $twoDSourcePulsePlanText "Phase 2 validation evidence" "Original 2D authoring plan Phase 2 evidence"
