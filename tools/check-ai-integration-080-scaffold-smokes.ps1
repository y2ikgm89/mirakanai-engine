#requires -Version 7.0
#requires -PSEdition Core

# Chapter 8 for check-ai-integration.ps1 static contracts.

try {
    & (Join-Path $PSScriptRoot "new-game.ps1") -Name "dry_run_game" -RepositoryRoot $headlessScaffoldRoot | Out-Null
    $headlessGameRoot = Join-Path $headlessScaffoldRoot "games/dry_run_game"
    foreach ($relativePath in @(
        "main.cpp",
        "README.md",
        "game.agent.json"
    )) {
        $path = Join-Path $headlessGameRoot $relativePath
        if (-not (Test-Path -LiteralPath $path)) {
            Write-Error "Headless scaffold did not create $relativePath"
        }
    }

    $headlessManifest = Get-Content -LiteralPath (Join-Path $headlessGameRoot "game.agent.json") -Raw | ConvertFrom-Json
    if ($headlessManifest.packagingTargets -notcontains "source-tree-default") {
        Write-Error "Headless scaffold manifest must preserve source-tree-default packaging target"
    }
    $headlessCmake = Get-Content -LiteralPath (Join-Path $headlessScaffoldRoot "games/CMakeLists.txt") -Raw
    Assert-ContainsText $headlessCmake "MK_add_game(dry_run_game" "Headless scaffold CMake"
    if ($headlessCmake.Contains("MK_add_desktop_runtime_game")) {
        Write-Error "Headless scaffold must not register as a desktop runtime game"
    }
} finally {
    Remove-ScaffoldCheckRoot $headlessScaffoldRoot
}

$desktopScaffoldRoot = New-ScaffoldCheckRoot
try {
    $desktopDisplayName = 'Desktop "Quoted" \ Game'
    & (Join-Path $PSScriptRoot "new-game.ps1") -Name "desktop_package_game" -DisplayName $desktopDisplayName -RepositoryRoot $desktopScaffoldRoot -Template DesktopRuntimePackage | Out-Null
    $desktopGameRoot = Join-Path $desktopScaffoldRoot "games/desktop_package_game"
    Resolve-RequiredAgentPath "tools/new-game.ps1" | Out-Null
    foreach ($relativePath in @(
        "main.cpp",
        "README.md",
        "game.agent.json",
        "runtime/desktop_package_game.config"
    )) {
        $path = Join-Path $desktopGameRoot $relativePath
        if (-not (Test-Path -LiteralPath $path)) {
            Write-Error "Desktop runtime package scaffold did not create $relativePath"
        }
    }

    $desktopManifest = Get-Content -LiteralPath (Join-Path $desktopGameRoot "game.agent.json") -Raw | ConvertFrom-Json
    if ($desktopManifest.packagingTargets -notcontains "desktop-game-runtime" -or
        $desktopManifest.packagingTargets -notcontains "desktop-runtime-release") {
        Write-Error "Desktop runtime package scaffold manifest must declare desktop package targets"
    }
    if ($desktopManifest.runtimePackageFiles -notcontains "runtime/desktop_package_game.config") {
        Write-Error "Desktop runtime package scaffold manifest must include runtime config in runtimePackageFiles"
    }

    $desktopCmake = Get-Content -LiteralPath (Join-Path $desktopScaffoldRoot "games/CMakeLists.txt") -Raw
    $desktopMain = Get-Content -LiteralPath (Join-Path $desktopGameRoot "main.cpp") -Raw
    $desktopConfig = Get-Content -LiteralPath (Join-Path $desktopGameRoot "runtime/desktop_package_game.config") -Raw
    Assert-ContainsText $desktopCmake "MK_add_desktop_runtime_game(desktop_package_game" "Desktop scaffold CMake"
    Assert-ContainsText $desktopCmake "PACKAGE_FILES_FROM_MANIFEST" "Desktop scaffold CMake"
    Assert-ContainsText $desktopCmake "GAME_MANIFEST" "Desktop scaffold CMake"
    Assert-ContainsText $desktopMain '.title = "Desktop \"Quoted\" \\ Game"' "Desktop scaffold main.cpp"
    Assert-ContainsText $desktopConfig 'displayName=Desktop "Quoted" \ Game' "Desktop scaffold config"
} finally {
    Remove-ScaffoldCheckRoot $desktopScaffoldRoot
}

$cookedSceneScaffoldRoot = New-ScaffoldCheckRoot
try {
    & (Join-Path $PSScriptRoot "new-game.ps1") -Name "desktop_cooked_scene_game" -RepositoryRoot $cookedSceneScaffoldRoot -Template DesktopRuntimeCookedScenePackage | Out-Null
    $cookedSceneGameRoot = Join-Path $cookedSceneScaffoldRoot "games/desktop_cooked_scene_game"
    $expectedCookedPackageFiles = @(
        "runtime/desktop_cooked_scene_game.config",
        "runtime/desktop_cooked_scene_game.geindex",
        "runtime/assets/generated/base_color.texture.geasset",
        "runtime/assets/generated/triangle.mesh",
        "runtime/assets/generated/lit.material",
        "runtime/assets/generated/packaged_scene.scene"
    )
    foreach ($relativePath in @("main.cpp", "README.md", "game.agent.json", "runtime/.gitattributes") + $expectedCookedPackageFiles) {
        $path = Join-Path $cookedSceneGameRoot $relativePath
        if (-not (Test-Path -LiteralPath $path)) {
            Write-Error "Desktop runtime cooked scene package scaffold did not create $relativePath"
        }
    }

    $cookedSceneManifest = Get-Content -LiteralPath (Join-Path $cookedSceneGameRoot "game.agent.json") -Raw | ConvertFrom-Json
    if ($cookedSceneManifest.packagingTargets -notcontains "desktop-game-runtime" -or
        $cookedSceneManifest.packagingTargets -notcontains "desktop-runtime-release") {
        Write-Error "Desktop runtime cooked scene package scaffold manifest must declare desktop package targets"
    }
    foreach ($relativePath in $expectedCookedPackageFiles) {
        if ($cookedSceneManifest.runtimePackageFiles -notcontains $relativePath) {
            Write-Error "Desktop runtime cooked scene package scaffold manifest must include $relativePath in runtimePackageFiles"
        }
    }
    Assert-RuntimeSceneValidationTarget `
        $cookedSceneManifest `
        "Desktop runtime cooked scene package scaffold manifest" `
        "packaged-scene" `
        "runtime/desktop_cooked_scene_game.geindex" `
        "desktop-cooked-scene-game/scenes/packaged-scene"
    Assert-PackageStreamingResidencyTarget `
        $cookedSceneManifest `
        "Desktop runtime cooked scene package scaffold manifest" `
        "packaged-scene-residency-budget" `
        "runtime/desktop_cooked_scene_game.geindex" `
        "packaged-scene"

    $cookedSceneCmake = Get-Content -LiteralPath (Join-Path $cookedSceneScaffoldRoot "games/CMakeLists.txt") -Raw
    $cookedSceneMain = Get-Content -LiteralPath (Join-Path $cookedSceneGameRoot "main.cpp") -Raw
    $cookedSceneGitAttributes = Get-Content -LiteralPath (Join-Path $cookedSceneGameRoot "runtime/.gitattributes") -Raw
    $cookedSceneIndex = Get-Content -LiteralPath (Join-Path $cookedSceneGameRoot "runtime/desktop_cooked_scene_game.geindex") -Raw
    Assert-ContainsText $cookedSceneCmake "MK_add_desktop_runtime_game(desktop_cooked_scene_game" "Desktop cooked scene scaffold CMake"
    Assert-ContainsText $cookedSceneCmake "PACKAGE_FILES_FROM_MANIFEST" "Desktop cooked scene scaffold CMake"
    Assert-ContainsText $cookedSceneCmake "--require-scene-package" "Desktop cooked scene scaffold CMake"
    Assert-ContainsText $cookedSceneCmake "target_link_libraries(desktop_cooked_scene_game" "Desktop cooked scene scaffold CMake"
    Assert-ContainsText $cookedSceneCmake "MK_runtime" "Desktop cooked scene scaffold CMake"
    Assert-ContainsText $cookedSceneCmake "MK_scene_renderer" "Desktop cooked scene scaffold CMake"
    Assert-ContainsText $cookedSceneMain "load_runtime_asset_package" "Desktop cooked scene scaffold main.cpp"
    Assert-ContainsText $cookedSceneMain "instantiate_runtime_scene_render_data" "Desktop cooked scene scaffold main.cpp"
    Assert-ContainsText $cookedSceneMain "submit_scene_render_packet" "Desktop cooked scene scaffold main.cpp"
    Assert-ContainsText $cookedSceneMain "packaged_scene_asset_id" "Desktop cooked scene scaffold main.cpp"
    foreach ($attributeRule in @(
        "*.geindex text eol=lf",
        "*.geasset text eol=lf",
        "*.mesh text eol=lf",
        "*.material text eol=lf",
        "*.scene text eol=lf"
    )) {
        Assert-ContainsText $cookedSceneGitAttributes $attributeRule "Desktop cooked scene scaffold runtime/.gitattributes"
    }
    foreach ($relativePath in @(
        "runtime/desktop_cooked_scene_game.geindex",
        "runtime/assets/generated/base_color.texture.geasset",
        "runtime/assets/generated/triangle.mesh",
        "runtime/assets/generated/lit.material",
        "runtime/assets/generated/packaged_scene.scene"
    )) {
        $bytes = [System.IO.File]::ReadAllBytes((Join-Path $cookedSceneGameRoot $relativePath))
        if ($bytes -contains [byte]13) {
            Write-Error "Desktop runtime cooked scene package scaffold wrote CR bytes into hash-sensitive file: $relativePath"
        }
    }
    Assert-ContainsText $cookedSceneIndex "dependency.0.kind=material_texture" "Desktop cooked scene scaffold geindex"
    Assert-ContainsText $cookedSceneIndex "dependency.0.path=runtime/assets/generated/base_color.texture.geasset" "Desktop cooked scene scaffold geindex"
    Assert-ContainsText $cookedSceneIndex "dependency.1.kind=scene_material" "Desktop cooked scene scaffold geindex"
    Assert-ContainsText $cookedSceneIndex "dependency.1.path=runtime/assets/generated/lit.material" "Desktop cooked scene scaffold geindex"
    Assert-ContainsText $cookedSceneIndex "dependency.2.kind=scene_mesh" "Desktop cooked scene scaffold geindex"
    Assert-ContainsText $cookedSceneIndex "dependency.2.path=runtime/assets/generated/triangle.mesh" "Desktop cooked scene scaffold geindex"
    if ($cookedSceneIndex.Contains("kind=source_file")) {
        Write-Error "Desktop runtime cooked scene package scaffold geindex must not use source_file dependency edges for runtime scene fixtures"
    }
} finally {
    Remove-ScaffoldCheckRoot $cookedSceneScaffoldRoot
}

$materialShaderScaffoldRoot = New-ScaffoldCheckRoot
try {
    & (Join-Path $PSScriptRoot "new-game.ps1") -Name "desktop_material_shader_game" -RepositoryRoot $materialShaderScaffoldRoot -Template DesktopRuntimeMaterialShaderPackage | Out-Null
    $materialShaderGameRoot = Join-Path $materialShaderScaffoldRoot "games/desktop_material_shader_game"
    $expectedMaterialShaderRuntimeFiles = @(
        "runtime/desktop_material_shader_game.config",
        "runtime/desktop_material_shader_game.geindex",
        "runtime/assets/generated/base_color.texture.geasset",
        "runtime/assets/generated/triangle.mesh",
        "runtime/assets/generated/lit.material",
        "runtime/assets/generated/packaged_scene.scene"
    )
    foreach ($relativePath in @(
        "main.cpp",
        "README.md",
        "game.agent.json",
        "runtime/.gitattributes",
        "source/materials/lit.material",
        "shaders/runtime_scene.hlsl",
        "shaders/runtime_postprocess.hlsl"
    ) + $expectedMaterialShaderRuntimeFiles) {
        $path = Join-Path $materialShaderGameRoot $relativePath
        if (-not (Test-Path -LiteralPath $path)) {
            Write-Error "Desktop runtime material/shader package scaffold did not create $relativePath"
        }
    }

    $materialShaderManifest = Get-Content -LiteralPath (Join-Path $materialShaderGameRoot "game.agent.json") -Raw | ConvertFrom-Json
    if ($materialShaderManifest.packagingTargets -notcontains "desktop-game-runtime" -or
        $materialShaderManifest.packagingTargets -notcontains "desktop-runtime-release") {
        Write-Error "Desktop runtime material/shader package scaffold manifest must declare desktop package targets"
    }
    foreach ($relativePath in $expectedMaterialShaderRuntimeFiles) {
        if ($materialShaderManifest.runtimePackageFiles -notcontains $relativePath) {
            Write-Error "Desktop runtime material/shader package scaffold manifest must include $relativePath in runtimePackageFiles"
        }
    }
    foreach ($authoringPath in @("source/materials/lit.material", "shaders/runtime_scene.hlsl", "shaders/runtime_postprocess.hlsl")) {
        if ($materialShaderManifest.runtimePackageFiles -contains $authoringPath) {
            Write-Error "Desktop runtime material/shader package scaffold must not ship authoring file in runtimePackageFiles: $authoringPath"
        }
    }
    Assert-RuntimeSceneValidationTarget `
        $materialShaderManifest `
        "Desktop runtime material/shader package scaffold manifest" `
        "packaged-scene" `
        "runtime/desktop_material_shader_game.geindex" `
        "desktop-material-shader-game/scenes/packaged-scene"
    Assert-MaterialShaderAuthoringTarget `
        $materialShaderManifest `
        "Desktop runtime material/shader package scaffold manifest" `
        "generated-lit-material-shaders" `
        "source/materials/lit.material" `
        "runtime/assets/generated/lit.material" `
        "runtime/desktop_material_shader_game.geindex"
    Assert-PackageStreamingResidencyTarget `
        $materialShaderManifest `
        "Desktop runtime material/shader package scaffold manifest" `
        "packaged-scene-residency-budget" `
        "runtime/desktop_material_shader_game.geindex" `
        "packaged-scene"

    $materialShaderCmake = Get-Content -LiteralPath (Join-Path $materialShaderScaffoldRoot "games/CMakeLists.txt") -Raw
    $materialShaderMain = Get-Content -LiteralPath (Join-Path $materialShaderGameRoot "main.cpp") -Raw
    $materialShaderSourceMaterial = Get-Content -LiteralPath (Join-Path $materialShaderGameRoot "source/materials/lit.material") -Raw
    $materialShaderSceneHlsl = Get-Content -LiteralPath (Join-Path $materialShaderGameRoot "shaders/runtime_scene.hlsl") -Raw
    $materialShaderPostprocessHlsl = Get-Content -LiteralPath (Join-Path $materialShaderGameRoot "shaders/runtime_postprocess.hlsl") -Raw
    Assert-ContainsText $materialShaderCmake "MK_add_desktop_runtime_game(desktop_material_shader_game" "Desktop material/shader scaffold CMake"
    Assert-ContainsText $materialShaderCmake "PACKAGE_FILES_FROM_MANIFEST" "Desktop material/shader scaffold CMake"
    Assert-ContainsText $materialShaderCmake "REQUIRES_D3D12_SHADERS" "Desktop material/shader scaffold CMake"
    Assert-ContainsText $materialShaderCmake "MK_configure_desktop_runtime_scene_shader_artifacts" "Desktop material/shader scaffold CMake"
    Assert-ContainsText $materialShaderCmake "runtime_scene.hlsl" "Desktop material/shader scaffold CMake"
    Assert-ContainsText $materialShaderCmake "runtime_postprocess.hlsl" "Desktop material/shader scaffold CMake"
    Assert-ContainsText $materialShaderCmake "--require-d3d12-scene-shaders" "Desktop material/shader scaffold CMake"
    $repositoryGamesCmake = Get-AgentSurfaceText "games/CMakeLists.txt"
    Assert-ContainsText $repositoryGamesCmake '${target_name}_runtime_files' "Desktop runtime package staging target"
    Assert-ContainsText $repositoryGamesCmake 'RUNTIME_OUTPUT_DIRECTORY' "Desktop runtime package staging target"
    Assert-ContainsText $repositoryGamesCmake '${target_name}' "Desktop runtime package staging target"
    Assert-ContainsText $repositoryGamesCmake 'DEPENDS ${MK_DESKTOP_GAME_PACKAGE_FILE_REAL_PATHS}' "Desktop runtime package staging target"
    Assert-ContainsText $repositoryGamesCmake 'add_dependencies(${target_name} ${target_name}_runtime_files)' "Desktop runtime package staging target"
    Assert-ContainsText $repositoryGamesCmake "function(MK_configure_desktop_runtime_scene_shader_artifacts)" "Desktop material/shader scaffold CMake helper"
    Assert-ContainsText $repositoryGamesCmake "MK_DESKTOP_RUNTIME_D3D12_SHADER_ARTIFACTS" "Desktop material/shader scaffold CMake helper"
    Assert-ContainsText $repositoryGamesCmake "MK_DESKTOP_RUNTIME_VULKAN_SHADER_ARTIFACTS" "Desktop material/shader scaffold CMake helper"
    Assert-ContainsText $repositoryGamesCmake "MORPH_VERTEX_ENTRY" "Desktop material/shader scaffold CMake helper"
    Assert-ContainsText $repositoryGamesCmake "_scene_morph.vs.dxil" "Desktop material/shader scaffold CMake helper"
    Assert-ContainsText $repositoryGamesCmake "_scene_morph.vs.spv" "Desktop material/shader scaffold CMake helper"
    Assert-ContainsText $repositoryGamesCmake "_scene_compute_morph.vs.spv" "Desktop material/shader scaffold CMake helper"
    Assert-ContainsText $repositoryGamesCmake "_scene_compute_morph.cs.spv" "Desktop material/shader scaffold CMake helper"
    Assert-ContainsText $repositoryGamesCmake "-spirv -fspv-target-env=vulkan1.3" "Desktop material/shader scaffold CMake helper"
    Assert-ContainsText $repositoryGamesCmake '${MK_SCENE_SHADER_TARGET}_shader_artifacts_smoke' "Desktop material/shader scaffold source-tree shader smoke"
    Assert-ContainsText ($materialShaderManifest.validationRecipes | ConvertTo-Json -Depth 12) "--require-vulkan-scene-shaders" "Desktop material/shader scaffold manifest validation recipes"
    Assert-ContainsText $materialShaderMain "load_runtime_asset_package" "Desktop material/shader scaffold main.cpp"
    Assert-ContainsText $materialShaderSourceMaterial "format=GameEngine.Material.v1" "Desktop material/shader scaffold source material"
    Assert-ContainsText $materialShaderSceneHlsl "vs_main" "Desktop material/shader scaffold scene shader"
    Assert-ContainsText $materialShaderSceneHlsl "ps_main" "Desktop material/shader scaffold scene shader"
    Assert-ContainsText $materialShaderPostprocessHlsl "vs_postprocess" "Desktop material/shader scaffold postprocess shader"
    Assert-ContainsText $materialShaderPostprocessHlsl "ps_postprocess" "Desktop material/shader scaffold postprocess shader"
} finally {
    Remove-ScaffoldCheckRoot $materialShaderScaffoldRoot
}

$desktop2dScaffoldRoot = New-ScaffoldCheckRoot
try {
    & (Join-Path $PSScriptRoot "new-game.ps1") -Name "desktop_2d_package_game" -RepositoryRoot $desktop2dScaffoldRoot -Template DesktopRuntime2DPackage | Out-Null
    $desktop2dGameRoot = Join-Path $desktop2dScaffoldRoot "games/desktop_2d_package_game"
    $expectedDesktop2dRuntimeFiles = @(
        "runtime/desktop_2d_package_game.config",
        "runtime/desktop_2d_package_game.geindex",
        "runtime/assets/2d/player.texture.geasset",
        "runtime/assets/2d/player.material",
        "runtime/assets/2d/jump.audio.geasset",
        "runtime/assets/2d/level.tilemap",
        "runtime/assets/2d/player.sprite_animation",
        "runtime/assets/2d/playable.scene"
    )
    foreach ($relativePath in @("main.cpp", "README.md", "game.agent.json", "runtime/.gitattributes", "shaders/runtime_2d_sprite.hlsl") + $expectedDesktop2dRuntimeFiles) {
        $path = Join-Path $desktop2dGameRoot $relativePath
        if (-not (Test-Path -LiteralPath $path)) {
            Write-Error "Desktop runtime 2D package scaffold did not create $relativePath"
        }
    }

    $desktop2dManifest = Get-Content -LiteralPath (Join-Path $desktop2dGameRoot "game.agent.json") -Raw | ConvertFrom-Json
    if ($desktop2dManifest.gameplayContract.productionRecipe -ne "2d-desktop-runtime-package") {
        Write-Error "Desktop runtime 2D package scaffold manifest must select 2d-desktop-runtime-package"
    }
    foreach ($module in @("MK_runtime", "MK_runtime_scene", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
        if (@($desktop2dManifest.engineModules) -notcontains $module) {
            Write-Error "Desktop runtime 2D package scaffold manifest missing engine module: $module"
        }
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($desktop2dManifest.packagingTargets) -notcontains $target) {
            Write-Error "Desktop runtime 2D package scaffold manifest packagingTargets missing $target"
        }
    }
    foreach ($relativePath in $expectedDesktop2dRuntimeFiles) {
        if ($desktop2dManifest.runtimePackageFiles -notcontains $relativePath) {
            Write-Error "Desktop runtime 2D package scaffold manifest must include $relativePath in runtimePackageFiles"
        }
    }
    foreach ($sourcePath in @("source/player.png", "source/scene.scene", "source/audio/jump.wav", "shaders/runtime_scene.hlsl", "shaders/runtime_2d_sprite.hlsl")) {
        if ($desktop2dManifest.runtimePackageFiles -contains $sourcePath) {
            Write-Error "Desktop runtime 2D package scaffold must not ship source authoring file in runtimePackageFiles: $sourcePath"
        }
    }
    Assert-RuntimeSceneValidationTarget `
        $desktop2dManifest `
        "Desktop runtime 2D package scaffold manifest" `
        "packaged-2d-scene" `
        "runtime/desktop_2d_package_game.geindex" `
        "desktop-2d-package-game/scenes/packaged-2d-scene"
    Assert-PackageStreamingResidencyTarget `
        $desktop2dManifest `
        "Desktop runtime 2D package scaffold manifest" `
        "packaged-2d-residency-budget" `
        "runtime/desktop_2d_package_game.geindex" `
        "packaged-2d-scene"
    Assert-AtlasTilemapAuthoringTarget `
        $desktop2dManifest `
        "Desktop runtime 2D package scaffold manifest" `
        "packaged-2d-tilemap" `
        "runtime/desktop_2d_package_game.geindex" `
        "runtime/assets/2d/level.tilemap" `
        "runtime/assets/2d/player.texture.geasset"
    foreach ($recipe in @("desktop-game-runtime", "desktop-runtime-release-target", "installed-2d-package-smoke", "installed-2d-sprite-animation-smoke", "installed-2d-tilemap-runtime-ux-smoke", "installed-native-2d-sprite-smoke")) {
        if (@($desktop2dManifest.validationRecipes | ForEach-Object { $_.name }) -notcontains $recipe) {
            Write-Error "Desktop runtime 2D package scaffold manifest validationRecipes missing $recipe"
        }
    }

    $desktop2dCmake = Get-Content -LiteralPath (Join-Path $desktop2dScaffoldRoot "games/CMakeLists.txt") -Raw
    $desktop2dMain = Get-Content -LiteralPath (Join-Path $desktop2dGameRoot "main.cpp") -Raw
    $desktop2dReadme = Get-Content -LiteralPath (Join-Path $desktop2dGameRoot "README.md") -Raw
    $desktop2dGitAttributes = Get-Content -LiteralPath (Join-Path $desktop2dGameRoot "runtime/.gitattributes") -Raw
    $desktop2dShader = Get-Content -LiteralPath (Join-Path $desktop2dGameRoot "shaders/runtime_2d_sprite.hlsl") -Raw
    $desktop2dIndex = Get-Content -LiteralPath (Join-Path $desktop2dGameRoot "runtime/desktop_2d_package_game.geindex") -Raw
    $desktop2dScene = Get-Content -LiteralPath (Join-Path $desktop2dGameRoot "runtime/assets/2d/playable.scene") -Raw
    $desktop2dSpriteAnimation = Get-Content -LiteralPath (Join-Path $desktop2dGameRoot "runtime/assets/2d/player.sprite_animation") -Raw
    foreach ($attributeRule in @(
        "*.geindex text eol=lf",
        "*.geasset text eol=lf",
        "*.material text eol=lf",
        "*.scene text eol=lf",
        "*.tilemap text eol=lf",
        "*.sprite_animation text eol=lf"
    )) {
        Assert-ContainsText $desktop2dGitAttributes $attributeRule "Desktop 2D scaffold runtime/.gitattributes"
    }
    Assert-ContainsText $desktop2dCmake "MK_add_desktop_runtime_game(desktop_2d_package_game" "Desktop 2D scaffold CMake"
    Assert-ContainsText $desktop2dCmake "PACKAGE_FILES_FROM_MANIFEST" "Desktop 2D scaffold CMake"
    Assert-ContainsText $desktop2dCmake "--require-scene-package" "Desktop 2D scaffold CMake"
    Assert-ContainsText $desktop2dCmake "--require-d3d12-shaders" "Desktop 2D scaffold CMake"
    Assert-ContainsText $desktop2dCmake "--require-native-2d-sprites" "Desktop 2D scaffold CMake"
    Assert-ContainsText $desktop2dCmake "--require-sprite-animation" "Desktop 2D scaffold CMake"
    Assert-ContainsText $desktop2dCmake "--require-tilemap-runtime-ux" "Desktop 2D scaffold CMake"
    Assert-ContainsText $desktop2dCmake "REQUIRES_D3D12_SHADERS" "Desktop 2D scaffold CMake"
    Assert-ContainsText $desktop2dCmake "MK_configure_desktop_runtime_2d_sprite_shader_artifacts" "Desktop 2D scaffold CMake"
    Assert-ContainsText $desktop2dCmake "runtime_2d_sprite.hlsl" "Desktop 2D scaffold CMake"
    Assert-ContainsText $desktop2dCmake "MK_audio" "Desktop 2D scaffold CMake"
    Assert-ContainsText $desktop2dCmake "MK_ui_renderer" "Desktop 2D scaffold CMake"
    Assert-ContainsText $desktop2dMain "RuntimeInputActionMap" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "validate_playable_2d_scene" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "submit_ui_renderer_submission" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "AudioMixer" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "packaged_audio_asset_id" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "packaged_sprite_animation_asset_id" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "packaged_tilemap_asset_id" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "runtime_sprite_animation_payload" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "runtime_tilemap_payload" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "sample_runtime_tilemap_visible_cells" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "sample_and_apply_runtime_scene_render_sprite_animation" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "desktop-2d-package-game/textures/player" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "desktop-2d-package-game/animations/player-sprite-animation" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "--require-native-2d-sprites" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "--require-sprite-animation" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "--require-tilemap-runtime-ux" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "native_2d_sprites_status" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "sprite_animation_frames_sampled" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dMain "tilemap_cells_sampled" "Desktop 2D scaffold main.cpp"
    Assert-ContainsText $desktop2dReadme "--require-native-2d-sprites" "Desktop 2D scaffold README"
    Assert-ContainsText $desktop2dReadme "--require-sprite-animation" "Desktop 2D scaffold README"
    Assert-ContainsText $desktop2dReadme "--require-tilemap-runtime-ux" "Desktop 2D scaffold README"
    Assert-ContainsText $desktop2dReadme "production sprite batching" "Desktop 2D scaffold README"
    Assert-ContainsText $desktop2dShader "vs_native_sprite_overlay" "Desktop 2D scaffold shader"
    Assert-ContainsText $desktop2dShader "ps_native_sprite_overlay" "Desktop 2D scaffold shader"
    Assert-ContainsText $desktop2dScene "node.1.camera.projection=orthographic" "Desktop 2D scaffold scene"
    Assert-ContainsText $desktop2dScene "node.2.sprite_renderer.visible=true" "Desktop 2D scaffold scene"
    Assert-ContainsText $desktop2dIndex "dependency.0.kind=scene_sprite" "Desktop 2D scaffold geindex"
    Assert-ContainsText $desktop2dIndex "entry.3.kind=audio" "Desktop 2D scaffold geindex"
    Assert-ContainsText $desktop2dIndex "entry.4.kind=tilemap" "Desktop 2D scaffold geindex"
    Assert-ContainsText $desktop2dIndex "entry.5.kind=sprite_animation" "Desktop 2D scaffold geindex"
    Assert-ContainsText $desktop2dIndex "dependency.3.kind=tilemap_texture" "Desktop 2D scaffold geindex"
    Assert-ContainsText $desktop2dIndex "dependency.4.kind=sprite_animation_texture" "Desktop 2D scaffold geindex"
    Assert-ContainsText $desktop2dIndex "dependency.5.kind=sprite_animation_material" "Desktop 2D scaffold geindex"
    Assert-ContainsText $desktop2dSpriteAnimation "format=GameEngine.CookedSpriteAnimation.v1" "Desktop 2D scaffold sprite animation"
    Assert-ContainsText $desktop2dSpriteAnimation "asset.kind=sprite_animation" "Desktop 2D scaffold sprite animation"
    Assert-ContainsText $desktop2dSpriteAnimation "target.node=Player" "Desktop 2D scaffold sprite animation"
    if ($desktop2dIndex.Contains("kind=source_file")) {
        Write-Error "Desktop runtime 2D package scaffold geindex must not use source_file dependency edges"
    }
} finally {
    Remove-ScaffoldCheckRoot $desktop2dScaffoldRoot
}

$spriteBatchHeader = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/sprite_batch.hpp"
$spriteBatchSource = Get-AgentSurfaceText "engine/renderer/src/sprite_batch.cpp"
$sceneRendererHeader = Get-AgentSurfaceText "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp"
$sceneRendererSource = Get-AgentSurfaceText "engine/scene_renderer/src/scene_renderer.cpp"
$sceneRendererTests = Get-AgentSurfaceText "tests/unit/scene_renderer_tests.cpp"
$runtimeHeader = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/asset_runtime.hpp"
$runtimeSource = Get-AgentSurfaceText "engine/runtime/src/asset_runtime.cpp"
$runtimeSessionHeader = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/session_services.hpp"
$runtimeSessionSource = Get-AgentSurfaceText "engine/runtime/src/session_services.cpp"
$runtimeTests = Get-AgentSurfaceText "tests/unit/runtime_tests.cpp"
$rendererTests = Get-AgentSurfaceText "tests/unit/renderer_rhi_tests.cpp"
$frameGraphHeader = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/frame_graph.hpp"
$frameGraphSource = Get-AgentSurfaceText "engine/renderer/src/frame_graph.cpp"
$frameGraphRhiHeader = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp"
$frameGraphRhiSource = Get-AgentSurfaceText "engine/renderer/src/frame_graph_rhi.cpp"
$rhiUploadStagingHeader = Get-AgentSurfaceText "engine/rhi/include/mirakana/rhi/upload_staging.hpp"
$rhiUploadStagingSource = Get-AgentSurfaceText "engine/rhi/src/upload_staging.cpp"
$rhiUploadStagingTests = Get-AgentSurfaceText "tests/unit/rhi_upload_staging_tests.cpp"
$runtimeRhiUploadHeader = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp"
$runtimeRhiUploadSource = Get-AgentSurfaceText "engine/runtime_rhi/src/runtime_upload.cpp"
$runtimeSceneRhiHeader = Get-AgentSurfaceText "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"
$runtimeSceneRhiSource = Get-AgentSurfaceText "engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp"
$runtimeRhiTests = Get-AgentSurfaceText "tests/unit/runtime_rhi_tests.cpp"
$runtimeSceneRhiTests = Get-AgentSurfaceText "tests/unit/runtime_scene_rhi_tests.cpp"
$runtimeUploadFencePlan = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-08-runtime-rhi-upload-submission-fence-rows-v1.md"
$frameGraphRhiTextureSchedulePlan =
    Get-AgentSurfaceText "docs/superpowers/plans/2026-05-08-frame-graph-rhi-texture-schedule-execution-v1.md"
$rhiUploadStaleGenerationPlan =
    Get-AgentSurfaceText "docs/superpowers/plans/2026-05-08-rhi-upload-stale-generation-diagnostics-v1.md"
$rendererCmake = Get-AgentSurfaceText "engine/renderer/CMakeLists.txt"
foreach ($needle in @(
    "FrameGraphPassExecutionBinding",
    "FrameGraphExecutionCallbacks",
    "FrameGraphExecutionResult",
    "execute_frame_graph_v1_schedule"
)) {
    Assert-ContainsText $frameGraphHeader $needle "Frame Graph callback execution header"
}
foreach ($needle in @(
    "frame graph pass callback is missing",
    "frame graph barrier callback is missing",
    "frame graph pass callback threw an exception",
    "frame graph barrier callback threw an exception",
    "frame graph pass callback failed",
    "frame graph barrier callback failed"
)) {
    Assert-ContainsText $frameGraphSource $needle "Frame Graph callback execution source"
}
foreach ($needle in @(
    "frame graph v1 dispatches barrier and pass callbacks in schedule order",
    "frame graph v1 callback execution diagnoses missing callbacks before later passes",
    "frame graph v1 callback execution converts thrown callbacks to diagnostics",
    "frame graph v1 callback execution copies pass bindings before dispatch",
    "frame graph v1 callback execution reports returned callback failures",
    "frame graph v1 callback execution converts thrown barrier callbacks to diagnostics"
)) {
    Assert-ContainsText $rendererTests $needle "MK_renderer_tests Frame Graph callback execution coverage"
}
foreach ($needle in @(
    "FrameGraphRhiTextureExecutionDesc",
    "FrameGraphRhiTextureExecutionResult",
    "execute_frame_graph_rhi_texture_schedule"
)) {
    Assert-ContainsText $frameGraphRhiHeader $needle "Frame Graph RHI texture schedule execution header"
}
foreach ($needle in @(
    "frame graph rhi texture schedule execution requires a command list",
    "frame graph rhi texture schedule execution cannot use a closed command list",
    "frame graph pass callback is empty",
    "frame graph pass callback is declared more than once",
    "frame graph texture barrier recording failed"
)) {
    Assert-ContainsText $frameGraphRhiSource $needle "Frame Graph RHI texture schedule execution source"
}
foreach ($needle in @(
    "frame graph rhi texture schedule execution interleaves barriers and pass callbacks",
    "frame graph rhi texture schedule execution validates barriers before pass callbacks",
    "frame graph rhi texture schedule execution validates pass callbacks before barriers"
)) {
    Assert-ContainsText $rendererTests $needle "MK_renderer_tests Frame Graph RHI texture schedule execution coverage"
}
foreach ($needle in @(
    "**Status:** Completed.",
    "FrameGraphRhiTextureExecutionDesc",
    "FrameGraphRhiTextureExecutionResult",
    "execute_frame_graph_rhi_texture_schedule"
)) {
    Assert-ContainsText $frameGraphRhiTextureSchedulePlan $needle "Frame Graph RHI texture schedule execution plan"
}
foreach ($needle in @(
    "submitted_fence",
    "RuntimeTextureUploadResult",
    "RuntimeMeshUploadResult",
    "RuntimeSkinnedMeshUploadResult",
    "RuntimeMorphMeshUploadResult",
    "RuntimeMaterialGpuBinding"
)) {
    Assert-ContainsText $runtimeRhiUploadHeader $needle "Runtime RHI upload submission fence header"
}
foreach ($needle in @("submitted_upload_fences", "submitted_upload_fence_count", "last_submitted_upload_fence")) {
    Assert-ContainsText $runtimeSceneRhiHeader $needle "Runtime scene RHI upload execution report header"
}
foreach ($needle in @(
    "result.submitted_fence = fence",
    "return RuntimeTextureUploadResult",
    "return RuntimeMeshUploadResult",
    "return RuntimeSkinnedMeshUploadResult",
    "return RuntimeMorphMeshUploadResult"
)) {
    Assert-ContainsText $runtimeRhiUploadSource $needle "Runtime RHI upload submission fence source"
}
foreach ($needle in @(
    "record_submitted_upload_fence",
    "result.submitted_upload_fences.push_back",
    "upload.submitted_fence",
    "base_upload.submitted_fence",
    "binding.submitted_fence",
    "bindings.submitted_upload_fences"
)) {
    Assert-ContainsText $runtimeSceneRhiSource $needle "Runtime scene RHI upload fence aggregation source"
}
foreach ($needle in @(
    "runtime rhi upload reports submitted fence without forcing wait",
    "result.submitted_fence.value != 0",
    "binding.submitted_fence.value != 0",
    "upload.submitted_fence.value != 0"
)) {
    Assert-ContainsText $runtimeRhiTests $needle "MK_runtime_rhi_tests upload submission fence coverage"
}
foreach ($needle in @(
    "runtime scene rhi upload execution preserves submitted fences in submit order across queues",
    "submitted_upload_fences[0].queue == mirakana::rhi::QueueKind::compute",
    "submitted_upload_fences[1].value == compute_resource.base_position_upload.submitted_fence.value",
    "submitted_upload_fence_count == 3",
    "submitted_upload_fence_count == 4",
    "last_submitted_upload_fence.value != 0"
)) {
    Assert-ContainsText $runtimeSceneRhiTests $needle "MK_runtime_scene_rhi_tests upload submission fence coverage"
}
foreach ($needle in @(
    "**Status:** Completed",
    "Runtime RHI Upload Submission Fence Rows v1",
    "submitted_upload_fences",
    "submitted_upload_fence_count",
    "native async upload execution"
)) {
    Assert-ContainsText $runtimeUploadFencePlan $needle "Runtime RHI Upload Submission Fence Rows plan"
}
foreach ($needle in @(
    "RhiUploadDiagnosticCode",
    "stale_generation",
    "RhiUploadRing"
)) {
    Assert-ContainsText $rhiUploadStagingHeader $needle "RHI upload staging stale-generation header"
}
foreach ($needle in @(
    "inactive_allocation_code",
    "RHI upload staging allocation generation is stale.",
    "span.allocation_generation",
    "RhiUploadDiagnosticCode::stale_generation"
)) {
    Assert-ContainsText $rhiUploadStagingSource $needle "RHI upload staging stale-generation source"
}
foreach ($needle in @(
    "rhi upload staging diagnoses stale allocation generations",
    "rhi upload ring ownership requires matching allocation generation",
    "RhiUploadDiagnosticCode::stale_generation",
    "!ring.owns_allocation(stale)"
)) {
    Assert-ContainsText $rhiUploadStagingTests $needle "MK_rhi_upload_staging_tests stale-generation coverage"
}
foreach ($needle in @(
    "**Status:** Completed.",
    "RHI Upload Stale Generation Diagnostics v1",
    "stale_generation",
    "native async upload execution"
)) {
    Assert-ContainsText $rhiUploadStaleGenerationPlan $needle "RHI Upload Stale Generation Diagnostics plan"
}
foreach ($docCheck in @(
    @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" },
    @{ Text = $rhiText; Label = "docs/rhi.md" },
    @{ Text = $masterPlanText; Label = "production completion master plan" }
)) {
    Assert-ContainsText $docCheck.Text "RHI Upload Stale Generation Diagnostics v1" $docCheck.Label
    Assert-ContainsText $docCheck.Text "stale_generation" $docCheck.Label
}
foreach ($needle in @(
    "SpriteBatchPlan",
    "SpriteBatchRange",
    "SpriteBatchDiagnosticCode",
    "plan_sprite_batches"
)) {
    Assert-ContainsText $spriteBatchHeader $needle "2D sprite batch planning header"
}
foreach ($needle in @(
    "append_or_extend_batch",
    "missing_texture_atlas",
    "invalid_uv_rect",
    "texture_bind_count"
)) {
    Assert-ContainsText $spriteBatchSource $needle "2D sprite batch planning source"
}
foreach ($needle in @(
    "sprite batch planner preserves order",
    "sprite batch planner diagnoses invalid texture metadata",
    "SpriteBatchDiagnosticCode::missing_texture_atlas",
    "SpriteBatchDiagnosticCode::invalid_uv_rect"
)) {
    Assert-ContainsText $rendererTests $needle "MK_renderer_tests sprite batch coverage"
}
foreach ($needle in @("scene sprite batch telemetry", "plan_scene_sprite_batches")) {
    Assert-ContainsText $sceneRendererTests $needle "MK_scene_renderer_tests sprite batch telemetry coverage"
}
Assert-ContainsText $rendererCmake "src/sprite_batch.cpp" "MK_renderer CMake"
foreach ($needle in @("mirakana/renderer/sprite_batch.hpp", "plan_scene_sprite_batches")) {
    Assert-ContainsText $sceneRendererHeader $needle "MK_scene_renderer sprite batch telemetry header"
    Assert-ContainsText $sceneRendererSource $needle "MK_scene_renderer sprite batch telemetry source"
}
foreach ($needle in @(
    "RuntimeSpriteAnimationPayload",
    "RuntimeSpriteAnimationFrame",
    "runtime_sprite_animation_payload"
)) {
    Assert-ContainsText $runtimeHeader $needle "MK_runtime sprite animation payload header"
    Assert-ContainsText $runtimeSource $needle "MK_runtime sprite animation payload source"
}
foreach ($needle in @(
    "RuntimeTilemapVisibleCellSampleResult",
    "sample_runtime_tilemap_visible_cells"
)) {
    Assert-ContainsText $runtimeHeader $needle "MK_runtime tilemap visible cell sampling header"
    Assert-ContainsText $runtimeSource $needle "MK_runtime tilemap visible cell sampling source"
}
foreach ($needle in @(
    "RuntimeInputRebindingProfile",
    "validate_runtime_input_rebinding_profile",
    "apply_runtime_input_rebinding_profile",
    "serialize_runtime_input_rebinding_profile",
    "deserialize_runtime_input_rebinding_profile"
)) {
    Assert-ContainsText $runtimeSessionHeader $needle "MK_runtime input rebinding profile header"
    Assert-ContainsText $runtimeSessionSource $needle "MK_runtime input rebinding profile source"
}
foreach ($needle in @(
    "RuntimeInputRebindingCaptureRequest",
    "RuntimeInputRebindingCaptureResult",
    "RuntimeInputRebindingCaptureStatus",
    "capture_runtime_input_rebinding_action",
    "RuntimeInputRebindingFocusCaptureRequest",
    "RuntimeInputRebindingFocusCaptureResult",
    "capture_runtime_input_rebinding_action_with_focus",
    "capture_not_armed",
    "invalid_capture_id",
    "invalid_capture_focus",
    "modal_layer_mismatch",
    "gameplay_input_consumed",
    "focus_retained"
)) {
    Assert-ContainsText $runtimeSessionHeader $needle "MK_runtime input rebinding capture header"
    Assert-ContainsText $runtimeSessionSource $needle "MK_runtime input rebinding capture source"
}
foreach ($needle in @(
    "RuntimeInputRebindingAxisCaptureRequest",
    "RuntimeInputRebindingAxisCaptureResult",
    "capture_runtime_input_rebinding_axis"
)) {
    Assert-ContainsText $runtimeSessionHeader $needle "MK_runtime input rebinding axis capture header"
    Assert-ContainsText $runtimeSessionSource $needle "MK_runtime input rebinding axis capture source"
}
foreach ($needle in @(
    "RuntimeInputRebindingPresentationToken",
    "RuntimeInputRebindingPresentationRow",
    "RuntimeInputRebindingPresentationModel",
    "present_runtime_input_action_trigger",
    "present_runtime_input_axis_source",
    "make_runtime_input_rebinding_presentation"
)) {
    Assert-ContainsText $runtimeSessionHeader $needle "MK_runtime input rebinding presentation header"
    Assert-ContainsText $runtimeSessionSource $needle "MK_runtime input rebinding presentation source"
}
foreach ($needle in @(
    "keyboard.key.",
    "keyboard.axis.",
    "runtime input rebinding profile diagnostics block presentation"
)) {
    Assert-ContainsText $runtimeSessionSource $needle "MK_runtime input rebinding presentation source"
}
foreach ($needle in @(
    "RuntimeSceneRenderSpriteAnimationApplyResult",
    "sample_and_apply_runtime_scene_render_sprite_animation"
)) {
    Assert-ContainsText $sceneRendererHeader $needle "MK_scene_renderer sprite animation header"
    Assert-ContainsText $sceneRendererSource $needle "MK_scene_renderer sprite animation source"
}
Assert-ContainsText $runtimeTests "runtime typed payload access decodes cooked sprite animation frames" "sprite animation runtime tests"
Assert-ContainsText $sceneRendererTests "scene renderer samples runtime sprite animation frames into render packet" "sprite animation scene renderer tests"
Assert-ContainsText $runtimeTests "runtime samples visible tilemap cells into deterministic counters" "tilemap runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding profile applies digital and axis overrides" "input rebinding profile runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding capture creates action override from pressed key" "input rebinding capture runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding capture uses deterministic source priority" "input rebinding capture runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding axis capture blocks missing base axis binding" "input rebinding axis capture runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding axis capture waits when no axis exceeds deadzone" "input rebinding axis capture runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding axis capture captures gamepad axis override" "input rebinding axis capture runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding focused capture waits and consumes gameplay input" "input rebinding focus consumption runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding focused capture can retain focus without consuming gameplay input" "input rebinding focus consumption runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding focused capture captures candidate and consumes pressed input" "input rebinding focus consumption runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding focused capture consumes rejected pressed input" "input rebinding focus consumption runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding focused capture blocks invalid focus guard before consuming input" "input rebinding focus consumption runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding focused capture blocks empty capture id" "input rebinding focus consumption runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding presentation rows expose action trigger tokens" "input rebinding presentation runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding presentation rows expose axis source tokens" "input rebinding presentation runtime tests"
Assert-ContainsText $runtimeTests "runtime input rebinding presentation reports invalid profiles" "input rebinding presentation runtime tests"
Assert-ContainsText $runtimeTests "model.diagnostics[0].path == row->id" "input rebinding presentation diagnostic row correlation test"
$editorCoreHeader = Get-AgentSurfaceText "editor/core/include/mirakana/editor/playtest_package_review.hpp"
$editorCoreSource = Get-AgentSurfaceText "editor/core/src/playtest_package_review.cpp"
$editorInputRebindingHeader = Get-AgentSurfaceText "editor/core/include/mirakana/editor/input_rebinding.hpp"
$editorInputRebindingSource = Get-AgentSurfaceText "editor/core/src/input_rebinding.cpp"
$editorWorkspaceHeader = Get-AgentSurfaceText "editor/core/include/mirakana/editor/workspace.hpp"
$editorWorkspaceSource = Get-AgentSurfaceText "editor/core/src/workspace.cpp"
$editorMainSource = Get-AgentSurfaceText "editor/src/main.cpp"
$editorCoreTests = Get-AgentSurfaceText "tests/unit/editor_core_tests.cpp"
foreach ($needle in @(
    "EditorTilemapPackageDiagnosticsModel",
    "make_editor_tilemap_package_diagnostics_model"
)) {
    Assert-ContainsText $editorCoreHeader $needle "editor tilemap package diagnostics header"
    Assert-ContainsText $editorCoreSource $needle "editor tilemap package diagnostics source"
}
Assert-ContainsText $editorCoreTests "editor tilemap package diagnostics surface runtime tilemap rows" "editor tilemap package diagnostics tests"
foreach ($needle in @(
    "EditorInputRebindingProfileReviewModel",
    "make_editor_input_rebinding_profile_review_model"
)) {
    Assert-ContainsText $editorInputRebindingHeader $needle "editor input rebinding profile review header"
    Assert-ContainsText $editorInputRebindingSource $needle "editor input rebinding profile review source"
}
Assert-ContainsText $editorCoreTests "editor input rebinding review model surfaces runtime conflicts" "editor input rebinding profile review tests"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorInputRebindingProfiles) "EditorInputRebindingProfileReviewModel" "editor input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorInputRebindingProfiles) "make_editor_input_rebinding_profile_review_model" "editor input rebinding profile guidance"
foreach ($needle in @(
    "EditorInputRebindingProfilePanelStatus",
    "EditorInputRebindingProfileBindingRow",
    "EditorInputRebindingProfilePanelModel",
    "make_editor_input_rebinding_profile_panel_model",
    "make_input_rebinding_profile_panel_ui_model"
)) {
    Assert-ContainsText $editorInputRebindingHeader $needle "editor input rebinding profile panel header"
    Assert-ContainsText $editorInputRebindingSource $needle "editor input rebinding profile panel source"
}
foreach ($needle in @(
    "editor input rebinding panel model exposes reviewed bindings and ui rows",
    "panel.input_rebinding=visible",
    "input_rebinding.bindings.action.gameplay.confirm.current",
    "input_rebinding.review.profile.status"
)) {
    Assert-ContainsText $editorCoreTests $needle "editor input rebinding profile panel tests"
}
Assert-ContainsText $editorWorkspaceHeader "input_rebinding" "editor workspace input rebinding panel header"
Assert-ContainsText $editorWorkspaceSource 'PanelToken{.id = PanelId::input_rebinding, .token = "input_rebinding"}' "editor workspace input rebinding panel source"
Assert-ContainsText $editorWorkspaceSource "PanelState{.id = PanelId::input_rebinding, .visible = false}" "editor workspace input rebinding panel source"
Assert-ContainsText $editorMainSource "draw_input_rebinding_panel" "MK_editor input rebinding panel source"
Assert-ContainsText $editorMainSource "view.input_rebinding" "MK_editor input rebinding panel source"
Assert-ContainsText $editorMainSource "make_editor_input_rebinding_profile_panel_model" "MK_editor input rebinding panel source"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorInputRebindingProfiles) "EditorInputRebindingProfilePanelModel" "editor input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorInputRebindingProfiles) "make_editor_input_rebinding_profile_panel_model" "editor input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorInputRebindingProfiles) "make_input_rebinding_profile_panel_ui_model" "editor input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorInputRebindingProfiles) "input_rebinding" "editor input rebinding profile guidance"
foreach ($needle in @(
    "EditorInputRebindingCaptureStatus",
    "EditorInputRebindingCaptureModel",
    "make_editor_input_rebinding_capture_action_model",
    "make_input_rebinding_capture_action_ui_model"
)) {
    Assert-ContainsText $editorInputRebindingHeader $needle "editor input rebinding capture panel header"
    Assert-ContainsText $editorInputRebindingSource $needle "editor input rebinding capture panel source"
    Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorInputRebindingProfiles) $needle "editor input rebinding capture guidance"
}
foreach ($needle in @(
    "EditorInputRebindingAxisCaptureDesc",
    "EditorInputRebindingAxisCaptureModel",
    "make_editor_input_rebinding_capture_axis_model",
    "make_input_rebinding_capture_axis_ui_model"
)) {
    Assert-ContainsText $editorInputRebindingHeader $needle "editor input rebinding axis capture panel header"
    Assert-ContainsText $editorInputRebindingSource $needle "editor input rebinding axis capture panel source"
    Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorInputRebindingProfiles) $needle "editor input rebinding axis capture guidance"
}
foreach ($needle in @(
    "editor input rebinding capture model captures pressed key candidate",
    "editor input rebinding capture model waits without mutating profile",
    "editor input rebinding capture model blocks unsupported file command and native claims",
    "editor input rebinding axis capture model captures gamepad axis candidate",
    "input_rebinding.capture.status",
    "input_rebinding.capture.diagnostics",
    "input_rebinding.capture.axis"
)) {
    Assert-ContainsText $editorCoreTests $needle "editor input rebinding capture panel tests"
}
foreach ($needle in @(
    "begin_input_frame",
    "draw_input_rebinding_capture_controls",
    "RuntimeInputStateView",
    "make_editor_input_rebinding_capture_action_model",
    "make_editor_input_rebinding_capture_axis_model",
    "arm_input_rebinding_axis_capture",
    "keyboard_input()",
    "pointer_input()",
    "gamepad_input()"
)) {
    Assert-ContainsText $editorMainSource $needle "MK_editor input rebinding capture panel source"
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorInputRebindingProfiles) "in-memory profile" "editor input rebinding capture guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorInputRebindingProfiles) "axis capture" "editor input rebinding capture guidance"

$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
Assert-DoesNotContainText $manifestText "generated-game morph package consumption/rendering" "engine/agent/manifest.json generated 3D morph stale unsupported claim"
foreach ($needle in @(
    "2d-sprite-batch-planning-contract",
    "2d-sprite-batch-package-telemetry",
    "plan_sprite_batches",
    "plan_scene_sprite_batches",
    "production sprite batching readiness",
    "native_sprite_batches_executed"
)) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json sprite batch planning contract"
}
foreach ($doc in @(
    @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" },
    @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" }
)) {
    Assert-ContainsText $doc.Text "2D Sprite Batch Planning Contract v1" $doc.Label
    Assert-ContainsText $doc.Text "2D Sprite Batch Package Telemetry v1" $doc.Label
    Assert-ContainsText $doc.Text "production sprite batching" $doc.Label
}
Assert-DoesNotContainText $manifestText "production sprite batching ready" "engine/agent/manifest.json sprite batch unsupported claim"

$desktop3dScaffoldRoot = New-ScaffoldCheckRoot
try {
    & (Join-Path $PSScriptRoot "new-game.ps1") -Name "desktop_3d_package_game" -RepositoryRoot $desktop3dScaffoldRoot -Template DesktopRuntime3DPackage | Out-Null
    $desktop3dGameRoot = Join-Path $desktop3dScaffoldRoot "games/desktop_3d_package_game"
    $expectedDesktop3dRuntimeFiles = @(
        "runtime/desktop_3d_package_game.config",
        "runtime/desktop_3d_package_game.geindex",
        "runtime/assets/3d/base_color.texture.geasset",
        "runtime/assets/3d/triangle.mesh",
        "runtime/assets/3d/packaged_mesh.morph_mesh_cpu",
        "runtime/assets/3d/lit.material",
        "runtime/assets/3d/packaged_mesh_bob.animation_float_clip",
        "runtime/assets/3d/packaged_mesh_morph_weights.animation_float_clip",
        "runtime/assets/3d/packaged_pose.animation_quaternion_clip",
        "runtime/assets/3d/skinned_triangle.skinned_mesh",
        "runtime/assets/3d/hud.uiatlas",
        "runtime/assets/3d/hud_text.uiatlas",
        "runtime/assets/3d/packaged_scene.scene",
        "runtime/assets/3d/collision.collision3d"
    )
    foreach ($relativePath in @(
        "main.cpp",
        "README.md",
        "game.agent.json",
        "runtime/.gitattributes",
        "source/assets/package.geassets",
        "source/scenes/packaged_scene.scene",
        "source/prefabs/static_prop.prefab",
        "source/textures/base_color.texture_source",
        "source/meshes/triangle.mesh_source",
        "source/morphs/packaged_mesh.morph_mesh_cpu_source",
        "source/animations/packaged_mesh_bob.animation_float_clip_source",
        "source/animations/packaged_mesh_morph_weights.animation_float_clip_source",
        "source/animations/packaged_pose.animation_quaternion_clip_source",
        "source/materials/lit.material",
        "shaders/runtime_scene.hlsl",
        "shaders/runtime_postprocess.hlsl"
    ) + $expectedDesktop3dRuntimeFiles) {
        $path = Join-Path $desktop3dGameRoot $relativePath
        if (-not (Test-Path -LiteralPath $path)) {
            Write-Error "Desktop runtime 3D package scaffold did not create $relativePath"
        }
    }

    $desktop3dManifest = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "game.agent.json") -Raw | ConvertFrom-Json
    if ($desktop3dManifest.gameplayContract.productionRecipe -ne "3d-playable-desktop-package") {
        Write-Error "Desktop runtime 3D package scaffold manifest must select 3d-playable-desktop-package"
    }
    foreach ($module in @("MK_ai", "MK_animation", "MK_audio", "MK_navigation", "MK_physics", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($desktop3dManifest.engineModules) -notcontains $module) {
            Write-Error "Desktop runtime 3D package scaffold manifest missing engine module: $module"
        }
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($desktop3dManifest.packagingTargets) -notcontains $target) {
            Write-Error "Desktop runtime 3D package scaffold manifest packagingTargets missing $target"
        }
    }
    foreach ($relativePath in $expectedDesktop3dRuntimeFiles) {
        if ($desktop3dManifest.runtimePackageFiles -notcontains $relativePath) {
            Write-Error "Desktop runtime 3D package scaffold manifest must include $relativePath in runtimePackageFiles"
        }
    }
    $desktop3dShaderTargets = @($desktop3dManifest.materialShaderAuthoringTargets | Where-Object { $_.id -eq "packaged-3d-lit-material-shaders" })
    if ($desktop3dShaderTargets.Count -ne 1) {
        Write-Error "Desktop runtime 3D package scaffold manifest must contain packaged-3d-lit-material-shaders"
    } else {
        foreach ($artifact in @("shaders/desktop_3d_package_game_scene_compute_morph.vs.dxil", "shaders/desktop_3d_package_game_scene_compute_morph.cs.dxil", "shaders/desktop_3d_package_game_scene_compute_morph_tangent_frame.vs.dxil", "shaders/desktop_3d_package_game_scene_compute_morph_tangent_frame.cs.dxil", "shaders/desktop_3d_package_game_scene_compute_morph_skinned.vs.dxil", "shaders/desktop_3d_package_game_scene_compute_morph_skinned.cs.dxil")) {
            if (@($desktop3dShaderTargets[0].d3d12ShaderArtifactPaths) -notcontains $artifact) {
                Write-Error "Desktop runtime 3D package scaffold manifest D3D12 artifacts missing $artifact"
            }
        }
        foreach ($artifact in @("shaders/desktop_3d_package_game_scene_compute_morph.vs.spv", "shaders/desktop_3d_package_game_scene_compute_morph.cs.spv", "shaders/desktop_3d_package_game_scene_compute_morph_tangent_frame.vs.spv", "shaders/desktop_3d_package_game_scene_compute_morph_tangent_frame.cs.spv")) {
            if (@($desktop3dShaderTargets[0].vulkanShaderArtifactPaths) -notcontains $artifact) {
                Write-Error "Desktop runtime 3D package scaffold manifest Vulkan artifacts missing $artifact"
            }
        }
    }
    foreach ($authoringPath in @("source/assets/package.geassets", "source/scenes/packaged_scene.scene", "source/prefabs/static_prop.prefab", "source/textures/base_color.texture_source", "source/meshes/triangle.mesh_source", "source/morphs/packaged_mesh.morph_mesh_cpu_source", "source/animations/packaged_mesh_bob.animation_float_clip_source", "source/animations/packaged_mesh_morph_weights.animation_float_clip_source", "source/materials/lit.material", "shaders/runtime_scene.hlsl", "shaders/runtime_postprocess.hlsl", "shaders/runtime_shadow.hlsl", "shaders/runtime_ui_overlay.hlsl", "source/scene.scene", "source/mesh.gltf")) {
        if ($desktop3dManifest.runtimePackageFiles -contains $authoringPath) {
            Write-Error "Desktop runtime 3D package scaffold must not ship source authoring file in runtimePackageFiles: $authoringPath"
        }
    }
    if (@($desktop3dManifest.importerRequirements.sourceFormats) -notcontains "GameEngine.MorphMeshCpuSource.v1") {
        Write-Error "Desktop runtime 3D package scaffold manifest sourceFormats must include GameEngine.MorphMeshCpuSource.v1"
    }
    if (@($desktop3dManifest.importerRequirements.sourceFormats) -notcontains "GameEngine.AnimationQuaternionClipSource.v1") {
        Write-Error "Desktop runtime 3D package scaffold manifest sourceFormats must include GameEngine.AnimationQuaternionClipSource.v1"
    }
    $desktop3dResidencyKinds = @($desktop3dManifest.packageStreamingResidencyTargets[0].residentResourceKinds)
    if ($desktop3dResidencyKinds -notcontains "morph_mesh_cpu") {
        Write-Error "Desktop runtime 3D package scaffold residency target must include morph_mesh_cpu"
    }
    if ($desktop3dResidencyKinds -notcontains "animation_quaternion_clip") {
        Write-Error "Desktop runtime 3D package scaffold residency target must include animation_quaternion_clip"
    }
    if ($desktop3dResidencyKinds -notcontains "skinned_mesh") {
        Write-Error "Desktop runtime 3D package scaffold residency target must include skinned_mesh"
    }
    if ($desktop3dResidencyKinds -notcontains "physics_collision_scene") {
        Write-Error "Desktop runtime 3D package scaffold residency target must include physics_collision_scene"
    }
    Assert-RuntimeSceneValidationTarget `
        $desktop3dManifest `
        "Desktop runtime 3D package scaffold manifest" `
        "packaged-3d-scene" `
        "runtime/desktop_3d_package_game.geindex" `
        "desktop-3d-package-game/scenes/packaged-3d-scene"
    Assert-PackageStreamingResidencyTarget `
        $desktop3dManifest `
        "Desktop runtime 3D package scaffold manifest" `
        "packaged-3d-residency-budget" `
        "runtime/desktop_3d_package_game.geindex" `
        "packaged-3d-scene"
    Assert-PrefabScenePackageAuthoringTarget `
        $desktop3dManifest `
        "Desktop runtime 3D package scaffold manifest" `
        "packaged-3d-prefab-scene" `
        "runtime/desktop_3d_package_game.geindex" `
        "runtime/assets/3d/packaged_scene.scene" `
        "packaged-3d-scene"
    Assert-RegisteredSourceAssetCookTarget `
        $desktop3dManifest `
        "Desktop runtime 3D package scaffold manifest" `
        "packaged-3d-registered-source-cook" `
        "packaged-3d-prefab-scene" `
        "source/assets/package.geassets" `
        "runtime/desktop_3d_package_game.geindex" `
        @("desktop-3d-package-game/materials/lit") `
        "registered_source_registry_closure" `
        "registry_closure"
    foreach ($recipe in @("desktop-game-runtime", "desktop-runtime-release-target", "installed-d3d12-3d-package-smoke", "installed-d3d12-3d-directional-shadow-smoke", "installed-d3d12-3d-shadow-morph-composition-smoke", "installed-d3d12-3d-native-ui-overlay-smoke", "installed-d3d12-3d-visible-production-proof-smoke", "installed-d3d12-3d-native-ui-textured-sprite-atlas-smoke", "installed-d3d12-3d-native-ui-text-glyph-atlas-smoke", "installed-d3d12-3d-scene-collision-package-smoke", "desktop-runtime-release-target-vulkan-toolchain-gated", "desktop-runtime-release-target-vulkan-directional-shadow-toolchain-gated")) {
        if (@($desktop3dManifest.validationRecipes | ForEach-Object { $_.name }) -notcontains $recipe) {
            Write-Error "Desktop runtime 3D package scaffold manifest validationRecipes missing $recipe"
        }
    }
    foreach ($recipe in @($desktop3dManifest.validationRecipes)) {
        $isDesktop3dShadowRecipe = [string]$recipe.command -match "--require-directional-shadow"
        $isDesktop3dShadowMorphRecipe = [string]$recipe.command -match "--require-shadow-morph-composition"
        $isDesktop3dSceneCollisionPackageRecipe = [string]$recipe.command -match "--require-scene-collision-package"
        $isDesktop3dBroadPackageRecipe = $recipe.name -match "installed-d3d12|vulkan" -and -not $isDesktop3dShadowRecipe -and -not $isDesktop3dShadowMorphRecipe -and -not $isDesktop3dSceneCollisionPackageRecipe
        if ($isDesktop3dBroadPackageRecipe -and [string]$recipe.command -notmatch "--require-morph-package") {
            Write-Error "Desktop runtime 3D package scaffold package validation recipe missing --require-morph-package: $($recipe.name)"
        }
        if ($isDesktop3dBroadPackageRecipe -and [string]$recipe.command -notmatch "--require-quaternion-animation") {
            Write-Error "Desktop runtime 3D package scaffold package validation recipe missing --require-quaternion-animation: $($recipe.name)"
        }
        if ($recipe.name -match "installed-d3d12" -and $isDesktop3dBroadPackageRecipe -and [string]$recipe.command -notmatch "--require-compute-morph") {
            Write-Error "Desktop runtime 3D package scaffold D3D12 package validation recipe missing --require-compute-morph: $($recipe.name)"
        }
        if ($recipe.name -match "vulkan" -and $isDesktop3dBroadPackageRecipe -and [string]$recipe.command -notmatch "--require-compute-morph") {
            Write-Error "Desktop runtime 3D package scaffold Vulkan package validation recipe missing --require-compute-morph: $($recipe.name)"
        }
        if ($recipe.name -match "vulkan" -and $isDesktop3dBroadPackageRecipe -and [string]$recipe.command -notmatch "--require-compute-morph-normal-tangent") {
            Write-Error "Desktop runtime 3D package scaffold Vulkan package validation recipe missing --require-compute-morph-normal-tangent: $($recipe.name)"
        }
        if ($recipe.name -match "installed-d3d12" -and $isDesktop3dBroadPackageRecipe -and [string]$recipe.command -notmatch "--require-compute-morph-skin") {
            Write-Error "Desktop runtime 3D package scaffold D3D12 package validation recipe missing --require-compute-morph-skin: $($recipe.name)"
        }
        if ($recipe.name -match "installed-d3d12" -and $isDesktop3dBroadPackageRecipe -and [string]$recipe.command -notmatch "--require-compute-morph-async-telemetry") {
            Write-Error "Desktop runtime 3D package scaffold D3D12 package validation recipe missing --require-compute-morph-async-telemetry: $($recipe.name)"
        }
        if ($isDesktop3dBroadPackageRecipe -and [string]$recipe.command -notmatch "--require-package-streaming-safe-point") {
            Write-Error "Desktop runtime 3D package scaffold package validation recipe missing --require-package-streaming-safe-point: $($recipe.name)"
        }
        if ($recipe.name -match "installed-d3d12|vulkan" -and -not $isDesktop3dShadowMorphRecipe -and -not $isDesktop3dSceneCollisionPackageRecipe -and [string]$recipe.command -notmatch "--require-renderer-quality-gates") {
            Write-Error "Desktop runtime 3D package scaffold package validation recipe missing --require-renderer-quality-gates: $($recipe.name)"
        }
        if ($recipe.name -match "installed-d3d12|vulkan" -and -not $isDesktop3dShadowMorphRecipe -and -not $isDesktop3dSceneCollisionPackageRecipe -and [string]$recipe.command -notmatch "--require-postprocess-depth-input") {
            Write-Error "Desktop runtime 3D package scaffold package validation recipe missing --require-postprocess-depth-input: $($recipe.name)"
        }
        if ($isDesktop3dBroadPackageRecipe -and [string]$recipe.command -notmatch "--require-playable-3d-slice") {
            Write-Error "Desktop runtime 3D package scaffold package validation recipe missing --require-playable-3d-slice: $($recipe.name)"
        }
        if ($recipe.name -match "scene-collision-package" -and [string]$recipe.command -notmatch "--require-scene-collision-package") {
            Write-Error "Desktop runtime 3D package scaffold scene collision recipe missing --require-scene-collision-package: $($recipe.name)"
        }
        if ($isDesktop3dSceneCollisionPackageRecipe -and [string]$recipe.command -notmatch "--require-scene-package") {
            Write-Error "Desktop runtime 3D package scaffold scene collision recipe missing --require-scene-package: $($recipe.name)"
        }
        if ($recipe.name -match "directional-shadow" -and [string]$recipe.command -notmatch "--require-directional-shadow-filtering") {
            Write-Error "Desktop runtime 3D package scaffold directional shadow recipe missing --require-directional-shadow-filtering: $($recipe.name)"
        }
        if ($recipe.name -match "shadow-morph" -and [string]$recipe.command -notmatch "--require-shadow-morph-composition") {
            Write-Error "Desktop runtime 3D package scaffold shadow-morph recipe missing --require-shadow-morph-composition: $($recipe.name)"
        }
        if ($recipe.name -match "native-ui-overlay" -and [string]$recipe.command -notmatch "--require-native-ui-overlay") {
            Write-Error "Desktop runtime 3D package scaffold native UI overlay recipe missing --require-native-ui-overlay: $($recipe.name)"
        }
        if ($recipe.name -match "visible-production-proof" -and [string]$recipe.command -notmatch "--require-visible-3d-production-proof") {
            Write-Error "Desktop runtime 3D package scaffold visible production proof recipe missing --require-visible-3d-production-proof: $($recipe.name)"
        }
        if ($recipe.name -match "native-ui-textured-sprite-atlas" -and [string]$recipe.command -notmatch "--require-native-ui-textured-sprite-atlas") {
            Write-Error "Desktop runtime 3D package scaffold textured UI atlas recipe missing --require-native-ui-textured-sprite-atlas: $($recipe.name)"
        }
        if ($recipe.name -match "native-ui-text-glyph-atlas" -and [string]$recipe.command -notmatch "--require-native-ui-text-glyph-atlas") {
            Write-Error "Desktop runtime 3D package scaffold text glyph UI atlas recipe missing --require-native-ui-text-glyph-atlas: $($recipe.name)"
        }
    }

    $desktop3dCmake = Get-Content -LiteralPath (Join-Path $desktop3dScaffoldRoot "games/CMakeLists.txt") -Raw
    $desktop3dMain = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "main.cpp") -Raw
    $desktop3dIndex = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "runtime/desktop_3d_package_game.geindex") -Raw
    $desktop3dMorph = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "runtime/assets/3d/packaged_mesh.morph_mesh_cpu") -Raw
    $desktop3dAnimation = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "runtime/assets/3d/packaged_mesh_bob.animation_float_clip") -Raw
    $desktop3dMorphAnimation = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "runtime/assets/3d/packaged_mesh_morph_weights.animation_float_clip") -Raw
    $desktop3dQuaternionAnimation = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "runtime/assets/3d/packaged_pose.animation_quaternion_clip") -Raw
    $desktop3dSkinnedMesh = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "runtime/assets/3d/skinned_triangle.skinned_mesh") -Raw
    $desktop3dUiAtlas = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "runtime/assets/3d/hud.uiatlas") -Raw
    $desktop3dUiTextGlyphAtlas = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "runtime/assets/3d/hud_text.uiatlas") -Raw
    $desktop3dScene = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "runtime/assets/3d/packaged_scene.scene") -Raw
    $desktop3dCollision = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "runtime/assets/3d/collision.collision3d") -Raw
    $desktop3dSourceRegistry = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "source/assets/package.geassets") -Raw
    $desktop3dSourceScene = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "source/scenes/packaged_scene.scene") -Raw
    $desktop3dSourcePrefab = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "source/prefabs/static_prop.prefab") -Raw
    $desktop3dSourceTexture = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "source/textures/base_color.texture_source") -Raw
    $desktop3dSourceMesh = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "source/meshes/triangle.mesh_source") -Raw
    $desktop3dSourceMorph = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "source/morphs/packaged_mesh.morph_mesh_cpu_source") -Raw
    $desktop3dSourceAnimation = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "source/animations/packaged_mesh_bob.animation_float_clip_source") -Raw
    $desktop3dSourceMorphAnimation = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "source/animations/packaged_mesh_morph_weights.animation_float_clip_source") -Raw
    $desktop3dSourceQuaternionAnimation = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "source/animations/packaged_pose.animation_quaternion_clip_source") -Raw
    $desktop3dSceneHlsl = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "shaders/runtime_scene.hlsl") -Raw
    $desktop3dPostprocessHlsl = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "shaders/runtime_postprocess.hlsl") -Raw
    $desktop3dShadowHlsl = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "shaders/runtime_shadow.hlsl") -Raw
    $desktop3dUiOverlayHlsl = Get-Content -LiteralPath (Join-Path $desktop3dGameRoot "shaders/runtime_ui_overlay.hlsl") -Raw
    Assert-ContainsText $desktop3dCmake "MK_add_desktop_runtime_game(desktop_3d_package_game" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "PACKAGE_FILES_FROM_MANIFEST" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-scene-package" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-primary-camera-controller" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-transform-animation" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-morph-package" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-quaternion-animation" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-package-streaming-safe-point" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-renderer-quality-gates" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-postprocess-depth-input" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-playable-3d-slice" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-native-ui-overlay" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-visible-3d-production-proof" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-native-ui-textured-sprite-atlas" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-scene-collision-package" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "MK_animation" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "MK_ui" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "MK_ui_renderer" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "REQUIRES_D3D12_SHADERS" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "MK_configure_desktop_runtime_scene_shader_artifacts" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "UI_OVERLAY_SHADER_SOURCE" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "MORPH_VERTEX_ENTRY vs_morph" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "COMPUTE_MORPH_VERTEX_ENTRY vs_compute_morph" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "COMPUTE_MORPH_ENTRY cs_compute_morph_position" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "COMPUTE_MORPH_TANGENT_VERTEX_ENTRY vs_compute_morph_tangent_frame" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "COMPUTE_MORPH_TANGENT_ENTRY cs_compute_morph_tangent_frame" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "COMPUTE_MORPH_SKINNED_VERTEX_ENTRY vs_compute_morph_skinned" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "COMPUTE_MORPH_SKINNED_ENTRY cs_compute_morph_skinned_position" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "SHADOW_SHADER_SOURCE" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "SHADOW_RECEIVER_ENTRY ps_shadow_receiver" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "SHADOW_VERTEX_ENTRY vs_shadow" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "SHADOW_FRAGMENT_ENTRY ps_shadow" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-compute-morph-skin" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dCmake "--require-compute-morph-async-telemetry" "Desktop 3D scaffold CMake"
    Assert-ContainsText $desktop3dMain "primary_camera_controller_passed" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-primary-camera-controller" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-transform-animation" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-morph-package" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-quaternion-animation" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-package-streaming-safe-point" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "execute_selected_runtime_package_streaming_safe_point" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "residency_hint_failed" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "required_preload_assets = {packaged_scene_asset_id()}" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "resident_resource_kinds" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "max_resident_packages = 1" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "package_streaming_status=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "package_streaming_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "package_streaming_resident_bytes=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "package_streaming_committed_records=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "package_streaming_required_preload_assets=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "package_streaming_resident_resource_kinds=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "package_streaming_resident_packages=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "package_streaming_diagnostics=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-renderer-quality-gates" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-native-ui-overlay" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-visible-3d-production-proof" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-native-ui-textured-sprite-atlas" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-native-ui-text-glyph-atlas" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-scene-collision-package" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "mirakana/runtime/physics_collision_runtime.hpp" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "packaged_collision_scene_asset_id" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "runtime_physics_collision_scene_3d_payload" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "collision_package_status=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "collision_package_bodies=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "collision_package_trigger_overlaps=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "gameplay_systems_collision_package_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "mirakana/ui/ui.hpp" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "mirakana/ui_renderer/ui_renderer.hpp" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "build_ui_renderer_image_palette_from_runtime_ui_atlas" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "hud_boxes=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "hud_images=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "hud_text_glyphs=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "text_glyphs_resolved=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "ui_atlas_metadata_status=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "ui_atlas_metadata_glyphs=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "ui_texture_overlay_status=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "ui_overlay_requested=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "visible_3d_status=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "visible_3d_presented_frames=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "visible_3d_d3d12_selected=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "load_packaged_d3d12_native_ui_overlay_shaders" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "enable_native_ui_overlay" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "enable_native_ui_overlay_textures" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "evaluate_sdl_desktop_presentation_quality_gate" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_quality_status=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_quality_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_quality_diagnostics=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_quality_expected_framegraph_passes=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_quality_framegraph_passes_ok=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_quality_framegraph_execution_budget_ok=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_quality_scene_gpu_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_quality_postprocess_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-postprocess-depth-input" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "require_postprocess_depth_input" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "enable_postprocess_depth_input" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "postprocess_depth_input_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_quality_postprocess_depth_input_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-directional-shadow" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-directional-shadow-filtering" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-shadow-morph-composition" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "require_shadow_morph_composition" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "require_graphics_morph_scene" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "load_packaged_d3d12_shifted_shadow_receiver_scene_shaders" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "load_packaged_vulkan_shifted_shadow_receiver_scene_shaders" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "shifted_shadow_receiver_shader_diagnostic" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "skinned_scene_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "enable_directional_shadow_smoke" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "directional_shadow_status=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "directional_shadow_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "directional_shadow_filter_mode=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_quality_directional_shadow_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_quality_directional_shadow_filter_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-playable-3d-slice" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "playable_3d_status=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "playable_3d_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "playable_3d_scene_mesh_plan_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "playable_3d_renderer_quality_ready=" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "packaged_animation_bindings" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "packaged_mesh_asset_id" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "packaged_skinned_mesh_asset_id" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "packaged_morph_mesh_asset_id" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "packaged_morph_animation_asset_id" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "runtime_morph_mesh_cpu_payload" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "runtime_animation_float_clip_payload" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "runtime_animation_quaternion_clip_payload" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "make_animation_joint_tracks_3d_from_f32_bytes" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "sample_animation_local_pose_3d" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "sample_and_apply_runtime_scene_render_animation_pose_3d" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "sample_and_apply_runtime_scene_render_animation_float_clip" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "sample_runtime_morph_mesh_cpu_animation_float_clip" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "transform_animation_passed" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "transform_animation_ticks" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "transform_animation_samples" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "transform_animation_applied" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "final_mesh_x" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "morph_package_passed" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dPostprocessHlsl "scene_depth_texture" "Desktop 3D scaffold postprocess shader"
    Assert-ContainsText $desktop3dPostprocessHlsl "scene_depth_sampler" "Desktop 3D scaffold postprocess shader"
    Assert-ContainsText $desktop3dSceneHlsl "ps_shadow_receiver" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "ShadowReceiverConstants" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "MK_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "shadow_depth_texture : register(t0, space2)" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "ShadowReceiverConstants : register(b2, space2)" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dShadowHlsl "vs_shadow" "Desktop 3D scaffold shadow shader"
    Assert-ContainsText $desktop3dShadowHlsl "ps_shadow" "Desktop 3D scaffold shadow shader"
    Assert-DoesNotContainText $desktop3dShadowHlsl "BLENDINDICES" "Desktop 3D scaffold shadow shader"
    Assert-DoesNotContainText $desktop3dShadowHlsl "BLENDWEIGHT" "Desktop 3D scaffold shadow shader"
    Assert-ContainsText $desktop3dMain "morph_package_samples" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "morph_package_weights" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "morph_package_vertices" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "morph_first_position_x" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "quaternion_animation_passed" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "quaternion_animation_ticks" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "quaternion_animation_tracks" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "quaternion_animation_failures" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "quaternion_animation_scene_applied" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "quaternion_animation_scene_rotation_z" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "load_packaged_d3d12_scene_morph_shaders" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "load_packaged_d3d12_scene_compute_morph_shaders" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "load_packaged_d3d12_scene_compute_morph_skinned_shaders" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "load_packaged_vulkan_scene_morph_shaders" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "load_packaged_vulkan_scene_compute_morph_shaders" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneMorphVertexShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneComputeMorphVertexShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneComputeMorphShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneComputeMorphSkinnedVertexShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneComputeMorphSkinnedShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneVulkanMorphVertexShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneVulkanComputeMorphVertexShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneVulkanComputeMorphShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneVulkanComputeMorphTangentFrameVertexShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneVulkanComputeMorphTangentFrameShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneVulkanComputeMorphSkinnedVertexShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneVulkanComputeMorphSkinnedShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "vulkan_compute_morph_shader_diagnostic" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "vulkan_compute_morph_tangent_frame_shader_diagnostic" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "vulkan_compute_morph_skinned_shader_diagnostic" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "Vulkan compute morph package smoke does not support async telemetry requirements" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "compute_morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "compute_morph_shader = mirakana::SdlDesktopPresentationShaderBytecode" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "compute_morph_skinned_shader = mirakana::SdlDesktopPresentationShaderBytecode" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "d3d12_scene_renderer->morph_mesh_assets = {packaged_morph_mesh_asset_id()}" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id()" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_skinned_mesh_asset_id()" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "compute_morph_mesh_bindings" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "compute_morph_skinned_mesh_bindings" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-compute-morph" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-compute-morph-normal-tangent" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-compute-morph-skin" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "--require-compute-morph-async-telemetry" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "require_compute_morph_async_telemetry" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "require_compute_morph_skin" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "activate_compute_morph_skinned_scene" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "runtime_compute_morph_skinned_scene_vertex_buffers" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "runtime_compute_morph_skinned_scene_vertex_attributes" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "require_compute_morph_normal_tangent" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "enable_compute_morph_tangent_frame_output" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "load_packaged_d3d12_scene_compute_morph_tangent_frame_shaders" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "load_packaged_vulkan_scene_compute_morph_tangent_frame_shaders" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneComputeMorphTangentFrameVertexShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneComputeMorphTangentFrameShaderPath" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_morph_mesh_bindings" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_morph_mesh_uploads" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_morph_mesh_resolved" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_uploaded_morph_bytes" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_gpu_morph_draws" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "renderer_morph_descriptor_binds" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_mesh_bindings" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_dispatches" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_queue_waits" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_async_compute_queue_submits" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_async_graphics_queue_waits" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_async_graphics_queue_submits" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_async_last_compute_fence" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_async_last_graphics_wait_fence" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_async_last_graphics_submit_fence" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_mesh_resolved" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_draws" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_tangent_frame_output" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_skinned_mesh_bindings" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_skinned_dispatches" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_skinned_queue_waits" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_skinned_mesh_resolved" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_skinned_draws" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_compute_morph_output_position_bytes" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_stats.morph_mesh_uploads < 1" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_stats.morph_mesh_bindings_resolved" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_stats.uploaded_morph_bytes == 0" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_stats.compute_morph_queue_waits < 1" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_stats.compute_morph_async_compute_queue_submits < 1" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_stats.compute_morph_async_last_graphics_submitted_fence_value == 0" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_stats.compute_morph_skinned_mesh_bindings < 1" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_stats.compute_morph_skinned_mesh_bindings_resolved" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_gpu_stats.compute_morph_output_position_bytes == 0" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "report.renderer_stats.gpu_morph_draws" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "report.renderer_stats.morph_descriptor_binds" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "report.renderer_stats.gpu_skinning_draws" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "report.renderer_stats.skinned_palette_descriptor_binds" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "camera_controller_ticks" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "plan_scene_mesh_draws" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_mesh_plan_meshes" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_mesh_plan_draws" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_mesh_plan_unique_meshes" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_mesh_plan_unique_materials" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "scene_mesh_plan_diagnostics" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dScene "node.1.mesh_renderer.visible=true" "Desktop 3D scaffold scene"
    Assert-ContainsText $desktop3dScene "node.2.light.type=directional" "Desktop 3D scaffold scene"
    Assert-ContainsText $desktop3dScene "node.2.light.casts_shadows=true" "Desktop 3D scaffold scene"
    Assert-ContainsText $desktop3dScene "node.3.camera.projection=perspective" "Desktop 3D scaffold scene"
    Assert-ContainsText $desktop3dScene "node.3.camera.primary=true" "Desktop 3D scaffold scene"
    Assert-ContainsText $desktop3dIndex "dependency.0.kind=material_texture" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "dependency.1.kind=scene_material" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "dependency.2.kind=scene_mesh" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "entry.3.kind=morph_mesh_cpu" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "entry.3.path=runtime/assets/3d/packaged_mesh.morph_mesh_cpu" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "entry.5.kind=animation_float_clip" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "entry.5.path=runtime/assets/3d/packaged_mesh_bob.animation_float_clip" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "entry.6.kind=animation_float_clip" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "entry.6.path=runtime/assets/3d/packaged_mesh_morph_weights.animation_float_clip" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "entry.7.kind=animation_quaternion_clip" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "entry.7.path=runtime/assets/3d/packaged_pose.animation_quaternion_clip" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "kind=skinned_mesh" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "runtime/assets/3d/skinned_triangle.skinned_mesh" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "kind=physics_collision_scene" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dIndex "runtime/assets/3d/collision.collision3d" "Desktop 3D scaffold geindex"
    Assert-ContainsText $desktop3dMorph "format=GameEngine.CookedMorphMeshCpu.v1" "Desktop 3D scaffold morph mesh CPU"
    Assert-ContainsText $desktop3dMorph "morph.vertex_count=3" "Desktop 3D scaffold morph mesh CPU"
    Assert-ContainsText $desktop3dMorph "morph.target.0.position_deltas_hex=0000803f" "Desktop 3D scaffold morph mesh CPU"
    Assert-ContainsText $desktop3dMorph "morph.bind_normals_hex=" "Desktop 3D scaffold morph mesh CPU"
    Assert-ContainsText $desktop3dMorph "morph.bind_tangents_hex=" "Desktop 3D scaffold morph mesh CPU"
    Assert-ContainsText $desktop3dMorph "morph.target.0.normal_deltas_hex=" "Desktop 3D scaffold morph mesh CPU"
    Assert-ContainsText $desktop3dMorph "morph.target.0.tangent_deltas_hex=" "Desktop 3D scaffold morph mesh CPU"
    Assert-ContainsText $desktop3dAnimation "format=GameEngine.CookedAnimationFloatClip.v1" "Desktop 3D scaffold animation clip"
    Assert-ContainsText $desktop3dAnimation "clip.track.0.target=gltf/node/0/translation/x" "Desktop 3D scaffold animation clip"
    Assert-ContainsText $desktop3dAnimation "clip.track.0.values_hex=000000000000003f" "Desktop 3D scaffold animation clip"
    Assert-ContainsText $desktop3dMorphAnimation "format=GameEngine.CookedAnimationFloatClip.v1" "Desktop 3D scaffold morph animation clip"
    Assert-ContainsText $desktop3dMorphAnimation "clip.track.0.target=gltf/node/0/weights/0" "Desktop 3D scaffold morph animation clip"
    Assert-ContainsText $desktop3dMorphAnimation "clip.track.0.values_hex=000000000000003f" "Desktop 3D scaffold morph animation clip"
    Assert-ContainsText $desktop3dQuaternionAnimation "format=GameEngine.CookedAnimationQuaternionClip.v1" "Desktop 3D scaffold quaternion animation clip"
    Assert-ContainsText $desktop3dQuaternionAnimation "asset.kind=animation_quaternion_clip" "Desktop 3D scaffold quaternion animation clip"
    Assert-ContainsText $desktop3dQuaternionAnimation "clip.track.0.rotation_keyframe_count=2" "Desktop 3D scaffold quaternion animation clip"
    Assert-ContainsText $desktop3dQuaternionAnimation "clip.track.0.rotations_xyzw_hex=0000000000000000000000000000803f00000000000000000000803f00000000" "Desktop 3D scaffold quaternion animation clip"
    Assert-ContainsText $desktop3dSkinnedMesh "format=GameEngine.CookedSkinnedMesh.v1" "Desktop 3D scaffold skinned mesh"
    Assert-ContainsText $desktop3dSkinnedMesh "asset.kind=skinned_mesh" "Desktop 3D scaffold skinned mesh"
    Assert-ContainsText $desktop3dSkinnedMesh "skinned_mesh.vertex_count=3" "Desktop 3D scaffold skinned mesh"
    Assert-ContainsText $desktop3dSkinnedMesh "skinned_mesh.joint_count=1" "Desktop 3D scaffold skinned mesh"
    Assert-ContainsText $desktop3dSkinnedMesh "skinned_mesh.vertex_data_hex=" "Desktop 3D scaffold skinned mesh"
    Assert-ContainsText $desktop3dSkinnedMesh "skinned_mesh.joint_palette_hex=" "Desktop 3D scaffold skinned mesh"
    Assert-ContainsText $desktop3dSourceRegistry "format=GameEngine.SourceAssetRegistry.v1" "Desktop 3D scaffold source registry"
    Assert-ContainsText $desktop3dSourceRegistry "asset.0.source_format=GameEngine.TextureSource.v1" "Desktop 3D scaffold source registry"
    Assert-ContainsText $desktop3dSourceRegistry "asset.1.source_format=GameEngine.MeshSource.v2" "Desktop 3D scaffold source registry"
    Assert-ContainsText $desktop3dSourceRegistry "asset.2.source_format=GameEngine.MorphMeshCpuSource.v1" "Desktop 3D scaffold source registry"
    Assert-ContainsText $desktop3dSourceRegistry "asset.3.source_format=GameEngine.Material.v1" "Desktop 3D scaffold source registry"
    Assert-ContainsText $desktop3dSourceRegistry "asset.4.source_format=GameEngine.AnimationFloatClipSource.v1" "Desktop 3D scaffold source registry"
    Assert-ContainsText $desktop3dSourceRegistry "asset.5.source_format=GameEngine.AnimationFloatClipSource.v1" "Desktop 3D scaffold source registry"
    Assert-ContainsText $desktop3dSourceRegistry "asset.6.source_format=GameEngine.AnimationQuaternionClipSource.v1" "Desktop 3D scaffold source registry"
    Assert-ContainsText $desktop3dSourceScene "format=GameEngine.Scene.v2" "Desktop 3D scaffold source scene"
    Assert-ContainsText $desktop3dSourceScene "node.2.id=node/primary-camera" "Desktop 3D scaffold source scene"
    Assert-ContainsText $desktop3dSourcePrefab "format=GameEngine.Prefab.v2" "Desktop 3D scaffold source prefab"
    Assert-ContainsText $desktop3dSourceTexture "format=GameEngine.TextureSource.v1" "Desktop 3D scaffold source texture"
    Assert-ContainsText $desktop3dSourceMesh "format=GameEngine.MeshSource.v2" "Desktop 3D scaffold source mesh"
    Assert-ContainsText $desktop3dSourceMorph "format=GameEngine.MorphMeshCpuSource.v1" "Desktop 3D scaffold source morph"
    Assert-ContainsText $desktop3dSourceMorph "morph.bind_normals_hex=" "Desktop 3D scaffold source morph"
    Assert-ContainsText $desktop3dSourceMorph "morph.bind_tangents_hex=" "Desktop 3D scaffold source morph"
    Assert-ContainsText $desktop3dSourceMorph "morph.target.0.normal_deltas_hex=" "Desktop 3D scaffold source morph"
    Assert-ContainsText $desktop3dSourceMorph "morph.target.0.tangent_deltas_hex=" "Desktop 3D scaffold source morph"
    Assert-ContainsText $desktop3dSourceAnimation "format=GameEngine.AnimationFloatClipSource.v1" "Desktop 3D scaffold source animation"
    Assert-ContainsText $desktop3dSourceMorphAnimation "clip.track.0.target=gltf/node/0/weights/0" "Desktop 3D scaffold source morph animation"
    Assert-ContainsText $desktop3dSourceQuaternionAnimation "format=GameEngine.AnimationQuaternionClipSource.v1" "Desktop 3D scaffold source quaternion animation"
    Assert-ContainsText $desktop3dSourceQuaternionAnimation "clip.track.0.target=desktop-3d-package-game/pose/root" "Desktop 3D scaffold source quaternion animation"
    Assert-ContainsText $desktop3dMain "kRuntimeSceneTangentSpaceStrideBytes{48}" "Desktop 3D scaffold main.cpp"
    Assert-ContainsText $desktop3dMain "mirakana::rhi::VertexSemantic::tangent" "Desktop 3D scaffold main.cpp"
    Assert-DoesNotContainText $desktop3dSceneHlsl "StructuredBuffer<float3> morph_position_deltas" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "RWByteAddressBuffer morph_position_deltas : register(u0, space1)" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "RWByteAddressBuffer morph_normal_deltas : register(u2, space1)" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "RWByteAddressBuffer morph_tangent_deltas : register(u3, space1)" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "float4 tangent : TANGENT" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "cbuffer MorphWeights" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "VsOut vs_morph" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "VsOut vs_compute_morph" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "VsOut vs_compute_morph_tangent_frame" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "VsOut vs_compute_morph_skinned" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "RWByteAddressBuffer compute_base_vertices : register(u0, space0)" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "RWByteAddressBuffer compute_output_positions : register(u3, space0)" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "RWByteAddressBuffer compute_normal_deltas : register(u4, space0)" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "RWByteAddressBuffer compute_tangent_deltas : register(u5, space0)" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "RWByteAddressBuffer compute_output_normals : register(u6, space0)" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "RWByteAddressBuffer compute_output_tangents : register(u7, space0)" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "cs_compute_morph_position" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "cs_compute_morph_tangent_frame" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "cs_compute_morph_skinned_position" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "JointPalette : register(b0, space1)" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "BLENDINDICES" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dSceneHlsl "BLENDWEIGHT" "Desktop 3D scaffold scene shader"
    Assert-ContainsText $desktop3dUiOverlayHlsl "vs_native_ui_overlay" "Desktop 3D scaffold UI overlay shader"
    Assert-ContainsText $desktop3dUiOverlayHlsl "ps_native_ui_overlay" "Desktop 3D scaffold UI overlay shader"
    foreach ($needle in @("format=GameEngine.UiAtlas.v1", "asset.kind=ui_atlas", "source.decoding=unsupported", "atlas.packing=unsupported", "page.0.asset_uri=runtime/assets/3d/base_color.texture.geasset", "image.0.resource_id=hud.texture_atlas_proof")) {
        Assert-ContainsText $desktop3dUiAtlas $needle "Desktop 3D scaffold hud.uiatlas"
    }
    foreach ($needle in @("format=GameEngine.UiAtlas.v1", "asset.kind=ui_atlas", "source.decoding=unsupported", "atlas.packing=unsupported", "page.0.asset_uri=runtime/assets/3d/base_color.texture.geasset", "glyph.0.font_family=engine-default", "glyph.0.glyph=65")) {
        Assert-ContainsText $desktop3dUiTextGlyphAtlas $needle "Desktop 3D scaffold hud_text.uiatlas"
    }
    foreach ($needle in @("format=GameEngine.PhysicsCollisionScene3D.v1", "asset.kind=physics_collision_scene", "backend.native=unsupported", "body.0.name=floor", "body.0.compound=level_static", "body.1.name=collision_probe", "body.1.compound=level_static", "body.2.name=pickup_trigger", "body.2.trigger=true", "body.2.compound=interaction_triggers")) {
        Assert-ContainsText $desktop3dCollision $needle "Desktop 3D scaffold collision.collision3d"
    }
    Assert-ContainsText $desktop3dIndex "kind=ui_atlas" "Desktop 3D scaffold package index"
    Assert-ContainsText $desktop3dIndex "kind=ui_atlas_texture" "Desktop 3D scaffold package index"
    if ($desktop3dIndex.Contains("kind=source_file")) {
        Write-Error "Desktop runtime 3D package scaffold geindex must not use source_file dependency edges"
    }
} finally {
    Remove-ScaffoldCheckRoot $desktop3dScaffoldRoot
}

$null = Resolve-RequiredAgentPath "games/sample_generated_desktop_runtime_3d_package"
$committedDesktop3dManifestPath = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$committedDesktop3dManifestFullPath = Resolve-RequiredAgentPath $committedDesktop3dManifestPath
$committedDesktop3dMainPath = Resolve-RequiredAgentPath "games/sample_generated_desktop_runtime_3d_package/main.cpp"
$committedDesktop3dReadmePath = Resolve-RequiredAgentPath "games/sample_generated_desktop_runtime_3d_package/README.md"
$committedDesktop3dIndexPath = Resolve-RequiredAgentPath "games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex"
$committedDesktop3dScenePath = Resolve-RequiredAgentPath "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/packaged_scene.scene"
$committedDesktop3dSceneShaderPath = Resolve-RequiredAgentPath "games/sample_generated_desktop_runtime_3d_package/shaders/runtime_scene.hlsl"
$committedDesktop3dPostprocessShaderPath = Resolve-RequiredAgentPath "games/sample_generated_desktop_runtime_3d_package/shaders/runtime_postprocess.hlsl"
$committedDesktop3dShadowShaderPath = Resolve-RequiredAgentPath "games/sample_generated_desktop_runtime_3d_package/shaders/runtime_shadow.hlsl"
$committedDesktop3dUiOverlayShaderPath = Resolve-RequiredAgentPath "games/sample_generated_desktop_runtime_3d_package/shaders/runtime_ui_overlay.hlsl"
$committedDesktop3dUiAtlasPath = Resolve-RequiredAgentPath "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud.uiatlas"
$committedDesktop3dUiTextGlyphAtlasPath = Resolve-RequiredAgentPath "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas"
$committedDesktop3dCollisionPath = Resolve-RequiredAgentPath "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/collision.collision3d"
$committedDesktop3dManifest = Get-Content -LiteralPath $committedDesktop3dManifestFullPath -Raw | ConvertFrom-Json
$committedDesktop3dManifestText = Get-Content -LiteralPath $committedDesktop3dManifestFullPath -Raw
$committedDesktop3dMainText = Get-Content -LiteralPath $committedDesktop3dMainPath -Raw
$committedDesktop3dReadmeText = Get-Content -LiteralPath $committedDesktop3dReadmePath -Raw
$committedDesktop3dIndexText = Get-Content -LiteralPath $committedDesktop3dIndexPath -Raw
$committedDesktop3dSceneText = Get-Content -LiteralPath $committedDesktop3dScenePath -Raw
$committedDesktop3dSceneShaderText = Get-Content -LiteralPath $committedDesktop3dSceneShaderPath -Raw
$committedDesktop3dPostprocessShaderText = Get-Content -LiteralPath $committedDesktop3dPostprocessShaderPath -Raw
$committedDesktop3dShadowShaderText = Get-Content -LiteralPath $committedDesktop3dShadowShaderPath -Raw
$committedDesktop3dUiOverlayShaderText = Get-Content -LiteralPath $committedDesktop3dUiOverlayShaderPath -Raw
$committedDesktop3dUiAtlasText = Get-Content -LiteralPath $committedDesktop3dUiAtlasPath -Raw
$committedDesktop3dUiTextGlyphAtlasText = Get-Content -LiteralPath $committedDesktop3dUiTextGlyphAtlasPath -Raw
$committedDesktop3dCollisionText = Get-Content -LiteralPath $committedDesktop3dCollisionPath -Raw
$committedDesktop3dCmakeText = Get-AgentSurfaceText "games/CMakeLists.txt"
$committedDesktop3dCmakeBlock = [regex]::Match(
    $committedDesktop3dCmakeText,
    "MK_add_desktop_runtime_game\(sample_generated_desktop_runtime_3d_package[\s\S]*?\n\s*\)"
).Value
if ([string]::IsNullOrWhiteSpace($committedDesktop3dCmakeBlock)) {
    Write-Error "games/CMakeLists.txt committed generated 3D package sample block was not found"
}

if ($committedDesktop3dManifest.gameplayContract.productionRecipe -ne "3d-playable-desktop-package") {
    Write-Error "$committedDesktop3dManifestPath must select 3d-playable-desktop-package"
}
foreach ($relativePath in @(
    "runtime/sample_generated_desktop_runtime_3d_package.config",
    "runtime/sample_generated_desktop_runtime_3d_package.geindex",
    "runtime/assets/3d/base_color.texture.geasset",
    "runtime/assets/3d/triangle.mesh",
    "runtime/assets/3d/packaged_mesh.morph_mesh_cpu",
    "runtime/assets/3d/lit.material",
    "runtime/assets/3d/packaged_mesh_bob.animation_float_clip",
    "runtime/assets/3d/packaged_mesh_morph_weights.animation_float_clip",
    "runtime/assets/3d/packaged_pose.animation_quaternion_clip",
    "runtime/assets/3d/skinned_triangle.skinned_mesh",
    "runtime/assets/3d/hud.uiatlas",
    "runtime/assets/3d/hud_text.uiatlas",
    "runtime/assets/3d/packaged_scene.scene",
    "runtime/assets/3d/collision.collision3d"
)) {
    if (@($committedDesktop3dManifest.runtimePackageFiles) -notcontains $relativePath) {
        Write-Error "$committedDesktop3dManifestPath runtimePackageFiles missing $relativePath"
    }
    Resolve-RequiredAgentPath (Join-Path "games/sample_generated_desktop_runtime_3d_package" $relativePath) | Out-Null
}
foreach ($authoringPath in @(
    "source/assets/package.geassets",
    "source/scenes/packaged_scene.scene",
    "source/prefabs/static_prop.prefab",
    "source/textures/base_color.texture_source",
    "source/meshes/triangle.mesh_source",
    "source/morphs/packaged_mesh.morph_mesh_cpu_source",
    "source/animations/packaged_mesh_bob.animation_float_clip_source",
    "source/animations/packaged_mesh_morph_weights.animation_float_clip_source",
    "source/animations/packaged_pose.animation_quaternion_clip_source",
    "source/materials/lit.material",
    "shaders/runtime_scene.hlsl",
    "shaders/runtime_postprocess.hlsl",
    "shaders/runtime_shadow.hlsl",
    "shaders/runtime_ui_overlay.hlsl"
)) {
    Resolve-RequiredAgentPath (Join-Path "games/sample_generated_desktop_runtime_3d_package" $authoringPath) | Out-Null
    if (@($committedDesktop3dManifest.runtimePackageFiles) -contains $authoringPath) {
        Write-Error "$committedDesktop3dManifestPath must not ship source authoring file in runtimePackageFiles: $authoringPath"
    }
}
Assert-RuntimeSceneValidationTarget `
    $committedDesktop3dManifest `
    $committedDesktop3dManifestPath `
    "packaged-3d-scene" `
    "runtime/sample_generated_desktop_runtime_3d_package.geindex" `
    "sample-generated-desktop-runtime-3d-package/scenes/packaged-3d-scene"
Assert-PackageStreamingResidencyTarget `
    $committedDesktop3dManifest `
    $committedDesktop3dManifestPath `
    "packaged-3d-residency-budget" `
    "runtime/sample_generated_desktop_runtime_3d_package.geindex" `
    "packaged-3d-scene"
Assert-PrefabScenePackageAuthoringTarget `
    $committedDesktop3dManifest `
    $committedDesktop3dManifestPath `
    "packaged-3d-prefab-scene" `
    "runtime/sample_generated_desktop_runtime_3d_package.geindex" `
    "runtime/assets/3d/packaged_scene.scene" `
    "packaged-3d-scene"
Assert-RegisteredSourceAssetCookTarget `
    $committedDesktop3dManifest `
    $committedDesktop3dManifestPath `
    "packaged-3d-registered-source-cook" `
    "packaged-3d-prefab-scene" `
    "source/assets/package.geassets" `
    "runtime/sample_generated_desktop_runtime_3d_package.geindex" `
    @("sample-generated-desktop-runtime-3d-package/materials/lit") `
    "registered_source_registry_closure" `
    "registry_closure"
foreach ($recipe in @("desktop-game-runtime", "desktop-runtime-release-target", "installed-d3d12-3d-package-smoke", "installed-d3d12-3d-directional-shadow-smoke", "installed-d3d12-3d-shadow-morph-composition-smoke", "installed-d3d12-3d-native-ui-overlay-smoke", "installed-d3d12-3d-visible-production-proof-smoke", "installed-d3d12-3d-native-ui-textured-sprite-atlas-smoke", "installed-d3d12-3d-native-ui-text-glyph-atlas-smoke", "installed-d3d12-3d-scene-collision-package-smoke", "desktop-runtime-release-target-vulkan-toolchain-gated", "desktop-runtime-release-target-vulkan-directional-shadow-toolchain-gated")) {
    if (@($committedDesktop3dManifest.validationRecipes | ForEach-Object { $_.name }) -notcontains $recipe) {
        Write-Error "$committedDesktop3dManifestPath validationRecipes missing $recipe"
    }
}
foreach ($recipe in @($committedDesktop3dManifest.validationRecipes)) {
    $isCommittedDesktop3dShadowRecipe = [string]$recipe.command -match "--require-directional-shadow"
    $isCommittedDesktop3dShadowMorphRecipe = [string]$recipe.command -match "--require-shadow-morph-composition"
    $isCommittedDesktop3dSceneCollisionPackageRecipe = [string]$recipe.command -match "--require-scene-collision-package"
    $isCommittedDesktop3dBroadPackageRecipe = $recipe.name -match "installed-d3d12|vulkan" -and -not $isCommittedDesktop3dShadowRecipe -and -not $isCommittedDesktop3dShadowMorphRecipe -and -not $isCommittedDesktop3dSceneCollisionPackageRecipe
    if ($isCommittedDesktop3dBroadPackageRecipe -and [string]$recipe.command -notmatch "--require-package-streaming-safe-point") {
        Write-Error "$committedDesktop3dManifestPath package validation recipe missing --require-package-streaming-safe-point: $($recipe.name)"
    }
    if ($recipe.name -match "installed-d3d12|vulkan" -and -not $isCommittedDesktop3dShadowMorphRecipe -and -not $isCommittedDesktop3dSceneCollisionPackageRecipe -and [string]$recipe.command -notmatch "--require-renderer-quality-gates") {
        Write-Error "$committedDesktop3dManifestPath package validation recipe missing --require-renderer-quality-gates: $($recipe.name)"
    }
    if ($recipe.name -match "installed-d3d12|vulkan" -and -not $isCommittedDesktop3dShadowMorphRecipe -and -not $isCommittedDesktop3dSceneCollisionPackageRecipe -and [string]$recipe.command -notmatch "--require-postprocess-depth-input") {
        Write-Error "$committedDesktop3dManifestPath package validation recipe missing --require-postprocess-depth-input: $($recipe.name)"
    }
    if ($isCommittedDesktop3dBroadPackageRecipe -and [string]$recipe.command -notmatch "--require-playable-3d-slice") {
        Write-Error "$committedDesktop3dManifestPath package validation recipe missing --require-playable-3d-slice: $($recipe.name)"
    }
    if ($isCommittedDesktop3dBroadPackageRecipe -and [string]$recipe.command -notmatch "--require-gameplay-systems") {
        Write-Error "$committedDesktop3dManifestPath package validation recipe missing --require-gameplay-systems: $($recipe.name)"
    }
    if ($recipe.name -match "scene-collision-package" -and [string]$recipe.command -notmatch "--require-scene-collision-package") {
        Write-Error "$committedDesktop3dManifestPath scene collision recipe missing --require-scene-collision-package: $($recipe.name)"
    }
    if ($isCommittedDesktop3dSceneCollisionPackageRecipe -and [string]$recipe.command -notmatch "--require-scene-package") {
        Write-Error "$committedDesktop3dManifestPath scene collision recipe missing --require-scene-package: $($recipe.name)"
    }
    if ($recipe.name -match "directional-shadow" -and [string]$recipe.command -notmatch "--require-directional-shadow-filtering") {
        Write-Error "$committedDesktop3dManifestPath directional shadow recipe missing --require-directional-shadow-filtering: $($recipe.name)"
    }
    if ($recipe.name -match "shadow-morph" -and [string]$recipe.command -notmatch "--require-shadow-morph-composition") {
        Write-Error "$committedDesktop3dManifestPath shadow-morph recipe missing --require-shadow-morph-composition: $($recipe.name)"
    }
    if ($recipe.name -match "native-ui-overlay" -and [string]$recipe.command -notmatch "--require-native-ui-overlay") {
        Write-Error "$committedDesktop3dManifestPath native UI overlay recipe missing --require-native-ui-overlay: $($recipe.name)"
    }
    if ($recipe.name -match "visible-production-proof" -and [string]$recipe.command -notmatch "--require-visible-3d-production-proof") {
        Write-Error "$committedDesktop3dManifestPath visible production proof recipe missing --require-visible-3d-production-proof: $($recipe.name)"
    }
    if ($recipe.name -match "native-ui-textured-sprite-atlas" -and [string]$recipe.command -notmatch "--require-native-ui-textured-sprite-atlas") {
        Write-Error "$committedDesktop3dManifestPath textured UI atlas recipe missing --require-native-ui-textured-sprite-atlas: $($recipe.name)"
    }
    if ($recipe.name -match "native-ui-text-glyph-atlas" -and [string]$recipe.command -notmatch "--require-native-ui-text-glyph-atlas") {
        Write-Error "$committedDesktop3dManifestPath text glyph UI atlas recipe missing --require-native-ui-text-glyph-atlas: $($recipe.name)"
    }
}
foreach ($needle in @(
    "camera/controller movement",
    "runtime source asset parsing remains unsupported",
    "broad dependency cooking remains unsupported",
    "selected host-gated package streaming safe-point smoke",
    "selected generated 3D renderer quality smoke",
    "selected generated 3D postprocess depth-input smoke",
    "selected generated 3D playable package smoke",
    "selected generated 3D gameplay systems package smoke",
    "selected D3D12 visible generated 3D production-style package proof",
    "selected generated 3D directional shadow package smoke",
    "selected D3D12 generated 3D graphics morph + directional shadow receiver package smoke",
    "selected D3D12 generated 3D native UI overlay HUD box package smoke",
    "selected D3D12 generated 3D cooked UI atlas image sprite package smoke",
    "selected D3D12 generated 3D cooked UI atlas text glyph package smoke",
    "selected generated 3D scene collision package smoke",
    "compute morph + shadow composition",
    "production text shaping, font rasterization, glyph atlas generation, runtime source image decoding",
    "broad async/background package streaming remains unsupported",
    "material graph, and live shader generation remain unsupported",
    "public native or RHI handle access remains unsupported",
    "Metal readiness remains unsupported"
)) {
    Assert-ContainsText $committedDesktop3dManifestText $needle $committedDesktop3dManifestPath
}
foreach ($needle in @(
    "MK_add_desktop_runtime_game(sample_generated_desktop_runtime_3d_package",
    "--require-primary-camera-controller",
    "--require-transform-animation",
    "--require-morph-package",
    "--require-compute-morph",
    "--require-compute-morph-skin",
    "--require-compute-morph-async-telemetry",
    "--require-quaternion-animation",
    "--require-package-streaming-safe-point",
    "--require-renderer-quality-gates",
    "--require-postprocess-depth-input",
    "--require-playable-3d-slice",
    "--require-gameplay-systems",
    "--require-native-ui-overlay",
    "--require-visible-3d-production-proof",
    "--require-native-ui-textured-sprite-atlas",
    "--require-scene-collision-package",
    "REQUIRES_D3D12_SHADERS",
    "PACKAGE_FILES_FROM_MANIFEST"
)) {
    Assert-ContainsText $committedDesktop3dCmakeBlock $needle "games/CMakeLists.txt committed generated 3D package sample"
}
foreach ($needle in @(
    "SHADOW_SHADER_SOURCE",
    "SHADOW_RECEIVER_ENTRY ps_shadow_receiver",
    "SHADOW_VERTEX_ENTRY vs_shadow",
    "SHADOW_FRAGMENT_ENTRY ps_shadow",
    "UI_OVERLAY_SHADER_SOURCE"
)) {
    Assert-ContainsText $committedDesktop3dCmakeText $needle "games/CMakeLists.txt committed generated 3D package shader helper"
}
foreach ($needle in @(
    "primary_camera_controller_passed",
    "camera_controller_ticks",
    "plan_scene_mesh_draws",
    "scene_mesh_plan_draws",
    "runtime_morph_mesh_cpu_payload",
    "runtime_animation_float_clip_payload",
    "runtime_animation_quaternion_clip_payload",
    "sample_and_apply_runtime_scene_render_animation_pose_3d",
    "scene_gpu_compute_morph_async_compute_queue_submits",
    "execute_selected_runtime_package_streaming_safe_point",
    "residency_hint_failed",
    "required_preload_assets = {packaged_scene_asset_id()}",
    "resident_resource_kinds",
    "max_resident_packages = 1",
    "package_streaming_status=",
    "package_streaming_ready=",
    "package_streaming_resident_bytes=",
    "package_streaming_committed_records=",
    "package_streaming_required_preload_assets=",
    "package_streaming_resident_resource_kinds=",
    "package_streaming_resident_packages=",
    "package_streaming_diagnostics=",
    "--require-renderer-quality-gates",
    "evaluate_sdl_desktop_presentation_quality_gate",
    "renderer_quality_status=",
    "renderer_quality_ready=",
    "renderer_quality_diagnostics=",
    "renderer_quality_expected_framegraph_passes=",
    "renderer_quality_framegraph_passes_ok=",
    "renderer_quality_framegraph_execution_budget_ok=",
    "renderer_quality_scene_gpu_ready=",
    "renderer_quality_postprocess_ready=",
    "--require-postprocess-depth-input",
    "require_postprocess_depth_input",
    "enable_postprocess_depth_input",
    "postprocess_depth_input_ready=",
    "renderer_quality_postprocess_depth_input_ready=",
    "--require-directional-shadow",
    "--require-directional-shadow-filtering",
    "--require-shadow-morph-composition",
    "require_shadow_morph_composition",
    "require_graphics_morph_scene",
    "load_packaged_d3d12_shifted_shadow_receiver_scene_shaders",
    "load_packaged_vulkan_shifted_shadow_receiver_scene_shaders",
    "shifted_shadow_receiver_shader_diagnostic",
    "skinned_scene_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode",
    "enable_directional_shadow_smoke",
    "directional_shadow_status=",
    "directional_shadow_ready=",
    "directional_shadow_filter_mode=",
    "renderer_quality_directional_shadow_ready=",
    "renderer_quality_directional_shadow_filter_ready=",
    "--require-playable-3d-slice",
    "playable_3d_status=",
    "playable_3d_ready=",
    "playable_3d_diagnostics=",
    "playable_3d_scene_mesh_plan_ready=",
    "playable_3d_renderer_quality_ready=",
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
    "--require-scene-collision-package",
    "require_native_ui_overlay",
    "require_visible_3d_production_proof",
    "require_native_ui_textured_sprite_atlas",
    "require_native_ui_text_glyph_atlas",
    "mirakana/ui/ui.hpp",
    "mirakana/ui_renderer/ui_renderer.hpp",
    "build_ui_renderer_image_palette_from_runtime_ui_atlas",
    "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas",
    "hud_boxes=",
    "hud_images=",
    "hud_text_glyphs=",
    "text_glyphs_resolved=",
    "ui_atlas_metadata_status=",
    "ui_atlas_metadata_glyphs=",
    "ui_texture_overlay_status=",
    "enable_native_ui_overlay_textures",
    "ui_overlay_requested=",
    "ui_overlay_status=",
    "ui_overlay_ready=",
    "ui_overlay_sprites_submitted=",
    "ui_overlay_draws=",
    "visible_3d_status=",
    "visible_3d_presented_frames=",
    "visible_3d_d3d12_selected=",
    "collision_package_status=",
    "collision_package_bodies=",
    "collision_package_trigger_overlaps=",
    "gameplay_systems_collision_package_ready=",
    "load_packaged_d3d12_native_ui_overlay_shaders",
    "load_packaged_vulkan_native_ui_overlay_shaders",
    "native_ui_overlay_vertex_shader",
    "enable_native_ui_overlay"
)) {
    Assert-ContainsText $committedDesktop3dMainText $needle "committed generated 3D sample main.cpp"
}
foreach ($needle in @(
    "node.1.mesh_renderer.visible=true",
    "node.2.light.type=directional",
    "node.2.light.casts_shadows=true",
    "node.3.camera.projection=perspective",
    "node.3.camera.primary=true"
)) {
    Assert-ContainsText $committedDesktop3dSceneText $needle "committed generated 3D sample scene"
}
foreach ($needle in @(
    "entry.3.kind=morph_mesh_cpu",
    "entry.5.kind=animation_float_clip",
    "entry.6.kind=animation_float_clip",
    "entry.7.kind=animation_quaternion_clip",
    "kind=skinned_mesh",
    "kind=physics_collision_scene",
    "runtime/assets/3d/collision.collision3d"
)) {
    Assert-ContainsText $committedDesktop3dIndexText $needle "committed generated 3D sample geindex"
}
foreach ($needle in @(
    "RWByteAddressBuffer morph_position_deltas : register(u0, space1)",
    "cs_compute_morph_position",
    "cs_compute_morph_tangent_frame",
    "cs_compute_morph_skinned_position",
    "BLENDINDICES",
    "BLENDWEIGHT",
    "ps_shadow_receiver",
    "ShadowReceiverConstants",
    "MK_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS",
    "shadow_depth_texture : register(t0, space2)",
    "ShadowReceiverConstants : register(b2, space2)"
)) {
    Assert-ContainsText $committedDesktop3dSceneShaderText $needle "committed generated 3D sample scene shader"
}
foreach ($needle in @(
    "vs_shadow",
    "ps_shadow"
)) {
    Assert-ContainsText $committedDesktop3dShadowShaderText $needle "committed generated 3D sample shadow shader"
}
foreach ($needle in @(
    "vs_native_ui_overlay",
    "ps_native_ui_overlay"
)) {
    Assert-ContainsText $committedDesktop3dUiOverlayShaderText $needle "committed generated 3D sample UI overlay shader"
}
foreach ($needle in @("format=GameEngine.UiAtlas.v1", "asset.kind=ui_atlas", "source.decoding=unsupported", "atlas.packing=unsupported", "page.0.asset_uri=runtime/assets/3d/base_color.texture.geasset", "image.0.resource_id=hud.texture_atlas_proof")) {
    Assert-ContainsText $committedDesktop3dUiAtlasText $needle "committed generated 3D sample hud.uiatlas"
}
foreach ($needle in @("format=GameEngine.UiAtlas.v1", "asset.kind=ui_atlas", "source.decoding=unsupported", "atlas.packing=unsupported", "page.0.asset_uri=runtime/assets/3d/base_color.texture.geasset", "glyph.0.font_family=engine-default", "glyph.0.glyph=65")) {
    Assert-ContainsText $committedDesktop3dUiTextGlyphAtlasText $needle "committed generated 3D sample hud_text.uiatlas"
}
foreach ($needle in @("format=GameEngine.PhysicsCollisionScene3D.v1", "asset.kind=physics_collision_scene", "backend.native=unsupported", "body.0.name=floor", "body.0.compound=level_static", "body.1.name=collision_probe", "body.1.compound=level_static", "body.2.name=pickup_trigger", "body.2.trigger=true", "body.2.compound=interaction_triggers")) {
    Assert-ContainsText $committedDesktop3dCollisionText $needle "committed generated 3D sample collision.collision3d"
}
Assert-ContainsText $committedDesktop3dIndexText "kind=ui_atlas" "committed generated 3D sample package index"
Assert-ContainsText $committedDesktop3dIndexText "kind=ui_atlas_texture" "committed generated 3D sample package index"
Assert-DoesNotContainText $committedDesktop3dShadowShaderText "BLENDINDICES" "committed generated 3D sample shadow shader"
Assert-DoesNotContainText $committedDesktop3dShadowShaderText "BLENDWEIGHT" "committed generated 3D sample shadow shader"
foreach ($needle in @(
    "scene_depth_texture",
    "scene_depth_sampler"
)) {
    Assert-ContainsText $committedDesktop3dPostprocessShaderText $needle "committed generated 3D sample postprocess shader"
}
Assert-ContainsText $committedDesktop3dReadmeText "DesktopRuntime3DPackage" "committed generated 3D sample README"
Assert-ContainsText $committedDesktop3dReadmeText "--require-package-streaming-safe-point" "committed generated 3D sample README"
Assert-ContainsText $committedDesktop3dReadmeText "--require-renderer-quality-gates" "committed generated 3D sample README"
Assert-ContainsText $committedDesktop3dReadmeText "--require-postprocess-depth-input" "committed generated 3D sample README"
Assert-ContainsText $committedDesktop3dReadmeText "--require-directional-shadow-filtering" "committed generated 3D sample README"
Assert-ContainsText $committedDesktop3dReadmeText "--require-shadow-morph-composition" "committed generated 3D sample README"
Assert-ContainsText $committedDesktop3dReadmeText "--require-playable-3d-slice" "committed generated 3D sample README"
Assert-ContainsText $committedDesktop3dReadmeText "--require-gameplay-systems" "committed generated 3D sample README"
Assert-ContainsText $committedDesktop3dReadmeText "--require-native-ui-overlay" "committed generated 3D sample README"
Assert-ContainsText $committedDesktop3dReadmeText "--require-visible-3d-production-proof" "committed generated 3D sample README"
Assert-ContainsText $committedDesktop3dReadmeText "--require-native-ui-textured-sprite-atlas" "committed generated 3D sample README"
Assert-ContainsText $committedDesktop3dReadmeText "--require-native-ui-text-glyph-atlas" "committed generated 3D sample README"
Assert-ContainsText $committedDesktop3dReadmeText "--require-scene-collision-package" "committed generated 3D sample README"
Assert-ContainsText $engineManifestText "desktopRuntime3dPackageStreamingSafePointSmoke" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "--require-package-streaming-safe-point" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "package_streaming_status" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "desktopRuntime3dRendererQualityPackageSmoke" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "--require-renderer-quality-gates" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "renderer_quality_status" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "renderer_quality_expected_framegraph_passes=2" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "desktopRuntime3dPostprocessDepthPackageSmoke" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "--require-postprocess-depth-input" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "postprocess_depth_input_ready=1" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "renderer_quality_postprocess_depth_input_ready=1" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "desktopRuntime3dPlayablePackageSmoke" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "--require-playable-3d-slice" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "playable_3d_status" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "desktopRuntime3dDirectionalShadowPackageSmoke" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "--require-directional-shadow-filtering" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "directional_shadow_status=ready" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "desktopRuntime3dShadowMorphCompositionPackageSmoke" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "--require-shadow-morph-composition" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "renderer_morph_descriptor_binds" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "desktopRuntime3dNativeUiOverlayPackageSmoke" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "--require-native-ui-overlay" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "ui_overlay_ready=1" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "desktopRuntime3dVisibleProductionPackageProof" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "--require-visible-3d-production-proof" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "visible_3d_status=ready" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "desktopRuntime3dNativeUiTexturedSpriteAtlasPackageSmoke" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "--require-native-ui-textured-sprite-atlas" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "ui_texture_overlay_atlas_ready=1" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "desktopRuntime3dNativeUiTextGlyphAtlasPackageSmoke" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "--require-native-ui-text-glyph-atlas" "engine/agent/manifest.json"
Assert-ContainsText $engineManifestText "text_glyphs_resolved=2" "engine/agent/manifest.json"
Assert-ContainsText $planRegistryText "Generated 3D Committed Package Sample v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $planRegistryText "Generated 3D Renderer Quality Package Smoke v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $planRegistryText "Generated 3D Playable Package Smoke v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $planRegistryText "Generated 3D Postprocess Depth Package Smoke v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $planRegistryText "Generated 3D Directional Shadow Package Smoke v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $planRegistryText "Generated 3D Shadow Morph Composition Package Smoke v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $planRegistryText "Generated 3D Native UI Overlay Package Smoke v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $planRegistryText "Generated 3D Visible Production Game Proof v1" "docs/superpowers/plans/README.md"
Assert-ContainsText $roadmapText "Generated 3D Committed Package Sample v1" "docs/roadmap.md"
Assert-ContainsText $roadmapText "Generated 3D Renderer Quality Package Smoke v1" "docs/roadmap.md"
Assert-ContainsText $roadmapText "Generated 3D Playable Package Smoke v1" "docs/roadmap.md"
Assert-ContainsText $roadmapText "Generated 3D Postprocess Depth Package Smoke v1" "docs/roadmap.md"
Assert-ContainsText $roadmapText "Generated 3D Directional Shadow Package Smoke v1" "docs/roadmap.md"
Assert-ContainsText $roadmapText "Generated 3D Shadow Morph Composition Package Smoke v1" "docs/roadmap.md"
Assert-ContainsText $roadmapText "Generated 3D Native UI Overlay Package Smoke v1" "docs/roadmap.md"
Assert-ContainsText $roadmapText "Generated 3D Visible Production-Style Package Proof v1" "docs/roadmap.md"
Assert-ContainsText $roadmapText "Generated 3D Native UI Text Glyph Atlas Package Smoke v1" "docs/roadmap.md"
Assert-ContainsText $currentCapabilitiesText "sample_generated_desktop_runtime_3d_package" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesText "Generated 3D Renderer Quality Package Smoke v1" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesText "Generated 3D Playable Package Smoke v1" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesText "Generated 3D Postprocess Depth Package Smoke v1" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesText "Generated 3D Directional Shadow Package Smoke v1" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesText "--require-shadow-morph-composition" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesText "--require-native-ui-overlay" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesText "--require-visible-3d-production-proof" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesText "--require-native-ui-text-glyph-atlas" "docs/current-capabilities.md"
Assert-ContainsText $gameSkillText "postprocess_depth_input_ready=1" "gameengine game-development skill"
Assert-ContainsText $gameSkillText "renderer_quality_postprocess_depth_input_ready=1" "gameengine game-development skill"
Assert-ContainsText $gameSkillText "--require-shadow-morph-composition" "gameengine game-development skill"
Assert-ContainsText $gameSkillText "--require-native-ui-overlay" "gameengine game-development skill"
Assert-ContainsText $gameSkillText "--require-visible-3d-production-proof" "gameengine game-development skill"
Assert-ContainsText $gameSkillText "--require-native-ui-text-glyph-atlas" "gameengine game-development skill"
Assert-ContainsText $generatedScenariosText "--require-shadow-morph-composition" "docs/specs/generated-game-validation-scenarios.md"
Assert-ContainsText $generatedScenariosText "--require-native-ui-overlay" "docs/specs/generated-game-validation-scenarios.md"
Assert-ContainsText $generatedScenariosText "--require-visible-3d-production-proof" "docs/specs/generated-game-validation-scenarios.md"
Assert-ContainsText $generatedScenariosText "--require-native-ui-text-glyph-atlas" "docs/specs/generated-game-validation-scenarios.md"

$packageRegistrationRoot = New-ScaffoldCheckRoot
try {
    $packageRegistrationGameRoot = Join-Path $packageRegistrationRoot "games/package_apply_game"
    New-Item -ItemType Directory -Path (Join-Path $packageRegistrationGameRoot "runtime/scenes") -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $packageRegistrationGameRoot "runtime/package_apply_game.config") -Value "config=ok`n" -NoNewline
    Set-Content -LiteralPath (Join-Path $packageRegistrationGameRoot "runtime/scenes/start.scene") -Value "scene=ok`n" -NoNewline
    Set-Content -LiteralPath (Join-Path $packageRegistrationGameRoot "runtime/package_apply_game.geindex") -Value "index=ok`n" -NoNewline
    Set-Content -LiteralPath (Join-Path $packageRegistrationGameRoot "game.agent.json") -Value (@{
            schemaVersion = 1
            name = "package-apply-game"
            entryPoint = "games/package_apply_game/main.cpp"
            target = "package_apply_game"
            aiWorkflow = @{ validate = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1" }
            gameplayContract = @{ appType = "mirakana::GameApp" }
            backendReadiness = @{ platform = "sdl3-desktop" }
            importerRequirements = @{ sourceFormats = @() }
            packagingTargets = @("desktop-game-runtime", "desktop-runtime-release")
            runtimePackageFiles = @("runtime/package_apply_game.config")
            validationRecipes = @(@{ name = "desktop-runtime-release"; command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1" })
        } | ConvertTo-Json -Depth 10) -NoNewline

    $registerTool = Resolve-RequiredAgentPath "tools/register-runtime-package-files.ps1"
    & $registerTool `
        -RepositoryRoot $packageRegistrationRoot `
        -GameManifest "games/package_apply_game/game.agent.json" `
        -RuntimePackageFile "runtime/scenes/start.scene", "runtime/package_apply_game.geindex" | Out-Null
    & $registerTool `
        -RepositoryRoot $packageRegistrationRoot `
        -GameManifest "games/package_apply_game/game.agent.json" `
        -RuntimePackageFile "runtime/scenes/start.scene" | Out-Null

    $updatedPackageRegistrationManifest = Get-Content -LiteralPath (Join-Path $packageRegistrationGameRoot "game.agent.json") -Raw | ConvertFrom-Json
    foreach ($relativePath in @("runtime/package_apply_game.config", "runtime/scenes/start.scene", "runtime/package_apply_game.geindex")) {
        if ($updatedPackageRegistrationManifest.runtimePackageFiles -notcontains $relativePath) {
            Write-Error "runtime package registration apply tool did not register $relativePath"
        }
        if (@($updatedPackageRegistrationManifest.runtimePackageFiles | Where-Object { $_ -eq $relativePath }).Count -ne 1) {
            Write-Error "runtime package registration apply tool duplicated $relativePath"
        }
    }

    Set-Content -LiteralPath (Join-Path $packageRegistrationGameRoot "runtime/dry_run_only.file") -Value "dry=run`n" -NoNewline
    & $registerTool `
        -RepositoryRoot $packageRegistrationRoot `
        -GameManifest "games/package_apply_game/game.agent.json" `
        -RuntimePackageFile "runtime/dry_run_only.file" `
        -DryRun | Out-Null
    $afterDryRunManifest = Get-Content -LiteralPath (Join-Path $packageRegistrationGameRoot "game.agent.json") -Raw | ConvertFrom-Json
    if ($afterDryRunManifest.runtimePackageFiles -contains "runtime/dry_run_only.file") {
        Write-Error "runtime package registration dry-run must not write runtimePackageFiles"
    }

    $missingPropertyGameRoot = Join-Path $packageRegistrationRoot "games/package_apply_missing_property"
    New-Item -ItemType Directory -Path (Join-Path $missingPropertyGameRoot "runtime") -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $missingPropertyGameRoot "runtime/generated.config") -Value "config=ok`n" -NoNewline
    Set-Content -LiteralPath (Join-Path $missingPropertyGameRoot "game.agent.json") -Value (@{
            schemaVersion = 1
            name = "package-apply-missing-property"
            entryPoint = "games/package_apply_missing_property/main.cpp"
            target = "package_apply_missing_property"
            aiWorkflow = @{ validate = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1" }
            gameplayContract = @{ appType = "mirakana::GameApp" }
            backendReadiness = @{ platform = "sdl3-desktop" }
            importerRequirements = @{ sourceFormats = @() }
            packagingTargets = @("desktop-game-runtime", "desktop-runtime-release")
            validationRecipes = @(@{ name = "desktop-runtime-release"; command = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1" })
        } | ConvertTo-Json -Depth 10) -NoNewline
    & $registerTool `
        -RepositoryRoot $packageRegistrationRoot `
        -GameManifest "games/package_apply_missing_property/game.agent.json" `
        -RuntimePackageFile "runtime/generated.config" | Out-Null
    $missingPropertyManifest = Get-Content -LiteralPath (Join-Path $missingPropertyGameRoot "game.agent.json") -Raw | ConvertFrom-Json
    if ($missingPropertyManifest.runtimePackageFiles -notcontains "runtime/generated.config") {
        Write-Error "runtime package registration apply tool must create runtimePackageFiles when absent"
    }

    Assert-RegisterRuntimePackageFileFailure @(
        "-RepositoryRoot",
        $packageRegistrationRoot,
        "-GameManifest",
        "games/package_apply_game/game.agent.json",
        "-RuntimePackageFile",
        "../escape.txt"
    ) "runtime package registration parent traversal validation"
    Assert-RegisterRuntimePackageFileFailure @(
        "-RepositoryRoot",
        $packageRegistrationRoot,
        "-GameManifest",
        "games/package_apply_game/game.agent.json",
        "-RuntimePackageFile",
        "games/package_apply_game/runtime/scenes/start.scene"
    ) "runtime package registration repository-relative validation"
    Assert-RegisterRuntimePackageFileFailure @(
        "-RepositoryRoot",
        $packageRegistrationRoot,
        "-GameManifest",
        "games/package_apply_game/game.agent.json",
        "-RuntimePackageFile",
        "runtime/missing.file"
    ) "runtime package registration missing file validation"
    Assert-RegisterRuntimePackageFileFailure @(
        "-RepositoryRoot",
        $packageRegistrationRoot,
        "-GameManifest",
        "games/package_apply_game/game.agent.json",
        "-RuntimePackageFile",
        ""
    ) "runtime package registration empty path validation"
    Assert-RegisterRuntimePackageFileFailure @(
        "-RepositoryRoot",
        $packageRegistrationRoot,
        "-GameManifest",
        "games/package_apply_game/game.agent.json",
        "-RuntimePackageFile",
        (Join-Path $packageRegistrationGameRoot "runtime/scenes/start.scene")
    ) "runtime package registration absolute path validation"
    Assert-RegisterRuntimePackageFileFailure @(
        "-RepositoryRoot",
        $packageRegistrationRoot,
        "-GameManifest",
        "games/package_apply_game/game.agent.json",
        "-RuntimePackageFile",
        "runtime/scenes"
    ) "runtime package registration directory validation"
    Assert-RegisterRuntimePackageFileFailure @(
        "-RepositoryRoot",
        $packageRegistrationRoot,
        "-GameManifest",
        "games/package_apply_game/game.agent.json",
        "-RuntimePackageFile",
        "runtime/scenes/start.scene;runtime/package_apply_game.geindex"
    ) "runtime package registration CMake list separator validation"
    Assert-RegisterRuntimePackageFileFailure @(
        "-RepositoryRoot",
        $packageRegistrationRoot,
        "-GameManifest",
        "games/package_apply_game/game.agent.json",
        "-RuntimePackageFile",
        "runtime/scenes/start.scene",
        "runtime/scenes/start.scene"
    ) "runtime package registration duplicate request validation"
} finally {
    Remove-ScaffoldCheckRoot $packageRegistrationRoot
}

$invalidDisplayNameRoot = New-ScaffoldCheckRoot
try {
    Assert-NewGameFailure @(
        "-Name",
        "bad_display_game",
        "-DisplayName",
        "Bad`nDisplay",
        "-RepositoryRoot",
        $invalidDisplayNameRoot
    ) "new-game control-character DisplayName validation"
} finally {
    Remove-ScaffoldCheckRoot $invalidDisplayNameRoot
}

$invalidGameNameRoot = New-ScaffoldCheckRoot
try {
    Assert-NewGameFailure @(
        "-Name",
        "bad-hyphen-game",
        "-RepositoryRoot",
        $invalidGameNameRoot
    ) "new-game lowercase snake_case Name validation"
} finally {
    Remove-ScaffoldCheckRoot $invalidGameNameRoot
}

$targetCollisionRoot = New-ScaffoldCheckRoot
try {
    Set-Content -LiteralPath (Join-Path $targetCollisionRoot "games/CMakeLists.txt") -Value @"
MK_add_game(collision_game
    SOURCES
        collision_game/main.cpp
)
"@ -NoNewline
    Assert-NewGameFailure @(
        "-Name",
        "collision_game",
        "-RepositoryRoot",
        $targetCollisionRoot
    ) "new-game target collision validation"
} finally {
    Remove-ScaffoldCheckRoot $targetCollisionRoot
}

Write-Information "ai-integration-check: ok" -InformationAction Continue
