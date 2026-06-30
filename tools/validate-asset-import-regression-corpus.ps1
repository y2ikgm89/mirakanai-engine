#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string]$CorpusRoot = "",
    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$repoRoot = Get-RepoRoot
$fixtureRoot = Join-Path $repoRoot "tests/fixtures/asset_import_regression"
$corpusRootWasSupplied = -not [string]::IsNullOrWhiteSpace($CorpusRoot)
$diagnostics = [System.Collections.Generic.List[string]]::new()

function ConvertTo-FullPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
        $Path
    } else {
        Join-Path $repoRoot $Path
    }
    if (-not (Test-Path -LiteralPath $candidate)) {
        Write-Error "Path does not exist: $Path"
    }
    return (Resolve-Path -LiteralPath $candidate).Path
}

function Test-SafeCorpusRelativePath {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyString()]
        [string]$RelativePath
    )

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        return $false
    }
    if ($RelativePath.StartsWith("/") -or $RelativePath.Contains("\") -or $RelativePath.Contains(":")) {
        return $false
    }
    foreach ($segment in $RelativePath.Split("/")) {
        if ($segment -eq "" -or $segment -eq "." -or $segment -eq "..") {
            return $false
        }
    }
    return $true
}

function Add-Diagnostic {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Code
    )

    $diagnostics.Add($Code) | Out-Null
}

function Read-KeyValueDocument {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $text = ConvertTo-LfText (Get-Content -LiteralPath $Path -Encoding utf8 -Raw)
    $values = [ordered]@{}
    $lineNumber = 0
    foreach ($line in $text.Split("`n")) {
        $lineNumber += 1
        if ([string]::IsNullOrEmpty($line)) {
            continue
        }
        if ($line.Contains("`r")) {
            Add-Diagnostic "$Name.line.$lineNumber.carriage_return"
            continue
        }
        $separator = $line.IndexOf("=")
        if ($separator -lt 1) {
            Add-Diagnostic "$Name.line.$lineNumber.missing_separator"
            continue
        }
        $key = $line.Substring(0, $separator)
        $value = $line.Substring($separator + 1)
        if ($values.Contains($key)) {
            Add-Diagnostic "$Name.$key.duplicate_key"
        } else {
            $values[$key] = $value
        }
    }
    return $values
}

function Get-RequiredValue {
    param(
        [Parameter(Mandatory = $true)]
        [System.Collections.IDictionary]$Values,

        [Parameter(Mandatory = $true)]
        [string]$Key,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    if (-not $Values.Contains($Key)) {
        Add-Diagnostic "$Name.$Key.missing"
        return ""
    }
    return [string]$Values[$Key]
}

function Test-PathInsideRoot {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RootPath,

        [Parameter(Mandatory = $true)]
        [string]$CandidatePath
    )

    $comparison = [System.StringComparison]::Ordinal
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        $comparison = [System.StringComparison]::OrdinalIgnoreCase
    }

    $rootFull = [System.IO.Path]::GetFullPath($RootPath)
    $candidateFull = [System.IO.Path]::GetFullPath($CandidatePath)
    if ($candidateFull.Equals($rootFull, $comparison)) {
        return $true
    }
    $rootWithSeparator = $rootFull.TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    return $candidateFull.StartsWith($rootWithSeparator, $comparison)
}

function Resolve-CorpusChildPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RootPath,

        [Parameter(Mandatory = $true)]
        [string]$RelativePath,

        [Parameter(Mandatory = $true)]
        [string]$Context
    )

    if (-not (Test-SafeCorpusRelativePath -RelativePath $RelativePath)) {
        Add-Diagnostic "$Context.unsafe_source_path"
        return ""
    }

    $nativeRelativePath = $RelativePath.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
    $candidate = [System.IO.Path]::GetFullPath((Join-Path $RootPath $nativeRelativePath))
    if (-not (Test-PathInsideRoot -RootPath $RootPath -CandidatePath $candidate)) {
        Add-Diagnostic "$Context.path_escapes_corpus_root"
        return ""
    }
    return $candidate
}

function Test-ReparsePointPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RootPath,

        [Parameter(Mandatory = $true)]
        [string]$CandidatePath
    )

    $rootFull = [System.IO.Path]::GetFullPath($RootPath).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    )
    $candidateFull = [System.IO.Path]::GetFullPath($CandidatePath)
    if (-not (Test-PathInsideRoot -RootPath $rootFull -CandidatePath $candidateFull)) {
        return $true
    }

    $relative = [System.IO.Path]::GetRelativePath($rootFull, $candidateFull)
    $probe = $rootFull
    foreach ($segment in $relative.Split([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)) {
        if ([string]::IsNullOrWhiteSpace($segment)) {
            continue
        }
        $probe = Join-Path $probe $segment
        if (Test-Path -LiteralPath $probe) {
            $item = Get-Item -LiteralPath $probe -Force
            if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
                return $true
            }
        }
    }
    return $false
}

function Get-LowerSha256 {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    return "sha256:$((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant())"
}

function ConvertTo-Sha256Text {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
    $hash = [System.Security.Cryptography.SHA256]::HashData($bytes)
    return "sha256:$([System.Convert]::ToHexString($hash).ToLowerInvariant())"
}

function Test-LegalLicenseAccepted {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Origin,

        [Parameter(Mandatory = $true)]
        [string]$LicenseId
    )

    if ($Origin -eq "first_party") {
        return $LicenseId -eq "LicenseRef-Proprietary"
    }

    $allowed = @("CC0-1.0", "MIT", "BSD-2-Clause", "BSD-3-Clause", "Apache-2.0", "Zlib", "CC-BY-4.0")
    if ($allowed -contains $LicenseId) {
        return $true
    }

    return $false
}

$resolvedCorpusRoot = if ($corpusRootWasSupplied) {
    ConvertTo-FullPath -Path $CorpusRoot
} else {
    ConvertTo-FullPath -Path $fixtureRoot
}

if (-not (Test-Path -LiteralPath $resolvedCorpusRoot -PathType Container)) {
    Write-Error "Corpus root must be a directory: $resolvedCorpusRoot"
}
if (Test-ReparsePointPath -RootPath $resolvedCorpusRoot -CandidatePath $resolvedCorpusRoot) {
    Write-Error "Corpus root must not contain reparse points: $resolvedCorpusRoot"
}

$manifestPath = Join-Path $resolvedCorpusRoot "corpus.gecorpus"
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    $fixtureManifestPath = Join-Path $resolvedCorpusRoot "first_party_corpus.gecorpus"
    if (Test-Path -LiteralPath $fixtureManifestPath -PathType Leaf) {
        $manifestPath = $fixtureManifestPath
    } else {
        Write-Error "Missing corpus manifest: $manifestPath"
    }
}
if (Test-ReparsePointPath -RootPath $resolvedCorpusRoot -CandidatePath $manifestPath) {
    Write-Error "Corpus manifest path must not contain reparse points: $manifestPath"
}

$corpus = Read-KeyValueDocument -Path $manifestPath -Name "corpus"
if ((Get-RequiredValue -Values $corpus -Key "format" -Name "corpus") -ne "GameEngine.AssetImportRegressionCorpus.v1") {
    Add-Diagnostic "corpus.format.unsupported"
}
if ((Get-RequiredValue -Values $corpus -Key "corpus.id" -Name "corpus") -ne "GameEngine.AssetImportRegressionCorpus.v1") {
    Add-Diagnostic "corpus.id.unsupported"
}

$assetCountText = Get-RequiredValue -Values $corpus -Key "asset.count" -Name "corpus"
$assetCount = 0
if (-not [int]::TryParse($assetCountText, [ref]$assetCount) -or $assetCount -lt 0) {
    Add-Diagnostic "corpus.asset.count.invalid"
    $assetCount = 0
}

$legalBlockedCount = 0
$failedCount = 0
$sourceHashRows = [System.Collections.Generic.List[string]]::new()
$previousAssetId = ""
$assetIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)

for ($index = 0; $index -lt $assetCount; ++$index) {
    $prefix = "asset.$index"
    $assetId = Get-RequiredValue -Values $corpus -Key "$prefix.asset_id" -Name $prefix
    $sourcePath = Get-RequiredValue -Values $corpus -Key "$prefix.source_path" -Name $prefix
    $expectedSha256 = Get-RequiredValue -Values $corpus -Key "$prefix.expected_sha256" -Name $prefix
    $licensePolicy = Get-RequiredValue -Values $corpus -Key "$prefix.license_policy" -Name $prefix
    $origin = Get-RequiredValue -Values $corpus -Key "$prefix.provenance.origin" -Name $prefix
    $licenseId = Get-RequiredValue -Values $corpus -Key "$prefix.provenance.license_id" -Name $prefix
    $noticeId = Get-RequiredValue -Values $corpus -Key "$prefix.provenance.notice_id" -Name $prefix
    $noticeComplete = Get-RequiredValue -Values $corpus -Key "$prefix.provenance.notice_complete" -Name $prefix
    $externalEngineMaterial = Get-RequiredValue -Values $corpus -Key "$prefix.provenance.external_engine_material" -Name $prefix

    if ([string]::IsNullOrWhiteSpace($assetId) -or -not $assetIds.Add($assetId)) {
        Add-Diagnostic "$prefix.asset_id.duplicate_or_missing"
        $failedCount += 1
    }
    if (-not [string]::IsNullOrWhiteSpace($previousAssetId) -and $previousAssetId.CompareTo($assetId) -gt 0) {
        Add-Diagnostic "$prefix.asset_id.not_sorted"
        $failedCount += 1
    }
    $previousAssetId = $assetId

    if ($licensePolicy -eq "rejected" -or $externalEngineMaterial -ne "false" -or $noticeComplete -ne "true" -or
        -not (Test-LegalLicenseAccepted -Origin $origin -LicenseId $licenseId) -or
        [string]::IsNullOrWhiteSpace($noticeId)) {
        Add-Diagnostic "$prefix.legal.blocked"
        $legalBlockedCount += 1
    }

    $resolvedSource = Resolve-CorpusChildPath -RootPath $resolvedCorpusRoot -RelativePath $sourcePath -Context $prefix
    if ([string]::IsNullOrWhiteSpace($resolvedSource)) {
        $failedCount += 1
        continue
    }
    if (-not (Test-Path -LiteralPath $resolvedSource -PathType Leaf)) {
        Add-Diagnostic "$prefix.source.missing"
        $failedCount += 1
        continue
    }
    if (Test-ReparsePointPath -RootPath $resolvedCorpusRoot -CandidatePath $resolvedSource) {
        Add-Diagnostic "$prefix.source.reparse_point"
        $failedCount += 1
        continue
    }

    $actualSha256 = Get-LowerSha256 -Path $resolvedSource
    if ($expectedSha256 -notmatch '^sha256:[0-9a-f]{64}$') {
        Add-Diagnostic "$prefix.expected_sha256.invalid"
        $failedCount += 1
    } elseif ($actualSha256 -ne $expectedSha256) {
        Add-Diagnostic "$prefix.source_hash.mismatch"
        $failedCount += 1
    }
    $sourceHashRows.Add("$assetId=$actualSha256") | Out-Null
}

$reportPath = Join-Path $resolvedCorpusRoot "report.gereport"
$reportPresent = Test-Path -LiteralPath $reportPath -PathType Leaf
$reportReady = $false
$reportFailedCount = 0
$reportLegalBlockedCount = 0
$reportNondeterministicCount = 0
if ($reportPresent) {
    if (Test-ReparsePointPath -RootPath $resolvedCorpusRoot -CandidatePath $reportPath) {
        Add-Diagnostic "report.reparse_point"
        $failedCount += 1
    } else {
        $report = Read-KeyValueDocument -Path $reportPath -Name "report"
        if ((Get-RequiredValue -Values $report -Key "format" -Name "report") -ne "GameEngine.AssetImportRegressionReport.v1") {
            Add-Diagnostic "report.format.unsupported"
        }
        $reportReady = (Get-RequiredValue -Values $report -Key "ready" -Name "report") -eq "true"
        [void][int]::TryParse((Get-RequiredValue -Values $report -Key "failed_count" -Name "report"), [ref]$reportFailedCount)
        [void][int]::TryParse((Get-RequiredValue -Values $report -Key "legal_blocked_count" -Name "report"), [ref]$reportLegalBlockedCount)
        [void][int]::TryParse((Get-RequiredValue -Values $report -Key "nondeterministic_count" -Name "report"), [ref]$reportNondeterministicCount)
    }
}

$largeCorpusPresent = $false
if ($corpusRootWasSupplied) {
    $fixtureRootFull = [System.IO.Path]::GetFullPath($fixtureRoot)
    $largeCorpusPresent = -not (Test-PathInsideRoot -RootPath $fixtureRootFull -CandidatePath $resolvedCorpusRoot)
}

$combinedFailedCount = $failedCount + $reportFailedCount
$combinedLegalBlockedCount = $legalBlockedCount + $reportLegalBlockedCount
$combinedNondeterministicCount = $reportNondeterministicCount
$ready = $largeCorpusPresent -and $reportPresent -and $reportReady -and
    $combinedFailedCount -eq 0 -and $combinedLegalBlockedCount -eq 0 -and $combinedNondeterministicCount -eq 0

if ($RequireReady) {
    if (-not $largeCorpusPresent) {
        Add-Diagnostic "require_ready.large_corpus_missing"
    }
    if (-not $reportPresent) {
        Add-Diagnostic "require_ready.report_missing"
    }
    if (-not $reportReady) {
        Add-Diagnostic "require_ready.report_not_ready"
    }
    if ($combinedFailedCount -ne 0 -or $combinedLegalBlockedCount -ne 0 -or $combinedNondeterministicCount -ne 0) {
        Add-Diagnostic "require_ready.report_or_manifest_has_blockers"
    }
}

$manifestHash = Get-LowerSha256 -Path $manifestPath
$reportHash = if ($reportPresent) { Get-LowerSha256 -Path $reportPath } else { "sha256:none" }
$replayHashInput = @(
    "manifest=$manifestHash",
    "report=$reportHash",
    "asset_count=$assetCount",
    ($sourceHashRows | Sort-Object)
) -join "`n"
$replayHash = ConvertTo-Sha256Text -Text $replayHashInput

Write-Output "asset_import_regression_corpus_manifest=$([System.IO.Path]::GetRelativePath($repoRoot, $manifestPath).Replace('\', '/'))"
Write-Output "asset_import_regression_asset_count=$assetCount"
Write-Output "asset_import_regression_large_corpus_present=$([int]$largeCorpusPresent)"
Write-Output "asset_import_regression_legal_blocked_count=$combinedLegalBlockedCount"
Write-Output "asset_import_regression_failed_count=$combinedFailedCount"
Write-Output "asset_import_regression_nondeterministic_count=$combinedNondeterministicCount"
Write-Output "asset_import_regression_corpus_ready=$([int]$ready)"
Write-Output "asset_import_regression_replay_hash=$replayHash"

if ($diagnostics.Count -gt 0) {
    foreach ($diagnostic in $diagnostics) {
        Write-Output "asset_import_regression_diagnostic=$diagnostic"
    }
    Write-Error "asset import regression corpus validation failed with $($diagnostics.Count) diagnostic(s)."
}
