#requires -Version 7.0
#requires -PSEdition Core

# Chapter 151 for check-ai-integration.ps1 static contracts.
# 2D Commercial Official Profiler Artifact Collectors v1 plan/manifest/script alignment.

$validatorScript = Get-AgentSurfaceText "tools/validate-2d-commercial-profiler-artifact-collectors.ps1"
$validationRecipesManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$composedManifest = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilities = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmap = Get-AgentSurfaceText "docs/roadmap.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"

foreach ($needle in @(
        "GameEngine.2DCommercialProfilerArtifactCollector.v1",
        "2d-commercial-profiler-artifact-collectors-v1",
        "tools/validate-2d-commercial-profiler-artifact-collectors.ps1 -SyntheticSmoke",
        "profiler_artifact_collector_status=ready",
        "profiler_artifact_collector_required_tool_rows=5",
        "profiler_artifact_collector_ready_rows=5",
        "profiler_artifact_collector_retained_artifact_rows=5",
        "profiler_artifact_collector_hash_mismatch_rows=0",
        "profiler_artifact_collector_external_engine_claim_rows=0",
        "profiler_artifact_collector_legal_approval_claim_rows=0",
        "windows-wpr-wpa",
        "pix-on-windows",
        "vulkan-timestamp-debug-utils",
        "apple-xctrace-metal-capture",
        "linux-perf"
    )) {
    Assert-ContainsText $validatorScript $needle "tools/validate-2d-commercial-profiler-artifact-collectors.ps1"
}

foreach ($needle in @(
        "2d-commercial-profiler-artifact-collectors",
        "tools/validate-2d-commercial-profiler-artifact-collectors.ps1 -SyntheticSmoke",
        "GameEngine.2DCommercialProfilerArtifactCollector.v1",
        "zero external-engine compatibility",
        "zero legal-approval claims"
    )) {
    Assert-ContainsText $validationRecipesManifest $needle "engine/agent/manifest.fragments/009-validationRecipes.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json validation recipes"
}

foreach ($needle in @(
        "twoDCommercialProfilerArtifactCollectorsPhase8Evidence",
        "2d-commercial-profiler-artifact-collectors-v1",
        "Windows WPR/WPA, PIX on Windows, Vulkan timestamp/debug-utils, Apple xctrace/Metal capture, and Linux perf",
        "real retained profiler captures remain host-owned"
    )) {
    Assert-ContainsText $productionLoopManifest $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json production loop"
}

foreach ($needle in @(
        "2D Commercial Official Profiler Artifact Collectors v1",
        "tools/validate-2d-commercial-profiler-artifact-collectors.ps1",
        "https://learn.microsoft.com/en-us/windows-hardware/test/wpt/",
        "https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/general/pix-overview",
        "https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html",
        "https://developer.apple.com/documentation/xcode/recording-performance-data",
        "https://perf.wiki.kernel.org/index.php/Main_Page",
        "Unity/Unreal/Godot compatibility"
    )) {
    Assert-ContainsText ($currentCapabilities + "`n" + $roadmap + "`n" + $planText) $needle `
        "2d commercial profiler artifact collector docs and plan"
}
