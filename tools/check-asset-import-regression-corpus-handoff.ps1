#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$repoRoot = Get-RepoRoot
$pwshCommand = (Get-Command pwsh -ErrorAction Stop).Source
$plannerScript = Join-Path $PSScriptRoot "plan-asset-import-regression-corpus-handoff.ps1"

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $repoRoot ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

function Write-Utf8TextFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Text
    )

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $parent -Force
    }
    Set-Content -LiteralPath $Path -Value $Text -Encoding utf8NoBOM -NoNewline
}

function New-Directory {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $Path -Force
    }
}

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

function Assert-LineStartsWith {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$ExpectedPrefix,
        [Parameter(Mandatory = $true)][string]$Context
    )

    foreach ($line in $Lines) {
        if ($line.StartsWith($ExpectedPrefix, [System.StringComparison]::Ordinal)) {
            return
        }
    }

    Write-Error "$Context missing expected prefix: $ExpectedPrefix"
}

function Invoke-HandoffPlanner {
    param([string[]]$PlannerArguments = @())

    return @(& $pwshCommand -NoProfile -ExecutionPolicy Bypass -File $plannerScript @PlannerArguments 2>&1)
}

function Write-HandoffReadyLayout {
    param([Parameter(Mandatory = $true)][string]$CorpusRoot)

    foreach ($relativeDirectory in @(
            "expected",
            "notices",
            "sources/gltf",
            "sources/textures",
            "sources/materials",
            "sources/audio",
            "retained/success",
            "retained/failure"
        )) {
        New-Directory -Path (Join-Path $CorpusRoot ($relativeDirectory -replace "/", [System.IO.Path]::DirectorySeparatorChar))
    }

    Write-Utf8TextFile -Path (Join-Path $CorpusRoot "corpus.gecorpus") -Text @'
format=GameEngine.AssetImportRegressionCorpus.v1
corpus.id=GameEngine.AssetImportRegressionCorpus.v1
asset.count=0
'@
    Write-Utf8TextFile -Path (Join-Path $CorpusRoot "expected/hashes.gehashes") -Text "`n"
    Write-Utf8TextFile -Path (Join-Path $CorpusRoot "notices/THIRD_PARTY_ASSET_NOTICES.md") -Text "# Synthetic notices`n"
    Write-Utf8TextFile -Path (Join-Path $CorpusRoot "retained/official-source-ledger.md") -Text "# Synthetic official source ledger`n"
    Write-Utf8TextFile -Path (Join-Path $CorpusRoot "retained/corpus-selection-summary.md") -Text @'
gltf_rows=40
texture_rows=30
material_rows=20
animation_rows=20
audio_rows=20
'@
}

function Write-Report {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$RunId,
        [Parameter(Mandatory = $true)][bool]$Ready,
        [Parameter(Mandatory = $true)][int]$FailedCount,
        [Parameter(Mandatory = $true)][int]$LegalBlockedCount,
        [Parameter(Mandatory = $true)][int]$NondeterministicCount
    )

    $readyText = if ($Ready) { "true" } else { "false" }
    Write-Utf8TextFile -Path $Path -Text @"
format=GameEngine.AssetImportRegressionReport.v1
corpus_id=GameEngine.AssetImportRegressionCorpus.v1
run_id=$RunId
asset_count=$FailedCount
succeeded_count=0
failed_count=$FailedCount
legal_blocked_count=$LegalBlockedCount
nondeterministic_count=$NondeterministicCount
ready=$readyText
row.count=0
"@
}

if (-not (Test-Path -LiteralPath $plannerScript -PathType Leaf)) {
    Write-Error "tools/plan-asset-import-regression-corpus-handoff.ps1 must exist."
}

$contractRootRelative = "out/tmp/asset-import-regression-corpus-handoff-$PID"
$contractRoot = ConvertTo-LocalPath $contractRootRelative
if (Test-Path -LiteralPath $contractRoot) {
    Remove-Item -LiteralPath $contractRoot -Recurse -Force
}

try {
    $missingRootLines = Invoke-HandoffPlanner -PlannerArguments @("-CorpusRoot", "$contractRootRelative/missing-corpus")
    Assert-LinePresent $missingRootLines "asset_import_regression_handoff_status=missing_corpus" "missing corpus handoff"
    Assert-LinePresent $missingRootLines "asset_import_regression_handoff_ready=0" "missing corpus handoff"
    Assert-LinePresent $missingRootLines "asset_import_regression_handoff_next_action=create_or_mount_approved_host_corpus" "missing corpus handoff"
    Assert-LinePresent $missingRootLines "asset_import_regression_handoff_external_engine_claim=0" "missing corpus handoff"
    Assert-LinePresent $missingRootLines "asset_import_regression_handoff_legal_approval_claim=0" "missing corpus handoff"

    $missingRootRequireReadyLines = Invoke-HandoffPlanner -PlannerArguments @(
        "-CorpusRoot",
        "$contractRootRelative/missing-corpus",
        "-RequireReady"
    )
    if ($LASTEXITCODE -eq 0) {
        Write-Error "handoff planner must fail under -RequireReady when corpus root is missing."
    }
    Assert-LinePresent $missingRootRequireReadyLines "asset_import_regression_handoff_diagnostic=require_ready.missing_corpus" "missing corpus require-ready handoff"

    $incompleteRoot = Join-Path $contractRoot "incomplete"
    New-Directory -Path $incompleteRoot
    $incompleteLines = Invoke-HandoffPlanner -PlannerArguments @("-CorpusRoot", "$contractRootRelative/incomplete")
    Assert-LinePresent $incompleteLines "asset_import_regression_handoff_status=corpus_manifest_required" "incomplete corpus handoff"
    Assert-LinePresent $incompleteLines "asset_import_regression_handoff_next_action=generate_manifest" "incomplete corpus handoff"
    Assert-LineStartsWith $incompleteLines "asset_import_regression_handoff_next_command.0=pwsh -NoProfile -ExecutionPolicy Bypass -File tools/generate-asset-import-regression-corpus-manifest.ps1" "incomplete corpus handoff"

    $blockedRoot = Join-Path $contractRoot "blocked-report"
    Write-HandoffReadyLayout -CorpusRoot $blockedRoot
    Write-Report -Path (Join-Path $blockedRoot "report.gereport") `
        -RunId "synthetic-blocked" `
        -Ready $false `
        -FailedCount 3 `
        -LegalBlockedCount 1 `
        -NondeterministicCount 1
    Write-Report -Path (Join-Path $blockedRoot "retained/success/report.gereport") `
        -RunId "synthetic-success" `
        -Ready $true `
        -FailedCount 0 `
        -LegalBlockedCount 0 `
        -NondeterministicCount 0
    Write-Report -Path (Join-Path $blockedRoot "retained/failure/report.gereport") `
        -RunId "synthetic-failure" `
        -Ready $false `
        -FailedCount 3 `
        -LegalBlockedCount 1 `
        -NondeterministicCount 1

    $blockedLines = Invoke-HandoffPlanner -PlannerArguments @("-CorpusRoot", "$contractRootRelative/blocked-report")
    Assert-LinePresent $blockedLines "asset_import_regression_handoff_status=operator_loop_required" "blocked report handoff"
    Assert-LinePresent $blockedLines "asset_import_regression_handoff_failed_count=3" "blocked report handoff"
    Assert-LinePresent $blockedLines "asset_import_regression_handoff_legal_blocked_count=1" "blocked report handoff"
    Assert-LinePresent $blockedLines "asset_import_regression_handoff_nondeterministic_count=1" "blocked report handoff"
    Assert-LinePresent $blockedLines "asset_import_regression_handoff_retained_success_reports=1" "blocked report handoff"
    Assert-LinePresent $blockedLines "asset_import_regression_handoff_retained_failure_reports=1" "blocked report handoff"
    Assert-LineStartsWith $blockedLines "asset_import_regression_handoff_next_command.0=pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-operator-loop.ps1" "blocked report handoff"
} finally {
    if (Test-Path -LiteralPath $contractRoot) {
        Remove-Item -LiteralPath $contractRoot -Recurse -Force
    }
}

Write-Information "asset-import-regression-corpus-handoff-check: ok" -InformationAction Continue
