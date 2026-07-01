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
    if ($Value) { return "1" }
    return "0"
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

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for MAVG DirectStorage Zstd preview review validation."
}

Write-Information "mavg-directstorage-zstd-preview-review: configuring dev preset..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $root "tools/cmake.ps1"), "--preset", "dev"
)

Write-Information "mavg-directstorage-zstd-preview-review: building focused runtime RHI test target..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $root "tools/cmake.ps1"), "--build",
    "--preset", "dev", "--target", "MK_runtime_rhi_mavg_ds_zstd_preview_tests"
)

Write-Information "mavg-directstorage-zstd-preview-review: running focused runtime RHI CTest lane..." `
    -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $root "tools/ctest.ps1"), "--preset",
    "dev", "--output-on-failure", "-R", "MK_runtime_rhi_mavg_ds_zstd_preview_tests"
)

$schemaText = Get-Content -LiteralPath (Join-Path $root "schemas/mavg-directstorage-zstd-preview-review.schema.json") -Raw
foreach ($needle in @(
        "GameEngine.MavgDirectStorageZstdPreviewReview.v1",
        "mavg-directstorage-zstd-preview-review-v1",
        "microsoft-directstorage-api-downloads",
        "microsoft-directstorage-1.4-zstd-preview",
        "microsoft-directstorage-nuget-1.4-preview",
        "microsoft-directstorage-1.3-enqueue-requests",
        "zstd_codec",
        "gacl_shuffle_transform",
        "creator_id",
        "preview_known_issue",
        "zstdCompressionFormatReviewed",
        "gaclBcnPostprocessScopeReviewed",
        "stagingBufferOver256MbKnownIssueReviewed",
        "zstdGpuFallbackShaderTdrKnownIssueReviewed",
        "dllReplacementEnablesFeatures",
        "unityUnrealGodotCompatibility"
    )) {
    Assert-TextContains -Text $schemaText -Needle $needle -Context "mavg DirectStorage Zstd preview schema"
}

$docPaths = @(
    "docs/current-capabilities.md",
    "docs/roadmap.md",
    "docs/superpowers/plans/README.md",
    "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md",
    "engine/agent/manifest.json"
)
foreach ($docPath in $docPaths) {
    $docText = Get-Content -LiteralPath (Join-Path $root $docPath) -Raw
    foreach ($needle in @(
            "mavg-directstorage-zstd-preview-review-v1",
            "mavg_directstorage_zstd_preview_review_ready=1",
            "mavg_directstorage_zstd_preview_selected=1",
            "mavg_directstorage_zstd_preview_ready=0",
            "mavg_directstorage_zstd_execution_ready=0",
            "mavg_directstorage_gacl_pipeline_ready=0",
            "mavg_directstorage_native_handles_exposed=0",
            "mavg_directstorage_performance_ready=0",
            "mavg_package_visible_backend_readiness_ready=0",
            "mavg_broad_cpu_gpu_memory_optimization_ready=0",
            "mavg_nanite_compatible=0",
            "mavg_nanite_equivalent=0",
            "mavg_nanite_superior=0",
            "mavg_external_engine_compatibility=0",
            "RuntimeMavgDirectStorageZstdPreviewResult",
            "evaluate_runtime_mavg_directstorage_zstd_preview_review",
            "DirectStorage 1.4",
            "Zstd",
            "GACL",
            "Unity",
            "Unreal",
            "Godot",
            "Nanite"
        )) {
        Assert-TextContains -Text $docText -Needle $needle -Context $docPath
    }
}

$ready = $true
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-directstorage-zstd-preview-review")
$lines.Add("mavg_directstorage_zstd_preview_review_status=preview_review_ready")
$lines.Add("mavg_directstorage_zstd_preview_review_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_directstorage_zstd_preview_selected=1")
$lines.Add("mavg_directstorage_zstd_preview_ready=0")
$lines.Add("mavg_directstorage_zstd_execution_ready=0")
$lines.Add("mavg_directstorage_gacl_pipeline_ready=0")
$lines.Add("mavg_directstorage_creator_id_policy_reviewed=1")
$lines.Add("mavg_directstorage_native_handles_exposed=0")
$lines.Add("mavg_directstorage_performance_ready=0")
$lines.Add("mavg_package_visible_backend_readiness_ready=0")
$lines.Add("mavg_broad_cpu_gpu_memory_optimization_ready=0")
$lines.Add("mavg_nanite_compatible=0")
$lines.Add("mavg_nanite_equivalent=0")
$lines.Add("mavg_nanite_superior=0")
$lines.Add("mavg_external_engine_compatibility=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG DirectStorage Zstd preview review is incomplete; focused tests, schema, docs, and manifest non-claim evidence are required."
}

Write-Information "mavg-directstorage-zstd-preview-review-check: ok" -InformationAction Continue
