#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$null = Assert-VcpkgExecutable -Purpose "the ENet network transport adapter validation"

Set-MirakanaiVcpkgEnvironment | Out-Null

$tools = Assert-CppBuildTools

Invoke-CheckedCommand $tools.CMake --preset network-enet

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
    "mirakana_runtime_network_enet",
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
    "MK_sandbox",
    "sample_headless",
    "sample_2d_playable_foundation",
    "sample_ai_navigation",
    "sample_gameplay_foundation",
    "sample_input_renderer",
    "sample_ui_audio_assets"
)
$testTargets = @(
    "MK_runtime_network_transport_adapter_tests",
    "MK_runtime_network_enet_tests"
)
$buildArguments = @("--build", "--preset", "network-enet", "--target") + $sdkTargets + $testTargets
Invoke-CheckedCommand $tools.CMake @buildArguments

Invoke-CheckedCommand $tools.CTest --preset network-enet --output-on-failure -R "MK_runtime_network_(transport_adapter|enet)_tests"

$installPrefix = Join-Path $root "out/install/network-enet"
$vcpkgInstalled = Join-Path $root "vcpkg_installed/x64-windows"
Invoke-CheckedCommand $tools.CMake --install (Join-Path $root "out/build/network-enet") --config Debug --prefix $installPrefix
& (Join-Path $PSScriptRoot "validate-installed-sdk.ps1") `
    -InstallPrefix $installPrefix `
    -BuildDir (Join-Path $root "out/build/installed-network-enet-consumer") `
    -BuildConfig Debug `
    -AdditionalRuntimePaths @((Join-Path $vcpkgInstalled "debug/bin")) `
    -AdditionalCMakeArgs @("-DCMAKE_PREFIX_PATH=$vcpkgInstalled")
