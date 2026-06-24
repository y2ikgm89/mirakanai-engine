#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-metal-memory-profiling-apple-host-artifacts-v1

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [switch]$RequireReady,

    [ValidateRange(0, 1024)]
    [int]$Jobs = 0,

    [ValidatePattern('^\d{4}-\d{2}-\d{2}-[a-z0-9][a-z0-9-]*$')]
    [string]$TaskId = "2026-06-24-apple-host-artifacts",

    [string]$ArtifactRootRelative = "artifacts/renderer/metal-memory-profiling-host-evidence",

    [string[]]$ExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "apple-host-helpers.ps1")

$root = Get-RepoRoot
Set-Location $root

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function ConvertTo-CounterValue {
    param([Parameter(Mandatory = $true)][string]$Value)

    return ($Value -replace '[^A-Za-z0-9_.-]', '_')
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

function Assert-PathUnderDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $pathFull = [System.IO.Path]::GetFullPath($Path)
    $directoryFull = [System.IO.Path]::GetFullPath($Directory).TrimEnd(
        [char[]]@([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
    $directoryPrefix = $directoryFull + [System.IO.Path]::DirectorySeparatorChar
    if ($pathFull -ne $directoryFull -and
        -not $pathFull.StartsWith($directoryPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "$Description escaped expected directory '$directoryFull': $pathFull"
    }
}

function Remove-GeneratedArtifactPath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$ArtifactRootFull
    )

    Assert-PathUnderDirectory -Path $Path -Directory $ArtifactRootFull `
        -Description "Renderer Metal memory/profiling generated artifact path"
    if (Test-Path -LiteralPath $Path) {
        if ($PSCmdlet.ShouldProcess($Path, "Remove stale generated artifact path")) {
            Remove-Item -LiteralPath $Path -Recurse -Force
        }
    }
}

function Invoke-CapturedTool {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $output = @(& $FilePath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE
    $text = [string]::Join("`n", @($output | ForEach-Object { [string]$_ }))
    if ($exitCode -ne 0) {
        Write-Error "$Description failed with exit code $exitCode.`n$text"
    }
    return $text
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
        Write-Error "Renderer Metal memory/profiling host artifact producer did not emit expected counter(s): $([string]::Join(', ', $missingExpectedCounters))"
    }
}

if (-not (Test-SafeRepoRelativePath -RelativePath $ArtifactRootRelative)) {
    Write-Error "ArtifactRootRelative must be a safe repo-relative path."
}

$artifactRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $ArtifactRootRelative))
$taskRootRelative = "$ArtifactRootRelative/$TaskId"
$taskRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $taskRootRelative))
$workloadId = "renderer_metal_memory_profiling_apple_host_probe"
$workloadRootRelative = "$taskRootRelative/$workloadId"
$workloadRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $workloadRootRelative))

if (-not (Test-IsMacOS)) {
    $hostName = if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Windows)) {
        "windows"
    } elseif ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Linux)) {
        "linux"
    } else {
        "unknown"
    }

    $counterLine = [string]::Join(" ", @(
            "renderer-metal-memory-profiling-host-artifacts:",
            "validation_recipe=renderer-metal-memory-profiling-host-evidence",
            "host=$hostName",
            "host_gate=metal-apple",
            "renderer_metal_memory_profiling_host_artifacts_status=host_gated",
            "renderer_metal_memory_profiling_host_artifacts_ready=0",
            "renderer_metal_memory_profiling_host_artifacts_probe_ready=0",
            "renderer_metal_memory_profiling_host_artifacts_written=0",
            "renderer_metal_memory_profiling_status=host_evidence_required",
            "renderer_metal_memory_profiling_ready=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_commercial_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_environment_ready=0"
        ))
    Write-Host $counterLine
    Assert-ExpectedCounters -CounterText $counterLine
    if ($RequireReady.IsPresent) {
        Write-Error "Renderer Metal memory/profiling host artifacts require macOS with full Xcode, Metal, MTLHeap, MTLResidencySet, MTLCaptureManager, MTLCaptureDescriptor, and MTLCaptureScope."
    }
    Write-Information "renderer-metal-memory-profiling-host-artifacts-check: host-gated" -InformationAction Continue
    return
}

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for renderer Metal memory/profiling host artifact generation."
}
$cmake = Get-CMakeCommand
if (-not $cmake) {
    Write-Error "CMake is required for renderer Metal memory/profiling host artifact generation."
}
if (-not (Get-NinjaCommand)) {
    Write-Error "Ninja is required because the ci-macos-appleclang preset uses the Ninja generator."
}

$developerDirectory = Get-AppleDeveloperDirectory
if (-not (Test-FullXcodeDeveloperDirectory -DeveloperDirectory $developerDirectory)) {
    Write-Error "Renderer Metal memory/profiling host artifacts require full Xcode selection."
}
$xcrun = Find-CommandOnCombinedPath "xcrun"
if (-not $xcrun) {
    Write-Error "xcrun is required for renderer Metal memory/profiling host artifacts."
}
foreach ($tool in @("metal", "metallib")) {
    if (-not (Test-XcrunToolAvailable -Xcrun $xcrun -SdkName "macosx" -ToolName $tool)) {
        Write-Error "xcrun --sdk macosx $tool is required for renderer Metal memory/profiling host artifacts."
    }
}

$jobsToUse = Resolve-ParallelJobCount -Jobs $Jobs
Remove-GeneratedArtifactPath -Path $taskRootFull -ArtifactRootFull $artifactRootFull
$null = New-Item -ItemType Directory -Force -Path $workloadRootFull

Write-Information "renderer-metal-memory-profiling-host-artifacts: configuring ci-macos-appleclang..." `
    -InformationAction Continue
$null = Invoke-CheckedCommand -FilePath $cmake -Arguments @("--preset", "ci-macos-appleclang")

Write-Information "renderer-metal-memory-profiling-host-artifacts: building Apple host probe..." `
    -InformationAction Continue
$null = Invoke-CheckedCommand -FilePath $cmake -Arguments @(
    "--build",
    "--preset",
    "ci-macos-appleclang",
    "--target",
    "MK_metal_memory_profiling_host_artifacts_probe",
    "--parallel",
    [string]$jobsToUse
)

$probePath = Join-Path $root "out/build/ci-macos-appleclang/MK_metal_memory_profiling_host_artifacts_probe"
if (-not (Test-Path -LiteralPath $probePath -PathType Leaf)) {
    Write-Error "Renderer Metal memory/profiling host artifact probe did not build at $probePath"
}

Write-Information "renderer-metal-memory-profiling-host-artifacts: running Apple host probe..." `
    -InformationAction Continue
$probeText = Invoke-CapturedTool -FilePath $probePath -Arguments @($workloadRootFull) `
    -Description "Renderer Metal memory/profiling Apple host probe"

$summaryPath = Join-Path $workloadRootFull "probe-summary.json"
if (-not (Test-Path -LiteralPath $summaryPath -PathType Leaf)) {
    Write-Error "Renderer Metal memory/profiling Apple host probe did not write probe-summary.json"
}
$summary = Get-Content -LiteralPath $summaryPath -Raw | ConvertFrom-Json
$captureArtifactRelativePath = "$workloadRootRelative/$($summary.capture_artifact)"
$captureArtifactFull = [System.IO.Path]::GetFullPath((Join-Path $root $captureArtifactRelativePath))
if (-not (Test-Path -LiteralPath $captureArtifactFull -PathType Leaf)) {
    Write-Error "Renderer Metal memory/profiling Apple host probe did not write capture artifact: $captureArtifactRelativePath"
}

$macosVersion = (& sw_vers -productVersion 2>$null | Select-Object -First 1)
$xcodebuild = Find-CommandOnCombinedPath "xcodebuild"
if (-not $xcodebuild) {
    Write-Error "xcodebuild is required to record the Xcode version."
}
$xcodeVersion = (& $xcodebuild -version 2>$null | Select-Object -First 1)
if ([string]::IsNullOrWhiteSpace($macosVersion) -or [string]::IsNullOrWhiteSpace($xcodeVersion)) {
    Write-Error "Renderer Metal memory/profiling host artifacts require macOS and Xcode version evidence."
}

Write-Information "renderer-metal-memory-profiling-host-artifacts: importing evidence.json..." `
    -InformationAction Continue
$collectorText = Invoke-CapturedTool -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/collect-renderer-metal-memory-profiling-host-evidence.ps1"),
    "-Mode",
    "Import",
    "-EvidenceRoot",
    $ArtifactRootRelative,
    "-WorkloadId",
    [string]$summary.workload_id,
    "-CaptureArtifactRelativePath",
    $captureArtifactRelativePath,
    "-MacosVersion",
    [string]$macosVersion,
    "-XcodeVersion",
    [string]$xcodeVersion,
    "-HeapResourceRows",
    [string]$summary.heap_resource_rows,
    "-HeapAllocatedBytes",
    [string]$summary.heap_allocated_bytes,
    "-ResidentBytes",
    [string]$summary.resident_bytes,
    "-BudgetBytes",
    [string]$summary.budget_bytes,
    "-ResidencySetAllocationRows",
    [string]$summary.residency_set_allocation_rows,
    "-MemoryPressureSampleRows",
    [string]$summary.memory_pressure_sample_rows,
    "-MemoryPressureBudgetStatus",
    [string]$summary.memory_pressure_budget_status,
    "-CaptureScopeLabel",
    [string]$summary.capture_scope_label,
    "-CaptureArtifactRows",
    [string]$summary.capture_artifact_rows
) -Description "Renderer Metal memory/profiling host evidence import"

Write-Information "renderer-metal-memory-profiling-host-artifacts: validating retained evidence..." `
    -InformationAction Continue
$validatorText = Invoke-CapturedTool -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/check-renderer-metal-memory-profiling-host-evidence.ps1"),
    "-ArtifactRootRelative",
    $taskRootRelative,
    "-RequireReady",
    "-SkipFocusedRendererBuild",
    "-ExpectedEvidenceCounters",
    "renderer_metal_memory_profiling_status=ready",
    "renderer_metal_memory_profiling_ready=1",
    "renderer_backend_parity_ready=0",
    "renderer_metal_broad_readiness=0",
    "renderer_commercial_readiness=0",
    "renderer_broad_quality_ready=0"
) -Description "Renderer Metal memory/profiling retained evidence validation"

$counterLine = [string]::Join(" ", @(
        "renderer-metal-memory-profiling-host-artifacts:",
        "validation_recipe=renderer-metal-memory-profiling-host-evidence",
        "host=macos",
        "host_gate=metal-apple",
        "renderer_metal_memory_profiling_host_artifacts_status=ready",
        "renderer_metal_memory_profiling_host_artifacts_ready=1",
        "renderer_metal_memory_profiling_host_artifacts_probe_ready=1",
        "renderer_metal_memory_profiling_host_artifacts_written=1",
        "renderer_metal_memory_profiling_host_artifacts_task_id=$(ConvertTo-CounterValue -Value $TaskId)",
        "renderer_metal_memory_profiling_host_artifacts_capture=$captureArtifactRelativePath",
        "renderer_metal_memory_profiling_status=ready",
        "renderer_metal_memory_profiling_ready=1",
        "renderer_backend_parity_ready=0",
        "renderer_metal_broad_readiness=0",
        "renderer_commercial_readiness=0",
        "renderer_broad_quality_ready=0",
        "renderer_environment_ready=0"
    ))
Write-Host $counterLine
Assert-ExpectedCounters -CounterText ([string]::Join("`n", @($probeText, $collectorText, $validatorText, $counterLine)))

Write-Information "renderer-metal-memory-profiling-host-artifacts-check: ok" -InformationAction Continue
