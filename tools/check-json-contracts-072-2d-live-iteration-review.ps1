#requires -Version 7.0
#requires -PSEdition Core

# Chapter 7.2 for check-json-contracts.ps1 2D live iteration editor review manifest contracts.

$engine = Read-Json "engine/agent/manifest.json"
$editor2DLiveModule = @($engine.modules | Where-Object { $_.name -eq "MK_editor_core" })
if ($editor2DLiveModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_editor_core module for 2D live iteration review"
}

if (@($editor2DLiveModule[0].publicHeaders) -notcontains "editor/core/include/mirakana/editor/playtest_package_review.hpp") {
    Write-Error "engine manifest MK_editor_core publicHeaders must include playtest_package_review.hpp"
}

$editor2DLiveContractText = [string]$editor2DLiveModule[0].purpose + " " + [string](@($editor2DLiveModule[0].recentEvidence) -join " ")
foreach ($needle in @(
        "Editor2DLiveIterationStageStatus",
        "Editor2DLiveIterationEvidenceRow",
        "Editor2DLiveIterationReviewDesc",
        "Editor2DLiveIterationReviewModel",
        "make_editor_2d_live_iteration_review_model",
        "make_editor_2d_live_iteration_review_ui_model",
        "2d_live_iteration.review",
        "2d_live_iteration.review.originality",
        "2d_live_iteration.review.runtime_scene_validation",
        "2d_live_iteration.review.source_pulse",
        "2d_live_iteration.review.package_replacement_safe_point",
        "2d_live_iteration.review.external_evidence",
        "ready_for_source_pulse_safe_point",
        "host_gated",
        "evidence_required",
        "validation execution",
        "package script execution",
        "native handle exposure",
        "external engine schemas/assets/code",
        "package-visible smoke counters"
    )) {
    if (-not $editor2DLiveContractText.Contains($needle)) {
        Write-Error "engine manifest MK_editor_core contract must describe the 2D live iteration editor review and non-claims: $needle"
    }
}
