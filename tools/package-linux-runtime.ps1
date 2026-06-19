#requires -Version 7.0
#requires -PSEdition Core

param(
    [ValidateRange(0, 1024)][int]$Jobs = 0,
    [string]$InstallPrefix = "",
    [string]$EvidenceDir = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

function Test-HostLinux {
    return [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::Linux)
}

function Assert-PathUnderRoot {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$AllowedRoot,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $fullRoot = [System.IO.Path]::GetFullPath($AllowedRoot)
    $trimChars = @([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
    $rootNormalized = $fullRoot.TrimEnd($trimChars)
    $pathNormalized = $fullPath.TrimEnd($trimChars)
    $rootPrefix = $rootNormalized + [System.IO.Path]::DirectorySeparatorChar
    if ($pathNormalized -ne $rootNormalized -and -not $pathNormalized.StartsWith($rootPrefix, [System.StringComparison]::Ordinal)) {
        Write-Error "$Description must stay under $rootNormalized`: $fullPath"
    }
    return $fullPath
}

if (-not (Test-HostLinux)) {
    Write-Error "linux-vulkan-runtime-package requires a Linux host."
}

$tools = Assert-CppBuildTools
if (-not $tools.CPack) {
    Write-Error "CPack is required but was not found. Install official CMake 3.30+ with CPack."
}

if ([string]::IsNullOrWhiteSpace($InstallPrefix)) {
    $InstallPrefix = Join-Path $root "out/install/linux-vulkan-runtime-release"
}
if ([string]::IsNullOrWhiteSpace($EvidenceDir)) {
    $EvidenceDir = Join-Path $root "out/evidence/linux-vulkan-runtime"
}

$installRoot = Join-Path $root "out/install"
$evidenceRoot = Join-Path $root "out/evidence"
$installPrefixPath = Assert-PathUnderRoot -Path $InstallPrefix -AllowedRoot $installRoot -Description "Linux runtime install prefix"
$evidenceDirPath = Assert-PathUnderRoot -Path $EvidenceDir -AllowedRoot $evidenceRoot -Description "Linux runtime evidence directory"

if (Test-Path -LiteralPath $installPrefixPath -PathType Container) {
    Remove-Item -LiteralPath $installPrefixPath -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $evidenceDirPath | Out-Null

$jobsToUse = Resolve-ParallelJobCount -Jobs $Jobs

Invoke-CheckedCommand $tools.CMake --preset linux-vulkan-runtime-release
Invoke-CheckedCommand $tools.CMake --build --preset linux-vulkan-runtime-release --target MK_linux_vulkan_runtime_package_build --parallel $jobsToUse
Invoke-CheckedCommand $tools.CTest --preset linux-vulkan-runtime-release --output-on-failure -R "MK_linux_vulkan_runtime_probe_tests|linux_vulkan_runtime_probe_smoke|MK_vulkan_environment_weather_solver_tests"

$buildDir = Join-Path $root "out/build/linux-vulkan-runtime-release"
Invoke-CheckedCommand $tools.CMake --install $buildDir --config Release --prefix $installPrefixPath

$evidenceFile = Join-Path $evidenceDirPath "linux-vulkan-runtime-evidence.txt"
$validator = Join-Path $PSScriptRoot "validate-installed-linux-runtime.ps1"
$validatorOutput = & $validator `
    -InstallPrefix $installPrefixPath `
    -RequireReady `
    -ExpectedEvidenceCounters @(
        "linux_installed_validator_ready=1",
        "linux_vulkan_runtime_probe_ready=1",
        "linux_vulkan_runtime_readback_ready=1",
        "linux_vulkan_runtime_probe_surface_family=offscreen_compute",
        "first_party_linux_runtime_host_ready=1",
        "native_handle_access=0"
    ) 2>&1
foreach ($line in @($validatorOutput)) {
    Write-Output $line
}

$packageLine = "validation_recipe=environment-platform-linux-vulkan-package linux_package_script_ready=1 linux_installed_validator_ready=1 linux_vulkan_runtime_probe_ready=1 linux_vulkan_runtime_readback_ready=1 linux_vulkan_runtime_probe_surface_family=offscreen_compute environment_all_platform_unconditional_ready=0 environment_commercial_ready=0 environment_ready=0"
$allEvidence = @($validatorOutput) + @($packageLine)
Set-Content -LiteralPath $evidenceFile -Value $allEvidence -Encoding utf8
Write-Output $packageLine
Write-Output "linux-vulkan-runtime-package: evidence_file=$evidenceFile"

Invoke-CheckedCommand $tools.CPack --preset linux-vulkan-runtime-release

Write-Host "linux-vulkan-runtime-package: ok"
