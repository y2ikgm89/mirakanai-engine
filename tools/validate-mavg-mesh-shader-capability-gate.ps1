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

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Get-CounterValueFromText {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$DefaultValue
    )

    $match = [regex]::Match($Text, "(^|\s)$([regex]::Escape($Name))=([^\s]+)")
    if ($match.Success) {
        return $match.Groups[2].Value
    }
    return $DefaultValue
}

function Assert-TextContains {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Text.Contains($Needle)) {
        Write-Error "$Context must contain '$Needle'."
    }
}

$isWindowsHost = [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
    [System.Runtime.InteropServices.OSPlatform]::Windows)

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for MAVG mesh shader capability gate validation."
}

Write-Information "mavg-mesh-shader-capability-gate: configuring dev preset..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--preset",
    "dev"
)

Write-Information "mavg-mesh-shader-capability-gate: building focused runtime RHI test target..." `
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
    "MK_runtime_rhi_mavg_mesh_shader_capability_gate_tests"
)

Write-Information "mavg-mesh-shader-capability-gate: running focused runtime RHI CTest lane..." `
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
    "MK_runtime_rhi_mavg_mesh_shader_capability_gate_tests"
)

if ($isWindowsHost) {
    Write-Information "mavg-mesh-shader-capability-gate: configuring desktop-runtime preset..." `
        -InformationAction Continue
    Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $root "tools/cmake.ps1"),
        "--preset",
        "desktop-runtime"
    )

    Write-Information "mavg-mesh-shader-capability-gate: building package smoke target..." `
        -InformationAction Continue
    Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $root "tools/cmake.ps1"),
        "--build",
        "--preset",
        "desktop-runtime",
        "--target",
        "sample_desktop_runtime_game"
    )
}

$packageSmokeOutput = @()
$packageSmokeExitCode = 1
if ($isWindowsHost) {
    $packageSmokeExe = Join-Path $root "out/build/desktop-runtime/games/Debug/sample_desktop_runtime_game/sample_desktop_runtime_game.exe"
    if (Test-Path -LiteralPath $packageSmokeExe -PathType Leaf) {
        $packageSmokeOutput = & $packageSmokeExe `
            "--smoke" `
            "--max-frames" `
            "1" `
            "--require-mavg-mesh-shader-capability-gate" 2>&1
        $packageSmokeExitCode = $LASTEXITCODE
    }
}
$packageSmokeText = [string]::Join("`n", @($packageSmokeOutput))

$hostReady = $isWindowsHost
$packageSmokeReady = $packageSmokeExitCode -eq 0
$status = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_status" `
    -DefaultValue "missing"
$gateReadyCounter = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_ready" `
    -DefaultValue "0"
$backendRows = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_backend_rows" `
    -DefaultValue "0"
$readyBackends = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_ready_backends" `
    -DefaultValue "0"
$d3d12Ready = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_d3d12_ready" `
    -DefaultValue "0"
$vulkanReady = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_vulkan_ready" `
    -DefaultValue "0"
$featureQueryRows = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_feature_query_rows" `
    -DefaultValue "0"
$pipelineStatisticsRows = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_pipeline_statistics_rows" `
    -DefaultValue "0"
$fallbackReady = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_fallback_ready" `
    -DefaultValue "0"
$fallbackToConventional = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_fallback_to_conventional_indexed_draws" `
    -DefaultValue "0"
$lodReady = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_lod_ready" `
    -DefaultValue "1"
$lodD3d12Ready = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_lod_d3d12_ready" `
    -DefaultValue "1"
$lodVulkanReady = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_lod_vulkan_ready" `
    -DefaultValue "1"
$nativeHandles = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_native_handles_exposed" `
    -DefaultValue "1"
$meshShaderExecution = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_mesh_shader_execution" `
    -DefaultValue "1"
$backendExecution = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_backend_execution" `
    -DefaultValue "1"
$metalReadiness = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_metal_readiness" `
    -DefaultValue "1"
$naniteEquivalence = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_nanite_equivalence" `
    -DefaultValue "1"
$broadBackendReadiness = Get-CounterValueFromText `
    -Text $packageSmokeText `
    -Name "mavg_mesh_shader_capability_gate_broad_backend_readiness" `
    -DefaultValue "1"

$planText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-06-21-mavg-mesh-shader-capability-gate-v1.md") -Raw
$capabilitiesText = Get-Content -LiteralPath (Join-Path $root "docs/current-capabilities.md") -Raw
$roadmapText = Get-Content -LiteralPath (Join-Path $root "docs/roadmap.md") -Raw
$manifestText = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.json") -Raw

foreach ($docCheck in @(
        @{ Text = $planText; Context = "MAVG mesh shader capability gate plan" },
        @{ Text = $capabilitiesText; Context = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Context = "docs/roadmap.md" },
        @{ Text = $manifestText; Context = "engine/agent/manifest.json" }
    )) {
    Assert-TextContains -Text $docCheck.Text -Needle "mavg-mesh-shader-capability-gate-v1" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "RuntimeMavgMeshShaderCapabilityGateResult" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "D3D12_FEATURE_D3D12_OPTIONS7" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "VkPhysicalDeviceMeshShaderFeaturesEXT" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "VkPhysicalDeviceMeshShaderPropertiesEXT" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "mavg_mesh_shader_capability_gate_ready=1" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "mesh shader execution remains 0" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "native handles" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "Metal readiness" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "Nanite" -Context $docCheck.Context
    Assert-TextContains -Text $docCheck.Text -Needle "broad MAVG backend readiness" -Context $docCheck.Context
}

$ready = $hostReady -and
    $packageSmokeReady -and
    $status -eq "ready" -and
    $gateReadyCounter -eq "1" -and
    $backendRows -eq "2" -and
    $readyBackends -eq "2" -and
    $d3d12Ready -eq "1" -and
    $vulkanReady -eq "1" -and
    $featureQueryRows -eq "2" -and
    $pipelineStatisticsRows -eq "2" -and
    $fallbackReady -eq "1" -and
    $fallbackToConventional -eq "1" -and
    $lodReady -eq "0" -and
    $lodD3d12Ready -eq "0" -and
    $lodVulkanReady -eq "0" -and
    $nativeHandles -eq "0" -and
    $meshShaderExecution -eq "0" -and
    $backendExecution -eq "0" -and
    $metalReadiness -eq "0" -and
    $naniteEquivalence -eq "0" -and
    $broadBackendReadiness -eq "0"

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-mesh-shader-capability-gate")
$lines.Add("mavg_mesh_shader_capability_gate_status=$(if ($ready) { 'ready' } else { 'blocked' })")
$lines.Add("mavg_mesh_shader_capability_gate_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_mesh_shader_capability_gate_windows_host=$(ConvertTo-CounterBit $hostReady)")
$lines.Add("mavg_mesh_shader_capability_gate_package_smoke_ready=$(ConvertTo-CounterBit $packageSmokeReady)")
$lines.Add("mavg_mesh_shader_capability_gate_backend_rows=$backendRows")
$lines.Add("mavg_mesh_shader_capability_gate_ready_backends=$readyBackends")
$lines.Add("mavg_mesh_shader_capability_gate_d3d12_ready=$d3d12Ready")
$lines.Add("mavg_mesh_shader_capability_gate_vulkan_ready=$vulkanReady")
$lines.Add("mavg_mesh_shader_capability_gate_feature_query_rows=$featureQueryRows")
$lines.Add("mavg_mesh_shader_capability_gate_pipeline_statistics_rows=$pipelineStatisticsRows")
$lines.Add("mavg_mesh_shader_capability_gate_fallback_ready=$fallbackReady")
$lines.Add("mavg_mesh_shader_capability_gate_fallback_to_conventional_indexed_draws=$fallbackToConventional")
$lines.Add("mavg_mesh_shader_capability_gate_lod_ready=$lodReady")
$lines.Add("mavg_mesh_shader_capability_gate_lod_d3d12_ready=$lodD3d12Ready")
$lines.Add("mavg_mesh_shader_capability_gate_lod_vulkan_ready=$lodVulkanReady")
$lines.Add("mavg_mesh_shader_capability_gate_native_handles_exposed=$nativeHandles")
$lines.Add("mavg_mesh_shader_capability_gate_mesh_shader_execution=$meshShaderExecution")
$lines.Add("mavg_mesh_shader_capability_gate_backend_execution=$backendExecution")
$lines.Add("mavg_mesh_shader_capability_gate_metal_readiness=$metalReadiness")
$lines.Add("mavg_mesh_shader_capability_gate_nanite_equivalence=$naniteEquivalence")
$lines.Add("mavg_mesh_shader_capability_gate_broad_backend_readiness=$broadBackendReadiness")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG mesh shader capability gate is incomplete; focused CTest, package smoke counters, fallback rows, and non-claim docs/manifest evidence are required."
}
