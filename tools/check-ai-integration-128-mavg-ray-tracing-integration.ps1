#requires -Version 7.0
#requires -PSEdition Core
# Chapter 128 for check-ai-integration.ps1 MAVG ray tracing integration policy gate.

$mavgRayTracingHeaderText = Get-AgentSurfaceText "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/mavg_ray_tracing_integration.hpp"
$mavgRayTracingSourceText = Get-AgentSurfaceText "engine/runtime_scene_rhi/src/mavg_ray_tracing_integration.cpp"
$mavgRayTracingTestsText = Get-AgentSurfaceText "tests/unit/runtime_scene_rhi_mavg_ray_tracing_integration_tests.cpp"
$runtimeSceneRhiCMakeText = Get-AgentSurfaceText "engine/runtime_scene_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$validatorText = Get-AgentSurfaceText "tools/validate-mavg-ray-tracing-integration.ps1"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgAdvancedPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "MavgRayTracingGeometryPolicy",
        "MavgRayTracingBackendKind",
        "MavgRayTracingIndexFormat",
        "MavgRayTracingMaterialAlphaPolicy",
        "MavgRayTracingIntegrationDiagnosticCode",
        "MavgRayTracingBlasInputRow",
        "MavgRayTracingBackendExecutionRow",
        "MavgRayTracingIntegrationDesc",
        "MavgRayTracingIntegrationResult",
        "plan_mavg_ray_tracing_blas_inputs",
        "has_mavg_ray_tracing_integration_diagnostic",
        "resident_cluster_blas",
        "fallback_mesh_blas",
        "implicit_geometry_mode_switch",
        "fallback_mode_mismatch",
        "d3d12_acceleration_structure_build_missing",
        "vulkan_acceleration_structure_build_missing",
        "bool mavg_ray_tracing_policy_ready{false};",
        "bool mavg_ray_tracing_integration_ready{false};",
        "bool mavg_metal_ray_tracing_ready{false};",
        "bool mavg_broad_ray_tracing_readiness_ready{false};"
    )) {
    Assert-ContainsText $mavgRayTracingHeaderText $needle "mavg_ray_tracing_integration.hpp public contract"
}

foreach ($needle in @(
        "validate_mavg_cluster_graph",
        "stable cluster ids",
        "stable replay hash",
        "positive geometry byte count",
        "valid transform row",
        "reject alpha-blended material policy",
        "exactly one explicit geometry source",
        "must not silently switch to fallback mesh geometry",
        "explicit fallback mesh match evidence",
        "BuildRaytracingAccelerationStructure evidence",
        "VK_KHR_acceleration_structure feature evidence",
        "Metal ray tracing readiness belongs to the Apple-host Metal task",
        "result.mavg_ray_tracing_integration_ready ="
    )) {
    Assert-ContainsText $mavgRayTracingSourceText $needle "mavg_ray_tracing_integration.cpp fail-closed implementation"
}

foreach ($needle in @(
        "runtime scene rhi mavg ray tracing integration accepts selected policy rows",
        "runtime scene rhi mavg ray tracing integration rejects implicit mode switches",
        "runtime scene rhi mavg ray tracing integration rejects incomplete blas input evidence",
        "runtime scene rhi mavg ray tracing integration requires backend execution before ready",
        "runtime scene rhi mavg ray tracing integration promotes ready only with backend as build rows",
        "runtime scene rhi mavg ray tracing integration rejects incomplete backend rows",
        "runtime scene rhi mavg ray tracing integration rejects native handles metal and broad readiness",
        "alpha_blended_rejected",
        "!result.mavg_ray_tracing_integration_ready"
    )) {
    Assert-ContainsText $mavgRayTracingTestsText $needle "MK_runtime_scene_rhi_mavg_ray_tracing_integration_tests coverage"
}

Assert-ContainsText $runtimeSceneRhiCMakeText "src/mavg_ray_tracing_integration.cpp" "engine/runtime_scene_rhi/CMakeLists.txt ray tracing source registration"
Assert-ContainsText $rootCMakeText "MK_runtime_scene_rhi_mavg_ray_tracing_integration_tests" "root CMake ray tracing test target"

foreach ($needle in @(
        "validation_recipe=mavg-ray-tracing-integration",
        "MK_runtime_scene_rhi_mavg_ray_tracing_integration_tests",
        '$status = "backend_acceleration_structure_build_required"',
        'mavg_ray_tracing_policy_ready=$(ConvertTo-CounterBit $policyReady)',
        'mavg_ray_tracing_integration_ready=$(ConvertTo-CounterBit $integrationReady)',
        "mavg_ray_tracing_backend_execution_rows=0",
        "mavg_ray_tracing_d3d12_dxr_feature_rows=0",
        "mavg_ray_tracing_d3d12_acceleration_structure_build_rows=0",
        "mavg_ray_tracing_vulkan_acceleration_structure_feature_rows=0",
        "mavg_ray_tracing_vulkan_acceleration_structure_build_rows=0",
        "mavg_ray_tracing_implicit_mode_switches=0",
        "mavg_ray_tracing_fallback_mode_mismatches=0",
        "mavg_ray_tracing_native_handles_exposed=0",
        "mavg_ray_tracing_metal_readiness=0",
        "mavg_ray_tracing_mesh_shader_execution=0",
        "mavg_ray_tracing_nanite_equivalence=0",
        "mavg_ray_tracing_broad_backend_readiness=0",
        "MAVG ray tracing integration is incomplete"
    )) {
    Assert-ContainsText $validatorText $needle "tools/validate-mavg-ray-tracing-integration.ps1"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgAdvancedPlanText; Label = "MAVG advanced evidence plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" }
    )) {
    foreach ($needle in @(
            "mavg-ray-tracing-integration-v1",
            "mavg_ray_tracing_integration.hpp",
            "MavgRayTracingGeometryPolicy",
            "MavgRayTracingBlasInputRow",
            "plan_mavg_ray_tracing_blas_inputs",
            "tools/validate-mavg-ray-tracing-integration.ps1",
            "mavg_ray_tracing_policy_ready=1",
            "mavg_ray_tracing_integration_ready=0",
            "backend_acceleration_structure_build_required",
            "resident-cluster BLAS",
            "fallback-mesh BLAS",
            "BuildRaytracingAccelerationStructure",
            "VK_KHR_acceleration_structure",
            "native handles",
            "mesh shader",
            "Metal readiness",
            "Nanite",
            "broad MAVG backend readiness",
            "broad"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) ray tracing integration policy gate"
    }
    foreach ($forbiddenNeedle in @(
            "mavg_ray_tracing_integration_ready=1",
            "mavg_ray_tracing_integration_status=ready"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden ray tracing ready claim"
    }
}

$manifest = $manifestText | ConvertFrom-Json
$runtimeSceneRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_scene_rhi" })
if ($runtimeSceneRhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_scene_rhi module" }
$runtimeSceneRhiManifestText = ((@($runtimeSceneRhiModule[0].publicHeaders) -join " "),
    (@($runtimeSceneRhiModule[0].recentEvidence) -join " "),
    [string]$runtimeSceneRhiModule[0].purpose) -join " "
foreach ($needle in @(
        "mavg_ray_tracing_integration.hpp",
        "MavgRayTracingGeometryPolicy",
        "MavgRayTracingBlasInputRow",
        "plan_mavg_ray_tracing_blas_inputs",
        "mavg_ray_tracing_policy_ready=1",
        "mavg_ray_tracing_integration_ready=0"
    )) {
    Assert-ContainsText $runtimeSceneRhiManifestText $needle "engine/agent/manifest.json MK_runtime_scene_rhi ray tracing evidence"
}
