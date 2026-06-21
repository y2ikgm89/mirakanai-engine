#requires -Version 7.0
#requires -PSEdition Core
# Chapter 123 for check-ai-integration.ps1 MAVG mesh shader capability gate.

$mavgMeshShaderGateHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_mesh_shader_capability_gate.hpp"
$mavgMeshShaderGateSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_mesh_shader_capability_gate.cpp"
$mavgMeshShaderGateTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_mesh_shader_capability_gate_tests.cpp"
$runtimeRhiCMakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$sampleDesktopRuntimeGameText = Get-AgentSurfaceText "games/sample_desktop_runtime_game/main.cpp"
$validatorText = Get-AgentSurfaceText "tools/validate-mavg-mesh-shader-capability-gate.ps1"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$productionMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$mavgMeshShaderGatePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-mavg-mesh-shader-capability-gate-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "RuntimeMavgMeshShaderCapabilityQueryKind",
        "d3d12_options7_mesh_shader_tier",
        "vulkan_ext_mesh_shader_features_properties",
        "RuntimeMavgMeshShaderCapabilityBackendRow",
        "RuntimeMavgMeshShaderCapabilityGateDesc",
        "RuntimeMavgMeshShaderCapabilityGateResult",
        "RuntimeMavgMeshShaderCapabilityDiagnosticCode",
        "std::span<const RuntimeMavgMeshShaderCapabilityBackendRow>",
        "evaluate_runtime_mavg_mesh_shader_capability_gate",
        "has_runtime_mavg_mesh_shader_capability_gate_diagnostic",
        "bool compute_indirect_fallback_ready{false};",
        "bool fallback_to_conventional_indexed_draws{false};",
        "bool mavg_mesh_shader_lod_ready{false};",
        "bool exposed_native_handles{false};",
        "bool executed_mesh_shader{false};",
        "bool claimed_backend_execution{false};",
        "bool claimed_metal_readiness{false};",
        "bool claimed_nanite_equivalence{false};",
        "bool claimed_broad_backend_readiness{false};",
        "official mesh/task shader feature and limit queries"
    )) {
    Assert-ContainsText $mavgMeshShaderGateHeaderText $needle "mavg_mesh_shader_capability_gate.hpp public contract"
}

foreach ($needle in @(
        "RuntimeMavgMeshShaderCapabilityDiagnosticCode::missing_feature_query",
        "RuntimeMavgMeshShaderCapabilityDiagnosticCode::mesh_shader_unsupported",
        "RuntimeMavgMeshShaderCapabilityDiagnosticCode::missing_required_limits",
        "RuntimeMavgMeshShaderCapabilityDiagnosticCode::invalid_mesh_output_limits",
        "RuntimeMavgMeshShaderCapabilityDiagnosticCode::missing_compute_indirect_fallback",
        "RuntimeMavgMeshShaderCapabilityDiagnosticCode::native_handle_access",
        "RuntimeMavgMeshShaderCapabilityDiagnosticCode::executed_mesh_shader",
        "RuntimeMavgMeshShaderCapabilityDiagnosticCode::claimed_backend_execution",
        "RuntimeMavgMeshShaderCapabilityDiagnosticCode::claimed_metal_readiness",
        "RuntimeMavgMeshShaderCapabilityDiagnosticCode::claimed_nanite_equivalence",
        "RuntimeMavgMeshShaderCapabilityDiagnosticCode::claimed_broad_backend_readiness",
        "result.capability_gate_ready = result.diagnostics.empty()",
        "desc.compute_indirect_fallback_ready",
        "result.d3d12_capability_ready",
        "result.vulkan_capability_ready"
    )) {
    Assert-ContainsText $mavgMeshShaderGateSourceText $needle "mavg_mesh_shader_capability_gate.cpp fail-closed implementation"
}

foreach ($needle in @(
        "runtime rhi mavg mesh shader capability gate accepts selected D3D12 and Vulkan query rows",
        "runtime rhi mavg mesh shader capability gate requires compute indirect fallback",
        "runtime rhi mavg mesh shader capability gate rejects missing feature query and unsupported backend",
        "runtime rhi mavg mesh shader capability gate rejects execution and broad claims",
        "runtime rhi mavg mesh shader capability gate rejects invalid mesh output limits",
        "result.backend_row_count == 2U",
        "result.ready_backend_count == 2U",
        "result.feature_query_row_count == 2U",
        "result.pipeline_statistics_row_count == 2U",
        "!result.executed_mesh_shader",
        "!result.claimed_broad_backend_readiness"
    )) {
    Assert-ContainsText $mavgMeshShaderGateTestsText $needle "MK_runtime_rhi_mavg_mesh_shader_capability_gate_tests coverage"
}

Assert-ContainsText $runtimeRhiCMakeText "src/mavg_mesh_shader_capability_gate.cpp" "engine/runtime_rhi/CMakeLists.txt mesh shader gate source registration"
Assert-ContainsText $rootCMakeText "MK_runtime_rhi_mavg_mesh_shader_capability_gate_tests" "root CMake mesh shader gate test target"

foreach ($needle in @(
        "--require-mavg-mesh-shader-capability-gate",
        "d3d12_options7_mesh_shader_tier",
        "vulkan_ext_mesh_shader_features_properties",
        "mavg_mesh_shader_capability_gate_status=",
        "mavg_mesh_shader_capability_gate_ready=",
        "mavg_mesh_shader_capability_gate_backend_rows=",
        "mavg_mesh_shader_capability_gate_ready_backends=",
        "mavg_mesh_shader_capability_gate_d3d12_ready=",
        "mavg_mesh_shader_capability_gate_vulkan_ready=",
        "mavg_mesh_shader_capability_gate_feature_query_rows=",
        "mavg_mesh_shader_capability_gate_pipeline_statistics_rows=",
        "mavg_mesh_shader_capability_gate_fallback_ready=",
        "mavg_mesh_shader_capability_gate_fallback_to_conventional_indexed_draws=",
        "mavg_mesh_shader_capability_gate_lod_ready=",
        "mavg_mesh_shader_capability_gate_lod_d3d12_ready=",
        "mavg_mesh_shader_capability_gate_lod_vulkan_ready=",
        "mavg_mesh_shader_capability_gate_native_handles_exposed=",
        "mavg_mesh_shader_capability_gate_mesh_shader_execution=",
        "mavg_mesh_shader_capability_gate_backend_execution=",
        "mavg_mesh_shader_capability_gate_metal_readiness=",
        "mavg_mesh_shader_capability_gate_nanite_equivalence=",
        "mavg_mesh_shader_capability_gate_broad_backend_readiness="
    )) {
    Assert-ContainsText $sampleDesktopRuntimeGameText $needle "sample_desktop_runtime_game mesh shader gate package counters"
}

foreach ($needle in @(
        "validation_recipe=mavg-mesh-shader-capability-gate",
        "MK_runtime_rhi_mavg_mesh_shader_capability_gate_tests",
        "sample_desktop_runtime_game",
        "--require-mavg-mesh-shader-capability-gate",
        '$gateReadyCounter -eq "1"',
        '$backendRows -eq "2"',
        '$readyBackends -eq "2"',
        '$featureQueryRows -eq "2"',
        '$pipelineStatisticsRows -eq "2"',
        '$fallbackToConventional -eq "1"',
        '$meshShaderExecution -eq "0"',
        '$broadBackendReadiness -eq "0"',
        'mavg_mesh_shader_capability_gate_ready=$(ConvertTo-CounterBit $ready)',
        'mavg_mesh_shader_capability_gate_broad_backend_readiness=$broadBackendReadiness'
    )) {
    Assert-ContainsText $validatorText $needle "tools/validate-mavg-mesh-shader-capability-gate.ps1"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $productionMasterPlanText; Label = "production completion master plan" },
        @{ Text = $mavgMasterPlanText; Label = "MAVG master plan" },
        @{ Text = $mavgMeshShaderGatePlanText; Label = "MAVG mesh shader capability gate plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $aiLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" }
    )) {
    foreach ($needle in @(
            "mavg-mesh-shader-capability-gate-v1",
            "RuntimeMavgMeshShaderCapabilityGateResult",
            "D3D12_FEATURE_D3D12_OPTIONS7",
            "VkPhysicalDeviceMeshShaderFeaturesEXT",
            "VkPhysicalDeviceMeshShaderPropertiesEXT",
            "mavg_mesh_shader_capability_gate_ready=1",
            "mesh shader execution remains 0"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) mesh shader capability gate claim"
    }
    foreach ($needle in @(
            "native handles",
            "Metal readiness",
            "Nanite",
            "broad MAVG backend readiness",
            "broad optimization"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) mesh shader capability gate non-claims"
    }
}

$productionLoopText = $manifest.aiOperableProductionLoop | ConvertTo-Json -Depth 8
foreach ($needle in @(
        "mavg-mesh-shader-capability-gate-v1",
        "RuntimeMavgMeshShaderCapabilityGateResult",
        "D3D12_FEATURE_D3D12_OPTIONS7",
        "MeshShaderTier",
        "VK_EXT_mesh_shader",
        "VkPhysicalDeviceMeshShaderFeaturesEXT",
        "VkPhysicalDeviceMeshShaderPropertiesEXT",
        "mavg_mesh_shader_capability_gate_ready=1",
        "mesh shader execution remains 0",
        "broad MAVG backend readiness"
    )) {
    Assert-ContainsText $productionLoopText $needle "engine/agent/manifest.json aiOperableProductionLoop mesh shader capability gate evidence"
}

$runtimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($runtimeRhiModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module"
}
$runtimeRhiManifestText = ((@($runtimeRhiModule[0].publicHeaders) -join " "),
    (@($runtimeRhiModule[0].recentEvidence) -join " "),
    [string]$runtimeRhiModule[0].purpose) -join " "
foreach ($needle in @(
        "mavg_mesh_shader_capability_gate.hpp",
        "RuntimeMavgMeshShaderCapabilityGateResult",
        "evaluate_runtime_mavg_mesh_shader_capability_gate",
        "D3D12_FEATURE_D3D12_OPTIONS7",
        "VkPhysicalDeviceMeshShaderFeaturesEXT",
        "mavg_mesh_shader_capability_gate_ready=1",
        "mesh shader execution remains 0",
        "broad MAVG backend readiness"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi mesh shader capability gate evidence"
}
