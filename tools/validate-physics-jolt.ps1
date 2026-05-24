#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$null = Assert-VcpkgExecutable -Purpose "the Jolt physics adapter validation"

Set-MirakanaiVcpkgEnvironment | Out-Null

$tools = Assert-CppBuildTools

Invoke-CheckedCommand $tools.CMake --preset physics-jolt

$sdkTargets = @(
    "mirakana_ai",
    "mirakana_animation",
    "mirakana_assets",
    "MK_audio",
    "mirakana_core",
    "mirakana_math",
    "mirakana_navigation",
    "mirakana_physics",
    "MK_platform",
    "mirakana_renderer",
    "mirakana_rhi",
    "mirakana_runtime",
    "MK_runtime_host",
    "mirakana_runtime_rhi",
    "mirakana_runtime_scene",
    "mirakana_runtime_scene_rhi",
    "MK_rhi_metal",
    "mirakana_rhi_vulkan",
    "mirakana_scene",
    "mirakana_scene_renderer",
    "MK_tools",
    "mirakana_ui",
    "mirakana_ui_renderer",
    "mirakana_physics_jolt",
    "MK_sandbox",
    "sample_headless",
    "sample_2d_playable_foundation",
    "sample_ai_navigation",
    "sample_gameplay_foundation",
    "sample_input_renderer",
    "sample_ui_audio_assets"
)
$testTargets = @(
    "MK_physics_native_adapter_tests",
    "MK_physics_jolt_tests"
)
$buildArguments = @("--build", "--preset", "physics-jolt", "--target") + $sdkTargets + $testTargets
Invoke-CheckedCommand $tools.CMake @buildArguments

Invoke-CheckedCommand $tools.CTest --preset physics-jolt --output-on-failure -R "MK_physics_(native_adapter|jolt)_tests"

$installPrefix = Join-Path $root "out/install/physics-jolt"
$vcpkgInstalled = Join-Path $root "vcpkg_installed/x64-windows"
Invoke-CheckedCommand $tools.CMake --install (Join-Path $root "out/build/physics-jolt") --config Debug --prefix $installPrefix
& (Join-Path $PSScriptRoot "validate-installed-sdk.ps1") `
    -InstallPrefix $installPrefix `
    -BuildDir (Join-Path $root "out/build/installed-physics-jolt-consumer") `
    -BuildConfig Debug `
    -AdditionalRuntimePaths @((Join-Path $vcpkgInstalled "debug/bin")) `
    -AdditionalCMakeArgs @("-DCMAKE_PREFIX_PATH=$vcpkgInstalled")
