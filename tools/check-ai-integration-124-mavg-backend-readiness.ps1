#requires -Version 7.0
#requires -PSEdition Core
# Chapter 124 for check-ai-integration.ps1 MAVG package-visible backend readiness.

$mavgBackendHeaderText = Get-AgentSurfaceText "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/mavg_backend_readiness_closeout.hpp"
$mavgBackendSourceText = Get-AgentSurfaceText "engine/runtime_scene_rhi/src/mavg_backend_readiness_closeout.cpp"
$mavgBackendTestsText = Get-AgentSurfaceText "tests/unit/runtime_scene_rhi_mavg_backend_readiness_closeout_tests.cpp"
$runtimeSceneRhiCMakeText = Get-AgentSurfaceText "engine/runtime_scene_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$sampleDesktopRuntimeGameText = Get-AgentSurfaceText "games/sample_desktop_runtime_game/main.cpp"
$sampleDesktopRuntimeCMakeText = Get-AgentSurfaceText "games/CMakeLists.txt"
$validatorText = Get-AgentSurfaceText "tools/validate-mavg-backend-readiness.ps1"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$mavgAdvancedPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "MavgBackendReadinessEvidenceKind",
        "MavgBackendReadinessEvidenceStatus",
        "MavgBackendReadinessDiagnosticCode",
        "MavgBackendReadinessEvidenceRow",
        "MavgBackendReadinessCloseoutDesc",
        "MavgBackendReadinessCloseoutResult",
        "evaluate_mavg_backend_readiness_closeout",
        "has_mavg_backend_readiness_closeout_diagnostic",
        "has_mavg_backend_readiness_closeout_row_diagnostic",
        "d3d12_compute_generated_indirect_consumption",
        "vulkan_compute_generated_indirect_consumption",
        "package_smoke_counter",
        "metal_host_gate",
        "bool mavg_package_visible_backend_readiness_ready{false};",
        "bool mavg_broad_backend_readiness_ready{false};"
    )) {
    Assert-ContainsText $mavgBackendHeaderText $needle "mavg_backend_readiness_closeout.hpp public contract"
}

foreach ($needle in @(
        "constexpr std::array<Kind, 9> kRequiredRows",
        "Code::missing_required_row",
        "Code::duplicate_required_row",
        "Code::execution_evidence_required",
        "Code::native_handle_access",
        "Code::metal_inference_rejected",
        "result.mavg_broad_backend_readiness_ready = false;",
        "result.mavg_package_visible_backend_readiness_ready"
    )) {
    Assert-ContainsText $mavgBackendSourceText $needle "mavg_backend_readiness_closeout.cpp fail-closed implementation"
}

foreach ($needle in @(
        "fails closed when evidence rows are empty",
        "succeeds only with every required package-visible row",
        "rejects missing vulkan row despite metal host gate",
        "rejects value-only rows where execution is required",
        "rejects native handle exposure",
        "rejects package smoke counters that are not ready",
        "rejects duplicate required rows"
    )) {
    Assert-ContainsText $mavgBackendTestsText $needle "MK_runtime_scene_rhi_mavg_backend_readiness_closeout_tests coverage"
}

Assert-ContainsText $runtimeSceneRhiCMakeText "src/mavg_backend_readiness_closeout.cpp" "engine/runtime_scene_rhi/CMakeLists.txt backend readiness source registration"
Assert-ContainsText $rootCMakeText "MK_runtime_scene_rhi_mavg_backend_readiness_closeout_tests" "root CMake backend readiness test target"
Assert-ContainsText $sampleDesktopRuntimeCMakeText "MK_runtime_scene_rhi" "sample_desktop_runtime_game links runtime_scene_rhi"

foreach ($needle in @(
        "--require-mavg-backend-readiness",
        "evaluate_mavg_backend_readiness",
        "mavg_package_visible_backend_readiness_status=",
        "mavg_package_visible_backend_readiness_ready=",
        "mavg_package_visible_backend_readiness_required_rows=",
        "mavg_package_visible_backend_readiness_ready_rows=",
        "mavg_package_visible_backend_readiness_diagnostics=",
        "mavg_package_visible_backend_readiness_native_handles_exposed=",
        "mavg_package_visible_backend_readiness_metal_inference=",
        "mavg_package_visible_backend_readiness_broad_backend_readiness="
    )) {
    Assert-ContainsText $sampleDesktopRuntimeGameText $needle "sample_desktop_runtime_game backend readiness package counters"
}

foreach ($needle in @(
        "validation_recipe=mavg-backend-readiness",
        "MK_runtime_scene_rhi_mavg_backend_readiness_closeout_tests|MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests",
        "sample_desktop_runtime_game",
        "--require-mavg-backend-readiness",
        '$readinessCounter -eq "1"',
        '$requiredRows -eq "9"',
        '$readyRows -eq "9"',
        '$diagnostics -eq "0"',
        '$nativeHandles -eq "0"',
        '$metalInference -eq "0"',
        '$broadBackendReadiness -eq "0"',
        'mavg_package_visible_backend_readiness_ready=$(ConvertTo-CounterBit $ready)'
    )) {
    Assert-ContainsText $validatorText $needle "tools/validate-mavg-backend-readiness.ps1"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "plan registry" },
        @{ Text = $mavgMasterPlanText; Label = "MAVG master plan" },
        @{ Text = $mavgAdvancedPlanText; Label = "MAVG advanced backend evidence plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" }
    )) {
    foreach ($needle in @(
            "mavg_backend_readiness_closeout.hpp",
            "MavgBackendReadinessCloseoutResult",
            "evaluate_mavg_backend_readiness_closeout",
            "tools/validate-mavg-backend-readiness.ps1 -RequireReady",
            "mavg_package_visible_backend_readiness_ready=1",
            "nine"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) backend readiness selected claim"
    }
    foreach ($needle in @(
            "native handles",
            "Metal",
            "Nanite",
            "broad"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) backend readiness non-claims"
    }
}

$runtimeSceneRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_scene_rhi" })
if ($runtimeSceneRhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_scene_rhi module" }
$runtimeSceneRhiManifestText = ((@($runtimeSceneRhiModule[0].publicHeaders) -join " "),
    (@($runtimeSceneRhiModule[0].recentEvidence) -join " "),
    [string]$runtimeSceneRhiModule[0].purpose) -join " "
foreach ($needle in @(
        "mavg_backend_readiness_closeout.hpp",
        "MavgBackendReadinessCloseoutDesc",
        "MavgBackendReadinessCloseoutResult",
        "evaluate_mavg_backend_readiness_closeout",
        "mavg_package_visible_backend_readiness_ready=1",
        "zero native handles",
        "zero Metal inference",
        "zero broad backend readiness"
    )) {
    Assert-ContainsText $runtimeSceneRhiManifestText $needle "engine/agent/manifest.json MK_runtime_scene_rhi backend readiness"
}

foreach ($forbiddenNeedle in @(
        "mavg_nanite_compatible=1",
        "mavg_nanite_equivalent=1",
        "mavg_nanite_superior=1",
        "mavg_package_visible_backend_readiness_broad_backend_readiness=1"
    )) {
    Assert-DoesNotContainText $manifestText $forbiddenNeedle "engine/agent/manifest.json forbidden MAVG backend readiness claim"
}
