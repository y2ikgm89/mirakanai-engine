#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string]$CorpusRoot = "out/host-artifacts/asset-import-regression-corpus",
    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$repoRoot = Get-RepoRoot
$pwshCommand = (Get-Command pwsh -ErrorAction Stop).Source

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $repoRoot ($Path -replace "/", [System.IO.Path]::DirectorySeparatorChar)))
}

function ConvertTo-CommandPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $rootFull = [System.IO.Path]::GetFullPath($repoRoot)
    $comparison = [System.StringComparison]::Ordinal
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        $comparison = [System.StringComparison]::OrdinalIgnoreCase
    }

    $rootWithSeparator = $rootFull.TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    ) + [System.IO.Path]::DirectorySeparatorChar
    if ($fullPath.StartsWith($rootWithSeparator, $comparison)) {
        return [System.IO.Path]::GetRelativePath($rootFull, $fullPath).Replace("\", "/")
    }

    return $fullPath.Replace("\", "/")
}

function Read-KeyValueDocument {
    param([Parameter(Mandatory = $true)][string]$Path)

    $values = [ordered]@{}
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $values
    }

    $text = ConvertTo-LfText (Get-Content -LiteralPath $Path -Encoding utf8 -Raw)
    foreach ($line in $text.Split("`n")) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }
        $separator = $line.IndexOf("=")
        if ($separator -lt 1) {
            continue
        }
        $key = $line.Substring(0, $separator)
        if (-not $values.Contains($key)) {
            $values[$key] = $line.Substring($separator + 1)
        }
    }

    return $values
}

function Get-ValueOrDefault {
    param(
        [Parameter(Mandatory = $true)][System.Collections.IDictionary]$Values,
        [Parameter(Mandatory = $true)][string]$Key,
        [Parameter(Mandatory = $true)][string]$DefaultValue
    )

    if ($Values.Contains($Key)) {
        return [string]$Values[$Key]
    }

    return $DefaultValue
}

function ConvertTo-NonNegativeInt {
    param([Parameter(Mandatory = $true)][string]$Value)

    $parsed = 0
    if ([int]::TryParse($Value, [ref]$parsed) -and $parsed -ge 0) {
        return $parsed
    }

    return 0
}

function Read-ReportSummary {
    param([Parameter(Mandatory = $true)][string]$Path)

    $values = Read-KeyValueDocument -Path $Path
    $readyText = Get-ValueOrDefault -Values $values -Key "ready" -DefaultValue "false"
    $failedCount = ConvertTo-NonNegativeInt -Value (Get-ValueOrDefault -Values $values -Key "failed_count" -DefaultValue "0")
    $legalBlockedCount = ConvertTo-NonNegativeInt -Value (Get-ValueOrDefault -Values $values -Key "legal_blocked_count" -DefaultValue "0")
    $nondeterministicCount = ConvertTo-NonNegativeInt -Value (Get-ValueOrDefault -Values $values -Key "nondeterministic_count" -DefaultValue "0")

    return [pscustomobject]@{
        Ready                 = ($readyText -eq "true")
        FailedCount           = $failedCount
        LegalBlockedCount     = $legalBlockedCount
        NondeterministicCount = $nondeterministicCount
        IsSuccessReport       = ($readyText -eq "true" -and $failedCount -eq 0 -and $legalBlockedCount -eq 0 -and $nondeterministicCount -eq 0)
        IsFailureReport       = ($readyText -ne "true" -or $failedCount -gt 0 -or $legalBlockedCount -gt 0 -or $nondeterministicCount -gt 0)
    }
}

function Get-RetainedReportSummary {
    param([Parameter(Mandatory = $true)][string]$ResolvedCorpusRoot)

    $retainedRoot = Join-Path $ResolvedCorpusRoot "retained"
    $reportFiles = @()
    if (Test-Path -LiteralPath $retainedRoot -PathType Container) {
        $reportFiles = @(Get-ChildItem -LiteralPath $retainedRoot -Recurse -Filter "report.gereport" -File | Sort-Object FullName)
    }

    $successReports = 0
    $failureReports = 0
    foreach ($reportFile in $reportFiles) {
        $summary = Read-ReportSummary -Path $reportFile.FullName
        if ($summary.IsSuccessReport) {
            $successReports += 1
        }
        if ($summary.IsFailureReport) {
            $failureReports += 1
        }
    }

    return [pscustomobject]@{
        ReportCount    = $reportFiles.Count
        SuccessReports = $successReports
        FailureReports = $failureReports
    }
}

function Test-RunnerPresent {
    $knownRunnerPath = Join-Path $repoRoot "out/build/dev/Debug/MK_asset_import_regression_runner.exe"
    if (Test-Path -LiteralPath $knownRunnerPath -PathType Leaf) {
        return $true
    }

    $buildRoot = Join-Path $repoRoot "out/build/dev"
    if (-not (Test-Path -LiteralPath $buildRoot -PathType Container)) {
        return $false
    }

    return @(Get-ChildItem -LiteralPath $buildRoot -Recurse -Filter "MK_asset_import_regression_runner.exe" -File -ErrorAction SilentlyContinue).Count -gt 0
}

function Get-NextCommands {
    param(
        [Parameter(Mandatory = $true)][string]$NextAction,
        [Parameter(Mandatory = $true)][string]$CommandCorpusRoot
    )

    $sourcesRoot = "$CommandCorpusRoot/sources"
    $noticesPath = "$CommandCorpusRoot/notices/THIRD_PARTY_ASSET_NOTICES.md"
    $manifestPath = "$CommandCorpusRoot/corpus.gecorpus"
    $outputRoot = "out/asset-import-regression/staging/real-corpus"
    switch ($NextAction) {
        "generate_manifest" {
            return @("pwsh -NoProfile -ExecutionPolicy Bypass -File tools/generate-asset-import-regression-corpus-manifest.ps1 -CorpusRoot $CommandCorpusRoot -SourcesRoot $sourcesRoot -NoticesPath $noticesPath -OutputManifest $manifestPath -FailOnMissingNotice")
        }
        "complete_notices" {
            return @("review and complete $noticesPath before manifest generation or corpus execution")
        }
        "populate_canonical_sources" {
            return @("populate $CommandCorpusRoot/sources/gltf $CommandCorpusRoot/sources/textures $CommandCorpusRoot/sources/materials $CommandCorpusRoot/sources/audio with approved host-owned assets")
        }
        "generate_expected_hashes" {
            return @("pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-asset-import-regression-corpus.ps1 -CorpusRoot $CommandCorpusRoot")
        }
        "complete_official_source_ledger_and_selection_summary" {
            return @("complete $CommandCorpusRoot/retained/official-source-ledger.md and $CommandCorpusRoot/retained/corpus-selection-summary.md")
        }
        "bootstrap_and_build_asset_importers" {
            return @(
                "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1 -Feature asset-importers",
                "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1"
            )
        }
        "run_corpus" {
            return @("pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-asset-import-regression-corpus.ps1 -CorpusRoot $CommandCorpusRoot -OutputRoot $outputRoot -WriteCookedOutputs -CompareExpectedHashes -CollectPreviewRows -RequireReady")
        }
        "retain_success_and_failure_reports" {
            return @("retain one successful and one failed real-corpus report under $CommandCorpusRoot/retained/**/report.gereport before operator-loop closeout")
        }
        "run_operator_loop" {
            return @("pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-operator-loop.ps1 -CorpusRoot $CommandCorpusRoot -RequireReady -OutputRoot out/asset-import-regression/operator-loop")
        }
        "validate_require_ready" {
            return @(
                "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1 -CorpusRoot $CommandCorpusRoot -RequireReady",
                "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-asset-import-regression-corpus.ps1 -CorpusRoot $CommandCorpusRoot -RequireReady"
            )
        }
        "promote_task17" {
            return @("update Task 17 docs/manifest only after retained corpus and operator-loop require-ready validation pass")
        }
        default {
            return @("create or mount an approved host-owned corpus at $CommandCorpusRoot")
        }
    }
}

$resolvedCorpusRoot = ConvertTo-LocalPath -Path $CorpusRoot
$commandCorpusRoot = ConvertTo-CommandPath -Path $resolvedCorpusRoot
$corpusRootPresent = Test-Path -LiteralPath $resolvedCorpusRoot -PathType Container
$manifestPath = Join-Path $resolvedCorpusRoot "corpus.gecorpus"
$expectedHashesPath = Join-Path $resolvedCorpusRoot "expected/hashes.gehashes"
$noticesPath = Join-Path $resolvedCorpusRoot "notices/THIRD_PARTY_ASSET_NOTICES.md"
$sourcesGltfPath = Join-Path $resolvedCorpusRoot "sources/gltf"
$sourcesTexturesPath = Join-Path $resolvedCorpusRoot "sources/textures"
$sourcesMaterialsPath = Join-Path $resolvedCorpusRoot "sources/materials"
$sourcesAudioPath = Join-Path $resolvedCorpusRoot "sources/audio"
$officialSourceLedgerPath = Join-Path $resolvedCorpusRoot "retained/official-source-ledger.md"
$selectionSummaryPath = Join-Path $resolvedCorpusRoot "retained/corpus-selection-summary.md"
$reportPath = Join-Path $resolvedCorpusRoot "report.gereport"

$manifestPresent = $corpusRootPresent -and (Test-Path -LiteralPath $manifestPath -PathType Leaf)
$expectedHashesPresent = $corpusRootPresent -and (Test-Path -LiteralPath $expectedHashesPath -PathType Leaf)
$noticesPresent = $corpusRootPresent -and (Test-Path -LiteralPath $noticesPath -PathType Leaf)
$sourcesGltfPresent = $corpusRootPresent -and (Test-Path -LiteralPath $sourcesGltfPath -PathType Container)
$sourcesTexturesPresent = $corpusRootPresent -and (Test-Path -LiteralPath $sourcesTexturesPath -PathType Container)
$sourcesMaterialsPresent = $corpusRootPresent -and (Test-Path -LiteralPath $sourcesMaterialsPath -PathType Container)
$sourcesAudioPresent = $corpusRootPresent -and (Test-Path -LiteralPath $sourcesAudioPath -PathType Container)
$officialSourceLedgerPresent = $corpusRootPresent -and (Test-Path -LiteralPath $officialSourceLedgerPath -PathType Leaf)
$selectionSummaryPresent = $corpusRootPresent -and (Test-Path -LiteralPath $selectionSummaryPath -PathType Leaf)
$reportPresent = $corpusRootPresent -and (Test-Path -LiteralPath $reportPath -PathType Leaf)
$runnerPresent = Test-RunnerPresent

$reportSummary = if ($reportPresent) {
    Read-ReportSummary -Path $reportPath
} else {
    [pscustomobject]@{
        Ready                 = $false
        FailedCount           = 0
        LegalBlockedCount     = 0
        NondeterministicCount = 0
        IsSuccessReport       = $false
        IsFailureReport       = $false
    }
}
$retainedSummary = if ($corpusRootPresent) {
    Get-RetainedReportSummary -ResolvedCorpusRoot $resolvedCorpusRoot
} else {
    [pscustomobject]@{
        ReportCount    = 0
        SuccessReports = 0
        FailureReports = 0
    }
}

$sourceLayoutReady = $sourcesGltfPresent -and $sourcesTexturesPresent -and $sourcesMaterialsPresent -and $sourcesAudioPresent
$retainedOperatorInputReady = $retainedSummary.SuccessReports -gt 0 -and $retainedSummary.FailureReports -gt 0
$reportHasBlockers = $reportPresent -and (
    (-not $reportSummary.Ready) -or
    $reportSummary.FailedCount -gt 0 -or
    $reportSummary.LegalBlockedCount -gt 0 -or
    $reportSummary.NondeterministicCount -gt 0
)

$status = "ready_for_promotion"
$nextAction = "promote_task17"
$ready = $true
if (-not $corpusRootPresent) {
    $status = "missing_corpus"
    $nextAction = "create_or_mount_approved_host_corpus"
    $ready = $false
} elseif (-not $manifestPresent) {
    $status = "corpus_manifest_required"
    $nextAction = "generate_manifest"
    $ready = $false
} elseif (-not $noticesPresent) {
    $status = "notices_required"
    $nextAction = "complete_notices"
    $ready = $false
} elseif (-not $sourceLayoutReady) {
    $status = "source_layout_required"
    $nextAction = "populate_canonical_sources"
    $ready = $false
} elseif (-not $expectedHashesPresent) {
    $status = "expected_hashes_required"
    $nextAction = "generate_expected_hashes"
    $ready = $false
} elseif ((-not $officialSourceLedgerPresent) -or (-not $selectionSummaryPresent)) {
    $status = "selection_review_required"
    $nextAction = "complete_official_source_ledger_and_selection_summary"
    $ready = $false
} elseif (-not $reportPresent) {
    if ($runnerPresent) {
        $status = "runner_required"
        $nextAction = "run_corpus"
    } else {
        $status = "dependency_host_required"
        $nextAction = "bootstrap_and_build_asset_importers"
    }
    $ready = $false
} elseif (-not $retainedOperatorInputReady) {
    $status = "retained_operator_evidence_required"
    $nextAction = "retain_success_and_failure_reports"
    $ready = $false
} elseif ($reportHasBlockers) {
    $status = "operator_loop_required"
    $nextAction = "run_operator_loop"
    $ready = $false
} else {
    $status = "validate_require_ready"
    $nextAction = "validate_require_ready"
    $ready = $false
}

if ($RequireReady -and $status -eq "validate_require_ready") {
    $validationScript = Join-Path $PSScriptRoot "validate-asset-import-regression-corpus.ps1"
    $validationLines = @(& $pwshCommand -NoProfile -ExecutionPolicy Bypass -File $validationScript -CorpusRoot $commandCorpusRoot -RequireReady 2>&1)
    if ($LASTEXITCODE -eq 0) {
        $status = "ready_for_promotion"
        $nextAction = "promote_task17"
        $ready = $true
    } else {
        $status = "require_ready_validation_failed"
        $nextAction = "fix_require_ready_diagnostics"
        $ready = $false
        Write-Output "asset_import_regression_handoff_diagnostic=require_ready.validation_failed"
        for ($index = 0; $index -lt $validationLines.Count; $index++) {
            $safeLine = ([string]$validationLines[$index]).Replace("`r", "").Replace("`n", " ")
            Write-Output "asset_import_regression_handoff_validation_line.$index=$safeLine"
        }
    }
}

$nextCommands = @(Get-NextCommands -NextAction $nextAction -CommandCorpusRoot $commandCorpusRoot)

Write-Output "asset_import_regression_handoff_status=$status"
Write-Output "asset_import_regression_handoff_ready=$([int]$ready)"
Write-Output "asset_import_regression_handoff_next_action=$nextAction"
Write-Output "asset_import_regression_handoff_corpus_root=$commandCorpusRoot"
Write-Output "asset_import_regression_handoff_corpus_root_present=$([int]$corpusRootPresent)"
Write-Output "asset_import_regression_handoff_corpus_manifest_present=$([int]$manifestPresent)"
Write-Output "asset_import_regression_handoff_expected_hashes_present=$([int]$expectedHashesPresent)"
Write-Output "asset_import_regression_handoff_notices_present=$([int]$noticesPresent)"
Write-Output "asset_import_regression_handoff_sources_gltf_present=$([int]$sourcesGltfPresent)"
Write-Output "asset_import_regression_handoff_sources_textures_present=$([int]$sourcesTexturesPresent)"
Write-Output "asset_import_regression_handoff_sources_materials_present=$([int]$sourcesMaterialsPresent)"
Write-Output "asset_import_regression_handoff_sources_audio_present=$([int]$sourcesAudioPresent)"
Write-Output "asset_import_regression_handoff_source_layout_ready=$([int]$sourceLayoutReady)"
Write-Output "asset_import_regression_handoff_official_source_ledger_present=$([int]$officialSourceLedgerPresent)"
Write-Output "asset_import_regression_handoff_selection_summary_present=$([int]$selectionSummaryPresent)"
Write-Output "asset_import_regression_handoff_report_present=$([int]$reportPresent)"
Write-Output "asset_import_regression_handoff_report_ready=$([int]$reportSummary.Ready)"
Write-Output "asset_import_regression_handoff_failed_count=$($reportSummary.FailedCount)"
Write-Output "asset_import_regression_handoff_legal_blocked_count=$($reportSummary.LegalBlockedCount)"
Write-Output "asset_import_regression_handoff_nondeterministic_count=$($reportSummary.NondeterministicCount)"
Write-Output "asset_import_regression_handoff_retained_reports=$($retainedSummary.ReportCount)"
Write-Output "asset_import_regression_handoff_retained_success_reports=$($retainedSummary.SuccessReports)"
Write-Output "asset_import_regression_handoff_retained_failure_reports=$($retainedSummary.FailureReports)"
Write-Output "asset_import_regression_handoff_operator_loop_input_ready=$([int]$retainedOperatorInputReady)"
Write-Output "asset_import_regression_handoff_asset_importers_runner_present=$([int]$runnerPresent)"
Write-Output "asset_import_regression_handoff_external_engine_claim=0"
Write-Output "asset_import_regression_handoff_legal_approval_claim=0"
Write-Output "asset_import_regression_handoff_unity_unreal_godot_compatibility_claim=0"
Write-Output "asset_import_regression_handoff_next_command.count=$($nextCommands.Count)"
for ($index = 0; $index -lt $nextCommands.Count; $index++) {
    Write-Output "asset_import_regression_handoff_next_command.$index=$($nextCommands[$index])"
}

if ($RequireReady -and -not $ready) {
    $diagnostic = "require_ready.$status"
    if ($status -eq "missing_corpus") {
        $diagnostic = "require_ready.missing_corpus"
    }
    Write-Output "asset_import_regression_handoff_diagnostic=$diagnostic"
    Write-Error "asset import regression corpus handoff is not ready: $status"
}
