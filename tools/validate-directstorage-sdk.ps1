#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

if (-not [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
    Write-Error "DirectStorage SDK validation is Windows-only."
}

$null = Assert-VcpkgExecutable -Purpose "the DirectStorage SDK dependency gate validation"

Set-MirakanaiVcpkgEnvironment | Out-Null

$tools = Assert-CppBuildTools

Invoke-CheckedCommand $tools.CMake --preset directstorage-sdk
Invoke-CheckedCommand $tools.CMake --build --preset directstorage-sdk --target MK_runtime_host_win32_directstorage_sdk_tests
Invoke-CheckedCommand $tools.CTest --preset directstorage-sdk --output-on-failure -R "MK_runtime_host_win32_directstorage_sdk_tests"
