#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10F

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Assemble")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/final-retained",

    [string]$D3d12HostEvidenceRelative = "",
    [string]$VulkanStrictHostEvidenceRelative = "",
    [string]$AppleMetalHostEvidenceRelative = "",
    [string]$MetalMemoryProfilingHostEvidenceRelative = "",
    [string]$PackageHostEvidenceRelative = "",
    [string]$QualityVfxHostEvidenceRelative = "",
    [string]$CleanRoomLegalReviewRelative = "",

    [switch]$RequireReady,
    [switch]$NoWrite
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
        "artifacts/renderer/commercial-readiness-evidence/",
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

function Invoke-RendererTool {
    param(
        [Parameter(Mandatory = $true)][string]$ScriptName,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $scriptPath = Join-Path $PSScriptRoot $ScriptName
    if (-not (Test-Path -LiteralPath $scriptPath -PathType Leaf)) {
        Write-Error "required_tool_missing: $ScriptName"
    }

    return @(& pwsh -NoProfile -ExecutionPolicy Bypass -File $scriptPath @Arguments)
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/commercial-readiness-evidence/."
}

$requiredFinalFiles = 12
$requiredHostInputs = @(
    @{ Name = "d3d12_host_evidence"; Value = $D3d12HostEvidenceRelative; Missing = "d3d12_host_evidence_required" },
    @{ Name = "vulkan_strict_host_evidence"; Value = $VulkanStrictHostEvidenceRelative; Missing = "vulkan_strict_host_evidence_required" },
    @{ Name = "apple_metal_host_evidence"; Value = $AppleMetalHostEvidenceRelative; Missing = "apple_metal_host_evidence_required" },
    @{ Name = "metal_memory_profiling_host_evidence"; Value = $MetalMemoryProfilingHostEvidenceRelative; Missing = "metal_memory_profiling_host_evidence_required" },
    @{ Name = "package_host_evidence"; Value = $PackageHostEvidenceRelative; Missing = "package_host_evidence_required" },
    @{ Name = "quality_vfx_host_evidence"; Value = $QualityVfxHostEvidenceRelative; Missing = "quality_vfx_host_evidence_required" },
    @{ Name = "clean_room_legal_review"; Value = $CleanRoomLegalReviewRelative; Missing = "clean_room_legal_review_required" }
)

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=renderer-commercial-readiness-final-retained-root-assembler"
    Write-Output "renderer_commercial_readiness_final_assembler_mode=Plan"
    Write-Output "renderer_commercial_readiness_final_assembler_output_root=$OutputRootRelative"
    Write-Output "renderer_commercial_readiness_final_assembler_required_host_inputs=$($requiredHostInputs.Count)"
    Write-Output "renderer_commercial_readiness_final_assembler_required_final_files=$requiredFinalFiles"
    Write-Output "renderer_commercial_readiness_final_assembler_writes_evidence=0"
    Write-Output "renderer_commercial_readiness_final_assembler_ready=0"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    Write-Output "renderer_environment_ready=0"
    return
}

$missingInputs = [System.Collections.Generic.List[string]]::new()
foreach ($inputRow in $requiredHostInputs) {
    if ([string]::IsNullOrWhiteSpace([string]$inputRow.Value)) {
        $missingInputs.Add([string]$inputRow.Missing)
        continue
    }
    $null = Resolve-RepoRelativePath -RelativePath ([string]$inputRow.Value) -Label ([string]$inputRow.Name)
}

if ($missingInputs.Count -gt 0) {
    $blocker = $missingInputs -join "+"
    Write-Output "validation_recipe=renderer-commercial-readiness-final-retained-root-assembler"
    Write-Output "renderer_commercial_readiness_final_assembler_mode=Assemble"
    Write-Output "renderer_commercial_readiness_final_assembler_status=blocked"
    Write-Output "renderer_commercial_readiness_final_assembler_ready=0"
    Write-Output "renderer_commercial_readiness_final_assembler_missing_host_inputs=$($missingInputs.Count)"
    Write-Output "renderer_commercial_readiness_final_assembler_blocker=$blocker"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    Write-Output "renderer_environment_ready=0"
    if ($RequireReady.IsPresent) {
        Write-Error "Renderer commercial readiness final retained root assembler is not ready: $($missingInputs -join ', ')"
    }
    return
}

$willWrite = -not $NoWrite.IsPresent
$producerRootRelative = "$OutputRootRelative/_producer-artifacts"

Write-Output "validation_recipe=renderer-commercial-readiness-final-retained-root-assembler"
Write-Output "renderer_commercial_readiness_final_assembler_mode=Assemble"
Write-Output "renderer_commercial_readiness_final_assembler_output_root=$OutputRootRelative"
Write-Output "renderer_commercial_readiness_final_assembler_writes_evidence=$(ConvertTo-CounterBit $willWrite)"

if (-not $willWrite) {
    Write-Output "renderer_commercial_readiness_final_assembler_status=planned"
    Write-Output "renderer_commercial_readiness_final_assembler_ready=0"
    return
}

$producerRuns = @(
    @{
        Script = "collect-renderer-d3d12-commercial-quality-artifact.ps1"
        Args = @("-Mode", "Assemble", "-OutputRootRelative", "$producerRootRelative/d3d12",
            "-D3d12HostEvidenceRelative", $D3d12HostEvidenceRelative)
    },
    @{
        Script = "collect-renderer-vulkan-strict-commercial-quality-artifact.ps1"
        Args = @("-Mode", "Assemble", "-OutputRootRelative", "$producerRootRelative/vulkan-strict",
            "-VulkanStrictHostEvidenceRelative", $VulkanStrictHostEvidenceRelative)
    },
    @{
        Script = "collect-renderer-apple-metal-commercial-quality-artifact.ps1"
        Args = @("-Mode", "Assemble", "-OutputRootRelative", "$producerRootRelative/apple-metal",
            "-AppleMetalHostEvidenceRelative", $AppleMetalHostEvidenceRelative,
            "-MetalMemoryProfilingHostEvidenceRelative", $MetalMemoryProfilingHostEvidenceRelative)
    },
    @{
        Script = "collect-renderer-package-commercial-quality-artifacts.ps1"
        Args = @("-Mode", "Assemble", "-OutputRootRelative", "$producerRootRelative/package",
            "-PackageHostEvidenceRelative", $PackageHostEvidenceRelative)
    },
    @{
        Script = "collect-renderer-quality-vfx-commercial-artifacts.ps1"
        Args = @("-Mode", "Assemble", "-OutputRootRelative", "$producerRootRelative/quality-vfx",
            "-QualityVfxHostEvidenceRelative", $QualityVfxHostEvidenceRelative)
    },
    @{
        Script = "collect-renderer-clean-room-legal-artifact.ps1"
        Args = @("-Mode", "Assemble", "-OutputRootRelative", "$producerRootRelative/clean-room-legal",
            "-CleanRoomLegalReviewRelative", $CleanRoomLegalReviewRelative)
    }
)

foreach ($producerRun in $producerRuns) {
    $producerArgs = [string[]]$producerRun["Args"]
    $producerParameters = @{
        ScriptName = [string]$producerRun["Script"]
        Arguments = $producerArgs
    }
    $producerLines = Invoke-RendererTool @producerParameters
    foreach ($line in $producerLines) {
        Write-Output $line
    }
}

$collectorArgs = @(
    "-Mode", "Assemble",
    "-OutputRootRelative", $OutputRootRelative,
    "-D3d12ArtifactRelative", "$producerRootRelative/d3d12/d3d12-quality.json",
    "-VulkanStrictArtifactRelative", "$producerRootRelative/vulkan-strict/vulkan-strict-quality.json",
    "-AppleMetalArtifactRelative", "$producerRootRelative/apple-metal/apple-metal-host.json",
    "-Visible3dPackageArtifactRelative", "$producerRootRelative/package/visible-3d-package.json",
    "-RuntimeUiPackageArtifactRelative", "$producerRootRelative/package/runtime-ui-package.json",
    "-EnvironmentPackageArtifactRelative", "$producerRootRelative/package/environment-package.json",
    "-GeneratedGamePackageArtifactRelative", "$producerRootRelative/package/generated-game-package.json",
    "-RendererQualityMatrixArtifactRelative", "$producerRootRelative/quality-vfx/renderer-quality-matrix.json",
    "-ProductionVfxProfilingArtifactRelative", "$producerRootRelative/quality-vfx/production-vfx-profiling.json",
    "-MetalMemoryHostEvidenceRelative", $MetalMemoryProfilingHostEvidenceRelative,
    "-CleanRoomLegalArtifactRelative", "$producerRootRelative/clean-room-legal/clean-room-legal.json"
)

$collectorToolArgs = [string[]]$collectorArgs
$collectorLines = Invoke-RendererTool -ScriptName "collect-renderer-commercial-readiness-evidence.ps1" `
    -Arguments $collectorToolArgs
foreach ($line in $collectorLines) {
    Write-Output $line
}

$preflightArgs = @("-ArtifactRootRelative", $OutputRootRelative)
if ($RequireReady.IsPresent) {
    $preflightArgs += "-RequireReady"
}
$preflightToolArgs = [string[]]$preflightArgs
$preflightLines = Invoke-RendererTool -ScriptName "validate-renderer-commercial-readiness-final-promotion-preflight.ps1" `
    -Arguments $preflightToolArgs
foreach ($line in $preflightLines) {
    Write-Output $line
}

$readyLine = $preflightLines | Where-Object {
    $_ -eq "renderer_commercial_readiness_final_preflight_ready=1"
} | Select-Object -First 1
$ready = -not [string]::IsNullOrWhiteSpace([string]$readyLine)

Write-Output "renderer_commercial_readiness_final_assembler_status=$(if ($ready) { "ready" } else { "blocked" })"
Write-Output "renderer_commercial_readiness_final_assembler_ready=$(ConvertTo-CounterBit $ready)"
Write-Information "renderer-commercial-readiness-final-retained-root-assembler: ok" -InformationAction Continue
