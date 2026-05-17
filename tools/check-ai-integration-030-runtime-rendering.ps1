#requires -Version 7.0
#requires -PSEdition Core

# Chapter 3 for check-ai-integration.ps1 static contracts.

if (-not ([string]$editorProductizationGap[0].notes).Contains("EditorPlaySession") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("IEditorPlaySessionDriver") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorPlaySessionTickContext") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorPlaySessionControlsModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorResourcePanelModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorAiCommandPanelModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorAiPlaytestEvidenceImportModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorAiPlaytestEvidenceImportReviewRow") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorAiReviewedValidationExecutionModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorAiReviewedValidationExecutionDesc") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorAiReviewedValidationExecutionBatchModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Host-Gated Validation Execution Ack v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor AI Reviewed Validation Batch Execution v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("-HostGateAcknowledgements") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorContentBrowserImportPanelModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorContentBrowserImportOpenDialogModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorContentBrowserImportExternalSourceCopyModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Content Browser Import Codec Adapter Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ExternalAssetImportAdapters") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorMaterialAssetPreviewPanelModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorMaterialGpuPreviewExecutionSnapshot") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("material_asset_preview.gpu.execution") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("material_asset_preview.gpu.execution.parity_checklist") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorMaterialGpuPreviewDisplayParityChecklistRow") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ge.editor.material_gpu_preview_display_parity_checklist.v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorInputRebindingProfilePanelModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("PrefabVariantConflictReviewModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("PrefabVariantConflictRow") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_content_browser_import_panel_ui_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_content_browser_import_open_dialog_request") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_content_browser_import_external_source_copy_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("content_browser_import.open_dialog") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("content_browser_import.external_copy") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Browse Import Sources") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Copy External Sources") -or
    -not ([string]$editorProductizationGap[0].notes).Contains(".png/.gltf/.glb/.wav/.mp3/.flac") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_material_asset_preview_panel_ui_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_input_rebinding_profile_panel_ui_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_editor_ai_playtest_evidence_import_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_editor_ai_playtest_evidence_import_ui_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_editor_ai_reviewed_validation_execution_plan") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_editor_ai_reviewed_validation_execution_batch") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_ai_reviewed_validation_execution_ui_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_prefab_variant_conflict_review_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_prefab_variant_conflict_review_ui_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("deserialize_prefab_variant_definition_for_review") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Remove missing-node override") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("PrefabNodeOverride::source_node_name") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("override.N.source_node_name") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("source_node_mismatch") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("accept_current_node") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Accept current node N") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("updates only source_node_name") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("strict MK_scene composition index-based") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Retarget override to node N") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("resolution kind/target") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_prefab_variant_conflict_resolution_action") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("PrefabVariantConflictBatchResolutionPlan") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("resolve_prefab_variant_conflicts") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_prefab_variant_conflict_batch_resolution_action") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("prefab_variant_conflicts.batch_resolution") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Apply All Reviewed") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ai_evidence_import") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ai_commands.execution") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ai_commands.execution.batch") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Execute Ready") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("prefab_variant_conflicts") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("simulation-scene viewport rendering") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("source-scene authoring plus undo/redo blocking") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorRuntimeHostPlaytestLaunchDesc") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorRuntimeHostPlaytestLaunchModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_editor_runtime_host_playtest_launch_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("play_in_editor.runtime_host") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Win32ProcessRunner") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Resources diagnostics") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("AI Commands diagnostics") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Assets diagnostics") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Input Rebinding diagnostics") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorInputRebindingCaptureModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("input_rebinding.capture") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("in-memory profile candidate application") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Prefab Variant Authoring conflict review") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("hot-reload summaries") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("residency enforcement") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("allocator policy") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Prefab Variant Base Refresh Merge Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("PrefabVariantBaseRefreshPlan") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("plan_prefab_variant_base_refresh") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("apply_prefab_variant_base_refresh") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("prefab_variant_base_refresh") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Prefab Instance Source-Link Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ScenePrefabSourceLink") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("PrefabInstantiateDesc") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("SceneNode::prefab_source") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ScenePrefabInstanceSourceLinkModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_scene_prefab_instance_source_link_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("scene_prefab_source_links") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Scene Prefab Instance Refresh Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Prefab Instance Local Child Refresh Resolution v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ScenePrefabInstanceRefreshPlan") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ScenePrefabInstanceRefreshPolicy") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("plan_scene_prefab_instance_refresh") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("plan_scene_prefab_instance_refresh_batch") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_scene_prefab_instance_refresh_action") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_scene_prefab_instance_refresh_batch_action") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("scene_prefab_instance_refresh") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("scene_prefab_instance_refresh_batch") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Refresh Prefab Instance") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Keep Local Children") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("keep_local_child") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Prefab Instance Stale Node Refresh Resolution v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("keep_stale_source_node_as_local") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Keep Stale Source Nodes") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Nested Prefab Refresh Resolution v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("keep_nested_prefab_instance") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("unsupported_nested_prefab_instance") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Keep Nested Prefab Instances") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ge.editor.scene_prefab_nested_variant_alignment.v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ge.editor.scene_prefab_local_child_variant_alignment.v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ge.editor.scene_prefab_stale_source_variant_alignment.v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ge.editor.scene_prefab_source_node_variant_alignment.v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ge.editor.scene_nested_prefab_propagation_preview.v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("prefab_variant_conflict_resolution_kind_label") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("unsupported local children") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("reviewed missing-node/source-mismatch retarget, accept-current hint repair, batch cleanup, explicit base-refresh apply, source-link diagnostics, explicit scene prefab instance refresh, reviewed local child refresh preservation, and reviewed stale source-node keep-as-local preservation") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("isolated simulation scene") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor In-Process Runtime Host Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorInProcessRuntimeHostDesc") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("begin_editor_in_process_runtime_host_session") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("play_in_editor.in_process_runtime_host") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("IEditorPlaySessionDriver handoff") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Dynamic Game Module Driver Load v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Game Module Driver Safe Reload Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Game Module Driver Contract Metadata Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("DynamicLibrary") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("LoadLibraryExW") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorGameModuleDriverApi") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorGameModuleDriverContractMetadataModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_editor_game_module_driver_contract_metadata_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("GameEngine.EditorGameModuleDriver.v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("mirakana_create_editor_game_module_driver_v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorGameModuleDriverReloadModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_editor_game_module_driver_reload_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("play_in_editor.game_module_driver") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("play_in_editor.game_module_driver.reload") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("play_in_editor.game_module_driver.contract") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("play_in_editor.game_module_driver.session") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorGameModuleDriverHostSessionPhase") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ge.editor.editor_game_module_driver_host_session.v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ge.editor.editor_game_module_driver_host_session_dll_barriers.v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Load Game Module Driver") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Reload Game Module Driver") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Productization 1.0 Scope Closeout v1 reclassifies") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("active-session hot reload and broader dynamic game-module loading and in-process runtime-host embedding beyond reviewed external runtime-host launch, linked-driver handoff, and explicit editor game-module driver load evidence") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("package script execution") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("automatic host-gated validation recipe execution") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("arbitrary shell") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("raw manifest command evaluation") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("free-form manifest edits") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("renderer/RHI handle exposure") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("arbitrary importer adapters") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("automatic import execution") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("hot reload readiness beyond reviewed recook/apply state") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("shader compiler execution from editor core") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("axis input rebinding capture") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("runtime UI focus/consumption") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("input glyph generation") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("multiplayer device assignment") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Vulkan/Metal material-preview display parity beyond D3D12 host-owned execution evidence") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("automatic host-gated AI command execution workflows") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("vendor-stable third-party editor DLL ABI and unacknowledged or automatic host-gated AI command execution are explicit Engine 1.0 exclusions rather than required-before-ready claims") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Vulkan display parity") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("nested prefab propagation/merge resolution UX")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop editor-productization gap must keep Play-In-Editor, resource diagnostics, AI command diagnostics, AI evidence import review, reviewed validation execution, content browser/material/input/prefab diagnostics, and remaining unsupported claims explicit"
}
$productionUiImporterPlatformGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "production-ui-importer-platform-adapters" })
if ($productionUiImporterPlatformGap.Count -ne 1 -or $productionUiImporterPlatformGap[0].status -ne "planned") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop production-ui-importer-platform-adapters gap must remain planned until OS/platform adapter work is complete"
}
foreach ($needle in @(
    "Runtime UI Accessibility Publish Plan v1",
    "AccessibilityPublishPlan",
    "AccessibilityPublishResult",
    "plan_accessibility_publish",
    "publish_accessibility_payload",
    "AccessibilityPayload",
    "IAccessibilityAdapter",
    "OS accessibility bridge publication",
    "native accessibility objects",
    "platform SDK calls",
    "dependency, legal, vcpkg, and notice records"
)) {
    Assert-ContainsText ([string]$productionUiImporterPlatformGap[0].notes) $needle "engine/agent/manifest.json aiOperableProductionLoop production-ui-importer-platform-adapters gap"
}
foreach ($needle in @(
    "Runtime UI IME Composition Publish Plan v1",
    "ImeCompositionPublishPlan",
    "ImeCompositionPublishResult",
    "plan_ime_composition_update",
    "publish_ime_composition",
    "ImeComposition",
    "IImeAdapter",
    "native IME/text-input sessions",
    "platform SDK calls",
    "dependency, legal, vcpkg, and notice records"
)) {
    Assert-ContainsText ([string]$productionUiImporterPlatformGap[0].notes) $needle "engine/agent/manifest.json aiOperableProductionLoop production-ui-importer-platform-adapters gap"
}
foreach ($needle in @(
    "Runtime UI Platform Text Input Session Plan v1",
    "PlatformTextInputSessionPlan",
    "PlatformTextInputSessionResult",
    "PlatformTextInputEndPlan",
    "PlatformTextInputEndResult",
    "plan_platform_text_input_session",
    "begin_platform_text_input",
    "plan_platform_text_input_end",
    "end_platform_text_input",
    "PlatformTextInputRequest",
    "IPlatformIntegrationAdapter",
    "native text-input object/session ownership",
    "virtual keyboard behavior",
    "platform SDK calls",
    "dependency, legal, vcpkg, and notice records"
)) {
    Assert-ContainsText ([string]$productionUiImporterPlatformGap[0].notes) $needle "engine/agent/manifest.json aiOperableProductionLoop production-ui-importer-platform-adapters gap"
}
foreach ($needle in @(
    "Runtime UI Text Shaping Request Plan v1",
    "TextShapingRequestPlan",
    "TextShapingResult",
    "plan_text_shaping_request",
    "shape_text_run",
    "TextLayoutRequest",
    "TextLayoutRun",
    "ITextShapingAdapter",
    "production text shaping implementation",
    "bidirectional reordering",
    "production line breaking",
    "dependency, legal, vcpkg, and notice records"
)) {
    Assert-ContainsText ([string]$productionUiImporterPlatformGap[0].notes) $needle "engine/agent/manifest.json aiOperableProductionLoop production-ui-importer-platform-adapters gap"
}
foreach ($needle in @(
    "Runtime UI Font Rasterization Request Plan v1",
    "FontRasterizationRequestPlan",
    "FontRasterizationResult",
    "plan_font_rasterization_request",
    "rasterize_font_glyph",
    "FontRasterizationRequest",
    "GlyphAtlasAllocation",
    "IFontRasterizerAdapter",
    "real font loading/rasterization",
    "renderer texture upload",
    "dependency, legal, vcpkg, and notice records"
)) {
    Assert-ContainsText ([string]$productionUiImporterPlatformGap[0].notes) $needle "engine/agent/manifest.json aiOperableProductionLoop production-ui-importer-platform-adapters gap"
}
foreach ($needle in @(
    "Runtime UI Image Decode Request Plan v1",
    "ImageDecodeRequestPlan",
    "ImageDecodeDispatchResult",
    "ImageDecodePixelFormat",
    "plan_image_decode_request",
    "decode_image_request",
    "ImageDecodeRequest",
    "ImageDecodeResult",
    "IImageDecodingAdapter",
    "runtime image decoding",
    "source image codecs",
    "SVG/vector decoding",
    "renderer texture upload",
    "dependency, legal, vcpkg, and notice records"
)) {
    Assert-ContainsText ([string]$productionUiImporterPlatformGap[0].notes) $needle "engine/agent/manifest.json aiOperableProductionLoop production-ui-importer-platform-adapters gap"
}
foreach ($needle in @(
    "Runtime UI PNG Image Decoding Adapter v1",
    "PngImageDecodingAdapter",
    "IImageDecodingAdapter",
    "decode_audited_png_rgba8",
    "libspng",
    "asset-importers",
    "without new dependency, legal, vcpkg, or notice records",
    "runtime image decoding beyond the reviewed PNG adapter",
    "broader source image codecs"
)) {
    Assert-ContainsText ([string]$productionUiImporterPlatformGap[0].notes) $needle "engine/agent/manifest.json aiOperableProductionLoop production-ui-importer-platform-adapters gap"
}
foreach ($needle in @(
    "Runtime UI Decoded Image Atlas Package Bridge v1",
    "PackedUiAtlasAuthoringDesc",
    "author_packed_ui_atlas_from_decoded_images",
    "plan_packed_ui_atlas_package_update",
    "apply_packed_ui_atlas_package_update",
    "pack_sprite_atlas_rgba8_max_side",
    "GameEngine.CookedTexture.v1",
    "GameEngine.UiAtlas.v1",
    "without new dependency, legal, vcpkg, or notice records",
    "renderer texture upload"
)) {
    Assert-ContainsText ([string]$productionUiImporterPlatformGap[0].notes) $needle "engine/agent/manifest.json aiOperableProductionLoop production-ui-importer-platform-adapters gap"
}
foreach ($needle in @(
    "Runtime UI Glyph Atlas Package Bridge v1",
    "UiAtlasMetadataGlyph",
    "RuntimeUiAtlasGlyph",
    "PackedUiGlyphAtlasAuthoringDesc",
    "author_packed_ui_glyph_atlas_from_rasterized_glyphs",
    "plan_packed_ui_glyph_atlas_package_update",
    "apply_packed_ui_glyph_atlas_package_update",
    "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas",
    "source.decoding=rasterized-glyph-adapter",
    "atlas.packing=deterministic-glyph-atlas-rgba8-max-side",
    "GameEngine.CookedTexture.v1",
    "GameEngine.UiAtlas.v1",
    "without new dependency, legal, vcpkg, or notice records",
    "real font loading/rasterization",
    "renderer texture upload"
)) {
    Assert-ContainsText ([string]$productionUiImporterPlatformGap[0].notes) $needle "engine/agent/manifest.json aiOperableProductionLoop production-ui-importer-platform-adapters gap"
}
$fullRepoQualityGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "full-repository-quality-gate" })
if ($fullRepoQualityGap.Count -ne 1 -or $fullRepoQualityGap[0].status -ne "partly-ready") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop full-repository-quality-gate gap must be partly-ready until Phase 1 quality gates complete"
}
if (-not ([string]$fullRepoQualityGap[0].notes).Contains("clang-tidy") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("coverage") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("sanitizer") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("targeted changed-file clang-tidy") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("Full Repository Static Analysis CI Contract v1") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("static-analysis") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("tools/check-tidy.ps1 -Strict") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("Phase 1")) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop full-repository-quality-gate gap must name tidy, coverage, sanitizer, and Phase 1 charter limits explicitly"
}
$vulkanGate = @($productionLoop.hostGates | Where-Object { $_.id -eq "vulkan-strict" })
if ($vulkanGate.Count -ne 1 -or $vulkanGate[0].status -ne "host-gated") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must keep vulkan-strict host-gated"
}
$metalGate = @($productionLoop.hostGates | Where-Object { $_.id -eq "metal-apple" })
if ($metalGate.Count -ne 1 -or $metalGate[0].status -ne "host-gated") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must keep metal-apple host-gated"
}

foreach ($surface in @("codex", "claudeCode")) {
    if (-not $manifest.aiSurfaces.PSObject.Properties.Name.Contains($surface)) {
        Write-Error "engine/agent/manifest.json aiSurfaces missing required field: $surface"
    }
    foreach ($field in @("requiredSkills", "requiredAgents", "readOnlyAgents")) {
        if (-not $manifest.aiSurfaces.$surface.PSObject.Properties.Name.Contains($field)) {
            Write-Error "engine/agent/manifest.json aiSurfaces.$surface missing required field: $field"
        }
    }
}
foreach ($skillName in @("cmake-build-system", "cpp-engine-debugging", "editor-change", "gameengine-agent-integration", "gameengine-feature", "gameengine-game-development", "license-audit", "rendering-change")) {
    if (@($manifest.aiSurfaces.codex.requiredSkills) -notcontains $skillName) {
        Write-Error "engine/agent/manifest.json aiSurfaces.codex.requiredSkills missing $skillName"
    }
}
foreach ($skillName in @("gameengine-agent-integration", "gameengine-cmake-build-system", "gameengine-debugging", "gameengine-editor", "gameengine-feature", "gameengine-game-development", "gameengine-license-audit", "gameengine-rendering")) {
    if (@($manifest.aiSurfaces.claudeCode.requiredSkills) -notcontains $skillName) {
        Write-Error "engine/agent/manifest.json aiSurfaces.claudeCode.requiredSkills missing $skillName"
    }
}
foreach ($agentName in @("build-fixer", "cpp-reviewer", "engine-architect", "explorer", "gameplay-builder", "rendering-auditor")) {
    if (@($manifest.aiSurfaces.codex.requiredAgents) -notcontains $agentName) {
        Write-Error "engine/agent/manifest.json aiSurfaces.codex.requiredAgents missing $agentName"
    }
    if (@($manifest.aiSurfaces.claudeCode.requiredAgents) -notcontains $agentName) {
        Write-Error "engine/agent/manifest.json aiSurfaces.claudeCode.requiredAgents missing $agentName"
    }
}
foreach ($agentName in @("cpp-reviewer", "engine-architect", "explorer", "rendering-auditor")) {
    if (@($manifest.aiSurfaces.codex.readOnlyAgents) -notcontains $agentName) {
        Write-Error "engine/agent/manifest.json aiSurfaces.codex.readOnlyAgents missing $agentName"
    }
    if (@($manifest.aiSurfaces.claudeCode.readOnlyAgents) -notcontains $agentName) {
        Write-Error "engine/agent/manifest.json aiSurfaces.claudeCode.readOnlyAgents missing $agentName"
    }
}

if (-not $manifest.PSObject.Properties.Name.Contains("documentationPolicy")) {
    Write-Error "engine/agent/manifest.json missing required field: documentationPolicy"
}
if (-not $manifest.documentationPolicy.PSObject.Properties.Name.Contains("entrypoints")) {
    Write-Error "engine/agent/manifest.json documentationPolicy missing required field: entrypoints"
}
foreach ($field in @("entrypoint", "currentStatus", "currentCapabilities", "workflows", "planRegistry", "specRegistry", "superpowersSpecRegistry", "activeRoadmap")) {
    if (-not $manifest.documentationPolicy.entrypoints.PSObject.Properties.Name.Contains($field)) {
        Write-Error "engine/agent/manifest.json documentationPolicy.entrypoints missing required field: $field"
    }
    Resolve-RequiredAgentPath $manifest.documentationPolicy.entrypoints.$field | Out-Null
}
if ($manifest.documentationPolicy.preferredMcp -ne "context7") {
    Write-Error "engine/agent/manifest.json documentationPolicy.preferredMcp must be context7"
}
if (-not $manifest.documentationPolicy.doNotStoreApiKeysInRepo) {
    Write-Error "engine/agent/manifest.json documentationPolicy must forbid storing API keys in the repository"
}

if (-not $manifest.commands.PSObject.Properties.Name.Contains("newGame")) {
    Write-Error "engine/agent/manifest.json must expose commands.newGame"
}
if (-not $manifest.commands.newGame.Contains("DesktopRuntimeMaterialShaderPackage")) {
    Write-Error "engine/agent/manifest.json commands.newGame must expose DesktopRuntimeMaterialShaderPackage"
}
if (-not $manifest.commands.newGame.Contains("DesktopRuntime2DPackage")) {
    Write-Error "engine/agent/manifest.json commands.newGame must expose DesktopRuntime2DPackage"
}
if (-not $manifest.commands.newGame.Contains("DesktopRuntime3DPackage")) {
    Write-Error "engine/agent/manifest.json commands.newGame must expose DesktopRuntime3DPackage"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("buildAssetImporters")) {
    Write-Error "engine/agent/manifest.json must expose commands.buildAssetImporters"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("registerRuntimePackageFiles")) {
    Write-Error "engine/agent/manifest.json must expose commands.registerRuntimePackageFiles"
}

if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("gameRoot")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.gameRoot"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentRuntimeUi")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentRuntimeUi"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentEditorAiReviewedValidationExecution")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentEditorAiReviewedValidationExecution"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentEditorProfilerTraceExport")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentEditorProfilerTraceExport"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentEditorPrefabVariantFileDialogs")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentEditorPrefabVariantFileDialogs"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentEditorPrefabInstanceSourceLinks")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentEditorPrefabInstanceSourceLinks"
}
$geUiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_ui" })
if ($geUiModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_ui module"
}
$geUiRendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_ui_renderer" })
if ($geUiRendererModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_ui_renderer module"
}
$geSceneModule = @($manifest.modules | Where-Object { $_.name -eq "MK_scene" })
if ($geSceneModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_scene module"
}
$geAssetsModule = @($manifest.modules | Where-Object { $_.name -eq "MK_assets" })
if ($geAssetsModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_assets module"
}
$geRuntimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($geRuntimeModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module"
}
$geAudioModule = @($manifest.modules | Where-Object { $_.name -eq "MK_audio" })
if ($geAudioModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_audio module"
}
$gePhysicsModule = @($manifest.modules | Where-Object { $_.name -eq "MK_physics" })
if ($gePhysicsModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_physics module"
}
$geToolsModule = @($manifest.modules | Where-Object { $_.name -eq "MK_tools" })
if ($geToolsModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_tools module"
}
$geEditorCoreModule = @($manifest.modules | Where-Object { $_.name -eq "MK_editor_core" })
if ($geEditorCoreModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_editor_core module"
}
if ($geUiModule[0].status -ne "implemented-runtime-ui-monospace-text-layout") {
    Write-Error "engine/agent/manifest.json MK_ui status must advertise the runtime UI monospace text layout slice honestly"
}
if ($geUiRendererModule[0].status -ne "implemented-runtime-ui-font-image-adapter") {
    Write-Error "engine/agent/manifest.json MK_ui_renderer status must advertise the runtime UI font image adapter slice honestly"
}
if ($geSceneModule[0].status -ne "implemented-scene-schema-v2-contract") {
    Write-Error "engine/agent/manifest.json MK_scene status must advertise the Scene/Component/Prefab Schema v2 contract slice honestly"
}
if (@($geSceneModule[0].publicHeaders) -notcontains "engine/scene/include/mirakana/scene/schema_v2.hpp") {
    Write-Error "engine/agent/manifest.json MK_scene publicHeaders must include schema_v2.hpp"
}
if ($geAssetsModule[0].status -ne "implemented-asset-identity-v2-foundation") {
    Write-Error "engine/agent/manifest.json MK_assets status must advertise the Asset Identity v2 foundation slice honestly"
}
if (@($geAssetsModule[0].publicHeaders) -notcontains "engine/assets/include/mirakana/assets/asset_identity.hpp") {
    Write-Error "engine/agent/manifest.json MK_assets publicHeaders must include asset_identity.hpp"
}
if ($geRuntimeModule[0].status -ne "ready-runtime-resource-v2-safe-point-controller") {
    Write-Error "engine/agent/manifest.json MK_runtime status must advertise the closed Runtime Resource v2 safe-point/controller surface honestly"
}
if (@($geRuntimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/resource_runtime.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime publicHeaders must include resource_runtime.hpp"
}
if ($geAudioModule[0].status -ne "implemented-device-streaming-baseline") {
    Write-Error "engine/agent/manifest.json MK_audio status must advertise the audio device streaming baseline honestly"
}
if (@($geAudioModule[0].publicHeaders) -notcontains "engine/audio/include/mirakana/audio/audio_mixer.hpp") {
    Write-Error "engine/agent/manifest.json MK_audio publicHeaders must include audio_mixer.hpp"
}
if ($gePhysicsModule[0].status -ne "implemented-physics-1-0-ready-surface") {
    Write-Error "engine/agent/manifest.json MK_physics status must advertise the Physics 1.0 ready surface honestly"
}
if (@($gePhysicsModule[0].publicHeaders) -notcontains "engine/physics/include/mirakana/physics/physics3d.hpp") {
    Write-Error "engine/agent/manifest.json MK_physics publicHeaders must include physics3d.hpp"
}
if (@($geToolsModule[0].publicHeaders) -notcontains "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") {
    Write-Error "engine/agent/manifest.json MK_tools publicHeaders must include gltf_node_animation_import.hpp"
}
if (@($geEditorCoreModule[0].publicHeaders) -notcontains "editor/core/include/mirakana/editor/play_in_editor.hpp") {
    Write-Error "engine/agent/manifest.json MK_editor_core publicHeaders must include play_in_editor.hpp"
}
Assert-ContainsText ([string]$geUiModule[0].purpose) "MonospaceTextLayoutPolicy" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "stable glyph ids" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "AccessibilityPublishPlan" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "AccessibilityPublishResult" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "plan_accessibility_publish" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "publish_accessibility_payload" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "IAccessibilityAdapter" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "OS accessibility bridge publication" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "ImeCompositionPublishPlan" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "ImeCompositionPublishResult" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "plan_ime_composition_update" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "publish_ime_composition" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "IImeAdapter" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "native IME/text-input session integration" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "PlatformTextInputSessionPlan" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "PlatformTextInputEndResult" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "begin_platform_text_input" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "end_platform_text_input" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "IPlatformIntegrationAdapter" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "TextShapingRequestPlan" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "TextShapingResult" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "plan_text_shaping_request" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "shape_text_run" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "ITextShapingAdapter" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "FontRasterizationRequestPlan" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "FontRasterizationResult" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "plan_font_rasterization_request" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "rasterize_font_glyph" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "IFontRasterizerAdapter" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "ImageDecodeRequestPlan" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "ImageDecodeDispatchResult" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "plan_image_decode_request" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "decode_image_request" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "IImageDecodingAdapter" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiRendererModule[0].purpose) "UiRendererImagePalette" "MK_ui_renderer module purpose"
Assert-ContainsText ([string]$geUiRendererModule[0].purpose) "UiRendererGlyphAtlasPalette" "MK_ui_renderer module purpose"
Assert-ContainsText ([string]$geUiRendererModule[0].purpose) "text glyph sprite submissions" "MK_ui_renderer module purpose"
Assert-ContainsText ([string]$geUiRendererModule[0].purpose) "image sprite submission" "MK_ui_renderer module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "contract-only" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "GameEngine.Scene.v2" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "nested prefab propagation/merge resolution UX" "MK_scene module purpose"
Assert-ContainsText ([string]$geAssetsModule[0].purpose) "Asset Identity v2" "MK_assets module purpose"
Assert-ContainsText ([string]$geAssetsModule[0].purpose) "GameEngine.AssetIdentity.v2" "MK_assets module purpose"
Assert-ContainsText ([string]$geAssetsModule[0].purpose) "foundation-only" "MK_assets module purpose"
Assert-ContainsText ([string]$geAssetsModule[0].purpose) "renderer/RHI residency" "MK_assets module purpose"
Assert-ContainsText ([string]$geAssetsModule[0].purpose) "GameEngine.MorphMeshCpuSource.v1" "MK_assets module purpose"
Assert-ContainsText ([string]$geAssetsModule[0].purpose) "GameEngine.AnimationFloatClipSource.v1" "MK_assets module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCharacterController3DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "move_physics_character_controller_3d" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsAuthoredCollisionScene3DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "build_physics_world_3d_from_authored_collision_scene" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "native backend requests failing closed" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsShape3DDesc::aabb" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsShape3DDesc::sphere" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsShape3DDesc::capsule" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsQueryFilter3D" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsWorld3D::exact_shape_sweep" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsExactSphereCast3DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsExactSphereCast3DResult" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsWorld3D::exact_sphere_cast" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsContactPoint3D" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsContactManifold3D" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsWorld3D::contact_manifolds" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "stable feature ids" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "warm-start-safe" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsContinuousStep3DConfig" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsContinuousStep3DRow" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsContinuousStep3DResult" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsWorld3D::step_continuous" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsWorld3D::step remains discrete" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCharacterDynamicPolicy3DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCharacterDynamicPolicy3DRowKind" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCharacterDynamicPolicy3DRow" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCharacterDynamicPolicy3DResult" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "evaluate_physics_character_dynamic_policy_3d" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsJoint3DStatus" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsDistanceJoint3DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsJointSolve3DResult" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "solve_physics_joints_3d" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsDeterminismGate3DStatus" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsDeterminismGate3DDiagnostic" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsDeterminismGate3DConfig" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsDeterminismGate3DCounts" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsReplaySignature3D" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsDeterminismGate3DResult" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "make_physics_replay_signature_3d" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "evaluate_physics_determinism_gate_3d" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "validated Physics 1.0 ready surface" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "explicit Jolt/native middleware exclusion" "MK_physics module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "import_gltf_node_transform_animation_tracks" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "import_gltf_node_transform_animation_tracks_3d" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "import_gltf_node_transform_animation_float_clip" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "import_gltf_node_transform_animation_binding_source" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "PngImageDecodingAdapter" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "IImageDecodingAdapter" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "decode_audited_png_rgba8" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "PackedUiAtlasAuthoringDesc" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "author_packed_ui_atlas_from_decoded_images" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "plan_packed_ui_atlas_package_update" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "GameEngine.CookedTexture.v1" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "PackedUiGlyphAtlasAuthoringDesc" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "author_packed_ui_glyph_atlas_from_rasterized_glyphs" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "plan_packed_ui_glyph_atlas_package_update" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "UiAtlasMetadataGlyph" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "GameEngine.UiAtlas.v1" "MK_tools module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "Runtime Resource v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "generation-checked" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "commit_runtime_resident_package_replace_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "commit_runtime_resident_package_unmount_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "host-driven reviewed hot-reload replacement safe point" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "plan_runtime_package_hot_reload_candidate_review_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimePackageHotReloadCandidateReviewResultV2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "plan_runtime_package_hot_reload_recook_change_review_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimePackageHotReloadRecookChangeReviewResultV2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "plan_runtime_package_hot_reload_replacement_intent_review_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimePackageHotReloadReplacementIntentReviewResultV2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "commit_runtime_package_hot_reload_recook_replacement_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimePackageHotReloadRecookReplacementResultV2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "candidate/discovery root coherence" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "defined overlay" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "file watching/recook execution" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "closed for the Engine 1.0 reviewed safe-point/controller surface" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "package streaming" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "native watcher ownership" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "renderer/RHI resource ownership" "MK_runtime module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "AudioDeviceStreamRequest" "MK_audio module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "AudioDeviceStreamPlan" "MK_audio module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "plan_audio_device_stream" "MK_audio module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "render_audio_device_stream_interleaved_float" "MK_audio module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "does not open OS audio devices" "MK_audio module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "runtime_morph_mesh_cpu_payload" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "runtime_animation_float_clip_payload" "MK_runtime module purpose"
Assert-ContainsText ([string]$geEditorCoreModule[0].purpose) "EditorPlaySession" "MK_editor_core module purpose"
Assert-ContainsText ([string]$geEditorCoreModule[0].purpose) "IEditorPlaySessionDriver" "MK_editor_core module purpose"
Assert-ContainsText ([string]$geEditorCoreModule[0].purpose) "EditorPlaySessionTickContext" "MK_editor_core module purpose"
Assert-ContainsText ([string]$geEditorCoreModule[0].purpose) "isolated simulation scene" "MK_editor_core module purpose"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditor) "EditorPlaySession" "editor game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditor) "IEditorPlaySessionDriver" "editor game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditor) "isolated simulation scene" "editor game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditor) "scene_prefab_source_links" "editor prefab source-link guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceExportModel" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_trace_export_model" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_export" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "mirakana::export_diagnostics_trace_json" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceFileSaveRequest" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "save_editor_profiler_trace_json" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_file_save" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceSaveDialogModel" "editor profiler native trace save dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_trace_save_dialog_request" "editor profiler native trace save dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_trace_save_dialog_model" "editor profiler native trace save dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_save_dialog" "editor profiler native trace save dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTelemetryHandoffModel" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_telemetry_handoff_model" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.telemetry" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "mirakana::build_diagnostics_ops_plan" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "DiagnosticsTraceImportReview" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "DiagnosticsTraceImportResult" "editor profiler trace import reconstruction guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "review_diagnostics_trace_json" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "import_diagnostics_trace_json" "editor profiler trace import reconstruction guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceImportReviewModel" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_trace_import_review_model" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_import" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_import.reconstructed_*" "editor profiler trace import reconstruction guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceFileImportRequest" "editor profiler trace file import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceFileImportResult" "editor profiler trace file import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "import_editor_profiler_trace_json" "editor profiler trace file import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_file_import" "editor profiler trace file import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceOpenDialogModel" "editor profiler native trace open dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_trace_open_dialog_request" "editor profiler native trace open dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_trace_open_dialog_model" "editor profiler native trace open dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_open_dialog" "editor profiler native trace open dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Copy Trace JSON" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Save Trace JSON" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Browse Save Trace JSON" "editor profiler native trace save dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Profiler Telemetry Handoff" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Review Trace JSON" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Trace Import Path" "editor profiler trace file import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Import Trace JSON" "editor profiler trace file import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Browse Trace JSON" "editor profiler native trace open dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "SdlFileDialogService" "editor profiler native trace open dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "broader editor native save/open dialogs outside Profiler" "editor profiler native trace dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "arbitrary JSON conversion" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "first-party exported Trace Event JSON subset" "editor profiler trace import reconstruction guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "telemetry SDK/upload execution" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "EditorAiReviewedValidationExecutionModel" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "EditorAiReviewedValidationExecutionBatchModel" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "make_editor_ai_reviewed_validation_execution_plan" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "make_editor_ai_reviewed_validation_execution_batch" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "make_ai_reviewed_validation_execution_batch_ui_model" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "mirakana::ProcessCommand" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "mirakana::Win32ProcessRunner" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "Host-Gated Validation Execution Ack v1" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "Reviewed Validation Batch Execution v1" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "-HostGateAcknowledgements" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "ai_commands.execution.batch" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "Execute Ready" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "automatic host-gated recipe execution" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "resolve_prefab_variant_conflict" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "PrefabVariantConflictBatchResolutionPlan" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "resolve_prefab_variant_conflicts" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "make_prefab_variant_conflict_resolution_action" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "make_prefab_variant_conflict_batch_resolution_action" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "prefab_variant_conflicts" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "prefab_variant_conflicts.batch_resolution" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "Apply All Reviewed" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "PrefabNodeOverride::source_node_name" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "override.N.source_node_name" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "source_node_mismatch" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "accept_current_node" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "Accept current node N" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "updates only source_node_name" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "Strict MK_scene composition remains index-based" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "deserialize_prefab_variant_definition_for_review" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "missing-node stale override rows" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "retargets a missing-node stale override" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "retargets an existing-node source_node_mismatch row" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "accepts the current indexed node" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "without creating duplicate node/kind overrides" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "Nested prefab propagation" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "automatic merge/rebase/resolution UX" "editor prefab variant reviewed resolution guidance"
foreach ($sourceLinkNeedle in @(
    "Editor Prefab Instance Source-Link Review v1",
    "ScenePrefabSourceLink",
    "PrefabInstantiateDesc",
    "SceneNode::prefab_source",
    "ScenePrefabInstanceSourceLinkModel",
    "make_scene_prefab_instance_source_link_model",
    "make_scene_prefab_instance_source_link_ui_model",
    "scene_prefab_source_links",
    "Editor Scene Prefab Instance Refresh Review v1",
    "Editor Prefab Instance Local Child Refresh Resolution v1",
    "Editor Prefab Instance Stale Node Refresh Resolution v1",
    "Editor Nested Prefab Refresh Resolution v1",
    "ScenePrefabInstanceRefreshPlan",
    "ScenePrefabInstanceRefreshRow",
    "ScenePrefabInstanceRefreshResult",
    "ScenePrefabInstanceRefreshPolicy",
    "plan_scene_prefab_instance_refresh",
    "plan_scene_prefab_instance_refresh_batch",
    "apply_scene_prefab_instance_refresh",
    "apply_scene_prefab_instance_refresh_batch",
    "make_scene_prefab_instance_refresh_action",
    "make_scene_prefab_instance_refresh_batch_action",
    "make_scene_prefab_instance_refresh_ui_model",
    "make_scene_prefab_instance_refresh_batch_ui_model",
    "scene_prefab_instance_refresh",
    "scene_prefab_instance_refresh_batch",
    "keep_local_child",
    "keep_local_children",
    "keep_stale_source_nodes_as_local",
    "keep_stale_source_node_as_local",
    "keep_nested_prefab_instances",
    "keep_nested_prefab_instance",
    "unsupported_nested_prefab_instance",
    "Refresh Prefab Instance",
    "Keep Local Children",
    "Keep Stale Source Nodes",
    "Keep Nested Prefab Instances",
    "preserves existing scene node state",
    "unsupported local children",
    "path-aware make_scene_authoring_instantiate_prefab_action",
    "full nested prefab propagation",
    "automatic merge/rebase/resolution UX"
)) {
    Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabInstanceSourceLinks) $sourceLinkNeedle "editor prefab source-link guidance"
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "EditorPrefabVariantFileDialogModel" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "make_prefab_variant_open_dialog_request" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "make_prefab_variant_save_dialog_request" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "make_prefab_variant_open_dialog_model" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "make_prefab_variant_save_dialog_model" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "make_prefab_variant_file_dialog_ui_model" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "prefab_variant_file_dialog.open" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "prefab_variant_file_dialog.save" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "Browse Load Variant" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "Browse Save Variant" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "SdlFileDialogService" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) ".prefabvariant" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "first non-Profiler native document dialog" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "Broader editor native save/open dialogs outside Profiler, Scene, and Prefab Variant" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "native handles" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "Editor Scene Native Dialog v1" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "EditorSceneFileDialogModel" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "EditorSceneFileDialogMode" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "make_scene_open_dialog_request" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "make_scene_save_dialog_request" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "make_scene_open_dialog_model" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "make_scene_save_dialog_model" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "make_scene_file_dialog_ui_model" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "SceneAuthoringDocument::set_scene_path" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "scene_file_dialog.open" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "scene_file_dialog.save" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "Open Scene..." "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "Save Scene As..." "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "Browse Open Scene" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "Browse Save Scene As" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "SdlFileDialogService" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) ".scene" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "Broader editor native save/open dialogs outside Profiler, Scene, and Prefab Variant" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "native handles" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "Editor Project Native Dialog v1" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "EditorProjectFileDialogModel" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "EditorProjectFileDialogMode" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "make_project_open_dialog_request" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "make_project_save_dialog_request" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "make_project_open_dialog_model" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "make_project_save_dialog_model" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "make_project_file_dialog_ui_model" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "ProjectBundlePaths" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "project_file_dialog.open" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "project_file_dialog.save" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "Open Project..." "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "Save Project As..." "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "SdlFileDialogService" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) ".geproject" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "load_project_bundle" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "save_project_bundle" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "Broader editor native save/open dialogs outside Profiler, Scene, Prefab Variant, and Project" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "native handles" "editor project native dialog guidance"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "author_cooked_ui_atlas_metadata" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "verify_cooked_ui_atlas_package_metadata" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "plan_cooked_ui_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "apply_cooked_ui_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "PackedUiAtlasAuthoringDesc" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "author_packed_ui_atlas_from_decoded_images" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "plan_packed_ui_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "apply_packed_ui_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_morph_animation_import.hpp") "import_gltf_morph_mesh_cpu_primitive" "MK_tools gltf morph import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_morph_animation_import.hpp") "import_gltf_morph_weights_animation_float_clip" "MK_tools gltf morph import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") "import_gltf_node_transform_animation_tracks" "MK_tools gltf node animation import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") "GltfNodeTransformAnimationTrack3d" "MK_tools gltf node animation import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") "import_gltf_node_transform_animation_tracks_3d" "MK_tools gltf node animation import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") "import_gltf_node_transform_animation_float_clip" "MK_tools gltf node animation import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") "import_gltf_node_transform_animation_binding_source" "MK_tools gltf node animation import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/gltf/CMakeLists.txt") "gltf_node_animation_import.cpp" "MK_tools gltf CMake source list"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/morph_mesh_cpu_source_bridge.hpp") "morph_mesh_cpu_source_document_from_animation_desc" "MK_tools morph mesh CPU source bridge public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/material_tool.hpp") "plan_material_instance_package_update" "MK_tools material tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/material_tool.hpp") "apply_material_instance_package_update" "MK_tools material tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/material_tool.hpp") "plan_material_graph_package_update" "MK_tools material tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/material_tool.hpp") "apply_material_graph_package_update" "MK_tools material tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/scene_tool.hpp") "plan_scene_package_update" "MK_tools scene tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/scene_tool.hpp") "apply_scene_package_update" "MK_tools scene tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/assets/include/mirakana/assets/ui_atlas_metadata.hpp") "GameEngine.UiAtlas.v1" "MK_assets ui atlas metadata public header"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "MonospaceTextLayoutPolicy" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_accessibility_publish" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "publish_accessibility_payload" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "IAccessibilityAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_ime_composition_update" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "publish_ime_composition" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "IImeAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_platform_text_input_session" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "begin_platform_text_input" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_platform_text_input_end" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "end_platform_text_input" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "IPlatformIntegrationAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_text_shaping_request" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "shape_text_run" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "ITextShapingAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "text shaping request validation" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_font_rasterization_request" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "rasterize_font_glyph" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "IFontRasterizerAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "adapter allocation diagnostics" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_image_decode_request" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "decode_image_request" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "IImageDecodingAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "adapter output diagnostics" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "PngImageDecodingAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "decode_audited_png_rgba8" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "author_packed_ui_atlas_from_decoded_images" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_packed_ui_atlas_package_update" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "GameEngine.CookedTexture.v1" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "instead of parsing source PNG files" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "UiRendererGlyphAtlasPalette" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "UiRendererGlyphAtlasBinding" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "text glyph availability/resolution/missing/submission" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "UiRendererImagePalette" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "image sprite submission" "runtime UI game guidance"
foreach ($runtimeUiGuidance in @(
    "docs/ui.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/testing.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".codex/agents/engine-architect.toml",
    ".claude/agents/engine-architect.md"
)) {
    $runtimeUiText = Get-AgentSurfaceText $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "MonospaceTextLayoutPolicy" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "UiRendererGlyphAtlasPalette" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "UiRendererImagePalette" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "AccessibilityPublishPlan" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "ImeCompositionPublishPlan" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "PlatformTextInputSessionPlan" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "FontRasterizationRequestPlan" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "ImageDecodeRequestPlan" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "plan_image_decode_request" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "glyph atlas generation" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "image decoding" $runtimeUiGuidance
}
foreach ($runtimeUiPngGuidance in @(
    "docs/ui.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "docs/ai-game-development.md",
    "docs/current-capabilities.md",
    "docs/dependencies.md",
    "docs/superpowers/plans/README.md",
    "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".codex/agents/engine-architect.toml",
    ".claude/agents/engine-architect.md"
)) {
    $runtimeUiPngText = Get-AgentSurfaceText $runtimeUiPngGuidance
    Assert-ContainsText $runtimeUiPngText "PngImageDecodingAdapter" $runtimeUiPngGuidance
    Assert-ContainsText $runtimeUiPngText "decode_audited_png_rgba8" $runtimeUiPngGuidance
}
foreach ($runtimeUiDecodedAtlasGuidance in @(
    "docs/ui.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "docs/ai-game-development.md",
    "docs/current-capabilities.md",
    "docs/dependencies.md",
    "docs/superpowers/plans/README.md",
    "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md"
)) {
    $runtimeUiDecodedAtlasText = Get-AgentSurfaceText $runtimeUiDecodedAtlasGuidance
    Assert-ContainsText $runtimeUiDecodedAtlasText "author_packed_ui_atlas_from_decoded_images" $runtimeUiDecodedAtlasGuidance
    Assert-ContainsText $runtimeUiDecodedAtlasText "plan_packed_ui_atlas_package_update" $runtimeUiDecodedAtlasGuidance
    Assert-ContainsText $runtimeUiDecodedAtlasText "GameEngine.CookedTexture.v1" $runtimeUiDecodedAtlasGuidance
}
$geUiHeaderText = Get-AgentSurfaceText "engine/ui/include/mirakana/ui/ui.hpp"
$geUiSourceText = Get-AgentSurfaceText "engine/ui/src/ui.cpp"
$sourceImageDecodeHeaderText = Get-AgentSurfaceText "engine/tools/include/mirakana/tools/source_image_decode.hpp"
$sourceImageDecodeSourceText = Get-AgentSurfaceText "engine/tools/asset/source_image_decode.cpp"
$uiAtlasToolHeaderText = Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp"
$uiAtlasToolSourceText = Get-AgentSurfaceText "engine/tools/asset/ui_atlas_tool.cpp"
$toolsTestsText = Get-AgentSurfaceText "tests/unit/tools_tests.cpp"
$uiRendererHeaderText = Get-AgentSurfaceText "engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp"
$uiRendererSourceText = Get-AgentSurfaceText "engine/ui_renderer/src/ui_renderer.cpp"
$uiRendererTestsText = Get-AgentSurfaceText "tests/unit/ui_renderer_tests.cpp"
Assert-ContainsText $geUiHeaderText "TextAdapterGlyphPlaceholder" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "std::uint32_t glyph" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "AccessibilityPublishPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "AccessibilityPublishResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "plan_accessibility_publish" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "publish_accessibility_payload" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "IAccessibilityAdapter" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "ImeCompositionPublishPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "ImeCompositionPublishResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "plan_ime_composition_update" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "publish_ime_composition" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "IImeAdapter" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "PlatformTextInputSessionPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "PlatformTextInputSessionResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "PlatformTextInputEndPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "PlatformTextInputEndResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "begin_platform_text_input" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "end_platform_text_input" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "IPlatformIntegrationAdapter" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "TextShapingRequestPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "TextShapingResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "plan_text_shaping_request" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "shape_text_run" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "ITextShapingAdapter" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "invalid_text_shaping_result" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "FontRasterizationRequestPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "FontRasterizationResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "plan_font_rasterization_request" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "rasterize_font_glyph" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "IFontRasterizerAdapter" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "invalid_font_allocation" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "ImageDecodeRequestPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "ImageDecodeDispatchResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "ImageDecodePixelFormat" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "plan_image_decode_request" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "decode_image_request" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "IImageDecodingAdapter" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "invalid_image_decode_result" "MK_ui public header"
Assert-ContainsText $geUiSourceText "utf8_scalar_glyph" "MK_ui source"
Assert-ContainsText $geUiSourceText "span.glyph" "MK_ui source"
Assert-ContainsText $geUiSourceText "AccessibilityPublishPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "AccessibilityPublishResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.publish_nodes" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_accessibility_bounds" "MK_ui source"
Assert-ContainsText $geUiSourceText "ImeCompositionPublishPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "ImeCompositionPublishResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.update_composition" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_ime_target" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_ime_cursor" "MK_ui source"
Assert-ContainsText $geUiSourceText "PlatformTextInputSessionPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "PlatformTextInputSessionResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "PlatformTextInputEndPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "PlatformTextInputEndResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.begin_text_input" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.end_text_input" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_platform_text_input_target" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_platform_text_input_bounds" "MK_ui source"
Assert-ContainsText $geUiSourceText "TextShapingRequestPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "TextShapingResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.shape_text" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_text_shaping_text" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_text_shaping_font_family" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_text_shaping_max_width" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_text_shaping_result" "MK_ui source"
Assert-ContainsText $geUiSourceText "FontRasterizationRequestPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "FontRasterizationResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.rasterize_glyph" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_font_family" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_font_glyph" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_font_pixel_size" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_font_allocation" "MK_ui source"
Assert-ContainsText $geUiSourceText "ImageDecodeRequestPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "ImageDecodeDispatchResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.decode_image" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_image_decode_uri" "MK_ui source"
Assert-ContainsText $geUiSourceText "empty_image_decode_bytes" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_image_decode_result" "MK_ui source"
Assert-ContainsText $sourceImageDecodeHeaderText "PngImageDecodingAdapter" "MK_tools source image decode public header"
Assert-ContainsText $sourceImageDecodeHeaderText "ui::IImageDecodingAdapter" "MK_tools source image decode public header"
Assert-ContainsText $sourceImageDecodeHeaderText "decode_image" "MK_tools source image decode public header"
Assert-ContainsText $sourceImageDecodeSourceText "PngImageDecodingAdapter::decode_image" "MK_tools source image decode source"
Assert-ContainsText $sourceImageDecodeSourceText "decode_audited_png_rgba8" "MK_tools source image decode source"
Assert-ContainsText $sourceImageDecodeSourceText "ImageDecodePixelFormat::rgba8_unorm" "MK_tools source image decode source"
Assert-ContainsText $sourceImageDecodeSourceText "catch (...)" "MK_tools source image decode source"
Assert-ContainsText $toolsTestsText "runtime UI PNG image decoding adapter fails closed when importers are disabled" "MK_tools tests"
Assert-ContainsText $toolsTestsText "runtime UI PNG image decoding adapter returns rgba8 image when importers are enabled" "MK_tools tests"
Assert-ContainsText $toolsTestsText "invalid_image_decode_result" "MK_tools tests"
Assert-ContainsText $uiAtlasToolHeaderText "PackedUiAtlasAuthoringDesc" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "PackedUiAtlasPackageUpdateDesc" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "author_packed_ui_atlas_from_decoded_images" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "plan_packed_ui_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "apply_packed_ui_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "PackedUiGlyphAtlasAuthoringDesc" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "PackedUiGlyphAtlasPackageUpdateDesc" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "author_packed_ui_glyph_atlas_from_rasterized_glyphs" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "plan_packed_ui_glyph_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "apply_packed_ui_glyph_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "rasterized-glyph-adapter" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "deterministic-glyph-atlas-rgba8-max-side" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolSourceText "pack_sprite_atlas_rgba8_max_side" "MK_tools ui atlas tool source"
Assert-ContainsText $uiAtlasToolSourceText "GameEngine.CookedTexture.v1" "MK_tools ui atlas tool source"
Assert-ContainsText $uiAtlasToolSourceText "decoded image must be RGBA8" "MK_tools ui atlas tool source"
Assert-ContainsText $uiAtlasToolSourceText "rasterized glyph must be RGBA8" "MK_tools ui atlas tool source"
Assert-ContainsText $toolsTestsText "packed runtime UI atlas authoring maps decoded images into texture page and metadata" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI atlas package update writes texture page metadata and package index" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI atlas rejects invalid decoded images and package path collisions" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI atlas apply leaves existing files unchanged when validation fails" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI glyph atlas authoring maps rasterized glyphs into texture page and metadata" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI glyph atlas package update writes texture page metadata and package index" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI glyph atlas rejects invalid glyph pixels and package path collisions" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI glyph atlas apply leaves existing files unchanged when validation fails" "MK_tools tests"
Assert-ContainsText $uiRendererHeaderText "UiRendererGlyphAtlasBinding" "MK_ui_renderer public header"
Assert-ContainsText $uiRendererHeaderText "UiRendererGlyphAtlasPalette" "MK_ui_renderer public header"
Assert-ContainsText $uiRendererHeaderText "text_glyph_sprites_submitted" "MK_ui_renderer public header"
Assert-ContainsText $uiRendererHeaderText "make_ui_text_glyph_sprite_command" "MK_ui_renderer public header"
Assert-ContainsText $uiRendererHeaderText "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas" "MK_ui_renderer public header"
Assert-ContainsText $uiRendererSourceText "resolve_ui_text_glyph_binding" "MK_ui_renderer source"
Assert-ContainsText $uiRendererSourceText "make_ui_text_glyph_sprite_command" "MK_ui_renderer source"
Assert-ContainsText $uiRendererSourceText "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas" "MK_ui_renderer source"
Assert-ContainsText $uiRendererSourceText "text_glyphs_missing" "MK_ui_renderer source"
Assert-ContainsText $uiRendererTestsText "ui renderer submits monospace text glyphs through glyph atlas palette" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui renderer reports missing glyph atlas bindings without fake sprites" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "CapturingAccessibilityAdapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui accessibility publish plan dispatches validated nodes to adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui accessibility publish plan blocks invalid nodes before adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "CapturingImeAdapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui ime composition publish plan dispatches valid composition to adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui ime composition publish plan blocks invalid composition before adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "CapturingFontRasterizerAdapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "InvalidFontRasterizerAdapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui font rasterization request plan dispatches valid request to adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui font rasterization request plan blocks invalid request before adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui font rasterization result reports invalid adapter allocation" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "CapturingTextShapingAdapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui text shaping request plan dispatches valid request to adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui text shaping request plan blocks invalid request before adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui text shaping result reports invalid adapter runs" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "CapturingImageDecodingAdapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui image decode request plan dispatches valid request to adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui image decode request plan blocks invalid request before adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui image decode result reports missing or invalid adapter output" "MK_ui_renderer tests"

$geRuntimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($geRuntimeModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module"
}
if ($geRuntimeModule[0].status -ne "ready-runtime-resource-v2-safe-point-controller") {
    Write-Error "engine/agent/manifest.json MK_runtime status must advertise the closed Runtime Resource v2 safe-point/controller surface honestly"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentInput")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentInput"
}
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimeInputStateView" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimeInputContextStack" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "GameEngine.RuntimeInputActions.v4" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "bind_gamepad_axis" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "bind_key_in_context" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "bind_pointer_in_context" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "bind_gamepad_button_in_context" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "bind_key_axis_in_context" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimeInputRebindingPresentationModel" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "make_runtime_input_rebinding_presentation" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "capture_runtime_input_rebinding_axis" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "symbolic glyph lookup keys" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "platform input glyph generation" "MK_runtime module purpose"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputStateView" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputContextStack" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "bind_gamepad_button" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "bind_gamepad_axis" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "bind_key_in_context" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "bind_pointer_in_context" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "bind_gamepad_button_in_context" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "bind_key_axis_in_context" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "axis_value" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "UI focus integration" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "global input consumption" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "radial stick deadzones" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "per-device profiles" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputRebindingCaptureRequest" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "capture_runtime_input_rebinding_action" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputRebindingFocusCaptureRequest" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputRebindingFocusCaptureResult" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "capture_runtime_input_rebinding_action_with_focus" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "gameplay_input_consumed" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputRebindingPresentationModel" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "make_runtime_input_rebinding_presentation" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputRebindingAxisCaptureRequest" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "capture_runtime_input_rebinding_axis" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "symbolic glyph lookup keys" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "GameEngine.RuntimeInputRebindingProfile.v1" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "apply_runtime_input_rebinding_profile" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "RuntimeInputRebindingCaptureResult" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "capture_runtime_input_rebinding_action" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "RuntimeInputRebindingFocusCaptureRequest" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "RuntimeInputRebindingFocusCaptureResult" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "capture_runtime_input_rebinding_action_with_focus" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "RuntimeInputRebindingAxisCaptureRequest" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "capture_runtime_input_rebinding_axis" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "gameplay_input_consumed" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "RuntimeInputRebindingPresentationToken" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "present_runtime_input_axis_source" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "platform input glyph generation" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "interactive runtime/game rebinding panels" "input rebinding profile guidance"
foreach ($inputGuidance in @(
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/testing.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".codex/agents/gameplay-builder.toml",
    ".claude/agents/gameplay-builder.md"
)) {
    $inputText = Get-AgentSurfaceText $inputGuidance
    Assert-ContainsText $inputText "RuntimeInputStateView" $inputGuidance
    Assert-ContainsText $inputText "RuntimeInputContextStack" $inputGuidance
    Assert-ContainsText $inputText "bind_gamepad_button" $inputGuidance
    Assert-ContainsText $inputText "GameEngine.RuntimeInputActions.v4" $inputGuidance
    Assert-ContainsText $inputText "bind_gamepad_axis" $inputGuidance
    Assert-ContainsText $inputText "bind_key_in_context" $inputGuidance
    Assert-ContainsText $inputText "bind_pointer_in_context" $inputGuidance
    Assert-ContainsText $inputText "bind_gamepad_button_in_context" $inputGuidance
    Assert-ContainsText $inputText "bind_key_axis_in_context" $inputGuidance
    Assert-ContainsText $inputText "axis_value" $inputGuidance
    Assert-ContainsText $inputText "UI focus integration" $inputGuidance
    Assert-ContainsText $inputText "global input consumption" $inputGuidance
    Assert-ContainsText $inputText "radial stick deadzones" $inputGuidance
    Assert-ContainsText $inputText "per-device profiles" $inputGuidance
}
foreach ($inputRebindingGuidance in @(
    "docs/roadmap.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $inputRebindingText = Get-AgentSurfaceText $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "GameEngine.RuntimeInputRebindingProfile.v1" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "RuntimeInputRebindingProfile" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "apply_runtime_input_rebinding_profile" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "RuntimeInputRebindingCaptureRequest" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "capture_runtime_input_rebinding_action" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "RuntimeInputRebindingFocusCaptureRequest" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "RuntimeInputRebindingFocusCaptureResult" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "capture_runtime_input_rebinding_action_with_focus" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "RuntimeInputRebindingAxisCaptureRequest" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "capture_runtime_input_rebinding_axis" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "gameplay_input_consumed" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "RuntimeInputRebindingPresentationModel" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "make_runtime_input_rebinding_presentation" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "symbolic glyph lookup keys" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "interactive runtime/game rebinding" $inputRebindingGuidance
}

if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentAudio")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentAudio"
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "AudioDeviceStreamRequest" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "AudioDeviceStreamPlan" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "plan_audio_device_stream" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "render_audio_device_stream_interleaved_float" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "device_frame + queued_frames" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "device hotplug/selection" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "mixer authoring" "audio game guidance"
foreach ($audioGuidance in @(
    "docs/architecture.md",
    "docs/current-capabilities.md",
    "docs/roadmap.md",
    "docs/testing.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $audioText = Get-AgentSurfaceText $audioGuidance
    Assert-ContainsText $audioText "AudioDeviceStreamRequest" $audioGuidance
    Assert-ContainsText $audioText "AudioDeviceStreamPlan" $audioGuidance
    Assert-ContainsText $audioText "plan_audio_device_stream" $audioGuidance
    Assert-ContainsText $audioText "render_audio_device_stream_interleaved_float" $audioGuidance
    Assert-ContainsText $audioText "device hotplug/selection" $audioGuidance
    Assert-ContainsText $audioText "mixer authoring" $audioGuidance
}

$geNavigationModule = @($manifest.modules | Where-Object { $_.name -eq "MK_navigation" })
if ($geNavigationModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_navigation module"
}
if ($geNavigationModule[0].status -ne "implemented-production-path-planner") {
    Write-Error "engine/agent/manifest.json MK_navigation status must advertise the production path planner slice honestly"
}
if ($geNavigationModule[0].publicHeaders -notcontains "engine/navigation/include/mirakana/navigation/navigation_replan.hpp") {
    Write-Error "engine/agent/manifest.json MK_navigation publicHeaders must include navigation_replan.hpp"
}
if ($geNavigationModule[0].publicHeaders -notcontains "engine/navigation/include/mirakana/navigation/local_avoidance.hpp") {
    Write-Error "engine/agent/manifest.json MK_navigation publicHeaders must include local_avoidance.hpp"
}
if ($geNavigationModule[0].publicHeaders -notcontains "engine/navigation/include/mirakana/navigation/path_smoothing.hpp") {
    Write-Error "engine/agent/manifest.json MK_navigation publicHeaders must include path_smoothing.hpp"
}
if ($geNavigationModule[0].publicHeaders -notcontains "engine/navigation/include/mirakana/navigation/navigation_path_planner.hpp") {
    Write-Error "engine/agent/manifest.json MK_navigation publicHeaders must include navigation_path_planner.hpp"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentNavigation")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentNavigation"
}
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "validate_navigation_grid_path" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "replan_navigation_grid_path" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "calculate_navigation_local_avoidance" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "smooth_navigation_grid_path" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "NavigationGridAgentPathRequest" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "NavigationGridAgentPathPlan" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "plan_navigation_grid_agent_path" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "navmesh" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "crowd" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "scene/physics integration" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "editor visualization" "MK_navigation module purpose"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "validate_navigation_grid_path" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "replan_navigation_grid_path" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "calculate_navigation_local_avoidance" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "smooth_navigation_grid_path" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "NavigationGridAgentPathRequest" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "NavigationGridAgentPathPlan" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "plan_navigation_grid_agent_path" "navigation game guidance"
foreach ($navigationGuidance in @(
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "docs/ai-game-development.md",
    "docs/specs/generated-game-validation-scenarios.md",
    "docs/specs/game-prompt-pack.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".codex/agents/gameplay-builder.toml",
    ".claude/agents/gameplay-builder.md"
)) {
    $navigationText = Get-AgentSurfaceText $navigationGuidance
    Assert-ContainsText $navigationText "validate_navigation_grid_path" $navigationGuidance
    Assert-ContainsText $navigationText "replan_navigation_grid_path" $navigationGuidance
    Assert-ContainsText $navigationText "calculate_navigation_local_avoidance" $navigationGuidance
    Assert-ContainsText $navigationText "smooth_navigation_grid_path" $navigationGuidance
    Assert-ContainsText $navigationText "NavigationGridAgentPathRequest" $navigationGuidance
    Assert-ContainsText $navigationText "NavigationGridAgentPathPlan" $navigationGuidance
    Assert-ContainsText $navigationText "plan_navigation_grid_agent_path" $navigationGuidance
    Assert-ContainsText $navigationText "navmesh" $navigationGuidance
    Assert-ContainsText $navigationText "crowd" $navigationGuidance
    Assert-ContainsText $navigationText "scene/physics" $navigationGuidance
    Assert-ContainsText $navigationText "editor" $navigationGuidance
}

if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentPhysics")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentPhysics"
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCharacterController3DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCharacterController3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "move_physics_character_controller_3d" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsAuthoredCollisionScene3DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsAuthoredCollisionScene3DBuildResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "build_physics_world_3d_from_authored_collision_scene" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsShape3DDesc::aabb" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsShape3DDesc::sphere" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsShape3DDesc::capsule" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsQueryFilter3D" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsWorld3D::exact_shape_sweep" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsExactSphereCast3DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsExactSphereCast3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsWorld3D::exact_sphere_cast" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsContactPoint3D" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsContactManifold3D" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsWorld3D::contact_manifolds" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "warm-start-safe" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsContinuousStep3DConfig" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsContinuousStep3DRow" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsContinuousStep3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsWorld3D::step_continuous" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCharacterDynamicPolicy3DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCharacterDynamicPolicy3DRowKind" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCharacterDynamicPolicy3DRow" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCharacterDynamicPolicy3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "evaluate_physics_character_dynamic_policy_3d" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsJoint3DStatus" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsDistanceJoint3DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsJointSolve3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "solve_physics_joints_3d" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsReplaySignature3D" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsDeterminismGate3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "make_physics_replay_signature_3d" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "evaluate_physics_determinism_gate_3d" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "first-party Physics 1.0 ready surface" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "Jolt/native backends" "physics game guidance"
foreach ($physicsGuidance in @(
    "docs/architecture.md",
    "docs/current-capabilities.md",
    "docs/roadmap.md",
    "docs/testing.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $physicsText = Get-AgentSurfaceText $physicsGuidance
    Assert-ContainsText $physicsText "move_physics_character_controller_3d" $physicsGuidance
    Assert-ContainsText $physicsText "build_physics_world_3d_from_authored_collision_scene" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsWorld3D::exact_shape_sweep" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsShape3DDesc::aabb" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsShape3DDesc::sphere" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsShape3DDesc::capsule" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsQueryFilter3D" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsWorld3D::exact_sphere_cast" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsWorld3D::contact_manifolds" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsWorld3D::step_continuous" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsCharacterDynamicPolicy3DDesc" $physicsGuidance
    Assert-ContainsText $physicsText "evaluate_physics_character_dynamic_policy_3d" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsJointSolve3DResult" $physicsGuidance
    Assert-ContainsText $physicsText "solve_physics_joints_3d" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsReplaySignature3D" $physicsGuidance
    Assert-ContainsText $physicsText "evaluate_physics_determinism_gate_3d" $physicsGuidance
}
foreach ($physicsUnsupportedGuidance in @(
    "docs/architecture.md",
    "docs/current-capabilities.md",
    "docs/roadmap.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $physicsText = Get-AgentSurfaceText $physicsUnsupportedGuidance
    Assert-ContainsText $physicsText "dynamic-vs-dynamic TOI" $physicsUnsupportedGuidance
    Assert-ContainsText $physicsText "rotational CCD" $physicsUnsupportedGuidance
    Assert-ContainsText $physicsText "2D CCD" $physicsUnsupportedGuidance
    Assert-ContainsText $physicsText "Jolt" $physicsUnsupportedGuidance
}

$geAiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_ai" })
if ($geAiModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_ai module"
}
if ($geAiModule[0].status -ne "implemented-behavior-tree-blackboard-perception-services") {
    Write-Error "engine/agent/manifest.json MK_ai status must advertise the behavior tree blackboard and perception services slice honestly"
}
if ($geAiModule[0].publicHeaders -notcontains "engine/ai/include/mirakana/ai/behavior_tree.hpp") {
    Write-Error "engine/agent/manifest.json MK_ai publicHeaders must include behavior_tree.hpp"
}
if ($geAiModule[0].publicHeaders -notcontains "engine/ai/include/mirakana/ai/perception.hpp") {
    Write-Error "engine/agent/manifest.json MK_ai publicHeaders must include perception.hpp"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentAi")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentAi"
}
Assert-ContainsText ([string]$geAiModule[0].purpose) "BehaviorTreeBlackboard" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "BehaviorTreeBlackboardCondition" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "BehaviorTreeEvaluationContext" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "AiPerceptionAgent2D" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "AiPerceptionTarget2D" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "build_ai_perception_snapshot_2d" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "write_ai_perception_blackboard" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "duplicate-blackboard-key" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "blackboard-type-mismatch" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "persistent blackboard services" "MK_ai module purpose"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "BehaviorTreeBlackboard" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "BehaviorTreeBlackboardCondition" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "BehaviorTreeEvaluationContext" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "AiPerceptionAgent2D" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "AiPerceptionTarget2D" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "build_ai_perception_snapshot_2d" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "write_ai_perception_blackboard" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "blackboard-type-mismatch" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "persistent blackboard services" "AI game guidance"
foreach ($aiApiGuidance in @(
    "docs/architecture.md",
    "docs/testing.md",
    "docs/ai-game-development.md",
    "docs/specs/generated-game-validation-scenarios.md",
    "docs/specs/game-prompt-pack.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".codex/agents/gameplay-builder.toml",
    ".claude/agents/gameplay-builder.md"
)) {
    $aiApiText = Get-AgentSurfaceText $aiApiGuidance
    Assert-ContainsText $aiApiText "BehaviorTreeBlackboard" $aiApiGuidance
    Assert-ContainsText $aiApiText "BehaviorTreeEvaluationContext" $aiApiGuidance
    Assert-ContainsText $aiApiText "AiPerceptionAgent2D" $aiApiGuidance
    Assert-ContainsText $aiApiText "AiPerceptionTarget2D" $aiApiGuidance
    Assert-ContainsText $aiApiText "build_ai_perception_snapshot_2d" $aiApiGuidance
    Assert-ContainsText $aiApiText "write_ai_perception_blackboard" $aiApiGuidance
    Assert-ContainsText $aiApiText "blackboard" $aiApiGuidance
}
foreach ($aiStatusGuidance in @(
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "engine/agent/manifest.json"
)) {
    $aiStatusText = Get-AgentSurfaceText $aiStatusGuidance
    Assert-ContainsText $aiStatusText "Behavior Tree Blackboard Conditions v0" $aiStatusGuidance
    Assert-ContainsText $aiStatusText "AI Perception Services v1" $aiStatusGuidance
    Assert-ContainsText $aiStatusText "persistent blackboard" $aiStatusGuidance
}
foreach ($sampleAiGuidance in @(
    "games/sample_ai_navigation/main.cpp",
    "games/sample_ai_navigation/README.md",
    "games/sample_ai_navigation/game.agent.json"
)) {
    $sampleAiText = Get-AgentSurfaceText $sampleAiGuidance
    Assert-ContainsText $sampleAiText "blackboard" $sampleAiGuidance
}
Assert-ContainsText (Get-AgentSurfaceText "games/sample_ai_navigation/game.agent.json") "behavior-tree-blackboard-perception-services-v1" "sample_ai_navigation manifest"

foreach ($sampleGameplayGuidance in @(
    "games/sample_gameplay_foundation/main.cpp",
    "games/sample_gameplay_foundation/README.md",
    "games/sample_gameplay_foundation/game.agent.json",
    "docs/current-capabilities.md",
    "docs/roadmap.md",
    "docs/ai-game-development.md",
    "engine/agent/manifest.json"
)) {
    $sampleGameplayText = Get-AgentSurfaceText $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "sample_gameplay_foundation" $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "build_physics_world_3d_from_authored_collision_scene" $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "move_physics_character_controller_3d" $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "plan_navigation_grid_agent_path" $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "build_ai_perception_snapshot_2d" $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "write_ai_perception_blackboard" $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "render_audio_device_stream_interleaved_float" $sampleGameplayGuidance
}
Assert-ContainsText (Get-AgentSurfaceText "games/sample_gameplay_foundation/game.agent.json") "headless runtime systems composition proof" "sample_gameplay_foundation manifest"
Assert-ContainsText (Get-AgentSurfaceText "games/sample_gameplay_foundation/README.md") "source-tree headless composition evidence only" "sample_gameplay_foundation README"

$geAnimationModule = @($manifest.modules | Where-Object { $_.name -eq "MK_animation" })
if ($geAnimationModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_animation module"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentAnimation")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentAnimation"
}
$geMathModule = @($manifest.modules | Where-Object { $_.name -eq "MK_math" })
if ($geMathModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_math module"
}
$geMathPublicHeaders = @($geMathModule[0].publicHeaders)
if ($geMathPublicHeaders -notcontains "engine/math/include/mirakana/math/quat.hpp") {
    Write-Error "engine/agent/manifest.json MK_math publicHeaders must include quat.hpp"
}
Assert-ContainsText ([string]$geMathModule[0].purpose) "unit quaternions" "MK_math module purpose"
Assert-ContainsText ([string]$geMathModule[0].purpose) "Mat4::rotation_quat" "MK_math module purpose"
Assert-ContainsText (Get-AgentSurfaceText "engine/math/include/mirakana/math/quat.hpp") "struct Quat" "MK_math quaternion public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/math/include/mirakana/math/mat4.hpp") "rotation_quat" "MK_math Mat4 public header"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationCpuSkinningDesc" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationMorphMeshCpuDesc" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationMorphTargetCpuDesc" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "apply_animation_morph_targets_normals_cpu" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "apply_animation_morph_targets_tangents_cpu" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "sample_animation_morph_weights_at_time" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "solve_animation_two_bone_ik_xy_plane" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "solve_animation_two_bone_ik_3d_orientation" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "solve_animation_fabrik_ik_xy_chain" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "solve_animation_fabrik_ik_3d_chain" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationSkeleton3dDesc" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "build_animation_model_pose_3d" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "apply_animation_fabrik_ik_3d_solution_to_pose" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "sample_quat_keyframes" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationJointTrack3dDesc" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "sample_animation_local_pose_3d" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationIkLocalRotationLimit" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationIkLocalRotationLimit3d" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "apply_animation_local_rotation_limits_3d" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "apply_animation_fabrik_ik_xy_solution_to_pose" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "make_float_animation_tracks_from_f32_bytes" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "apply_float_animation_samples_to_transform3d" "MK_animation module purpose"
$geAnimationPublicHeaders = @($geAnimationModule[0].publicHeaders)
if ($geAnimationPublicHeaders -notcontains "engine/animation/include/mirakana/animation/chain_ik.hpp") {
    Write-Error "engine/agent/manifest.json MK_animation publicHeaders must include chain_ik.hpp"
}
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/keyframe_animation.hpp") "apply_float_animation_samples_to_transform3d" "MK_animation keyframe animation public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/keyframe_animation.hpp") "QuatKeyframe" "MK_animation keyframe animation public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/keyframe_animation.hpp") "sample_quat_keyframes" "MK_animation keyframe animation public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/chain_ik.hpp") "AnimationFabrikIk3dDesc" "MK_animation chain IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/chain_ik.hpp") "solve_animation_fabrik_ik_3d_chain" "MK_animation chain IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/chain_ik.hpp") "apply_animation_fabrik_ik_3d_solution_to_pose" "MK_animation chain IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/chain_ik.hpp") "AnimationIkLocalRotationLimit3d" "MK_animation chain IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/chain_ik.hpp") "apply_animation_local_rotation_limits_3d" "MK_animation chain IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/two_bone_ik.hpp") "AnimationTwoBoneIk3dDesc" "MK_animation two bone IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/two_bone_ik.hpp") "solve_animation_two_bone_ik_3d_orientation" "MK_animation two bone IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/skeleton.hpp") "AnimationSkeleton3dDesc" "MK_animation skeleton public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/skeleton.hpp") "build_animation_model_pose_3d" "MK_animation skeleton public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/skeleton.hpp") "AnimationJointTrack3dDesc" "MK_animation skeleton public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/skeleton.hpp") "sample_animation_local_pose_3d" "MK_animation skeleton public header"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "skin_animation_vertices_cpu" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "make_float_animation_tracks_from_f32_bytes" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "apply_float_animation_samples_to_transform3d" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "import_gltf_node_transform_animation_binding_source" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "import_gltf_node_transform_animation_tracks_3d" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "sample_and_apply_runtime_scene_render_animation_float_clip" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "sample_runtime_morph_mesh_cpu_animation_float_clip" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "solve_animation_two_bone_ik_3d_orientation" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "solve_animation_fabrik_ik_xy_chain" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "solve_animation_fabrik_ik_3d_chain" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "AnimationSkeleton3dDesc" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "build_animation_model_pose_3d" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "apply_animation_fabrik_ik_3d_solution_to_pose" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "sample_quat_keyframes" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "AnimationJointTrack3dDesc" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "sample_animation_local_pose_3d" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "AnimationIkLocalRotationLimit" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "AnimationIkLocalRotationLimit3d" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "apply_animation_local_rotation_limits_3d" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "apply_animation_fabrik_ik_xy_solution_to_pose" "animation game guidance"
foreach ($animationGuidance in @(
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/testing.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".codex/agents/engine-architect.toml",
    ".claude/agents/engine-architect.md",
    ".codex/agents/gameplay-builder.toml",
    ".claude/agents/gameplay-builder.md"
)) {
    $animationText = Get-AgentSurfaceText $animationGuidance
    Assert-ContainsText $animationText "Animation CPU Skinning" $animationGuidance
}
foreach ($animationApiGuidance in @(
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".codex/agents/engine-architect.toml",
    ".claude/agents/engine-architect.md",
    ".codex/agents/gameplay-builder.toml",
    ".claude/agents/gameplay-builder.md"
)) {
    $animationApiText = Get-AgentSurfaceText $animationApiGuidance
    Assert-ContainsText $animationApiText "skin_animation_vertices_cpu" $animationApiGuidance
    Assert-ContainsText $animationApiText "solve_animation_two_bone_ik_3d_orientation" $animationApiGuidance
    Assert-ContainsText $animationApiText "solve_animation_fabrik_ik_3d_chain" $animationApiGuidance
    Assert-ContainsText $animationApiText "AnimationSkeleton3dDesc" $animationApiGuidance
    Assert-ContainsText $animationApiText "build_animation_model_pose_3d" $animationApiGuidance
    Assert-ContainsText $animationApiText "apply_animation_fabrik_ik_3d_solution_to_pose" $animationApiGuidance
    Assert-ContainsText $animationApiText "sample_quat_keyframes" $animationApiGuidance
    Assert-ContainsText $animationApiText "AnimationJointTrack3dDesc" $animationApiGuidance
    Assert-ContainsText $animationApiText "sample_animation_local_pose_3d" $animationApiGuidance
    Assert-ContainsText $animationApiText "import_gltf_node_transform_animation_tracks_3d" $animationApiGuidance
    Assert-ContainsText $animationApiText "AnimationIkLocalRotationLimit3d" $animationApiGuidance
    Assert-ContainsText $animationApiText "apply_animation_local_rotation_limits_3d" $animationApiGuidance
}

$geRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($geRhiModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module"
}
$geRendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($geRendererModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module"
}
$geRuntimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($geRuntimeRhiModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module"
}
$geSceneRendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_scene_renderer" })
if ($geSceneRendererModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_scene_renderer module"
}
Assert-ContainsText ([string]$geRhiModule[0].purpose) "RenderPassDepthAttachment" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "DepthStencilStateDesc" "MK_rhi module purpose"
if (@($geRhiModule[0].publicHeaders) -notcontains "engine/rhi/include/mirakana/rhi/resource_lifetime.hpp") {
    Write-Error "engine/agent/manifest.json MK_rhi publicHeaders must include resource_lifetime.hpp"
}
if (@($geRhiModule[0].publicHeaders) -notcontains "engine/rhi/include/mirakana/rhi/upload_staging.hpp") {
    Write-Error "engine/agent/manifest.json MK_rhi publicHeaders must include upload_staging.hpp"
}
Assert-ContainsText ([string]$geRhiModule[0].purpose) "RhiResourceLifetimeRegistry" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "RhiUploadStagingPlan" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "FenceValue" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "foundation-only" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "GPU allocator" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph v1 foundation-only" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphV1Desc" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "barrier intent" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph Pass Callback Execution v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "execute_frame_graph_v1_schedule" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph RHI Texture Schedule Execution v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph Render Pass Envelope v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphRhiTextureExecutionDesc" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphTexturePassTargetAccess" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "build_frame_graph_texture_pass_target_accesses" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphTexturePassTargetState" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "pass_target_state_barriers_recorded" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphTextureFinalState" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "final_state_barriers_recorded" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "aliasing_barriers_recorded" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphRhiRenderPassDesc" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "render_passes_recorded" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "automatic aliasing-barrier insertion" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "execute_frame_graph_rhi_texture_schedule" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph Transient Texture Alias Planning v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphTransientTextureAliasPlan" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "plan_frame_graph_transient_texture_aliases" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph Backend-Neutral Distinct Alias-Group Lease Binding v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "IRhiDevice::acquire_transient_texture_alias_group" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "distinct resource-name FrameGraphTextureBinding rows" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "acquire_frame_graph_transient_texture_lease_bindings" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph Texture Aliasing Barrier Command v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphTextureAliasingBarrier" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "record_frame_graph_texture_aliasing_barriers" "MK_renderer module purpose"
if (@($geRuntimeRhiModule[0].publicHeaders) -notcontains "engine/runtime_rhi/include/mirakana/runtime_rhi/package_streaming_frame_graph.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime_rhi publicHeaders must include package_streaming_frame_graph.hpp"
}
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "make_runtime_package_streaming_frame_graph_texture_bindings" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RuntimePackageStreamingFrameGraphTextureBindingSource" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "broad/background package streaming" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dPackageStreamingSafePointSmoke) "make_runtime_package_streaming_frame_graph_texture_bindings" "desktop runtime 3d package streaming guidance"
foreach ($packageStreamingFrameGraphGuidance in @(
        "docs/rhi.md",
        "docs/current-capabilities.md",
        "docs/ai-game-development.md",
        "docs/architecture.md",
        ".agents/skills/rendering-change/references/full-guidance.md",
        ".claude/skills/gameengine-rendering/references/full-guidance.md",
        ".codex/agents/rendering-auditor.toml",
        ".claude/agents/rendering-auditor.md"
    )) {
    $packageStreamingFrameGraphText = Get-AgentSurfaceText $packageStreamingFrameGraphGuidance
    Assert-ContainsText $packageStreamingFrameGraphText "make_runtime_package_streaming_frame_graph_texture_bindings" $packageStreamingFrameGraphGuidance
    Assert-ContainsText $packageStreamingFrameGraphText "broad" $packageStreamingFrameGraphGuidance
}
Assert-ContainsText ([string]$geRhiModule[0].purpose) "IRhiCommandList::texture_aliasing_barrier" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "RhiStats::texture_aliasing_barriers" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "RhiFrameRenderer executor-owned primary_color pass timing" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "primary_color pass timing and render pass envelope" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "RhiFrameRenderer primary_color texture binding and target-state evidence" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "optional primary_depth depth target-state" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "RhiPostprocessFrameRenderer executor-owned scene-color/scene-depth target preparation plus executor-owned scene/postprocess-chain render pass envelopes, pass-body callback recording, and post-chain/final-state transition adoption" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "RhiDirectionalShadowSmokeFrameRenderer scheduled shadow-depth/scene-color/scene-depth inter-pass plus shadow-color/shadow-depth/scene-color/scene-depth writer-access-backed target-state, executor-owned shadow-depth/scene-receiver/postprocess render pass envelope, and scene-depth/shadow-depth final-state transition adoption" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "RhiViewportSurface viewport_color render_target/copy_source/shader_read color-state transitions through execute_frame_graph_rhi_texture_schedule plus executor-owned viewport.clear render pass envelope" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph RHI Queue Dependency Plan v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "plan_frame_graph_rhi_queue_waits" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "IRhiDevice::wait_for_queue" "MK_renderer module purpose"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph Remaining Render Pass Envelopes v1" "recommended next plan completed context"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph Primary Pass Target-State Evidence v1" "recommended next plan completed context"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "Frame Graph RHI Queue Dependency Plan v1" "recommended next plan completed context"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) "record_frame_graph_rhi_queue_waits" "recommended next plan completed context"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) "primary pass texture-binding/target-state evidence is complete" "recommended next plan reason"
Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) "backend-neutral RHI queue dependency wait planning/recording is complete" "recommended next plan reason"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "RHI Depth Attachment Contract v0" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Stable Directional Light-Space Policy v0" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "DirectionalShadowLightSpacePlan" "MK_renderer module purpose"
Assert-ContainsText ([string]$geSceneRendererModule[0].purpose) "build_scene_directional_shadow_light_space_plan" "MK_scene_renderer module purpose"
Assert-ContainsText ([string]$geSceneRendererModule[0].purpose) "sample_and_apply_runtime_scene_render_animation_float_clip" "MK_scene_renderer module purpose"
Assert-ContainsText ([string]$geSceneRendererModule[0].purpose) "sample_runtime_morph_mesh_cpu_animation_float_clip" "MK_scene_renderer module purpose"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRhi) "RHI Depth Attachment Contract v0" "RHI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRhi) "RenderPassDepthAttachment" "RHI game guidance"
foreach ($depthGuidance in @(
    "docs/rhi.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md",
    ".codex/agents/rendering-auditor.toml",
    ".claude/agents/rendering-auditor.md"
)) {
    $depthText = Get-AgentSurfaceText $depthGuidance
    Assert-ContainsText $depthText "RHI Depth Attachment Contract v0" $depthGuidance
}
foreach ($sampledDepthGuidance in @(
    "docs/testing.md",
    "docs/rhi.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md",
    ".codex/agents/rendering-auditor.toml",
    ".claude/agents/rendering-auditor.md"
)) {
    $sampledDepthText = Get-AgentSurfaceText $sampledDepthGuidance
    Assert-ContainsText $sampledDepthText "MK_VULKAN_TEST_DEPTH_VERTEX_SPV" $sampledDepthGuidance
    Assert-ContainsText $sampledDepthText "MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV" $sampledDepthGuidance
    Assert-ContainsText $sampledDepthText "MK_VULKAN_TEST_DEPTH_SAMPLE_VERTEX_SPV" $sampledDepthGuidance
    Assert-ContainsText $sampledDepthText "MK_VULKAN_TEST_DEPTH_SAMPLE_FRAGMENT_SPV" $sampledDepthGuidance
    Assert-ContainsText $sampledDepthText "sampled-depth readback" $sampledDepthGuidance
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_DEPTH_VERTEX_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_DEPTH_SAMPLE_VERTEX_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_DEPTH_SAMPLE_FRAGMENT_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "sampled-depth readback" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_COMPUTE_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_COMPUTE_MORPH_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "create_runtime_compute_pipeline" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "record_runtime_compute_dispatch" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkanRuntimeOwners) "VulkanRuntimeComputePipeline" "Vulkan runtime owner guidance"
$vulkanBackendSource = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_backend.cpp"
Assert-ContainsText $vulkanBackendSource "refresh_surface_probe_queue_family_snapshots" "Vulkan surface support implementation"
Assert-ContainsText $vulkanBackendSource "same native instance handles" "Vulkan surface support implementation"
$frameGraphRhiHeader = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp"
$frameGraphRhiSource = Get-AgentSurfaceText "engine/renderer/src/frame_graph_rhi.cpp"
$rhiFrameRendererSource = Get-AgentSurfaceText "engine/renderer/src/rhi_frame_renderer.cpp"
$rhiPostprocessSource = Get-AgentSurfaceText "engine/renderer/src/rhi_postprocess_frame_renderer.cpp"
$rhiDirectionalShadowSource = Get-AgentSurfaceText "engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp"
$rhiViewportSurfaceSource = Get-AgentSurfaceText "engine/renderer/src/rhi_viewport_surface.cpp"
Assert-ContainsText $frameGraphRhiHeader "FrameGraphTexturePassTargetAccess" "Frame graph RHI pass target access public API"
Assert-ContainsText $frameGraphRhiHeader "build_frame_graph_texture_pass_target_accesses" "Frame graph RHI pass target access public API"
Assert-ContainsText $frameGraphRhiHeader "std::span<const FrameGraphTexturePassTargetAccess> pass_target_accesses" "Frame graph RHI pass target access public API"
Assert-ContainsText $frameGraphRhiHeader "FrameGraphTransientTextureLeaseBindingResult" "Frame graph transient texture lease binding public API"
Assert-ContainsText $frameGraphRhiHeader "acquire_frame_graph_transient_texture_lease_bindings" "Frame graph transient texture lease binding public API"
Assert-ContainsText $frameGraphRhiHeader "release_frame_graph_transient_texture_lease_bindings" "Frame graph transient texture lease binding public API"
Assert-ContainsText $frameGraphRhiHeader "rhi::TransientTextureAliasGroup transient_alias_group" "Frame graph transient texture lease binding public API"
Assert-ContainsText $frameGraphRhiHeader "FrameGraphTextureAliasingBarrier" "Frame graph texture aliasing barrier command public API"
Assert-ContainsText $frameGraphRhiHeader "FrameGraphTextureAliasingBarrierRecordResult" "Frame graph texture aliasing barrier command public API"
Assert-ContainsText $frameGraphRhiHeader "record_frame_graph_texture_aliasing_barriers" "Frame graph texture aliasing barrier command public API"
Assert-ContainsText $rhiPublicHeaderText "texture_aliasing_barriers" "RHI texture aliasing barrier command public API"
Assert-ContainsText $rhiPublicHeaderText "texture_aliasing_barrier(TextureHandle before, TextureHandle after)" "RHI texture aliasing barrier command public API"
Assert-ContainsText $rhiPublicHeaderText "TransientTextureAliasGroup" "RHI transient texture alias-group public API"
Assert-ContainsText $rhiPublicHeaderText "std::vector<TextureHandle> textures" "RHI transient texture alias-group public API"
Assert-ContainsText $rhiPublicHeaderText "acquire_transient_texture_alias_group(const TextureDesc& desc" "RHI transient texture alias-group public API"
Assert-ContainsText $rhiPublicHeaderText "transient_texture_heap_allocations" "RHI D3D12 placed transient texture lease evidence"
Assert-ContainsText $rhiPublicHeaderText "transient_texture_placed_allocations" "RHI D3D12 placed transient texture lease evidence"
Assert-ContainsText $rhiPublicHeaderText "transient_texture_placed_resources_alive" "RHI D3D12 placed transient texture lease evidence"
Assert-ContainsText $nullRhiSourceText "void texture_aliasing_barrier(TextureHandle before, TextureHandle after) override" "NullRHI texture aliasing barrier command"
Assert-ContainsText $nullRhiSourceText "NullRhiDevice::acquire_transient_texture_alias_group" "NullRHI transient texture alias-group lease"
$d3d12RhiHeaderText = Get-AgentSurfaceText "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "null_resource_aliasing_barriers" "D3D12 texture aliasing barrier command evidence"
Assert-ContainsText $d3d12RhiHeaderText "placed_texture_heaps_created" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiHeaderText "placed_texture_alias_groups_created" "D3D12 placed alias group evidence"
Assert-ContainsText $d3d12RhiHeaderText "placed_textures_created" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiHeaderText "placed_resources_alive" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiHeaderText "placed_resource_activation_barriers" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiHeaderText "placed_resource_aliasing_barriers" "D3D12 placed alias group evidence"
Assert-ContainsText $d3d12RhiHeaderText "create_placed_texture" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiHeaderText "create_placed_texture_alias_group" "D3D12 placed alias group evidence"
Assert-ContainsText $d3d12RhiHeaderText "activate_placed_texture" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiSourceText "backend-private" "D3D12 texture aliasing barrier command"
Assert-ContainsText $d3d12RhiSourceText "null-resource aliasing barrier" "D3D12 texture aliasing barrier command"
Assert-ContainsText $d3d12RhiSourceText "D3D12_RESOURCE_BARRIER_TYPE_ALIASING" "D3D12 texture aliasing barrier command"
Assert-ContainsText $d3d12RhiSourceText "CreateHeap" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiSourceText "CreatePlacedResource" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiSourceText "GetResourceAllocationInfo" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiSourceText "D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiSourceText "pResourceAfter = texture_resource" "D3D12 placed transient texture activation evidence"
Assert-ContainsText $d3d12RhiSourceText "resource_alias_group_ids" "D3D12 placed alias group evidence"
Assert-ContainsText $d3d12RhiSourceText "resources_share_placed_alias_group" "D3D12 placed alias group evidence"
Assert-ContainsText $d3d12RhiSourceText "create_placed_texture_alias_group(desc, texture_count)" "D3D12 transient texture alias-group lease"
Assert-ContainsText $d3d12RhiSourceText "pResourceBefore = before_resource" "D3D12 placed alias group non-null barrier evidence"
Assert-ContainsText $d3d12RhiSourceText "pResourceAfter = after_resource" "D3D12 placed alias group non-null barrier evidence"
Assert-ContainsText $d3d12RhiSourceText "placed_resource_state_updates" "D3D12 placed alias group submit-time state evidence"
Assert-ContainsText $vulkanRhiSourceText "record_runtime_texture_aliasing_barrier" "Vulkan texture aliasing barrier command"
Assert-ContainsText $vulkanRhiSourceText "acquire_transient_texture_alias_group" "Vulkan transient texture alias-group fallback"
Assert-ContainsText $frameGraphRhiSource "frame graph transient texture alias group acquisition failed" "Frame graph transient texture lease binding failure cleanup"
Assert-ContainsText $frameGraphRhiSource "device.acquire_transient_texture_alias_group(group.desc, group.resources.size())" "Frame graph transient texture distinct alias-group lease binding"
Assert-ContainsText $frameGraphRhiSource "frame graph transient texture alias group has no resources" "Frame graph transient texture lease binding malformed-plan validation"
Assert-ContainsText $frameGraphRhiSource "frame graph transient texture alias group resource name is empty" "Frame graph transient texture lease binding malformed-plan validation"
Assert-ContainsText $frameGraphRhiSource "frame graph transient texture alias group returned an invalid texture count" "Frame graph transient texture lease binding backend validation"
Assert-ContainsText $frameGraphRhiSource "frame graph transient texture alias group returned duplicate texture handles" "Frame graph transient texture lease binding backend validation"
Assert-ContainsText $frameGraphRhiSource "release_frame_graph_transient_texture_lease_bindings" "Frame graph transient texture lease binding failure cleanup"
Assert-ContainsText $frameGraphRhiSource "propagate_shared_simulated_texture_state" "Frame graph shared TextureHandle state handoff"
Assert-ContainsText $frameGraphRhiSource "propagate_shared_texture_binding_state" "Frame graph shared TextureHandle state handoff"
Assert-ContainsText $frameGraphRhiSource "frame graph texture bindings sharing a handle disagree on current state" "Frame graph shared TextureHandle state handoff"
Assert-ContainsText $frameGraphRhiSource "record_frame_graph_texture_aliasing_barriers" "Frame graph texture aliasing barrier command"
Assert-ContainsText $frameGraphRhiSource "texture_aliasing_barrier" "Frame graph texture aliasing barrier command"
Assert-ContainsText $frameGraphRhiSource "frame graph texture aliasing barrier requires distinct texture handles" "Frame graph texture aliasing barrier command"
Assert-ContainsText $frameGraphRhiSource "frame graph texture pass target state has no declared writer access" "Frame graph RHI pass target access validation"
Assert-ContainsText $frameGraphRhiSource "frame graph texture pass target state disagrees with writer access" "Frame graph RHI pass target access validation"
Assert-ContainsText $frameGraphRhiSource "frame graph texture pass target access is declared more than once" "Frame graph RHI pass target access validation"
Assert-ContainsText $frameGraphRhiSource "FrameGraphRhiRenderPassDesc" "Frame graph render pass envelope"
Assert-ContainsText $frameGraphRhiSource "begin_planned_render_pass" "Frame graph render pass envelope"
Assert-ContainsText $frameGraphRhiSource "attachment references an unknown resource" "Frame graph render pass envelope validation"
Assert-ContainsText $rhiFrameRendererSource "PrimaryColorFrameGraphExecutionPlan" "RhiFrameRenderer primary target-state evidence"
Assert-ContainsText $rhiFrameRendererSource "build_frame_graph_texture_pass_target_accesses" "RhiFrameRenderer primary target-state evidence"
Assert-ContainsText $rhiFrameRendererSource ".pass_target_states = pass_target_states" "RhiFrameRenderer primary target-state evidence"
Assert-ContainsText $rhiFrameRendererSource "primary_depth" "RhiFrameRenderer primary depth target-state evidence"
$rendererRhiTests = Get-AgentSurfaceText "tests/unit/renderer_rhi_tests.cpp"
Assert-ContainsText $rendererRhiTests "frame graph v1 texture barrier recording propagates shared texture handle state" "Frame graph shared TextureHandle state handoff tests"
Assert-ContainsText $rendererRhiTests "frame graph v1 texture barrier recording rejects conflicting shared texture handle states" "Frame graph shared TextureHandle state handoff tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi texture schedule execution hands off shared texture handle state between aliases" "Frame graph shared TextureHandle state handoff tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi texture aliasing barrier recording maps resource names to texture handles" "Frame graph texture aliasing barrier command tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi texture aliasing barrier recording rejects missing resources and shared handles" "Frame graph texture aliasing barrier command tests"
Assert-ContainsText $rendererRhiTests "rhi frame renderer carries primary target state across texture frames" "RhiFrameRenderer primary target-state evidence tests"
Assert-ContainsText $rendererRhiTests "rhi frame renderer reports primary target state failures before pass body" "RhiFrameRenderer primary target-state evidence tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi transient texture lease binding acquires distinct handles per alias group resource" "Frame graph transient texture distinct alias-group lease tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi transient texture lease binding rejects duplicate backend alias handles" "Frame graph transient texture distinct alias-group lease tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi texture schedule execution wraps render pass envelopes around callbacks" "Frame graph render pass envelope tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi texture schedule execution rejects invalid render pass envelopes before callbacks" "Frame graph render pass envelope tests"
Assert-ContainsText $rhiTestsText "null rhi records texture aliasing barriers without changing texture state" "NullRHI texture aliasing barrier command tests"
Assert-ContainsText $rhiTestsText "null rhi transient texture alias group returns distinct handles under one lease" "NullRHI transient texture alias-group lease tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi device records texture aliasing barrier commands" "D3D12 texture aliasing barrier command tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi device transient texture alias group returns distinct placed handles" "D3D12 transient texture alias-group lease tests"
Assert-ContainsText $d3d12RhiTestsText "null_resource_aliasing_barriers" "D3D12 texture aliasing barrier command evidence tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 device context creates placed transient texture resources" "D3D12 placed transient texture lease tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 device context keeps unrelated placed texture aliasing barriers conservative" "D3D12 placed alias group tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 device context records non null placed resource aliasing barriers" "D3D12 placed alias group tests"
Assert-ContainsText $d3d12RhiTestsText "placed_resource_activation_barriers" "D3D12 placed transient texture activation tests"
Assert-ContainsText $d3d12RhiTestsText "placed_texture_alias_groups_created" "D3D12 placed alias group tests"
Assert-ContainsText $d3d12RhiTestsText "placed_resource_aliasing_barriers" "D3D12 placed alias group tests"
Assert-ContainsText $d3d12RhiTestsText "transient_texture_heap_allocations" "D3D12 placed transient texture lease tests"
Assert-ContainsText $d3d12RhiTestsText "transient_texture_placed_allocations" "D3D12 placed transient texture lease tests"
Assert-ContainsText $d3d12RhiTestsText "transient_texture_placed_resources_alive" "D3D12 placed transient texture release tests"
Assert-ContainsText $backendScaffoldTestsText "vulkan rhi device bridge records texture aliasing barrier" "Vulkan texture aliasing barrier command tests"
Assert-ContainsText $rhiFrameRendererSource "execute_frame_graph_rhi_texture_schedule" "RHI frame renderer primary pass ownership"
Assert-ContainsText $rhiFrameRendererSource "primary_color" "RHI frame renderer primary pass ownership"
Assert-ContainsText $rhiFrameRendererSource "framegraph_passes_executed" "RHI frame renderer primary pass ownership"
Assert-ContainsText $rhiFrameRendererSource "record_queued_mesh_command(draw.mesh, recorded_primary_stats)" "RHI frame renderer primary pass ownership"
Assert-ContainsText $rhiFrameRendererSource "FrameGraphRhiRenderPassDesc" "RHI frame renderer primary render pass envelope"
Assert-ContainsText $rhiFrameRendererSource ".render_passes = render_passes" "RHI frame renderer primary render pass envelope"
Assert-DoesNotContainText $rhiFrameRendererSource "commands_->begin_render_pass" "RHI frame renderer primary render pass envelope"
Assert-DoesNotContainText $rhiFrameRendererSource "commands_->end_render_pass" "RHI frame renderer primary render pass envelope"
$beginFrameFunctionMatch = [regex]::Match(
    $rhiFrameRendererSource,
    '(?s)void RhiFrameRenderer::begin_frame\(\)(?<body>.*?)\r?\nvoid RhiFrameRenderer::draw_sprite'
)
if (-not $beginFrameFunctionMatch.Success) {
    Write-Error "RhiFrameRenderer::begin_frame body could not be located for primary pass ownership checks"
}
$beginFrameFunctionBody = $beginFrameFunctionMatch.Groups["body"].Value
Assert-DoesNotContainText $beginFrameFunctionBody "begin_render_pass" "RHI frame renderer primary pass ownership"
Assert-DoesNotContainText $rhiPostprocessSource "void RhiPostprocessFrameRenderer::draw_sprite(const SpriteCommand&) {`r`n    require_active_frame();`r`n    commands_->draw(3, 1);" "RHI postprocess sprite submission"
Assert-DoesNotContainText $rhiPostprocessSource "void RhiPostprocessFrameRenderer::draw_sprite(const SpriteCommand&) {`n    require_active_frame();`n    commands_->draw(3, 1);" "RHI postprocess sprite submission"
Assert-ContainsText $rhiPostprocessSource "execute_frame_graph_rhi_texture_schedule" "RHI postprocess frame graph RHI execution"
Assert-ContainsText $rhiPostprocessSource "build_frame_graph_texture_pass_target_accesses" "RHI postprocess frame graph pass target access execution"
Assert-ContainsText $rhiPostprocessSource ".pass_target_accesses = postprocess_frame_graph_target_accesses_" "RHI postprocess frame graph pass target access execution"
Assert-ContainsText $rhiPostprocessSource "pending_meshes_" "RHI postprocess scene pass ownership"
Assert-ContainsText $rhiPostprocessSource "record_scene_pass_body" "RHI postprocess scene pass ownership"
Assert-ContainsText $rhiPostprocessSource ".pass_name = `"scene_color`"" "RHI postprocess scene pass ownership"
Assert-ContainsText $rhiPostprocessSource ".resource = `"scene_depth`"" "RHI postprocess scene pass ownership"
Assert-ContainsText $rhiPostprocessSource "FrameGraphRhiRenderPassDesc" "RHI postprocess frame graph render pass envelope"
Assert-ContainsText $rhiPostprocessSource ".render_passes = render_passes" "RHI postprocess frame graph render pass envelope"
Assert-DoesNotContainText $rhiPostprocessSource "commands_->begin_render_pass" "RHI postprocess frame graph render pass envelope"
Assert-DoesNotContainText $rhiPostprocessSource "commands_->end_render_pass" "RHI postprocess frame graph render pass envelope"
Assert-ContainsText $rhiPostprocessSource "FrameGraphTexturePassTargetState" "RHI postprocess frame graph pass target-state execution"
Assert-ContainsText $rhiPostprocessSource ".pass_target_states = pass_target_states" "RHI postprocess frame graph pass target-state execution"
Assert-ContainsText $rhiPostprocessSource "FrameGraphTextureFinalState" "RHI postprocess frame graph final-state execution"
Assert-ContainsText $rhiPostprocessSource ".final_states = final_states" "RHI postprocess frame graph final-state execution"
Assert-ContainsText $rhiPostprocessSource "frame_graph_execution.barriers_recorded" "RHI postprocess frame graph RHI execution"
Assert-ContainsText $rhiPostprocessSource "frame_graph_execution.pass_callbacks_invoked" "RHI postprocess frame graph RHI execution"
Assert-ContainsText $rhiDirectionalShadowSource "execute_frame_graph_rhi_texture_schedule" "RHI directional shadow frame graph RHI execution"
Assert-ContainsText $rhiDirectionalShadowSource "build_frame_graph_texture_pass_target_accesses" "RHI directional shadow frame graph pass target access execution"
Assert-ContainsText $rhiDirectionalShadowSource ".pass_target_accesses = shadow_smoke_frame_graph_target_accesses_" "RHI directional shadow frame graph pass target access execution"
Assert-ContainsText $rhiDirectionalShadowSource "FrameGraphRhiRenderPassDesc" "RHI directional shadow frame graph render pass envelope"
Assert-ContainsText $rhiDirectionalShadowSource ".render_passes = render_passes" "RHI directional shadow frame graph render pass envelope"
Assert-DoesNotContainText $rhiDirectionalShadowSource "commands_->begin_render_pass" "RHI directional shadow frame graph render pass envelope"
Assert-DoesNotContainText $rhiDirectionalShadowSource "commands_->end_render_pass" "RHI directional shadow frame graph render pass envelope"
Assert-ContainsText $rhiDirectionalShadowSource "FrameGraphTexturePassTargetState" "RHI directional shadow frame graph pass target-state execution"
Assert-ContainsText $rhiDirectionalShadowSource ".pass_target_states = pass_target_states" "RHI directional shadow frame graph pass target-state execution"
Assert-ContainsText $rhiDirectionalShadowSource ".resource = `"shadow_color`"" "RHI directional shadow shadow_color pass target-state execution"
Assert-ContainsText $rhiDirectionalShadowSource ".access = FrameGraphAccess::color_attachment_write" "RHI directional shadow shadow_color writer access"
if ($rhiDirectionalShadowSource -notmatch '(?s)FrameGraphResourceAccess\{\s*\.resource\s*=\s*"shadow_color",\s*\.access\s*=\s*FrameGraphAccess::color_attachment_write\s*\}') {
    Write-Error "RHI directional shadow shadow_color writer access must be a declared color_attachment_write row"
}
if ($rhiDirectionalShadowSource -notmatch '(?s)FrameGraphTexturePassTargetState\{\s*\.pass_name\s*=\s*"shadow\.directional\.depth",\s*\.resource\s*=\s*"shadow_color",\s*\.state\s*=\s*rhi::ResourceState::render_target,?\s*\}') {
    Write-Error "RHI directional shadow shadow_color target-state row must prepare shadow.directional.depth as render_target"
}
Assert-DoesNotContainText $rhiDirectionalShadowSource "transition_texture(shadow_color_texture_" "RHI directional shadow shadow_color target-state ownership"
Assert-ContainsText $rhiDirectionalShadowSource "FrameGraphTextureFinalState" "RHI directional shadow frame graph final-state execution"
Assert-ContainsText $rhiDirectionalShadowSource ".final_states = final_states" "RHI directional shadow frame graph final-state execution"
Assert-ContainsText $rhiDirectionalShadowSource "frame_graph_execution.barriers_recorded" "RHI directional shadow frame graph RHI execution"
Assert-ContainsText $rhiDirectionalShadowSource "frame_graph_execution.pass_callbacks_invoked" "RHI directional shadow frame graph RHI execution"
Assert-DoesNotContainText $rhiDirectionalShadowSource "pending_sprites_" "RHI directional shadow sprite submission"
Assert-ContainsText $rhiViewportSurfaceSource "execute_frame_graph_rhi_texture_schedule" "RHI viewport surface frame graph color state execution"
Assert-ContainsText $rhiViewportSurfaceSource "FrameGraphRhiRenderPassDesc" "RHI viewport surface frame graph render pass envelope"
Assert-ContainsText $rhiViewportSurfaceSource ".render_passes = std::span<const FrameGraphRhiRenderPassDesc>{render_passes}" "RHI viewport surface frame graph render pass envelope"
Assert-DoesNotContainText $rhiViewportSurfaceSource "commands->begin_render_pass" "RHI viewport surface frame graph render pass envelope"
Assert-DoesNotContainText $rhiViewportSurfaceSource "commands->end_render_pass" "RHI viewport surface frame graph render pass envelope"
Assert-ContainsText $rhiViewportSurfaceSource "FrameGraphTextureFinalState" "RHI viewport surface frame graph color final state execution"
Assert-ContainsText $rhiViewportSurfaceSource ".resource = `"viewport_color`"" "RHI viewport surface frame graph color final state execution"
Assert-ContainsText $rhiViewportSurfaceSource ".final_states = std::span<const FrameGraphTextureFinalState>{final_states}" "RHI viewport surface frame graph color final state execution"
Assert-ContainsText $rhiViewportSurfaceSource "color_state_ = texture_bindings.front().current_state" "RHI viewport surface recorded color state adoption"
Assert-DoesNotContainText $rhiViewportSurfaceSource "transition_texture(" "RHI viewport surface high-level color transition ownership"
foreach ($renderingGuidancePath in @(
    ".agents/skills/rendering-change/references/full-guidance.md",
    ".claude/skills/gameengine-rendering/references/full-guidance.md"
)) {
    $renderingGuidanceText = Get-AgentSurfaceText $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "declared shadow-color/shadow-depth/scene-color/scene-depth writer-access-backed target-state preparation" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "viewport color-state executor slices" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "FrameGraphTransientTextureLeaseBindingResult" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "one backend-neutral ``IRhiDevice::acquire_transient_texture_alias_group`` lease per alias group" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "distinct texture handles in group resource order" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "shared-handle state-handoff aware" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "conflicting initial shared-handle states" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "FrameGraphTextureAliasingBarrier" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "record_frame_graph_texture_aliasing_barriers" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "same alias-group placed pairs" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "same-offset placed textures" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "after ``ExecuteCommandLists`` submits work rather than after fence completion" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "automatic aliasing barrier before the first pass" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "aliasing_barriers_recorded" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "Frame Graph RHI queue dependency work" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "plan_frame_graph_rhi_queue_waits" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "IRhiDevice::wait_for_queue" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "wildcard/null public aliasing barriers" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "data inheritance/content preservation" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "FrameGraphRhiRenderPassDesc`` envelope around the ``primary_color`` callback" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "viewport.clear`` render pass envelopes" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "direct render pass begin/end APIs" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText 'engine/renderer/src/rhi_viewport_surface.cpp` must not call `transition_texture(' $renderingGuidancePath
    Assert-DoesNotContainText $renderingGuidanceText "declared shadow-depth/scene-color/scene-depth writer-access-backed target-state preparation" $renderingGuidancePath
    Assert-DoesNotContainText $renderingGuidanceText "Treat those bindings as acquisition output only until a separate alias-aware executor state handoff/barrier slice exists" $renderingGuidancePath
}
foreach ($renderingAuditorPath in @(
    ".codex/agents/rendering-auditor.toml",
    ".claude/agents/rendering-auditor.md"
)) {
    $renderingAuditorText = Get-AgentSurfaceText $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "FrameGraphTransientTextureLeaseBindingResult" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "acquire_frame_graph_transient_texture_lease_bindings" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "IRhiDevice::acquire_transient_texture_alias_group" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "distinct-handle binding rows" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "shared-handle executor state handoff" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "conflicting initial shared-handle state rejection" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "FrameGraphTextureAliasingBarrier" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "record_frame_graph_texture_aliasing_barriers" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "same alias-group placed pairs" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "same-offset placed textures" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "ExecuteCommandLists" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "submits work rather than after fence completion" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "automatic executor aliasing barriers" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "aliasing_barriers_recorded" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "wildcard/null public aliasing barriers" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "primary_color" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "render pass envelope" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "viewport.clear" $renderingAuditorPath
    Assert-ContainsText $renderingAuditorText "direct render pass begin/end" $renderingAuditorPath
    Assert-DoesNotContainText $renderingAuditorText "treating the current executor as alias-aware" $renderingAuditorPath
}
foreach ($postprocessDepthGuidance in @(
    "docs/testing.md",
    "docs/rhi.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md",
    ".codex/agents/rendering-auditor.toml",
    ".claude/agents/rendering-auditor.md"
)) {
    $postprocessDepthText = Get-AgentSurfaceText $postprocessDepthGuidance
    Assert-ContainsText $postprocessDepthText "Postprocess Depth Input Readback Foundation v0" $postprocessDepthGuidance
    Assert-ContainsText $postprocessDepthText "MK_VULKAN_TEST_POSTPROCESS_DEPTH_VERTEX_SPV" $postprocessDepthGuidance
    Assert-ContainsText $postprocessDepthText "MK_VULKAN_TEST_POSTPROCESS_DEPTH_FRAGMENT_SPV" $postprocessDepthGuidance
    Assert-ContainsText $postprocessDepthText "package-visible" $postprocessDepthGuidance
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "Postprocess Depth Input Readback Foundation v0" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "stable scene color/depth bindings 0/1 and 2/3" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "postprocess_depth_input_ready=1" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "--require-postprocess-depth-input" "runtime game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "generated color-postprocess scaffold" "runtime game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_POSTPROCESS_DEPTH_VERTEX_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_POSTPROCESS_DEPTH_FRAGMENT_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "postprocess-depth readback" "Vulkan game guidance"
foreach ($postprocessDepthReadyGuidance in @(
    "docs/testing.md",
    "docs/rhi.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md",
    ".codex/agents/rendering-auditor.toml",
    ".claude/agents/rendering-auditor.md",
    "games/sample_desktop_runtime_game/README.md"
)) {
    $postprocessDepthReadyText = Get-AgentSurfaceText $postprocessDepthReadyGuidance
    Assert-ContainsText $postprocessDepthReadyText "postprocess_depth_input_ready" $postprocessDepthReadyGuidance
}
foreach ($postprocessDepthPackageCommandGuidance in @(
    "engine/agent/manifest.json",
    "games/CMakeLists.txt",
    "games/sample_desktop_runtime_game/game.agent.json",
    "games/sample_desktop_runtime_game/README.md",
    "tools/validate-installed-desktop-runtime.ps1"
)) {
    $postprocessDepthPackageCommandText = Get-AgentSurfaceText $postprocessDepthPackageCommandGuidance
    Assert-ContainsText $postprocessDepthPackageCommandText "--require-postprocess-depth-input" $postprocessDepthPackageCommandGuidance
}
foreach ($gameDevelopmentDepthGuidance in @(
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".codex/agents/gameplay-builder.toml",
    ".claude/agents/gameplay-builder.md"
)) {
    $gameDevelopmentDepthText = Get-AgentSurfaceText $gameDevelopmentDepthGuidance
    Assert-ContainsText $gameDevelopmentDepthText "generated color-postprocess scaffold" $gameDevelopmentDepthGuidance
    Assert-ContainsText $gameDevelopmentDepthText "postprocess_depth_input_ready" $gameDevelopmentDepthGuidance
}
foreach ($shadowReceiverGuidance in @(
    "docs/testing.md",
    "docs/rhi.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md",
    ".codex/agents/rendering-auditor.toml",
    ".claude/agents/rendering-auditor.md"
)) {
    $shadowReceiverText = Get-AgentSurfaceText $shadowReceiverGuidance
    Assert-ContainsText $shadowReceiverText "MK_VULKAN_TEST_SHADOW_RECEIVER_VERTEX_SPV" $shadowReceiverGuidance
    Assert-ContainsText $shadowReceiverText "MK_VULKAN_TEST_SHADOW_RECEIVER_FRAGMENT_SPV" $shadowReceiverGuidance
    Assert-ContainsText $shadowReceiverText "shadow receiver readback" $shadowReceiverGuidance
    Assert-ContainsText $shadowReceiverText "package-visible" $shadowReceiverGuidance
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_SHADOW_RECEIVER_VERTEX_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_SHADOW_RECEIVER_FRAGMENT_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "shadow receiver readback" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "Directional Shadow Receiver Readback Proof v0" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "Package-Visible Directional Shadow Filtering v0" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "directional_shadow_status=ready" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "directional_shadow_ready=1" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "directional_shadow_filter_mode=fixed_pcf_3x3" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "directional_shadow_filter_taps=9" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "directional_shadow_filter_radius_texels=1" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "framegraph_passes=3" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "Stable Directional Light-Space Policy v0" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "DirectionalShadowLightSpacePlan" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "--require-directional-shadow" "runtime game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "--require-directional-shadow-filtering" "runtime game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "directional_shadow_ready=1" "runtime game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "directional_shadow_filter_mode=fixed_pcf_3x3" "runtime game guidance"
foreach ($directionalShadowPackageGuidance in @(
    "engine/agent/manifest.json",
    "games/CMakeLists.txt",
    "games/sample_desktop_runtime_game/game.agent.json",
    "games/sample_desktop_runtime_game/README.md",
    "tools/validate-installed-desktop-runtime.ps1"
)) {
    $directionalShadowPackageText = Get-AgentSurfaceText $directionalShadowPackageGuidance
    Assert-ContainsText $directionalShadowPackageText "--require-directional-shadow" $directionalShadowPackageGuidance
    Assert-ContainsText $directionalShadowPackageText "--require-directional-shadow-filtering" $directionalShadowPackageGuidance
}
foreach ($rendererQualityPackageGuidance in @(
    "engine/agent/manifest.json",
    "games/sample_desktop_runtime_game/game.agent.json",
    "games/sample_desktop_runtime_game/README.md",
    "tools/validate-installed-desktop-runtime.ps1"
)) {
    $rendererQualityPackageText = Get-AgentSurfaceText $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "--require-renderer-quality-gates" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "renderer_quality_status" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "renderer_quality_framegraph_execution_budget_ok" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "renderer_quality_expected_framegraph_barrier_steps" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "renderer_quality_framegraph_barrier_steps_ok" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "framegraph_barrier_steps_executed" $rendererQualityPackageGuidance
}
$rendererQualityCMakeText = Get-AgentSurfaceText "games/CMakeLists.txt"
Assert-ContainsText $rendererQualityCMakeText "--require-renderer-quality-gates" "games/CMakeLists.txt"
foreach ($directionalShadowStatusGuidance in @(
    "engine/agent/manifest.json",
    "games/sample_desktop_runtime_game/game.agent.json",
    "games/sample_desktop_runtime_game/README.md",
    "tools/validate-installed-desktop-runtime.ps1"
)) {
    $directionalShadowStatusText = Get-AgentSurfaceText $directionalShadowStatusGuidance
    Assert-ContainsText $directionalShadowStatusText "directional_shadow_status" $directionalShadowStatusGuidance
    Assert-ContainsText $directionalShadowStatusText "directional_shadow_ready" $directionalShadowStatusGuidance
    Assert-ContainsText $directionalShadowStatusText "directional_shadow_filter_mode" $directionalShadowStatusGuidance
    Assert-ContainsText $directionalShadowStatusText "directional_shadow_filter_taps" $directionalShadowStatusGuidance
    Assert-ContainsText $directionalShadowStatusText "directional_shadow_filter_radius_texels" $directionalShadowStatusGuidance
}
$installedDesktopRuntimeValidation = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedDesktopRuntimeValidation "did not emit the required" "installed desktop runtime validation"
foreach ($directionalShadowGameDevelopmentGuidance in @(
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $directionalShadowGameDevelopmentText = Get-AgentSurfaceText $directionalShadowGameDevelopmentGuidance
    Assert-ContainsText $directionalShadowGameDevelopmentText "--require-directional-shadow" $directionalShadowGameDevelopmentGuidance
    Assert-ContainsText $directionalShadowGameDevelopmentText "--require-directional-shadow-filtering" $directionalShadowGameDevelopmentGuidance
    Assert-ContainsText $directionalShadowGameDevelopmentText "directional_shadow_ready" $directionalShadowGameDevelopmentGuidance
    Assert-ContainsText $directionalShadowGameDevelopmentText "directional_shadow_filter_mode" $directionalShadowGameDevelopmentGuidance
}
foreach ($stableLightSpaceGuidance in @(
    "docs/testing.md",
    "docs/rhi.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "engine/agent/manifest.json",
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".codex/agents/rendering-auditor.toml",
    ".claude/agents/rendering-auditor.md",
    ".codex/agents/gameplay-builder.toml",
    ".claude/agents/gameplay-builder.md",
    "games/sample_desktop_runtime_game/README.md"
)) {
    $stableLightSpaceText = Get-AgentSurfaceText $stableLightSpaceGuidance
    Assert-ContainsText $stableLightSpaceText "Stable Directional Light-Space Policy v0" $stableLightSpaceGuidance
}
foreach ($stableLightSpaceApiGuidance in @(
    "docs/rhi.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "engine/agent/manifest.json",
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".codex/agents/rendering-auditor.toml",
    ".claude/agents/rendering-auditor.md",
    ".codex/agents/gameplay-builder.toml",
    ".claude/agents/gameplay-builder.md"
)) {
    $stableLightSpaceApiText = Get-AgentSurfaceText $stableLightSpaceApiGuidance
    Assert-ContainsText $stableLightSpaceApiText "DirectionalShadowLightSpacePlan" $stableLightSpaceApiGuidance
}

if (-not $manifest.PSObject.Properties.Name.Contains("aiDrivenGameWorkflow")) {
    Write-Error "engine/agent/manifest.json must expose aiDrivenGameWorkflow"
}
foreach ($field in @(
    "sampleGame",
    "inputRendererSample",
    "gameplayFoundationSample",
    "aiNavigationSample",
    "uiAudioAssetsSample",
    "desktopRuntimeShellSample",
    "desktopRuntimeGameSample",
    "generatedDesktopRuntimePackageSample",
    "generatedDesktopRuntimeCookedScenePackageSample",
    "generatedDesktopRuntimeMaterialShaderPackageSample",
    "sample2dDesktopRuntimePackageSample"
)) {
    if (-not $manifest.aiDrivenGameWorkflow.PSObject.Properties.Name.Contains($field)) {
        Write-Error "engine/agent/manifest.json aiDrivenGameWorkflow missing required sample field: $field"
    }
    Resolve-RequiredAgentPath $manifest.aiDrivenGameWorkflow.$field | Out-Null
}

Resolve-RequiredAgentPath "games/CMakeLists.txt" | Out-Null
Resolve-RequiredAgentPath "docs/README.md" | Out-Null
Resolve-RequiredAgentPath "docs/current-capabilities.md" | Out-Null
Resolve-RequiredAgentPath "docs/roadmap.md" | Out-Null
Resolve-RequiredAgentPath "docs/workflows.md" | Out-Null
Resolve-RequiredAgentPath "games/sample_headless/game.agent.json" | Out-Null
Resolve-RequiredAgentPath "docs/ai-game-development.md" | Out-Null
Resolve-RequiredAgentPath "docs/superpowers/plans/README.md" | Out-Null
Resolve-RequiredAgentPath "docs/specs/README.md" | Out-Null
Resolve-RequiredAgentPath "docs/specs/game-template.md" | Out-Null
Resolve-RequiredAgentPath "docs/specs/generated-game-validation-scenarios.md" | Out-Null
Resolve-RequiredAgentPath "docs/specs/game-prompt-pack.md" | Out-Null

$editorShell = Get-AgentSurfaceText "editor/src/main.cpp"
$editorSceneAuthoringNeedles = @(
    '#include "mirakana/editor/scene_authoring.hpp"',
    "mirakana::editor::SceneAuthoringDocument",
    "make_scene_authoring_create_node_action",
    "make_scene_authoring_delete_node_action",
    "make_scene_authoring_duplicate_subtree_action",
    "make_scene_authoring_transform_edit_action",
    "make_scene_authoring_component_edit_action",
    "build_prefab_from_selected_node",
    "save_prefab_authoring_document",
    "load_prefab_authoring_document",
    "refresh_selected_prefab_instance",
    "refresh_batch_prefab_instances_review",
    "Refresh Prefab Instance",
    "Review batch prefab refresh",
    "Clear batch selection",
    "Keep Local Children",
    "Keep Stale Source Nodes",
    "Keep Nested Prefab Instances",
    "Apply Reviewed Prefab Refresh",
    "make_scene_prefab_instance_refresh_action",
    "make_scene_prefab_instance_refresh_batch_action",
    "validate_scene_authoring_references",
    "make_scene_package_candidate_rows",
    "current_prefab_path",
    "current_cooked_scene_path",
    "current_package_index_path"
)
$missingEditorSceneAuthoringNeedles = @()
foreach ($needle in $editorSceneAuthoringNeedles) {
    if (-not $editorShell.Contains($needle)) {
        $missingEditorSceneAuthoringNeedles += $needle
    }
}
if ($missingEditorSceneAuthoringNeedles.Count -gt 0) {
    Write-Error "ai-integration-check: editor shell missing SceneAuthoringDocument wiring: $($missingEditorSceneAuthoringNeedles -join ', ')"
}
