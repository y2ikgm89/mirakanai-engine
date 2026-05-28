#requires -Version 7.0
#requires -PSEdition Core

# Chapter 8.1 for check-ai-integration.ps1 committed generated 3D package evidence contracts.

foreach ($needle in @(
        "desktopRuntime3dPackageStreamingSafePointSmoke",
        "--require-package-streaming-safe-point",
        "package_streaming_status",
        "currentKtxBasisTextureReviewPackageSmoke",
        "--require-ktx2-basis-texture-review",
        "ktx_basis_texture_review_status=host_evidence_required",
        "ktx_basis_texture_review_ready=1",
        "vcpkg.ktx",
        "zero runtime transcoding",
        "zero GPU upload",
        "zero compression tool invocation",
        "broad texture codec readiness not claimed",
        "desktopRuntime3dRendererQualityPackageSmoke",
        "--require-renderer-quality-gates",
        "renderer_quality_status",
        "renderer_quality_expected_framegraph_passes=2",
        "renderer_quality_expected_framegraph_barrier_steps=4",
        "framegraph_barrier_steps_executed",
        "desktopRuntime3dPostprocessDepthPackageSmoke",
        "--require-postprocess-depth-input",
        "postprocess_depth_input_ready=1",
        "renderer_quality_postprocess_depth_input_ready=1",
        "desktopRuntime3dPlayablePackageSmoke",
        "--require-playable-3d-slice",
        "playable_3d_status",
        "desktopRuntime3dDirectionalShadowPackageSmoke",
        "--require-directional-shadow-filtering",
        "directional_shadow_status=ready",
        "framegraph_barrier_steps_executed=15",
        "desktopRuntime3dShadowMorphCompositionPackageSmoke",
        "--require-shadow-morph-composition",
        "renderer_morph_descriptor_binds",
        "framegraph_render_passes_recorded=6",
        "desktopRuntime3dNativeUiOverlayPackageSmoke",
        "--require-native-ui-overlay",
        "ui_overlay_ready=1",
        "desktopRuntime3dEntityScaleCullingPackageSmoke",
        "installed-d3d12-3d-entity-scale-culling-smoke",
        "desktopRuntime3dVisibleProductionPackageProof",
        "--require-visible-3d-production-proof",
        "visible_3d_status=ready",
        "desktopRuntime3dNativeUiTexturedSpriteAtlasPackageSmoke",
        "--require-native-ui-textured-sprite-atlas",
        "ui_texture_overlay_atlas_ready=1",
        "desktopRuntime3dNativeUiTextGlyphAtlasPackageSmoke",
        "--require-native-ui-text-glyph-atlas",
        "text_glyphs_resolved=2"
    )) {
    Assert-ContainsText $engineManifestText $needle "engine/agent/manifest.json"
}

foreach ($needle in @("remaining plan evidence that is still referenced", "Git history for deleted evidence")) {
    Assert-ContainsText $historicalPlanEvidenceText $needle "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
}

foreach ($needle in @(
        "Generated 3D Committed Package Sample v1",
        "Generated 3D Renderer Quality Package Smoke v1",
        "Generated 3D Playable Package Smoke v1",
        "Generated 3D Postprocess Depth Package Smoke v1",
        "Generated 3D Directional Shadow Package Smoke v1",
        "Generated 3D Shadow Morph Composition Package Smoke v1",
        "Generated 3D Native UI Overlay Package Smoke v1",
        "Generated 3D Visible Production-Style Package Proof v1",
        "Generated 3D Native UI Text Glyph Atlas Package Smoke v1"
    )) {
    Assert-ContainsText $roadmapText $needle "docs/roadmap.md"
}

foreach ($needle in @(
        "sample_generated_desktop_runtime_3d_package",
        "Generated 3D Renderer Quality Package Smoke v1",
        "Generated 3D Playable Package Smoke v1",
        "Generated 3D Postprocess Depth Package Smoke v1",
        "Generated 3D Directional Shadow Package Smoke v1",
        "--require-shadow-morph-composition",
        "--require-native-ui-overlay",
        "--require-visible-3d-production-proof",
        "--require-native-ui-text-glyph-atlas"
    )) {
    Assert-ContainsText $currentCapabilitiesText $needle "docs/current-capabilities.md"
}

foreach ($needle in @(
        "postprocess_depth_input_ready=1",
        "renderer_quality_postprocess_depth_input_ready=1",
        "renderer_quality_expected_framegraph_barrier_steps=9",
        "framegraph_barrier_steps_executed",
        "--require-shadow-morph-composition",
        "--require-native-ui-overlay",
        "--require-visible-3d-production-proof",
        "--require-native-ui-text-glyph-atlas"
    )) {
    Assert-ContainsText $gameSkillText $needle "gameengine game-development skill"
}

foreach ($needle in @(
        "--require-shadow-morph-composition",
        "--require-native-ui-overlay",
        "--require-visible-3d-production-proof",
        "--require-native-ui-text-glyph-atlas"
    )) {
    Assert-ContainsText $generatedScenariosText $needle "docs/specs/generated-game-validation-scenarios.md"
}
