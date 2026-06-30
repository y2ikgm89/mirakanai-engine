#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(Mandatory = $true)]
    [string]$CorpusRoot,

    [Parameter(Mandatory = $true)]
    [string]$SourcesRoot,

    [Parameter(Mandatory = $true)]
    [string]$NoticesPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputManifest,

    [switch]$FailOnMissingNotice
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$repoRoot = Get-RepoRoot
Set-Location $repoRoot

function ConvertTo-FullPath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label,
        [switch]$RequireLeaf,
        [switch]$RequireContainer
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
        $Path
    } else {
        Join-Path $repoRoot $Path
    }
    if (-not (Test-Path -LiteralPath $candidate)) {
        Write-Error "$Label does not exist: $Path"
    }
    $resolved = (Resolve-Path -LiteralPath $candidate).Path
    if ($RequireLeaf -and -not (Test-Path -LiteralPath $resolved -PathType Leaf)) {
        Write-Error "$Label must be a file: $Path"
    }
    if ($RequireContainer -and -not (Test-Path -LiteralPath $resolved -PathType Container)) {
        Write-Error "$Label must be a directory: $Path"
    }
    return $resolved
}

function ConvertTo-OutputPath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
        $Path
    } else {
        Join-Path $repoRoot $Path
    }
    $fullPath = [System.IO.Path]::GetFullPath($candidate)
    $parent = Split-Path -Parent $fullPath
    if ([string]::IsNullOrWhiteSpace($parent)) {
        Write-Error "$Label must have a parent directory: $Path"
    }
    return $fullPath
}

function Test-PathInsideRoot {
    param(
        [Parameter(Mandatory = $true)][string]$RootPath,
        [Parameter(Mandatory = $true)][string]$CandidatePath
    )

    $comparison = [System.StringComparison]::Ordinal
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        $comparison = [System.StringComparison]::OrdinalIgnoreCase
    }
    $rootFull = [System.IO.Path]::GetFullPath($RootPath).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    )
    $candidateFull = [System.IO.Path]::GetFullPath($CandidatePath)
    if ($candidateFull.Equals($rootFull, $comparison)) {
        return $true
    }
    return $candidateFull.StartsWith(
        $rootFull + [System.IO.Path]::DirectorySeparatorChar,
        $comparison
    )
}

function Assert-ExactPath {
    param(
        [Parameter(Mandatory = $true)][string]$ActualPath,
        [Parameter(Mandatory = $true)][string]$ExpectedPath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $comparison = [System.StringComparison]::Ordinal
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        $comparison = [System.StringComparison]::OrdinalIgnoreCase
    }
    $actual = [System.IO.Path]::GetFullPath($ActualPath).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    )
    $expected = [System.IO.Path]::GetFullPath($ExpectedPath).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    )
    if (-not $actual.Equals($expected, $comparison)) {
        Write-Error "$Label must use the canonical asset import regression corpus layout: $expected"
    }
}

function Test-SafeRelativePath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        return $false
    }
    if ($RelativePath.Contains("\") -or $RelativePath.Contains(":") -or [System.IO.Path]::IsPathRooted($RelativePath)) {
        return $false
    }
    foreach ($segment in $RelativePath.Split("/")) {
        if ([string]::IsNullOrWhiteSpace($segment) -or $segment -eq "." -or $segment -eq "..") {
            return $false
        }
    }
    return $true
}

function ConvertTo-SafeRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $normalized = $RelativePath.Replace("\", "/")
    if (-not (Test-SafeRelativePath -RelativePath $normalized)) {
        Write-Error "unsafe_relative_path: $Label must be relative, slash-separated, and must not contain absolute, drive, backslash, empty, '.', or '..' segments."
    }
    return $normalized
}

function ConvertTo-BooleanText {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($Text -eq "true" -or $Text -eq "false") {
        return $Text
    }
    Write-Error "$Label must be true or false."
}

function Get-RequiredValue {
    param(
        [Parameter(Mandatory = $true)][System.Collections.IDictionary]$Values,
        [Parameter(Mandatory = $true)][string]$Key
    )

    if (-not $Values.Contains($Key) -or [string]::IsNullOrWhiteSpace([string]$Values[$Key])) {
        Write-Error "missing_notice_field: $Key"
    }
    return [string]$Values[$Key]
}

function Read-KeyValueDocument {
    param([Parameter(Mandatory = $true)][string]$Path)

    $text = ConvertTo-LfText (Get-Content -LiteralPath $Path -Encoding utf8 -Raw)
    $values = [ordered]@{}
    $lineNumber = 0
    foreach ($line in $text.Split("`n")) {
        $lineNumber += 1
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }
        if ($line.Contains("`r")) {
            Write-Error "notice.line.$lineNumber.carriage_return"
        }
        $separator = $line.IndexOf("=")
        if ($separator -le 0) {
            continue
        }
        $key = $line.Substring(0, $separator)
        $value = $line.Substring($separator + 1)
        if ($values.Contains($key)) {
            Write-Error "notice.$key.duplicate_key"
        }
        $values[$key] = $value
    }
    return $values
}

function Test-NoticeAnchorPresent {
    param(
        [Parameter(Mandatory = $true)][string]$NoticeText,
        [Parameter(Mandatory = $true)][string]$NoticeId
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

function ConvertTo-List {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $items = [System.Collections.Generic.List[string]]::new()
    foreach ($item in $Text.Split(",")) {
        $trimmed = $item.Trim()
        if ([string]::IsNullOrWhiteSpace($trimmed)) {
            Write-Error "$Label contains an empty list item."
        }
        $items.Add($trimmed) | Out-Null
    }
    return [string[]]$items
}

function Add-IndexedListLines {
    param(
        [Parameter(Mandatory = $true)][System.Collections.Generic.List[string]]$Lines,
        [Parameter(Mandatory = $true)][string]$Prefix,
        [Parameter(Mandatory = $true)][string[]]$Items
    )

    $Lines.Add("$Prefix.count=$($Items.Count)") | Out-Null
    for ($index = 0; $index -lt $Items.Count; ++$index) {
        $Lines.Add("$Prefix.$index=$($Items[$index])") | Out-Null
    }
}

function Get-Sha256Label {
    param([Parameter(Mandatory = $true)][string]$Path)

    return "sha256:$((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant())"
}

function Get-AssetIndexList {
    param([Parameter(Mandatory = $true)][System.Collections.IDictionary]$Values)

    $assetCountText = Get-RequiredValue -Values $Values -Key "asset.count"
    $assetCount = 0
    if (-not [int]::TryParse($assetCountText, [ref]$assetCount) -or $assetCount -lt 1) {
        Write-Error "asset.count must be a positive integer."
    }
    return 0..($assetCount - 1)
}

$resolvedCorpusRoot = ConvertTo-FullPath -Path $CorpusRoot -Label "CorpusRoot" -RequireContainer
$resolvedSourcesRoot = ConvertTo-FullPath -Path $SourcesRoot -Label "SourcesRoot" -RequireContainer
$resolvedNoticesPath = ConvertTo-FullPath -Path $NoticesPath -Label "NoticesPath" -RequireLeaf
$resolvedOutputManifest = ConvertTo-OutputPath -Path $OutputManifest -Label "OutputManifest"

if (-not (Test-PathInsideRoot -RootPath $resolvedCorpusRoot -CandidatePath $resolvedSourcesRoot)) {
    Write-Error "SourcesRoot must be inside CorpusRoot for host-owned corpus layout."
}
if (-not (Test-PathInsideRoot -RootPath $resolvedCorpusRoot -CandidatePath $resolvedNoticesPath)) {
    Write-Error "NoticesPath must be inside CorpusRoot for host-owned corpus layout."
}
if (-not (Test-PathInsideRoot -RootPath $resolvedCorpusRoot -CandidatePath $resolvedOutputManifest)) {
    Write-Error "OutputManifest must be inside CorpusRoot for host-owned corpus layout."
}
Assert-ExactPath `
    -ActualPath $resolvedSourcesRoot `
    -ExpectedPath (Join-Path $resolvedCorpusRoot "sources") `
    -Label "SourcesRoot"
Assert-ExactPath `
    -ActualPath $resolvedNoticesPath `
    -ExpectedPath (Join-Path $resolvedCorpusRoot "notices/THIRD_PARTY_ASSET_NOTICES.md") `
    -Label "NoticesPath"
Assert-ExactPath `
    -ActualPath $resolvedOutputManifest `
    -ExpectedPath (Join-Path $resolvedCorpusRoot "corpus.gecorpus") `
    -Label "OutputManifest"

$noticeText = Get-Content -LiteralPath $resolvedNoticesPath -Encoding utf8 -Raw
$noticeValues = Read-KeyValueDocument -Path $resolvedNoticesPath
$assetRows = [System.Collections.Generic.List[object]]::new()
$assetIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)

foreach ($sourceIndex in Get-AssetIndexList -Values $noticeValues) {
    $prefix = "asset.$sourceIndex"
    $assetId = Get-RequiredValue -Values $noticeValues -Key "$prefix.asset_id"
    if (-not $assetIds.Add($assetId)) {
        Write-Error "duplicate_asset_id: $assetId"
    }

    $sourceRelativeToSources = ConvertTo-SafeRelativePath `
        -RelativePath (Get-RequiredValue -Values $noticeValues -Key "$prefix.source_path") `
        -Label "$prefix.source_path"
    $sourcePath = [System.IO.Path]::GetFullPath((Join-Path $resolvedSourcesRoot ($sourceRelativeToSources -replace "/", [System.IO.Path]::DirectorySeparatorChar)))
    if (-not (Test-PathInsideRoot -RootPath $resolvedSourcesRoot -CandidatePath $sourcePath)) {
        Write-Error "$prefix.source_path escapes SourcesRoot."
    }
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        Write-Error "$prefix.source_path missing source file: $sourceRelativeToSources"
    }

    $expectedSha256 = Get-Sha256Label -Path $sourcePath
    if ($noticeValues.Contains("$prefix.expected_sha256") -and
        [string]$noticeValues["$prefix.expected_sha256"] -ne $expectedSha256) {
        Write-Error "$prefix.expected_sha256_mismatch"
    }

    $noticeId = Get-RequiredValue -Values $noticeValues -Key "$prefix.provenance.notice_id"
    if ($FailOnMissingNotice -and -not (Test-NoticeAnchorPresent -NoticeText $noticeText -NoticeId $noticeId)) {
        Write-Error "$prefix.missing_notice_anchor: $noticeId"
    }

    $relativeSourcePath = [System.IO.Path]::GetRelativePath($resolvedCorpusRoot, $sourcePath).Replace("\", "/")
    $null = ConvertTo-SafeRelativePath -RelativePath $relativeSourcePath -Label "$prefix.generated_source_path"

    $row = [ordered]@{
        asset_id = $assetId
        kind = Get-RequiredValue -Values $noticeValues -Key "$prefix.kind"
        asset_key = ConvertTo-SafeRelativePath -RelativePath (Get-RequiredValue -Values $noticeValues -Key "$prefix.asset_key") -Label "$prefix.asset_key"
        source_path = $relativeSourcePath
        expected_sha256 = $expectedSha256
        expected_output_kinds = ConvertTo-List -Text (Get-RequiredValue -Values $noticeValues -Key "$prefix.expected_output_kinds") -Label "$prefix.expected_output_kinds"
        required_features = ConvertTo-List -Text (Get-RequiredValue -Values $noticeValues -Key "$prefix.required_features") -Label "$prefix.required_features"
        preset_metadata = ConvertTo-List -Text (Get-RequiredValue -Values $noticeValues -Key "$prefix.preset_metadata") -Label "$prefix.preset_metadata"
        mesh_unit_scale = Get-RequiredValue -Values $noticeValues -Key "$prefix.mesh.unit_scale"
        mesh_up_axis = Get-RequiredValue -Values $noticeValues -Key "$prefix.mesh.up_axis"
        mesh_triangulate = ConvertTo-BooleanText -Text (Get-RequiredValue -Values $noticeValues -Key "$prefix.mesh.triangulate") -Label "$prefix.mesh.triangulate"
        mesh_generate_normals = ConvertTo-BooleanText -Text (Get-RequiredValue -Values $noticeValues -Key "$prefix.mesh.generate_normals") -Label "$prefix.mesh.generate_normals"
        mesh_generate_tangents = ConvertTo-BooleanText -Text (Get-RequiredValue -Values $noticeValues -Key "$prefix.mesh.generate_tangents") -Label "$prefix.mesh.generate_tangents"
        mesh_material_extraction = Get-RequiredValue -Values $noticeValues -Key "$prefix.mesh.material_extraction"
        license_policy = Get-RequiredValue -Values $noticeValues -Key "$prefix.license_policy"
        allow_external_resources = ConvertTo-BooleanText -Text (Get-RequiredValue -Values $noticeValues -Key "$prefix.allow_external_resources") -Label "$prefix.allow_external_resources"
        allow_checked_in_distribution = ConvertTo-BooleanText -Text (Get-RequiredValue -Values $noticeValues -Key "$prefix.allow_checked_in_distribution") -Label "$prefix.allow_checked_in_distribution"
        provenance_origin = Get-RequiredValue -Values $noticeValues -Key "$prefix.provenance.origin"
        provenance_source_url = Get-RequiredValue -Values $noticeValues -Key "$prefix.provenance.source_url"
        provenance_retrieved_date = Get-RequiredValue -Values $noticeValues -Key "$prefix.provenance.retrieved_date"
        provenance_version_or_commit = Get-RequiredValue -Values $noticeValues -Key "$prefix.provenance.version_or_commit"
        provenance_copyright_holder = Get-RequiredValue -Values $noticeValues -Key "$prefix.provenance.copyright_holder"
        provenance_license_id = Get-RequiredValue -Values $noticeValues -Key "$prefix.provenance.license_id"
        provenance_modification_status = Get-RequiredValue -Values $noticeValues -Key "$prefix.provenance.modification_status"
        provenance_distribution_target = Get-RequiredValue -Values $noticeValues -Key "$prefix.provenance.distribution_target"
        provenance_notice_id = $noticeId
        provenance_notice_complete = ConvertTo-BooleanText -Text (Get-RequiredValue -Values $noticeValues -Key "$prefix.provenance.notice_complete") -Label "$prefix.provenance.notice_complete"
        provenance_external_engine_material = ConvertTo-BooleanText -Text (Get-RequiredValue -Values $noticeValues -Key "$prefix.provenance.external_engine_material") -Label "$prefix.provenance.external_engine_material"
    }
    $assetRows.Add([pscustomobject]$row) | Out-Null
}

$sortedRows = @($assetRows | Sort-Object -Property asset_id)
$manifestLines = [System.Collections.Generic.List[string]]::new()
$manifestLines.Add("format=GameEngine.AssetImportRegressionCorpus.v1") | Out-Null
$manifestLines.Add("corpus.id=GameEngine.AssetImportRegressionCorpus.v1") | Out-Null
$manifestLines.Add("corpus.version=1") | Out-Null
$manifestLines.Add("root_path=out/host-artifacts/asset-import-regression-corpus") | Out-Null
$manifestLines.Add("row_budget=100000") | Out-Null
$manifestLines.Add("asset.count=$($sortedRows.Count)") | Out-Null

$hashLines = [System.Collections.Generic.List[string]]::new()
for ($index = 0; $index -lt $sortedRows.Count; ++$index) {
    $row = $sortedRows[$index]
    $prefix = "asset.$index"
    $manifestLines.Add("$prefix.asset_id=$($row.asset_id)") | Out-Null
    $manifestLines.Add("$prefix.kind=$($row.kind)") | Out-Null
    $manifestLines.Add("$prefix.asset_key=$($row.asset_key)") | Out-Null
    $manifestLines.Add("$prefix.source_path=$($row.source_path)") | Out-Null
    $manifestLines.Add("$prefix.expected_sha256=$($row.expected_sha256)") | Out-Null
    Add-IndexedListLines -Lines $manifestLines -Prefix "$prefix.expected_output_kind" -Items $row.expected_output_kinds
    Add-IndexedListLines -Lines $manifestLines -Prefix "$prefix.required_feature" -Items $row.required_features
    Add-IndexedListLines -Lines $manifestLines -Prefix "$prefix.preset_metadata" -Items $row.preset_metadata
    $manifestLines.Add("$prefix.mesh.unit_scale=$($row.mesh_unit_scale)") | Out-Null
    $manifestLines.Add("$prefix.mesh.up_axis=$($row.mesh_up_axis)") | Out-Null
    $manifestLines.Add("$prefix.mesh.triangulate=$($row.mesh_triangulate)") | Out-Null
    $manifestLines.Add("$prefix.mesh.generate_normals=$($row.mesh_generate_normals)") | Out-Null
    $manifestLines.Add("$prefix.mesh.generate_tangents=$($row.mesh_generate_tangents)") | Out-Null
    $manifestLines.Add("$prefix.mesh.material_extraction=$($row.mesh_material_extraction)") | Out-Null
    $manifestLines.Add("$prefix.license_policy=$($row.license_policy)") | Out-Null
    $manifestLines.Add("$prefix.allow_external_resources=$($row.allow_external_resources)") | Out-Null
    $manifestLines.Add("$prefix.allow_checked_in_distribution=$($row.allow_checked_in_distribution)") | Out-Null
    $manifestLines.Add("$prefix.provenance.asset_key=$($row.asset_key)") | Out-Null
    $manifestLines.Add("$prefix.provenance.origin=$($row.provenance_origin)") | Out-Null
    $manifestLines.Add("$prefix.provenance.source_url=$($row.provenance_source_url)") | Out-Null
    $manifestLines.Add("$prefix.provenance.retrieved_date=$($row.provenance_retrieved_date)") | Out-Null
    $manifestLines.Add("$prefix.provenance.version_or_commit=$($row.provenance_version_or_commit)") | Out-Null
    $manifestLines.Add("$prefix.provenance.copyright_holder=$($row.provenance_copyright_holder)") | Out-Null
    $manifestLines.Add("$prefix.provenance.license_id=$($row.provenance_license_id)") | Out-Null
    $manifestLines.Add("$prefix.provenance.modification_status=$($row.provenance_modification_status)") | Out-Null
    $manifestLines.Add("$prefix.provenance.distribution_target=$($row.provenance_distribution_target)") | Out-Null
    $manifestLines.Add("$prefix.provenance.notice_id=$($row.provenance_notice_id)") | Out-Null
    $manifestLines.Add("$prefix.provenance.notice_complete=$($row.provenance_notice_complete)") | Out-Null
    $manifestLines.Add("$prefix.provenance.external_engine_material=$($row.provenance_external_engine_material)") | Out-Null
    $hashLines.Add("$($row.asset_id)=$($row.expected_sha256)") | Out-Null
}

$outputParent = Split-Path -Parent $resolvedOutputManifest
if (-not (Test-Path -LiteralPath $outputParent -PathType Container)) {
    $null = New-Item -ItemType Directory -Path $outputParent -Force
}
Set-Content -LiteralPath $resolvedOutputManifest -Value $manifestLines -Encoding utf8NoBOM

$expectedRoot = Join-Path $resolvedCorpusRoot "expected"
if (-not (Test-Path -LiteralPath $expectedRoot -PathType Container)) {
    $null = New-Item -ItemType Directory -Path $expectedRoot -Force
}
$expectedHashesPath = Join-Path $expectedRoot "hashes.gehashes"
Set-Content -LiteralPath $expectedHashesPath -Value $hashLines -Encoding utf8NoBOM

$relativeManifest = [System.IO.Path]::GetRelativePath($repoRoot, $resolvedOutputManifest).Replace("\", "/")
$relativeHashes = [System.IO.Path]::GetRelativePath($repoRoot, $expectedHashesPath).Replace("\", "/")
Write-Output "asset_import_regression_manifest_generator_asset_count=$($sortedRows.Count)"
Write-Output "asset_import_regression_manifest_generator_manifest=$relativeManifest"
Write-Output "asset_import_regression_manifest_generator_expected_hashes=$relativeHashes"
Write-Output "asset_import_regression_manifest_generator_downloaded_assets=0"
Write-Output "asset_import_regression_manifest_generator_license_inference=0"
Write-Output "asset_import_regression_manifest_generator_ready=1"
Write-Information "asset-import-regression-corpus-manifest-generator: ok" -InformationAction Continue
