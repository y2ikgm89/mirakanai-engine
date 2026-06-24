#requires -Version 7.0
#requires -PSEdition Core
# Chapter 140 for check-ai-integration.ps1 renderer Metal memory/profiling host evidence collector.

$collectorText = Get-AgentSurfaceText "tools/collect-renderer-metal-memory-profiling-host-evidence.ps1"
$collectorCheckText = Get-AgentSurfaceText "tools/check-renderer-metal-memory-profiling-host-evidence-collector.ps1"
$validatorText = Get-AgentSurfaceText "tools/check-renderer-metal-memory-profiling-host-evidence.ps1"
$validateText = Get-AgentSurfaceText "tools/validate.ps1"
$classifierText = Get-AgentSurfaceText "tools/classify-pr-validation-tier.ps1"
$ciMatrixText = Get-AgentSurfaceText "tools/check-ci-matrix.ps1"
$commandsFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$productionLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$gameGuidanceFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$testingText = Get-AgentSurfaceText "docs/testing.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-24-renderer-metal-memory-profiling-host-evidence-collector-v1.md"
$renderingGuidanceText = Get-AgentSurfaceText ".agents/skills/rendering-change/references/full-guidance.md"
$gameRenderingGuidanceText = Get-AgentSurfaceText ".agents/skills/gameengine-rendering/references/full-guidance.md"
$claudeRenderingGuidanceText = Get-AgentSurfaceText ".claude/skills/gameengine-rendering/references/full-guidance.md"

foreach ($needle in @(
        "renderer-metal-memory-profiling-host-evidence-collector-v1",
        "validation_recipe=renderer-metal-memory-profiling-host-evidence",
        'renderer_metal_memory_profiling_host_evidence_collector_mode=$Mode',
        "renderer_metal_memory_profiling_host_evidence_collector_plan_ready=1",
        'renderer_metal_memory_profiling_host_evidence_collector_writes_evidence=$(ConvertTo-CounterBit $willWrite)',
        'renderer_metal_memory_profiling_host_evidence_collector_written=$(ConvertTo-CounterBit',
        'renderer_metal_memory_profiling_host_evidence_collector_capture_artifact_hash=$captureArtifactHash',
        "renderer_metal_memory_profiling_host_evidence_collector_native_handles_exposed=0",
        "renderer_metal_memory_profiling_host_evidence_collector_external_engine_api_parity=0",
        "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1",
        "renderer-metal-memory-profiling-host-evidence-v1",
        "Apple-Metal-MTLHeap-2026-06-24",
        "Apple-Metal-MTLResidencySet-2026-06-24",
        "Apple-Metal-MTLCaptureManager-2026-06-24",
        "MTLHeap",
        "MTLResidencySet",
        "MTLCaptureManager",
        "MTLCaptureScope",
        "memory_residency",
        "profiling_capture",
        "capture_artifact_hash_sha256",
        'broad_backend_parity_ready = $false',
        'broad_metal_readiness = $false',
        'commercial_renderer_readiness = $false',
        'broad_renderer_quality = $false',
        'external_engine_api_parity = $false'
    )) {
    Assert-ContainsText $collectorText $needle "tools/collect-renderer-metal-memory-profiling-host-evidence.ps1"
}

foreach ($needle in @(
        "collect-renderer-metal-memory-profiling-host-evidence.ps1",
        "check-renderer-metal-memory-profiling-host-evidence.ps1",
        "renderer_metal_memory_profiling_host_evidence_collector_mode=Plan",
        "renderer_metal_memory_profiling_host_evidence_collector_mode=Import",
        "renderer_metal_memory_profiling_host_evidence_collector_written=1",
        "renderer_metal_memory_profiling_status=ready",
        "renderer_metal_memory_profiling_ready=1",
        "renderer_backend_parity_ready=0",
        "renderer_metal_broad_readiness=0",
        "renderer_commercial_readiness=0",
        "renderer_broad_quality_ready=0"
    )) {
    Assert-ContainsText $collectorCheckText $needle "tools/check-renderer-metal-memory-profiling-host-evidence-collector.ps1"
}

foreach ($needle in @(
        "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1",
        "renderer-metal-memory-profiling-host-evidence-v1",
        "capture_artifact_hash_sha256"
    )) {
    Assert-ContainsText $validatorText $needle "tools/check-renderer-metal-memory-profiling-host-evidence.ps1"
}

Assert-ContainsText $validateText "check-renderer-metal-memory-profiling-host-evidence-collector.ps1" "tools/validate.ps1"
foreach ($needle in @(
        "collect-renderer-metal-memory-profiling-host-evidence",
        "check-renderer-metal-memory-profiling-host-evidence",
        "check-renderer-metal-memory-profiling-host-evidence-collector"
    )) {
    Assert-ContainsText $classifierText $needle "tools/classify-pr-validation-tier.ps1"
}
Assert-ContainsText $ciMatrixText "tools/collect-renderer-metal-memory-profiling-host-evidence.ps1" "tools/check-ci-matrix.ps1"
Assert-ContainsText $commandsFragmentText "rendererMetalMemoryProfilingHostEvidenceCollectorCheck" "manifest commands renderer Metal memory/profiling collector command"

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $testingText; Label = "docs/testing.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $planText; Label = "renderer Metal memory/profiling collector plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $productionLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" },
        @{ Text = $gameGuidanceFragmentText; Label = "engine/agent/manifest.fragments/014-gameCodeGuidance.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" },
        @{ Text = $renderingGuidanceText; Label = ".agents/skills/rendering-change/references/full-guidance.md" },
        @{ Text = $gameRenderingGuidanceText; Label = ".agents/skills/gameengine-rendering/references/full-guidance.md" },
        @{ Text = $claudeRenderingGuidanceText; Label = ".claude/skills/gameengine-rendering/references/full-guidance.md" }
    )) {
    foreach ($needle in @(
            "renderer-metal-memory-profiling-host-evidence-collector-v1",
            "tools/collect-renderer-metal-memory-profiling-host-evidence.ps1",
            "tools/check-renderer-metal-memory-profiling-host-evidence-collector.ps1",
            "host-owned",
            "evidence.json",
            "MTLHeap",
            "MTLResidencySet",
            "MTLCaptureManager",
            "capture artifact",
            "broad backend parity",
            "broad Metal readiness",
            "commercial renderer readiness",
            "broad renderer quality"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) renderer Metal memory/profiling collector"
    }
    foreach ($forbiddenNeedle in @(
            "renderer_backend_parity_ready=1",
            "renderer_metal_broad_readiness=1",
            "renderer_commercial_readiness=1",
            "renderer_broad_quality_ready=1"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden renderer Metal memory/profiling collector claim"
    }
}
