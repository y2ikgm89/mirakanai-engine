#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function Get-LinuxVulkanHostBlocker {
    param(
        [Parameter(Mandatory = $true)][string]$Kind,
        [Parameter(Mandatory = $true)][string]$Message
    )

    return [pscustomobject]@{
        Kind = $Kind
        Message = $Message
    }
}

function Add-LinuxVulkanHostBlocker {
    param(
        [Parameter(Mandatory = $true)]$Blockers,
        [Parameter(Mandatory = $true)][string]$Kind,
        [Parameter(Mandatory = $true)][string]$Message
    )

    $Blockers.Add((Get-LinuxVulkanHostBlocker -Kind $Kind -Message $Message)) | Out-Null
}

function Test-RepositoryPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Test-Path -LiteralPath (Join-Path $root $RelativePath)
}

$hostIsWindows = [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
    [System.Runtime.InteropServices.OSPlatform]::Windows)
$hostIsLinux = [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
    [System.Runtime.InteropServices.OSPlatform]::Linux)
$hostIsMacOS = [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
    [System.Runtime.InteropServices.OSPlatform]::OSX)
$hostName = if ($hostIsLinux) {
    "linux"
} elseif ($hostIsWindows) {
    "windows"
} elseif ($hostIsMacOS) {
    "macos"
} else {
    "unknown"
}

$vulkanInfo = Find-CommandOnCombinedPath "vulkaninfo"
$dxc = Find-CommandOnCombinedPath "dxc"
$spirvVal = Find-CommandOnCombinedPath "spirv-val"
$cmake = Get-CMakeCommand
$ctest = Get-CTestCommand

$blockers = [System.Collections.Generic.List[object]]::new()
if (-not $hostIsLinux) {
    Add-LinuxVulkanHostBlocker -Blockers $blockers -Kind "not_linux" -Message "Linux Vulkan platform evidence requires a Linux host; Windows Vulkan and Win32 package evidence are forbidden as Linux proof."
}
if (-not $vulkanInfo) {
    Add-LinuxVulkanHostBlocker -Blockers $blockers -Kind "missing_vulkaninfo" -Message "vulkaninfo is required to prove Linux Vulkan ICD/runtime/driver and device capability evidence."
}
if (-not $dxc) {
    Add-LinuxVulkanHostBlocker -Blockers $blockers -Kind "missing_dxc" -Message "DXC with SPIR-V CodeGen is required for strict Linux Vulkan shader evidence."
}
if (-not $spirvVal) {
    Add-LinuxVulkanHostBlocker -Blockers $blockers -Kind "missing_spirv_val" -Message "spirv-val is required for strict Linux Vulkan SPIR-V validation evidence."
}
if (-not $cmake) {
    Add-LinuxVulkanHostBlocker -Blockers $blockers -Kind "missing_cmake" -Message "CMake is required for the future desktop-runtime-linux configure/build lane."
}
if (-not $ctest) {
    Add-LinuxVulkanHostBlocker -Blockers $blockers -Kind "missing_ctest" -Message "CTest is required for the future desktop-runtime-linux focused test lane."
}
if (-not (Test-RepositoryPath "engine/runtime_host/linux/CMakeLists.txt")) {
    Add-LinuxVulkanHostBlocker -Blockers $blockers -Kind "missing_first_party_linux_runtime_host" -Message "engine/runtime_host/linux is not implemented yet; Linux readiness cannot reuse the Win32 host."
}
if (-not (Test-RepositoryPath "tools/package-desktop-runtime-linux.ps1")) {
    Add-LinuxVulkanHostBlocker -Blockers $blockers -Kind "missing_linux_package_script" -Message "tools/package-desktop-runtime-linux.ps1 is required before any Linux package smoke can be accepted."
}
if (-not (Test-RepositoryPath "tools/validate-installed-linux-desktop-runtime.ps1")) {
    Add-LinuxVulkanHostBlocker -Blockers $blockers -Kind "missing_linux_installed_validator" -Message "tools/validate-installed-linux-desktop-runtime.ps1 is required before installed Linux package evidence can be accepted."
}

$linuxHostReady = $hostIsLinux -and $vulkanInfo -and $dxc -and $spirvVal -and $cmake -and $ctest
$firstPartyLaneReady = (Test-RepositoryPath "engine/runtime_host/linux/CMakeLists.txt") -and
    (Test-RepositoryPath "tools/package-desktop-runtime-linux.ps1") -and
    (Test-RepositoryPath "tools/validate-installed-linux-desktop-runtime.ps1")
$ready = $linuxHostReady -and $firstPartyLaneReady
$status = if ($ready) { "preflight_ready" } else { "host-gated" }
$blockerCount = @($blockers).Count

Write-Host "environment-platform-linux-vulkan-host-gate: host=$hostName"
Write-Host "environment-platform-linux-vulkan-host-gate: status=$status"
Write-Host "environment-platform-linux-vulkan-host-gate: linux_host_ready=$(if ($linuxHostReady) { 1 } else { 0 })"
Write-Host "environment-platform-linux-vulkan-host-gate: first_party_linux_runtime_host_ready=$(if ($firstPartyLaneReady) { 1 } else { 0 })"
Write-Host "environment-platform-linux-vulkan-host-gate: vulkaninfo_ready=$(if ($vulkanInfo) { 1 } else { 0 })"
Write-Host "environment-platform-linux-vulkan-host-gate: dxc_spirv_codegen_ready=$(if ($dxc) { 1 } else { 0 })"
Write-Host "environment-platform-linux-vulkan-host-gate: spirv_validation_ready=$(if ($spirvVal) { 1 } else { 0 })"
Write-Host "environment-platform-linux-vulkan-host-gate: environment_platform_linux_vulkan_ready=0"
Write-Host "environment-platform-linux-vulkan-host-gate: environment_platform_requires_linux_vulkan_host_evidence=1"
Write-Host "environment-platform-linux-vulkan-host-gate: environment_all_platform_unconditional_ready=0"
Write-Host "environment-platform-linux-vulkan-host-gate: environment_platform_windows_vulkan_inferred=0"
Write-Host "environment-platform-linux-vulkan-host-gate: environment_platform_native_handle_access=0"
Write-Host "environment-platform-linux-vulkan-host-gate: blocker_count=$blockerCount"
foreach ($blocker in $blockers) {
    Write-Host "environment-platform-linux-vulkan-host-gate: blocker kind=$($blocker.Kind) - $($blocker.Message)"
}

if ($RequireReady -and -not $ready) {
    Write-Error "Linux Vulkan platform host evidence is host-gated; run on a Linux host after first-party Linux runtime host, Linux package script, installed validator, Vulkan ICD/runtime/driver, VK_LAYER_KHRONOS_validation, DXC SPIR-V CodeGen, and spirv-val evidence are available."
}

