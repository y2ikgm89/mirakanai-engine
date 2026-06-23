#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady,
    [string[]]$ExpectedEvidenceCounters = @(),
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AdditionalExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$ExpectedEvidenceCounters = @($ExpectedEvidenceCounters) + @($AdditionalExpectedEvidenceCounters)

$root = Get-RepoRoot
Set-Location $root

function Get-CounterValueFromLines {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$DefaultValue
    )

    $prefix = "$Name="
    foreach ($line in $Lines) {
        if ($line.StartsWith($prefix, [System.StringComparison]::Ordinal)) {
            return $line.Substring($prefix.Length)
        }
    }
    return $DefaultValue
}

function Find-MavgVulkanMeshShaderLodTestExecutable {
    param([Parameter(Mandatory = $true)][string]$RepositoryRoot)

    $candidatePaths = @(
        "out/build/dev/Debug/MK_mavg_vulkan_mesh_shader_lod_tests.exe",
        "out/build/dev/MK_mavg_vulkan_mesh_shader_lod_tests.exe",
        "out/build/dev/MK_mavg_vulkan_mesh_shader_lod_tests"
    )
    foreach ($candidatePath in $candidatePaths) {
        $absolutePath = Join-Path $RepositoryRoot $candidatePath
        if (Test-Path -LiteralPath $absolutePath -PathType Leaf) {
            return $absolutePath
        }
    }

    Write-Error "MK_mavg_vulkan_mesh_shader_lod_tests executable was not found after build."
}

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for MAVG Vulkan mesh shader indirect validation."
}

Write-Information "mavg-vulkan-mesh-shader-indirect-dispatch: configuring dev preset..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--preset",
    "dev"
)

Write-Information "mavg-vulkan-mesh-shader-indirect-dispatch: building focused Vulkan test target..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--build",
    "--preset",
    "dev",
    "--target",
    "MK_mavg_vulkan_mesh_shader_lod_tests"
)

Write-Information "mavg-vulkan-mesh-shader-indirect-dispatch: running focused CTest lane..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/ctest.ps1"),
    "--preset",
    "dev",
    "--output-on-failure",
    "-R",
    "MK_mavg_vulkan_mesh_shader_lod_tests"
)

$testExecutable = Find-MavgVulkanMeshShaderLodTestExecutable -RepositoryRoot $root
$previousPrintEvidence = $env:MK_VULKAN_MAVG_MESH_SHADER_LOD_PRINT_EVIDENCE
try {
    $env:MK_VULKAN_MAVG_MESH_SHADER_LOD_PRINT_EVIDENCE = "1"
    $evidenceOutput = & $testExecutable 2>&1
    $evidenceExitCode = $LASTEXITCODE
} finally {
    if ($null -eq $previousPrintEvidence) {
        Remove-Item Env:\MK_VULKAN_MAVG_MESH_SHADER_LOD_PRINT_EVIDENCE -ErrorAction SilentlyContinue
    } else {
        $env:MK_VULKAN_MAVG_MESH_SHADER_LOD_PRINT_EVIDENCE = $previousPrintEvidence
    }
}

if ($evidenceExitCode -ne 0) {
    Write-Error "MAVG Vulkan mesh shader indirect evidence command failed with exit code $evidenceExitCode."
}

$evidenceLines = @($evidenceOutput | ForEach-Object { [string]$_ })
$indirectReady = Get-CounterValueFromLines `
    -Lines $evidenceLines `
    -Name "mavg_vulkan_mesh_shader_indirect_dispatch_ready" `
    -DefaultValue "0"
$indirectCountReady = Get-CounterValueFromLines `
    -Lines $evidenceLines `
    -Name "mavg_vulkan_mesh_shader_indirect_count_ready" `
    -DefaultValue "0"
$payloadReady = Get-CounterValueFromLines `
    -Lines $evidenceLines `
    -Name "mavg_vulkan_mesh_shader_payload_consumption_ready" `
    -DefaultValue "0"
$meshLodReady = Get-CounterValueFromLines `
    -Lines $evidenceLines `
    -Name "mavg_mesh_shader_lod_ready" `
    -DefaultValue "1"
$naniteCompatible = Get-CounterValueFromLines `
    -Lines $evidenceLines `
    -Name "mavg_nanite_compatible" `
    -DefaultValue "1"
$naniteEquivalent = Get-CounterValueFromLines `
    -Lines $evidenceLines `
    -Name "mavg_nanite_equivalent" `
    -DefaultValue "1"
$naniteSuperior = Get-CounterValueFromLines `
    -Lines $evidenceLines `
    -Name "mavg_nanite_superior" `
    -DefaultValue "1"
$nativeHandles = Get-CounterValueFromLines `
    -Lines $evidenceLines `
    -Name "mavg_vulkan_native_handles_exposed" `
    -DefaultValue "1"

$ready = $indirectReady -eq "1" -and
    $indirectCountReady -eq "1" -and
    $payloadReady -eq "1" -and
    $meshLodReady -eq "0" -and
    $naniteCompatible -eq "0" -and
    $naniteEquivalent -eq "0" -and
    $naniteSuperior -eq "0" -and
    $nativeHandles -eq "0"

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $evidenceLines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $evidenceLines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG Vulkan mesh shader indirect dispatch is not ready; mesh/task SPIR-V artifacts, Vulkan mesh/task shader support, drawIndirectCount support, indirect/count execution, payload barriers, and non-claim counters are required."
}
