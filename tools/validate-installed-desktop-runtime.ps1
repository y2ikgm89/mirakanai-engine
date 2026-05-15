#requires -Version 7.0
#requires -PSEdition Core

param(
    [string]$InstallPrefix = "",
    [string]$BuildConfig = "Release",
    [string]$GameTarget = "",
    [string[]]$SmokeArgs = @(),
    [switch]$RequireD3d12Shaders,
    [switch]$RequireVulkanShaders
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$tools = Assert-CppBuildTools

if ([string]::IsNullOrWhiteSpace($InstallPrefix)) {
    $InstallPrefix = Join-Path $root "out/install/desktop-runtime-release"
}
$installPrefixPath = [System.IO.Path]::GetFullPath($InstallPrefix)
$installedBin = Join-Path $installPrefixPath "bin"
$installedMetadataPath = Join-Path $installPrefixPath "share/Mirakanai/desktop-runtime-games.json"
$gameMetadata = $null
if (Test-Path -LiteralPath $installedMetadataPath -PathType Leaf) {
    $metadata = Read-DesktopRuntimeGameMetadata -Path $installedMetadataPath
    if ([string]::IsNullOrWhiteSpace($metadata.selectedPackageTarget)) {
        Write-Error "Installed desktop runtime metadata must declare selectedPackageTarget: $installedMetadataPath"
    }
    if ([string]::IsNullOrWhiteSpace($GameTarget)) {
        $GameTarget = $metadata.selectedPackageTarget
    } elseif ($metadata.selectedPackageTarget -ne $GameTarget) {
        Write-Error "Installed desktop runtime metadata selected '$($metadata.selectedPackageTarget)' but validation requested '$GameTarget'."
    }
    $gameMetadata = Get-DesktopRuntimeGameMetadata -Metadata $metadata -GameTarget $GameTarget
    Assert-DesktopRuntimeGameManifestContract `
        -GameMetadata $gameMetadata `
        -Root $installPrefixPath `
        -RequiredPackagingTargets @("desktop-game-runtime", "desktop-runtime-release") `
        -Installed | Out-Null
    Assert-DesktopRuntimeGamePackageFiles `
        -GameMetadata $gameMetadata `
        -Root $installPrefixPath `
        -Installed | Out-Null
}
if ([string]::IsNullOrWhiteSpace($GameTarget)) {
    Write-Error "GameTarget is required when installed desktop runtime metadata is unavailable."
}
if ($SmokeArgs.Count -eq 0 -and $null -ne $gameMetadata) {
    $SmokeArgs = @($gameMetadata.packageSmokeArgs)
}
if ($SmokeArgs.Count -eq 0) {
    Write-Error "SmokeArgs are required when installed desktop runtime metadata is unavailable or incomplete."
}
$requireD3d12ShaderArtifacts = $RequireD3d12Shaders.IsPresent -or ($null -ne $gameMetadata -and [bool]$gameMetadata.requiresD3d12Shaders)
$requireVulkanShaderArtifacts = $RequireVulkanShaders.IsPresent -or ($null -ne $gameMetadata -and $gameMetadata.PSObject.Properties.Name.Contains("requiresVulkanShaders") -and [bool]$gameMetadata.requiresVulkanShaders)
$runtimeExecutableName = $GameTarget
if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
    $runtimeExecutableName = "$GameTarget.exe"
}
$runtimeExecutable = Join-Path $installedBin $runtimeExecutableName
$sdlDll = Join-Path $installedBin "SDL3.dll"

function Invoke-InstalledRuntimeSmoke {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [string[]]$Arguments = @()
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.WorkingDirectory = (Get-Location).Path
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in $Arguments) {
        $startInfo.ArgumentList.Add($argument) | Out-Null
    }
    $startInfo.Environment.Clear()
    foreach ($entry in Get-NormalizedProcessEnvironment) {
        $startInfo.Environment[$entry.Key] = $entry.Value
    }

    $process = [System.Diagnostics.Process]::Start($startInfo)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    if (-not [string]::IsNullOrWhiteSpace($stdout)) {
        Write-Host $stdout.TrimEnd()
    }
    if (-not [string]::IsNullOrWhiteSpace($stderr)) {
        Write-Host $stderr.TrimEnd()
    }
    if ($process.ExitCode -ne 0) {
        Write-Error "Command failed with exit code $($process.ExitCode): $FilePath $($Arguments -join ' ')"
    }
    return $stdout
}

if (-not (Test-Path -LiteralPath $runtimeExecutable -PathType Leaf)) {
    Write-Error "Installed desktop runtime game was not found: $runtimeExecutable"
}

if ($requireD3d12ShaderArtifacts) {
    $shaderArtifacts = @()
    if ($null -ne $gameMetadata -and $gameMetadata.PSObject.Properties.Name.Contains("d3d12ShaderArtifacts")) {
        $shaderArtifacts = @($gameMetadata.d3d12ShaderArtifacts | ForEach-Object { [string]$_ })
    }
    if ($shaderArtifacts.Count -eq 0) {
        $shaderArtifacts = @("shaders/runtime_shell.vs.dxil", "shaders/runtime_shell.ps.dxil")
    }

    foreach ($shaderArtifact in $shaderArtifacts) {
        $normalizedShaderArtifact = ConvertTo-DesktopRuntimeMetadataRelativePath `
            -Path $shaderArtifact `
            -Description "Installed desktop runtime D3D12 shader artifact"
        if (-not $normalizedShaderArtifact.StartsWith("shaders/", [System.StringComparison]::Ordinal)) {
            Write-Error "Installed desktop runtime D3D12 shader artifact must stay under shaders/: $normalizedShaderArtifact"
        }
        $shaderPath = Join-Path $installedBin $normalizedShaderArtifact.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
        if (-not (Test-Path -LiteralPath $shaderPath -PathType Leaf)) {
            Write-Error "Installed desktop runtime D3D12 shader artifact was not found: $shaderPath"
        }
        if ((Get-Item -LiteralPath $shaderPath).Length -le 0) {
            Write-Error "Installed desktop runtime D3D12 shader artifact is empty: $shaderPath"
        }
    }
}

if ($requireVulkanShaderArtifacts) {
    $shaderArtifacts = @()
    if ($null -ne $gameMetadata -and $gameMetadata.PSObject.Properties.Name.Contains("vulkanShaderArtifacts")) {
        $shaderArtifacts = @($gameMetadata.vulkanShaderArtifacts | ForEach-Object { [string]$_ })
    }
    if ($shaderArtifacts.Count -eq 0) {
        $shaderArtifacts = @("shaders/runtime_shell.vs.spv", "shaders/runtime_shell.ps.spv")
    }

    foreach ($shaderArtifact in $shaderArtifacts) {
        $normalizedShaderArtifact = ConvertTo-DesktopRuntimeMetadataRelativePath `
            -Path $shaderArtifact `
            -Description "Installed desktop runtime Vulkan shader artifact"
        if (-not $normalizedShaderArtifact.StartsWith("shaders/", [System.StringComparison]::Ordinal)) {
            Write-Error "Installed desktop runtime Vulkan shader artifact must stay under shaders/: $normalizedShaderArtifact"
        }
        $shaderPath = Join-Path $installedBin $normalizedShaderArtifact.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
        if (-not (Test-Path -LiteralPath $shaderPath -PathType Leaf)) {
            Write-Error "Installed desktop runtime Vulkan shader artifact was not found: $shaderPath"
        }
        if ((Get-Item -LiteralPath $shaderPath).Length -le 0) {
            Write-Error "Installed desktop runtime Vulkan shader artifact is empty: $shaderPath"
        }
    }
}

if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
    if (-not (Test-Path -LiteralPath $sdlDll -PathType Leaf)) {
        Write-Error "Installed SDL3 runtime DLL was not found: $sdlDll"
    }
}

$env:PATH = (($installedBin, $env:PATH) -join [System.IO.Path]::PathSeparator)
$smokeOutput = Invoke-InstalledRuntimeSmoke -FilePath $runtimeExecutable -Arguments @($SmokeArgs)
$requiresPostprocessDepthInput = @($SmokeArgs) -contains "--require-postprocess-depth-input"
$requiresDirectionalShadow = @($SmokeArgs) -contains "--require-directional-shadow"
$requiresDirectionalShadowFiltering = @($SmokeArgs) -contains "--require-directional-shadow-filtering"
$requiresShadowMorphComposition = @($SmokeArgs) -contains "--require-shadow-morph-composition"
$requiresRendererQualityGates = @($SmokeArgs) -contains "--require-renderer-quality-gates"
$requiresPlayable3dSlice = @($SmokeArgs) -contains "--require-playable-3d-slice"
$requiresVisible3dProductionProof = @($SmokeArgs) -contains "--require-visible-3d-production-proof"
$requiresComputeMorphNormalTangent = @($SmokeArgs) -contains "--require-compute-morph-normal-tangent"
$requiresComputeMorphSkin = @($SmokeArgs) -contains "--require-compute-morph-skin"
$requiresComputeMorphAsyncTelemetry = @($SmokeArgs) -contains "--require-compute-morph-async-telemetry"
$requiresComputeMorph = (@($SmokeArgs) -contains "--require-compute-morph") -or $requiresComputeMorphNormalTangent -or $requiresComputeMorphSkin -or $requiresComputeMorphAsyncTelemetry
$requiresNativeUiOverlay = @($SmokeArgs) -contains "--require-native-ui-overlay"
$requiresNativeUiTexturedSpriteAtlas = @($SmokeArgs) -contains "--require-native-ui-textured-sprite-atlas"
$requiresNativeUiTextGlyphAtlas = @($SmokeArgs) -contains "--require-native-ui-text-glyph-atlas"
$requiresGpuSkinning = @($SmokeArgs) -contains "--require-gpu-skinning"
$requiresNative2dSprites = @($SmokeArgs) -contains "--require-native-2d-sprites"
$requiresSpriteAnimation = @($SmokeArgs) -contains "--require-sprite-animation"
$requiresTilemapRuntimeUx = @($SmokeArgs) -contains "--require-tilemap-runtime-ux"
$requiresPackageStreamingSafePoint = @($SmokeArgs) -contains "--require-package-streaming-safe-point"
$requiresSceneCollisionPackage = @($SmokeArgs) -contains "--require-scene-collision-package"
$expectedSmokeFrames = 2
for ($index = 0; $index -lt ($SmokeArgs.Count - 1); ++$index) {
    if ($SmokeArgs[$index] -eq "--max-frames") {
        $parsedMaxFrames = 0
        if (-not [int]::TryParse($SmokeArgs[$index + 1], [ref]$parsedMaxFrames) -or $parsedMaxFrames -le 0) {
            Write-Error "Installed desktop runtime smoke includes invalid --max-frames value: $($SmokeArgs[$index + 1])"
        }
        $expectedSmokeFrames = $parsedMaxFrames
    }
}
if ($requiresPlayable3dSlice) {
    $requiresRendererQualityGates = $true
    $requiresPackageStreamingSafePoint = $true
}
if ($requiresVisible3dProductionProof) {
    $requiresPostprocessDepthInput = $true
    $requiresRendererQualityGates = $true
    $requiresPlayable3dSlice = $true
    $requiresPackageStreamingSafePoint = $true
    $requiresNativeUiOverlay = $true
}
if ($requiresSceneCollisionPackage) {
    $requiresPackageStreamingSafePoint = $true
}
if ($requiresShadowMorphComposition) {
    $requiresDirectionalShadow = $true
    $requiresDirectionalShadowFiltering = $true
    $requiresPostprocessDepthInput = $true
    $requiresRendererQualityGates = $true
}
if ($requiresDirectionalShadowFiltering) {
    $requiresDirectionalShadow = $true
}
if ($requiresNativeUiTexturedSpriteAtlas) {
    $requiresNativeUiOverlay = $true
}
if ($requiresNativeUiTextGlyphAtlas) {
    $requiresNativeUiOverlay = $true
}
$escapedGameTarget = [regex]::Escape($GameTarget)
$statusLinePattern = "(?m)^$escapedGameTarget status="
if ($smokeOutput -notmatch $statusLinePattern) {
    Write-Error "Installed desktop runtime smoke did not emit the required '$GameTarget status=' report line."
}
if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpresentation_selected=") {
    Write-Error "Installed desktop runtime smoke status line did not include presentation_selected report field."
}
if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpresentation_backend_reports=") {
    Write-Error "Installed desktop runtime smoke status line did not include presentation_backend_reports report field."
}
if ($GameTarget -eq "sample_desktop_runtime_game") {
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcamera_primary=1\b") {
        Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove a primary 3D camera."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcamera_controller_ticks=\d+\b") {
        Write-Error "Installed sample_desktop_runtime_game smoke status line did not include camera_controller_ticks."
    }
    foreach ($field in @(
            "scene_mesh_plan_meshes",
            "scene_mesh_plan_draws",
            "scene_mesh_plan_unique_meshes",
            "scene_mesh_plan_unique_materials",
            "scene_mesh_plan_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed sample_desktop_runtime_game smoke status line did not include 3D scene mesh plan field: $field"
        }
    }
    foreach ($field in @(
            "scene_mesh_plan_meshes",
            "scene_mesh_plan_draws",
            "scene_mesh_plan_unique_meshes",
            "scene_mesh_plan_unique_materials"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove 3D scene mesh plan count: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bscene_mesh_plan_diagnostics=0\b") {
        Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove clean 3D scene mesh planning diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bhud_boxes=\d+\b") {
        Write-Error "Installed sample_desktop_runtime_game smoke status line did not include HUD diagnostics."
    }
    if ($requiresNativeUiOverlay) {
        foreach ($field in @("ui_overlay_requested", "ui_overlay_status", "ui_overlay_ready", "ui_overlay_sprites_submitted", "ui_overlay_draws")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not include native UI overlay field: $field"
            }
        }
    }
    if ($requiresNativeUiTexturedSpriteAtlas) {
        foreach ($field in @("ui_texture_overlay_requested", "ui_texture_overlay_status", "ui_texture_overlay_atlas_ready", "ui_texture_overlay_sprites_submitted", "ui_texture_overlay_texture_binds", "ui_texture_overlay_draws", "ui_atlas_metadata_status", "ui_atlas_metadata_pages", "ui_atlas_metadata_bindings")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not include textured UI overlay field: $field"
            }
        }
    }
}
if ($GameTarget -eq "sample_2d_desktop_runtime_package") {
    foreach ($field in @(
            "sprite_batch_plan_sprites",
            "sprite_batch_plan_textured_sprites",
            "sprite_batch_plan_draws",
            "sprite_batch_plan_texture_binds",
            "sprite_batch_plan_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not include sprite batch plan field: $field"
        }
    }
    foreach ($field in @(
            "sprite_batch_plan_sprites",
            "sprite_batch_plan_textured_sprites",
            "sprite_batch_plan_draws",
            "sprite_batch_plan_texture_binds"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove sprite batch plan count: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_batch_plan_diagnostics=0\b") {
        Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean sprite batch planning diagnostics."
    }
}
if ($GameTarget -eq "sample_generated_desktop_runtime_3d_package" -and $requiresNativeUiOverlay) {
    foreach ($field in @("hud_boxes", "ui_overlay_requested", "ui_overlay_status", "ui_overlay_ready", "ui_overlay_sprites_submitted", "ui_overlay_draws")) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not include native UI overlay HUD field: $field"
        }
    }
    $expectedGenerated3dUiOverlaySprites = $expectedSmokeFrames
    $expectedGenerated3dTextureOverlaySprites = 0
    if ($requiresNativeUiTexturedSpriteAtlas) {
        $expectedGenerated3dUiOverlaySprites += $expectedSmokeFrames
        $expectedGenerated3dTextureOverlaySprites += $expectedSmokeFrames
        foreach ($field in @("hud_images", "ui_atlas_metadata_requested", "ui_atlas_metadata_status", "ui_atlas_metadata_pages", "ui_atlas_metadata_bindings", "ui_texture_overlay_requested", "ui_texture_overlay_status", "ui_texture_overlay_atlas_ready", "ui_texture_overlay_sprites_submitted", "ui_texture_overlay_texture_binds", "ui_texture_overlay_draws")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
                Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not include textured UI atlas field: $field"
            }
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bhud_images=$expectedSmokeFrames\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact HUD image submission."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_requested=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove UI atlas metadata was requested."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_status=ready\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready UI atlas metadata."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_pages=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact UI atlas page count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_bindings=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact UI atlas image binding count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_requested=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove texture overlay was requested."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_status=ready\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready texture overlay status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_atlas_ready=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove texture atlas page readiness."
        }
    }
    if ($requiresNativeUiTextGlyphAtlas) {
        $expectedGenerated3dUiOverlaySprites += $expectedSmokeFrames
        $expectedGenerated3dTextureOverlaySprites += $expectedSmokeFrames
        foreach ($field in @("hud_text_glyphs", "text_glyphs_available", "text_glyphs_resolved", "text_glyphs_missing", "text_glyph_sprites_submitted", "ui_atlas_metadata_requested", "ui_atlas_metadata_status", "ui_atlas_metadata_pages", "ui_atlas_metadata_bindings", "ui_atlas_metadata_glyphs", "ui_texture_overlay_requested", "ui_texture_overlay_status", "ui_texture_overlay_atlas_ready", "ui_texture_overlay_sprites_submitted", "ui_texture_overlay_texture_binds", "ui_texture_overlay_draws")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
                Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not include text glyph UI atlas field: $field"
            }
        }
        foreach ($field in @("hud_text_glyphs", "text_glyphs_available", "text_glyphs_resolved", "text_glyph_sprites_submitted")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedSmokeFrames\b") {
                Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact text glyph field: $field"
            }
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\btext_glyphs_missing=0\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove zero missing text glyphs."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_requested=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove UI glyph atlas metadata was requested."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_status=ready\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready UI glyph atlas metadata."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_pages=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact UI glyph atlas page count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_bindings=0\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove zero image bindings for the glyph atlas package."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_glyphs=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact UI glyph binding count."
        }
    }
    if ($expectedGenerated3dTextureOverlaySprites -gt 0) {
        foreach ($field in @("ui_texture_overlay_sprites_submitted", "ui_texture_overlay_texture_binds", "ui_texture_overlay_draws")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedGenerated3dTextureOverlaySprites\b") {
                Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact texture overlay field: $field"
            }
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bhud_boxes=$expectedSmokeFrames\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact HUD box submission."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_requested=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove native UI overlay was requested."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_status=ready\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready native UI overlay status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_ready=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready native UI overlay."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_sprites_submitted=$expectedGenerated3dUiOverlaySprites\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact native UI overlay sprite submission."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_draws=$expectedSmokeFrames\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact native UI overlay draw output."
    }
}
if ($GameTarget -eq "sample_generated_desktop_runtime_3d_package" -and $requiresVisible3dProductionProof) {
    foreach ($field in @(
            "visible_3d_status",
            "visible_3d_ready",
            "visible_3d_diagnostics",
            "visible_3d_expected_frames",
            "visible_3d_presented_frames",
            "visible_3d_d3d12_selected",
            "visible_3d_null_fallback_used",
            "visible_3d_scene_gpu_ready",
            "visible_3d_postprocess_ready",
            "visible_3d_renderer_quality_ready",
            "visible_3d_playable_ready",
            "visible_3d_ui_overlay_ready"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not include visible 3D production proof field: $field"
        }
    }
    foreach ($field in @(
            "visible_3d_ready",
            "visible_3d_d3d12_selected",
            "visible_3d_scene_gpu_ready",
            "visible_3d_postprocess_ready",
            "visible_3d_renderer_quality_ready",
            "visible_3d_playable_ready",
            "visible_3d_ui_overlay_ready"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready visible 3D field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bvisible_3d_status=ready\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready visible 3D status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bvisible_3d_diagnostics=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean visible 3D diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bvisible_3d_expected_frames=$expectedSmokeFrames\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact visible 3D expected frame count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bvisible_3d_presented_frames=$expectedSmokeFrames\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact visible 3D presented frame count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bvisible_3d_null_fallback_used=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove visible 3D avoided null fallback."
    }
}
if ($GameTarget -eq "sample_generated_desktop_runtime_3d_package" -and $requiresSceneCollisionPackage) {
    foreach ($field in @(
            "collision_package_status",
            "collision_package_ready",
            "collision_package_diagnostics",
            "collision_package_bodies",
            "collision_package_triggers",
            "collision_package_contacts",
            "collision_package_trigger_overlaps",
            "collision_package_world_ready",
            "gameplay_systems_collision_package_ready"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not include scene collision package field: $field"
        }
    }
    foreach ($field in @(
            "collision_package_ready",
            "collision_package_world_ready",
            "gameplay_systems_collision_package_ready"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready scene collision package field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_package_status=ready\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready scene collision package status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_package_diagnostics=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean scene collision package diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_package_bodies=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact scene collision body count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_package_triggers=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact scene collision trigger count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_package_contacts=[1-9]\d*\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove non-zero scene collision contacts."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_package_trigger_overlaps=[1-9]\d*\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove non-zero scene collision trigger overlaps."
    }
}
if ($requiresNative2dSprites) {
    foreach ($field in @(
            "native_2d_sprites_requested",
            "native_2d_sprites_status",
            "native_2d_sprites_ready",
            "native_2d_sprites_submitted",
            "native_2d_textured_sprites_submitted",
            "native_2d_texture_atlas_ready",
            "native_2d_texture_binds",
            "native_2d_draws",
            "native_2d_textured_draws",
            "native_2d_sprite_batches_executed",
            "native_2d_sprite_batch_sprites_executed",
            "native_2d_sprite_batch_textured_sprites_executed",
            "native_2d_sprite_batch_texture_binds"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include native 2D sprite field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprites_requested=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite overlay was requested."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprites_status=ready\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready native 2D sprite overlay status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprites_ready=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready native 2D sprite overlay."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprites_submitted=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite submission."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_textured_sprites_submitted=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D textured sprite submission."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_texture_atlas_ready=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready native 2D sprite atlas texture."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_texture_binds=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite texture binding."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_draws=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite draw output."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_textured_draws=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D textured sprite draw output."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprite_batches_executed=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite batch execution."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprite_batch_sprites_executed=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite batch sprite execution."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprite_batch_textured_sprites_executed=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D textured sprite batch execution."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprite_batch_texture_binds=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite batch texture binding."
    }
}
if ($requiresSpriteAnimation) {
    foreach ($field in @(
            "sprite_animation_frames_sampled",
            "sprite_animation_frames_applied",
            "sprite_animation_selected_frame_sum",
            "sprite_animation_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include sprite animation field: $field"
        }
    }
    foreach ($field in @(
            "sprite_animation_frames_sampled",
            "sprite_animation_frames_applied",
            "sprite_animation_selected_frame_sum"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove sprite animation count: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_animation_diagnostics=0\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove clean sprite animation diagnostics."
    }
}
if ($requiresTilemapRuntimeUx) {
    foreach ($field in @(
            "tilemap_layers",
            "tilemap_visible_layers",
            "tilemap_tiles",
            "tilemap_non_empty_cells",
            "tilemap_cells_sampled",
            "tilemap_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include tilemap runtime UX field: $field"
        }
    }
    foreach ($field in @(
            "tilemap_layers",
            "tilemap_visible_layers",
            "tilemap_tiles",
            "tilemap_non_empty_cells",
            "tilemap_cells_sampled"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove tilemap runtime UX count: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\btilemap_diagnostics=0\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove clean tilemap runtime UX diagnostics."
    }
}
if ($requiresPackageStreamingSafePoint) {
    foreach ($field in @(
            "package_streaming_status",
            "package_streaming_ready",
            "package_streaming_resident_bytes",
            "package_streaming_resident_budget_bytes",
            "package_streaming_committed_records",
            "package_streaming_stale_handles",
            "package_streaming_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include package streaming field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpackage_streaming_status=committed\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove committed package streaming status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpackage_streaming_ready=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready package streaming safe-point execution."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpackage_streaming_resident_bytes=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove positive package streaming resident bytes."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpackage_streaming_committed_records=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove committed package streaming records."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpackage_streaming_diagnostics=0\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove clean package streaming diagnostics."
    }
}
if ($smokeOutput -match "(?m)^$escapedGameTarget status=.*\bscene_gpu_status=ready") {
    foreach ($field in @(
            "scene_gpu_texture_uploads",
            "scene_gpu_material_uploads",
            "scene_gpu_material_pipeline_layouts",
            "scene_gpu_uploaded_texture_bytes",
            "scene_gpu_uploaded_mesh_bytes",
            "scene_gpu_uploaded_material_factor_bytes"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove scene GPU upload execution field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bscene_gpu_mesh_uploads=[1-9]\d*\b" -and
        $smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bscene_gpu_skinned_mesh_uploads=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove scene GPU static or skinned mesh upload execution."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bscene_gpu_mesh_resolved=[1-9]\d*\b" -and
        $smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bscene_gpu_skinned_mesh_resolved=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove scene GPU static or skinned mesh resolution."
    }
    if ($requiresShadowMorphComposition) {
        foreach ($field in @(
                "scene_gpu_morph_mesh_bindings",
                "scene_gpu_morph_mesh_uploads",
                "scene_gpu_morph_mesh_resolved",
                "scene_gpu_uploaded_morph_bytes",
                "renderer_gpu_morph_draws",
                "renderer_morph_descriptor_binds"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove shadow morph composition field: $field"
            }
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpostprocess_status=ready") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready postprocess_status for a ready scene GPU path."
    }
    $expectedFramegraphPasses = if ($requiresDirectionalShadow) { 3 } else { 2 }
    $expectedFramegraphBarrierSteps = if ($requiresDirectionalShadow) {
        5
    } elseif ($requiresPostprocessDepthInput) {
        3
    } else {
        1
    }
    $expectedFramegraphPassExecutions = $expectedSmokeFrames * $expectedFramegraphPasses
    $expectedFramegraphBarrierExecutions = $expectedSmokeFrames * $expectedFramegraphBarrierSteps
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bframegraph_passes=$expectedFramegraphPasses\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove the frame graph pass count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bframegraph_passes_executed=$expectedFramegraphPassExecutions\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove frame graph pass execution count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bframegraph_barrier_steps_executed=$expectedFramegraphBarrierExecutions\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove frame graph barrier-step execution count."
    }
    if ($requiresPostprocessDepthInput -and
        $smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpostprocess_depth_input_ready=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready postprocess depth input."
    }
    if ($requiresDirectionalShadow) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bdirectional_shadow_status=ready\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready directional shadow status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bdirectional_shadow_ready=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready directional shadow smoke."
        }
        if ($requiresDirectionalShadowFiltering) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bdirectional_shadow_filter_mode=fixed_pcf_3x3\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove fixed PCF directional shadow filtering."
            }
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bdirectional_shadow_filter_taps=9\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove 3x3 directional shadow filter tap count."
            }
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bdirectional_shadow_filter_radius_texels=1\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove directional shadow filter radius."
            }
        }
    }
    if ($requiresNativeUiOverlay) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_requested=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove native UI overlay was requested."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_status=ready\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready native UI overlay status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_ready=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready native UI overlay."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_sprites_submitted=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove native UI overlay sprite submission."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_draws=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove native UI overlay draw output."
        }
    }
    if ($requiresNativeUiTexturedSpriteAtlas) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_status=ready\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready UI atlas package metadata."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_pages=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove UI atlas metadata page rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_bindings=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove UI atlas metadata image bindings."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_requested=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove textured UI overlay was requested."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_status=ready\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready textured UI overlay status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_atlas_ready=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready textured UI atlas page."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_sprites_submitted=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove textured UI sprite submission."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_texture_binds=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove textured UI texture binding."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_draws=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove textured UI overlay draw output."
        }
    }
    if ($requiresNativeUiTextGlyphAtlas) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_status=ready\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready UI glyph atlas package metadata."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_pages=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove UI glyph atlas metadata page rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_glyphs=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove UI glyph atlas metadata glyph bindings."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_requested=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove glyph UI overlay was requested."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_status=ready\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready glyph UI overlay status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_atlas_ready=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready glyph UI atlas page."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_sprites_submitted=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove glyph UI sprite submission."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_texture_binds=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove glyph UI texture binding."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_draws=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove glyph UI overlay draw output."
        }
    }
    if ($requiresGpuSkinning) {
        foreach ($field in @(
                "scene_gpu_skinned_mesh_bindings",
                "scene_gpu_skinned_mesh_uploads",
                "scene_gpu_skinned_mesh_resolved",
                "renderer_gpu_skinning_draws",
                "renderer_skinned_palette_descriptor_binds"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove GPU skinning field: $field"
            }
        }
    }
}
if ($requiresRendererQualityGates) {
    $expectedRendererQualityFramegraphPasses = if ($requiresDirectionalShadow) { "3" } else { "2" }
    $expectedRendererQualityFramegraphBarrierSteps = if ($requiresDirectionalShadow) {
        "5"
    } elseif ($requiresPostprocessDepthInput) {
        "3"
    } else {
        "1"
    }
    $expectedRendererQualityFields = @{
        "renderer_quality_status" = "ready"
        "renderer_quality_ready" = "1"
        "renderer_quality_diagnostics" = "0"
        "renderer_quality_expected_framegraph_passes" = $expectedRendererQualityFramegraphPasses
        "renderer_quality_expected_framegraph_barrier_steps" = $expectedRendererQualityFramegraphBarrierSteps
        "renderer_quality_framegraph_passes_ok" = "1"
        "renderer_quality_framegraph_barrier_steps_ok" = "1"
        "renderer_quality_framegraph_execution_budget_ok" = "1"
        "renderer_quality_scene_gpu_ready" = "1"
        "renderer_quality_postprocess_ready" = "1"
    }
    if ($requiresPostprocessDepthInput) {
        $expectedRendererQualityFields["renderer_quality_postprocess_depth_input_ready"] = "1"
    }
    if ($requiresDirectionalShadow) {
        $expectedRendererQualityFields["renderer_quality_directional_shadow_ready"] = "1"
    }
    if ($requiresDirectionalShadowFiltering) {
        $expectedRendererQualityFields["renderer_quality_directional_shadow_filter_ready"] = "1"
    }
    foreach ($field in $expectedRendererQualityFields.Keys) {
        $expectedValue = [regex]::Escape($expectedRendererQualityFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove renderer quality gate field: $field=$($expectedRendererQualityFields[$field])"
        }
    }
}
if ($requiresPlayable3dSlice) {
    $expectedPlayable3dFields = @{
        "playable_3d_status" = "ready"
        "playable_3d_ready" = "1"
        "playable_3d_diagnostics" = "0"
        "playable_3d_expected_frames" = "2"
        "playable_3d_frames_ok" = "1"
        "playable_3d_game_frames_ok" = "1"
        "playable_3d_scene_mesh_plan_ready" = "1"
        "playable_3d_camera_controller_ready" = "1"
        "playable_3d_animation_ready" = "1"
        "playable_3d_morph_ready" = "1"
        "playable_3d_quaternion_ready" = "1"
        "playable_3d_package_streaming_ready" = "1"
        "playable_3d_scene_gpu_ready" = "1"
        "playable_3d_postprocess_ready" = "1"
        "playable_3d_renderer_quality_ready" = "1"
    }
    if ($requiresComputeMorph) {
        $expectedPlayable3dFields["playable_3d_compute_morph_selected"] = "1"
        $expectedPlayable3dFields["playable_3d_compute_morph_ready"] = "1"
    }
    if ($requiresComputeMorphNormalTangent -and -not $requiresComputeMorphSkin) {
        $expectedPlayable3dFields["playable_3d_compute_morph_normal_tangent_selected"] = "1"
        $expectedPlayable3dFields["playable_3d_compute_morph_normal_tangent_ready"] = "1"
    }
    if ($requiresComputeMorphSkin) {
        $expectedPlayable3dFields["playable_3d_compute_morph_skin_selected"] = "1"
        $expectedPlayable3dFields["playable_3d_compute_morph_skin_ready"] = "1"
    }
    if ($requiresComputeMorphAsyncTelemetry) {
        $expectedPlayable3dFields["playable_3d_compute_morph_async_telemetry_selected"] = "1"
        $expectedPlayable3dFields["playable_3d_compute_morph_async_telemetry_ready"] = "1"
    }
    foreach ($field in $expectedPlayable3dFields.Keys) {
        $expectedValue = [regex]::Escape($expectedPlayable3dFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove generated 3D playable package field: $field=$($expectedPlayable3dFields[$field])"
        }
    }
}

Write-Host "installed-desktop-runtime-validation: ok ($GameTarget)"
