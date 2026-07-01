#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$ReportPath = "",
    [string]$CorpusRoot = "",
    [string]$OutputRoot = "",
    [switch]$SyntheticSmoke,
    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$repoRoot = Get-RepoRoot
$pwshCommand = (Get-Command pwsh -ErrorAction Stop).Source

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    return Join-Path $repoRoot ($Path -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

function Test-ReparsePointPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $item = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    return (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0)
}

function Test-PathUnderRoot {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $rootFull = [System.IO.Path]::GetFullPath($Root)
    $pathFull = [System.IO.Path]::GetFullPath($Path)
    $rootWithSeparator = $rootFull.TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar,
        [System.IO.Path]::AltDirectorySeparatorChar
    ) + [System.IO.Path]::DirectorySeparatorChar

    return $pathFull.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)
}

function Write-RequireReadyDiagnostic {
    param([Parameter(Mandatory = $true)][string]$Code)

    Write-Output "asset_import_regression_operator_loop_diagnostic=$Code"
    Write-Error "asset import regression operator loop require-ready diagnostic: $Code"
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

function Get-RequiredValue {
    param(
        [Parameter(Mandatory = $true)]
        [System.Collections.Generic.Dictionary[string, string]]$Map,
        [Parameter(Mandatory = $true)][string]$Key
    )

    if (-not $Map.ContainsKey($Key)) {
        Write-Error "asset import regression operator loop missing required key: $Key"
    }

    return $Map[$Key]
}

function ConvertTo-CheckedInt {
    param(
        [Parameter(Mandatory = $true)][string]$Value,
        [Parameter(Mandatory = $true)][string]$Context
    )

    $parsed = 0
    if (-not [int]::TryParse($Value, [ref]$parsed) -or $parsed -lt 0) {
        Write-Error "$Context must be a non-negative integer."
    }

    return $parsed
}

function ConvertTo-CheckedBool {
    param(
        [Parameter(Mandatory = $true)][string]$Value,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if ($Value -eq "true") {
        return $true
    }
    if ($Value -eq "false") {
        return $false
    }

    Write-Error "$Context must be true or false."
}

function Test-CleanText {
    param([Parameter(Mandatory = $true)][string]$Value)

    return -not ($Value.Contains("`n") -or $Value.Contains("`r") -or $Value.Contains([string][char]0))
}

function Test-ParentSegment {
    param([Parameter(Mandatory = $true)][string]$Value)

    foreach ($segment in $Value.Split("/")) {
        if ($segment -eq "..") {
            return $true
        }
    }

    return $false
}

function ConvertTo-SafeRelativePath {
    param([Parameter(Mandatory = $true)][string]$Value)

    if (-not [string]::IsNullOrWhiteSpace($Value) -and
        (Test-CleanText -Value $Value) -and
        -not $Value.StartsWith("/") -and
        -not $Value.Contains("\") -and
        -not $Value.Contains(":") -and
        -not $Value.StartsWith("http://") -and
        -not $Value.StartsWith("https://") -and
        -not (Test-ParentSegment -Value $Value)) {
        return $Value
    }

    return "unsafe_source_path_redacted"
}

function ConvertTo-SafeToken {
    param([Parameter(Mandatory = $true)][string]$Value)

    $builder = [System.Text.StringBuilder]::new()
    foreach ($character in $Value.ToCharArray()) {
        if (($character -ge [char]"a" -and $character -le [char]"z") -or
            ($character -ge [char]"A" -and $character -le [char]"Z") -or
            ($character -ge [char]"0" -and $character -le [char]"9") -or
            $character -eq [char]"_" -or
            $character -eq [char]"-" -or
            $character -eq [char]".") {
            $null = $builder.Append($character)
        } else {
            $null = $builder.Append("_")
        }
    }

    if ($builder.Length -eq 0) {
        return "unknown"
    }

    return $builder.ToString()
}

function Get-Sha256Label {
    param([Parameter(Mandatory = $true)][string]$Text)

    $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    try {
        $hashBytes = $sha256.ComputeHash($bytes)
    } finally {
        $sha256.Dispose()
    }
    $builder = [System.Text.StringBuilder]::new("sha256:")
    foreach ($hashByte in $hashBytes) {
        $null = $builder.Append($hashByte.ToString("x2"))
    }

    return $builder.ToString()
}

function Get-RecommendedAction {
    param([Parameter(Mandatory = $true)][string]$Code)

    switch ($Code) {
        "none" { return "none" }
        { $PSItem -in @("invalid_manifest", "duplicate_asset_id", "unsafe_source_path", "row_budget_exceeded") } {
            return "split_corpus_or_raise_reviewed_budget"
        }
        { $PSItem -in @("missing_source_file", "source_hash_mismatch") } {
            return "refresh_corpus_manifest"
        }
        { $PSItem -in @("missing_license_provenance", "rejected_license", "external_engine_material") } {
            return "fix_notice_or_remove_asset"
        }
        { $PSItem -in @(
                "unsupported_format",
                "unsupported_extension",
                "unsupported_animation_channel",
                "unsupported_skin_or_morph_combination"
            ) } {
            return "record_unsupported_or_reduce_source"
        }
        { $PSItem -in @(
                "parser_error",
                "validator_error",
                "missing_external_resource",
                "unsafe_external_resource_path",
                "material_extraction_failed"
            ) } {
            return "inspect_source_asset"
        }
        "coordinate_normalization_failed" { return "open_axis_unit_preview" }
        { $PSItem -in @("texture_decode_failed", "texture_transcode_failed") } {
            return "inspect_codec_dependency"
        }
        { $PSItem -in @("cooked_output_mismatch", "nondeterministic_output") } {
            return "rerun_isolated_and_compare_hashes"
        }
        default {
            Write-Error "asset import regression operator loop unsupported diagnostic code: $Code"
        }
    }
}

function Get-ReimportDecision {
    param([Parameter(Mandatory = $true)][string]$RecommendedAction)

    switch ($RecommendedAction) {
        "none" { return "not_needed" }
        { $PSItem -in @("open_axis_unit_preview", "inspect_source_asset", "inspect_codec_dependency") } {
            return "dry_run_allowed"
        }
        default { return "blocked" }
    }
}

function Get-TriageSeverity {
    param([Parameter(Mandatory = $true)][string]$ReimportDecision)

    switch ($ReimportDecision) {
        "not_needed" { return "info" }
        "dry_run_allowed" { return "action_required" }
        default { return "blocked" }
    }
}

function Test-LegalBlockedCode {
    param([Parameter(Mandatory = $true)][string]$Code)

    return $Code -in @("missing_license_provenance", "rejected_license", "external_engine_material")
}

function Test-PresetDiffCode {
    param([Parameter(Mandatory = $true)][string]$Code)

    return $Code -in @("material_extraction_failed", "texture_decode_failed", "texture_transcode_failed")
}

function Assert-AssetImportReportCounterConsistency {
    param(
        [Parameter(Mandatory = $true)]$Report,
        [Parameter(Mandatory = $true)][string]$Context
    )

    $succeededCount = 0
    $failedCount = 0
    $legalBlockedCount = 0
    $nondeterministicCount = 0
    foreach ($row in $Report.Rows) {
        if ($row.Succeeded) {
            $succeededCount++
        } else {
            $failedCount++
        }
        if (Test-LegalBlockedCode -Code $row.Code) {
            $legalBlockedCount++
        }
        if ($row.Code -eq "nondeterministic_output") {
            $nondeterministicCount++
        }
    }

    if ($Report.AssetCount -ne $Report.Rows.Count) {
        Write-Error "$Context asset_count must match row.count."
    }
    if ($Report.SucceededCount -ne $succeededCount) {
        Write-Error "$Context succeeded_count must match succeeded rows."
    }
    if ($Report.FailedCount -ne $failedCount) {
        Write-Error "$Context failed_count must match failed rows."
    }
    if ($Report.LegalBlockedCount -ne $legalBlockedCount) {
        Write-Error "$Context legal_blocked_count must match legal blocked rows."
    }
    if ($Report.NondeterministicCount -ne $nondeterministicCount) {
        Write-Error "$Context nondeterministic_count must match nondeterministic rows."
    }
    if ($Report.Ready -and ($failedCount -ne 0 -or $legalBlockedCount -ne 0 -or $nondeterministicCount -ne 0)) {
        Write-Error "$Context ready=true is invalid when failed, legal-blocked, or nondeterministic rows exist."
    }
}

function Test-OperatorLoopSuccessReport {
    param([Parameter(Mandatory = $true)]$Report)

    return $Report.Ready -and
        $Report.FailedCount -eq 0 -and
        $Report.LegalBlockedCount -eq 0 -and
        $Report.NondeterministicCount -eq 0
}

function Test-OperatorLoopFailureReport {
    param([Parameter(Mandatory = $true)]$Report)

    return (-not $Report.Ready) -or
        $Report.FailedCount -gt 0 -or
        $Report.LegalBlockedCount -gt 0 -or
        $Report.NondeterministicCount -gt 0
}

function Read-AssetImportRegressionReport {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Write-Error "asset import regression operator loop report is missing: $Path"
    }

    $text = Get-Content -LiteralPath $Path -Encoding utf8 -Raw
    if ($text.Contains("`r")) {
        Write-Error "asset import regression operator loop report must be LF-only."
    }

    $map = [System.Collections.Generic.Dictionary[string, string]]::new([System.StringComparer]::Ordinal)
    $seenKeys = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($line in $text.Split("`n")) {
        if ([string]::IsNullOrEmpty($line)) {
            continue
        }

        $separator = $line.IndexOf("=")
        if ($separator -lt 0) {
            Write-Error "asset import regression operator loop report line is missing '='."
        }

        $key = $line.Substring(0, $separator)
        $value = $line.Substring($separator + 1)
        if (-not $seenKeys.Add($key)) {
            Write-Error "asset import regression operator loop report contains duplicate key: $key"
        }
        $map[$key] = $value
    }

    $format = Get-RequiredValue -Map $map -Key "format"
    if ($format -ne "GameEngine.AssetImportRegressionReport.v1") {
        Write-Error "asset import regression operator loop report format is unsupported: $format"
    }

    $rowCount = ConvertTo-CheckedInt -Value (Get-RequiredValue -Map $map -Key "row.count") -Context "row.count"
    $rows = [System.Collections.Generic.List[object]]::new()
    for ($index = 0; $index -lt $rowCount; $index++) {
        $prefix = "row.$index."
        $null = ConvertTo-CheckedBool -Value (Get-RequiredValue -Map $map -Key "${prefix}succeeded") -Context "${prefix}succeeded"
        $rows.Add([pscustomobject]@{
                AssetId                    = Get-RequiredValue -Map $map -Key "${prefix}asset_id"
                Kind                       = Get-RequiredValue -Map $map -Key "${prefix}kind"
                Asset                      = ConvertTo-CheckedInt -Value (Get-RequiredValue -Map $map -Key "${prefix}asset") -Context "${prefix}asset"
                SourcePath                 = Get-RequiredValue -Map $map -Key "${prefix}source_path"
                SourceSha256               = Get-RequiredValue -Map $map -Key "${prefix}source_sha256"
                PresetSha256               = Get-RequiredValue -Map $map -Key "${prefix}preset_sha256"
                ImporterId                 = Get-RequiredValue -Map $map -Key "${prefix}importer_id"
                ImporterVersion            = Get-RequiredValue -Map $map -Key "${prefix}importer_version"
                Phase                      = Get-RequiredValue -Map $map -Key "${prefix}phase"
                Code                       = Get-RequiredValue -Map $map -Key "${prefix}code"
                Message                    = Get-RequiredValue -Map $map -Key "${prefix}message"
                DeterministicOutputHash    = Get-RequiredValue -Map $map -Key "${prefix}deterministic_output_hash"
                Succeeded                  = ConvertTo-CheckedBool -Value (Get-RequiredValue -Map $map -Key "${prefix}succeeded") -Context "${prefix}succeeded"
                ReadyForCommercialEvidence = ConvertTo-CheckedBool -Value (Get-RequiredValue -Map $map -Key "${prefix}ready_for_commercial_evidence") -Context "${prefix}ready_for_commercial_evidence"
            })
    }

    return [pscustomobject]@{
        CorpusId              = Get-RequiredValue -Map $map -Key "corpus_id"
        RunId                 = Get-RequiredValue -Map $map -Key "run_id"
        AssetCount            = ConvertTo-CheckedInt -Value (Get-RequiredValue -Map $map -Key "asset_count") -Context "asset_count"
        SucceededCount        = ConvertTo-CheckedInt -Value (Get-RequiredValue -Map $map -Key "succeeded_count") -Context "succeeded_count"
        FailedCount           = ConvertTo-CheckedInt -Value (Get-RequiredValue -Map $map -Key "failed_count") -Context "failed_count"
        LegalBlockedCount     = ConvertTo-CheckedInt -Value (Get-RequiredValue -Map $map -Key "legal_blocked_count") -Context "legal_blocked_count"
        NondeterministicCount = ConvertTo-CheckedInt -Value (Get-RequiredValue -Map $map -Key "nondeterministic_count") -Context "nondeterministic_count"
        Ready                 = ConvertTo-CheckedBool -Value (Get-RequiredValue -Map $map -Key "ready") -Context "ready"
        Rows                  = $rows
    }
}

function New-OperatorTriage {
    param([Parameter(Mandatory = $true)]$Report)

    $rows = [System.Collections.Generic.List[object]]::new()
    $failedCount = 0
    $blockedCount = 0
    $reimportCandidateCount = 0
    $presetDiffRequiredCount = 0
    $axisUnitPreviewRequiredCount = 0
    $legalBlockedCount = 0
    $nondeterministicCount = 0
    $sanitizedRunId = ConvertTo-SafeToken -Value $Report.RunId
    $reproCommand = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-asset-import-regression-corpus.ps1 -CorpusRoot out/host-artifacts/asset-import-regression-corpus -OutputRoot out/asset-import-regression/staging/$sanitizedRunId"

    foreach ($reportRow in $Report.Rows) {
        $action = Get-RecommendedAction -Code $reportRow.Code
        $decision = Get-ReimportDecision -RecommendedAction $action
        $legalBlocked = Test-LegalBlockedCode -Code $reportRow.Code
        $presetDiffRequired = Test-PresetDiffCode -Code $reportRow.Code
        $axisUnitPreviewRequired = $reportRow.Code -eq "coordinate_normalization_failed"
        $nondeterministic = $reportRow.Code -eq "nondeterministic_output"
        $failed = $reportRow.Code -ne "none"

        if ($failed) {
            $failedCount++
        }
        if ($decision -eq "blocked") {
            $blockedCount++
        }
        if ($decision -eq "dry_run_allowed") {
            $reimportCandidateCount++
        }
        if ($presetDiffRequired) {
            $presetDiffRequiredCount++
        }
        if ($axisUnitPreviewRequired) {
            $axisUnitPreviewRequiredCount++
        }
        if ($legalBlocked) {
            $legalBlockedCount++
        }
        if ($nondeterministic) {
            $nondeterministicCount++
        }

        $assetId = ConvertTo-SafeToken -Value $reportRow.AssetId
        $rows.Add([pscustomobject]@{
                AssetId                 = $assetId
                Kind                    = $reportRow.Kind
                Asset                   = $reportRow.Asset
                SourcePath              = ConvertTo-SafeRelativePath -Value $reportRow.SourcePath
                SourceSha256            = $reportRow.SourceSha256
                PresetSha256            = $reportRow.PresetSha256
                ImporterId              = ConvertTo-SafeToken -Value $reportRow.ImporterId
                ImporterVersion         = ConvertTo-SafeToken -Value $reportRow.ImporterVersion
                Phase                   = ConvertTo-SafeToken -Value $reportRow.Phase
                Code                    = $reportRow.Code
                Severity                = Get-TriageSeverity -ReimportDecision $decision
                RecommendedAction       = $action
                ReimportDecision        = $decision
                ReproCommandId          = "asset_import_regression.repro.$assetId"
                ReproCommand            = $reproCommand
                SourceExcerptHash       = Get-Sha256Label -Text "$($reportRow.AssetId)|$($reportRow.Code)|$($reportRow.Phase)|$($reportRow.Message)"
                PresetDiffRequired      = $presetDiffRequired
                AxisUnitPreviewRequired = $axisUnitPreviewRequired
                LegalBlocked            = $legalBlocked
                Nondeterministic        = $nondeterministic
            })
    }

    return [pscustomobject]@{
        Format                       = "GameEngine.AssetImportRegressionTriage.v1"
        CorpusId                     = $Report.CorpusId
        RunId                        = $Report.RunId
        Rows                         = $rows
        RowCount                     = $rows.Count
        FailedCount                  = $failedCount
        BlockedCount                 = $blockedCount
        ReimportCandidateCount       = $reimportCandidateCount
        PresetDiffRequiredCount      = $presetDiffRequiredCount
        AxisUnitPreviewRequiredCount = $axisUnitPreviewRequiredCount
        LegalBlockedCount            = $legalBlockedCount
        NondeterministicCount        = $nondeterministicCount
        ReadyForOperatorReview       = $rows.Count -gt 0
    }
}

function ConvertTo-BoolText {
    param([Parameter(Mandatory = $true)][bool]$Value)

    if ($Value) {
        return "true"
    }

    return "false"
}

function ConvertTo-TriageText {
    param([Parameter(Mandatory = $true)]$Triage)

    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add("format=$($Triage.Format)")
    $lines.Add("corpus_id=$($Triage.CorpusId)")
    $lines.Add("run_id=$($Triage.RunId)")
    $lines.Add("row_count=$($Triage.RowCount)")
    $lines.Add("failed_count=$($Triage.FailedCount)")
    $lines.Add("blocked_count=$($Triage.BlockedCount)")
    $lines.Add("reimport_candidate_count=$($Triage.ReimportCandidateCount)")
    $lines.Add("preset_diff_required_count=$($Triage.PresetDiffRequiredCount)")
    $lines.Add("axis_unit_preview_required_count=$($Triage.AxisUnitPreviewRequiredCount)")
    $lines.Add("legal_blocked_count=$($Triage.LegalBlockedCount)")
    $lines.Add("nondeterministic_count=$($Triage.NondeterministicCount)")
    $lines.Add("ready_for_operator_review=$(ConvertTo-BoolText -Value $Triage.ReadyForOperatorReview)")
    $lines.Add("row.count=$($Triage.Rows.Count)")
    for ($index = 0; $index -lt $Triage.Rows.Count; $index++) {
        $row = $Triage.Rows[$index]
        $prefix = "row.$index"
        $lines.Add("$prefix.asset_id=$($row.AssetId)")
        $lines.Add("$prefix.kind=$($row.Kind)")
        $lines.Add("$prefix.asset=$($row.Asset)")
        $lines.Add("$prefix.source_path=$($row.SourcePath)")
        $lines.Add("$prefix.source_sha256=$($row.SourceSha256)")
        $lines.Add("$prefix.preset_sha256=$($row.PresetSha256)")
        $lines.Add("$prefix.importer_id=$($row.ImporterId)")
        $lines.Add("$prefix.importer_version=$($row.ImporterVersion)")
        $lines.Add("$prefix.phase=$($row.Phase)")
        $lines.Add("$prefix.code=$($row.Code)")
        $lines.Add("$prefix.severity=$($row.Severity)")
        $lines.Add("$prefix.recommended_action=$($row.RecommendedAction)")
        $lines.Add("$prefix.reimport_decision=$($row.ReimportDecision)")
        $lines.Add("$prefix.repro_command_id=$($row.ReproCommandId)")
        $lines.Add("$prefix.repro_command=$($row.ReproCommand)")
        $lines.Add("$prefix.source_excerpt_hash=$($row.SourceExcerptHash)")
        $lines.Add("$prefix.preset_diff_required=$(ConvertTo-BoolText -Value $row.PresetDiffRequired)")
        $lines.Add("$prefix.axis_unit_preview_required=$(ConvertTo-BoolText -Value $row.AxisUnitPreviewRequired)")
        $lines.Add("$prefix.legal_blocked=$(ConvertTo-BoolText -Value $row.LegalBlocked)")
        $lines.Add("$prefix.nondeterministic=$(ConvertTo-BoolText -Value $row.Nondeterministic)")
    }

    return (($lines -join "`n") + "`n")
}

function Assert-TriageTextSafe {
    param([Parameter(Mandatory = $true)][string]$TriageText)

    if ($TriageText.Contains("\") -or $TriageText.Contains("C:") -or $TriageText.Contains("Users/") -or
        $TriageText.Contains("https://") -or $TriageText.Contains("http://") -or $TriageText.Contains("Unity") -or
        $TriageText.Contains("Unreal") -or $TriageText.Contains("Godot")) {
        Write-Error "asset import regression operator loop triage text contains forbidden source, URL, or external-engine material."
    }
}

function Write-OperatorTriageFile {
    param(
        [Parameter(Mandatory = $true)]$Triage,
        [Parameter(Mandatory = $true)][string]$TriageOutputRoot
    )

    $triageText = ConvertTo-TriageText -Triage $Triage
    Assert-TriageTextSafe -TriageText $triageText

    $triagePath = Join-Path $TriageOutputRoot "triage.geoperator"
    if ($PSCmdlet.ShouldProcess($triagePath, "Write asset import regression operator triage")) {
        Write-Utf8TextFile -Path $triagePath -Text $triageText
    }
}

function Invoke-RepoScript {
    param(
        [Parameter(Mandatory = $true)][string]$RelativeScript,
        [string[]]$ScriptArguments = @()
    )

    $scriptPath = Join-Path $PSScriptRoot $RelativeScript
    $arguments = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $scriptPath) + $ScriptArguments
    Invoke-CheckedCommand -FilePath $pwshCommand -Arguments $arguments
}

function Invoke-OperatorLoopSyntheticSmoke {
    param([Parameter(Mandatory = $true)][string]$SyntheticRoot)

    if (Test-Path -LiteralPath $SyntheticRoot) {
        Remove-Item -LiteralPath $SyntheticRoot -Recurse -Force
    }

    try {
        $reportPath = Join-Path $SyntheticRoot "report.gereport"
        $triageRoot = Join-Path $SyntheticRoot "triage"
        Write-Utf8TextFile -Path $reportPath -Text @'
format=GameEngine.AssetImportRegressionReport.v1
corpus_id=GameEngine.AssetImportRegressionCorpus.v1
run_id=run-operator-loop-smoke
asset_count=6
succeeded_count=1
failed_count=5
legal_blocked_count=1
nondeterministic_count=1
ready=false
row.count=6
row.0.asset_id=mesh.legal
row.0.kind=gltf_mesh
row.0.asset=100
row.0.source_path=sources/gltf/legal.gltf
row.0.source_sha256=sha256:legal
row.0.preset_sha256=sha256:preset-legal
row.0.importer_id=mirakana.importer.gltf_mesh
row.0.importer_version=asset-import-regression-v1
row.0.phase=legal
row.0.code=rejected_license
row.0.message=license row rejected after source review
row.0.deterministic_output_hash=
row.0.succeeded=false
row.0.ready_for_commercial_evidence=false
row.1.asset_id=mesh.hash
row.1.kind=gltf_mesh
row.1.asset=101
row.1.source_path=sources/gltf/hash.gltf
row.1.source_sha256=sha256:actual
row.1.preset_sha256=sha256:preset-hash
row.1.importer_id=mirakana.importer.gltf_mesh
row.1.importer_version=asset-import-regression-v1
row.1.phase=source
row.1.code=source_hash_mismatch
row.1.message=source hash mismatch
row.1.deterministic_output_hash=
row.1.succeeded=false
row.1.ready_for_commercial_evidence=false
row.2.asset_id=mesh.axis
row.2.kind=gltf_mesh
row.2.asset=102
row.2.source_path=sources/gltf/axis.gltf
row.2.source_sha256=sha256:axis
row.2.preset_sha256=sha256:preset-axis
row.2.importer_id=mirakana.importer.gltf_mesh
row.2.importer_version=asset-import-regression-v1
row.2.phase=normalization
row.2.code=coordinate_normalization_failed
row.2.message=axis or unit preview required
row.2.deterministic_output_hash=
row.2.succeeded=false
row.2.ready_for_commercial_evidence=false
row.3.asset_id=texture.codec
row.3.kind=png_texture
row.3.asset=103
row.3.source_path=sources/textures/codec.png
row.3.source_sha256=sha256:codec
row.3.preset_sha256=sha256:preset-codec
row.3.importer_id=mirakana.importer.png_texture
row.3.importer_version=asset-import-regression-v1
row.3.phase=decode
row.3.code=texture_decode_failed
row.3.message=texture codec adapter failed
row.3.deterministic_output_hash=
row.3.succeeded=false
row.3.ready_for_commercial_evidence=false
row.4.asset_id=audio.nondeterministic
row.4.kind=audio_source
row.4.asset=104
row.4.source_path=sources/audio/nondeterministic.wav
row.4.source_sha256=sha256:audio
row.4.preset_sha256=sha256:preset-audio
row.4.importer_id=mirakana.importer.audio_source
row.4.importer_version=asset-import-regression-v1
row.4.phase=determinism
row.4.code=nondeterministic_output
row.4.message=fresh process hash mismatch
row.4.deterministic_output_hash=
row.4.succeeded=false
row.4.ready_for_commercial_evidence=false
row.5.asset_id=material.ok
row.5.kind=material_document
row.5.asset=105
row.5.source_path=sources/materials/ok.material
row.5.source_sha256=sha256:material
row.5.preset_sha256=sha256:preset-material
row.5.importer_id=mirakana.importer.material_document
row.5.importer_version=asset-import-regression-v1
row.5.phase=cook
row.5.code=none
row.5.message=asset import succeeded
row.5.deterministic_output_hash=fnv64:0123456789abcdef
row.5.succeeded=true
row.5.ready_for_commercial_evidence=true
'@

        $lines = @(Invoke-OperatorLoopReport -InputReportPath $reportPath -TriageOutputRoot $triageRoot)
        Assert-LinePresent $lines "asset_import_regression_operator_loop_report_rows=6" "asset import regression operator-loop synthetic smoke"
        Assert-LinePresent $lines "asset_import_regression_operator_loop_failed_rows=5" "asset import regression operator-loop synthetic smoke"
        Assert-LinePresent $lines "asset_import_regression_operator_loop_legal_blocked_rows=1" "asset import regression operator-loop synthetic smoke"
        Assert-LinePresent $lines "asset_import_regression_operator_loop_nondeterministic_rows=1" "asset import regression operator-loop synthetic smoke"
        Assert-LinePresent $lines "asset_import_regression_operator_loop_reimport_candidates=2" "asset import regression operator-loop synthetic smoke"
        Assert-LinePresent $lines "asset_import_regression_operator_loop_blocked_rows=3" "asset import regression operator-loop synthetic smoke"
        Assert-LinePresent $lines "asset_import_regression_operator_loop_preset_diff_required=1" "asset import regression operator-loop synthetic smoke"
        Assert-LinePresent $lines "asset_import_regression_operator_loop_axis_unit_preview_required=1" "asset import regression operator-loop synthetic smoke"

        $triageLines = @(Get-Content -LiteralPath (Join-Path $triageRoot "triage.geoperator") -Encoding utf8)
        Assert-LinePresent $triageLines "failed_count=5" "asset import regression operator-loop generated triage"
        Assert-LinePresent $triageLines "legal_blocked_count=1" "asset import regression operator-loop generated triage"
        Assert-LinePresent $triageLines "nondeterministic_count=1" "asset import regression operator-loop generated triage"

        Invoke-RepoScript -RelativeScript "cmake.ps1" -ScriptArguments @("--build", "--preset", "dev", "--target", "MK_asset_import_regression_tests", "MK_editor_core_tests")
        Invoke-RepoScript -RelativeScript "ctest.ps1" -ScriptArguments @("--preset", "dev", "--output-on-failure", "-R", "MK_asset_import_regression_tests|MK_editor_core_tests")

        $lines
        "asset_import_regression_operator_loop_synthetic_smoke=1"
    } finally {
        if (Test-Path -LiteralPath $SyntheticRoot) {
            Remove-Item -LiteralPath $SyntheticRoot -Recurse -Force
        }
    }
}

function Invoke-OperatorLoopReport {
    param(
        [Parameter(Mandatory = $true)][string]$InputReportPath,
        [Parameter(Mandatory = $true)][string]$TriageOutputRoot
    )

    $report = Read-AssetImportRegressionReport -Path $InputReportPath
    Assert-AssetImportReportCounterConsistency -Report $report -Context "asset import regression operator loop report"
    $triage = New-OperatorTriage -Report $report
    Write-OperatorTriageFile -Triage $triage -TriageOutputRoot $TriageOutputRoot

    "asset_import_regression_operator_loop_triage_format=$($triage.Format)"
    "asset_import_regression_operator_loop_report_rows=$($triage.RowCount)"
    "asset_import_regression_operator_loop_failed_rows=$($report.FailedCount)"
    "asset_import_regression_operator_loop_legal_blocked_rows=$($triage.LegalBlockedCount)"
    "asset_import_regression_operator_loop_nondeterministic_rows=$($triage.NondeterministicCount)"
    "asset_import_regression_operator_loop_reimport_candidates=$($triage.ReimportCandidateCount)"
    "asset_import_regression_operator_loop_blocked_rows=$($triage.BlockedCount)"
    "asset_import_regression_operator_loop_preset_diff_required=$($triage.PresetDiffRequiredCount)"
    "asset_import_regression_operator_loop_axis_unit_preview_required=$($triage.AxisUnitPreviewRequiredCount)"
    "asset_import_regression_operator_loop_ready=$([int]$triage.ReadyForOperatorReview)"
    "asset_import_regression_operator_loop_editor_core_value_only=1"
    "asset_import_regression_operator_loop_external_engine_claim=0"
}

function Get-OperatorLoopRetainedReports {
    param(
        [Parameter(Mandatory = $true)][string]$ResolvedCorpusRoot,
        [Parameter(Mandatory = $true)][bool]$RequireReadyGate
    )

    if (Test-ReparsePointPath -Path $ResolvedCorpusRoot) {
        if ($RequireReadyGate) {
            Write-RequireReadyDiagnostic -Code "require_ready.corpus_root_reparse_point"
        }
        Write-Error "asset import regression operator loop corpus root must not be a reparse point."
    }

    $retainedRoot = Join-Path $ResolvedCorpusRoot "retained"
    if (-not (Test-Path -LiteralPath $retainedRoot -PathType Container)) {
        if ($RequireReadyGate) {
            Write-RequireReadyDiagnostic -Code "require_ready.retained_reports_missing"
        }
        Write-Error "asset import regression operator loop retained report root is missing: $retainedRoot"
    }
    if (Test-ReparsePointPath -Path $retainedRoot) {
        if ($RequireReadyGate) {
            Write-RequireReadyDiagnostic -Code "require_ready.retained_root_reparse_point"
        }
        Write-Error "asset import regression operator loop retained root must not be a reparse point."
    }

    $reportFiles = @(Get-ChildItem -LiteralPath $retainedRoot -Recurse -Filter "report.gereport" -File | Sort-Object FullName)
    if ($reportFiles.Count -eq 0) {
        if ($RequireReadyGate) {
            Write-RequireReadyDiagnostic -Code "require_ready.retained_reports_missing"
        }
        Write-Error "asset import regression operator loop retained report files are missing."
    }

    foreach ($reportFile in $reportFiles) {
        if ((Test-ReparsePointPath -Path $reportFile.FullName) -or -not (Test-PathUnderRoot -Root $ResolvedCorpusRoot -Path $reportFile.FullName)) {
            if ($RequireReadyGate) {
                Write-RequireReadyDiagnostic -Code "require_ready.retained_report_path_rejected"
            }
            Write-Error "asset import regression operator loop retained report path is unsafe: $($reportFile.FullName)"
        }
    }

    return $reportFiles
}

function Invoke-OperatorLoopCorpusRoot {
    param(
        [Parameter(Mandatory = $true)][string]$ResolvedCorpusRoot,
        [Parameter(Mandatory = $true)][string]$TriageOutputRoot,
        [Parameter(Mandatory = $true)][bool]$RequireReadyGate
    )

    $reportFiles = @(Get-OperatorLoopRetainedReports -ResolvedCorpusRoot $ResolvedCorpusRoot -RequireReadyGate $RequireReadyGate)
    $successReportCount = 0
    $failureReportCount = 0
    $rowCount = 0
    $failedCount = 0
    $legalBlockedCount = 0
    $nondeterministicCount = 0
    $reimportCandidateCount = 0
    $blockedCount = 0
    $presetDiffRequiredCount = 0
    $axisUnitPreviewRequiredCount = 0
    $triageEntries = [System.Collections.Generic.List[object]]::new()

    for ($index = 0; $index -lt $reportFiles.Count; $index++) {
        $reportFile = $reportFiles[$index]
        $report = Read-AssetImportRegressionReport -Path $reportFile.FullName
        Assert-AssetImportReportCounterConsistency -Report $report -Context "asset import regression operator loop retained report"
        if (Test-OperatorLoopSuccessReport -Report $report) {
            $successReportCount++
        }
        if (Test-OperatorLoopFailureReport -Report $report) {
            $failureReportCount++
        }

        $triage = New-OperatorTriage -Report $report
        $runId = ConvertTo-SafeToken -Value $report.RunId
        $triageEntries.Add([pscustomobject]@{
                Index  = $index
                RunId  = $runId
                Triage = $triage
            }) | Out-Null

        $rowCount += $triage.RowCount
        $failedCount += $report.FailedCount
        $legalBlockedCount += $triage.LegalBlockedCount
        $nondeterministicCount += $triage.NondeterministicCount
        $reimportCandidateCount += $triage.ReimportCandidateCount
        $blockedCount += $triage.BlockedCount
        $presetDiffRequiredCount += $triage.PresetDiffRequiredCount
        $axisUnitPreviewRequiredCount += $triage.AxisUnitPreviewRequiredCount
    }

    if ($RequireReadyGate) {
        if ($successReportCount -eq 0) {
            Write-RequireReadyDiagnostic -Code "require_ready.retained_success_report_missing"
        }
        if ($failureReportCount -eq 0) {
            Write-RequireReadyDiagnostic -Code "require_ready.retained_failure_report_missing"
        }
    }

    foreach ($triageEntry in $triageEntries) {
        $triageRoot = Join-Path $TriageOutputRoot "retained-$($triageEntry.Index)-$($triageEntry.RunId)"
        Write-OperatorTriageFile -Triage $triageEntry.Triage -TriageOutputRoot $triageRoot
    }

    $corpusReady = $successReportCount -gt 0 -and $failureReportCount -gt 0
    "asset_import_regression_operator_loop_corpus_retained_reports=$($reportFiles.Count)"
    "asset_import_regression_operator_loop_corpus_success_reports=$successReportCount"
    "asset_import_regression_operator_loop_corpus_failure_reports=$failureReportCount"
    "asset_import_regression_operator_loop_require_ready=$([int]$RequireReadyGate)"
    "asset_import_regression_operator_loop_triage_format=GameEngine.AssetImportRegressionTriage.v1"
    "asset_import_regression_operator_loop_report_rows=$rowCount"
    "asset_import_regression_operator_loop_failed_rows=$failedCount"
    "asset_import_regression_operator_loop_legal_blocked_rows=$legalBlockedCount"
    "asset_import_regression_operator_loop_nondeterministic_rows=$nondeterministicCount"
    "asset_import_regression_operator_loop_reimport_candidates=$reimportCandidateCount"
    "asset_import_regression_operator_loop_blocked_rows=$blockedCount"
    "asset_import_regression_operator_loop_preset_diff_required=$presetDiffRequiredCount"
    "asset_import_regression_operator_loop_axis_unit_preview_required=$axisUnitPreviewRequiredCount"
    "asset_import_regression_operator_loop_ready=$([int]($rowCount -gt 0))"
    "asset_import_regression_operator_loop_editor_core_value_only=1"
    "asset_import_regression_operator_loop_external_engine_claim=0"
    "asset_import_regression_operator_loop_corpus_ready=$([int]$corpusReady)"
}

if ($SyntheticSmoke -and ((-not [string]::IsNullOrWhiteSpace($ReportPath)) -or (-not [string]::IsNullOrWhiteSpace($CorpusRoot)) -or $RequireReady)) {
    Write-Error "-SyntheticSmoke cannot be combined with -ReportPath, -CorpusRoot, or -RequireReady."
}

if ($SyntheticSmoke) {
    $syntheticRoot = if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
        ConvertTo-LocalPath "out/tmp/asset-import-regression-operator-loop-$PID"
    } else {
        ConvertTo-LocalPath $OutputRoot
    }
    Invoke-OperatorLoopSyntheticSmoke -SyntheticRoot $syntheticRoot
    Write-Information "asset-import-regression-operator-loop-check: synthetic smoke ok" -InformationAction Continue
    exit 0
}

if ((-not [string]::IsNullOrWhiteSpace($CorpusRoot)) -and (-not [string]::IsNullOrWhiteSpace($ReportPath))) {
    Write-Error "-CorpusRoot and -ReportPath are mutually exclusive."
}

if ($RequireReady -and [string]::IsNullOrWhiteSpace($CorpusRoot)) {
    Write-Error "-RequireReady requires -CorpusRoot."
}

if (-not [string]::IsNullOrWhiteSpace($CorpusRoot)) {
    $resolvedCorpusRoot = ConvertTo-LocalPath $CorpusRoot
    if (-not (Test-Path -LiteralPath $resolvedCorpusRoot -PathType Container)) {
        if ($RequireReady) {
            Write-RequireReadyDiagnostic -Code "require_ready.corpus_root_missing"
        }
        Write-Error "asset import regression operator loop corpus root is missing: $resolvedCorpusRoot"
    }
    $resolvedCorpusRoot = [System.IO.Path]::GetFullPath((Resolve-Path -LiteralPath $resolvedCorpusRoot -ErrorAction Stop).ProviderPath)
    $resolvedOutputRoot = if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
        ConvertTo-LocalPath "out/asset-import-regression/operator-loop"
    } else {
        ConvertTo-LocalPath $OutputRoot
    }

    Invoke-OperatorLoopCorpusRoot -ResolvedCorpusRoot $resolvedCorpusRoot -TriageOutputRoot $resolvedOutputRoot -RequireReadyGate ([bool]$RequireReady)
    Write-Information "asset-import-regression-operator-loop-check: corpus ok" -InformationAction Continue
    exit 0
}

if ([string]::IsNullOrWhiteSpace($ReportPath)) {
    Write-Error "ReportPath is required unless -SyntheticSmoke is used."
}

$resolvedReportPath = ConvertTo-LocalPath $ReportPath
$resolvedOutputRoot = if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    ConvertTo-LocalPath "out/asset-import-regression/operator-loop"
} else {
    ConvertTo-LocalPath $OutputRoot
}

Invoke-OperatorLoopReport -InputReportPath $resolvedReportPath -TriageOutputRoot $resolvedOutputRoot
Write-Information "asset-import-regression-operator-loop-check: ok" -InformationAction Continue
