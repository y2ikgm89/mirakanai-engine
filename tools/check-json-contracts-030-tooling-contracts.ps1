#requires -Version 7.0
#requires -PSEdition Core
# Chapter 3 for check-json-contracts.ps1 static contracts.
$validateDesktopRuntimeScript = Get-Content -LiteralPath (Join-Path $root "tools/validate-desktop-game-runtime.ps1") -Raw
foreach ($requiredNeedle in @(
        "MK_runtime_host_win32_public_api_compile",
        "MK_runtime_host_win32_tests",
        "MK_win32_platform_tests",
        "MK_wasapi_audio_tests"
    )) {
    if (-not $validateDesktopRuntimeScript.Contains($requiredNeedle)) {
        Write-Error "desktop runtime validation must target Windows native host evidence: $requiredNeedle"
    }
}
foreach ($forbiddenNeedle in @(
        "MK_runtime_host_sdl3_public_api_compile",
        "MK_runtime_host_sdl3_tests",
        "MK_sdl3_platform_tests",
        "MK_sdl3_audio_tests"
    )) {
    if ($validateDesktopRuntimeScript.Contains($forbiddenNeedle)) {
        Write-Error "desktop runtime validation must not require SDL3 test target: $forbiddenNeedle"
    }
}
if (-not $validateDesktopRuntimeScript.Contains("--preset desktop-runtime -C Debug --output-on-failure")) {
    Write-Error "desktop runtime validation must pass Debug configuration to the desktop-runtime multi-config CTest preset"
}
$packageDesktopRuntimeScript = Get-Content -LiteralPath (Join-Path $root "tools/package-desktop-runtime.ps1") -Raw
foreach ($requiredNeedle in @(
        "MK_runtime_host_tests",
        "MK_runtime_host_win32_tests",
        "MK_win32_platform_tests",
        "MK_wasapi_audio_tests"
    )) {
    if (-not $packageDesktopRuntimeScript.Contains($requiredNeedle)) {
        Write-Error "desktop runtime package validation must include Windows native host CTest pattern: $requiredNeedle"
    }
}
foreach ($forbiddenNeedle in @(
        "mirakana_runtime_host_sdl3_tests",
        "mirakana_sdl3_platform_tests",
        "mirakana_sdl3_audio_tests",
        "MK_runtime_host_sdl3_tests",
        "MK_sdl3_platform_tests",
        "MK_sdl3_audio_tests"
    )) {
    if ($packageDesktopRuntimeScript.Contains($forbiddenNeedle)) {
        Write-Error "desktop runtime package validation must not require SDL3 CTest pattern: $forbiddenNeedle"
    }
}
$validateInstalledDesktopRuntimeScript =
    Get-Content -LiteralPath (Join-Path $root "tools/validate-installed-desktop-runtime.ps1") -Raw
foreach ($requiredNeedle in @(
        "Installed desktop runtime package must not ship SDL3 runtime DLL",
        "SDL3.dll"
    )) {
    if (-not $validateInstalledDesktopRuntimeScript.Contains($requiredNeedle)) {
        Write-Error "installed desktop runtime validation must reject SDL3 runtime DLL artifacts: $requiredNeedle"
    }
}
if ($validateInstalledDesktopRuntimeScript.Contains("Installed SDL3 runtime DLL was not found")) {
    Write-Error "installed desktop runtime validation must not require SDL3.dll"
}
$installedSdkValidationScript = Get-Content -LiteralPath (Join-Path $root "tools/installed-sdk-validation.ps1") -Raw
if (-not $installedSdkValidationScript.Contains("Installed Mirakanai SDK must not ship SDL3 runtime DLL")) {
    Write-Error "installed SDK validation must reject SDL3 runtime DLL artifacts"
}
$releasePackageArtifactsScript = Get-Content -LiteralPath (Join-Path $root "tools/release-package-artifacts.ps1") -Raw
foreach ($requiredNeedle in @(
        "Assert-ReleasePackageHasNoForbiddenRuntimeDlls",
        "SDL3.dll"
    )) {
    if (-not $releasePackageArtifactsScript.Contains($requiredNeedle)) {
        Write-Error "release package artifact validation must reject SDL3 runtime DLL artifacts: $requiredNeedle"
    }
}
$editorCmakeContent = Get-Content -LiteralPath (Join-Path $root "editor/CMakeLists.txt") -Raw
foreach ($forbiddenNeedle in @(
        "find_package(SDL3",
        "SDL3::SDL3",
        "MK_platform_sdl3"
    )) {
    if ($editorCmakeContent.Contains($forbiddenNeedle)) {
        Write-Error "native editor shell must remain SDL3-free: $forbiddenNeedle"
    }
}
foreach ($requiredNeedle in @(
        "add_library(MK_editor_shell_common",
        "src/native_editor_launch.cpp", "src/native_editor_text_atlas_handoff.cpp",
        "src/native_editor_uia_provider.cpp",
        "src/native_editor_text_font_adapters.cpp",
        "add_library(MK_editor_shell_win32",
        "src/native_editor_app.cpp",
        "src/win32_first_party_editor_host.cpp",
        "MK_editor_core",
        "MK_renderer",
        "MK_ui_renderer",
        "MK_platform_win32", "dwrite", "uiautomationcore",
        "MK_ENABLE_DESKTOP_EDITOR is Windows-only",
        "MK_ENABLE_DESKTOP_EDITOR requires MK_platform_win32",
        "add_executable(MK_editor",
        "src/main.cpp",
        "Native MK_editor shell is SDL3-free"
    )) {
    if (-not $editorCmakeContent.Contains($requiredNeedle)) {
        Write-Error "editor/CMakeLists.txt must expose the native editor shell contract: $requiredNeedle"
    }
}

$rootCmakeContent = Get-Content -LiteralPath (Join-Path $root "CMakeLists.txt") -Raw
if ($rootCmakeContent.Contains("if(MK_ENABLE_DESKTOP_EDITOR)`r`n    set(MK_DESKTOP_RUNTIME_ENABLED ON)")) {
    Write-Error "MK_ENABLE_DESKTOP_EDITOR must not imply the removed SDL3 desktop runtime lane"
}
$editorDockingSmokeNeedles = @("editor_shell_docking_status=single_window_ready", "editor_shell_dock_tab_headers=11", "editor_shell_dock_split_gutters=3", "editor_shell_dock_active_panels=4", "editor_shell_dock_focusable_controls=11")
$editorNativeVisibleTextureSmokeNeedles = @("editor_shell_viewport_status=d3d12_texture_ready", "editor_shell_viewport_visible_texture_composites", "editor_shell_viewport_native_handles_exposed=0", "editor_shell_material_preview_status=d3d12_texture_ready", "editor_shell_material_preview_visible_texture_composites", "editor_shell_material_preview_native_handles_exposed=0")
$editorTextAtlasSmokeNeedles = @("editor_shell_text_atlas_handoff_status=glyphs_ready_atlas_handoff_host_gated", "editor_shell_text_font_adapter_invoked=1", "editor_shell_text_font_glyphs_ready=1", "editor_shell_text_font_fallback_used=0", "editor_shell_text_atlas_handoff_host_gated_rows=1", "editor_shell_text_atlas_handoff_unsupported_rows=1", "editor_shell_text_font_native_handles_exposed=0")
$editorImeSmokeNeedles = @("editor_shell_text_input_service=win32_tsf", "editor_shell_ime_service=win32_tsf", "editor_shell_ime_status=win32_tsf_selected", "editor_shell_ime_caret_rect_rows=1", "editor_shell_ime_surrounding_text_rows=1", "editor_shell_ime_native_handles_exposed=0")
$editorAccessibilitySmokeNeedles = @("editor_shell_accessibility_service=win32_uia", "editor_shell_accessibility_status=uia_provider_ready", "editor_shell_accessibility_nodes", "editor_shell_accessibility_role_rows", "editor_shell_accessibility_name_rows", "editor_shell_accessibility_state_rows", "editor_shell_accessibility_focus_rows=1", "editor_shell_accessibility_action_rows", "editor_shell_accessibility_relationship_rows", "editor_shell_accessibility_tree_navigation_rows", "editor_shell_accessibility_diagnostics=0", "editor_shell_accessibility_missing_name_diagnostics=0", "editor_shell_accessibility_missing_role_diagnostics=0", "editor_shell_accessibility_invalid_bounds_diagnostics=0", "editor_shell_accessibility_hidden_nodes=0", "editor_shell_accessibility_unsupported_pattern_diagnostics=0", "editor_shell_accessibility_native_handles_exposed=0")
foreach ($requiredNeedle in @(
        "MK_editor_native_shell_tests",
        "MK_editor_smoke",
        "--smoke-resize",
        "--no-user-config",
        "editor_shell_ui=first_party",
        "editor_shell_backend=d3d12",
        "editor_shell_imgui=0",
        "editor_shell_panels=11"
    ) + $editorDockingSmokeNeedles + @(
        "editor_shell_sdl3=0"
    ) + $editorNativeVisibleTextureSmokeNeedles + $editorTextAtlasSmokeNeedles + $editorImeSmokeNeedles + $editorAccessibilitySmokeNeedles + @(
        "editor_shell_renderer_boxes_submitted",
        "editor_shell_renderer_text_runs_available",
        "tests/unit/editor_native_shell_tests.cpp",
        "MK_editor_shell_common"
    )) {
    if (-not $rootCmakeContent.Contains($requiredNeedle)) {
        Write-Error "root CMakeLists.txt must register native editor shell launch-contract tests: $requiredNeedle"
    }
}

$buildEditorScript = Get-Content -LiteralPath (Join-Path $root "tools/build-editor.ps1") -Raw
foreach ($requiredNeedle in @(
        "--preset desktop-editor",
        "--build --preset desktop-editor",
        "--output-on-failure"
    )) {
    if (-not $buildEditorScript.Contains($requiredNeedle)) {
        Write-Error "tools/build-editor.ps1 must validate the native desktop editor lane: $requiredNeedle"
    }
}
foreach ($forbiddenNeedle in @(
        "Assert-VcpkgExecutable",
        "visible editor shell is deferred",
        "launch-contract skeleton",
        "SDL3 removal"
    )) {
    if ($buildEditorScript.Contains($forbiddenNeedle)) {
        Write-Error "tools/build-editor.ps1 must not keep deferred or dependency wording: $forbiddenNeedle"
    }
}

$applicationManifestFragment = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.fragments/005-applications.json") -Raw
foreach ($requiredNeedle in @(
        "optional-native-win32-shell-active",
        "Win32 first-party retained shell",
        "MK_ui_renderer",
        "editor_shell_ui=first_party",
        "editor_shell_backend=d3d12",
        "editor_shell_imgui=0",
        "editor_shell_panels=11"
    ) + $editorDockingSmokeNeedles + $editorNativeVisibleTextureSmokeNeedles + $editorTextAtlasSmokeNeedles + $editorImeSmokeNeedles + $editorAccessibilitySmokeNeedles + @(
        "editor_shell_renderer_boxes_submitted",
        "editor_shell_renderer_text_runs_available", "Windows SDK DirectWrite text-layout/glyph-raster",
        "private Windows TSF text-input/IME session adapter",
        "Viewport",
        "Material Preview",
        "tools/build-editor.ps1",
        "tools/evaluate-cpp23.ps1 -Editor",
        "must not depend on SDL3"
    )) {
    if (-not $applicationManifestFragment.Contains($requiredNeedle)) {
        Write-Error "MK_editor application manifest must describe the active native editor shell: $requiredNeedle"
    }
}

$validationRecipeManifestFragment =
    Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.fragments/009-validationRecipes.json") -Raw
foreach ($requiredNeedle in @(
        "native Win32 first-party retained MK_editor shell",
        "make_editor_rich_text_view_model",
        "make_editor_rich_text_ai_snapshot", "private native_editor_text_font_adapters.cpp DirectWrite text/font adapter validation",
        "private native_editor_text_atlas_handoff.cpp selected text atlas handoff evidence",
        "private native_editor_text_input.cpp focused text-input/IME controller evidence",
        "private native_editor_tsf_text_input.cpp TSF session selection evidence",
        "console rich-text coverage proves read-only diagnostic spans",
        "MK_editor_native_shell_tests",
        "MK_editor_smoke",
        "editor_shell_ui=first_party",
        "editor_shell_backend=d3d12",
        "editor_shell_imgui=0",
        "editor_shell_panels=11"
    ) + $editorDockingSmokeNeedles + $editorNativeVisibleTextureSmokeNeedles + $editorTextAtlasSmokeNeedles + $editorImeSmokeNeedles + $editorAccessibilitySmokeNeedles + @(
        "editor_shell_renderer_boxes_submitted",
        "editor_shell_renderer_text_runs_available",
        "SDL3-free"
    )) {
    if (-not $validationRecipeManifestFragment.Contains($requiredNeedle)) {
        Write-Error "desktop-editor validation recipe must describe the active native editor shell: $requiredNeedle"
    }
}

$sampleDesktopRuntimeShellSource = Get-Content -LiteralPath (Join-Path $root "games/sample_desktop_runtime_shell/main.cpp") -Raw
foreach ($requiredNeedle in @(
        "mirakana/runtime_host/win32/win32_desktop_game_host.hpp",
        "mirakana::Win32DesktopGameHost"
    )) {
    if (-not $sampleDesktopRuntimeShellSource.Contains($requiredNeedle)) {
        Write-Error "sample_desktop_runtime_shell must exercise the Windows native runtime host: $requiredNeedle"
    }
}
foreach ($forbiddenNeedle in @(
        "SdlDesktopGameHost",
        "mirakana/runtime_host/sdl3"
    )) {
    if ($sampleDesktopRuntimeShellSource.Contains($forbiddenNeedle)) {
        Write-Error "sample_desktop_runtime_shell must not use SDL3 runtime host surface: $forbiddenNeedle"
    }
}

$newGameTemplatesScript = Get-Content -LiteralPath (Join-Path $root "tools/new-game-templates.ps1") -Raw
foreach ($requiredNeedle in @(
        "mirakana/runtime_host/win32/win32_desktop_game_host.hpp",
        "mirakana::Win32DesktopGameHost",
        "MK_platform_win32",
        "MK_runtime_host_win32",
        "MK_runtime_host_win32_presentation",
        "win32-desktop"
    )) {
    if (-not $newGameTemplatesScript.Contains($requiredNeedle)) {
        Write-Error "generated desktop game templates must target Windows native host evidence: $requiredNeedle"
    }
}
foreach ($forbiddenNeedle in @(
        "mirakana/runtime_host/sdl3",
        "mirakana::SdlDesktopGameHost",
        "mirakana::SdlDesktopPresentation",
        "SdlDesktopPresentation",
        "sdl_desktop_",
        "sdl3-desktop",
        "--video-driver",
        "MK_platform_sdl3",
        "MK_runtime_host_sdl3"
    )) {
    if ($newGameTemplatesScript.Contains($forbiddenNeedle)) {
        Write-Error "generated desktop game templates must not use SDL3 host surface: $forbiddenNeedle"
    }
}

$gamesCmakeContent = Get-Content -LiteralPath (Join-Path $root "games/CMakeLists.txt") -Raw
$generatedDesktopPackageSamples = @(
    "sample_generated_desktop_runtime_package",
    "sample_generated_desktop_runtime_cooked_scene_package",
    "sample_generated_desktop_runtime_material_shader_package",
    "sample_2d_desktop_runtime_package",
    "sample_generated_desktop_runtime_3d_package"
)
foreach ($sampleGame in $generatedDesktopPackageSamples) {
    $escapedSampleGame = [regex]::Escape($sampleGame)
    $targetBlockMatch =
        [regex]::Match($gamesCmakeContent, "(?s)MK_add_desktop_runtime_game\($escapedSampleGame.*?PACKAGE_FILES_FROM_MANIFEST\s*\)")
    if (-not $targetBlockMatch.Success) {
        Write-Error "games/CMakeLists.txt must register generated desktop package target: $sampleGame"
    } else {
        $targetBlock = $targetBlockMatch.Value
        foreach ($requiredNeedle in @(
                "HOST_BACKEND",
                "win32"
            )) {
            if (-not $targetBlock.Contains($requiredNeedle)) {
                Write-Error "$sampleGame CMake registration must select Windows native host backend: $requiredNeedle"
            }
        }
        if ($targetBlock.Contains("--video-driver")) {
            Write-Error "$sampleGame CMake registration must not pass SDL3 video driver hints"
        }
    }
}

foreach ($sampleGame in $generatedDesktopPackageSamples) {
    $sampleGameSource = Get-Content -LiteralPath (Join-Path $root "games/$sampleGame/main.cpp") -Raw
    foreach ($requiredNeedle in @(
            "mirakana/runtime_host/win32/win32_desktop_game_host.hpp",
            "mirakana::Win32DesktopGameHost"
        )) {
        if (-not $sampleGameSource.Contains($requiredNeedle)) {
            Write-Error "$sampleGame must exercise the Windows native runtime host: $requiredNeedle"
        }
    }
    foreach ($forbiddenNeedle in @(
            "mirakana/runtime_host/sdl3",
            "SdlDesktopGameHost",
            "SdlDesktopPresentation",
            "sdl_desktop_",
            "--video-driver"
        )) {
        if ($sampleGameSource.Contains($forbiddenNeedle)) {
            Write-Error "$sampleGame must not use SDL3 runtime host surface: $forbiddenNeedle"
        }
    }

    $sampleGameManifestPath = Join-Path $root "games/$sampleGame/game.agent.json"
    $sampleGameManifestText = Get-Content -LiteralPath $sampleGameManifestPath -Raw
    $sampleGameManifest = $sampleGameManifestText | ConvertFrom-Json
    foreach ($requiredModule in @(
            "MK_platform_win32",
            "MK_runtime_host_win32",
            "MK_runtime_host_win32_presentation"
        )) {
        if (@($sampleGameManifest.engineModules) -notcontains $requiredModule) {
            Write-Error "$sampleGame manifest must declare Windows native host module: $requiredModule"
        }
    }
    foreach ($forbiddenNeedle in @(
            "MK_platform_sdl3",
            "MK_runtime_host_sdl3",
            "MK_runtime_host_sdl3_presentation",
            "sdl3-desktop",
            "sdl3-desktop-host",
            "--video-driver"
        )) {
        if ($sampleGameManifestText.Contains($forbiddenNeedle)) {
            Write-Error "$sampleGame manifest must not retain SDL3 desktop host readiness text: $forbiddenNeedle"
        }
    }
}

foreach ($commandId in $scenePrefabAuthoringCommandIds) {
    $scenePrefabCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq $commandId })
    if ($scenePrefabCommand.Count -ne 1 -or $scenePrefabCommand[0].status -ne "ready") {
        Write-Error "engine manifest aiOperableProductionLoop must expose one ready Scene/Prefab v2 authoring command surface: $commandId"
    } else {
        $scenePrefabModes = @{}
        foreach ($mode in @($scenePrefabCommand[0].requestModes)) {
            $scenePrefabModes[$mode.id] = $mode
        }
        if (-not $scenePrefabModes.ContainsKey("dry-run") -or $scenePrefabModes["dry-run"].status -ne "ready" -or
            -not $scenePrefabModes.ContainsKey("apply") -or $scenePrefabModes["apply"].status -ne "ready") {
            Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' must keep dry-run and apply ready"
        }
        foreach ($module in @("MK_scene", "MK_tools")) {
            if (@($scenePrefabCommand[0].requiredModules) -notcontains $module) {
                Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' missing required module: $module"
            }
        }
        foreach ($field in @("changedFiles", "modelMutations", "validationRecipes", "unsupportedGapIds", "undoToken")) {
            if (@($scenePrefabCommand[0].resultShape.dryRunFields) -notcontains $field) {
                Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' dryRunFields missing: $field"
            }
        }
        foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
            if (@($scenePrefabCommand[0].resultShape.applyFields) -notcontains $field) {
                Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' applyFields missing: $field"
            }
        }
        $scenePrefabPolicyText = "$($scenePrefabCommand[0].summary) $($scenePrefabCommand[0].requestShape.pathPolicy) $($scenePrefabCommand[0].notes)"
        foreach ($needle in @(
                "GameEngine.Scene.v2",
                "GameEngine.Prefab.v2",
                "plan_scene_prefab_authoring",
                "apply_scene_prefab_authoring",
                "safe repository-relative",
                "does not evaluate arbitrary shell",
                "free-form edits are not supported",
                "Scene v2 runtime package migration",
                "editor productization",
                "nested prefab merge/resolution UX"
            )) {
            if (-not $scenePrefabPolicyText.Contains($needle)) {
                Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' must document reviewed helper/policy text: $needle"
            }
        }
    }
}
$sourceAssetCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "register-source-asset" })
if ($sourceAssetCommand.Count -ne 1 -or $sourceAssetCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready register-source-asset command surface"
} else {
    $sourceAssetModes = @{}
    foreach ($mode in @($sourceAssetCommand[0].requestModes)) {
        $sourceAssetModes[$mode.id] = $mode
    }
    if (-not $sourceAssetModes.ContainsKey("dry-run") -or $sourceAssetModes["dry-run"].status -ne "ready" -or
        -not $sourceAssetModes.ContainsKey("apply") -or $sourceAssetModes["apply"].status -ne "ready") {
        Write-Error "engine manifest register-source-asset must keep dry-run and apply ready"
    }
    foreach ($module in @("MK_assets", "MK_tools")) {
        if (@($sourceAssetCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest register-source-asset missing required module: $module"
        }
    }
    foreach ($field in @("changedFiles", "modelMutations", "importMetadata", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($sourceAssetCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest register-source-asset dryRunFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("cookedOutputHint", "packageIndexPath", "backend", "shaderArtifactRequirements", "renderer", "rhi", "metal", "descriptor", "pipeline", "nativeHandle")) {
        if (@($sourceAssetCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($sourceAssetCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine manifest register-source-asset must not expose package/renderer/native field: $forbiddenField"
        }
    }
    foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
        if (@($sourceAssetCommand[0].resultShape.applyFields) -notcontains $field) {
            Write-Error "engine manifest register-source-asset applyFields missing: $field"
        }
    }
    $sourceAssetPolicyText = "$($sourceAssetCommand[0].summary) $($sourceAssetCommand[0].requestShape.pathPolicy) $($sourceAssetCommand[0].notes)"
    foreach ($needle in @(
            "GameEngine.AssetIdentity.v2",
            "GameEngine.SourceAssetRegistry.v1",
            "plan_source_asset_registration",
            "apply_source_asset_registration",
            "safe repository-relative",
            "does not evaluate arbitrary shell",
            "free-form edits are not supported",
            "external importer execution is not supported",
            "does not write cooked artifacts",
            "does not update .geindex",
            "package cooking",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles"
        )) {
        if (-not $sourceAssetPolicyText.Contains($needle)) {
            Write-Error "engine manifest register-source-asset must document reviewed helper/policy text: $needle"
        }
    }
    $sourceAssetHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/source_asset_registration_tool.hpp"
    $sourceAssetSourcePath = Join-Path $root "engine/tools/asset/source_asset_registration_tool.cpp"
    foreach ($requiredPath in @($sourceAssetHeaderPath, $sourceAssetSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "register-source-asset reviewed helper file is missing: $requiredPath"
        }
    }
    foreach ($helperPath in @($sourceAssetHeaderPath, $sourceAssetSourcePath)) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/tools/asset_import_tool.hpp",
                "mirakana/tools/asset_package_tool.hpp",
                "mirakana/assets/asset_package.hpp",
                "mirakana/renderer/",
                "mirakana/rhi/",
                "execute_asset_import_plan",
                "assemble_asset_cooked_package",
                "write_asset_cooked_package_index"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must not use package/import execution or renderer/RHI surface: $forbiddenText"
                }
            }
        }
    }
}
$registeredCookCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "cook-registered-source-assets" })
if ($registeredCookCommand.Count -ne 1 -or $registeredCookCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready cook-registered-source-assets command surface"
} else {
    $registeredCookModes = @{}
    foreach ($mode in @($registeredCookCommand[0].requestModes)) {
        $registeredCookModes[$mode.id] = $mode
    }
    if (-not $registeredCookModes.ContainsKey("dry-run") -or $registeredCookModes["dry-run"].status -ne "ready" -or
        -not $registeredCookModes.ContainsKey("apply") -or $registeredCookModes["apply"].status -ne "ready") {
        Write-Error "engine manifest cook-registered-source-assets must keep dry-run and apply ready"
    }
    foreach ($module in @("MK_assets", "MK_tools")) {
        if (@($registeredCookCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest cook-registered-source-assets missing required module: $module"
        }
    }
    foreach ($field in @("changedFiles", "modelMutations", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($registeredCookCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest cook-registered-source-assets dryRunFields missing: $field"
        }
    }
    foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
        if (@($registeredCookCommand[0].resultShape.applyFields) -notcontains $field) {
            Write-Error "engine manifest cook-registered-source-assets applyFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("backend", "nativeHandle", "rhiHandle", "rendererBackend", "metalDevice")) {
        if (@($registeredCookCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($registeredCookCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine manifest cook-registered-source-assets must not expose renderer/native handle field: $forbiddenField"
        }
    }
    $registeredCookPolicyText = "$($registeredCookCommand[0].summary) $($registeredCookCommand[0].requestShape.pathPolicy) $($registeredCookCommand[0].notes)"
    foreach ($needle in @(
            "GameEngine.SourceAssetRegistry.v1",
            "explicitly selected",
            "plan_registered_source_asset_cook_package",
            "apply_registered_source_asset_cook_package",
            "build_asset_import_plan",
            "execute_asset_import_plan",
            "assemble_asset_cooked_package",
            "safe repository-relative",
            "package-relative",
            "external importer execution is not supported",
            "broad dependency cooking",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles",
            "general production renderer quality",
            "free-form edits are not supported"
        )) {
        if (-not $registeredCookPolicyText.Contains($needle)) {
            Write-Error "engine manifest cook-registered-source-assets must document reviewed helper/policy text: $needle"
        }
    }
    $registeredCookHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/registered_source_asset_cook_package_tool.hpp"
    $registeredCookSourcePath = Join-Path $root "engine/tools/asset/registered_source_asset_cook_package_tool.cpp"
    foreach ($requiredPath in @($registeredCookHeaderPath, $registeredCookSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "cook-registered-source-assets reviewed helper file is missing: $requiredPath"
        }
    }
    if (Test-Path -LiteralPath $registeredCookSourcePath -PathType Leaf) {
        $helperText = Get-Content -LiteralPath $registeredCookSourcePath -Raw
        foreach ($requiredText in @(
            "mirakana/tools/asset_import_tool.hpp",
            "mirakana/tools/asset_package_tool.hpp",
            "build_asset_import_plan",
            "execute_asset_import_plan",
            "assemble_asset_cooked_package"
        )) {
            if (-not $helperText.Contains($requiredText)) {
                Write-Error "$registeredCookSourcePath must reuse selected source asset import/package helper: $requiredText"
            }
        }
    }
    foreach ($helperPath in @($registeredCookHeaderPath, $registeredCookSourcePath)) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/renderer/",
                "mirakana/rhi/",
                "IRhiDevice",
                "ID3D12",
                "VkDevice",
                "MTLDevice"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must not use renderer/RHI/native surfaces: $forbiddenText"
                }
            }
        }
    }
}
$sceneMigrationCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "migrate-scene-v2-runtime-package" })
if ($sceneMigrationCommand.Count -ne 1 -or $sceneMigrationCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready migrate-scene-v2-runtime-package command surface"
} else {
    $sceneMigrationModes = @{}
    foreach ($mode in @($sceneMigrationCommand[0].requestModes)) {
        $sceneMigrationModes[$mode.id] = $mode
    }
    if (-not $sceneMigrationModes.ContainsKey("dry-run") -or $sceneMigrationModes["dry-run"].status -ne "ready" -or
        -not $sceneMigrationModes.ContainsKey("apply") -or $sceneMigrationModes["apply"].status -ne "ready") {
        Write-Error "engine manifest migrate-scene-v2-runtime-package must keep dry-run and apply ready"
    }
    foreach ($module in @("MK_scene", "MK_assets", "MK_tools")) {
        if (@($sceneMigrationCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package missing required module: $module"
        }
    }
    foreach ($field in @("changedFiles", "modelMutations", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($sceneMigrationCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package dryRunFields missing: $field"
        }
    }
    foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
        if (@($sceneMigrationCommand[0].resultShape.applyFields) -notcontains $field) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package applyFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("backend", "nativeHandle", "rhiHandle", "rendererBackend", "metalDevice")) {
        if (@($sceneMigrationCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($sceneMigrationCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package must not expose renderer/native handle field: $forbiddenField"
        }
    }
    $sceneMigrationPolicyText = "$($sceneMigrationCommand[0].summary) $($sceneMigrationCommand[0].requestShape.pathPolicy) $($sceneMigrationCommand[0].notes)"
    foreach ($needle in @(
            "GameEngine.Scene.v2",
            "GameEngine.SourceAssetRegistry.v1",
            "GameEngine.Scene.v1",
            "plan_scene_v2_runtime_package_migration",
            "apply_scene_v2_runtime_package_migration",
            "plan_scene_package_update",
            "apply_scene_package_update",
            "safe repository-relative",
            "package-relative",
            "external importer execution is not supported",
            "package cooking",
            "dependent asset cooking",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles",
            "general production renderer quality",
            "free-form edits are not supported"
        )) {
        if (-not $sceneMigrationPolicyText.Contains($needle)) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package must document reviewed helper/policy text: $needle"
        }
    }
    $sceneMigrationHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/scene_v2_runtime_package_migration_tool.hpp"
    $sceneMigrationSourcePath = Join-Path $root "engine/tools/scene/scene_v2_runtime_package_migration_tool.cpp"
    foreach ($requiredPath in @($sceneMigrationHeaderPath, $sceneMigrationSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "migrate-scene-v2-runtime-package reviewed helper file is missing: $requiredPath"
        }
    }
    foreach ($helperPath in @($sceneMigrationHeaderPath, $sceneMigrationSourcePath)) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/tools/asset_import_tool.hpp",
                "mirakana/tools/asset_package_tool.hpp",
                "mirakana/renderer/",
                "mirakana/rhi/",
                "execute_asset_import_plan",
                "assemble_asset_cooked_package",
                "write_asset_cooked_package_index",
                "IRhiDevice",
                "ID3D12",
                "VkDevice",
                "MTLDevice"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must not use importer/package execution or renderer/RHI/native surfaces: $forbiddenText"
                }
            }
        }
    }
    if (Test-Path -LiteralPath $sceneMigrationSourcePath -PathType Leaf) {
        $sceneMigrationSourceText = Get-Content -LiteralPath $sceneMigrationSourcePath -Raw
        foreach ($requiredCall in @(
                "plan_scene_package_update(",
                "apply_scene_package_update("
            )) {
            if (-not $sceneMigrationSourceText.Contains($requiredCall)) {
                Write-Error "migrate-scene-v2-runtime-package source must reuse existing scene package helper call: $requiredCall"
            }
        }
    }
}
$runtimeSceneValidationCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "validate-runtime-scene-package" })
if ($runtimeSceneValidationCommand.Count -ne 1 -or $runtimeSceneValidationCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready validate-runtime-scene-package command surface"
} else {
    $runtimeSceneValidationModes = @{}
    foreach ($mode in @($runtimeSceneValidationCommand[0].requestModes)) {
        $runtimeSceneValidationModes[$mode.id] = $mode
    }
    if (-not $runtimeSceneValidationModes.ContainsKey("dry-run") -or
        $runtimeSceneValidationModes["dry-run"].status -ne "ready" -or
        -not $runtimeSceneValidationModes.ContainsKey("execute") -or
        $runtimeSceneValidationModes["execute"].status -ne "ready") {
        Write-Error "engine manifest validate-runtime-scene-package must keep dry-run and execute ready"
    }
    if ($runtimeSceneValidationModes.ContainsKey("apply") -and $runtimeSceneValidationModes["apply"].status -eq "ready") {
        Write-Error "engine manifest validate-runtime-scene-package must remain non-mutating and must not expose ready apply"
    }
    foreach ($module in @("MK_runtime", "MK_runtime_scene", "MK_tools")) {
        if (@($runtimeSceneValidationCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest validate-runtime-scene-package missing required module: $module"
        }
    }
    foreach ($field in @("packageIndexPath", "sceneAssetKey")) {
        if (@($runtimeSceneValidationCommand[0].requestShape.requiredFields) -notcontains $field) {
            Write-Error "engine manifest validate-runtime-scene-package requestShape requiredFields missing: $field"
        }
    }
    foreach ($field in @("contentRoot", "validateAssetReferences", "requireUniqueNodeNames")) {
        if (@($runtimeSceneValidationCommand[0].requestShape.optionalFields) -notcontains $field) {
            Write-Error "engine manifest validate-runtime-scene-package requestShape optionalFields missing: $field"
        }
    }
    foreach ($field in @("packageSummary", "sceneAsset", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($runtimeSceneValidationCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest validate-runtime-scene-package dryRunFields missing: $field"
        }
    }
    foreach ($field in @("packageSummary", "sceneSummary", "references", "packageRecordCount", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($runtimeSceneValidationCommand[0].resultShape.executeFields) -notcontains $field) {
            Write-Error "engine manifest validate-runtime-scene-package executeFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("backend", "nativeHandle", "rhiHandle", "rendererBackend", "metalDevice", "shaderArtifactRequirements")) {
        if (@($runtimeSceneValidationCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($runtimeSceneValidationCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine manifest validate-runtime-scene-package must not expose renderer/native handle field: $forbiddenField"
        }
    }
    $runtimeSceneValidationPolicyText = "$($runtimeSceneValidationCommand[0].summary) $($runtimeSceneValidationCommand[0].requestShape.pathPolicy) $($runtimeSceneValidationCommand[0].notes)"
    foreach ($needle in @(
            "plan_runtime_scene_package_validation",
            "execute_runtime_scene_package_validation",
            "mirakana::runtime::load_runtime_asset_package",
            "mirakana::runtime_scene::instantiate_runtime_scene",
            "safe package-relative",
            "does not mutate",
            "runtime source parsing is not supported",
            "package cooking is not supported",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles",
            "general production renderer quality",
            "free-form edits are not supported"
        )) {
        if (-not $runtimeSceneValidationPolicyText.Contains($needle)) {
            Write-Error "engine manifest validate-runtime-scene-package must document reviewed helper/policy text: $needle"
        }
    }
    $runtimeSceneValidationHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/runtime_scene_package_validation_tool.hpp"
    $runtimeSceneValidationSourcePath = Join-Path $root "engine/tools/scene/runtime_scene_package_validation_tool.cpp"
    foreach ($requiredPath in @($runtimeSceneValidationHeaderPath, $runtimeSceneValidationSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "validate-runtime-scene-package reviewed helper file is missing: $requiredPath"
        }
    }
    if (Test-Path -LiteralPath $runtimeSceneValidationSourcePath -PathType Leaf) {
        $helperText = Get-Content -LiteralPath $runtimeSceneValidationSourcePath -Raw
        foreach ($requiredText in @(
            "mirakana/runtime/asset_runtime.hpp",
            "mirakana/runtime_scene/runtime_scene.hpp",
            "load_runtime_asset_package",
            "instantiate_runtime_scene"
        )) {
            if (-not $helperText.Contains($requiredText)) {
                Write-Error "$runtimeSceneValidationSourcePath must reuse runtime package/scene validation helper: $requiredText"
            }
        }
        foreach ($forbiddenText in @(
            "mirakana/tools/asset_import_tool.hpp",
            "mirakana/tools/asset_package_tool.hpp",
            "mirakana/renderer/",
            "mirakana/rhi/",
            "execute_asset_import_plan",
            "assemble_asset_cooked_package",
            "write_text(",
            "IRhiDevice",
            "ID3D12",
            "VkDevice",
            "MTLDevice"
        )) {
            if ($helperText.Contains($forbiddenText)) {
                Write-Error "$runtimeSceneValidationSourcePath must not mutate, import, package, or use renderer/RHI/native surfaces: $forbiddenText"
            }
        }
    }
}
$publicApiBoundaryScriptPath = Join-Path $root "tools/check-public-api-boundaries.ps1"
if (-not (Test-Path -LiteralPath $publicApiBoundaryScriptPath -PathType Leaf)) {
    Write-Error "tools/check-public-api-boundaries.ps1 is missing"
} else {
    $publicApiBoundaryText = Get-Content -LiteralPath $publicApiBoundaryScriptPath -Raw
    if (-not $publicApiBoundaryText.Contains("function Get-PublicApiHeaderFile")) {
        Write-Error "tools/check-public-api-boundaries.ps1 must use one shared Get-PublicApiHeaderFile enumeration helper"
    }
    $publicApiHeaderScanMatches = [System.Text.RegularExpressions.Regex]::Matches(
        $publicApiBoundaryText,
        "Get-ChildItem\s+-LiteralPath\s+\`$absoluteRoot\s+-Recurse\s+-File\s+-Include\s+\*\.h,\s+\*\.hpp,\s+\*\.hh,\s+\*\.hxx")
    if ($publicApiHeaderScanMatches.Count -ne 1) {
        Write-Error "tools/check-public-api-boundaries.ps1 must enumerate public headers once; found $($publicApiHeaderScanMatches.Count) recursive header scans"
    }
}
$commonScriptPath = Join-Path $root "tools/common.ps1"
$commonScriptText = Get-Content -LiteralPath $commonScriptPath -Raw
if (-not $commonScriptText.Contains("function Get-CxxSourceFile")) {
    Write-Error "tools/common.ps1 must expose shared Get-CxxSourceFile for format entrypoints"
}
foreach ($formatScriptName in @("check-format.ps1", "format.ps1")) {
    $formatScriptPath = Join-Path $root "tools/$formatScriptName"
    $formatScriptText = Get-Content -LiteralPath $formatScriptPath -Raw
    if (-not $formatScriptText.Contains("Get-CxxSourceFile -Root `$root")) {
        Write-Error "tools/$formatScriptName must use shared Get-CxxSourceFile"
    }
    if ($formatScriptText.Contains('$sourceRoots = @("engine", "editor", "examples", "games", "tests")')) {
        Write-Error "tools/$formatScriptName must not duplicate C++ source root lists"
    }
}
$checkFormatScriptText = Get-Content -LiteralPath (Join-Path $root "tools/check-format.ps1") -Raw
foreach ($needle in @(
        "Invoke-FormatBackgroundJob",
        "Start-ThreadJob",
        "Start-Job",
        "Resolve-ParallelJobCount -Jobs 0 -MaximumJobs 4",
        "Wait-Job -Job @(`$running | ForEach-Object { `$_.Job }) -Any",
        "clang-format failed for batch"
    )) {
    if (-not $checkFormatScriptText.Contains($needle)) {
        Write-Error "tools/check-format.ps1 must use bounded parallel clang-format dry-run batches: $needle"
    }
}
$licenseScriptPath = Join-Path $root "tools/check-license.ps1"
$licenseScriptText = Get-Content -LiteralPath $licenseScriptPath -Raw
if (-not $licenseScriptText.Contains("function Get-LicenseCheckedSourceFile")) {
    Write-Error "tools/check-license.ps1 must use one shared Get-LicenseCheckedSourceFile enumeration helper"
}
if (-not $licenseScriptText.Contains("git -C `$Root ls-files --cached --others --exclude-standard -- '*.cpp' '*.hpp'")) {
    Write-Error "tools/check-license.ps1 must enumerate tracked and unignored C++ license inputs through git ls-files"
}
if (-not $licenseScriptText.Contains('Write-Information "license-check: ok" -InformationAction Continue')) {
    Write-Error "tools/check-license.ps1 must report success through the information stream"
}
if ($licenseScriptText.Contains('Get-ChildItem -LiteralPath $directory.FullName')) {
    Write-Error "tools/check-license.ps1 must not traverse directory trees after switching to git ls-files enumeration"
}
if ($licenseScriptText.Contains('Get-ChildItem -Path $Root -Recurse -File -Include')) {
    Write-Error "tools/check-license.ps1 must not recursively scan excluded trees before filtering"
}
$validationRunnerCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "run-validation-recipe" })
if ($validationRunnerCommand.Count -ne 1 -or $validationRunnerCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready run-validation-recipe command surface"
} else {
    $runnerModes = @{}
    foreach ($mode in @($validationRunnerCommand[0].requestModes)) {
        $runnerModes[$mode.id] = $mode
    }
    if (-not $runnerModes.ContainsKey("dry-run") -or $runnerModes["dry-run"].status -ne "ready" -or
        -not $runnerModes.ContainsKey("execute") -or $runnerModes["execute"].status -ne "ready") {
        Write-Error "engine manifest run-validation-recipe must keep dry-run and execute ready"
    }
    foreach ($requiredField in @("mode", "validationRecipe")) {
        if (@($validationRunnerCommand[0].requestShape.requiredFields) -notcontains $requiredField) {
            Write-Error "engine manifest run-validation-recipe requestShape missing required field: $requiredField"
        }
    }
    foreach ($optionalField in @("gameTarget", "strictBackend", "hostGateAcknowledgements", "timeoutSeconds")) {
        if (@($validationRunnerCommand[0].requestShape.optionalFields) -notcontains $optionalField) {
            Write-Error "engine manifest run-validation-recipe requestShape missing optional field: $optionalField"
        }
    }
    $runnerPolicyText = "$($validationRunnerCommand[0].requestShape.pathPolicy) $($validationRunnerCommand[0].notes)"
    foreach ($needle in @(
            "tools/run-validation-recipe.ps1",
            "Get-ValidationRecipeCommandPlan",
            "Invoke-ValidationRecipeCommandPlan",
            "does not evaluate raw manifest command strings",
            "free-form arguments are rejected",
            "not arbitrary shell"
        )) {
        if (-not $runnerPolicyText.Contains($needle)) {
            Write-Error "engine manifest run-validation-recipe must document runner path/helper/policy text: $needle"
        }
    }
    foreach ($field in @("recipe", "status", "command", "argv", "hostGates", "diagnostics", "blockedBy")) {
        if (@($validationRunnerCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest run-validation-recipe dryRunFields missing: $field"
        }
    }
    foreach ($field in @("recipe", "status", "exitCode", "durationSeconds", "stdoutSummary", "stderrSummary", "hostGates", "diagnostics")) {
        if (@($validationRunnerCommand[0].resultShape.executeFields) -notcontains $field) {
            Write-Error "engine manifest run-validation-recipe executeFields missing: $field"
        }
    }
    foreach ($recipe in @(
            "agent-contract",
            "default",
            "public-api-boundary", "shader-toolchain",
            "renderer-metal-apple-host-evidence", "renderer-metal-environment-aggregate-apple-host-evidence",
            "desktop-game-runtime",
            "desktop-editor",
            "desktop-runtime-sample-game-scene-gpu-package",
            "desktop-runtime-sample-game-environment-fog-package",
            "desktop-runtime-sample-game-vulkan-environment-fog-package",
            "desktop-runtime-sample-game-environment-volumetric-fog-package",
            "desktop-runtime-sample-game-vulkan-physical-sky-package",
            "desktop-runtime-sample-game-cloud-layer-package",
            "desktop-runtime-sample-game-cloud-layer-renderer-execution",
            "desktop-runtime-sample-game-environment-precipitation-package",
            "desktop-runtime-sample-game-environment-precipitation-renderer-execution",
            "desktop-runtime-sample-game-vulkan-environment-precipitation-renderer-execution",
            "desktop-runtime-sample-game-environment-snow-package",
            "desktop-runtime-sample-game-environment-snow-renderer-execution",
            "desktop-runtime-sample-game-environment-material-weathering",
            "desktop-runtime-sample-game-environment-ibl-renderer-execution",
            "desktop-runtime-sample-game-vulkan-environment-ibl-renderer-execution",
            "desktop-runtime-sample-game-environment-profile-package", "desktop-runtime-sample-game-environment-ready-aggregate", "desktop-runtime-sample-game-environment-vulkan-strict-aggregate", "desktop-runtime-sample-game-environment-backend-parity", "desktop-runtime-sample-game-environment-backend-parity-ready", "desktop-runtime-sample-game-environment-platform-readiness", "desktop-runtime-sample-game-environment-artist-workflow-package", "desktop-runtime-sample-game-environment-commercial-readiness", "desktop-runtime-sample-game-environment-commercial-vulkan-evidence", "desktop-runtime-sample-game-environment-weather-simulation-vulkan-solver-package", "environment-weather-metal-solver-host-gate",
            "desktop-runtime-generated-material-shader-scaffold-package",
            "desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict",
            "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package",
            "installed-2d-performance-baseline-smoke",
            "installed-2d-long-run-readiness-smoke",
            "host-2d-long-run-readiness-soak",
            "dev-windows-editor-game-module-driver-load-tests"
        )) {
        if (@($validationRunnerCommand[0].validationRecipes) -notcontains $recipe) {
            Write-Error "engine manifest run-validation-recipe validationRecipes missing allowlisted recipe: $recipe"
        }
    }
    if (@($validationRunnerCommand[0].validationRecipes).Count -ne 41) {
        Write-Error "engine manifest run-validation-recipe validationRecipes must be exactly the reviewed allowlist of 41 recipes"
    }
    if (@($validationRunnerCommand[0].requestModes | Where-Object { $_.id -eq "apply" -and $_.status -eq "ready" }).Count -gt 0) {
        Write-Error "engine manifest run-validation-recipe must not expose a ready apply mode"
    }
    $runnerScriptPath = Join-Path $root "tools/run-validation-recipe.ps1"
    if (-not (Test-Path -LiteralPath $runnerScriptPath -PathType Leaf)) {
        Write-Error "engine manifest run-validation-recipe references missing tools/run-validation-recipe.ps1"
    }
    $runnerText = Get-Content -LiteralPath $runnerScriptPath -Raw
    foreach ($needle in @("validation-recipe-core.ps1", "Get-ValidationRecipeCommandPlan", "Invoke-ValidationRecipeCommandPlan")) {
        if (-not $runnerText.Contains($needle)) {
            Write-Error "tools/run-validation-recipe.ps1 missing reviewed helper name: $needle"
        }
    }
    $runnerCoreScriptPath = Join-Path $root "tools/validation-recipe-core.ps1"
    if (-not (Test-Path -LiteralPath $runnerCoreScriptPath -PathType Leaf)) {
        Write-Error "engine manifest run-validation-recipe references missing tools/validation-recipe-core.ps1"
    }
    $runnerCoreText = Get-Content -LiteralPath $runnerCoreScriptPath -Raw
    foreach ($needle in @("Test-ValidationRecipeRequest", "New-ValidationRecipeDryRunResult", "New-ValidationRecipeRejectedResult")) {
        if (-not $runnerCoreText.Contains($needle)) {
            Write-Error "tools/validation-recipe-core.ps1 missing reviewed helper name: $needle"
        }
    }
    foreach ($forbiddenRunnerPattern in @(
        "\bInvoke-Expression\b",
        "\biex\b",
        "\[scriptblock\]::Create",
        "\bcmd\s*/c\b",
        "\bbash\s+-c\b",
        "\bpwsh\b[^\r\n]*-Command\b",
        "\bpowershell\b[^\r\n]*-Command\b"
    )) {
        foreach ($runnerFile in @(
            @{ Label = "tools/run-validation-recipe.ps1"; Text = $runnerText },
            @{ Label = "tools/validation-recipe-core.ps1"; Text = $runnerCoreText }
        )) {
            if ([System.Text.RegularExpressions.Regex]::IsMatch($runnerFile.Text, $forbiddenRunnerPattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)) {
                Write-Error "$($runnerFile.Label) must not contain shell-eval pattern: $forbiddenRunnerPattern"
            }
        }
    }
}
foreach ($commandId in $expectedCommandSurfaceIds) {
    if (-not $commandSurfaceIds.ContainsKey($commandId)) {
        Write-Error "engine manifest aiOperableProductionLoop missing command surface id: $commandId"
    }
}

$aiGameDevelopmentText = Get-Content -LiteralPath (Join-Path $root "docs/ai-game-development.md") -Raw
$aiIntegrationText = Get-Content -LiteralPath (Join-Path $root "docs/ai-integration.md") -Raw
$generatedScenariosText = Get-Content -LiteralPath (Join-Path $root "docs/specs/generated-game-validation-scenarios.md") -Raw
$promptPackText = Get-Content -LiteralPath (Join-Path $root "docs/specs/game-prompt-pack.md") -Raw
$handoffPromptText = Get-Content -LiteralPath (Join-Path $root "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md") -Raw
$roadmapText = Get-Content -LiteralPath (Join-Path $root "docs/roadmap.md") -Raw
$authoredRuntimeWorkflowRequiredText = @(
    "validated authored-to-runtime workflow",
    "register-source-asset -> cook-registered-source-assets -> migrate-scene-v2-runtime-package -> validate-runtime-scene-package"
)
foreach ($workflowDoc in @(
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" }
)) {
    foreach ($requiredText in $authoredRuntimeWorkflowRequiredText) {
        if (-not $workflowDoc.Text.Contains($requiredText)) {
            Write-Error "$($workflowDoc.Label) did not contain expected text: $requiredText"
        }
    }
}
$runtimeScenePackageValidationRequiredText = @(
    "validate-runtime-scene-package",
    "plan_runtime_scene_package_validation",
    "execute_runtime_scene_package_validation",
    "non-mutating runtime scene package validation"
)
foreach ($runtimeSceneValidationDoc in @(
    @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" }
)) {
    foreach ($requiredText in $runtimeScenePackageValidationRequiredText) {
        if (-not $runtimeSceneValidationDoc.Text.Contains($requiredText)) {
            Write-Error "$($runtimeSceneValidationDoc.Label) did not contain expected text: $requiredText"
        }
    }
}
foreach ($forbiddenScenePrefabAuthoringClaim in @(
    "Scene/Prefab v2 authoring makes Scene v2 runtime package migration ready",
    "Scene/Prefab v2 authoring alone makes Scene v2 package migration ready",
    "editor productization is ready",
    "nested prefab merge/resolution UX is ready",
    "arbitrary free-form scene edits are supported",
    "arbitrary free-form prefab edits are supported",
    "Scene/Prefab v2 authoring runs arbitrary shell"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenScenePrefabAuthoringClaim)) {
            Write-Error "$($doc.Label) contains forbidden Scene/Prefab v2 authoring claim: $forbiddenScenePrefabAuthoringClaim"
        }
    }
}
foreach ($forbiddenSceneMigrationClaim in @(
    "migrate-scene-v2-runtime-package executes external importers",
    "Scene v2 runtime package migration executes external importers",
    "migrate-scene-v2-runtime-package cooks dependent assets",
    "Scene v2 runtime package migration cooks dependent assets",
    "migrate-scene-v2-runtime-package performs broad package cooking",
    "Scene v2 runtime package migration performs broad package cooking",
    "migrate-scene-v2-runtime-package makes renderer/RHI residency ready",
    "Scene v2 runtime package migration makes renderer/RHI residency ready",
    "migrate-scene-v2-runtime-package makes package streaming ready",
    "Scene v2 runtime package migration makes package streaming ready",
    "migrate-scene-v2-runtime-package supports material graphs",
    "Scene v2 runtime package migration supports material graphs",
    "migrate-scene-v2-runtime-package supports shader graphs",
    "Scene v2 runtime package migration supports shader graphs",
    "migrate-scene-v2-runtime-package supports live shader generation",
    "Scene v2 runtime package migration supports live shader generation",
    "migrate-scene-v2-runtime-package makes editor productization ready",
    "Scene v2 runtime package migration makes editor productization ready",
    "migrate-scene-v2-runtime-package makes Metal ready",
    "Scene v2 runtime package migration makes Metal ready",
    "migrate-scene-v2-runtime-package exposes public native/RHI handles",
    "Scene v2 runtime package migration exposes public native/RHI handles",
    "migrate-scene-v2-runtime-package makes general production renderer quality ready",
    "Scene v2 runtime package migration makes general production renderer quality ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenSceneMigrationClaim)) {
            Write-Error "$($doc.Label) contains forbidden Scene v2 runtime package migration claim: $forbiddenSceneMigrationClaim"
        }
    }
}
foreach ($forbiddenSourceAssetRegistrationClaim in @(
    "source asset registration executes external importers",
    "register-source-asset executes external importers",
    "source asset registration cooks runtime packages",
    "register-source-asset cooks runtime packages",
    "source asset registration makes renderer/RHI residency ready",
    "source asset registration makes package streaming ready",
    "source asset registration supports material graphs",
    "source asset registration supports shader graphs",
    "source asset registration supports live shader generation",
    "source asset registration makes editor productization ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenSourceAssetRegistrationClaim)) {
            Write-Error "$($doc.Label) contains forbidden source asset registration claim: $forbiddenSourceAssetRegistrationClaim"
        }
    }
}
foreach ($forbiddenRegisteredCookClaim in @(
    "cook-registered-source-assets performs broad dependency cooking",
    "cook-registered-source-assets cooks unselected dependencies",
    "cook-registered-source-assets makes renderer/RHI residency ready",
    "cook-registered-source-assets makes package streaming ready",
    "cook-registered-source-assets supports material graphs",
    "cook-registered-source-assets supports shader graphs",
    "cook-registered-source-assets supports live shader generation",
    "cook-registered-source-assets makes editor productization ready",
    "cook-registered-source-assets makes Metal ready",
    "cook-registered-source-assets exposes public native/RHI handles",
    "cook-registered-source-assets makes general production renderer quality ready",
    "cook-registered-source-assets executes arbitrary shell",
    "cook-registered-source-assets supports free-form edits"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenRegisteredCookClaim)) {
            Write-Error "$($doc.Label) contains forbidden registered source asset cook/package claim: $forbiddenRegisteredCookClaim"
        }
    }
}
foreach ($forbiddenAuthoredRuntimeWorkflowClaim in @(
    "authored-to-runtime workflow performs broad package cooking",
    "authored-to-runtime workflow cooks unselected dependencies",
    "authored-to-runtime workflow parses source assets at runtime",
    "authored-to-runtime workflow makes renderer/RHI residency ready",
    "authored-to-runtime workflow makes package streaming ready",
    "authored-to-runtime workflow supports material graphs",
    "authored-to-runtime workflow supports shader graphs",
    "authored-to-runtime workflow supports live shader generation",
    "authored-to-runtime workflow makes editor productization ready",
    "authored-to-runtime workflow makes Metal ready",
    "authored-to-runtime workflow exposes public native/RHI handles",
    "authored-to-runtime workflow makes general production renderer quality ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenAuthoredRuntimeWorkflowClaim)) {
            Write-Error "$($doc.Label) contains forbidden authored-to-runtime workflow claim: $forbiddenAuthoredRuntimeWorkflowClaim"
        }
    }
}
foreach ($forbiddenRuntimeSceneValidationClaim in @(
    "validate-runtime-scene-package performs package cooking",
    "validate-runtime-scene-package parses source assets at runtime",
    "validate-runtime-scene-package executes external importers",
    "validate-runtime-scene-package makes renderer/RHI residency ready",
    "validate-runtime-scene-package makes package streaming ready",
    "validate-runtime-scene-package supports material graphs",
    "validate-runtime-scene-package supports shader graphs",
    "validate-runtime-scene-package supports live shader generation",
    "validate-runtime-scene-package makes editor productization ready",
    "validate-runtime-scene-package makes Metal ready",
    "validate-runtime-scene-package exposes public native/RHI handles",
    "validate-runtime-scene-package makes general production renderer quality ready",
    "validate-runtime-scene-package executes arbitrary shell",
    "validate-runtime-scene-package supports free-form edits"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenRuntimeSceneValidationClaim)) {
            Write-Error "$($doc.Label) contains forbidden runtime scene package validation claim: $forbiddenRuntimeSceneValidationClaim"
        }
    }
}

$authoringSurfaceIds = @{}
foreach ($authoringSurface in $productionLoop.authoringSurfaces) {
    Assert-Properties $authoringSurface @("id", "status", "owner", "notes") "engine manifest aiOperableProductionLoop authoringSurfaces"
    if ($authoringSurfaceIds.ContainsKey($authoringSurface.id)) {
        Write-Error "engine manifest aiOperableProductionLoop authoring surface id is duplicated: $($authoringSurface.id)"
    }
    $authoringSurfaceIds[$authoringSurface.id] = $true
    if ($allowedProductionStatuses -notcontains $authoringSurface.status) {
        Write-Error "engine manifest aiOperableProductionLoop authoring surface '$($authoringSurface.id)' has invalid status: $($authoringSurface.status)"
    }
}
$sceneAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "scene-component-prefab-schema-v2" })
if ($sceneAuthoringSurface.Count -ne 1 -or $sceneAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop authoring surface scene-component-prefab-schema-v2 must be ready as a contract-only MK_scene surface"
}
if (-not ([string]$sceneAuthoringSurface[0].notes).Contains("Contract-only") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("SceneNodePrefabSourceV2") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("SceneComponentPrefabSourceV2") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("prefab_source") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("ScenePrefabInstanceRefreshPlanV2") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("plan_scene_prefab_instance_refresh_v2") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("ScenePrefabInstanceRefreshResultV2") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("apply_scene_prefab_instance_refresh_v2") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("duplicate_prefab_source_identity") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("unsupported_nested_prefab_instance") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("unsupported_local_prefab_child") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("unsupported_local_prefab_component") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("source_node_id") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("source_component_id") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("nested prefab propagation/merge resolution UX") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("2D/3D vertical slices")) {
    Write-Error "engine manifest scene-component-prefab-schema-v2 authoring surface must keep contract-only follow-up limits explicit"
}
$assetIdentityAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "asset-identity-v2" })
if ($assetIdentityAuthoringSurface.Count -ne 1 -or $assetIdentityAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop authoring surface asset-identity-v2 must be ready as a foundation-only MK_assets surface"
}
if (-not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("Foundation-only") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("GameEngine.AssetIdentity.v2") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("plan_asset_identity_placements_v2") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("Reviewed command-owned apply surfaces") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("placement_rows") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("ContentBrowserState") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("SourceAssetRegistryDocumentV1") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("ContentBrowserState::refresh_from") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("content_browser_import.assets") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("GameEngine.Project.v4 project.source_registry") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("refresh_content_browser_from_project_source_registry") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("Reload Source Registry") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("audit_runtime_scene_asset_identity") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("AssetKeyV2 key-first") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("tools/new-game.ps1") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("runtime source registry parsing") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("2D/3D vertical slices")) {
    Write-Error "engine manifest asset-identity-v2 authoring surface must keep foundation-only follow-up limits explicit"
}
$uiAtlasAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "ui-atlas-metadata-authoring-tooling-v1" })
if ($uiAtlasAuthoringSurface.Count -ne 1 -or $uiAtlasAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop authoring surface ui-atlas-metadata-authoring-tooling-v1 must be ready as a cooked-metadata-only MK_assets/MK_tools surface"
}
if (-not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("GameEngine.UiAtlas.v1") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("author_cooked_ui_atlas_metadata") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("verify_cooked_ui_atlas_package_metadata") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("author_packed_ui_atlas_from_decoded_images") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("author_packed_ui_glyph_atlas_from_rasterized_glyphs") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("RuntimeUiAtlasGlyph") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("GameEngine.CookedTexture.v1") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("renderer texture upload")) {
    Write-Error "engine manifest ui-atlas-metadata-authoring-tooling-v1 authoring surface must keep cooked metadata tooling, decoded/glyph atlas bridges, and renderer-upload limits explicit"
}
$assetPlaceholderAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "asset-placeholder-generation-v1" })
if ($assetPlaceholderAuthoringSurface.Count -ne 1 -or $assetPlaceholderAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop authoring surface asset-placeholder-generation-v1 must be ready as an MK_tools surface"
}
if (-not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("plan_placeholder_asset_bundle") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("plan_placeholder_asset_cook_package") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("placeholder_asset_tool.hpp") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetBundleRequest") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetBundlePlan") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetCookPackageRequest") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetCookPackagePlan") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetChangedFile") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetProvenanceRow") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("PlaceholderAssetDiagnostic") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("GameEngine.SourceAssetRegistry.v1") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("external asset downloader") -or
    -not ([string]$assetPlaceholderAuthoringSurface[0].notes).Contains("renderer/RHI residency")) {
    Write-Error "engine manifest asset-placeholder-generation-v1 authoring surface must keep placeholder contract and non-goals explicit"
}
$spriteAtlasAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "sprite-atlas-authoring-v1" })
if ($spriteAtlasAuthoringSurface.Count -ne 1 -or $spriteAtlasAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop authoring surface sprite-atlas-authoring-v1 must be ready as an MK_tools surface"
}
if (-not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("SpriteAtlasSourceFrameDesc") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("SpriteAtlasSourcePagePolicyDesc") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("SpriteAtlasSourcePivot") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("SpriteAtlasSourceSliceBorder") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("SpriteAtlasSourceAuthoringDesc") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("SpriteAtlasSourceAuthoringPlan") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("plan_sprite_atlas_source_authoring") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("sprite_atlas_tool.hpp") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("GameEngine.TextureSource.v1") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("GameEngine.SourceAssetRegistry.v1") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("single-page-tight-rgba8-texture-source") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("slice-border") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("renderer/RHI residency") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("package streaming") -or
    -not ([string]$spriteAtlasAuthoringSurface[0].notes).Contains("animation semantics")) {
    Write-Error "engine manifest sprite-atlas-authoring-v1 authoring surface must keep source atlas contract and non-goals explicit"
}
$runtimeGameplayDebugOverlayAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "runtime-gameplay-debug-overlay-v1" })
if ($runtimeGameplayDebugOverlayAuthoringSurface.Count -ne 1 -or
    $runtimeGameplayDebugOverlayAuthoringSurface[0].status -ne "ready" -or
    $runtimeGameplayDebugOverlayAuthoringSurface[0].owner -ne "MK_ui") {
    Write-Error "engine manifest aiOperableProductionLoop authoring surface runtime-gameplay-debug-overlay-v1 must be ready as an MK_ui surface"
}
if (-not ([string]$runtimeGameplayDebugOverlayAuthoringSurface[0].notes).Contains("RuntimeGameplayDebugOverlayRowDesc") -or
    -not ([string]$runtimeGameplayDebugOverlayAuthoringSurface[0].notes).Contains("RuntimeGameplayDebugOverlayCategory") -or
    -not ([string]$runtimeGameplayDebugOverlayAuthoringSurface[0].notes).Contains("RuntimeGameplayDebugOverlayRowKind") -or
    -not ([string]$runtimeGameplayDebugOverlayAuthoringSurface[0].notes).Contains("RuntimeGameplayDebugOverlayDiagnosticCode") -or
    -not ([string]$runtimeGameplayDebugOverlayAuthoringSurface[0].notes).Contains("RuntimeGameplayDebugOverlayPlan") -or
    -not ([string]$runtimeGameplayDebugOverlayAuthoringSurface[0].notes).Contains("plan_runtime_gameplay_debug_overlay") -or
    -not ([string]$runtimeGameplayDebugOverlayAuthoringSurface[0].notes).Contains("game-specific debug schema") -or
    -not ([string]$runtimeGameplayDebugOverlayAuthoringSurface[0].notes).Contains("renderer/RHI execution path")) {
    Write-Error "engine manifest runtime-gameplay-debug-overlay-v1 authoring surface must keep debug overlay contract and non-goals explicit"
}

foreach ($packageSurface in $productionLoop.packageSurfaces) {
    Assert-Properties $packageSurface @("id", "status", "targets", "validationRecipes", "notes") "engine manifest aiOperableProductionLoop packageSurfaces"
    if ($allowedProductionStatuses -notcontains $packageSurface.status) {
        Write-Error "engine manifest aiOperableProductionLoop package surface '$($packageSurface.id)' has invalid status: $($packageSurface.status)"
    }
    foreach ($target in @($packageSurface.targets)) {
        if (-not $packagingTargetNames.ContainsKey($target)) {
            Write-Error "engine manifest aiOperableProductionLoop package surface '$($packageSurface.id)' references unknown packaging target: $target"
        }
    }
    foreach ($validationRecipe in @($packageSurface.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop package surface '$($packageSurface.id)' references unknown validation recipe: $validationRecipe"
        }
    }
}

$requiredGapIds = @()
$gapIds = @{}
foreach ($gap in $productionLoop.unsupportedProductionGaps) {
    Assert-Properties $gap @("id", "oneDotZeroCloseoutTier", "status", "requiredBeforeReadyClaim", "notes") "engine manifest aiOperableProductionLoop unsupportedProductionGaps"
    $tier = [string]$gap.oneDotZeroCloseoutTier
    if (@("foundation-follow-up", "package-evidence", "closeout-wedge") -notcontains $tier) {
        Write-Error "engine manifest aiOperableProductionLoop unsupported gap '$($gap.id)' has invalid oneDotZeroCloseoutTier '$tier'"
    }
    $gapIds[$gap.id] = $true
    if ($gap.status -eq "ready") {
        Write-Error "engine manifest aiOperableProductionLoop unsupported gap '$($gap.id)' must not be ready"
    }
}
foreach ($gapId in $requiredGapIds) {
    if (-not $gapIds.ContainsKey($gapId)) {
        Write-Error "engine manifest aiOperableProductionLoop missing unsupported gap id: $gapId"
    }
}
$sceneSchemaGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "scene-component-prefab-schema-v2" })
if ($sceneSchemaGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop scene-component-prefab-schema-v2 gap must leave unsupportedProductionGaps after foundation closeout"
}
$playable2dGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "2d-playable-vertical-slice" })
if ($playable2dGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop 2d-playable-vertical-slice gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$playable3dGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "3d-playable-vertical-slice" })
if ($playable3dGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop 3d-playable-vertical-slice gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$editorProductizationGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "editor-productization" })
if ($editorProductizationGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop editor-productization gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$productionUiImporterPlatformGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "production-ui-importer-platform-adapters" })
if ($productionUiImporterPlatformGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop production-ui-importer-platform-adapters gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$fullRepoQualityGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "full-repository-quality-gate" })
if ($fullRepoQualityGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop full-repository-quality-gate gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$assetIdentityGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "asset-identity-v2" })
if ($assetIdentityGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop asset-identity-v2 gap must leave unsupportedProductionGaps after reference cleanup closeout"
}
$runtimeResourceGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "runtime-resource-v2" })
if ($runtimeResourceGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop runtime-resource-v2 gap must leave unsupportedProductionGaps after 1.0 scope closeout"
}
$recommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
if ([string]$productionLoop.recommendedNextPlan.id -eq "general-purpose-game-production-v1") {
    foreach ($needle in @(
        "General Purpose Game Production v1",
        "gameplay-runtime-scheduler-production-v1",
        "world-entity-model-production-v1",
        "addressable-content-streaming-production-v1",
        "production-authoring-workflows-v1",
        "production-runtime-ui-workbench-v1",
        "unsupportedProductionGaps empty"
    )) {
        Assert-ContainsText $recommendedText $needle "engine manifest aiOperableProductionLoop recommendedNextPlan production milestone"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "generated-game-studio-v1") {
    foreach ($needle in @(
        "Generated Game Studio v1",
        "EditorAiGeneratedGameStudioV1Model",
        "EditorAiCommandPanelModel",
        "ai-generated-game-playtest-loop-v1",
        "ai-validation-remediation-recipes-v1",
        "unsupportedProductionGaps empty"
    )) {
        Assert-ContainsText $recommendedText $needle "engine manifest aiOperableProductionLoop recommendedNextPlan generated game studio milestone"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "engine-1-0-gap-matrix-v1") {
    foreach ($needle in @(
        "Engine 1.0 Gap Matrix v1",
        "Generated Game Studio v1",
        "implemented-1x-foundation",
        "renderer-backend-parity-v1",
        "strict Vulkan evidence",
        "Metal remains Apple-host-gated",
        "unsupportedProductionGaps empty",
        "broad commercial-engine"
    )) {
        Assert-ContainsText $recommendedText $needle "engine manifest aiOperableProductionLoop recommendedNextPlan engine gap matrix"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "next-production-gap-selection") {
    foreach ($needle in @("First-Party Desktop Platform And SDL3 Removal v1", "MK_platform_win32", "MK_runtime_host_win32", "MK_runtime_host_win32_presentation", "MK_audio_wasapi", "Job Execution Placement Policy v1", "job_execution_placement_policy_status=ready", "host-independent CPU placement policy evidence", "Windows CPU Set Worker Placement v1", "windows_cpu_set_worker_placement_status=ready", "windows_cpu_set_worker_placement_native_thread_handles_exposed=0", "unsupportedProductionGaps = []", "selection gate")) {
        if (-not $recommendedText.Contains($needle)) {
            Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe SDL3 removal closeout and selection gate: $needle"
        }
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "physics-navigation-commercial-coverage-v1") {
    foreach ($needle in @(
    "Physics Navigation Commercial Coverage v1",
    "Jolt/Recast/Detour-class",
    "adapter_boundary_id",
    "host_validation_recipe_id",
    "adapter_lifecycle_reviewed",
    "unsupportedProductionGaps = []",
    "native handles hidden",
    "broad middleware parity fail-closed"
    )) {
        if (-not $recommendedText.Contains($needle)) {
            Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe physics/navigation commercial coverage selection: $needle"
        }
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "renderer-backend-parity-metal-apple-evidence-v1") {
    foreach ($needle in @(
    "Renderer Backend Parity Metal Apple Evidence v1",
    "renderer-backend-parity-v1",
    "metal-apple remains host-gated",
    "shader-toolchain",
    "mobile-packaging",
    "ios-simulator-smoke",
    "Apple/Metal host evidence",
    "Windows/Vulkan proof must not promote Metal readiness",
    "no SDL3",
    "native handles remain hidden",
    "unsupportedProductionGaps = []"
    )) {
        if (-not $recommendedText.Contains($needle)) {
            Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe renderer Metal Apple selection: $needle"
        }
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "renderer-backend-parity-apple-evidence-closeout-v1") { foreach ($needle in @("Renderer Backend Parity Apple Evidence Closeout v1", "renderer-backend-parity-v1", "renderer-metal-apple-host-evidence", "Apple/Metal host evidence", "Windows/Vulkan proof must not promote Metal readiness", "native handles remain hidden", "unsupportedProductionGaps = []")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe renderer backend parity Apple evidence closeout selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "renderer-postprocess-tone-mapping-evidence-v1") {
    foreach ($needle in @(
    "Renderer Postprocess Tone Mapping Evidence v1",
    "renderer-postprocess-v1",
    "PostprocessToneMappingEvidencePlan",
    "plan_postprocess_tone_mapping_evidence",
    "D3D12/Vulkan",
    "Metal host-gated",
    "no SDL3",
    "native handles hidden",
    "subjective visual quality",
    "unsupportedProductionGaps = []"
    )) {
        if (-not $recommendedText.Contains($needle)) {
            Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe renderer postprocess tone-mapping selection: $needle"
        }
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "sandbox-world-network-modding-gate-v1") {
    foreach ($needle in @(
    "Selected focused child plan",
    "sandbox-world-specific mutation replication",
    "reviewed modding policy gates",
    "unsupportedProductionGaps = []",
    "Broad online multiplayer",
    "SDL3",
    "native handle exposure"
    )) {
        if (-not $recommendedText.Contains($needle)) {
            Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe sandbox world network/modding selection: $needle"
        }
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "sandbox-world-package-validation-performance-budgets-v1") {
    foreach ($needle in @(
    "Selected focused child plan",
    "sample package smoke flags",
    "installed validation",
    "package-visible counters",
    "--require-sandbox-package-budgets",
    "sandbox_package_budget_*",
    "unsupportedProductionGaps = []",
    "broad renderer quality",
    "package mutation",
    "SDL3",
    "native handle exposure"
    )) {
        if (-not $recommendedText.Contains($needle)) {
            Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe sandbox world package validation and performance budget selection: $needle"
        }
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "ai-operable-performance-budget-and-evidence-v1") { foreach ($needle in @("performanceBudgets", "budgetRows", "evidenceRows", "validation recipe", "unsupported broad optimization claims", "CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe performance budget evidence selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "performance-baseline-v1") { foreach ($needle in @("reproducible benchmark scenes/packages", "trace export recipes", "subsystem counters", "p95/p99 frame budget reporting", "broad CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe performance baseline selection: $needle" } } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "long-running-performance-readiness-v1-phase-1") { foreach ($needle in @("Long-Running Performance Readiness v1", "sample_2d_desktop_runtime_package", "--require-long-run-performance-readiness", "long_run_readiness_status=ready", "host-2d-long-run-readiness-soak", "Linux affinity", "NUMA", "broad SIMD", "GPU async overlap", "CUDA", "HIP", "SYCL", "unsupportedProductionGaps = []")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe long-running performance readiness selection: $needle" } } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "long-running-performance-readiness-v1-phase-2") { foreach ($needle in @("Intel/AMD CPU Profiling Matrix v1", "cpuProfilingMatrix", "host-cpu-profiling-matrix", "representative Intel/AMD host classes", "before/after trace", "regression budgets", "Linux affinity", "NUMA", "broader SIMD", "PGO/LTO", "data-layout", "host-gated", "unsupportedProductionGaps = []")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe CPU profiling matrix selection: $needle" } } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "long-running-performance-readiness-v1-phase-7") { foreach ($needle in @("Optional GPU Compute Review v1", "optionalGpuComputeReview", "host-optional-gpu-compute-review", "rhi_compute", "offline_tool_acceleration", "cuda_hip_private_adapter_candidate", "sycl_private_adapter_candidate", "non_goal", "data transfer cost", "memory residency", "synchronization", "stream/event usage", "queue/profiler visibility", "dependency burden", "scalar or RHI fallback", "CUDA/HIP/SYCL runtime dependency", "vcpkg.json", "CMake", "default validation", "broad GPU compute", "async overlap", "cross-vendor", "cross-backend", "broad CPU/GPU/memory optimization", "unsupportedProductionGaps = []")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe optional GPU compute review selection: $needle" } } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "memory-lifetime-taxonomy-v1") { foreach ($needle in @("memory lifetime taxonomy", "ownership semantics", "raw pointers are non-owning", "std::unique_ptr", "std::span", "allocator/job/NUMA/GPU memory", "broad CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe memory lifetime taxonomy selection: $needle" } } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "memory-diagnostics-v1") { foreach ($needle in @("memory diagnostics", "memory class counters", "high-water marks", "budget pressure", "stale-generation", "use-after-safe-point", "allocator replacement", "broad CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe memory diagnostics selection: $needle" } } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "frame-thread-scratch-v1") { foreach ($needle in @("frame temporary", "worker scratch", "first-party frame arenas", "per-worker scratch arenas", "explicit ownership APIs", "high-water marks", "false-sharing diagnostics", "allocator replacement", "all-core CPU scheduling", "broad CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe frame/thread scratch selection: $needle" } } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "job-scheduling-evidence-v1") { foreach ($needle in @("job scheduling", "worker topology", "bounded job queues", "deterministic job/scratch evidence", "queue/steal/wait/merge diagnostics", "work stealing", "processor groups", "package-visible job_scheduling_evidence_* counters", "all-core CPU scheduling", "affinity pinning", "NUMA placement", "SIMD dispatch", "broad CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe job scheduling evidence selection: $needle" } } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "job-execution-worker-pool-v1") { foreach ($needle in @("Job Execution Worker Pool v1", "persistent worker-thread pool", "explicit worker_count", "bounded worker queues", "std::thread", "JobExecutionStopToken", "execute(batch)", "worker-local ScratchArena", "deterministic publish order", "JobSchedulingExecutionEvidence", "unsupportedProductionGaps = []", "broad CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe job execution worker pool selection: $needle" } } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "job-execution-topology-policy-v1") { foreach ($needle in @("Job Execution Topology Policy v1", "portable MK_core worker-count selection", "JobExecutionTopologyPolicyDesc", "select_job_execution_topology_policy", "observe_job_execution_logical_processor_count", "derived JobExecutionPoolDesc", "fallback/cap/reserve rules", "processor-group and NUMA host-evidence diagnostics", "--require-job-execution-topology-policy", "job_execution_topology_policy_status=ready", "job_execution_topology_policy_ready=1", "selected worker count = 2", "unsupportedProductionGaps = []", "broad all-core CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe job execution topology policy selection: $needle" } } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "job-execution-work-stealing-v1") { foreach ($needle in @("Job Execution Work Stealing v1", "opt-in JobExecutionPool", "work_stealing_enabled", "JobExecutionTopologyPolicyDesc.enable_work_stealing", "steal attempt/success/wait counters", "--require-job-execution-work-stealing", "job_execution_work_stealing_status=ready", "job_execution_work_stealing_applied=1", "deterministic publish order", "unsupportedProductionGaps = []", "affinity", "NUMA", "SMT/hybrid", "SIMD", "GPU async overlap", "CUDA/HIP/SYCL", "broad all-core CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe job execution work stealing selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "job-execution-placement-policy-v1") { foreach ($needle in @("Job Execution Placement Policy v1", "host-independent CPU placement policy evidence", "topology", "worker-pool", "work-stealing", "affinity", "NUMA", "SMT", "hybrid-core", "Windows CPU Set", "Linux affinity", "SIMD", "GPU async", "CUDA/HIP/SYCL", "broad all-core CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe job execution placement policy selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "windows-cpu-set-worker-placement-v1") { foreach ($needle in @("Windows CPU Set Worker Placement v1", "Windows CPU Sets", "worker-start placement", "value-only MK_core callback", "MK_platform_win32 adapter", "sample_desktop_runtime_game", "unsupportedProductionGaps = []", "native handles", "Linux affinity", "NUMA allocation execution", "hybrid P-core/E-core", "SMT scheduling", "SIMD dispatch", "GPU async overlap", "CUDA/HIP/SYCL", "broad CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe Windows CPU Set worker placement selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "windows-cpu-set-smt-worker-placement-v1") { foreach ($needle in @("Windows CPU Set SMT Worker Placement v1", "avoid_smt_siblings", "CoreIndex", "distinct cores", "SMT sibling", "--require-windows-cpu-set-smt-worker-placement", "windows_cpu_set_smt_worker_placement_status=ready", "smt_policy_applied=1", "unsupportedProductionGaps = []", "native handles", "hybrid P-core/E-core", "Linux affinity", "NUMA allocation execution", "SIMD dispatch", "GPU async overlap", "CUDA/HIP/SYCL", "broad CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe Windows CPU Set SMT worker placement selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "avx2-reviewed-target-execution-v1") { foreach ($needle in @("AVX2 Reviewed Target Execution v1", "target-local AVX2 OBJECT", "mirakana_core_avx2", "simd_dispatch_avx2.cpp", "CpuSimdFeatureSet", "avx2_compile_supported", "avx2_runtime_supported", "auto_select", "sample_desktop_runtime_game --require-simd-dispatch-policy", "unsupportedProductionGaps = []", "global /arch:AVX2", "NUMA allocation execution", "GPU async overlap", "CUDA/HIP/SYCL", "broad CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe AVX2 reviewed target execution selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "simd-dispatch-policy-and-evidence-v1") { foreach ($needle in @("SIMD Dispatch Policy And Evidence v1", "scalar/SSE2", "CPU SIMD dispatch", "simd_dispatch_policy_*", "Intel and AMD x86/x64", "AVX2 behind compile/runtime gates", "span-based inputs", "raw pointers non-owning", "unsupportedProductionGaps = []", "ARM NEON", "NUMA allocation execution", "GPU async overlap", "CUDA/HIP/SYCL", "broad CPU/GPU/memory optimization")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe SIMD dispatch policy selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "environment-system-v1") { foreach ($needle in @("Environment System v1", "MK_environment", "EnvironmentProfileDesc", "validate_environment_profile", "official docs/Context7", "sky", "sun/moon", "fog", "clouds", "rain/snow/storm", "time-of-day", "quality tiers", "D3D12", "strict Vulkan", "Metal host-gated", "unsupportedProductionGaps = []", "broad environment_ready", "native handles", "Dear ImGui", "SDL3", "OpenEXR/KTX")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe environment system selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "environment-rendering-readiness-v1") { foreach ($needle in @("Environment Rendering Readiness v1", "Environment System v1", "official docs/Context7", "snow package readiness", "physical-sky package/Vulkan proof", "cloud/precipitation renderer execution", "volumetric-cloud execution/package readiness", "height/volumetric-fog package proof", "environment lighting/IBL package proof", "later renderer upload/runtime-capture proof", "D3D12", "strict Vulkan", "Metal host-gated", "broad optimization", "broad environment_ready", "native handles", "Dear ImGui", "SDL3", "unsupportedProductionGaps = []")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe environment rendering readiness selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "environment-production-excellence-v1") { foreach ($needle in @("Environment Production Excellence v1", "Phase 2 is complete", "Phase 4 is complete", "Phase 5 precipitation candidate", "Phase 5 physical-sky package candidate", "EnvironmentProfile.v2", "local environment volumes", "runtime cubemap capture", "renderer IBL upload", "material wetness", "snow accumulation", "weather audio playback", "desktop-runtime-sample-game-environment-audio-playback", "environment_audio_playback_status=ready", "environment_audio_trigger_rows=4", "environment_precipitation_audio_playback=0", "desktop-runtime-sample-game-vulkan-environment-precipitation-renderer-execution", "environment_precipitation_vulkan_status=ready", "desktop-runtime-sample-game-vulkan-physical-sky-package", "environment_physical_sky_vulkan_status=ready", "physical WASAPI endpoint playback", "strict Vulkan", "Apple-host Metal", "quality budgets", "exact environment_ready", "official docs/Context7", "OpenEXR/KTX/Basis asset pipeline", "broad renderer quality", "broad CPU/GPU/memory optimization", "all-platform unconditional parity", "unsupportedProductionGaps = []", "Dear ImGui", "SDL3", "public native handles", "no inferred Vulkan/Metal readiness", "no broad optimization", "no broad environment_ready")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe environment production excellence selection: $needle" } } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "environment-highest-commercial-readiness-v1") { foreach ($needle in @("Environment Highest Commercial Readiness v1", "official docs/Context7", "Source gate ready", "environment_highest_readiness_context7_verified_rows=9", "environment_highest_readiness_context7_partial_rows=2", "environment_highest_readiness_official_fallback_rows=2", "environment_highest_readiness_authoritative_doc_ready_rows=11", "environment_highest_readiness_source_gate_status=ready", "environment_highest_readiness_code_edit_allowed=1", "official_fallback.apple_metal_framework_api=ready", "official_fallback.khronos_gltf_spec=ready", "strict Vulkan", "Apple Metal", "backend parity v2", "all-platform readiness", "broad measured optimization", "AAA preset asset library", "full OpenEXR/KTX2/Basis asset pipeline", "physical weather simulation", "production artist workflow", "commercial aggregate", "broad environment_ready", "unsupportedProductionGaps = []")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe environment highest commercial readiness selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "mavg-research-legal-benchmark-baseline-v1") { foreach ($needle in @("MAVG Phase 0", "research/specification", "official-source checks", "clean-room/legal guardrails", "benchmark methodology", "stale-doc cleanup", "MAVG not implemented", "no SDL3/Dear ImGui", "no public native handles", "no Nanite/UE compatibility", "unsupportedProductionGaps = []")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe MAVG Phase 0 selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "mavg-asset-graph-v1") { foreach ($needle in @("MAVG Asset Graph v1", "MAVG Phase 0 completed", "deterministic MK_assets MAVG cluster graph validation", "mavg_cluster_graph.hpp", "MavgClusterGraphDocument", "GameEngine.MavgClusterGraph.v1", "AssetKind::mavg_cluster_graph", "mavg_source_mesh", "mavg_material", "MK_tools cook/package planning", "mavg_cluster_cook.hpp", "MavgClusterCookRequest", "plan_mavg_cluster_graph_cook_package", "apply_mavg_cluster_graph_cook_package", "no SDL3/Dear ImGui", "no public native handles", "no Nanite/UE compatibility", "CPU selection", "renderer execution", "streaming/residency", "unsupportedProductionGaps = []")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe MAVG asset graph selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "mavg-runtime-lod-milestone-v1") { foreach ($needle in @("MAVG Runtime LOD Milestone v1", "MAVG Asset Graph v1 merged through PR #516", "MAVG runtime LOD graph", "MavgClusterGraphCluster", "parent_cluster_index", "resident_fallback_cluster_index", "geometric_error", "first_index", "index_count", "vertex_base", "GameEngine.MavgClusterGraph.v1", "hierarchy/error/fallback graph rows", "draw-ready static payload rows", "MavgClusterCookVertex", "GameEngine.MavgClusterPayload.v1", "vertex.data_hex", "index.data_hex", "per-material root/leaf", "CPU select_mavg_lod_clusters", "CPU LOD selection", "mavg_lod_selection.hpp", "MavgLodViewDesc", "MavgLodResidentPageSet", "MavgLodPreviousSelection", "MavgLodSelectedCluster", "MavgLodSelectionResult", "MK_mavg_lod_selection_tests", "MavgLodPageRequest", "runtime resident-page evidence", "mavg_lod_residency.hpp", "RuntimeMavgLodResidencyDesc", "RuntimeMavgLodResidencyResult", "build_runtime_mavg_lod_residency", "MK_runtime_mavg_lod_residency_tests", "range-aware conventional indexed draws", "MeshIndexedDrawRange", "rhi::IRhiCommandList::draw_indexed", "MeshCommand submission", "mavg_scene_lod.hpp", "plan_mavg_scene_lod_mesh_commands", "GPU culling", "indirect draw execution", "mesh shaders", "package streaming execution", "deformation", "ray tracing", "Metal readiness", "Nanite compatibility/equivalence/superiority", "broad CPU/GPU/memory optimization", "unsupportedProductionGaps = []")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe MAVG runtime LOD selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "mavg-page-streaming-eviction-review-v1") { foreach ($needle in @("MAVG Page Streaming Eviction Review v1", "RuntimeMavgResidentPageMountRow", "RuntimeMavgPageStreamingSelectedClusterRow", "RuntimeMavgPageStreamingEvictionReviewResult", "review_runtime_mavg_page_streaming_evictions", "plan_runtime_resident_package_evictions_v2", "selected/fallback page protection", "automatic eviction policy", "autonomous/background package streaming execution", "Nanite compatibility/equivalence/superiority", "broad CPU/GPU/memory optimization", "unsupportedProductionGaps = []")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe MAVG page streaming eviction review selection: $needle" } }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "first-party-ui-editor-production-stack-v1") {
    foreach ($needle in @("First-Party UI Editor Production Stack v1", "MK_editor", "MK_editor_core", "desktop-editor", "mirakana::ui", "MK_ui_renderer", "dock graph", "rich text", "DirectWrite", "Text Services Framework", "UI Automation", "D3D12 viewport/material texture display", "AI-operable", "compatibility shims", "unsupportedProductionGaps = []", "SDL3", "native handles", "Dear ImGui")) {
        if (-not $recommendedText.Contains($needle)) {
            Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe first-party UI editor production selection: $needle"
        }
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "first-party-editor-shell-v1") {
    foreach ($needle in @(
    "First-Party Editor Shell v1",
    "MK_editor",
    "MK_editor_core",
    "first-party retained",
    "desktop-editor",
    "mirakana::ui",
    "MK_ui_renderer",
    "EditorAiOperationSnapshot",
    "EditorAiCommandCatalog",
    "EditorAiCommandRequest",
    "EditorAiCommandDryRunResult",
    "EditorAiCommandApplyResult",
    "dock graph",
    "rich-text",
    "DirectWrite",
    "Text Services Framework",
    "UI Automation",
    "unsupportedProductionGaps = []",
    "SDL3",
    "native handles",
    "Dear ImGui"
    )) {
        if (-not $recommendedText.Contains($needle)) {
            Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe first-party editor shell selection: $needle"
        }
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "native-win32-editor-shell-v1") {
    foreach ($needle in @(
    "Native Win32 Editor Shell v1",
    "MK_editor",
    "Dear ImGui",
    "Direct3D 12",
    "desktop-gui",
    "PR #316",
    "PR #318",
    "PR #320",
    "editor_shell_panels=11",
    "editor_shell_viewport_status=d3d12_texture_ready",
    "editor_shell_viewport_visible_texture_composites",
    "unsupportedProductionGaps = []",
    "SDL3",
    "native handles",
    "editor/developer-shell"
    )) {
        if (-not $recommendedText.Contains($needle)) {
            Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe native editor shell selection: $needle"
        }
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "mavg-win32-directstorage-sdk-adapter-v1") { $mavgWin32DirectStorageRecommendedText = $productionLoop.recommendedNextPlan | ConvertTo-Json -Depth 8; foreach ($needle in @("MAVG Win32 DirectStorage SDK Adapter v1", "MAVG DirectStorage Page IO Execution v1", "Microsoft DirectStorage SDK 1.3.0", "vcpkg port dstorage 1.3.0", "find_package(dstorage CONFIG REQUIRED)", "Microsoft::DirectStorage", "dstorage.h", "DStorageGetFactory", "IDStorageFactory", "IDStorageQueue", "IDStorageStatusArray", "IDStorageQueue::EnqueueRequest", "IDStorageQueue::EnqueueStatus", "IDStorageQueue::Submit", "IDStorageStatusArray::GetHResult", "MK_platform_win32", "IByteRangeIoExecutor", "ByteRangeIoBackendKind::direct_storage", "first-party Win32 DirectStorage SDK adapter", "public native handles", "GPU destinations", "GDeflate", "DirectStorage 1.4 preview APIs", "async-overlap/performance proof", "mesh shaders", "Metal readiness", "Nanite", "broad optimization", "unsupportedProductionGaps = []")) { if (-not $mavgWin32DirectStorageRecommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe MAVG Win32 DirectStorage SDK adapter selection: $needle" } }
} else { foreach ($needle in @("Frame Graph Transient Texture Alias Planning v1", "FrameGraphTransientTextureAliasPlan", "plan_frame_graph_transient_texture_aliases", "Frame Graph Shadow Scratch Color Target-State Ownership v1", "shadow_color", "6 pass callbacks/15 barrier steps", "Frame Graph Viewport Surface Color State Executor v1", "RhiViewportSurface", "viewport_color", "Frame Graph Texture Aliasing Barrier Command v1", "record_frame_graph_texture_aliasing_barriers", "Frame Graph Automatic Aliasing Barrier Insertion v1", "Package Streaming Frame Graph Texture Binding Handoff v1", "make_runtime_package_streaming_frame_graph_texture_bindings", "Package Static Mesh Upload Binding Transaction v1", "upload_runtime_package_streaming_mesh_gpu_bindings", "Frame Graph Render Pass Envelope v1", "render_passes_recorded", "Frame Graph RHI Queue Dependency Plan v1", "plan_frame_graph_rhi_queue_waits", "IRhiDevice::wait_for_queue", "Frame Graph RHI Multi-Queue Executor v1", "execute_frame_graph_rhi_multi_queue_schedule", "Frame Graph RHI Multi-Queue Texture Barrier Execution v1", "FrameGraphRhiMultiQueueExecutionResult::barriers_recorded", "Frame Graph v1 1.0 Scope Closeout v1 closes frame-graph-v1", "upload-staging-v1", "native async upload execution", "package skinned/morph streaming", "staging-pool production adoption")) { if (-not $recommendedText.Contains($needle)) { Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe frame-graph closeout and upload-staging next gap: $needle" } } }
$rendererRhiGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "renderer-rhi-resource-foundation" })
if ($rendererRhiGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop renderer-rhi-resource-foundation gap must leave unsupportedProductionGaps after 1.0 scope closeout"
}
