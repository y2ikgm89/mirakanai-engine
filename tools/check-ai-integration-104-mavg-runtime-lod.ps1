#requires -Version 7.0
#requires -PSEdition Core
# Chapter 104 for check-ai-integration.ps1 static contracts.

$runtimeRhiMavgConventionalHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_conventional_upload.hpp"
$runtimeRhiMavgConventionalSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_conventional_upload.cpp"
$runtimeRhiMavgConventionalTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_conventional_upload_tests.cpp"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"

foreach ($needle in @(
        "RuntimeMavgConventionalMeshUploadDesc",
        "RuntimeMavgConventionalMeshBinding",
        "RuntimeMavgConventionalMeshUploadResult",
        "upload_runtime_mavg_conventional_mesh_binding",
        "executed_gpu_culling",
        "executed_indirect_draw",
        "executed_mesh_shader",
        "touched_native_handles"
    )) {
    Assert-ContainsText $runtimeRhiMavgConventionalHeaderText $needle "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_conventional_upload.hpp"
}
foreach ($needle in @(
        "AssetKind::mavg_cluster_graph",
        "upload_runtime_mesh",
        "wait_for_runtime_uploads_on_queue",
        "package-streaming-not-committed",
        "runtime-resource-not-mavg-cluster-graph",
        "mavg-draw-range-outside-payload"
    )) {
    Assert-ContainsText $runtimeRhiMavgConventionalSourceText $needle "engine/runtime_rhi/src/mavg_conventional_upload.cpp"
}
foreach ($needle in @(
        "mavg conventional upload publishes package visible mesh binding for scene lod planning",
        "mavg conventional upload rejects non committed streaming before creating gpu buffers",
        "mavg conventional upload rejects graph draw ranges outside runtime payload"
    )) {
    Assert-ContainsText $runtimeRhiMavgConventionalTestsText $needle "tests/unit/runtime_rhi_mavg_conventional_upload_tests.cpp"
}
foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" }
    )) {
    foreach ($needle in @("mavg_conventional_upload.hpp", "RuntimeMavgConventionalMeshUploadResult", "upload_runtime_mavg_conventional_mesh_binding", "background/page package streaming execution")) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG conventional runtime upload evidence"
    }
}
$runtimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($runtimeRhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module" }
if (@($runtimeRhiModule[0].publicHeaders) -notcontains "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_conventional_upload.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime_rhi publicHeaders missing mavg_conventional_upload.hpp"
}
$manifestText = ((@($runtimeRhiModule[0].recentEvidence) -join " "), [string]$runtimeRhiModule[0].purpose) -join " "
foreach ($needle in @("MAVG Conventional Runtime Upload Evidence v1", "RuntimeMavgConventionalMeshUploadDesc", "RuntimeMavgConventionalMeshUploadResult", "upload_runtime_mavg_conventional_mesh_binding", "AssetKind::mavg_cluster_graph", "background/page streaming execution")) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json MK_runtime_rhi MAVG conventional runtime upload evidence"
}
