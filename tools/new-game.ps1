#requires -Version 7.0
#requires -PSEdition Core

param(
    [Parameter(Mandatory = $true)]
    [string]$Name,

    [string]$DisplayName = "",

    [ValidateSet("Headless", "DesktopRuntimePackage", "DesktopRuntimeCookedScenePackage", "DesktopRuntimeMaterialShaderPackage", "DesktopRuntime2DPackage", "DesktopRuntime3DPackage")]
    [string]$Template = "Headless",

    [string]$RepositoryRoot = "",

    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$scriptRepositoryRoot = Get-RepoRoot

. (Join-Path $PSScriptRoot "new-game-helpers.ps1")
. (Join-Path $PSScriptRoot "new-game-templates.ps1")

if ([string]::IsNullOrWhiteSpace($RepositoryRoot)) {
    $root = $scriptRepositoryRoot
} else {
    $root = (Resolve-Path -LiteralPath $RepositoryRoot).Path
}

if ($Name -notmatch "^[a-z][a-z0-9_]*$") {
    Write-Error "Game name must match ^[a-z][a-z0-9_]*$"
}

$targetName = $Name
$manifestName = $Name.Replace("_", "-")
$gameDir = Join-Path $root (Join-Path "games" $Name)
$runtimeDir = Join-Path $gameDir "runtime"
$gamesCmake = Join-Path $root "games/CMakeLists.txt"

if ([string]::IsNullOrWhiteSpace($DisplayName)) {
    $DisplayName = $manifestName
}

if (Test-ContainsControlCharacter -Text $DisplayName) {
    Write-Error "DisplayName must not contain control characters."
}

if (-not (Test-Path -LiteralPath $gamesCmake)) {
    Write-Error "games/CMakeLists.txt does not exist under repository root: $root"
}

if (Test-Path -LiteralPath $gameDir) {
    Write-Error "Game directory already exists: $gameDir"
}

$gamesCmakeContent = Get-Content -LiteralPath $gamesCmake -Raw
$escapedTargetName = [System.Text.RegularExpressions.Regex]::Escape($targetName)
if ($gamesCmakeContent -match "(?m)^\s*MK_add_(?:desktop_runtime_)?game\(\s*$escapedTargetName(?=\s|\))") {
    Write-Error "Game target already exists in games/CMakeLists.txt: $targetName"
}

$planned = @(
    $gameDir,
    (Join-Path $gameDir "main.cpp"),
    (Join-Path $gameDir "README.md"),
    (Join-Path $gameDir "game.agent.json")
)

if ($Template -ne "Headless") {
    $planned += @(
        $runtimeDir,
        (Join-Path $runtimeDir "$Name.config")
    )
}
if ($Template -eq "DesktopRuntimeCookedScenePackage" -or $Template -eq "DesktopRuntimeMaterialShaderPackage") {
    $planned += @(
        (Join-Path $runtimeDir ".gitattributes"),
        (Join-Path $runtimeDir "$Name.geindex"),
        (Join-Path $runtimeDir "assets/generated/base_color.texture.geasset"),
        (Join-Path $runtimeDir "assets/generated/triangle.mesh"),
        (Join-Path $runtimeDir "assets/generated/lit.material"),
        (Join-Path $runtimeDir "assets/generated/packaged_scene.scene")
    )
}
if ($Template -eq "DesktopRuntime3DPackage") {
    $planned += @(
        (Join-Path $runtimeDir ".gitattributes"),
        (Join-Path $runtimeDir "$Name.geindex"),
        (Join-Path $runtimeDir "assets/3d/base_color.texture.geasset"),
        (Join-Path $runtimeDir "assets/3d/triangle.mesh"),
        (Join-Path $runtimeDir "assets/3d/skinned_triangle.skinned_mesh"),
        (Join-Path $runtimeDir "assets/3d/lit.material"),
        (Join-Path $runtimeDir "assets/3d/packaged_scene.scene"),
        (Join-Path $gameDir "source/assets/package.geassets"),
        (Join-Path $gameDir "source/scenes/packaged_scene.scene"),
        (Join-Path $gameDir "source/prefabs/static_prop.prefab"),
        (Join-Path $gameDir "source/textures/base_color.texture_source"),
        (Join-Path $gameDir "source/meshes/triangle.mesh_source")
    )
}
if ($Template -eq "DesktopRuntime2DPackage") {
    $planned += @(
        (Join-Path $runtimeDir ".gitattributes"),
        (Join-Path $runtimeDir "$Name.geindex"),
        (Join-Path $runtimeDir "assets/2d/player.texture.geasset"),
        (Join-Path $runtimeDir "assets/2d/player.material"),
        (Join-Path $runtimeDir "assets/2d/jump.audio.geasset"),
        (Join-Path $runtimeDir "assets/2d/level.tilemap"),
        (Join-Path $runtimeDir "assets/2d/player.sprite_animation"),
        (Join-Path $runtimeDir "assets/2d/playable.scene"),
        (Join-Path $gameDir "shaders/runtime_2d_sprite.hlsl")
    )
}
if ($Template -eq "DesktopRuntimeMaterialShaderPackage" -or $Template -eq "DesktopRuntime3DPackage") {
    $planned += @(
        (Join-Path $gameDir "source/materials/lit.material"),
        (Join-Path $gameDir "shaders/runtime_scene.hlsl"),
        (Join-Path $gameDir "shaders/runtime_postprocess.hlsl")
    )
}
if ($Template -eq "DesktopRuntime3DPackage") {
    $planned += @(
        (Join-Path $gameDir "shaders/runtime_shadow.hlsl"),
        (Join-Path $gameDir "shaders/runtime_ui_overlay.hlsl")
    )
}
$planned += $gamesCmake

if ($DryRun) {
    $planned | ForEach-Object { Write-Host "would create/update: $_" }
    exit 0
}

New-Item -ItemType Directory -Path $gameDir | Out-Null

if ($Template -eq "Headless") {
    $mainCpp = New-HeadlessMainCpp -GameName $Name -TargetName $targetName
    $readme = New-HeadlessReadme -Title $DisplayName
    $manifest = New-HeadlessManifest -GameName $Name -DisplayTitle $DisplayName -TargetName $targetName
    $registration = New-HeadlessRegistration -GameName $Name -TargetName $targetName
} elseif ($Template -eq "DesktopRuntimePackage") {
    New-Item -ItemType Directory -Path $runtimeDir | Out-Null
    $mainCpp = New-DesktopRuntimeMainCpp -GameName $Name -TargetName $targetName -Title $DisplayName
    $readme = New-DesktopRuntimeReadme -Title $DisplayName -TargetName $targetName
    $manifest = New-DesktopRuntimeManifest -GameName $Name -DisplayTitle $DisplayName -TargetName $targetName
    $registration = New-DesktopRuntimeRegistration -GameName $Name -TargetName $targetName
    $runtimeConfig = @"
format=GameEngine.GeneratedDesktopRuntimePackage.Config.v1
name=$Name
displayName=$DisplayName
renderer=null-fallback
"@
    Set-Content -LiteralPath (Join-Path $runtimeDir "$Name.config") -Value $runtimeConfig -NoNewline
} elseif ($Template -eq "DesktopRuntimeCookedScenePackage") {
    New-Item -ItemType Directory -Path (Join-Path $runtimeDir "assets/generated") -Force | Out-Null
    $cookedScenePackage = New-DesktopRuntimeCookedScenePackageFiles -GameName $Name -DisplayTitle $DisplayName
    $mainCpp = New-DesktopRuntimeCookedSceneMainCpp -GameName $Name -TargetName $targetName -Title $DisplayName -SceneAssetName $cookedScenePackage.SceneAssetName
    $readme = New-DesktopRuntimeCookedSceneReadme -Title $DisplayName -TargetName $targetName -GameName $Name
    $manifest = New-DesktopRuntimeCookedSceneManifest -GameName $Name -DisplayTitle $DisplayName -TargetName $targetName
    $registration = New-DesktopRuntimeCookedSceneRegistration -GameName $Name -TargetName $targetName
    $runtimeConfig = @"
format=GameEngine.GeneratedDesktopRuntimeCookedScenePackage.Config.v1
name=$Name
displayName=$DisplayName
renderer=null-fallback
scenePackage=runtime/$Name.geindex
"@
    Set-Content -LiteralPath (Join-Path $runtimeDir "$Name.config") -Value $runtimeConfig -NoNewline
    foreach ($entry in $cookedScenePackage.Files.GetEnumerator()) {
        $path = Join-Path $gameDir $entry.Key
        $directory = Split-Path -Parent $path
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        Set-Content -LiteralPath $path -Value $entry.Value -NoNewline
    }
} elseif ($Template -eq "DesktopRuntime2DPackage") {
    New-Item -ItemType Directory -Path (Join-Path $runtimeDir "assets/2d") -Force | Out-Null
    $desktop2dPackage = New-DesktopRuntime2DPackageFiles -GameName $Name -DisplayTitle $DisplayName
    $mainCpp = New-DesktopRuntime2DMainCpp -GameName $Name -TargetName $targetName -Title $DisplayName
    $readme = New-DesktopRuntime2DReadme -Title $DisplayName -TargetName $targetName -GameName $Name
    $manifest = New-DesktopRuntime2DManifest -GameName $Name -DisplayTitle $DisplayName -TargetName $targetName
    $registration = New-DesktopRuntime2DRegistration -GameName $Name -TargetName $targetName
    $runtimeConfig = @"
format=GameEngine.GeneratedDesktopRuntime2DPackage.Config.v1
name=$Name
displayName=$DisplayName
renderer=null-fallback
scenePackage=runtime/$Name.geindex
"@
    Set-Content -LiteralPath (Join-Path $runtimeDir "$Name.config") -Value $runtimeConfig -NoNewline
    foreach ($entry in $desktop2dPackage.Files.GetEnumerator()) {
        $path = Join-Path $gameDir $entry.Key
        $directory = Split-Path -Parent $path
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        Set-Content -LiteralPath $path -Value $entry.Value -NoNewline
    }
} elseif ($Template -eq "DesktopRuntime3DPackage") {
    New-Item -ItemType Directory -Path (Join-Path $runtimeDir "assets/3d") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $gameDir "source/materials") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $gameDir "shaders") -Force | Out-Null
    $desktop3dPackage = New-DesktopRuntime3DPackageFiles -GameName $Name -DisplayTitle $DisplayName
    $shaderAuthoringPackage = New-DesktopRuntimeMaterialShaderPackageFiles -GameName $Name -DisplayTitle $DisplayName
    $desktop3dPackage.Files["source/materials/lit.material"] = $desktop3dPackage.Files["runtime/assets/3d/lit.material"]
    $desktop3dPackage.Files["shaders/runtime_scene.hlsl"] = $shaderAuthoringPackage.Files["shaders/runtime_scene.hlsl"]
    $desktop3dPackage.Files["shaders/runtime_postprocess.hlsl"] = $shaderAuthoringPackage.Files["shaders/runtime_postprocess.hlsl"]
    $desktop3dPackage.Files["shaders/runtime_postprocess.hlsl"] = $desktop3dPackage.Files["shaders/runtime_postprocess.hlsl"].Replace(
        "Texture2D scene_color_texture : register(t0);`nSamplerState scene_color_sampler : register(s1);",
        "Texture2D scene_color_texture : register(t0);`nSamplerState scene_color_sampler : register(s1);`nTexture2D<float> scene_depth_texture : register(t2);`nSamplerState scene_depth_sampler : register(s3);")
    $desktop3dPackage.Files["shaders/runtime_postprocess.hlsl"] = $desktop3dPackage.Files["shaders/runtime_postprocess.hlsl"].Replace(
        "    float3 graded = saturate(scene.rgb * float3(1.04, 1.02, 0.98) + float3(0.012, 0.008, 0.0));",
        "    float scene_depth = scene_depth_texture.Sample(scene_depth_sampler, input.uv);`n    float near_depth_grade = saturate(1.0 - scene_depth) * 0.08;`n    float3 graded = saturate(scene.rgb * float3(1.04, 1.02, 0.98) +`n                             float3(0.012 + near_depth_grade, 0.008 + near_depth_grade * 0.55,`n                                    near_depth_grade * 0.25));")
    $shadowShader = @"
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct VsIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT;
};

struct VsOut {
    float4 position : SV_Position;
};

VsOut vs_shadow(VsIn input) {
    VsOut output;
    output.position = float4(input.position.xy, 0.35, 1.0);
    return output;
}

float4 ps_shadow(VsOut input) : SV_Target {
    float depth_tint = saturate(input.position.z);
    return float4(depth_tint * 0.0, 0.0, 0.0, 1.0);
}
"@
    $desktop3dPackage.Files["shaders/runtime_shadow.hlsl"] = ConvertTo-LfText -Text $shadowShader
    $nativeUiOverlayShaderPath = Join-Path $scriptRepositoryRoot "games/sample_generated_desktop_runtime_3d_package/shaders/runtime_ui_overlay.hlsl"
    if (Test-Path -LiteralPath $nativeUiOverlayShaderPath) {
        $desktop3dPackage.Files["shaders/runtime_ui_overlay.hlsl"] =
            ConvertTo-LfText -Text (Get-Content -LiteralPath $nativeUiOverlayShaderPath -Raw)
    } else {
        $nativeUiOverlayShader = @"
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct NativeUiOverlayVertexIn {
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    uint texture_index : TEXCOORD1;
};

struct NativeUiOverlayVertexOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    uint texture_index : TEXCOORD1;
};

NativeUiOverlayVertexOut vs_native_ui_overlay(NativeUiOverlayVertexIn input) {
    NativeUiOverlayVertexOut output;
    output.position = float4(input.position, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    output.texture_index = input.texture_index;
    return output;
}

float4 ps_native_ui_overlay(NativeUiOverlayVertexOut input) : SV_Target {
    return saturate(input.color);
}
"@
        $desktop3dPackage.Files["shaders/runtime_ui_overlay.hlsl"] = ConvertTo-LfText -Text $nativeUiOverlayShader
    }
    $mainCpp = New-DesktopRuntime3DMainCpp -GameName $Name -TargetName $targetName -Title $DisplayName -SceneAssetName $desktop3dPackage.SceneAssetName
    $readme = New-DesktopRuntime3DReadme -Title $DisplayName -TargetName $targetName -GameName $Name
    $manifest = New-DesktopRuntime3DManifest -GameName $Name -DisplayTitle $DisplayName -TargetName $targetName
    $registration = New-DesktopRuntime3DRegistration -GameName $Name -TargetName $targetName
    $runtimeConfig = @"
format=GameEngine.GeneratedDesktopRuntime3DPackage.Config.v1
name=$Name
displayName=$DisplayName
renderer=host-built-d3d12-vulkan-shader-artifacts-with-null-fallback
scenePackage=runtime/$Name.geindex
"@
    Set-Content -LiteralPath (Join-Path $runtimeDir "$Name.config") -Value $runtimeConfig -NoNewline
    foreach ($entry in $desktop3dPackage.Files.GetEnumerator()) {
        $path = Join-Path $gameDir $entry.Key
        $directory = Split-Path -Parent $path
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        Set-Content -LiteralPath $path -Value $entry.Value -NoNewline
    }
} else {
    New-Item -ItemType Directory -Path (Join-Path $runtimeDir "assets/generated") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $gameDir "source/materials") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $gameDir "shaders") -Force | Out-Null
    $materialShaderPackage = New-DesktopRuntimeMaterialShaderPackageFiles -GameName $Name -DisplayTitle $DisplayName
    $mainCpp = New-DesktopRuntimeCookedSceneMainCpp -GameName $Name -TargetName $targetName -Title $DisplayName -SceneAssetName $materialShaderPackage.SceneAssetName
    $readme = New-DesktopRuntimeMaterialShaderReadme -Title $DisplayName -TargetName $targetName -GameName $Name
    $manifest = New-DesktopRuntimeMaterialShaderManifest -GameName $Name -DisplayTitle $DisplayName -TargetName $targetName
    $registration = New-DesktopRuntimeMaterialShaderRegistration -GameName $Name -TargetName $targetName
    $runtimeConfig = @"
format=GameEngine.GeneratedDesktopRuntimeCookedScenePackage.Config.v1
name=$Name
displayName=$DisplayName
renderer=host-built-d3d12-vulkan-shader-artifacts-with-null-fallback
scenePackage=runtime/$Name.geindex
"@
    Set-Content -LiteralPath (Join-Path $runtimeDir "$Name.config") -Value $runtimeConfig -NoNewline
    foreach ($entry in $materialShaderPackage.Files.GetEnumerator()) {
        $path = Join-Path $gameDir $entry.Key
        $directory = Split-Path -Parent $path
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        Set-Content -LiteralPath $path -Value $entry.Value -NoNewline
    }
}

$mainCpp = Format-CppSourceText -Text $mainCpp
Set-Content -LiteralPath (Join-Path $gameDir "main.cpp") -Value $mainCpp -NoNewline
Set-Content -LiteralPath (Join-Path $gameDir "README.md") -Value $readme -NoNewline
Set-Content -LiteralPath (Join-Path $gameDir "game.agent.json") -Value ($manifest | ConvertTo-Json -Depth 12) -NoNewline

Add-Content -LiteralPath $gamesCmake -Value $registration

Write-Host "created game: games/$Name"
Write-Host "target: $targetName"
Write-Host "template: $Template"


