#requires -Version 7.0
#requires -PSEdition Core
# Chapter 141 for renderer Metal memory/profiling Apple-host artifact production.

$producerText = Get-AgentSurfaceText "tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1"
$cmakeText = Get-AgentSurfaceText "CMakeLists.txt"
$probeText = Get-AgentSurfaceText "tests/fixtures/metal_memory_profiling_host_artifacts_probe.mm"
$collectorText = Get-AgentSurfaceText "tools/collect-renderer-metal-memory-profiling-host-evidence.ps1"
$validatorText = Get-AgentSurfaceText "tools/check-renderer-metal-memory-profiling-host-evidence.ps1"
$validateText = Get-AgentSurfaceText "tools/validate.ps1"
$classifierText = Get-AgentSurfaceText "tools/classify-pr-validation-tier.ps1"
$ciMatrixText = Get-AgentSurfaceText "tools/check-ci-matrix.ps1"
$workflowText = Get-AgentSurfaceText ".github/workflows/validate.yml"
$commandsFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$recipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$testingText = Get-AgentSurfaceText "docs/testing.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-24-renderer-metal-memory-profiling-apple-host-artifacts-v1.md"
$renderingGuidanceText = Get-AgentSurfaceText ".agents/skills/rendering-change/references/full-guidance.md"
$gameRenderingGuidanceText = Get-AgentSurfaceText ".agents/skills/gameengine-rendering/references/full-guidance.md"
$claudeRenderingGuidanceText = Get-AgentSurfaceText ".claude/skills/gameengine-rendering/references/full-guidance.md"

foreach ($needle in @(
        "renderer-metal-memory-profiling-apple-host-artifacts-v1",
        "validation_recipe=renderer-metal-memory-profiling-host-evidence",
        "renderer_metal_memory_profiling_host_artifacts_status=host_gated",
        "renderer_metal_memory_profiling_host_gate_reason",
        "renderer_metal_memory_profiling_host_gate_probe_exit_code",
        "renderer_metal_memory_profiling_host_gate_macos_version",
        "renderer_metal_memory_profiling_host_gate_xcode_version",
        "renderer_metal_memory_profiling_host_artifacts_ready=0",
        "renderer_metal_memory_profiling_host_artifacts_ready=1",
        "renderer_metal_memory_profiling_status=ready",
        "renderer_metal_memory_profiling_ready=1",
        "renderer_backend_parity_ready=0",
        "renderer_metal_broad_readiness=0",
        "renderer_commercial_readiness=0",
        "renderer_broad_quality_ready=0",
        "LASTEXITCODE = 0",
        "collect-renderer-metal-memory-profiling-host-evidence.ps1",
        "check-renderer-metal-memory-profiling-host-evidence.ps1",
        "MK_metal_memory_profiling_host_artifacts_probe",
        "MTLHeap",
        "MTLResidencySet",
        "MTLCaptureManager",
        "MTLCaptureDescriptor",
        "MTLCaptureScope"
    )) {
    Assert-ContainsText $producerText $needle "tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1"
}

foreach ($needle in @(
        "MK_metal_memory_profiling_host_artifacts_probe",
        "metal_memory_profiling_host_artifacts_probe.mm",
        "Foundation",
        "Metal",
        "OBJCXX",
        "-fobjc-arc"
    )) {
    Assert-ContainsText $cmakeText $needle "CMakeLists.txt renderer Metal memory/profiling host artifact probe"
}

foreach ($needle in @(
        "renderer-metal-memory-profiling-apple-host-artifacts-v1",
        "MTLCreateSystemDefaultDevice",
        "MTLHeapDescriptor",
        "MTLResidencySetDescriptor",
        "residency_descriptor.initialCapacity = 2",
        "commit",
        "requestResidency",
        "addResidencySet",
        "MTLCaptureManager",
        "MTLCaptureDescriptor",
        "MTLCaptureScope",
        "capture.gputrace",
        "probe-summary.json",
        "native_handles_exposed=0"
    )) {
    Assert-ContainsText $probeText $needle "tests/fixtures/metal_memory_profiling_host_artifacts_probe.mm"
}

foreach ($needle in @(
        "renderer-metal-memory-profiling-apple-host-artifacts-v1",
        "generate-renderer-metal-memory-profiling-host-artifacts.ps1"
    )) {
    Assert-ContainsText $validateText $needle "tools/validate.ps1"
}

foreach ($needle in @(
        "generate-renderer-metal-memory-profiling-host-artifacts",
        "check-renderer-metal-memory-profiling-host-evidence",
        "collect-renderer-metal-memory-profiling-host-evidence"
    )) {
    Assert-ContainsText $classifierText $needle "tools/classify-pr-validation-tier.ps1"
}

Assert-ContainsText $ciMatrixText "tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1" "tools/check-ci-matrix.ps1 renderer Metal host artifact case"
Assert-ContainsText $workflowText "GitHub-hosted macOS can be Metal-capable while still rejecting MTLResidencySet creation." ".github/workflows/validate.yml renderer Metal host artifact lane"
Assert-ContainsText $workflowText "generate-renderer-metal-memory-profiling-host-artifacts.ps1 -Jobs `$jobs" ".github/workflows/validate.yml renderer Metal host artifact lane"
Assert-DoesNotContainText $workflowText "generate-renderer-metal-memory-profiling-host-artifacts.ps1 -Jobs `$jobs -RequireReady" ".github/workflows/validate.yml renderer Metal host artifact hosted lane"
Assert-ContainsText $workflowText "artifacts/renderer/metal-memory-profiling-host-evidence/**" ".github/workflows/validate.yml renderer Metal host artifact upload"
Assert-ContainsText $commandsFragmentText "rendererMetalMemoryProfilingHostArtifactsCheck" "manifest commands renderer Metal memory/profiling host artifacts"

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $testingText; Label = "docs/testing.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $planText; Label = "renderer Metal memory/profiling Apple-host artifacts plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $recipesFragmentText; Label = "engine/agent/manifest.fragments/009-validationRecipes.json" },
        @{ Text = $productionLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" },
        @{ Text = $renderingGuidanceText; Label = ".agents/skills/rendering-change/references/full-guidance.md" },
        @{ Text = $gameRenderingGuidanceText; Label = ".agents/skills/gameengine-rendering/references/full-guidance.md" },
        @{ Text = $claudeRenderingGuidanceText; Label = ".claude/skills/gameengine-rendering/references/full-guidance.md" }
    )) {
    foreach ($needle in @(
            "renderer-metal-memory-profiling-apple-host-artifacts-v1",
            "tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1",
            "MTLHeap",
            "MTLResidencySet",
            "MTLCaptureManager",
            "MTLCaptureScope",
            "Apple-host",
            "renderer_metal_memory_profiling_ready=1",
            "broad backend parity",
            "broad Metal readiness",
            "commercial renderer readiness",
            "broad renderer quality"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) renderer Metal memory/profiling Apple-host artifacts"
    }
    foreach ($forbiddenNeedle in @(
            "renderer_backend_parity_ready=1",
            "renderer_metal_broad_readiness=1",
            "renderer_commercial_readiness=1",
            "renderer_broad_quality_ready=1"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden renderer Metal memory/profiling Apple-host artifacts claim"
    }
}

foreach ($needle in @(
        "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1",
        "renderer-metal-memory-profiling-host-evidence-v1",
        "capture_artifact_hash_sha256"
    )) {
    Assert-ContainsText $collectorText $needle "collector retained evidence contract"
    Assert-ContainsText $validatorText $needle "validator retained evidence contract"
}
