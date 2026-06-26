#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10F.2

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [ValidateSet("Plan", "Generate")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/d3d12-commercial-quality-host-evidence/renderer-commercial-readiness/supplement",

    [string]$BuildPreset = "dev",

    [string]$BuildConfig = "Debug",

    [switch]$NoBuild,

    [switch]$RequireReady,

    [string[]]$ExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Test-SafeRepoRelativePath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        return $false
    }
    if ($RelativePath.Contains("\")) {
        return $false
    }
    if ([System.IO.Path]::IsPathRooted($RelativePath)) {
        return $false
    }
    if ($RelativePath -match "^[A-Za-z]:") {
        return $false
    }
    if ($RelativePath.Contains(":")) {
        return $false
    }
    if ($RelativePath -match "(^|/)\.\.(/|$)") {
        return $false
    }
    return $true
}

function Test-AllowedOutputRoot {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    return $normalizedPath.StartsWith(
        "artifacts/renderer/d3d12-commercial-quality-host-evidence/",
        [System.StringComparison]::Ordinal)
}

function Resolve-RepoRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not (Test-SafeRepoRelativePath -RelativePath $RelativePath)) {
        Write-Error "unsafe_relative_path: $Label must be repo-relative without absolute, drive-qualified, colon, backslash, or '..' segments."
    }
    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    $rootWithSeparator = $root.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "unsafe_relative_path: $Label must resolve under the repository root."
    }
    return $fullPath
}

function Assert-ExpectedCounters {
    param([Parameter(Mandatory = $true)][string]$CounterText)

    $missingExpectedCounters = @()
    foreach ($counter in @($ExpectedEvidenceCounters)) {
        if (-not $CounterText.Contains([string]$counter)) {
            $missingExpectedCounters += [string]$counter
        }
    }
    if ($missingExpectedCounters.Count -ne 0) {
        Write-Error "Renderer D3D12 host supplement producer did not emit expected counter(s): $([string]::Join(', ', $missingExpectedCounters))"
    }
}

function Invoke-CapturedToolResult {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$Arguments = @()
    )

    $output = @(& $FilePath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE
    $text = [string]::Join("`n", @($output | ForEach-Object { [string]$_ }))
    return [pscustomobject]@{
        ExitCode = $exitCode
        Text = $text
    }
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/d3d12-commercial-quality-host-evidence/."
}

if ($Mode -eq "Plan") {
    $planLines = @(
            "validation_recipe=renderer-d3d12-commercial-quality-host-supplement",
            "renderer_d3d12_commercial_quality_host_supplement_mode=Plan",
            "renderer_d3d12_commercial_quality_host_supplement_output_root=$OutputRootRelative",
            "renderer_d3d12_commercial_quality_host_supplement_probe_ready=0",
            "renderer_d3d12_commercial_quality_host_supplement_ready=0",
            "renderer_d3d12_commercial_quality_host_supplement_written=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )
    foreach ($line in $planLines) {
        Write-Output $line
    }
    Assert-ExpectedCounters -CounterText ([string]::Join("`n", $planLines))
    return
}

if (-not [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::Windows)) {
    $counterLine = [string]::Join(" ", @(
            "validation_recipe=renderer-d3d12-commercial-quality-host-supplement",
            "host_gate=d3d12-windows-primary",
            "renderer_d3d12_commercial_quality_host_supplement_status=host_gated",
            "renderer_d3d12_commercial_quality_host_supplement_probe_ready=0",
            "renderer_d3d12_commercial_quality_host_supplement_ready=0",
            "renderer_d3d12_commercial_quality_host_supplement_written=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        ))
    Write-Output $counterLine
    Assert-ExpectedCounters -CounterText $counterLine
    if ($RequireReady.IsPresent) {
        Write-Error "Renderer D3D12 host supplement generation requires a Windows host with Direct3D 12, WARP or hardware D3D12, debug layers, GPU-based validation, and D3D12 residency APIs."
    }
    return
}

$outputRootFull = Resolve-RepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
if ($PSCmdlet.ShouldProcess($outputRootFull, "Create D3D12 host supplement output root")) {
    $null = New-Item -ItemType Directory -Force -Path $outputRootFull
}
foreach ($staleFile in @("d3d12-host-supplement.json", "host-gate-summary.json", "probe-summary.json")) {
    $stalePath = Join-Path $outputRootFull $staleFile
    if (Test-Path -LiteralPath $stalePath -PathType Leaf) {
        if ($PSCmdlet.ShouldProcess($stalePath, "Remove stale D3D12 host supplement probe output")) {
            Remove-Item -LiteralPath $stalePath -Force
        }
    }
}

$buildDirectory = Resolve-CMakeBuildDirectory -Arguments @("--build", "--preset", $BuildPreset)
if ([string]::IsNullOrWhiteSpace($buildDirectory)) {
    Write-Error "D3D12 host supplement producer could not resolve CMake build directory for build preset '$BuildPreset'."
}

if (-not $NoBuild.IsPresent) {
    $tools = Assert-CppBuildTools
    Invoke-CheckedCommand $tools.CMake --preset $BuildPreset
    Invoke-CheckedCommand $tools.CMake --build --preset $BuildPreset --target MK_d3d12_commercial_quality_host_supplement_probe
}

$probeCandidates = @(
    (Join-Path (Join-Path $buildDirectory $BuildConfig) "MK_d3d12_commercial_quality_host_supplement_probe.exe"),
    (Join-Path $buildDirectory "MK_d3d12_commercial_quality_host_supplement_probe.exe")
)
$probePath = @($probeCandidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1)
if ($probePath.Count -eq 0) {
    Write-Error "D3D12 host supplement probe executable was not found. Build target MK_d3d12_commercial_quality_host_supplement_probe first or omit -NoBuild."
}

$probeResult = Invoke-CapturedToolResult -FilePath ([string]$probePath[0]) -Arguments @($outputRootFull)
$supplementFull = Join-Path $outputRootFull "d3d12-host-supplement.json"
$hostGateFull = Join-Path $outputRootFull "host-gate-summary.json"
$supplementReady = $probeResult.ExitCode -eq 0 -and (Test-Path -LiteralPath $supplementFull -PathType Leaf)

if (-not $supplementReady) {
    $counterLine = [string]::Join(" ", @(
            "validation_recipe=renderer-d3d12-commercial-quality-host-supplement",
            "renderer_d3d12_commercial_quality_host_supplement_mode=Generate",
            "renderer_d3d12_commercial_quality_host_supplement_status=host_evidence_required",
            "renderer_d3d12_commercial_quality_host_supplement_output_root=$OutputRootRelative",
            "renderer_d3d12_commercial_quality_host_supplement_probe_ready=0",
            "renderer_d3d12_commercial_quality_host_supplement_ready=0",
            "renderer_d3d12_commercial_quality_host_supplement_written=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        ))
    Write-Output $counterLine
    if (Test-Path -LiteralPath $hostGateFull -PathType Leaf) {
        Write-Output "renderer_d3d12_commercial_quality_host_supplement_host_gate_summary=$OutputRootRelative/host-gate-summary.json"
    }
    Assert-ExpectedCounters -CounterText ([string]::Join("`n", @($probeResult.Text, $counterLine)))
    if ($RequireReady.IsPresent) {
        Write-Error "Renderer D3D12 host supplement probe did not produce ready supplemental evidence.`n$($probeResult.Text)"
    }
    $global:LASTEXITCODE = 0
    return
}

$supplementHash = (Get-FileHash -LiteralPath $supplementFull -Algorithm SHA256).Hash.ToLowerInvariant()
$counterLine = [string]::Join(" ", @(
        "validation_recipe=renderer-d3d12-commercial-quality-host-supplement",
        "renderer_d3d12_commercial_quality_host_supplement_mode=Generate",
        "renderer_d3d12_commercial_quality_host_supplement_status=ready",
        "renderer_d3d12_commercial_quality_host_supplement_output_root=$OutputRootRelative",
        "renderer_d3d12_commercial_quality_host_supplement_path=$OutputRootRelative/d3d12-host-supplement.json",
        "renderer_d3d12_commercial_quality_host_supplement_hash=$supplementHash",
        "renderer_d3d12_commercial_quality_host_supplement_probe_ready=1",
        "renderer_d3d12_commercial_quality_host_supplement_ready=1",
        "renderer_d3d12_commercial_quality_host_supplement_written=1",
        "renderer_backend_parity_ready=0",
        "renderer_metal_broad_readiness=0",
        "renderer_broad_quality_ready=0",
        "renderer_commercial_readiness=0",
        "renderer_environment_ready=0"
    ))
Write-Output $counterLine
Assert-ExpectedCounters -CounterText ([string]::Join("`n", @($probeResult.Text, $counterLine)))

Write-Information "renderer-d3d12-commercial-quality-host-supplement-generator: ok" -InformationAction Continue
