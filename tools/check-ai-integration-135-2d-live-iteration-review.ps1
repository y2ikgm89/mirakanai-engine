#requires -Version 7.0
#requires -PSEdition Core

# Chapter 13.5 for check-ai-integration.ps1 2D live iteration editor review contracts.

$editor2DLiveHeaderText = Get-AgentSurfaceText "editor/core/include/mirakana/editor/playtest_package_review.hpp"
$editor2DLiveSourceText = Get-AgentSurfaceText "editor/core/src/playtest_package_review.cpp"
$editor2DLiveTestsText = Get-AgentSurfaceText "tests/unit/editor_core_tests.cpp"
$editor2DLiveManifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$editor2DLiveCurrentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$editor2DLivePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-24-original-2d-commercial-authoring-live-iteration-v1.md"

foreach ($needle in @(
        "Editor2DLiveIterationStageStatus",
        "Editor2DLiveIterationEvidenceRow",
        "Editor2DLiveIterationReviewDesc",
        "Editor2DLiveIterationReviewModel",
        "make_editor_2d_live_iteration_review_model",
        "make_editor_2d_live_iteration_review_ui_model",
        "request_validation_execution",
        "request_package_script_execution",
        "request_native_handle_exposure"
    )) {
    Assert-ContainsText $editor2DLiveHeaderText $needle "2D live iteration review public header"
}

foreach ($needle in @(
        "2d_live_iteration.review",
        "2d_live_iteration.review.originality",
        "2d_live_iteration.review.runtime_scene_validation",
        "2d_live_iteration.review.source_pulse",
        "2d_live_iteration.review.package_replacement_safe_point",
        "2d_live_iteration.review.external_evidence",
        "ready_for_source_pulse_safe_point",
        "runtime scene package validation evidence is required",
        "2D live iteration review rejects validation execution from editor core",
        "2D live iteration review rejects package script execution",
        "2D live iteration review does not expose native handles"
    )) {
    Assert-ContainsText $editor2DLiveSourceText $needle "2D live iteration review source"
}

foreach ($forbiddenNeedle in @(
        "std::system",
        "CreateProcess",
        "ShellExecute",
        "WindowsFileWatcher",
        "PollingFileWatcher",
        "ReadDirectoryChangesW"
    )) {
    Assert-DoesNotContainText $editor2DLiveSourceText $forbiddenNeedle "2D live iteration editor review execution boundary"
}

foreach ($needle in @(
        "editor 2d live iteration review reports ready when originality validation source pulse and safe point rows",
        "editor 2d live iteration review keeps editor core non mutating and non executing",
        "editor 2d live iteration review renders retained ui rows",
        "editor 2d live iteration review rejects package script arbitrary shell and native handle claims",
        "editor 2d live iteration review keeps host gated rows host gated until external evidence is supplied"
    )) {
    Assert-ContainsText $editor2DLiveTestsText $needle "2D live iteration review tests"
}

foreach ($needle in @(
        "Editor2DLiveIterationReviewModel",
        "make_editor_2d_live_iteration_review_model",
        "2d_live_iteration.review.package_replacement_safe_point",
        "rejects mutation, validation execution, arbitrary shell execution, package script execution, and native handle exposure from editor core"
    )) {
    Assert-ContainsText $editor2DLiveManifestText $needle "engine manifest 2D live iteration review"
}

Assert-ContainsText $editor2DLiveCurrentCapabilitiesText "2D Live Iteration Review Model v1" "docs/current-capabilities.md 2D live iteration review"
Assert-ContainsText $editor2DLiveCurrentCapabilitiesText "does not execute recook, mutate packages, construct file watchers" "docs/current-capabilities.md 2D live iteration review non-claims"
Assert-ContainsText $editor2DLivePlanText "Phase 4 validation evidence" "Original 2D authoring plan Phase 4 evidence"
