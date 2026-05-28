#requires -Version 7.0
#requires -PSEdition Core

# Chapter 9.8 for check-ai-integration.ps1 renderer policy evidence counters.

$retiredSampleGamePackageProof = Test-RetiredSdl3DesktopRuntimeSamplePath "games/sample_desktop_runtime_game/main.cpp"

$gpuMemoryEvidenceSurfaces = @(
    "engine/agent/manifest.json",
    "games/sample_desktop_runtime_game/game.agent.json",
    "games/sample_desktop_runtime_game/README.md",
    "docs/ai-game-development.md",
    "docs/current-capabilities.md",
    ".agents/skills/rendering-change/references/full-guidance.md",
    ".claude/skills/gameengine-rendering/references/full-guidance.md"
)

$gpuMemoryEvidenceNeedles = @(
    "gpu_memory_policy_declared_budget_requests",
    "gpu_memory_policy_residency_pressure_requests",
    "gpu_memory_policy_package_counter_requests",
    "gpu_memory_policy_residency_pressure_events",
    "gpu_memory_policy_memory_budget_evidence_ready=1",
    "gpu_memory_policy_residency_pressure_evidence_ready=1",
    "gpu_memory_policy_package_counter_evidence_ready=1"
)

if (-not $retiredSampleGamePackageProof) {
    foreach ($surfacePath in $gpuMemoryEvidenceSurfaces) {
        $surfaceText = Get-AgentSurfaceText $surfacePath
        foreach ($needle in $gpuMemoryEvidenceNeedles) {
            Assert-ContainsText $surfaceText $needle $surfacePath
        }
    }

    foreach ($needle in @(
            '"gpu_memory_policy_memory_budget_evidence_ready" = "1"',
            '"gpu_memory_policy_residency_pressure_evidence_ready" = "1"',
            '"gpu_memory_policy_package_counter_evidence_ready" = "1"',
            '"gpu_memory_policy_declared_budget_requests"',
            '"gpu_memory_policy_residency_pressure_requests"',
            '"gpu_memory_policy_package_counter_requests"',
            '"gpu_memory_policy_residency_pressure_events"'
        )) {
        Assert-ContainsText (Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1") $needle "tools/validate-installed-desktop-runtime.ps1"
    }
}
else {
    $aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
    Assert-ContainsText $aiGameDevelopmentText "GPU Memory Policy v1 renderer planning" "docs/ai-game-development.md retired GPU memory package evidence"
    Assert-ContainsText $aiGameDevelopmentText "Backend-specific package counters from the deleted SDL3 sample lane are historical" "docs/ai-game-development.md retired GPU memory package evidence"
}

foreach ($needle in @(
        "require_declared_budget_evidence",
        "require_residency_pressure_evidence",
        "require_package_counter_evidence",
        "memory_budget_evidence_ready",
        "residency_pressure_evidence_ready",
        "package_counter_evidence_ready"
    )) {
    Assert-ContainsText (Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/gpu_memory_policy.hpp") $needle "engine/renderer/include/mirakana/renderer/gpu_memory_policy.hpp"
}

$debugProfilingEvidenceSurfaces = @(
    "engine/agent/manifest.json",
    "games/sample_desktop_runtime_game/game.agent.json",
    "games/sample_desktop_runtime_game/README.md",
    "docs/ai-game-development.md",
    "docs/current-capabilities.md",
    ".agents/skills/rendering-change/references/full-guidance.md",
    ".claude/skills/gameengine-rendering/references/full-guidance.md"
)

$debugProfilingEvidenceNeedles = @(
    "debug_profiling_policy_cpu_profile_zones",
    "debug_profiling_policy_trace_capture_handoff_rows",
    "debug_profiling_policy_cpu_profile_zone_requests",
    "debug_profiling_policy_trace_capture_handoff_requests",
    "debug_profiling_policy_package_counter_requests",
    "debug_profiling_policy_cpu_profile_zone_evidence_ready=1",
    "debug_profiling_policy_trace_capture_handoff_evidence_ready=1",
    "debug_profiling_policy_package_counter_evidence_ready=1"
)

if (-not $retiredSampleGamePackageProof) {
    foreach ($surfacePath in $debugProfilingEvidenceSurfaces) {
        $surfaceText = Get-AgentSurfaceText $surfacePath
        foreach ($needle in $debugProfilingEvidenceNeedles) {
            Assert-ContainsText $surfaceText $needle $surfacePath
        }
    }

    foreach ($needle in @(
            '"debug_profiling_policy_cpu_profile_zone_evidence_ready" = "1"',
            '"debug_profiling_policy_trace_capture_handoff_evidence_ready" = "1"',
            '"debug_profiling_policy_package_counter_evidence_ready" = "1"',
            '"debug_profiling_policy_cpu_profile_zones"',
            '"debug_profiling_policy_trace_capture_handoff_rows"',
            '"debug_profiling_policy_cpu_profile_zone_requests"',
            '"debug_profiling_policy_trace_capture_handoff_requests"',
            '"debug_profiling_policy_package_counter_requests"'
        )) {
        Assert-ContainsText (Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1") $needle "tools/validate-installed-desktop-runtime.ps1"
    }
}
else {
    $aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
    Assert-ContainsText $aiGameDevelopmentText "Debug Profiling Policy v1 renderer planning" "docs/ai-game-development.md retired debug profiling package evidence"
    Assert-ContainsText $aiGameDevelopmentText "Backend-specific package counters from the deleted SDL3 sample lane are historical" "docs/ai-game-development.md retired debug profiling package evidence"
}

foreach ($needle in @(
        "require_cpu_profile_zone_evidence",
        "require_trace_capture_handoff_evidence",
        "require_package_counter_evidence",
        "cpu_profile_zone_evidence_ready",
        "trace_capture_handoff_evidence_ready",
        "package_counter_evidence_ready"
    )) {
    Assert-ContainsText (Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/debug_profiling_policy.hpp") $needle "engine/renderer/include/mirakana/renderer/debug_profiling_policy.hpp"
}
