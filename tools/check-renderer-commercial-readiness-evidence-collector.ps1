#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function Assert-LinePresent {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$ExpectedLine,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Lines.Contains($ExpectedLine)) {
        Write-Error "$Context missing expected line: $ExpectedLine"
    }
}

function Get-LineByPrefix {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$Prefix,
        [Parameter(Mandatory = $true)][string]$Context
    )

    $matchesForPrefix = @($Lines | Where-Object { [string]$_ -like "$Prefix*" })
    if ($matchesForPrefix.Count -ne 1) {
        Write-Error "$Context expected one line with prefix '$Prefix' but found $($matchesForPrefix.Count)."
    }
    return [string]$matchesForPrefix[0]
}

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

function ConvertFrom-KeyValueLines {
    param([string[]]$Lines = @())

    $values = @{}
    foreach ($line in @($Lines)) {
        $text = [string]$line
        $separator = $text.IndexOf("=")
        if ($separator -le 0) {
            continue
        }
        $values[$text.Substring(0, $separator)] = $text.Substring($separator + 1)
    }
    return $values
}

$collectorScript = Join-Path $root "tools/collect-renderer-commercial-readiness-evidence.ps1"
if (-not (Test-Path -LiteralPath $collectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-commercial-readiness-evidence.ps1 must exist for Task 10 retained artifact assembly."
}

$expectedNoPromotionLine = "renderer_commercial_readiness=0"
$fixtureRoot = "tests/fixtures/renderer/commercial-readiness-evidence/ready"
$evidenceRootRelative = "artifacts/renderer/commercial-readiness-evidence/collector-contract-$PID"
$outputRootRelative = "$evidenceRootRelative/copied-fixture-root"
$evidenceRootPath = ConvertTo-LocalPath $evidenceRootRelative
if (Test-Path -LiteralPath $evidenceRootPath) {
    Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
}

$collectorArguments = @{
    OutputRootRelative = $outputRootRelative
    D3d12ArtifactRelative = "$fixtureRoot/d3d12-quality.json"
    VulkanStrictArtifactRelative = "$fixtureRoot/vulkan-strict-quality.json"
    AppleMetalArtifactRelative = "$fixtureRoot/apple-metal-host.json"
    Visible3dPackageArtifactRelative = "$fixtureRoot/visible-3d-package.json"
    RuntimeUiPackageArtifactRelative = "$fixtureRoot/runtime-ui-package.json"
    EnvironmentPackageArtifactRelative = "$fixtureRoot/environment-package.json"
    GeneratedGamePackageArtifactRelative = "$fixtureRoot/generated-game-package.json"
    RendererQualityMatrixArtifactRelative = "$fixtureRoot/renderer-quality-matrix.json"
    ProductionVfxProfilingArtifactRelative = "$fixtureRoot/production-vfx-profiling.json"
    MetalMemoryResidencyArtifactRelative = "$fixtureRoot/metal-memory-residency.json"
    MetalProfilingCaptureArtifactRelative = "$fixtureRoot/metal-profiling-capture.json"
    OfficialDocsOnlyReviewReady = $true
    LegalReviewReady = $true
    ExternalEngineZeroMaterialReviewReady = $true
    ThirdPartyNoticesComplete = $true
}

$planLines = @(& $collectorScript -Mode Plan -OutputRootRelative $outputRootRelative)
Assert-LinePresent $planLines `
    "renderer_commercial_readiness_evidence_collector_mode=Plan" `
    "collector Plan mode"
Assert-LinePresent $planLines `
    "renderer_commercial_readiness_evidence_collector_writes_evidence=0" `
    "collector Plan mode"
Assert-LinePresent $planLines `
    "renderer_commercial_readiness_evidence_collector_required_artifact_rows=11" `
    "collector Plan mode"
Assert-LinePresent $planLines `
    "renderer_commercial_readiness_evidence_collector_commercial_renderer=0" `
    "collector Plan mode"

$rejectLines = @()
$rejectFailed = $false
try {
    $rejectLines = @(& $collectorScript -Mode Assemble @collectorArguments 2>&1)
}
catch {
    $rejectFailed = $true
    $rejectLines = @([string]$_.Exception.Message)
}
if (-not $rejectFailed) {
    Write-Error "collector Assemble mode must reject fixture artifacts unless self-test allowance is explicit."
}
if (-not (($rejectLines -join "`n").Contains("fixture_artifact_input_rejected"))) {
    Write-Error "collector fixture rejection did not report fixture_artifact_input_rejected."
}

$assembleLines = @(& $collectorScript -Mode Assemble @collectorArguments -AllowFixtureArtifactsForSelfTest)
Assert-LinePresent $assembleLines `
    "renderer_commercial_readiness_evidence_collector_mode=Assemble" `
    "collector Assemble mode"
Assert-LinePresent $assembleLines `
    "renderer_commercial_readiness_evidence_collector_written=1" `
    "collector Assemble mode"
Assert-LinePresent $assembleLines `
    "renderer_commercial_readiness_evidence_collector_artifact_rows=11" `
    "collector Assemble mode"
Assert-LinePresent $assembleLines `
    "renderer_commercial_readiness_evidence_collector_fixture_artifacts=11" `
    "collector Assemble mode"
Assert-LinePresent $assembleLines `
    "renderer_commercial_readiness_evidence_collector_real_promotion_candidate=0" `
    "collector Assemble mode"
Assert-LinePresent $assembleLines `
    "renderer_commercial_readiness_evidence_collector_native_handles_exposed=0" `
    "collector Assemble mode"
Assert-LinePresent $assembleLines `
    "renderer_commercial_readiness_evidence_collector_external_engine_parity=0" `
    "collector Assemble mode"

$evidencePath = ConvertTo-LocalPath "$outputRootRelative/evidence.json"
if (-not (Test-Path -LiteralPath $evidencePath -PathType Leaf)) {
    Write-Error "collector Assemble mode did not write evidence.json."
}
$evidence = Get-Content -LiteralPath $evidencePath -Raw | ConvertFrom-Json
if ([string]$evidence.schema_version -ne "GameEngine.RendererCommercialReadinessEvidence.v1") {
    Write-Error "collector evidence schema_version mismatch."
}
if ([string]$evidence.backend_rows.d3d12.artifact_path -ne "$outputRootRelative/d3d12-quality.json") {
    Write-Error "collector d3d12 artifact_path mismatch."
}
if ([string]$evidence.metal_memory_profiling_rows.memory_residency.artifact_path -ne
    "$outputRootRelative/metal-memory-residency.json") {
    Write-Error "collector metal memory artifact_path mismatch."
}

$d3d12HashLine = Get-LineByPrefix `
    -Lines $assembleLines `
    -Prefix "renderer_commercial_readiness_evidence_collector_d3d12_hash=" `
    -Context "collector Assemble mode"
$d3d12Hash = $d3d12HashLine.Substring(
    "renderer_commercial_readiness_evidence_collector_d3d12_hash=".Length)
if ($d3d12Hash -cnotmatch "^[0-9a-f]{64}$") {
    Write-Error "collector emitted invalid d3d12 hash: $d3d12Hash"
}
if ([string]$evidence.backend_rows.d3d12.artifact_hash_sha256 -ne $d3d12Hash) {
    Write-Error "collector evidence d3d12 hash mismatch."
}

$validationLines = @(& (Join-Path $PSScriptRoot "validate-renderer-commercial-readiness-evidence.ps1") `
        -ArtifactRootRelative $outputRootRelative)
$validationValues = ConvertFrom-KeyValueLines -Lines $validationLines
if ("renderer_commercial_readiness=$($validationValues["renderer_commercial_readiness"])" -cne
    $expectedNoPromotionLine) {
    Write-Error "copied fixture collector output must not promote renderer_commercial_readiness."
}
if ([string]$validationValues["renderer_commercial_readiness_fixture_artifacts_rejected"] -ne "11") {
    Write-Error "copied fixture collector output must be rejected by fixture guard."
}

if (Test-Path -LiteralPath $evidenceRootPath) {
    Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
}

Write-Information "renderer-commercial-readiness-evidence-collector-check: ok" -InformationAction Continue
