#requires -Version 7.0
#requires -PSEdition Core

# Chapter 9.9 for check-ai-integration.ps1 Runtime UI Production Stack Evidence contracts.

$runtimeUiPublicHeaderText = Get-AgentSurfaceText "engine/ui/include/mirakana/ui/ui.hpp"
$runtimeUiHeaderText = Get-AgentSurfaceText "engine/ui/include/mirakana/ui/runtime_ui_production_stack.hpp"
$runtimeUiSourceText = Get-AgentSurfaceText "engine/ui/src/runtime_ui_production_stack.cpp"
$runtimeUiCMakeText = Get-AgentSurfaceText "engine/ui/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeUiTestsText = Get-AgentSurfaceText "tests/unit/runtime_ui_production_stack_tests.cpp"
$uiRendererTestsText = Get-AgentSurfaceText "tests/unit/ui_renderer_tests.cpp"
$sample2dMainText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample2dManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample2dReadmeText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/README.md"
$gamesCMakeText = Get-AgentSurfaceText "games/CMakeLists.txt"
$installedValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
$phasePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$uiDocsText = Get-AgentSurfaceText "docs/ui.md"
$aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
$generatedValidationText = Get-AgentSurfaceText "docs/specs/generated-game-validation-scenarios.md"
$workflowsText = Get-AgentSurfaceText "docs/workflows.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$codexGameGuidanceText = Get-AgentSurfaceText ".agents/skills/gameengine-game-development/references/full-guidance.md"
$claudeGameGuidanceText = Get-AgentSurfaceText ".claude/skills/gameengine-game-development/references/full-guidance.md"
$runtimeBackendReadinessFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
$validationRecipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$gameCodeGuidanceFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "RuntimeUiProductionStackStatus",
        "RuntimeUiProductionFeatureKind",
        "RuntimeUiProductionProofKind",
        "RuntimeUiProductionDiagnosticCode",
        "RuntimeUiProductionEvidenceRow",
        "RuntimeUiProductionStackRequest",
        "RuntimeUiProductionDiagnostic",
        "RuntimeUiProductionStackPlan",
        "runtime_ui_production_stack_status_name",
        "plan_runtime_ui_production_stack"
    )) {
    Assert-ContainsText $runtimeUiHeaderText $needle "engine/ui/include/mirakana/ui/runtime_ui_production_stack.hpp"
}

foreach ($needle in @(
        "TextShapingSegmentEvidence",
        "TextShapedGlyph",
        "TextBoundaryEvidence",
        "TextFontFallbackEvidence",
        "TextLineBreakRun",
        "FontRasterizationPixelFormat",
        "GlyphRasterBitmap",
        "GlyphRasterMetrics"
    )) {
    Assert-ContainsText $runtimeUiPublicHeaderText $needle "engine/ui/include/mirakana/ui/ui.hpp"
}

foreach ($needle in @(
        "missing_shaping_glyph_clusters",
        "missing_shaping_direction_script_language",
        "missing_raster_glyph_bitmaps",
        "missing_raster_pixel_format_rows",
        "missing_atlas_eviction_diagnostics",
        "missing_ime_text_area_rows",
        "missing_accessibility_os_publication_gate",
        "unsupported_native_handle",
        "unsupported_ui_middleware_api",
        "unsupported_broad_production_claim",
        "unreviewed_dependency_adapter",
        "side_effect_invocation",
        "bool invoked_text_shaping{false}",
        "bool invoked_font_rasterization{false}",
        "bool invoked_renderer_upload{false}",
        "bool invoked_ime_adapter{false}",
        "bool invoked_accessibility_bridge{false}",
        "bool invoked_native_platform{false}"
    )) {
    Assert-ContainsText $runtimeUiHeaderText $needle "engine/ui/include/mirakana/ui/runtime_ui_production_stack.hpp"
}

foreach ($needle in @(
        "RuntimeUiProductionStackStatus::host_evidence_required",
        "RuntimeUiProductionDiagnosticCode::missing_selected_package_counter_evidence",
        "RuntimeUiProductionDiagnosticCode::missing_shaping_direction_script_language",
        "RuntimeUiProductionDiagnosticCode::missing_raster_pixel_format_rows",
        "RuntimeUiProductionDiagnosticCode::missing_platform_dispatch_boundary",
        "RuntimeUiProductionDiagnosticCode::unsupported_native_handle",
        "RuntimeUiProductionDiagnosticCode::side_effect_invocation",
        "compute_replay_hash",
        "append_missing_feature_diagnostics",
        "production_runtime_ui_ready"
    )) {
    Assert-ContainsText $runtimeUiSourceText $needle "engine/ui/src/runtime_ui_production_stack.cpp"
}

Assert-ContainsText $runtimeUiCMakeText "src/runtime_ui_production_stack.cpp" "engine/ui/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_ui_production_stack_tests" "CMakeLists.txt"
Assert-ContainsText $runtimeUiTestsText "runtime ui production stack reports host gated selected package evidence" "tests/unit/runtime_ui_production_stack_tests.cpp"
Assert-ContainsText $runtimeUiTestsText "runtime ui production stack becomes ready only when host evidence is present" "tests/unit/runtime_ui_production_stack_tests.cpp"
Assert-ContainsText $runtimeUiTestsText "missing_shaping_direction_script_language" "tests/unit/runtime_ui_production_stack_tests.cpp"
Assert-ContainsText $runtimeUiTestsText "missing_raster_pixel_format_rows" "tests/unit/runtime_ui_production_stack_tests.cpp"
Assert-ContainsText $uiRendererTestsText "ui line breaking adapter contract does not require shaping glyph evidence" "tests/unit/ui_renderer_tests.cpp"
Assert-ContainsText $runtimeUiTestsText "runtime ui production stack rejects incomplete ime and accessibility evidence" "tests/unit/runtime_ui_production_stack_tests.cpp"
Assert-ContainsText $runtimeUiTestsText "runtime ui production stack rejects duplicate missing and unsafe claim rows" "tests/unit/runtime_ui_production_stack_tests.cpp"

foreach ($needle in @(
        "mirakana/ui/runtime_ui_production_stack.hpp",
        "--require-runtime-ui-production-stack",
        "shaping_direction_script_language",
        "glyph_pixel_format_rows",
        "RuntimeUiProductionStackProbeResult",
        "validate_runtime_ui_production_stack_package_evidence",
        "plan_runtime_ui_production_stack",
        "runtime_ui_production_stack_status=",
        "runtime_ui_production_stack_reviewed=",
        "runtime_ui_production_stack_ready=",
        "runtime_ui_production_stack_rows=",
        "runtime_ui_production_stack_ready_rows=",
        "runtime_ui_production_stack_host_gated_rows=",
        "runtime_ui_production_stack_text_contract_ready=",
        "runtime_ui_production_stack_selected_package_evidence_ready=",
        "runtime_ui_production_stack_production_ready=",
        "runtime_ui_production_stack_requires_ime_host_evidence=",
        "runtime_ui_production_stack_ime_host_evidence=",
        "runtime_ui_production_stack_requires_accessibility_host_evidence=",
        "runtime_ui_production_stack_accessibility_host_evidence=",
        "runtime_ui_production_stack_invoked_text_shaping=",
        "runtime_ui_production_stack_invoked_font_rasterization=",
        "runtime_ui_production_stack_invoked_ime=",
        "runtime_ui_production_stack_invoked_accessibility_bridge=",
        "runtime_ui_production_stack_invoked_native_platform=",
        "runtime_ui_production_stack_invoked_renderer_upload=",
        "runtime_ui_production_stack_diagnostics=",
        "runtime_ui_production_stack_replay_hash="
    )) {
    Assert-ContainsText $sample2dMainText $needle "games/sample_2d_desktop_runtime_package/main.cpp"
}

foreach ($needle in @(
        "--require-runtime-ui-production-stack",
        '"runtime-ui-production-stack"',
        '"installed-2d-runtime-ui-production-stack-smoke"',
        "Runtime UI Production Stack selected package evidence"
    )) {
    Assert-ContainsText $sample2dManifestText $needle "games/sample_2d_desktop_runtime_package/game.agent.json"
}

foreach ($needle in @(
        "--require-runtime-ui-production-stack",
        "runtime_ui_production_stack_status=host_evidence_required",
        "runtime_ui_production_stack_reviewed=1",
        "runtime_ui_production_stack_ready=1",
        "runtime_ui_production_stack_rows=6",
        "runtime_ui_production_stack_ready_rows=4",
        "runtime_ui_production_stack_host_gated_rows=2",
        "runtime_ui_production_stack_production_ready=0",
        "runtime_ui_production_stack_diagnostics=0"
    )) {
    Assert-ContainsText $sample2dReadmeText $needle "games/sample_2d_desktop_runtime_package/README.md"
}

Assert-ContainsText $gamesCMakeText "--require-runtime-ui-production-stack" "games/CMakeLists.txt"
Assert-ContainsText $installedValidationText '$requiresRuntimeUiProductionStack' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText "--require-runtime-ui-production-stack" "tools/validate-installed-desktop-runtime.ps1"

foreach ($needle in @(
        "runtime_ui_production_stack_status",
        "runtime_ui_production_stack_reviewed",
        "runtime_ui_production_stack_ready",
        "runtime_ui_production_stack_rows",
        "runtime_ui_production_stack_ready_rows",
        "runtime_ui_production_stack_host_gated_rows",
        "runtime_ui_production_stack_text_contract_ready",
        "runtime_ui_production_stack_selected_package_evidence_ready",
        "runtime_ui_production_stack_production_ready",
        "runtime_ui_production_stack_requires_ime_host_evidence",
        "runtime_ui_production_stack_ime_host_evidence",
        "runtime_ui_production_stack_requires_accessibility_host_evidence",
        "runtime_ui_production_stack_accessibility_host_evidence",
        "runtime_ui_production_stack_invoked_text_shaping",
        "runtime_ui_production_stack_invoked_font_rasterization",
        "runtime_ui_production_stack_invoked_ime",
        "runtime_ui_production_stack_invoked_accessibility_bridge",
        "runtime_ui_production_stack_invoked_native_platform",
        "runtime_ui_production_stack_invoked_renderer_upload",
        "runtime_ui_production_stack_diagnostics",
        "runtime_ui_production_stack_replay_hash"
    )) {
    Assert-ContainsText $installedValidationText $needle "tools/validate-installed-desktop-runtime.ps1"
}
Assert-ContainsText $installedValidationText '"runtime_ui_production_stack_status" = "host_evidence_required"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"runtime_ui_production_stack_rows" = "6"' "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedValidationText '"runtime_ui_production_stack_production_ready" = "0"' "tools/validate-installed-desktop-runtime.ps1"

foreach ($docSurface in @(
        @{ Text = $phasePlanText; Label = "docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $uiDocsText; Label = "docs/ui.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $codexGameGuidanceText; Label = ".agents/skills/gameengine-game-development/references/full-guidance.md" },
        @{ Text = $claudeGameGuidanceText; Label = ".claude/skills/gameengine-game-development/references/full-guidance.md" }
    )) {
    Assert-ContainsText $docSurface.Text "plan_runtime_ui_production_stack" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $phasePlanText; Label = "docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $uiDocsText; Label = "docs/ui.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $generatedValidationText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $workflowsText; Label = "docs/workflows.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $codexGameGuidanceText; Label = ".agents/skills/gameengine-game-development/references/full-guidance.md" },
        @{ Text = $claudeGameGuidanceText; Label = ".claude/skills/gameengine-game-development/references/full-guidance.md" }
    )) {
    Assert-ContainsText $docSurface.Text "--require-runtime-ui-production-stack" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $generatedValidationText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $workflowsText; Label = "docs/workflows.md" },
        @{ Text = $codexGameGuidanceText; Label = ".agents/skills/gameengine-game-development/references/full-guidance.md" },
        @{ Text = $claudeGameGuidanceText; Label = ".claude/skills/gameengine-game-development/references/full-guidance.md" }
    )) {
    Assert-ContainsText $docSurface.Text "runtime_ui_production_stack_status=host_evidence_required" $docSurface.Label
}

foreach ($needle in @(
        "runtime-ui-production-stack-evidence",
        "RuntimeUiProductionEvidenceRow",
        "plan_runtime_ui_production_stack",
        "shaping direction/script/language",
        "glyph bitmap/metrics/pixel-format",
        "runtime_ui_production_stack_status=host_evidence_required"
    )) {
    Assert-ContainsText $runtimeBackendReadinessFragmentText $needle "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
}

foreach ($needle in @(
        "desktop-runtime-2d-runtime-ui-production-stack-proof",
        "--require-runtime-ui-production-stack",
        "shaping direction/script/language",
        "glyph bitmap/metrics/pixel-format",
        "runtime_ui_production_stack_status=host_evidence_required"
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle "engine/agent/manifest.fragments/009-validationRecipes.json"
}

foreach ($needle in @(
        "currentRuntimeUiProductionStack",
        "RuntimeUiProductionStackRequest",
        "plan_runtime_ui_production_stack",
        "shaping direction/script/language",
        "glyph bitmap/metrics/pixel-format",
        "runtime_ui_production_stack_status=host_evidence_required"
    )) {
    Assert-ContainsText $gameCodeGuidanceFragmentText $needle "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
}

foreach ($needle in @(
        "runtime-ui-production-stack-evidence",
        "desktop-runtime-2d-runtime-ui-production-stack-proof",
        "currentRuntimeUiProductionStack",
        "RuntimeUiProductionStackPlan",
        "plan_runtime_ui_production_stack",
        "shaping direction/script/language",
        "glyph bitmap/metrics/pixel-format",
        "runtime_ui_production_stack_status=host_evidence_required"
    )) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}
