#requires -Version 7.0
#requires -PSEdition Core
# Chapter 125 for check-ai-integration.ps1 MAVG D3D12 mesh shader LOD execution.

$mavgMeshShaderLodHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/mavg_mesh_shader_lod.hpp"
$mavgMeshShaderLodSourceText = Get-AgentSurfaceText "engine/renderer/src/mavg_mesh_shader_lod.cpp"
$d3d12MavgMeshShaderLodHeaderText = Get-AgentSurfaceText "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_mavg_mesh_shader_lod.hpp"
$d3d12MavgMeshShaderLodSourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_mavg_mesh_shader_lod.cpp"
$mavgMeshShaderLodTestsText = Get-AgentSurfaceText "tests/unit/mavg_mesh_shader_lod_tests.cpp"
$d3d12MavgMeshShaderLodTestsText = Get-AgentSurfaceText "tests/unit/d3d12_mavg_mesh_shader_lod_tests.cpp"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$rendererCMakeText = Get-AgentSurfaceText "engine/renderer/CMakeLists.txt"
$d3d12CMakeText = Get-AgentSurfaceText "engine/rhi/d3d12/CMakeLists.txt"
$hlslText = Get-AgentSurfaceText "shaders/d3d12/mavg_mesh_shader_lod.hlsl"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$advancedMavgPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "enum class MavgMeshShaderLodDiagnosticCode",
        "MavgMeshShaderLodMaterialRootRow",
        "MavgMeshShaderLodMeshletRow",
        "MavgMeshShaderLodTaskRow",
        "MavgMeshShaderLodFallbackDrawRow",
        "MavgMeshShaderLodDesc",
        "MavgMeshShaderLodPlan",
        "plan_mavg_mesh_shader_lod_tasks",
        "has_mavg_mesh_shader_lod_diagnostic",
        "bool fallback_promotes_mesh_shader_lod_ready{false};",
        "bool mavg_mesh_shader_lod_ready{false};",
        "bool executed_mesh_shader{false};",
        "bool executed_d3d12{false};",
        "bool executed_vulkan{false};",
        "bool claimed_metal_readiness{false};",
        "bool claimed_nanite_equivalence{false};"
    )) {
    Assert-ContainsText $mavgMeshShaderLodHeaderText $needle "mavg_mesh_shader_lod.hpp public contract"
}

foreach ($needle in @(
        "MavgMeshShaderLodDiagnosticCode::missing_material_root",
        "MavgMeshShaderLodDiagnosticCode::mismatched_material_roots",
        "MavgMeshShaderLodDiagnosticCode::invalid_meshlet_group_size",
        "MavgMeshShaderLodDiagnosticCode::invalid_fallback_draw_range",
        "MavgMeshShaderLodDiagnosticCode::max_task_rows_exceeded",
        "plan.uses_mesh_shader_bind_points = true",
        "plan.uses_amplification_shader_bind_point = true",
        "plan.requires_input_assembler = false",
        "plan.requires_index_buffer = false",
        "fail_closed(plan)"
    )) {
    Assert-ContainsText $mavgMeshShaderLodSourceText $needle "mavg_mesh_shader_lod.cpp fail-closed implementation"
}

foreach ($needle in @(
        "D3d12MavgMeshShaderLodCapabilityResult",
        "D3d12MavgMeshShaderLodTaskRow",
        "D3d12MavgMeshShaderLodDispatchDesc",
        "D3d12MavgMeshShaderLodDispatchResult",
        "probe_d3d12_mavg_mesh_shader_lod_capability",
        "execute_d3d12_mavg_mesh_shader_lod",
        "bool mavg_mesh_shader_lod_d3d12_ready{false};",
        "bool pipeline_statistics_available{false};",
        "bool pipeline_statistics_host_gated{true};",
        'std::string amplification_shader_model{"as_6_5"};',
        'std::string mesh_shader_model{"ms_6_5"};',
        'std::string pixel_shader_model{"ps_6_0"};',
        "bool claimed_nanite_equivalence{false};",
        "bool claimed_metal_readiness{false};"
    )) {
    Assert-ContainsText $d3d12MavgMeshShaderLodHeaderText $needle "d3d12_mavg_mesh_shader_lod.hpp public contract"
}

foreach ($needle in @(
        "D3D12_FEATURE_DATA_D3D12_OPTIONS7",
        "CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7",
        "options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1",
        'LoadLibraryW(L"dxcompiler.dll")',
        "as_6_5",
        "ms_6_5",
        "ps_6_0",
        "D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS",
        "D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS",
        "CreatePipelineState",
        "DispatchMesh(1, 1, 1)",
        "result.dispatch_mesh_direct_calls = 1U",
        "result.execute_indirect_mesh_dispatch_calls == 0U",
        "result.used_input_layout = false",
        "result.used_index_buffer = false",
        "center_pixel_is_green",
        "d3d12_mesh_shader_tier_not_supported",
        "dxcompiler_unavailable"
    )) {
    Assert-ContainsText $d3d12MavgMeshShaderLodSourceText $needle "d3d12_mavg_mesh_shader_lod.cpp host-gated execution"
}

foreach ($needle in @(
        "mavg mesh shader lod planning converts selected clusters into deterministic meshlet tasks",
        "mavg mesh shader lod planning fails closed on wrong meshlet group size",
        "mavg mesh shader lod planning rejects mismatched material roots",
        "mavg mesh shader lod planning preserves conventional fallback without promoting mesh execution",
        "!plan.requires_input_assembler",
        "!plan.requires_index_buffer",
        "!plan.mavg_mesh_shader_lod_ready"
    )) {
    Assert-ContainsText $mavgMeshShaderLodTestsText $needle "MK_mavg_mesh_shader_lod_tests coverage"
}

foreach ($needle in @(
        "d3d12 mavg mesh shader lod capability probe records options7 support state",
        "d3d12 mavg mesh shader lod execution fails closed before host work for invalid rows",
        "d3d12 mavg mesh shader lod execution is host gated when tier support is unavailable",
        "d3d12 mavg mesh shader lod execution renders readback only on supported hosts",
        "result.dispatch_mesh_direct_calls == 1U",
        "result.execute_indirect_mesh_dispatch_calls == 0U",
        "!result.used_index_buffer",
        "!result.used_input_layout",
        "result.pipeline_statistics_host_gated",
        'result.amplification_shader_model == "as_6_5"',
        "!result.claimed_nanite_equivalence",
        "!result.claimed_metal_readiness"
    )) {
    Assert-ContainsText $d3d12MavgMeshShaderLodTestsText $needle "MK_d3d12_mavg_mesh_shader_lod_tests coverage"
}

foreach ($needle in @(
        "MK_mavg_mesh_shader_lod_tests",
        "MK_d3d12_mavg_mesh_shader_lod_tests"
    )) {
    Assert-ContainsText $rootCMakeText $needle "root CMake MAVG mesh shader LOD test target"
}
Assert-ContainsText $rendererCMakeText "src/mavg_mesh_shader_lod.cpp" "engine/renderer/CMakeLists.txt MAVG mesh shader LOD source"
Assert-ContainsText $d3d12CMakeText "src/d3d12_mavg_mesh_shader_lod.cpp" "engine/rhi/d3d12/CMakeLists.txt D3D12 MAVG mesh shader LOD source"

foreach ($needle in @(
        "DispatchMesh(1, 1, 1, payload_out)",
        "SetMeshOutputCounts(3, 1)",
        "float4 ps_main(VertexOut input) : SV_Target"
    )) {
    Assert-ContainsText $hlslText $needle "shaders/d3d12/mavg_mesh_shader_lod.hlsl traceability"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgMasterPlanText; Label = "MAVG master plan" },
        @{ Text = $advancedMavgPlanText; Label = "MAVG advanced backend evidence plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $aiLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" }
    )) {
    foreach ($needle in @(
            "MAVG Advanced Backend Evidence Closeout v1 Task 3",
            "mavg_mesh_shader_lod.hpp",
            "D3d12MavgMeshShaderLodDispatchResult",
            "D3D12_FEATURE_D3D12_OPTIONS7",
            "MeshShaderTier",
            "DispatchMesh",
            "host-gated"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) D3D12 mesh shader LOD evidence"
    }
    foreach ($needle in @(
            "mavg_mesh_shader_lod_ready",
            "Vulkan execution",
            "Metal readiness",
            "Nanite",
            "broad optimization"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) D3D12 mesh shader LOD non-claims"
    }
}

$rendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($rendererModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module"
}
$rendererModuleText = ((@($rendererModule[0].publicHeaders) -join " "),
    (@($rendererModule[0].recentEvidence) -join " "),
    [string]$rendererModule[0].purpose) -join " "
foreach ($needle in @(
        "mavg_mesh_shader_lod.hpp",
        "MavgMeshShaderLodPlan",
        "plan_mavg_mesh_shader_lod_tasks",
        "fallback_promotes_mesh_shader_lod_ready=false"
    )) {
    Assert-ContainsText $rendererModuleText $needle "engine/agent/manifest.json MK_renderer D3D12 mesh shader LOD evidence"
}

$d3d12Module = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi_d3d12" })
if ($d3d12Module.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi_d3d12 module"
}
$d3d12ModuleText = ((@($d3d12Module[0].publicHeaders) -join " "),
    (@($d3d12Module[0].recentEvidence) -join " "),
    [string]$d3d12Module[0].purpose) -join " "
foreach ($needle in @(
        "d3d12_mavg_mesh_shader_lod.hpp",
        "D3d12MavgMeshShaderLodDispatchResult",
        "execute_d3d12_mavg_mesh_shader_lod",
        "D3D12_FEATURE_D3D12_OPTIONS7",
        "MeshShaderTier",
        "DispatchMesh",
        "pipeline statistics host-gated"
    )) {
    Assert-ContainsText $d3d12ModuleText $needle "engine/agent/manifest.json MK_rhi_d3d12 mesh shader LOD evidence"
}
