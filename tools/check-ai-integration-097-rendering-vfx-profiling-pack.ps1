#requires -Version 7.0
#requires -PSEdition Core

# Chapter 9.7 for check-ai-integration.ps1 Rendering VFX Profiling Pack production contracts.

$rendererHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/production_vfx_profiling.hpp"
$rendererSourceText = Get-AgentSurfaceText "engine/renderer/src/production_vfx_profiling.cpp"
$rendererCMakeText = Get-AgentSurfaceText "engine/renderer/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$rendererTestsText = Get-AgentSurfaceText "tests/unit/renderer_production_vfx_profiling_tests.cpp"
$sample3dMainText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/main.cpp"
$sample3dManifestText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$installedValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-25-general-purpose-game-production-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$generatedValidationText = Get-AgentSurfaceText "docs/specs/generated-game-validation-scenarios.md"
$aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
$sample3dReadmeText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/README.md"
$backlogText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
$projectionText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "RendererProductionVfxFeatureRow",
        "RendererProductionGpuParticleBudgetRow",
        "RendererProductionPostprocessRow",
        "RendererProductionBackendTimingRow",
        "RendererProductionBackendEvidenceRow",
        "RendererProductionCpuProfileRow",
        "RendererProductionPackageCounterRow",
        "RendererProductionCrashTelemetryHandoffRow",
        "RendererProductionVfxProfilingPlan",
        "RendererProductionVfxProfilingDiagnosticCode",
        "plan_renderer_production_vfx_profiling"
    )) {
    Assert-ContainsText $rendererHeaderText $needle "engine/renderer/include/mirakana/renderer/production_vfx_profiling.hpp"
}

foreach ($needle in @(
        "bool requires_metal_host_evidence{false}",
        "bool has_metal_host_evidence{false}",
        "std::size_t cpu_profile_row_count{0U}",
        "std::size_t package_counter_row_count{0U}",
        "std::size_t package_counter_ready_count{0U}",
        "std::size_t package_counter_host_gated_count{0U}",
        "bool invoked_gpu_commands{false}",
        "bool invoked_native_capture{false}",
        "bool invoked_crash_upload{false}"
    )) {
    Assert-ContainsText $rendererHeaderText $needle "engine/renderer/include/mirakana/renderer/production_vfx_profiling.hpp"
}

foreach ($needle in @(
        "RendererProductionVfxProfilingDiagnosticCode::missing_backend_parity",
        "RendererProductionVfxProfilingDiagnosticCode::missing_backend_timing",
        "RendererProductionVfxProfilingDiagnosticCode::missing_backend_synchronization_evidence",
        "RendererProductionVfxProfilingDiagnosticCode::missing_backend_shader_validation",
        "RendererProductionVfxProfilingDiagnosticCode::missing_backend_validation_evidence",
        "RendererProductionVfxProfilingDiagnosticCode::missing_backend_host_evidence",
        "RendererProductionVfxProfilingDiagnosticCode::missing_backend_capture_handoff",
        "RendererProductionVfxProfilingDiagnosticCode::missing_cpu_profile_rows",
        "RendererProductionVfxProfilingDiagnosticCode::missing_package_counter_rows",
        "RendererProductionVfxProfilingDiagnosticCode::broad_performance_claim",
        "RendererProductionVfxProfilingDiagnosticCode::unsupported_native_handle_claim",
        "RendererProductionVfxProfilingDiagnosticCode::unsupported_crash_upload",
        "RendererProductionVfxProfilingStatus::host_evidence_required",
        "compute_host_evidence",
        "compute_replay_hash"
    )) {
    Assert-ContainsText $rendererSourceText $needle "engine/renderer/src/production_vfx_profiling.cpp"
}

Assert-ContainsText $rendererCMakeText "src/production_vfx_profiling.cpp" "engine/renderer/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_renderer_production_vfx_profiling_tests" "CMakeLists.txt"
Assert-ContainsText $rendererTestsText "production renderer VFX profiling keeps Metal host evidence gated" "tests/unit/renderer_production_vfx_profiling_tests.cpp"
Assert-ContainsText $rendererTestsText "production renderer VFX profiling is ready with per-backend host timing evidence" "tests/unit/renderer_production_vfx_profiling_tests.cpp"
Assert-ContainsText $rendererTestsText "production renderer VFX profiling rejects cross-backend proof transfer" "tests/unit/renderer_production_vfx_profiling_tests.cpp"
Assert-ContainsText $rendererTestsText "production renderer VFX profiling rejects broad readiness without cpu profile package counter and capture" "tests/unit/renderer_production_vfx_profiling_tests.cpp"
Assert-ContainsText $rendererTestsText "production renderer VFX profiling rejects unsafe rows and broad claims" "tests/unit/renderer_production_vfx_profiling_tests.cpp"
Assert-ContainsText $rendererTestsText "production renderer VFX profiling reports no rows without backend claims" "tests/unit/renderer_production_vfx_profiling_tests.cpp"

foreach ($needle in @(
        "mirakana/renderer/production_vfx_profiling.hpp",
        "plan_renderer_production_vfx_profiling",
        "--require-rendering-vfx-profiling",
        "rendering_vfx_profiling_status=",
        "rendering_vfx_profiling_reviewed=",
        "rendering_vfx_profiling_ready=",
        "rendering_vfx_profiling_feature_rows=",
        "rendering_vfx_profiling_gpu_particle_budget_rows=",
        "rendering_vfx_profiling_postprocess_rows=",
        "rendering_vfx_profiling_backend_timing_rows=",
        "rendering_vfx_profiling_backend_evidence_rows=",
        "rendering_vfx_profiling_backend_evidence_ready=",
        "rendering_vfx_profiling_backend_evidence_host_gated=",
        "rendering_vfx_profiling_cpu_profile_rows=",
        "rendering_vfx_profiling_package_counter_rows=",
        "rendering_vfx_profiling_package_counter_ready=",
        "rendering_vfx_profiling_package_counter_host_gated=",
        "rendering_vfx_profiling_crash_telemetry_handoff_rows=",
        "rendering_vfx_profiling_replay_hash=",
        "rendering_vfx_profiling_d3d12_host_evidence_ready=",
        "rendering_vfx_profiling_vulkan_strict_host_evidence_ready=",
        "rendering_vfx_profiling_metal_host_evidence_ready=",
        "rendering_vfx_profiling_requires_metal_host_evidence=",
        "rendering_vfx_profiling_metal_host_evidence=",
        "rendering_vfx_profiling_invoked_gpu_commands=",
        "rendering_vfx_profiling_invoked_native_capture=",
        "rendering_vfx_profiling_invoked_crash_upload=",
        "rendering_vfx_profiling_debug_policy_ready=",
        "rendering_vfx_profiling_debug_cpu_profile_zones=",
        "rendering_vfx_profiling_debug_trace_capture_handoff_rows=",
        "rendering_vfx_profiling_debug_package_counter_requests=",
        "rendering_vfx_profiling_debug_cpu_profile_zone_evidence_ready=",
        "rendering_vfx_profiling_debug_trace_capture_handoff_evidence_ready=",
        "rendering_vfx_profiling_debug_package_counter_evidence_ready=",
        "rendering_vfx_profiling_memory_policy_ready=",
        "rendering_vfx_profiling_memory_residency_pressure_events=",
        "rendering_vfx_profiling_memory_declared_budget_requests=",
        "rendering_vfx_profiling_memory_package_counter_requests=",
        "rendering_vfx_profiling_memory_budget_evidence_ready=",
        "rendering_vfx_profiling_memory_residency_pressure_evidence_ready=",
        "rendering_vfx_profiling_memory_package_counter_evidence_ready=",
        "rendering_vfx_profiling_diagnostics="
    )) {
    Assert-ContainsText $sample3dMainText $needle "games/sample_generated_desktop_runtime_3d_package/main.cpp"
}

foreach ($needle in @(
        '"rendering-vfx-profiling"',
        "rendering_vfx_profiling_status=host_evidence_required",
        "rendering_vfx_profiling_reviewed=1",
        "rendering_vfx_profiling_ready=0",
        "rendering_vfx_profiling_diagnostics=0",
        "debug CPU profile zone",
        "memory budget",
        "D3D12 and strict Vulkan host evidence ready",
        "--require-native-ui-overlay",
        "--require-visible-3d-production-proof",
        "--require-scene-collision-package",
        "--require-native-ui-textured-sprite-atlas",
        "--require-rendering-vfx-profiling"
    )) {
    Assert-ContainsText $sample3dManifestText $needle "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
}

foreach ($needle in @(
        "requiresRenderingVfxProfiling",
        "rendering_vfx_profiling_status",
        "rendering_vfx_profiling_reviewed",
        "rendering_vfx_profiling_ready",
        "rendering_vfx_profiling_feature_rows",
        "rendering_vfx_profiling_gpu_particle_budget_rows",
        "rendering_vfx_profiling_postprocess_rows",
        "rendering_vfx_profiling_backend_timing_rows",
        "rendering_vfx_profiling_backend_evidence_rows",
        "rendering_vfx_profiling_backend_evidence_ready",
        "rendering_vfx_profiling_backend_evidence_host_gated",
        "rendering_vfx_profiling_cpu_profile_rows",
        "rendering_vfx_profiling_package_counter_rows",
        "rendering_vfx_profiling_package_counter_ready",
        "rendering_vfx_profiling_package_counter_host_gated",
        "rendering_vfx_profiling_crash_telemetry_handoff_rows",
        "rendering_vfx_profiling_host_validated_backends",
        "rendering_vfx_profiling_rejected_unsafe_rows",
        "rendering_vfx_profiling_replay_hash",
        "rendering_vfx_profiling_d3d12_host_evidence_ready",
        "rendering_vfx_profiling_vulkan_strict_host_evidence_ready",
        "rendering_vfx_profiling_metal_host_evidence_ready",
        "rendering_vfx_profiling_requires_metal_host_evidence",
        "rendering_vfx_profiling_metal_host_evidence",
        "rendering_vfx_profiling_invoked_gpu_commands",
        "rendering_vfx_profiling_invoked_native_capture",
        "rendering_vfx_profiling_invoked_crash_upload",
        "rendering_vfx_profiling_debug_policy_ready",
        "rendering_vfx_profiling_debug_cpu_profile_zone_evidence_ready",
        "rendering_vfx_profiling_debug_trace_capture_handoff_evidence_ready",
        "rendering_vfx_profiling_debug_package_counter_evidence_ready",
        "rendering_vfx_profiling_memory_policy_ready",
        "rendering_vfx_profiling_memory_budget_evidence_ready",
        "rendering_vfx_profiling_memory_residency_pressure_evidence_ready",
        "rendering_vfx_profiling_memory_package_counter_evidence_ready",
        "rendering_vfx_profiling_diagnostics"
    )) {
    Assert-ContainsText $installedValidationText $needle "tools/validate-installed-desktop-runtime.ps1"
}
Assert-ContainsText $installedValidationText '"rendering_vfx_profiling_feature_rows" = "3"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"rendering_vfx_profiling_cpu_profile_rows" = "3"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"rendering_vfx_profiling_package_counter_rows" = "3"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"rendering_vfx_profiling_package_counter_ready" = "2"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"rendering_vfx_profiling_host_validated_backends" = "2"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"rendering_vfx_profiling_vulkan_strict_host_evidence_ready" = "1"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"rendering_vfx_profiling_debug_policy_ready" = "1"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"rendering_vfx_profiling_memory_policy_ready" = "1"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"rendering_vfx_profiling_debug_cpu_profile_zone_evidence_ready" = "1"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"rendering_vfx_profiling_memory_residency_pressure_evidence_ready" = "1"' "tools/validate-installed-desktop-runtime.ps1"

foreach ($docSurface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-05-25-general-purpose-game-production-v1.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $generatedValidationText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $sample3dReadmeText; Label = "games/sample_generated_desktop_runtime_3d_package/README.md" }
    )) {
    Assert-ContainsText $docSurface.Text "plan_renderer_production_vfx_profiling" $docSurface.Label
    Assert-ContainsText $docSurface.Text "rendering_vfx_profiling_reviewed=1" $docSurface.Label
    Assert-ContainsText $docSurface.Text "rendering_vfx_profiling_ready=0" $docSurface.Label
    Assert-ContainsText $docSurface.Text "D3D12 and strict Vulkan host evidence" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $sample3dReadmeText; Label = "games/sample_generated_desktop_runtime_3d_package/README.md" },
        @{ Text = $backlogText; Label = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md" },
        @{ Text = $projectionText; Label = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "CPU profile" $docSurface.Label
    Assert-ContainsText $docSurface.Text "residency pressure" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $backlogText; Label = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md" },
        @{ Text = $projectionText; Label = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "production-rendering-vfx-profiling-v1" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_renderer_production_vfx_profiling" $docSurface.Label
}

foreach ($needle in @(
        '"unsupportedProductionGaps": []',
        "engine/renderer/include/mirakana/renderer/production_vfx_profiling.hpp",
        "RendererProductionBackendEvidenceRow",
        "RendererProductionCpuProfileRow",
        "RendererProductionPackageCounterRow",
        "RendererProductionVfxProfilingPlan",
        "plan_renderer_production_vfx_profiling",
        "rendering_vfx_profiling_*",
        "rendering_vfx_profiling_debug_policy_ready",
        "rendering_vfx_profiling_memory_policy_ready",
        "currentRendererProductionVfxProfiling"
    )) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}
if ([string]$productionLoop.recommendedNextPlan.id -ne "environment-highest-commercial-readiness-v1") {
    Assert-ContainsText $manifestText "production-rendering-vfx-profiling-v1" "engine/agent/manifest.json"
}

foreach ($needle in @(
        "Production Rendering VFX Profiling v1",
        "RendererProductionBackendEvidenceRow",
        "RendererProductionCpuProfileRow",
        "RendererProductionPackageCounterRow",
        "D3D12 and strict Vulkan host evidence ready",
        "Metal host evidence absent",
        "trace handoff",
        "residency pressure"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json"
}
