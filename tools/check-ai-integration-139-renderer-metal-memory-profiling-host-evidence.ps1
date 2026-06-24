#requires -Version 7.0
#requires -PSEdition Core
# Chapter 139 for check-ai-integration.ps1 renderer Metal memory/profiling retained host evidence.

$validatorText = Get-AgentSurfaceText "tools/check-renderer-metal-memory-profiling-host-evidence.ps1"
$schemaText = Get-AgentSurfaceText "schemas/renderer-metal-memory-profiling-host-evidence.schema.json"
$fixtureEvidenceText = Get-AgentSurfaceText "tests/fixtures/renderer/metal-memory-profiling-host-evidence/ready/evidence.json"
$fixtureCaptureText = Get-AgentSurfaceText "tests/fixtures/renderer/metal-memory-profiling-host-evidence/ready/capture-summary.txt"
$commandsFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$productionLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$gameGuidanceFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-24-renderer-metal-memory-profiling-host-evidence-v1.md"
$previousPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-24-renderer-backend-parity-apple-memory-profiling-proof-rows-v1.md"
$renderingGuidanceText = Get-AgentSurfaceText ".agents/skills/rendering-change/references/full-guidance.md"
$gameRenderingGuidanceText = Get-AgentSurfaceText ".agents/skills/gameengine-rendering/references/full-guidance.md"
$claudeRenderingGuidanceText = Get-AgentSurfaceText ".claude/skills/gameengine-rendering/references/full-guidance.md"

foreach ($needle in @(
        "validation_recipe=renderer-metal-memory-profiling-host-evidence",
        '$ArtifactRootRelative = "artifacts/renderer/metal-memory-profiling-host-evidence"',
        "schemas/renderer-metal-memory-profiling-host-evidence.schema.json",
        "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1",
        "renderer-metal-memory-profiling-host-evidence-v1",
        "Apple-Metal-MTLHeap-2026-06-24",
        "Apple-Metal-MTLResidencySet-2026-06-24",
        "Apple-Metal-MTLCaptureManager-2026-06-24",
        'renderer_metal_memory_profiling_status=$status',
        'renderer_metal_memory_profiling_ready=$(ConvertTo-CounterBit $ready)',
        'renderer_metal_memory_profiling_retained_apple_host_evidence=$(ConvertTo-CounterBit $ready)',
        'renderer_metal_memory_residency_ready=$(ConvertTo-CounterBit $memoryReady)',
        'renderer_metal_profiling_capture_ready=$(ConvertTo-CounterBit $captureReady)',
        'renderer_metal_memory_profiling_artifact_rows=$artifactRows',
        'renderer_metal_memory_profiling_ready_rows=$readyRows',
        'renderer_metal_memory_profiling_invalid_rows=$invalidRows',
        'renderer_metal_memory_profiling_missing_artifacts=$missingArtifactRows',
        'renderer_metal_memory_profiling_heap_rows=$heapRows',
        'renderer_metal_memory_profiling_residency_set_rows=$residencySetRows',
        'renderer_metal_memory_profiling_residency_commit_rows=$residencyCommitRows',
        'renderer_metal_memory_profiling_pressure_rows=$pressureRows',
        'renderer_metal_memory_profiling_capture_scope_rows=$captureScopeRows',
        'renderer_metal_memory_profiling_capture_artifact_rows=$captureArtifactRows',
        "renderer_backend_parity_ready=0",
        "renderer_metal_broad_readiness=0",
        "renderer_commercial_readiness=0",
        "renderer_broad_quality_ready=0",
        "renderer_environment_ready=0",
        "Renderer Metal memory/profiling readiness is incomplete"
    )) {
    Assert-ContainsText $validatorText $needle "tools/check-renderer-metal-memory-profiling-host-evidence.ps1"
}

foreach ($needle in @(
        "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1",
        "renderer-metal-memory-profiling-host-evidence-v1",
        "Apple-Metal-MTLHeap-2026-06-24",
        "Apple-Metal-MTLResidencySet-2026-06-24",
        "Apple-Metal-MTLCaptureManager-2026-06-24",
        "heap_allocation_ready",
        "residency_set_ready",
        "residency_request_ready",
        "residency_commit_ready",
        "residency_pressure_evidence_ready",
        "capture_manager_ready",
        "capture_descriptor_ready",
        "capture_scope_ready",
        "capture_boundary_ready",
        "capture_artifact_hash_sha256",
        "deterministic_capture_hash_sha256",
        "broad_backend_parity_ready",
        "broad_metal_readiness",
        "commercial_renderer_readiness",
        "broad_renderer_quality",
        "environment_ready"
    )) {
    Assert-ContainsText $schemaText $needle "renderer Metal memory/profiling retained evidence schema"
    Assert-ContainsText $fixtureEvidenceText $needle "renderer Metal memory/profiling ready fixture"
}

foreach ($needle in @(
        "MTLHeap",
        "MTLResidencySet",
        "MTLCaptureManager",
        "MTLCaptureScope",
        "memory_residency",
        "profiling_capture"
    )) {
    Assert-ContainsText $fixtureCaptureText $needle "renderer Metal memory/profiling capture fixture"
}

Assert-ContainsText $commandsFragmentText "rendererMetalMemoryProfilingHostEvidenceCheck" "manifest commands renderer Metal memory/profiling host evidence command"

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $planText; Label = "renderer Metal memory/profiling plan" },
        @{ Text = $previousPlanText; Label = "renderer Apple memory/profiling proof rows plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $productionLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" },
        @{ Text = $gameGuidanceFragmentText; Label = "engine/agent/manifest.fragments/014-gameCodeGuidance.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" },
        @{ Text = $renderingGuidanceText; Label = ".agents/skills/rendering-change/references/full-guidance.md" },
        @{ Text = $gameRenderingGuidanceText; Label = ".agents/skills/gameengine-rendering/references/full-guidance.md" },
        @{ Text = $claudeRenderingGuidanceText; Label = ".claude/skills/gameengine-rendering/references/full-guidance.md" }
    )) {
    foreach ($needle in @(
            "renderer-metal-memory-profiling-host-evidence-v1",
            "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1",
            "tools/check-renderer-metal-memory-profiling-host-evidence.ps1",
            "renderer_metal_memory_profiling_status=host_evidence_required",
            "renderer_metal_memory_profiling_ready=0",
            "memory_residency",
            "profiling_capture",
            "MTLHeap",
            "MTLResidencySet",
            "MTLCaptureManager",
            "capture artifact",
            "broad backend parity",
            "broad Metal readiness",
            "commercial renderer readiness",
            "broad renderer quality"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) renderer Metal memory/profiling host evidence"
    }
    foreach ($forbiddenNeedle in @(
            "renderer_backend_parity_ready=1",
            "renderer_metal_broad_readiness=1",
            "renderer_commercial_readiness=1",
            "renderer_broad_quality_ready=1"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden renderer Metal memory/profiling claim"
    }
}

$manifest = $manifestText | ConvertFrom-Json
$rendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($rendererModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module" }
$rendererManifestText = ((@($rendererModule[0].publicHeaders) -join " "),
    (@($rendererModule[0].recentEvidence) -join " "),
    [string]$rendererModule[0].purpose) -join " "
foreach ($needle in @(
        "BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc",
        "make_backend_renderer_parity_apple_metal_memory_profiling_proofs",
        "renderer-metal-memory-profiling-host-evidence-v1",
        "renderer_metal_memory_profiling_ready=0"
    )) {
    Assert-ContainsText $rendererManifestText $needle "engine/agent/manifest.json MK_renderer Metal memory/profiling host evidence"
}
