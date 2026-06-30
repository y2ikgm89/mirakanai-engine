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

function Read-KeyValueMetadataDocument {
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
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }
        if ($line.Contains("`r")) {
            Add-Diagnostic "$Name.line.$lineNumber.carriage_return"
            continue
        }
        $separator = $line.IndexOf("=")
        if ($separator -lt 1) {
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

function Get-RequiredIntegerMetadata {
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
        return 0
    }

    $value = 0
    if (-not [int]::TryParse([string]$Values[$Key], [ref]$value) -or $value -lt 0) {
        Add-Diagnostic "$Name.$Key.invalid"
        return 0
    }
    return $value
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

function Test-NoticeAnchorPresent {
    param(
        [Parameter(Mandatory = $true)]
        [string]$NoticeText,

        [Parameter(Mandatory = $true)]
        [string]$NoticeId
    )

    foreach ($line in (ConvertTo-LfText $NoticeText).Split("`n")) {
        $trimmed = $line.Trim()
        if ([string]::IsNullOrWhiteSpace($trimmed)) {
            continue
        }
        if ($trimmed.StartsWith("asset.", [System.StringComparison]::Ordinal) -or
            $trimmed.StartsWith("asset.count=", [System.StringComparison]::Ordinal)) {
            continue
        }
        if ($trimmed -eq $NoticeId) {
            return $true
        }
    }
    return $false
}

function Test-RequiredSummaryFlag {
    param(
        [Parameter(Mandatory = $true)]
        [System.Collections.IDictionary]$Values,

        [Parameter(Mandatory = $true)]
        [string]$Key
    )

    if (-not $Values.Contains($Key) -or [string]$Values[$Key] -ne "true") {
        if ($RequireReady) {
            Add-Diagnostic "selection_summary.$Key.missing_or_false"
        }
        return $false
    }
    return $true
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
$canonicalManifestPresent = Test-Path -LiteralPath $manifestPath -PathType Leaf
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
$sourcePathRows = [System.Collections.Generic.List[object]]::new()
$previousAssetId = ""
$assetIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
$kindCounts = [System.Collections.Generic.Dictionary[string, int]]::new([System.StringComparer]::Ordinal)
$requiredFeatures = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
$noticeRows = [System.Collections.Generic.List[object]]::new()

for ($index = 0; $index -lt $assetCount; ++$index) {
    $prefix = "asset.$index"
    $assetId = Get-RequiredValue -Values $corpus -Key "$prefix.asset_id" -Name $prefix
    $kind = Get-RequiredValue -Values $corpus -Key "$prefix.kind" -Name $prefix
    $sourcePath = Get-RequiredValue -Values $corpus -Key "$prefix.source_path" -Name $prefix
    $expectedSha256 = Get-RequiredValue -Values $corpus -Key "$prefix.expected_sha256" -Name $prefix
    $licensePolicy = Get-RequiredValue -Values $corpus -Key "$prefix.license_policy" -Name $prefix
    $origin = Get-RequiredValue -Values $corpus -Key "$prefix.provenance.origin" -Name $prefix
    $licenseId = Get-RequiredValue -Values $corpus -Key "$prefix.provenance.license_id" -Name $prefix
    $noticeId = Get-RequiredValue -Values $corpus -Key "$prefix.provenance.notice_id" -Name $prefix
    $noticeComplete = Get-RequiredValue -Values $corpus -Key "$prefix.provenance.notice_complete" -Name $prefix
    $externalEngineMaterial = Get-RequiredValue -Values $corpus -Key "$prefix.provenance.external_engine_material" -Name $prefix
    $requiredFeatureCountText = Get-RequiredValue -Values $corpus -Key "$prefix.required_feature.count" -Name $prefix
    $requiredFeatureCount = 0
    if (-not [int]::TryParse($requiredFeatureCountText, [ref]$requiredFeatureCount) -or $requiredFeatureCount -lt 1) {
        Add-Diagnostic "$prefix.required_feature.count.invalid"
        $failedCount += 1
        $requiredFeatureCount = 0
    }

    if ([string]::IsNullOrWhiteSpace($assetId) -or -not $assetIds.Add($assetId)) {
        Add-Diagnostic "$prefix.asset_id.duplicate_or_missing"
        $failedCount += 1
    }
    if (-not [string]::IsNullOrWhiteSpace($previousAssetId) -and $previousAssetId.CompareTo($assetId) -gt 0) {
        Add-Diagnostic "$prefix.asset_id.not_sorted"
        $failedCount += 1
    }
    $previousAssetId = $assetId
    if (-not [string]::IsNullOrWhiteSpace($kind)) {
        if ($kindCounts.ContainsKey($kind)) {
            $kindCounts[$kind] += 1
        } else {
            $kindCounts[$kind] = 1
        }
    }
    for ($featureIndex = 0; $featureIndex -lt $requiredFeatureCount; ++$featureIndex) {
        $feature = Get-RequiredValue -Values $corpus -Key "$prefix.required_feature.$featureIndex" -Name $prefix
        if (-not [string]::IsNullOrWhiteSpace($feature)) {
            $requiredFeatures.Add($feature) | Out-Null
        }
    }
    if (-not [string]::IsNullOrWhiteSpace($assetId) -and -not [string]::IsNullOrWhiteSpace($noticeId)) {
        $noticeRows.Add([pscustomobject]@{
                AssetId = $assetId
                NoticeId = $noticeId
            }) | Out-Null
    }

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
    $sourcePathRows.Add([pscustomobject]@{
            AssetId = $assetId
            Kind = $kind
            SourcePath = $sourcePath
        }) | Out-Null

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

$expectedHashesPath = Join-Path $resolvedCorpusRoot "expected/hashes.gehashes"
$noticesPath = Join-Path $resolvedCorpusRoot "notices/THIRD_PARTY_ASSET_NOTICES.md"
$sourcesGltfPath = Join-Path $resolvedCorpusRoot "sources/gltf"
$sourcesTexturesPath = Join-Path $resolvedCorpusRoot "sources/textures"
$sourcesMaterialsPath = Join-Path $resolvedCorpusRoot "sources/materials"
$sourcesAudioPath = Join-Path $resolvedCorpusRoot "sources/audio"
$officialSourceLedgerPath = Join-Path $resolvedCorpusRoot "retained/official-source-ledger.md"
$selectionSummaryPath = Join-Path $resolvedCorpusRoot "retained/corpus-selection-summary.md"
$expectedHashesPresent = Test-Path -LiteralPath $expectedHashesPath -PathType Leaf
$noticesPresent = Test-Path -LiteralPath $noticesPath -PathType Leaf
$sourcesGltfPresent = Test-Path -LiteralPath $sourcesGltfPath -PathType Container
$sourcesTexturesPresent = Test-Path -LiteralPath $sourcesTexturesPath -PathType Container
$sourcesMaterialsPresent = Test-Path -LiteralPath $sourcesMaterialsPath -PathType Container
$sourcesAudioPresent = Test-Path -LiteralPath $sourcesAudioPath -PathType Container
$officialSourceLedgerPresent = Test-Path -LiteralPath $officialSourceLedgerPath -PathType Leaf
$selectionSummaryPresent = Test-Path -LiteralPath $selectionSummaryPath -PathType Leaf
$expectedHashesReady = $expectedHashesPresent
$noticesReady = $noticesPresent
$sourceLayoutReady = $sourcesGltfPresent -and $sourcesTexturesPresent -and $sourcesMaterialsPresent -and $sourcesAudioPresent
$sourcePathLayoutReady = $true
$officialSourceLedgerReady = $officialSourceLedgerPresent
$selectionSummaryReady = $selectionSummaryPresent
$minimumCompositionReady = $false
$requiredFeatureCategoriesReady = $false

if ($expectedHashesPresent) {
    if (Test-ReparsePointPath -RootPath $resolvedCorpusRoot -CandidatePath $expectedHashesPath) {
        Add-Diagnostic "expected_hashes.reparse_point"
        $expectedHashesReady = $false
    } else {
        $expectedHashes = Read-KeyValueDocument -Path $expectedHashesPath -Name "expected_hashes"
        foreach ($sourceHashRow in $sourceHashRows) {
            $separator = $sourceHashRow.IndexOf("=")
            if ($separator -le 0) {
                continue
            }
            $assetId = $sourceHashRow.Substring(0, $separator)
            $actualHash = $sourceHashRow.Substring($separator + 1)
            if (-not $expectedHashes.Contains($assetId)) {
                Add-Diagnostic "expected_hashes.$assetId.missing"
                $expectedHashesReady = $false
            } elseif ([string]$expectedHashes[$assetId] -ne $actualHash) {
                Add-Diagnostic "expected_hashes.$assetId.mismatch"
                $expectedHashesReady = $false
            }
        }
    }
}
if ($noticesPresent) {
    if (Test-ReparsePointPath -RootPath $resolvedCorpusRoot -CandidatePath $noticesPath) {
        Add-Diagnostic "notices.reparse_point"
        $noticesReady = $false
    } else {
        $noticeText = Get-Content -LiteralPath $noticesPath -Encoding utf8 -Raw
        foreach ($noticeRow in $noticeRows) {
            if (-not (Test-NoticeAnchorPresent -NoticeText $noticeText -NoticeId $noticeRow.NoticeId)) {
                Add-Diagnostic "notices.$($noticeRow.AssetId).missing_anchor"
                $noticesReady = $false
            }
        }
    }
}
foreach ($sourceLayoutEntry in @(
        @{ Label = "sources_gltf"; Path = $sourcesGltfPath; Present = $sourcesGltfPresent },
        @{ Label = "sources_textures"; Path = $sourcesTexturesPath; Present = $sourcesTexturesPresent },
        @{ Label = "sources_materials"; Path = $sourcesMaterialsPath; Present = $sourcesMaterialsPresent },
        @{ Label = "sources_audio"; Path = $sourcesAudioPath; Present = $sourcesAudioPresent }
    )) {
    if ([bool]$sourceLayoutEntry.Present -and
        (Test-ReparsePointPath -RootPath $resolvedCorpusRoot -CandidatePath ([string]$sourceLayoutEntry.Path))) {
        Add-Diagnostic "$($sourceLayoutEntry.Label).reparse_point"
        $sourceLayoutReady = $false
    }
}
if ($RequireReady) {
    foreach ($sourcePathRow in $sourcePathRows) {
        $expectedPrefix = ""
        switch ([string]$sourcePathRow.Kind) {
        "gltf_scene" { $expectedPrefix = "sources/gltf/" }
        "gltf_mesh" { $expectedPrefix = "sources/gltf/" }
        "gltf_animation" { $expectedPrefix = "sources/gltf/" }
        "png_texture" { $expectedPrefix = "sources/textures/" }
        "openexr_texture" { $expectedPrefix = "sources/textures/" }
        "ktx2_basis_texture" { $expectedPrefix = "sources/textures/" }
        "material_document" { $expectedPrefix = "sources/materials/" }
        "audio_source" { $expectedPrefix = "sources/audio/" }
        default {
            Add-Diagnostic "source_layout.$($sourcePathRow.AssetId).unsupported_kind"
            $sourcePathLayoutReady = $false
        }
        }
        if (-not [string]::IsNullOrWhiteSpace($expectedPrefix) -and
            -not ([string]$sourcePathRow.SourcePath).StartsWith($expectedPrefix, [System.StringComparison]::Ordinal)) {
            Add-Diagnostic "source_layout.$($sourcePathRow.AssetId).noncanonical_source_path"
            $sourcePathLayoutReady = $false
        }
    }
}
if ($officialSourceLedgerPresent -and
    (Test-ReparsePointPath -RootPath $resolvedCorpusRoot -CandidatePath $officialSourceLedgerPath)) {
    Add-Diagnostic "official_source_ledger.reparse_point"
    $officialSourceLedgerReady = $false
}
if ($selectionSummaryPresent) {
    if (Test-ReparsePointPath -RootPath $resolvedCorpusRoot -CandidatePath $selectionSummaryPath) {
        Add-Diagnostic "selection_summary.reparse_point"
        $selectionSummaryReady = $false
    } else {
        $selectionSummary = Read-KeyValueMetadataDocument -Path $selectionSummaryPath -Name "selection_summary"
        $summaryGltfRows = Get-RequiredIntegerMetadata -Values $selectionSummary -Key "gltf_rows" -Name "selection_summary"
        $summaryTextureRows = Get-RequiredIntegerMetadata -Values $selectionSummary -Key "texture_rows" -Name "selection_summary"
        $summaryMaterialRows = Get-RequiredIntegerMetadata -Values $selectionSummary -Key "material_rows" -Name "selection_summary"
        $summaryAnimationRows = Get-RequiredIntegerMetadata -Values $selectionSummary -Key "animation_rows" -Name "selection_summary"
        $summaryAudioRows = Get-RequiredIntegerMetadata -Values $selectionSummary -Key "audio_rows" -Name "selection_summary"

        $manifestGltfRows = 0
        foreach ($kind in @("gltf_scene", "gltf_mesh", "gltf_animation")) {
            if ($kindCounts.ContainsKey($kind)) {
                $manifestGltfRows += $kindCounts[$kind]
            }
        }
        $manifestTextureRows = 0
        foreach ($kind in @("png_texture", "openexr_texture", "ktx2_basis_texture")) {
            if ($kindCounts.ContainsKey($kind)) {
                $manifestTextureRows += $kindCounts[$kind]
            }
        }
        $manifestMaterialRows = if ($kindCounts.ContainsKey("material_document")) { $kindCounts["material_document"] } else { 0 }
        $manifestAnimationRows = if ($kindCounts.ContainsKey("gltf_animation")) { $kindCounts["gltf_animation"] } else { 0 }
        $manifestAudioRows = if ($kindCounts.ContainsKey("audio_source")) { $kindCounts["audio_source"] } else { 0 }

        $requiredSummaryFlags = @(
            "gltf.mesh_only",
            "gltf.scene",
            "gltf.skin",
            "gltf.morph",
            "gltf.material",
            "gltf.external_buffer",
            "gltf.external_image",
            "gltf.animation_trs",
            "gltf.quaternion_animation",
            "gltf.invalid_external_path",
            "gltf.invalid_extension",
            "gltf.invalid_accessor",
            "gltf.unsupported_interpolation",
            "texture.png",
            "texture.openexr",
            "texture.ktx2_basis",
            "texture.color_management",
            "texture.alpha",
            "texture.hdr_metadata",
            "texture.mip_array_cubemap",
            "texture.needs_transcoding",
            "texture.backend_target_policy",
            "material.factor_only",
            "material.texture_backed",
            "material.missing_dependency",
            "material.duplicated_texture_reference",
            "material.unsupported_graph_export",
            "material.package_output",
            "animation.node_trs",
            "animation.skin",
            "animation.morph_weight",
            "animation.valid_quaternion",
            "animation.invalid_quaternion",
            "animation.unit_up_axis_conversion",
            "animation.unsupported_scalar_z_up_rotation",
            "audio.wav",
            "audio.mp3",
            "audio.flac",
            "audio.static_pcm",
            "audio.streaming_source_review",
            "audio.invalid_truncated_decode",
            "audio.channel_sample_rate_variance",
            "audio.loop_normalization_preset"
        )
        $summaryFlagsReady = $true
        foreach ($flag in $requiredSummaryFlags) {
            if (-not (Test-RequiredSummaryFlag -Values $selectionSummary -Key $flag)) {
                $summaryFlagsReady = $false
            }
        }

        $requiredManifestFeatureTokens = @(
            "gltf_mesh_only",
            "gltf_scene",
            "gltf_skin",
            "gltf_morph",
            "gltf_material",
            "gltf_external_buffer",
            "gltf_external_image",
            "gltf_animation_trs",
            "gltf_quaternion_animation",
            "diagnostic_invalid_external_path",
            "diagnostic_invalid_extension",
            "diagnostic_invalid_accessor",
            "diagnostic_unsupported_interpolation",
            "texture_png",
            "texture_openexr",
            "texture_ktx2_basis",
            "texture_color_management",
            "texture_alpha",
            "texture_hdr_metadata",
            "texture_mip_array_cubemap",
            "texture_needs_transcoding",
            "texture_backend_target_policy",
            "material_factor_only",
            "material_texture_backed",
            "material_missing_dependency",
            "material_duplicated_texture_reference",
            "material_unsupported_graph_export",
            "material_package_output",
            "animation_node_trs",
            "animation_skin",
            "animation_morph_weight",
            "animation_valid_quaternion",
            "animation_invalid_quaternion",
            "animation_unit_up_axis_conversion",
            "animation_unsupported_scalar_z_up_rotation",
            "audio_wav",
            "audio_mp3",
            "audio_flac",
            "audio_static_pcm",
            "audio_streaming_source_review",
            "audio_invalid_truncated_decode",
            "audio_channel_sample_rate_variance",
            "audio_loop_normalization_preset"
        )
        $manifestFeatureTokensReady = $true
        foreach ($featureToken in $requiredManifestFeatureTokens) {
            if (-not $requiredFeatures.Contains($featureToken)) {
                if ($RequireReady) {
                    Add-Diagnostic "required_feature.$featureToken.missing"
                }
                $manifestFeatureTokensReady = $false
            }
        }
        $requiredFeatureCategoriesReady = $summaryFlagsReady -and $manifestFeatureTokensReady

        $minimumCompositionReady =
            $summaryGltfRows -ge 40 -and $manifestGltfRows -ge 40 -and
            $summaryTextureRows -ge 30 -and $manifestTextureRows -ge 30 -and
            $summaryMaterialRows -ge 20 -and $manifestMaterialRows -ge 20 -and
            $summaryAnimationRows -ge 20 -and $manifestAnimationRows -ge 20 -and
            $summaryAudioRows -ge 20 -and $manifestAudioRows -ge 20
    }
}

$combinedFailedCount = $failedCount + $reportFailedCount
$combinedLegalBlockedCount = $legalBlockedCount + $reportLegalBlockedCount
$combinedNondeterministicCount = $reportNondeterministicCount
$ready = $largeCorpusPresent -and $canonicalManifestPresent -and $reportPresent -and $reportReady -and
    $expectedHashesReady -and $noticesReady -and $sourceLayoutReady -and
    $sourcePathLayoutReady -and
    $officialSourceLedgerReady -and $selectionSummaryReady -and
    $minimumCompositionReady -and $requiredFeatureCategoriesReady -and
    $combinedFailedCount -eq 0 -and $combinedLegalBlockedCount -eq 0 -and $combinedNondeterministicCount -eq 0

if ($RequireReady) {
    if (-not $largeCorpusPresent) {
        Add-Diagnostic "require_ready.large_corpus_missing"
    }
    if (-not $canonicalManifestPresent) {
        Add-Diagnostic "require_ready.corpus_manifest_missing"
    }
    if (-not $reportPresent) {
        Add-Diagnostic "require_ready.report_missing"
    }
    if (-not $reportReady) {
        Add-Diagnostic "require_ready.report_not_ready"
    }
    if (-not $expectedHashesReady) {
        Add-Diagnostic "require_ready.expected_hashes_missing"
    }
    if (-not $noticesReady) {
        Add-Diagnostic "require_ready.notices_missing"
    }
    if (-not $sourcesGltfPresent) {
        Add-Diagnostic "require_ready.sources_gltf_missing"
    }
    if (-not $sourcesTexturesPresent) {
        Add-Diagnostic "require_ready.sources_textures_missing"
    }
    if (-not $sourcesMaterialsPresent) {
        Add-Diagnostic "require_ready.sources_materials_missing"
    }
    if (-not $sourcesAudioPresent) {
        Add-Diagnostic "require_ready.sources_audio_missing"
    }
    if (-not $sourcePathLayoutReady) {
        Add-Diagnostic "require_ready.source_paths_noncanonical"
    }
    if (-not $officialSourceLedgerReady) {
        Add-Diagnostic "require_ready.official_source_ledger_missing"
    }
    if (-not $selectionSummaryReady) {
        Add-Diagnostic "require_ready.selection_summary_missing"
    }
    if (-not $noticesReady) {
        Add-Diagnostic "require_ready.notice_rows_incomplete"
    }
    if (-not $minimumCompositionReady) {
        Add-Diagnostic "require_ready.minimum_composition_not_met"
    }
    if (-not $requiredFeatureCategoriesReady) {
        Add-Diagnostic "require_ready.required_feature_categories_missing"
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
Write-Output "asset_import_regression_corpus_manifest_present=$([int]$canonicalManifestPresent)"
Write-Output "asset_import_regression_large_corpus_present=$([int]$largeCorpusPresent)"
Write-Output "asset_import_regression_legal_blocked_count=$combinedLegalBlockedCount"
Write-Output "asset_import_regression_failed_count=$combinedFailedCount"
Write-Output "asset_import_regression_nondeterministic_count=$combinedNondeterministicCount"
Write-Output "asset_import_regression_expected_hashes_present=$([int]$expectedHashesPresent)"
Write-Output "asset_import_regression_notices_present=$([int]$noticesPresent)"
Write-Output "asset_import_regression_sources_gltf_present=$([int]$sourcesGltfPresent)"
Write-Output "asset_import_regression_sources_textures_present=$([int]$sourcesTexturesPresent)"
Write-Output "asset_import_regression_sources_materials_present=$([int]$sourcesMaterialsPresent)"
Write-Output "asset_import_regression_sources_audio_present=$([int]$sourcesAudioPresent)"
Write-Output "asset_import_regression_source_paths_canonical=$([int]$sourcePathLayoutReady)"
Write-Output "asset_import_regression_official_source_ledger_present=$([int]$officialSourceLedgerPresent)"
Write-Output "asset_import_regression_selection_summary_present=$([int]$selectionSummaryPresent)"
Write-Output "asset_import_regression_minimum_composition_ready=$([int]$minimumCompositionReady)"
Write-Output "asset_import_regression_required_feature_categories_ready=$([int]$requiredFeatureCategoriesReady)"
Write-Output "asset_import_regression_corpus_ready=$([int]$ready)"
Write-Output "asset_import_regression_replay_hash=$replayHash"

if ($diagnostics.Count -gt 0) {
    foreach ($diagnostic in $diagnostics) {
        Write-Output "asset_import_regression_diagnostic=$diagnostic"
    }
    Write-Error "asset import regression corpus validation failed with $($diagnostics.Count) diagnostic(s)."
}
