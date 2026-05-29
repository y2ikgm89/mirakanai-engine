#requires -Version 7.0
#requires -PSEdition Core

# Chapter 9.8 for check-ai-integration.ps1 Renderer Quality Matrix production contracts.

$rendererHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/renderer_quality_matrix.hpp"
$rendererSourceText = Get-AgentSurfaceText "engine/renderer/src/renderer_quality_matrix.cpp"
$rendererCMakeText = Get-AgentSurfaceText "engine/renderer/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$rendererTestsText = Get-AgentSurfaceText "tests/unit/renderer_quality_matrix_tests.cpp"
$sample3dMainText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/main.cpp"
$sample3dManifestText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$sample3dReadmeText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/README.md"
$gamesCMakeText = Get-AgentSurfaceText "games/CMakeLists.txt"
$installedValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
$phasePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md"
$activeRendererPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-27-renderer-production-quality-backend-parity-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
$generatedValidationText = Get-AgentSurfaceText "docs/specs/generated-game-validation-scenarios.md"
$backlogText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
$projectionText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md"
$codexRenderingGuidanceText = Get-AgentSurfaceText ".agents/skills/rendering-change/references/full-guidance.md"
$claudeRenderingGuidanceText = Get-AgentSurfaceText ".claude/skills/gameengine-rendering/references/full-guidance.md"
$codexGameSkillText = Get-AgentSurfaceText ".agents/skills/gameengine-game-development/SKILL.md"
$claudeGameSkillText = Get-AgentSurfaceText ".claude/skills/gameengine-game-development/SKILL.md"
$codexGameGuidanceText = Get-AgentSurfaceText ".agents/skills/gameengine-game-development/references/full-guidance.md"
$claudeGameGuidanceText = Get-AgentSurfaceText ".claude/skills/gameengine-game-development/references/full-guidance.md"
$cursorGameSkillText = Get-AgentSurfaceText ".cursor/skills/gameengine-game-development/SKILL.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$runtimeFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
$packagingFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/008-packagingTargets.json"
$gameGuidanceFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "RendererQualityMatrixStatus",
        "RendererQualityFeatureKind",
        "RendererQualityProofKind",
        "RendererQualityMatrixRowStatus",
        "RendererQualityMatrixDiagnosticCode",
        "RendererQualityMatrixRow",
        "RendererQualityMatrixRequest",
        "RendererQualityMatrixPlan",
        "plan_renderer_quality_matrix"
    )) {
    Assert-ContainsText $rendererHeaderText $needle "engine/renderer/include/mirakana/renderer/renderer_quality_matrix.hpp"
}

foreach ($needle in @(
        "bool d3d12_resource_state_barrier_evidence{false}",
        "bool d3d12_fence_evidence{false}",
        "bool vulkan_synchronization2_evidence{false}",
        "bool vulkan_layout_transition_evidence{false}",
        "bool vulkan_validation_layer_evidence{false}",
        "bool vulkan_spirv_validation_evidence{false}",
        "bool metal_resource_synchronization_evidence{false}",
        "bool metal_feature_set_evidence{false}",
        "RendererQualityMatrixRowStatus status{RendererQualityMatrixRowStatus::ready}",
        "bool dependency_gate_required{false}",
        "std::size_t dependency_gated_row_count{0U}",
        "std::size_t unsupported_row_count{0U}",
        "bool general_renderer_quality_ready{false}",
        "bool invoked_gpu_commands{false}",
        "bool invoked_native_capture{false}",
        "bool invoked_crash_upload{false}"
    )) {
    Assert-ContainsText $rendererHeaderText $needle "engine/renderer/include/mirakana/renderer/renderer_quality_matrix.hpp"
}

foreach ($needle in @(
        "RendererQualityMatrixDiagnosticCode::missing_resource_synchronization_evidence",
        "RendererQualityMatrixDiagnosticCode::missing_backend_validation_evidence",
        "RendererQualityMatrixDiagnosticCode::missing_shader_tool_validation_evidence",
        "RendererQualityMatrixDiagnosticCode::missing_package_counter_evidence",
        "RendererQualityMatrixDiagnosticCode::missing_timing_budget_evidence",
        "RendererQualityMatrixDiagnosticCode::missing_gpu_memory_evidence",
        "RendererQualityMatrixDiagnosticCode::missing_backend_parity_evidence",
        "RendererQualityMatrixDiagnosticCode::unsupported_native_handle_claim",
        "RendererQualityMatrixDiagnosticCode::unsupported_capture_execution",
        "RendererQualityMatrixDiagnosticCode::unsupported_crash_upload_execution",
        "RendererQualityMatrixDiagnosticCode::unsupported_inferred_backend_parity",
        "RendererQualityMatrixDiagnosticCode::unsupported_subjective_visual_quality_claim",
        "RendererQualityMatrixStatus::host_evidence_required",
        "RendererQualityMatrixStatus::dependency_evidence_required",
        "RendererQualityMatrixStatus::unsupported",
        "RendererQualityMatrixRowStatus::dependency_gated",
        "RendererQualityMatrixRowStatus::unsupported",
        "resource_synchronization_ready",
        "backend_validation_ready",
        "validate_row_status",
        "compute_readiness",
        "compute_replay_hash"
    )) {
    Assert-ContainsText $rendererSourceText $needle "engine/renderer/src/renderer_quality_matrix.cpp"
}

Assert-ContainsText $rendererCMakeText "src/renderer_quality_matrix.cpp" "engine/renderer/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_renderer_quality_matrix_tests" "CMakeLists.txt"

foreach ($needle in @(
        "renderer quality matrix keeps general production claim host gated by Metal",
        "renderer quality matrix is ready when every backend has local host evidence",
        "renderer quality matrix rejects missing D3D12 barrier and fence evidence",
        "renderer quality matrix rejects missing strict Vulkan synchronization validation and SPIR-V evidence",
        "renderer quality matrix reports dependency-gated rows without accepting broad readiness",
        "renderer quality matrix reports unsupported rows without accepting broad readiness",
        "renderer quality matrix rejects mismatched explicit row taxonomy",
        "renderer quality matrix rejects unsafe side effects and subjective quality claims",
        "renderer quality matrix rejects native handle tokens in ids and counters",
        "renderer quality matrix rejects missing backend feature rows and duplicate backend rows",
        "renderer quality matrix reports no rows without backend claims"
    )) {
    Assert-ContainsText $rendererTestsText $needle "tests/unit/renderer_quality_matrix_tests.cpp"
}

foreach ($needle in @(
        "mirakana/renderer/renderer_quality_matrix.hpp",
        "RendererQualityMatrixProbeResult",
        "plan_renderer_quality_matrix",
        "validate_renderer_quality_matrix_package_evidence",
        "--require-renderer-quality-matrix",
        "renderer_quality_matrix_status=",
        "renderer_quality_matrix_reviewed=",
        "renderer_quality_matrix_ready=",
        "renderer_quality_matrix_rows=",
        "renderer_quality_matrix_ready_rows=",
        "renderer_quality_matrix_host_gated_rows=",
        "renderer_quality_matrix_dependency_gated_rows=",
        "renderer_quality_matrix_unsupported_rows=",
        "renderer_quality_matrix_host_validated_backends=",
        "renderer_quality_matrix_replay_hash=",
        "renderer_quality_matrix_d3d12_ready=",
        "renderer_quality_matrix_vulkan_strict_ready=",
        "renderer_quality_matrix_metal_ready=",
        "renderer_quality_matrix_requires_metal_host_evidence=",
        "renderer_quality_matrix_metal_host_evidence=",
        "renderer_quality_matrix_selected_package_evidence_ready=",
        "renderer_quality_matrix_general_renderer_quality_ready=",
        "renderer_quality_matrix_invoked_gpu_commands=",
        "renderer_quality_matrix_invoked_native_capture=",
        "renderer_quality_matrix_invoked_crash_upload=",
        "renderer_quality_matrix_diagnostics="
    )) {
    Assert-ContainsText $sample3dMainText $needle "games/sample_generated_desktop_runtime_3d_package/main.cpp"
}

Assert-ContainsText $sample3dManifestText '"renderer-quality-matrix"' "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
Assert-ContainsText $sample3dReadmeText "renderer quality matrix" "games/sample_generated_desktop_runtime_3d_package/README.md"

foreach ($needle in @(
        "renderer_quality_matrix_status=host_evidence_required",
        "renderer_quality_matrix_reviewed=1",
        "renderer_quality_matrix_ready=0",
        "renderer_quality_matrix_rows=21",
        "renderer_quality_matrix_ready_rows=14",
        "renderer_quality_matrix_host_gated_rows=7",
        "renderer_quality_matrix_dependency_gated_rows=0",
        "renderer_quality_matrix_unsupported_rows=0",
        "renderer_quality_matrix_d3d12_ready=1",
        "renderer_quality_matrix_vulkan_strict_ready=1",
        "renderer_quality_matrix_metal_ready=0",
        "renderer_quality_matrix_general_renderer_quality_ready=0",
        "renderer_quality_matrix_diagnostics=0",
        "--require-renderer-quality-matrix",
        "--require-rendering-vfx-profiling"
    )) {
    Assert-ContainsText $sample3dManifestText $needle "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
    Assert-ContainsText $sample3dReadmeText $needle "games/sample_generated_desktop_runtime_3d_package/README.md"
}

Assert-ContainsText $gamesCMakeText "--require-renderer-quality-matrix" "games/CMakeLists.txt"

foreach ($needle in @(
        "requiresRendererQualityMatrix",
        "renderer_quality_matrix_status",
        "renderer_quality_matrix_reviewed",
        "renderer_quality_matrix_ready",
        "renderer_quality_matrix_rows",
        "renderer_quality_matrix_ready_rows",
        "renderer_quality_matrix_host_gated_rows",
        "renderer_quality_matrix_dependency_gated_rows",
        "renderer_quality_matrix_unsupported_rows",
        "renderer_quality_matrix_host_validated_backends",
        "renderer_quality_matrix_replay_hash",
        "renderer_quality_matrix_d3d12_ready",
        "renderer_quality_matrix_vulkan_strict_ready",
        "renderer_quality_matrix_metal_ready",
        "renderer_quality_matrix_requires_metal_host_evidence",
        "renderer_quality_matrix_metal_host_evidence",
        "renderer_quality_matrix_selected_package_evidence_ready",
        "renderer_quality_matrix_general_renderer_quality_ready",
        "renderer_quality_matrix_invoked_gpu_commands",
        "renderer_quality_matrix_invoked_native_capture",
        "renderer_quality_matrix_invoked_crash_upload",
        "renderer_quality_matrix_diagnostics"
    )) {
    Assert-ContainsText $installedValidationText $needle "tools/validate-installed-desktop-runtime.ps1"
}
Assert-ContainsText $installedValidationText '"renderer_quality_matrix_rows" = "21"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"renderer_quality_matrix_ready_rows" = "14"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"renderer_quality_matrix_dependency_gated_rows" = "0"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"renderer_quality_matrix_unsupported_rows" = "0"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"renderer_quality_matrix_host_validated_backends" = "2"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"renderer_quality_matrix_general_renderer_quality_ready" = "0"' "tools/validate-installed-desktop-runtime.ps1"

foreach ($docSurface in @(
        @{ Text = $phasePlanText; Label = "docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md" },
        @{ Text = $activeRendererPlanText; Label = "docs/superpowers/plans/2026-05-27-renderer-production-quality-backend-parity-v1.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $generatedValidationText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $backlogText; Label = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md" },
        @{ Text = $projectionText; Label = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "Renderer General Quality Matrix v1" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_renderer_quality_matrix" $docSurface.Label
    Assert-ContainsText $docSurface.Text "renderer_quality_matrix_status=host_evidence_required" $docSurface.Label
    Assert-ContainsText $docSurface.Text "renderer_quality_matrix_dependency_gated_rows=0" $docSurface.Label
    Assert-ContainsText $docSurface.Text "renderer_quality_matrix_unsupported_rows=0" $docSurface.Label
    Assert-ContainsText $docSurface.Text "renderer_quality_matrix_general_renderer_quality_ready=0" $docSurface.Label
}

Assert-ContainsText $activeRendererPlanText "renderer-quality-status-taxonomy-v1" "docs/superpowers/plans/2026-05-27-renderer-production-quality-backend-parity-v1.md"
Assert-ContainsText $activeRendererPlanText "RendererQualityMatrixRowStatus" "docs/superpowers/plans/2026-05-27-renderer-production-quality-backend-parity-v1.md"
Assert-ContainsText $activeRendererPlanText "RED build failed before implementation" "docs/superpowers/plans/2026-05-27-renderer-production-quality-backend-parity-v1.md"
Assert-ContainsText $activeRendererPlanText "Focused GREEN passed" "docs/superpowers/plans/2026-05-27-renderer-production-quality-backend-parity-v1.md"

foreach ($skillSurface in @(
        @{ Text = $codexRenderingGuidanceText; Label = ".agents/skills/rendering-change/references/full-guidance.md" },
        @{ Text = $claudeRenderingGuidanceText; Label = ".claude/skills/gameengine-rendering/references/full-guidance.md" },
        @{ Text = $codexGameSkillText; Label = ".agents/skills/gameengine-game-development/SKILL.md" },
        @{ Text = $claudeGameSkillText; Label = ".claude/skills/gameengine-game-development/SKILL.md" },
        @{ Text = $codexGameGuidanceText; Label = ".agents/skills/gameengine-game-development/references/full-guidance.md" },
        @{ Text = $claudeGameGuidanceText; Label = ".claude/skills/gameengine-game-development/references/full-guidance.md" },
        @{ Text = $cursorGameSkillText; Label = ".cursor/skills/gameengine-game-development/SKILL.md" }
    )) {
    Assert-ContainsText $skillSurface.Text "Renderer General Quality Matrix v1" $skillSurface.Label
    Assert-ContainsText $skillSurface.Text "--require-renderer-quality-matrix" $skillSurface.Label
    Assert-ContainsText $skillSurface.Text "renderer_quality_matrix_dependency_gated_rows=0" $skillSurface.Label
    Assert-ContainsText $skillSurface.Text "renderer_quality_matrix_unsupported_rows=0" $skillSurface.Label
    Assert-ContainsText $skillSurface.Text "renderer_quality_matrix_general_renderer_quality_ready=0" $skillSurface.Label
}

foreach ($needle in @(
        "Renderer General Quality Matrix v1",
        "engine/renderer/include/mirakana/renderer/renderer_quality_matrix.hpp",
        "RendererQualityMatrixRow",
        "RendererQualityMatrixPlan",
        "plan_renderer_quality_matrix",
        "renderer_quality_matrix_*",
        "renderer_quality_matrix_status=host_evidence_required",
        "renderer_quality_matrix_dependency_gated_rows=0",
        "renderer_quality_matrix_unsupported_rows=0",
        "renderer_quality_matrix_general_renderer_quality_ready=0",
        "currentRendererQualityMatrix"
    )) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}

foreach ($fragmentSurface in @(
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $runtimeFragmentText; Label = "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json" },
        @{ Text = $packagingFragmentText; Label = "engine/agent/manifest.fragments/008-packagingTargets.json" },
        @{ Text = $gameGuidanceFragmentText; Label = "engine/agent/manifest.fragments/014-gameCodeGuidance.json" }
    )) {
    Assert-ContainsText $fragmentSurface.Text "Renderer General Quality Matrix v1" $fragmentSurface.Label
    Assert-ContainsText $fragmentSurface.Text "renderer_quality_matrix_status=host_evidence_required" $fragmentSurface.Label
    Assert-ContainsText $fragmentSurface.Text "renderer_quality_matrix_dependency_gated_rows=0" $fragmentSurface.Label
    Assert-ContainsText $fragmentSurface.Text "renderer_quality_matrix_unsupported_rows=0" $fragmentSurface.Label
    Assert-ContainsText $fragmentSurface.Text "renderer_quality_matrix_general_renderer_quality_ready=0" $fragmentSurface.Label
}

Assert-ContainsText $planRegistryText "Engine General Production Quality Expansion v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $planRegistryText "Renderer General Quality Matrix v1" "docs/superpowers/plans/README.md"
