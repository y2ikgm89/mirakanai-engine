#requires -Version 7.0
#requires -PSEdition Core
# Chapter 127 for check-ai-integration.ps1 MAVG deformation integration policy gate.

$mavgDeformationHeaderText = Get-AgentSurfaceText "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/mavg_deformation_integration.hpp"
$mavgDeformationSourceText = Get-AgentSurfaceText "engine/runtime_scene_rhi/src/mavg_deformation_integration.cpp"
$mavgDeformationTestsText = Get-AgentSurfaceText "tests/unit/runtime_scene_rhi_mavg_deformation_integration_tests.cpp"
$runtimeSceneRhiCMakeText = Get-AgentSurfaceText "engine/runtime_scene_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$validatorText = Get-AgentSurfaceText "tools/validate-mavg-deformation-integration.ps1"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgAdvancedPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "MavgDeformationIntegrationKind",
        "MavgDeformationBackendKind",
        "MavgDeformationIntegrationDiagnosticCode",
        "MavgDeformationClusterBoundsRow",
        "MavgDeformationBackendExecutionRow",
        "MavgDeformationBackendExecutionEvidenceDesc",
        "MavgDeformationBackendExecutionEvidenceResult",
        "MavgDeformationIntegrationDesc",
        "MavgDeformationIntegratedCluster",
        "MavgDeformationIntegrationResult",
        "plan_mavg_deformation_integrated_clusters",
        "evaluate_mavg_deformation_backend_execution_evidence",
        "has_mavg_deformation_integration_diagnostic",
        "rigid_transform",
        "linear_blend_skinning",
        "morph_target",
        "topology_changing_deformation",
        "runtime_generated_triangle_topology",
        "unbounded_vertex_displacement",
        "backend_execution_required",
        "bool mavg_deformation_policy_ready{false};",
        "bool mavg_deformation_integration_ready{false};",
        "bool mavg_broad_deformation_readiness_ready{false};"
    )) {
    Assert-ContainsText $mavgDeformationHeaderText $needle "mavg_deformation_integration.hpp public contract"
}

foreach ($needle in @(
        "validate_mavg_cluster_graph",
        "stable cluster ids",
        "conservative bounds must contain the original cluster bounds",
        "material partition roots",
        "valid resident page evidence",
        "preserve fallback draw ranges",
        "rejects topology-changing deformation",
        "rejects runtime-generated triangle topology",
        "rejects unbounded vertex displacement",
        "one to four joint influences",
        "explicit conservative bounds expansion",
        "must not expose native handles",
        "does not promote broad deformation readiness",
        "selected backend upload",
        "compute morph/skinned payload",
        "submitted fence",
        "result.mavg_deformation_integration_ready ="
    )) {
    Assert-ContainsText $mavgDeformationSourceText $needle "mavg_deformation_integration.cpp fail-closed implementation"
}

foreach ($needle in @(
        "runtime scene rhi mavg deformation integration accepts selected value policy rows",
        "runtime scene rhi mavg deformation integration rejects topology changing rows",
        "runtime scene rhi mavg deformation integration rejects unbounded skinning",
        "runtime scene rhi mavg deformation integration rejects nonconservative morph bounds",
        "runtime scene rhi mavg deformation integration preserves material page and draw range",
        "runtime scene rhi mavg deformation integration requires backend execution before ready",
        "runtime scene rhi mavg deformation backend evidence accepts selected compute morph rows",
        "runtime scene rhi mavg deformation backend evidence rejects missing execution counters",
        "runtime scene rhi mavg deformation backend evidence keeps metal host gated",
        "runtime scene rhi mavg deformation backend evidence rejects native handles and broad readiness",
        "runtime scene rhi mavg deformation integration promotes ready only with reviewed backend rows",
        "runtime scene rhi mavg deformation integration rejects native handles and broad readiness",
        "max_joint_influences = 8",
        "!result.mavg_deformation_integration_ready"
    )) {
    Assert-ContainsText $mavgDeformationTestsText $needle "MK_runtime_scene_rhi_mavg_deformation_integration_tests coverage"
}

Assert-ContainsText $runtimeSceneRhiCMakeText "src/mavg_deformation_integration.cpp" "engine/runtime_scene_rhi/CMakeLists.txt deformation source registration"
Assert-ContainsText $rootCMakeText "MK_runtime_scene_rhi_mavg_deformation_integration_tests" "root CMake deformation test target"

foreach ($needle in @(
        "validation_recipe=mavg-deformation-integration",
        "MK_runtime_scene_rhi_mavg_deformation_integration_tests",
        '$status = "ready"',
        'mavg_deformation_policy_ready=$(ConvertTo-CounterBit $policyReady)',
        'mavg_deformation_integration_ready=$(ConvertTo-CounterBit $integrationReady)',
        "mavg_deformation_backend_execution_rows=2",
        "mavg_deformation_backend_execution_ready_rows=2",
        "mavg_deformation_backend_execution_status=ready",
        "mavg_deformation_backend_execution_bridge_ready=1",
        "mavg_deformation_backend_execution_selected_d3d12_bridge_ready=1",
        "mavg_deformation_backend_execution_selected_vulkan_bridge_ready=1",
        "mavg_deformation_backend_execution_selected_metal_bridge_ready=0",
        "mavg_deformation_native_d3d12_command_execution=0",
        "mavg_deformation_native_vulkan_queue_execution=0",
        "mavg_deformation_topology_changing_rows=0",
        "mavg_deformation_runtime_generated_triangle_topology=0",
        "mavg_deformation_unbounded_vertex_displacement=0",
        "mavg_deformation_native_handles_exposed=0",
        "mavg_deformation_ray_tracing_integration=0",
        "mavg_deformation_mesh_shader_execution=0",
        "mavg_deformation_metal_readiness=0",
        "mavg_deformation_nanite_equivalence=0",
        "mavg_deformation_broad_backend_readiness=0",
        "mavg-deformation-integration-check: ok"
    )) {
    Assert-ContainsText $validatorText $needle "tools/validate-mavg-deformation-integration.ps1"
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
            "mavg-deformation-integration-v1",
            "mavg_deformation_integration.hpp",
            "MavgDeformationIntegrationDesc",
            "MavgDeformationClusterBoundsRow",
            "MavgDeformationBackendExecutionEvidenceDesc",
            "evaluate_mavg_deformation_backend_execution_evidence",
            "plan_mavg_deformation_integrated_clusters",
            "tools/validate-mavg-deformation-integration.ps1",
            "mavg_deformation_policy_ready=1",
            "mavg_deformation_integration_ready=1",
            "mavg_deformation_backend_execution_bridge_ready=1",
            "compute morph/skinned",
            "topology-changing",
            "runtime-generated triangle topology",
            "unbounded vertex displacement",
            "native handles",
            "ray tracing",
            "mesh shader",
            "Metal readiness",
            "Nanite",
            "broad MAVG backend readiness",
            "broad"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) deformation integration policy gate"
    }
    foreach ($forbiddenNeedle in @(
            "mavg_deformation_broad_backend_readiness=1",
            "mavg_deformation_ray_tracing_integration=1",
            "mavg_deformation_mesh_shader_execution=1",
            "mavg_deformation_metal_readiness=1",
            "mavg_deformation_native_d3d12_command_execution=1",
            "mavg_deformation_native_vulkan_queue_execution=1"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden deformation integration broad/native claim"
    }
}

$manifest = $manifestText | ConvertFrom-Json
$runtimeSceneRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_scene_rhi" })
if ($runtimeSceneRhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_scene_rhi module" }
$runtimeSceneRhiManifestText = ((@($runtimeSceneRhiModule[0].publicHeaders) -join " "),
    (@($runtimeSceneRhiModule[0].recentEvidence) -join " "),
    [string]$runtimeSceneRhiModule[0].purpose) -join " "
foreach ($needle in @(
        "mavg_deformation_integration.hpp",
        "MavgDeformationIntegrationDesc",
        "MavgDeformationClusterBoundsRow",
        "MavgDeformationBackendExecutionEvidenceDesc",
        "evaluate_mavg_deformation_backend_execution_evidence",
        "plan_mavg_deformation_integrated_clusters",
        "mavg_deformation_policy_ready=1",
        "mavg_deformation_integration_ready=1",
        "mavg_deformation_backend_execution_bridge_ready=1"
    )) {
    Assert-ContainsText $runtimeSceneRhiManifestText $needle "engine/agent/manifest.json MK_runtime_scene_rhi deformation evidence"
}
