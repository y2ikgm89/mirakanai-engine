#requires -Version 7.0
#requires -PSEdition Core

# Chapter 5 for check-json-contracts.ps1 static contracts.

foreach ($needle in @(
    "AccessibilityPublishPlan",
    "AccessibilityPublishResult",
    "plan_accessibility_publish",
    "publish_accessibility_payload",
    "IAccessibilityAdapter",
    "ImeCompositionPublishPlan",
    "ImeCompositionPublishResult",
    "plan_ime_composition_update",
    "publish_ime_composition",
    "IImeAdapter",
    "PlatformTextInputSessionPlan",
    "PlatformTextInputSessionResult",
    "PlatformTextInputEndPlan",
    "PlatformTextInputEndResult",
    "begin_platform_text_input",
    "end_platform_text_input",
    "IPlatformIntegrationAdapter",
    "TextShapingRequestPlan",
    "TextShapingResult",
    "plan_text_shaping_request",
    "shape_text_run",
    "ITextShapingAdapter",
    "invalid_text_shaping_result",
    "FontRasterizationRequestPlan",
    "FontRasterizationResult",
    "plan_font_rasterization_request",
    "rasterize_font_glyph",
    "IFontRasterizerAdapter",
    "invalid_font_allocation",
    "ImageDecodeRequestPlan",
    "ImageDecodeDispatchResult",
    "ImageDecodePixelFormat",
    "plan_image_decode_request",
    "decode_image_request",
    "IImageDecodingAdapter",
    "invalid_image_decode_result"
)) {
    if (-not $geUiHeaderText.Contains($needle)) {
        Write-Error "engine/ui/include/mirakana/ui/ui.hpp missing runtime UI publish API: $needle"
    }
}
foreach ($needle in @(
    "AccessibilityPublishPlan::ready",
    "AccessibilityPublishResult::succeeded",
    "adapter.publish_nodes",
    "invalid_accessibility_bounds",
    "ImeCompositionPublishPlan::ready",
    "ImeCompositionPublishResult::succeeded",
    "adapter.update_composition",
    "invalid_ime_target",
    "invalid_ime_cursor",
    "PlatformTextInputSessionPlan::ready",
    "PlatformTextInputSessionResult::succeeded",
    "PlatformTextInputEndPlan::ready",
    "PlatformTextInputEndResult::succeeded",
    "adapter.begin_text_input",
    "adapter.end_text_input",
    "invalid_platform_text_input_target",
    "invalid_platform_text_input_bounds",
    "TextShapingRequestPlan::ready",
    "TextShapingResult::succeeded",
    "adapter.shape_text",
    "invalid_text_shaping_text",
    "invalid_text_shaping_font_family",
    "invalid_text_shaping_max_width",
    "invalid_text_shaping_result",
    "FontRasterizationRequestPlan::ready",
    "FontRasterizationResult::succeeded",
    "adapter.rasterize_glyph",
    "invalid_font_family",
    "invalid_font_glyph",
    "invalid_font_pixel_size",
    "invalid_font_allocation",
    "ImageDecodeRequestPlan::ready",
    "ImageDecodeDispatchResult::succeeded",
    "adapter.decode_image",
    "invalid_image_decode_uri",
    "empty_image_decode_bytes",
    "invalid_image_decode_result"
)) {
    if (-not $geUiSourceText.Contains($needle)) {
        Write-Error "engine/ui/src/ui.cpp missing runtime UI publish implementation evidence: $needle"
    }
}
foreach ($needle in @(
    "PngImageDecodingAdapter",
    "ui::IImageDecodingAdapter",
    "decode_image"
)) {
    if (-not $sourceImageDecodeHeaderText.Contains($needle)) {
        Write-Error "engine/tools/include/mirakana/tools/source_image_decode.hpp missing Runtime UI PNG adapter API: $needle"
    }
}
foreach ($needle in @(
    "PngImageDecodingAdapter::decode_image",
    "decode_audited_png_rgba8",
    "ImageDecodePixelFormat::rgba8_unorm",
    "catch (...)"
)) {
    if (-not $sourceImageDecodeSourceText.Contains($needle)) {
        Write-Error "engine/tools/asset/source_image_decode.cpp missing Runtime UI PNG adapter implementation evidence: $needle"
    }
}
foreach ($needle in @(
    "runtime UI PNG image decoding adapter fails closed when importers are disabled",
    "runtime UI PNG image decoding adapter returns rgba8 image when importers are enabled",
    "invalid_image_decode_result"
)) {
    if (-not $toolsTestsText.Contains($needle)) {
        Write-Error "tests/unit/tools_tests.cpp missing Runtime UI PNG adapter test evidence: $needle"
    }
}
foreach ($needle in @(
    "PackedUiAtlasAuthoringDesc",
    "PackedUiAtlasPackageUpdateDesc",
    "author_packed_ui_atlas_from_decoded_images",
    "plan_packed_ui_atlas_package_update",
    "apply_packed_ui_atlas_package_update"
)) {
    if (-not $uiAtlasToolHeaderText.Contains($needle)) {
        Write-Error "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp missing Runtime UI decoded atlas API: $needle"
    }
}
foreach ($needle in @(
    "PackedUiGlyphAtlasAuthoringDesc",
    "PackedUiGlyphAtlasPackageUpdateDesc",
    "author_packed_ui_glyph_atlas_from_rasterized_glyphs",
    "plan_packed_ui_glyph_atlas_package_update",
    "apply_packed_ui_glyph_atlas_package_update",
    "rasterized-glyph-adapter",
    "deterministic-glyph-atlas-rgba8-max-side"
)) {
    if (-not $uiAtlasToolHeaderText.Contains($needle)) {
        Write-Error "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp missing Runtime UI glyph atlas API: $needle"
    }
}
foreach ($needle in @(
    "pack_sprite_atlas_rgba8_max_side",
    "GameEngine.CookedTexture.v1",
    "decoded image must be RGBA8",
    "ui atlas page output path collides"
)) {
    if (-not $uiAtlasToolSourceText.Contains($needle)) {
        Write-Error "engine/tools/asset/ui_atlas_tool.cpp missing Runtime UI decoded atlas implementation evidence: $needle"
    }
}
foreach ($needle in @(
    "rasterized glyph must be RGBA8",
    "ui atlas page output path collides"
)) {
    if (-not $uiAtlasToolSourceText.Contains($needle)) {
        Write-Error "engine/tools/asset/ui_atlas_tool.cpp missing Runtime UI glyph atlas implementation evidence: $needle"
    }
}
foreach ($needle in @(
    "packed runtime UI atlas authoring maps decoded images into texture page and metadata",
    "packed runtime UI atlas package update writes texture page metadata and package index",
    "packed runtime UI atlas rejects invalid decoded images and package path collisions",
    "packed runtime UI atlas apply leaves existing files unchanged when validation fails"
)) {
    if (-not $toolsTestsText.Contains($needle)) {
        Write-Error "tests/unit/tools_tests.cpp missing Runtime UI decoded atlas test evidence: $needle"
    }
}
foreach ($needle in @(
    "packed runtime UI glyph atlas authoring maps rasterized glyphs into texture page and metadata",
    "packed runtime UI glyph atlas package update writes texture page metadata and package index",
    "packed runtime UI glyph atlas rejects invalid glyph pixels and package path collisions",
    "packed runtime UI glyph atlas apply leaves existing files unchanged when validation fails"
)) {
    if (-not $toolsTestsText.Contains($needle)) {
        Write-Error "tests/unit/tools_tests.cpp missing Runtime UI glyph atlas test evidence: $needle"
    }
}
foreach ($needle in @(
    "CapturingAccessibilityAdapter",
    "ui accessibility publish plan dispatches validated nodes to adapter",
    "ui accessibility publish plan blocks invalid nodes before adapter",
    "CapturingImeAdapter",
    "ui ime composition publish plan dispatches valid composition to adapter",
    "ui ime composition publish plan blocks invalid composition before adapter",
    "CapturingFontRasterizerAdapter",
    "InvalidFontRasterizerAdapter",
    "ui font rasterization request plan dispatches valid request to adapter",
    "ui font rasterization request plan blocks invalid request before adapter",
    "ui font rasterization result reports invalid adapter allocation",
    "CapturingTextShapingAdapter",
    "ui text shaping request plan dispatches valid request to adapter",
    "ui text shaping request plan blocks invalid request before adapter",
    "ui text shaping result reports invalid adapter runs",
    "CapturingImageDecodingAdapter",
    "ui image decode request plan dispatches valid request to adapter",
    "ui image decode request plan blocks invalid request before adapter",
    "ui image decode result reports missing or invalid adapter output"
)) {
    if (-not $uiRendererTestsText.Contains($needle)) {
        Write-Error "tests/unit/ui_renderer_tests.cpp missing runtime UI publish test evidence: $needle"
    }
}

$geRhiModule = @($engine.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($geRhiModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_rhi module"
}
if (@($geRhiModule[0].publicHeaders) -notcontains "engine/rhi/include/mirakana/rhi/resource_lifetime.hpp") {
    Write-Error "engine manifest MK_rhi publicHeaders must include resource_lifetime.hpp"
}
if (@($geRhiModule[0].publicHeaders) -notcontains "engine/rhi/include/mirakana/rhi/upload_staging.hpp") {
    Write-Error "engine manifest MK_rhi publicHeaders must include upload_staging.hpp"
}
if (-not ([string]$geRhiModule[0].purpose).Contains("RhiResourceLifetimeRegistry") -or
    -not ([string]$geRhiModule[0].purpose).Contains("RhiUploadStagingPlan") -or
    -not ([string]$geRhiModule[0].purpose).Contains("FenceValue") -or
    -not ([string]$geRhiModule[0].purpose).Contains("foundation-only") -or
    -not ([string]$geRhiModule[0].purpose).Contains("GPU allocator")) {
    Write-Error "engine manifest MK_rhi purpose must describe foundation-only resource lifetime/upload staging and remaining GPU allocator limits"
}

foreach ($hostGate in $productionLoop.hostGates) {
    Assert-Properties $hostGate @("id", "status", "hosts", "validationRecipes", "notes") "engine manifest aiOperableProductionLoop hostGates"
    foreach ($validationRecipe in @($hostGate.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop host gate '$($hostGate.id)' references unknown validation recipe: $validationRecipe"
        }
    }
}
$vulkanGate = @($productionLoop.hostGates | Where-Object { $_.id -eq "vulkan-strict" })
if ($vulkanGate.Count -ne 1 -or $vulkanGate[0].status -ne "host-gated") {
    Write-Error "engine manifest aiOperableProductionLoop must keep vulkan-strict host-gated"
}
$metalGate = @($productionLoop.hostGates | Where-Object { $_.id -eq "metal-apple" })
if ($metalGate.Count -ne 1 -or $metalGate[0].status -ne "host-gated") {
    Write-Error "engine manifest aiOperableProductionLoop must keep metal-apple host-gated"
}

$recipeMapIds = @{}
foreach ($map in $productionLoop.validationRecipeMap) {
    Assert-Properties $map @("recipeId", "validationRecipes") "engine manifest aiOperableProductionLoop validationRecipeMap"
    if (-not $productionRecipeIds.ContainsKey($map.recipeId)) {
        Write-Error "engine manifest aiOperableProductionLoop validationRecipeMap references unknown recipe: $($map.recipeId)"
    }
    $recipeMapIds[$map.recipeId] = $true
    foreach ($validationRecipe in @($map.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop validationRecipeMap '$($map.recipeId)' references unknown validation recipe: $validationRecipe"
        }
    }
}
foreach ($recipeId in $expectedProductionRecipeIds) {
    if (-not $recipeMapIds.ContainsKey($recipeId)) {
        Write-Error "engine manifest aiOperableProductionLoop validationRecipeMap missing recipe id: $recipeId"
    }
}
Assert-Properties $engine.aiDrivenGameWorkflow @("steps", "templateSpec", "validationScenarios", "promptPack", "sampleGame") "engine manifest aiDrivenGameWorkflow"
foreach ($workflowDoc in @($engine.aiDrivenGameWorkflow.templateSpec, $engine.aiDrivenGameWorkflow.validationScenarios, $engine.aiDrivenGameWorkflow.promptPack)) {
    if (-not (Test-Path (Join-Path $root $workflowDoc))) {
        Write-Error "engine manifest aiDrivenGameWorkflow references missing document: $workflowDoc"
    }
}

$desktopRuntimeGameRegistrations = Get-DesktopRuntimeGameRegistrations

Get-ChildItem -Path (Join-Path $root "games") -Recurse -Filter "game.agent.json" | ForEach-Object {
    $relative = Get-RelativeRepoPath $_.FullName
    $game = Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json
    Assert-Properties $game @("schemaVersion", "name", "displayName", "language", "entryPoint", "target", "engineModules", "aiWorkflow", "gameplayContract", "backendReadiness", "importerRequirements", "packagingTargets", "validationRecipes") $relative
    Assert-Properties $game.backendReadiness @("platform", "graphics", "audio", "ui") "$relative backendReadiness"
    Assert-Properties $game.importerRequirements @("sourceFormats", "cookedOnlyRuntime", "externalImportersRequired") "$relative importerRequirements"
    if ($game.language -ne "C++23") {
        Write-Error "$relative must declare language C++23"
    }
    if ($game.name -notmatch "^[a-z][a-z0-9-]*$") {
        Write-Error "$relative has invalid game name: $($game.name)"
    }
    if ($game.target -notmatch "^[a-z][a-z0-9_]*$") {
        Write-Error "$relative has invalid target: $($game.target)"
    }
    if (-not (Test-Path (Join-Path $root $game.entryPoint))) {
        Write-Error "$relative references missing entryPoint: $($game.entryPoint)"
    }
    foreach ($module in $game.engineModules) {
        if (-not $moduleNames.ContainsKey($module)) {
            Write-Error "$relative references unknown engine module: $module"
        }
    }
    if (-not $game.importerRequirements.cookedOnlyRuntime) {
        Write-Error "$relative must keep cookedOnlyRuntime enabled"
    }
    foreach ($target in $game.packagingTargets) {
        if (-not $packagingTargetNames.ContainsKey($target)) {
            Write-Error "$relative references unknown packaging target: $target"
        }
    }
    $selectsDesktopGameRuntime = @($game.packagingTargets) -contains "desktop-game-runtime"
    $selectsDesktopRuntimeRelease = @($game.packagingTargets) -contains "desktop-runtime-release"
    if ($selectsDesktopRuntimeRelease -and -not $selectsDesktopGameRuntime) {
        Write-Error "$relative declares desktop-runtime-release but does not declare desktop-game-runtime"
    }
    if ($selectsDesktopGameRuntime) {
        if (-not $desktopRuntimeGameRegistrations.ContainsKey($game.target)) {
            Write-Error "$relative declares desktop-game-runtime but target '$($game.target)' is not registered with MK_add_desktop_runtime_game in games/CMakeLists.txt"
        }
        $registration = $desktopRuntimeGameRegistrations[$game.target]
        if (-not $registration.hasSmokeArgs) {
            Write-Error "$relative desktop runtime target '$($game.target)' must declare SMOKE_ARGS in MK_add_desktop_runtime_game"
        }
        $registeredManifest = Assert-DesktopRuntimeGameManifestPath $registration.gameManifest "desktop runtime target '$($game.target)'"
        if ($registeredManifest -ne $relative) {
            Write-Error "$relative desktop runtime target '$($game.target)' is registered with GAME_MANIFEST '$registeredManifest'"
        }
        Assert-DesktopRuntimePackageFileRegistration $game $relative $registration
    }
    if ($selectsDesktopRuntimeRelease) {
        Assert-DesktopRuntimePackageRecipe $game $relative
    }
    $declaredRuntimeFiles = @($game.runtimePackageFiles)
    $hasRuntimeScenePackage = @($declaredRuntimeFiles | Where-Object { ([string]$_).EndsWith(".geindex") }).Count -gt 0 -and
        @($declaredRuntimeFiles | Where-Object { ([string]$_).EndsWith(".scene") }).Count -gt 0
    Assert-RuntimeSceneValidationTargets $game $relative $hasRuntimeScenePackage
    $sourceFormats = @($game.importerRequirements.sourceFormats)
    $requiresMaterialShaderTargets = $sourceFormats -contains "first-party-material-source" -and $sourceFormats -contains "hlsl-source"
    Assert-MaterialShaderAuthoringTargets $game $relative $requiresMaterialShaderTargets
    $requiresAtlasTilemapTargets = $game.gameplayContract.productionRecipe -eq "2d-desktop-runtime-package"
    Assert-AtlasTilemapAuthoringTargets $game $relative $requiresAtlasTilemapTargets
    $requiresPrefabScene3dTargets = $game.gameplayContract.productionRecipe -eq "3d-playable-desktop-package"
    Assert-PrefabScenePackageAuthoringTargets $game $relative $requiresPrefabScene3dTargets
    Assert-RegisteredSourceAssetCookTargets $game $relative $requiresPrefabScene3dTargets
    Assert-PackageStreamingResidencyTargets $game $relative $hasRuntimeScenePackage
    foreach ($recipe in $game.validationRecipes) {
        Assert-Properties $recipe @("name", "command") "$relative validationRecipes"
        if ([string]::IsNullOrWhiteSpace($recipe.command)) {
            Write-Error "$relative validation recipe '$($recipe.name)' must declare a command"
        }
    }
}

$sample2dManifestPath = "games/sample_2d_playable_foundation/game.agent.json"
$sample2dManifestFullPath = Join-Path $root $sample2dManifestPath
if (-not (Test-Path $sample2dManifestFullPath)) {
    Write-Error "2d-playable-source-tree recipe must have a sample game manifest: $sample2dManifestPath"
} else {
    $sample2dManifest = Get-Content -LiteralPath $sample2dManifestFullPath -Raw | ConvertFrom-Json
    if ($sample2dManifest.target -ne "sample_2d_playable_foundation") {
        Write-Error "$sample2dManifestPath target must be sample_2d_playable_foundation"
    }
    if ($sample2dManifest.gameplayContract.productionRecipe -ne "2d-playable-source-tree") {
        Write-Error "$sample2dManifestPath gameplayContract.productionRecipe must be 2d-playable-source-tree"
    }
    foreach ($module in @("MK_runtime", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
        if (@($sample2dManifest.engineModules) -notcontains $module) {
            Write-Error "$sample2dManifestPath engineModules missing $module"
        }
    }
    if (@($sample2dManifest.packagingTargets) -notcontains "source-tree-default") {
        Write-Error "$sample2dManifestPath must use source-tree-default packaging target"
    }
    if (@($sample2dManifest.packagingTargets) -contains "desktop-game-runtime") {
        Write-Error "$sample2dManifestPath must not claim desktop-game-runtime readiness in the source-tree 2D slice"
    }
}

$sample2dDesktopManifestPath = "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample2dDesktopManifestFullPath = Join-Path $root $sample2dDesktopManifestPath
if (-not (Test-Path $sample2dDesktopManifestFullPath)) {
    Write-Error "2d-desktop-runtime-package recipe must have a sample game manifest: $sample2dDesktopManifestPath"
} else {
    $sample2dDesktopManifest = Get-Content -LiteralPath $sample2dDesktopManifestFullPath -Raw | ConvertFrom-Json
    if ($sample2dDesktopManifest.target -ne "sample_2d_desktop_runtime_package") {
        Write-Error "$sample2dDesktopManifestPath target must be sample_2d_desktop_runtime_package"
    }
    if ($sample2dDesktopManifest.gameplayContract.productionRecipe -ne "2d-desktop-runtime-package") {
        Write-Error "$sample2dDesktopManifestPath gameplayContract.productionRecipe must be 2d-desktop-runtime-package"
    }
    foreach ($module in @("MK_runtime", "MK_runtime_scene", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
        if (@($sample2dDesktopManifest.engineModules) -notcontains $module) {
            Write-Error "$sample2dDesktopManifestPath engineModules missing $module"
        }
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($sample2dDesktopManifest.packagingTargets) -notcontains $target) {
            Write-Error "$sample2dDesktopManifestPath packagingTargets missing $target"
        }
    }
    foreach ($packageFile in @(
        "runtime/sample_2d_desktop_runtime_package.config",
        "runtime/sample_2d_desktop_runtime_package.geindex",
        "runtime/assets/2d/player.texture.geasset",
        "runtime/assets/2d/player.material",
        "runtime/assets/2d/jump.audio.geasset",
        "runtime/assets/2d/level.tilemap",
        "runtime/assets/2d/playable.scene"
    )) {
        if (@($sample2dDesktopManifest.runtimePackageFiles) -notcontains $packageFile) {
            Write-Error "$sample2dDesktopManifestPath runtimePackageFiles missing $packageFile"
        }
    }
    if (-not $desktopRuntimeGameRegistrations.ContainsKey($sample2dDesktopManifest.target)) {
        Write-Error "$sample2dDesktopManifestPath target must be registered with MK_add_desktop_runtime_game"
    }
    $sample2dManifestText = Get-Content -LiteralPath $sample2dDesktopManifestFullPath -Raw
    foreach ($needle in @(
        "D3D12 package window smoke",
        "Vulkan package window smoke",
        "native 2D sprite package proof",
        "installed-d3d12-window-smoke",
        "installed-vulkan-window-smoke",
        "installed-native-2d-sprite-smoke",
        "--require-d3d12-shaders",
        "--require-d3d12-renderer",
        "--require-vulkan-shaders",
        "--require-vulkan-renderer",
        "--require-native-2d-sprites",
        "public native or RHI handle access remains unsupported",
        "broad production sprite batching readiness remains unsupported",
        "general production renderer quality remains unsupported"
    )) {
        if (-not $sample2dManifestText.Contains($needle)) {
            Write-Error "$sample2dDesktopManifestPath missing 2D native window presentation contract text: $needle"
        }
    }
    $sample2dMainPath = Join-Path $root "games/sample_2d_desktop_runtime_package/main.cpp"
    $sample2dMainText = Get-Content -LiteralPath $sample2dMainPath -Raw
    foreach ($needle in @(
        "mirakana/runtime_host/shader_bytecode.hpp",
        "mirakana/renderer/sprite_batch.hpp",
        "--require-d3d12-shaders",
        "--require-d3d12-renderer",
        "--require-vulkan-shaders",
        "--require-vulkan-renderer",
        "--require-native-2d-sprites",
        "sample_2d_desktop_runtime_package_sprite.vs.dxil",
        "sample_2d_desktop_runtime_package_sprite.ps.dxil",
        "sample_2d_desktop_runtime_package_sprite.vs.spv",
        "sample_2d_desktop_runtime_package_sprite.ps.spv",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.spv",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.spv",
        "native_2d_sprites_status",
        "native_2d_textured_sprites_submitted",
        "native_2d_texture_binds",
        "plan_scene_sprite_batches",
        "sprite_batch_plan_draws",
        "sprite_batch_plan_texture_binds",
        "sprite_batch_plan_diagnostics",
        "required_native_2d_sprites_unavailable",
        "required_d3d12_renderer_unavailable",
        "required_vulkan_renderer_unavailable"
    )) {
        if (-not $sample2dMainText.Contains($needle)) {
            Write-Error "games/sample_2d_desktop_runtime_package/main.cpp missing native presentation smoke field: $needle"
        }
    }
    $gamesCMakeText = Get-Content -LiteralPath (Join-Path $root "games/CMakeLists.txt") -Raw
    foreach ($needle in @(
        "sample_2d_desktop_runtime_package_sprite.vs.dxil",
        "sample_2d_desktop_runtime_package_sprite.ps.dxil",
        "sample_2d_desktop_runtime_package_sprite.vs.spv",
        "sample_2d_desktop_runtime_package_sprite.ps.spv",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.spv",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.spv",
        "--require-native-2d-sprites",
        "sample_2d_desktop_runtime_package_shaders",
        "sample_2d_desktop_runtime_package_vulkan_shaders",
        "REQUIRES_D3D12_SHADERS"
    )) {
        if (-not $gamesCMakeText.Contains($needle)) {
            Write-Error "games/CMakeLists.txt missing 2D native presentation package metadata: $needle"
        }
    }
    $installedDesktopRuntimeValidationText = Get-Content -LiteralPath (Join-Path $root "tools/validate-installed-desktop-runtime.ps1") -Raw
    foreach ($needle in @(
        "--require-native-2d-sprites",
        "native_2d_sprites_status",
        "native_2d_textured_sprites_submitted",
        "native_2d_texture_binds",
        "sprite_batch_plan_draws",
        "sprite_batch_plan_texture_binds",
        "sprite_batch_plan_diagnostics"
    )) {
        if (-not $installedDesktopRuntimeValidationText.Contains($needle)) {
            Write-Error "tools/validate-installed-desktop-runtime.ps1 missing 2D native sprite package validation field: $needle"
        }
    }
    $newGameText = (Get-Content -LiteralPath (Join-Path $root "tools/new-game.ps1") -Raw) +
        "`n" +
        (Get-Content -LiteralPath (Join-Path $root "tools/new-game-templates.ps1") -Raw)
    foreach ($needle in @(
        "shaders/runtime_2d_sprite.hlsl",
        "installed-native-2d-sprite-smoke",
        "--require-native-2d-sprites",
        "MK_configure_desktop_runtime_2d_sprite_shader_artifacts"
    )) {
        if (-not $newGameText.Contains($needle)) {
            Write-Error "tools/new-game.ps1 missing generated 2D native sprite scaffold contract: $needle"
        }
    }
    if (-not $gamesCMakeText.Contains("function(MK_configure_desktop_runtime_2d_sprite_shader_artifacts)")) {
        Write-Error "games/CMakeLists.txt missing generated 2D sprite shader artifact helper"
    }
}

$spriteBatchHeaderPath = Join-Path $root "engine/renderer/include/mirakana/renderer/sprite_batch.hpp"
$spriteBatchSourcePath = Join-Path $root "engine/renderer/src/sprite_batch.cpp"
if (-not (Test-Path -LiteralPath $spriteBatchHeaderPath)) {
    Write-Error "Missing 2D sprite batch planning header"
}
if (-not (Test-Path -LiteralPath $spriteBatchSourcePath)) {
    Write-Error "Missing 2D sprite batch planning source"
}
$spriteBatchHeaderText = Get-Content -LiteralPath $spriteBatchHeaderPath -Raw
$spriteBatchSourceText = Get-Content -LiteralPath $spriteBatchSourcePath -Raw
$sceneRendererHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp") -Raw
$sceneRendererSourceText = Get-Content -LiteralPath (Join-Path $root "engine/scene_renderer/src/scene_renderer.cpp") -Raw
$sceneRendererTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/scene_renderer_tests.cpp") -Raw
$rendererTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/renderer_rhi_tests.cpp") -Raw
$frameGraphHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/include/mirakana/renderer/frame_graph.hpp") -Raw
$frameGraphSourceText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/src/frame_graph.cpp") -Raw
$frameGraphRhiHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp") -Raw
$frameGraphRhiSourceText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/src/frame_graph_rhi.cpp") -Raw
$rhiUploadStagingHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/rhi/include/mirakana/rhi/upload_staging.hpp") -Raw
$rhiUploadStagingSourceText = Get-Content -LiteralPath (Join-Path $root "engine/rhi/src/upload_staging.cpp") -Raw
$rhiUploadStagingTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/rhi_upload_staging_tests.cpp") -Raw
$runtimeRhiUploadHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp") -Raw
$runtimeRhiUploadSourceText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_rhi/src/runtime_upload.cpp") -Raw
$runtimeSceneRhiHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp") -Raw
$runtimeSceneRhiSourceText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp") -Raw
$runtimeRhiTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/runtime_rhi_tests.cpp") -Raw
$runtimeSceneRhiTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/runtime_scene_rhi_tests.cpp") -Raw
$runtimeUploadFencePlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-runtime-rhi-upload-submission-fence-rows-v1.md") -Raw
$frameGraphRhiTextureSchedulePlanText =
    Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-frame-graph-rhi-texture-schedule-execution-v1.md") -Raw
$rhiUploadStaleGenerationPlanText =
    Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-rhi-upload-stale-generation-diagnostics-v1.md") -Raw
$rendererCmakeText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/CMakeLists.txt") -Raw
$engineManifestText = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.json") -Raw
foreach ($needle in @("FrameGraphPassExecutionBinding", "FrameGraphExecutionCallbacks", "FrameGraphExecutionResult", "execute_frame_graph_v1_schedule")) {
    if (-not $frameGraphHeaderText.Contains($needle)) {
        Write-Error "Frame Graph callback execution header missing contract text: $needle"
    }
}
foreach ($needle in @(
    "frame graph pass callback is missing",
    "frame graph barrier callback is missing",
    "frame graph pass callback threw an exception",
    "frame graph barrier callback threw an exception",
    "frame graph pass callback failed",
    "frame graph barrier callback failed"
)) {
    if (-not $frameGraphSourceText.Contains($needle)) {
        Write-Error "Frame Graph callback execution source missing diagnostic text: $needle"
    }
}
foreach ($needle in @(
    "frame graph v1 dispatches barrier and pass callbacks in schedule order",
    "frame graph v1 callback execution diagnoses missing callbacks before later passes",
    "frame graph v1 callback execution converts thrown callbacks to diagnostics",
    "frame graph v1 callback execution copies pass bindings before dispatch",
    "frame graph v1 callback execution reports returned callback failures",
    "frame graph v1 callback execution converts thrown barrier callbacks to diagnostics"
)) {
    if (-not $rendererTestsText.Contains($needle)) {
        Write-Error "MK_renderer_tests missing Frame Graph callback execution coverage: $needle"
    }
}
foreach ($needle in @("FrameGraphRhiTextureExecutionDesc", "FrameGraphRhiTextureExecutionResult", "execute_frame_graph_rhi_texture_schedule")) {
    if (-not $frameGraphRhiHeaderText.Contains($needle)) {
        Write-Error "Frame Graph RHI texture schedule execution header missing contract text: $needle"
    }
}
foreach ($needle in @(
    "frame graph rhi texture schedule execution requires a command list",
    "frame graph rhi texture schedule execution cannot use a closed command list",
    "frame graph pass callback is empty",
    "frame graph pass callback is declared more than once",
    "frame graph texture barrier recording failed"
)) {
    if (-not $frameGraphRhiSourceText.Contains($needle)) {
        Write-Error "Frame Graph RHI texture schedule execution source missing diagnostic text: $needle"
    }
}
foreach ($needle in @(
    "frame graph rhi texture schedule execution interleaves barriers and pass callbacks",
    "frame graph rhi texture schedule execution validates barriers before pass callbacks",
    "frame graph rhi texture schedule execution validates pass callbacks before barriers"
)) {
    if (-not $rendererTestsText.Contains($needle)) {
        Write-Error "MK_renderer_tests missing Frame Graph RHI texture schedule execution coverage: $needle"
    }
}
foreach ($needle in @("**Status:** Completed.", "FrameGraphRhiTextureExecutionDesc", "FrameGraphRhiTextureExecutionResult", "execute_frame_graph_rhi_texture_schedule")) {
    if (-not $frameGraphRhiTextureSchedulePlanText.Contains($needle)) {
        Write-Error "Frame Graph RHI texture schedule execution plan missing contract text: $needle"
    }
}
foreach ($needle in @(
    "submitted_fence",
    "RuntimeTextureUploadResult",
    "RuntimeMeshUploadResult",
    "RuntimeSkinnedMeshUploadResult",
    "RuntimeMorphMeshUploadResult",
    "RuntimeMaterialGpuBinding"
)) {
    if (-not $runtimeRhiUploadHeaderText.Contains($needle)) {
        Write-Error "Runtime RHI upload submission fence header missing contract text: $needle"
    }
}
foreach ($needle in @("submitted_upload_fences", "submitted_upload_fence_count", "last_submitted_upload_fence")) {
    if (-not $runtimeSceneRhiHeaderText.Contains($needle)) {
        Write-Error "Runtime scene RHI upload execution report header missing contract text: $needle"
    }
}
foreach ($needle in @(
    "result.submitted_fence = fence",
    "return RuntimeTextureUploadResult",
    "return RuntimeMeshUploadResult",
    "return RuntimeSkinnedMeshUploadResult",
    "return RuntimeMorphMeshUploadResult"
)) {
    if (-not $runtimeRhiUploadSourceText.Contains($needle)) {
        Write-Error "Runtime RHI upload submission fence source missing contract text: $needle"
    }
}
foreach ($needle in @(
    "record_submitted_upload_fence",
    "result.submitted_upload_fences.push_back",
    "upload.submitted_fence",
    "base_upload.submitted_fence",
    "binding.submitted_fence",
    "bindings.submitted_upload_fences"
)) {
    if (-not $runtimeSceneRhiSourceText.Contains($needle)) {
        Write-Error "Runtime scene RHI upload fence aggregation source missing contract text: $needle"
    }
}
foreach ($needle in @(
    "runtime rhi upload reports submitted fence without forcing wait",
    "result.submitted_fence.value != 0",
    "binding.submitted_fence.value != 0",
    "upload.submitted_fence.value != 0"
)) {
    if (-not $runtimeRhiTestsText.Contains($needle)) {
        Write-Error "MK_runtime_rhi_tests missing upload submission fence coverage: $needle"
    }
}
foreach ($needle in @(
    "runtime scene rhi upload execution preserves submitted fences in submit order across queues",
    "submitted_upload_fences[0].queue == mirakana::rhi::QueueKind::compute",
    "submitted_upload_fences[1].value == compute_resource.base_position_upload.submitted_fence.value",
    "submitted_upload_fence_count == 3",
    "submitted_upload_fence_count == 4",
    "last_submitted_upload_fence.value != 0"
)) {
    if (-not $runtimeSceneRhiTestsText.Contains($needle)) {
        Write-Error "MK_runtime_scene_rhi_tests missing upload submission fence coverage: $needle"
    }
}
foreach ($needle in @(
    "**Status:** Completed",
    "Runtime RHI Upload Submission Fence Rows v1",
    "submitted_upload_fences",
    "submitted_upload_fence_count",
    "native async upload execution"
)) {
    if (-not $runtimeUploadFencePlanText.Contains($needle)) {
        Write-Error "Runtime RHI Upload Submission Fence Rows plan missing text: $needle"
    }
}
foreach ($needle in @("RhiUploadDiagnosticCode", "stale_generation", "RhiUploadRing")) {
    if (-not $rhiUploadStagingHeaderText.Contains($needle)) {
        Write-Error "RHI upload staging header missing stale-generation contract text: $needle"
    }
}
foreach ($needle in @(
    "inactive_allocation_code",
    "RHI upload staging allocation generation is stale.",
    "span.allocation_generation",
    "RhiUploadDiagnosticCode::stale_generation"
)) {
    if (-not $rhiUploadStagingSourceText.Contains($needle)) {
        Write-Error "RHI upload staging source missing stale-generation implementation text: $needle"
    }
}
foreach ($needle in @(
    "rhi upload staging diagnoses stale allocation generations",
    "rhi upload ring ownership requires matching allocation generation",
    "RhiUploadDiagnosticCode::stale_generation",
    "!ring.owns_allocation(stale)"
)) {
    if (-not $rhiUploadStagingTestsText.Contains($needle)) {
        Write-Error "MK_rhi_upload_staging_tests missing stale-generation coverage: $needle"
    }
}
foreach ($needle in @("**Status:** Completed.", "RHI Upload Stale Generation Diagnostics v1", "stale_generation", "native async upload execution")) {
    if (-not $rhiUploadStaleGenerationPlanText.Contains($needle)) {
        Write-Error "RHI Upload Stale Generation Diagnostics plan missing text: $needle"
    }
}
foreach ($needle in @("SpriteBatchPlan", "SpriteBatchRange", "SpriteBatchDiagnosticCode", "plan_sprite_batches")) {
    if (-not $spriteBatchHeaderText.Contains($needle)) {
        Write-Error "2D sprite batch planning header missing contract text: $needle"
    }
}
foreach ($needle in @("append_or_extend_batch", "missing_texture_atlas", "invalid_uv_rect", "texture_bind_count")) {
    if (-not $spriteBatchSourceText.Contains($needle)) {
        Write-Error "2D sprite batch planning source missing contract text: $needle"
    }
}
foreach ($needle in @(
    "sprite batch planner preserves order",
    "sprite batch planner diagnoses invalid texture metadata",
    "SpriteBatchDiagnosticCode::missing_texture_atlas",
    "SpriteBatchDiagnosticCode::invalid_uv_rect"
)) {
    if (-not $rendererTestsText.Contains($needle)) {
        Write-Error "MK_renderer_tests missing 2D sprite batch planning coverage: $needle"
    }
}
foreach ($needle in @("scene sprite batch telemetry", "plan_scene_sprite_batches")) {
    if (-not $sceneRendererTestsText.Contains($needle)) {
        Write-Error "MK_scene_renderer_tests missing 2D sprite batch telemetry coverage: $needle"
    }
}
if (-not $rendererCmakeText.Contains("src/sprite_batch.cpp")) {
    Write-Error "MK_renderer CMake missing sprite_batch.cpp"
}
foreach ($needle in @("mirakana/renderer/sprite_batch.hpp", "plan_scene_sprite_batches")) {
    if (-not $sceneRendererHeaderText.Contains($needle)) {
        Write-Error "MK_scene_renderer header missing 2D sprite batch telemetry contract: $needle"
    }
    if (-not $sceneRendererSourceText.Contains($needle)) {
        Write-Error "MK_scene_renderer source missing 2D sprite batch telemetry contract: $needle"
    }
}
foreach ($needle in @(
    "2d-sprite-batch-planning-contract",
    "2d-sprite-batch-package-telemetry",
    "plan_sprite_batches",
    "plan_scene_sprite_batches",
    "production sprite batching readiness",
    "native_sprite_batches_executed"
)) {
    if (-not $engineManifestText.Contains($needle)) {
        Write-Error "engine manifest missing sprite batch planning contract text: $needle"
    }
}
if ($engineManifestText.Contains("production sprite batching ready")) {
    Write-Error "engine manifest must not claim production sprite batching ready"
}

$sample3dManifestPath = "games/sample_desktop_runtime_game/game.agent.json"
$sample3dManifestFullPath = Join-Path $root $sample3dManifestPath
if (-not (Test-Path $sample3dManifestFullPath)) {
    Write-Error "3d-playable-desktop-package recipe must have a sample game manifest: $sample3dManifestPath"
} else {
    $sample3dManifest = Get-Content -LiteralPath $sample3dManifestFullPath -Raw | ConvertFrom-Json
    if ($sample3dManifest.target -ne "sample_desktop_runtime_game") {
        Write-Error "$sample3dManifestPath target must be sample_desktop_runtime_game"
    }
    if ($sample3dManifest.gameplayContract.productionRecipe -ne "3d-playable-desktop-package") {
        Write-Error "$sample3dManifestPath gameplayContract.productionRecipe must be 3d-playable-desktop-package"
    }
    foreach ($module in @("MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($sample3dManifest.engineModules) -notcontains $module) {
            Write-Error "$sample3dManifestPath engineModules missing $module"
        }
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($sample3dManifest.packagingTargets) -notcontains $target) {
            Write-Error "$sample3dManifestPath packagingTargets missing $target"
        }
    }
    foreach ($packageFile in @(
        "runtime/sample_desktop_runtime_game.config",
        "runtime/sample_desktop_runtime_game.geindex",
        "runtime/assets/desktop_runtime/base_color.texture.geasset",
        "runtime/assets/desktop_runtime/hud.uiatlas",
        "runtime/assets/desktop_runtime/skinned_triangle.skinned_mesh",
        "runtime/assets/desktop_runtime/unlit.material",
        "runtime/assets/desktop_runtime/packaged_scene.scene"
    )) {
        if (@($sample3dManifest.runtimePackageFiles) -notcontains $packageFile) {
            Write-Error "$sample3dManifestPath runtimePackageFiles missing $packageFile"
        }
    }
    if (-not $desktopRuntimeGameRegistrations.ContainsKey($sample3dManifest.target)) {
        Write-Error "$sample3dManifestPath target must be registered with MK_add_desktop_runtime_game"
    }
    $sample3dManifestText = Get-Content -LiteralPath $sample3dManifestFullPath -Raw
    foreach ($needle in @(
        "material instance intent",
        "camera/controller movement",
        "HUD diagnostics",
        "runtime source asset parsing remains unsupported",
        "material graph remains unsupported",
        "skeletal animation production path remains unsupported",
        "GPU skinning is host-proven on the D3D12 package smoke lane",
        "sample_desktop_runtime_game --require-gpu-skinning",
        "package streaming remains unsupported",
        "native GPU runtime UI overlay",
        "textured UI sprite atlas",
        "production text/font/image/atlas/accessibility remains unsupported",
        "public native or RHI handle access remains unsupported",
        "general production renderer quality remains unsupported"
    )) {
        if (-not $sample3dManifestText.Contains($needle)) {
            Write-Error "$sample3dManifestPath missing 3D boundary text: $needle"
        }
    }
    if ($sample3dManifestText.Contains("native GPU HUD or sprite overlay output remains unsupported")) {
        Write-Error "$sample3dManifestPath keeps a stale native GPU HUD or sprite overlay unsupported claim"
    }
    $sample3dUiAtlasPath = Join-Path $root "games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/hud.uiatlas"
    $sample3dUiAtlasText = Get-Content -LiteralPath $sample3dUiAtlasPath -Raw
    foreach ($needle in @("format=GameEngine.UiAtlas.v1", "source.decoding=unsupported", "atlas.packing=unsupported", "page.count=1", "image.count=1")) {
        if (-not $sample3dUiAtlasText.Contains($needle)) {
            Write-Error "games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/hud.uiatlas missing cooked UI atlas metadata text: $needle"
        }
    }
    $sample3dIndexPath = Join-Path $root "games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.geindex"
    $sample3dIndexText = Get-Content -LiteralPath $sample3dIndexPath -Raw
    foreach ($needle in @("kind=ui_atlas", "kind=ui_atlas_texture")) {
        if (-not $sample3dIndexText.Contains($needle)) {
            Write-Error "games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.geindex missing UI atlas package row: $needle"
        }
    }
    $sample3dMainPath = Join-Path $root "games/sample_desktop_runtime_game/main.cpp"
    $sample3dMainText = Get-Content -LiteralPath $sample3dMainPath -Raw
    foreach ($needle in @(
        "mirakana/ui/ui.hpp",
        "mirakana/ui_renderer/ui_renderer.hpp",
        "--require-native-ui-overlay",
        "--require-native-ui-textured-sprite-atlas",
        "plan_scene_mesh_draws",
        "camera_primary=",
        "camera_controller_ticks=",
        "scene_mesh_plan_draws=",
        "scene_mesh_plan_unique_materials=",
        "scene_mesh_plan_diagnostics=",
        "hud_boxes=",
        "ui_overlay_requested=",
        "ui_overlay_status=",
        "ui_overlay_ready=",
        "ui_overlay_sprites_submitted=",
        "ui_overlay_draws=",
        "ui_texture_overlay_requested=",
        "ui_texture_overlay_status=",
        "ui_texture_overlay_atlas_ready=",
        "ui_texture_overlay_sprites_submitted=",
        "ui_texture_overlay_texture_binds=",
        "ui_texture_overlay_draws=",
        "ui_atlas_metadata_status=",
        "ui_atlas_metadata_pages=",
        "ui_atlas_metadata_bindings=",
        "--require-renderer-quality-gates",
        "renderer_quality_status=",
        "renderer_quality_ready=",
        "renderer_quality_diagnostics=",
        "renderer_quality_expected_framegraph_passes=",
        "renderer_quality_framegraph_passes_ok=",
        "renderer_quality_framegraph_execution_budget_ok=",
        "renderer_quality_scene_gpu_ready=",
        "renderer_quality_postprocess_ready=",
        "renderer_quality_postprocess_depth_input_ready=",
        "renderer_quality_directional_shadow_ready=",
        "renderer_quality_directional_shadow_filter_ready=",
        "primary_camera_seen_",
        "hud_boxes_submitted_"
    )) {
        if (-not $sample3dMainText.Contains($needle)) {
            Write-Error "games/sample_desktop_runtime_game/main.cpp missing 3D smoke field or HUD contract: $needle"
        }
    }
    $sceneRendererHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp") -Raw
    foreach ($needle in @(
        "SceneMeshDrawPlan",
        "SceneMeshDrawPlanDiagnosticCode",
        "plan_scene_mesh_draws"
    )) {
        if (-not $sceneRendererHeaderText.Contains($needle)) {
            Write-Error "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp missing 3D scene mesh package telemetry API: $needle"
        }
    }
    $installedDesktopRuntimeValidationText = Get-Content -LiteralPath (Join-Path $root "tools/validate-installed-desktop-runtime.ps1") -Raw
    foreach ($field in @(
        "scene_mesh_plan_meshes",
        "scene_mesh_plan_draws",
        "scene_mesh_plan_unique_meshes",
        "scene_mesh_plan_unique_materials",
        "scene_mesh_plan_diagnostics",
        "--require-renderer-quality-gates",
        "renderer_quality_status",
        "renderer_quality_ready",
        "renderer_quality_diagnostics",
        "renderer_quality_expected_framegraph_passes",
        "renderer_quality_framegraph_passes_ok",
        "renderer_quality_framegraph_execution_budget_ok",
        "renderer_quality_scene_gpu_ready",
        "renderer_quality_postprocess_ready",
        "renderer_quality_postprocess_depth_input_ready",
        "renderer_quality_directional_shadow_ready",
        "renderer_quality_directional_shadow_filter_ready",
        "--require-native-ui-overlay",
        "hud_boxes",
        "ui_overlay_requested",
        "ui_overlay_status",
        "ui_overlay_ready",
        "ui_overlay_sprites_submitted",
        "ui_overlay_draws"
    )) {
        if (-not $installedDesktopRuntimeValidationText.Contains($field)) {
            Write-Error "tools/validate-installed-desktop-runtime.ps1 missing 3D scene mesh package telemetry validation field: $field"
        }
    }
}

$generated3dPackageManifestPath = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$generated3dPackageManifestFullPath = Join-Path $root $generated3dPackageManifestPath
if (-not (Test-Path -LiteralPath $generated3dPackageManifestFullPath)) {
    Write-Error "Generated 3D package gameplay systems smoke must have a sample game manifest: $generated3dPackageManifestPath"
} else {
    $generated3dPackageManifest = Get-Content -LiteralPath $generated3dPackageManifestFullPath -Raw | ConvertFrom-Json
    $generated3dPackageManifestText = Get-Content -LiteralPath $generated3dPackageManifestFullPath -Raw
    $generated3dPackageReadmeText = Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/README.md") -Raw
    $generated3dPackageMainText = Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/main.cpp") -Raw
    $gamesCMakeText = Get-Content -LiteralPath (Join-Path $root "games/CMakeLists.txt") -Raw

    if ($generated3dPackageManifest.target -ne "sample_generated_desktop_runtime_3d_package") {
        Write-Error "$generated3dPackageManifestPath target must be sample_generated_desktop_runtime_3d_package"
    }
    foreach ($recipe in @($generated3dPackageManifest.validationRecipes)) {
        $isGenerated3dPackageRecipe = [string]$recipe.name -match "installed-d3d12|vulkan"
        $isDirectionalShadowRecipe = [string]$recipe.command -match "--require-directional-shadow"
        $isShadowMorphRecipe = [string]$recipe.command -match "--require-shadow-morph-composition"
        $isSceneCollisionPackageRecipe = [string]$recipe.command -match "--require-scene-collision-package"
        if ($isGenerated3dPackageRecipe -and -not $isDirectionalShadowRecipe -and -not $isShadowMorphRecipe -and -not $isSceneCollisionPackageRecipe -and [string]$recipe.command -notmatch "--require-gameplay-systems") {
            Write-Error "$generated3dPackageManifestPath package validation recipe missing --require-gameplay-systems: $($recipe.name)"
        }
        if ($recipe.name -match "scene-collision-package" -and [string]$recipe.command -notmatch "--require-scene-collision-package") {
            Write-Error "$generated3dPackageManifestPath scene collision package recipe missing --require-scene-collision-package: $($recipe.name)"
        }
        if ($recipe.name -match "shadow-morph" -and [string]$recipe.command -notmatch "--require-shadow-morph-composition") {
            Write-Error "$generated3dPackageManifestPath shadow-morph recipe missing --require-shadow-morph-composition: $($recipe.name)"
        }
        if ($recipe.name -match "native-ui-overlay" -and [string]$recipe.command -notmatch "--require-native-ui-overlay") {
            Write-Error "$generated3dPackageManifestPath native UI overlay recipe missing --require-native-ui-overlay: $($recipe.name)"
        }
        if ($recipe.name -match "visible-production-proof" -and [string]$recipe.command -notmatch "--require-visible-3d-production-proof") {
            Write-Error "$generated3dPackageManifestPath visible production proof recipe missing --require-visible-3d-production-proof: $($recipe.name)"
        }
    }
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-shadow-morph-composition-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-native-ui-overlay-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-visible-production-proof-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-native-ui-textured-sprite-atlas-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-native-ui-text-glyph-atlas-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "runtime/assets/3d/hud.uiatlas" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "runtime/assets/3d/hud_text.uiatlas" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "shadow_receiver_shifted.ps.dxil" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "shadow_receiver_shifted.ps.spv" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "runtime_ui_overlay.hlsl" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "ui_overlay.vs.dxil" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "ui_overlay.ps.dxil" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "ui_overlay.vs.spv" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "ui_overlay.ps.spv" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-shadow-morph-composition" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-native-ui-overlay" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-visible-3d-production-proof" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-native-ui-textured-sprite-atlas" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-native-ui-text-glyph-atlas" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-gameplay-systems" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageReadmeText "--require-shadow-morph-composition" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-native-ui-overlay" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-visible-3d-production-proof" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-native-ui-textured-sprite-atlas" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-native-ui-text-glyph-atlas" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-gameplay-systems" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $gamesCMakeText "shadow_receiver_shifted.ps.dxil" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "GE_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "--require-gameplay-systems" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "UI_OVERLAY_SHADER_SOURCE" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "--require-native-ui-overlay" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "--require-visible-3d-production-proof" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "--require-native-ui-textured-sprite-atlas" "games/CMakeLists.txt"
    $generated3dPackageUiAtlasText = Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud.uiatlas") -Raw
    Assert-ContainsText $generated3dPackageUiAtlasText "format=GameEngine.UiAtlas.v1" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud.uiatlas"
    Assert-ContainsText $generated3dPackageUiAtlasText "page.0.asset_uri=runtime/assets/3d/base_color.texture.geasset" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud.uiatlas"
    $generated3dPackageUiTextGlyphAtlasText = Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas") -Raw
    Assert-ContainsText $generated3dPackageUiTextGlyphAtlasText "format=GameEngine.UiAtlas.v1" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas"
    Assert-ContainsText $generated3dPackageUiTextGlyphAtlasText "glyph.0.font_family=engine-default" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas"
    Assert-ContainsText $generated3dPackageUiTextGlyphAtlasText "glyph.0.glyph=65" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas"
    Assert-ContainsText (Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex") -Raw) "kind=ui_atlas_texture" "games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex"
    $newGameScaffoldText = (Get-Content -LiteralPath (Join-Path $root "tools/new-game.ps1") -Raw) +
        "`n" +
        (Get-Content -LiteralPath (Join-Path $root "tools/new-game-templates.ps1") -Raw)
    Assert-ContainsText $newGameScaffoldText "runtime/assets/3d/hud.uiatlas" "tools/new-game scaffolding"
    Assert-ContainsText $newGameScaffoldText "runtime/assets/3d/hud_text.uiatlas" "tools/new-game scaffolding"
    foreach ($needle in @(
        "--require-shadow-morph-composition",
        "require_shadow_morph_composition",
        "require_graphics_morph_scene",
        "load_packaged_d3d12_shifted_shadow_receiver_scene_shaders",
        "load_packaged_vulkan_shifted_shadow_receiver_scene_shaders",
        "skinned_scene_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode",
        "--require-gameplay-systems",
        "require_gameplay_systems",
        "gameplay_systems_status=",
        "gameplay_systems_ready=",
        "gameplay_systems_navigation_plan_status=",
        "gameplay_systems_blackboard_status=",
        "gameplay_systems_behavior_status=",
        "gameplay_systems_audio_status=",
        "gameplay_systems_audio_first_sample=",
        "--require-native-ui-overlay",
        "--require-visible-3d-production-proof",
        "--require-native-ui-textured-sprite-atlas",
        "--require-native-ui-text-glyph-atlas",
        "require_native_ui_overlay",
        "require_visible_3d_production_proof",
        "require_native_ui_textured_sprite_atlas",
        "require_native_ui_text_glyph_atlas",
        "build_ui_renderer_image_palette_from_runtime_ui_atlas",
        "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas",
        "hud_boxes=",
        "hud_images=",
        "hud_text_glyphs=",
        "text_glyphs_resolved=",
        "ui_atlas_metadata_status=",
        "ui_atlas_metadata_glyphs=",
        "ui_texture_overlay_status=",
        "ui_overlay_requested=",
        "ui_overlay_status=",
        "ui_overlay_ready=",
        "ui_overlay_sprites_submitted=",
        "ui_overlay_draws=",
        "visible_3d_status=",
        "visible_3d_presented_frames=",
        "visible_3d_d3d12_selected=",
        "load_packaged_d3d12_native_ui_overlay_shaders",
        "load_packaged_vulkan_native_ui_overlay_shaders",
        "native_ui_overlay_vertex_shader",
        "enable_native_ui_overlay",
        "enable_native_ui_overlay_textures"
    )) {
        Assert-ContainsText $generated3dPackageMainText $needle "games/sample_generated_desktop_runtime_3d_package/main.cpp"
    }
}

foreach ($registration in $desktopRuntimeGameRegistrations.Values) {
    $manifestRelativePath = Assert-DesktopRuntimeGameManifestPath $registration.gameManifest "desktop runtime target '$($registration.target)'"
    $manifestPath = Join-Path $root $manifestRelativePath
    if (-not (Test-Path $manifestPath)) {
        Write-Error "desktop runtime target '$($registration.target)' references missing GAME_MANIFEST: $manifestRelativePath"
    }
    $game = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($game.target -ne $registration.target) {
        Write-Error "$manifestRelativePath target '$($game.target)' does not match desktop runtime registration '$($registration.target)'"
    }
    if (-not (@($game.packagingTargets) -contains "desktop-game-runtime")) {
        Write-Error "$manifestRelativePath is registered with MK_add_desktop_runtime_game but does not declare desktop-game-runtime"
    }
    if (-not (@($game.packagingTargets) -contains "desktop-runtime-release")) {
        Write-Error "$manifestRelativePath is registered with MK_add_desktop_runtime_game but does not declare desktop-runtime-release"
    }
    Assert-DesktopRuntimePackageFileRegistration $game $manifestRelativePath $registration
    Assert-DesktopRuntimePackageRecipe $game $manifestRelativePath
}

Write-Information "json-contract-check: ok" -InformationAction Continue
