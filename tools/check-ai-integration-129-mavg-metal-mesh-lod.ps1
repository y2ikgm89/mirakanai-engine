#requires -Version 7.0
#requires -PSEdition Core
# Chapter 129 for check-ai-integration.ps1 MAVG Metal mesh LOD host evidence gate.

$metalMavgHeaderText = Get-AgentSurfaceText "engine/rhi/metal/include/mirakana/rhi/metal/metal_mavg_mesh_lod.hpp"
$metalMavgSourceText = Get-AgentSurfaceText "engine/rhi/metal/src/metal_mavg_mesh_lod.cpp"
$metalMavgTestsText = Get-AgentSurfaceText "tests/unit/metal_mavg_mesh_lod_tests.cpp"
$metalCMakeText = Get-AgentSurfaceText "engine/rhi/metal/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$validatorText = Get-AgentSurfaceText "tools/check-mavg-metal-host-evidence.ps1"
$commandsFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgAdvancedPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md"

foreach ($needle in @(
        "MetalMavgMeshLodHostEvidenceDesc",
        "MetalMavgMeshLodHostEvidenceResult",
        "MetalMavgMeshLodDiagnosticCode",
        "MetalMavgMeshLodEvidenceStatus",
        "evaluate_mavg_metal_mesh_lod_host_evidence",
        "metal_mavg_mesh_lod_host_evidence_status_line",
        "has_mavg_metal_mesh_lod_diagnostic",
        "missing_feature_set_table_row",
        "missing_mesh_shader_support",
        "missing_object_shader_support",
        "missing_object_mesh_pipeline",
        "missing_object_mesh_dispatch",
        "simulator_only_evidence",
        "cross_backend_inference",
        "ray_tracing_pipeline_conflation",
        "nanite_claim_not_allowed",
        "broad_readiness_not_allowed",
        "bool mavg_metal_mesh_lod_ready{false};",
        "bool mavg_mesh_shader_lod_ready{false};",
        "bool mavg_metal_ray_tracing_ready{false};"
    )) {
    Assert-ContainsText $metalMavgHeaderText $needle "metal_mavg_mesh_lod.hpp public contract"
}

foreach ($needle in @(
        "Metal MAVG mesh LOD evidence requires a macOS Apple host with Xcode Metal tooling",
        "Metal MAVG mesh LOD evidence requires an Apple Metal Feature Set Tables row id",
        "Metal MAVG mesh LOD evidence requires object/mesh shader dispatch execution",
        "D3D12 or Vulkan evidence cannot promote Metal MAVG mesh LOD readiness",
        "Metal ray tracing evidence must stay separate from Metal mesh LOD evidence",
        "result.mavg_metal_mesh_lod_ready = true",
        "mavg_metal_mesh_lod_ready=",
        "mavg_mesh_shader_lod_ready="
    )) {
    Assert-ContainsText $metalMavgSourceText $needle "metal_mavg_mesh_lod.cpp fail-closed implementation"
}

foreach ($needle in @(
        "metal mavg mesh lod host gate rejects non apple hosts",
        "metal mavg mesh lod host gate requires apple feature table and toolchain evidence",
        "metal mavg mesh lod host gate requires object mesh execution proof",
        "metal mavg mesh lod host gate promotes only selected metal local readiness",
        "metal mavg mesh lod host gate blocks inference native handles nanite and broad claims",
        "mavg_metal_mesh_lod_ready=1",
        "mavg_mesh_shader_lod_ready=0",
        "mavg_metal_ray_tracing_ready=0"
    )) {
    Assert-ContainsText $metalMavgTestsText $needle "MK_mavg_metal_mesh_lod_tests coverage"
}

Assert-ContainsText $metalCMakeText "src/metal_mavg_mesh_lod.cpp" "engine/rhi/metal/CMakeLists.txt Metal MAVG source registration"
Assert-ContainsText $rootCMakeText "MK_mavg_metal_mesh_lod_tests" "root CMake Metal MAVG test target"

foreach ($needle in @(
        "validation_recipe=mavg-metal-mesh-lod-host-evidence",
        "MK_mavg_metal_mesh_lod_tests",
        'mavg_metal_mesh_lod_status=$status',
        'mavg_metal_mesh_lod_ready=$(ConvertTo-CounterBit $ready)',
        "mavg_mesh_shader_lod_ready=0",
        "mavg_metal_ray_tracing_ready=0",
        "mavg_metal_mesh_lod_feature_set_table_source=Apple-Metal-Feature-Set-Tables-2026-05-21",
        "mavg_metal_mesh_lod_object_mesh_pipeline_rows=0",
        "mavg_metal_mesh_lod_object_mesh_dispatch_rows=0",
        "mavg_metal_mesh_lod_readback_hash_rows=0",
        "mavg_metal_mesh_lod_cross_backend_inference=0",
        "mavg_metal_mesh_lod_ray_tracing_pipeline_conflation=0",
        "mavg_metal_mesh_lod_native_handles_exposed=0",
        "mavg_metal_mesh_lod_nanite_compatible=0",
        "mavg_metal_mesh_lod_broad_backend_readiness=0",
        "MAVG Metal mesh LOD readiness is incomplete"
    )) {
    Assert-ContainsText $validatorText $needle "tools/check-mavg-metal-host-evidence.ps1"
}

Assert-ContainsText $commandsFragmentText "mavgMetalHostEvidenceCheck" "manifest commands MAVG Metal host evidence command"

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgAdvancedPlanText; Label = "MAVG advanced evidence plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" }
    )) {
    foreach ($needle in @(
            "mavg-metal-mesh-lod-host-evidence-v1",
            "metal_mavg_mesh_lod.hpp",
            "MetalMavgMeshLodHostEvidenceDesc",
            "evaluate_mavg_metal_mesh_lod_host_evidence",
            "tools/check-mavg-metal-host-evidence.ps1",
            "mavg_metal_mesh_lod_status=host_evidence_required",
            "mavg_metal_mesh_lod_ready=0",
            "Apple Metal Feature Set Tables",
            "object/mesh shader",
            "readback hash",
            "package-visible output",
            "cross-backend inference",
            "ray tracing",
            "Nanite",
            "broad MAVG backend readiness"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) Metal MAVG mesh LOD host gate"
    }
    foreach ($forbiddenNeedle in @(
            "mavg_metal_mesh_lod_ready=1",
            "mavg_metal_mesh_lod_status=ready",
            "mavg_metal_ray_tracing_ready=1"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden Metal MAVG ready claim"
    }
}

$manifest = $manifestText | ConvertFrom-Json
$metalModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi_metal" })
if ($metalModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi_metal module" }
$metalManifestText = ((@($metalModule[0].publicHeaders) -join " "),
    (@($metalModule[0].recentEvidence) -join " "),
    [string]$metalModule[0].purpose) -join " "
foreach ($needle in @(
        "metal_mavg_mesh_lod.hpp",
        "MetalMavgMeshLodHostEvidenceDesc",
        "mavg_metal_mesh_lod_ready=0",
        "mavg_mesh_shader_lod_ready=0",
        "mavg_metal_ray_tracing_ready=0"
    )) {
    Assert-ContainsText $metalManifestText $needle "engine/agent/manifest.json MK_rhi_metal Metal MAVG evidence"
}
