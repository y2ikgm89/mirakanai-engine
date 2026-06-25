#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10E

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Assemble")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/clean-room-legal",

    [string]$CleanRoomLegalReviewRelative = "",

    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$artifactRelative = "$OutputRootRelative/clean-room-legal.json"

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

function Test-AllowedReviewEvidencePath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    return $normalizedPath.StartsWith(
        "artifacts/renderer/commercial-readiness-evidence/",
        [System.StringComparison]::Ordinal) -or
        $normalizedPath.StartsWith(
            "artifacts/renderer/clean-room-legal-review/",
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

function Read-JsonFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Write-Error "$Label does not exist: $Path"
    }
    try {
        return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    }
    catch {
        Write-Error "$Label is not valid JSON: $Path"
    }
}

function Get-JsonPropertyValue {
    param(
        [AllowNull()]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if ($null -eq $JsonObject) {
        return $null
    }
    $property = $JsonObject.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }
    return $property.Value
}

function Assert-ExactJsonProperties {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string[]]$ExpectedNames,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($null -eq $JsonObject) {
        Write-Error "$Label is missing."
    }
    $actualNames = @($JsonObject.PSObject.Properties.Name)
    foreach ($expected in $ExpectedNames) {
        if ($actualNames -notcontains $expected) {
            Write-Error "$Label is missing required property '$expected'."
        }
    }
    foreach ($actual in $actualNames) {
        if ($ExpectedNames -notcontains $actual) {
            Write-Error "$Label has unexpected property '$actual'."
        }
    }
}

function Assert-StringProperty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $actual = [string](Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name)
    if ($actual -cne $Expected) {
        Write-Error "$Label expected $Name=$Expected but found '$actual'."
    }
}

function Assert-TrueProperty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    if ($true -ne [bool]$value) {
        Write-Error "$Label expected $Name=true."
    }
}

function Assert-BooleanProperty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    if ($value -isnot [bool]) {
        Write-Error "$Label expected $Name to be boolean."
    }
}

function Assert-FalseProperty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    if ($false -ne [bool]$value -or $value -isnot [bool]) {
        Write-Error "$Label expected $Name=false."
    }
}

function Test-ForbiddenMaterialRows {
    param([Parameter(Mandatory = $true)]$Rows)

    $detectedRows = 0
    $rejectedRows = 0
    foreach ($row in @($Rows)) {
        if ($null -eq $row) {
            continue
        }
        Assert-ExactJsonProperties -JsonObject $row -Label "forbidden_material_row" -ExpectedNames @(
            "material_kind",
            "detected",
            "rejected",
            "used_in_engine",
            "diagnostic_code"
        )
        $kind = [string](Get-JsonPropertyValue -JsonObject $row -Name "material_kind")
        if ($kind -notin @(
                "external_engine_code",
                "external_engine_sample",
                "external_engine_shader",
                "external_engine_asset",
                "external_engine_trademark",
                "external_engine_ui_expression",
                "external_engine_api",
                "compatibility_claim",
                "equivalence_claim",
                "parity_claim"
            )) {
            Write-Error "forbidden_material_row has invalid material_kind=$kind."
        }
        Assert-BooleanProperty -JsonObject $row -Name "detected" -Label "forbidden_material_row"
        Assert-TrueProperty -JsonObject $row -Name "rejected" -Label "forbidden_material_row"
        Assert-FalseProperty -JsonObject $row -Name "used_in_engine" -Label "forbidden_material_row"
        Assert-StringProperty -JsonObject $row -Name "diagnostic_code" `
            -Expected "external_engine_material_rejected" `
            -Label "forbidden_material_row"
        if ([bool](Get-JsonPropertyValue -JsonObject $row -Name "detected")) {
            $detectedRows += 1
        }
        if ([bool](Get-JsonPropertyValue -JsonObject $row -Name "rejected")) {
            $rejectedRows += 1
        }
    }

    return [pscustomobject]@{
        DetectedRows = $detectedRows
        RejectedRows = $rejectedRows
    }
}

function Test-CleanRoomRows {
    param([Parameter(Mandatory = $true)]$Rows)

    Assert-ExactJsonProperties -JsonObject $Rows -Label "clean_room_rows" -ExpectedNames @(
        "official_docs_only",
        "legal_review",
        "external_engine_zero_material_review",
        "third_party_notices",
        "forbidden_material_rows"
    )

    $officialDocs = Get-JsonPropertyValue -JsonObject $Rows -Name "official_docs_only"
    foreach ($requiredTrue in @(
            "ready",
            "public_documentation_only",
            "context7_verified",
            "official_fallback_documented",
            "external_engine_source_review_complete"
        )) {
        Assert-TrueProperty -JsonObject $officialDocs -Name $requiredTrue -Label "official_docs_only"
    }

    $legalReview = Get-JsonPropertyValue -JsonObject $Rows -Name "legal_review"
    foreach ($requiredTrue in @(
            "ready",
            "unity_terms_reviewed",
            "unreal_eula_trademark_reviewed",
            "godot_trademark_reviewed"
        )) {
        Assert-TrueProperty -JsonObject $legalReview -Name $requiredTrue -Label "legal_review"
    }
    foreach ($requiredFalse in @(
            "unity_compatibility",
            "unreal_compatibility",
            "godot_compatibility",
            "compatibility_claims",
            "equivalence_claims",
            "parity_claims"
        )) {
        Assert-FalseProperty -JsonObject $legalReview -Name $requiredFalse -Label "legal_review"
    }

    $zeroMaterialReview = Get-JsonPropertyValue -JsonObject $Rows -Name "external_engine_zero_material_review"
    foreach ($booleanName in @(
            "ready",
            "external_engine_code_used",
            "external_engine_sample_used",
            "external_engine_shader_used",
            "external_engine_asset_used",
            "external_engine_trademark_used",
            "external_engine_ui_expression_used",
            "external_engine_project_import_used",
            "external_engine_api_used",
            "external_engine_compatibility_claim",
            "external_engine_equivalence_claim",
            "external_engine_parity_claim"
        )) {
        Assert-BooleanProperty -JsonObject $zeroMaterialReview -Name $booleanName `
            -Label "external_engine_zero_material_review"
    }

    $thirdPartyNotices = Get-JsonPropertyValue -JsonObject $Rows -Name "third_party_notices"
    Assert-TrueProperty -JsonObject $thirdPartyNotices -Name "ready" -Label "third_party_notices"
    Assert-TrueProperty -JsonObject $thirdPartyNotices -Name "complete" -Label "third_party_notices"
    Assert-StringProperty -JsonObject $thirdPartyNotices -Name "notices_path" `
        -Expected "THIRD_PARTY_NOTICES.md" `
        -Label "third_party_notices"
    if (-not (Test-Path -LiteralPath (Join-Path $root "THIRD_PARTY_NOTICES.md") -PathType Leaf)) {
        Write-Error "THIRD_PARTY_NOTICES.md must exist for clean-room legal artifact production."
    }

    $forbiddenRows = @(Get-JsonPropertyValue -JsonObject $Rows -Name "forbidden_material_rows")
    $forbiddenSummary = Test-ForbiddenMaterialRows -Rows $forbiddenRows
    $zeroMaterialReady = [bool](Get-JsonPropertyValue -JsonObject $zeroMaterialReview -Name "ready")
    if ($forbiddenSummary.DetectedRows -eq 0 -and -not $zeroMaterialReady) {
        Write-Error "external_engine_zero_material_review.ready must be true when no forbidden material rows are detected."
    }
    if ($forbiddenSummary.DetectedRows -gt 0 -and $zeroMaterialReady) {
        Write-Error "external_engine_zero_material_review.ready must be false when forbidden material rows are detected."
    }

    return $forbiddenSummary
}

function Test-CleanRoomLegalReview {
    param([Parameter(Mandatory = $true)]$Review)

    Assert-ExactJsonProperties -JsonObject $Review -Label "clean-room legal review" -ExpectedNames @(
        "schema_version",
        "claim_id",
        "validation_recipe",
        "fixture_only",
        "source_rows",
        "clean_room_rows",
        "human_review",
        "non_claims"
    )
    Assert-StringProperty -JsonObject $Review -Name "schema_version" `
        -Expected "GameEngine.RendererCleanRoomLegalReviewInput.v1" `
        -Label "clean-room legal review"
    Assert-StringProperty -JsonObject $Review -Name "claim_id" `
        -Expected "renderer-clean-room-legal-artifact-v1" `
        -Label "clean-room legal review"
    Assert-StringProperty -JsonObject $Review -Name "validation_recipe" `
        -Expected "renderer-clean-room-legal-artifact" `
        -Label "clean-room legal review"

    $fixtureOnly = Get-JsonPropertyValue -JsonObject $Review -Name "fixture_only"
    if ($fixtureOnly -isnot [bool]) {
        Write-Error "clean-room legal review expected fixture_only to be boolean."
    }
    if ([bool]$fixtureOnly) {
        Write-Error "fixture_artifact_input_rejected: $CleanRoomLegalReviewRelative"
    }

    $sourceRows = Get-JsonPropertyValue -JsonObject $Review -Name "source_rows"
    Assert-ExactJsonProperties -JsonObject $sourceRows -Label "source_rows" -ExpectedNames @(
        "unity_terms_source_id",
        "unity_trademark_source_id",
        "unreal_eula_source_id",
        "unreal_release_trademark_source_id",
        "godot_license_source_id",
        "godot_trademark_source_id"
    )
    Assert-StringProperty -JsonObject $sourceRows -Name "unity_terms_source_id" `
        -Expected "Unity-Legal-Terms-2026-06-25" -Label "source_rows"
    Assert-StringProperty -JsonObject $sourceRows -Name "unity_trademark_source_id" `
        -Expected "Unity-Trademark-Guidelines-2026-06-25" -Label "source_rows"
    Assert-StringProperty -JsonObject $sourceRows -Name "unreal_eula_source_id" `
        -Expected "Epic-Unreal-Engine-EULA-Trademark-2026-06-25" -Label "source_rows"
    Assert-StringProperty -JsonObject $sourceRows -Name "unreal_release_trademark_source_id" `
        -Expected "Epic-Unreal-Engine-Release-Trademark-2026-06-25" -Label "source_rows"
    Assert-StringProperty -JsonObject $sourceRows -Name "godot_license_source_id" `
        -Expected "Godot-License-2026-06-25" -Label "source_rows"
    Assert-StringProperty -JsonObject $sourceRows -Name "godot_trademark_source_id" `
        -Expected "Godot-Trademark-Licensing-2026-06-25" -Label "source_rows"

    $forbiddenSummary = Test-CleanRoomRows -Rows (Get-JsonPropertyValue -JsonObject $Review -Name "clean_room_rows")

    $humanReview = Get-JsonPropertyValue -JsonObject $Review -Name "human_review"
    Assert-ExactJsonProperties -JsonObject $humanReview -Label "human_review" -ExpectedNames @(
        "external_material_selected",
        "legal_review_required_for_external_material",
        "technical_review_required_for_external_material",
        "legal_review_id",
        "technical_review_id",
        "external_material_approved"
    )
    foreach ($name in @(
            "external_material_selected",
            "legal_review_required_for_external_material",
            "technical_review_required_for_external_material",
            "external_material_approved"
        )) {
        Assert-BooleanProperty -JsonObject $humanReview -Name $name -Label "human_review"
    }

    $externalMaterialSelected = [bool](Get-JsonPropertyValue -JsonObject $humanReview -Name "external_material_selected")
    $legalReviewId = [string](Get-JsonPropertyValue -JsonObject $humanReview -Name "legal_review_id")
    $technicalReviewId = [string](Get-JsonPropertyValue -JsonObject $humanReview -Name "technical_review_id")
    $externalMaterialApproved = [bool](Get-JsonPropertyValue -JsonObject $humanReview -Name "external_material_approved")
    $externalMaterialHumanApproved =
        (-not $externalMaterialSelected) -or
        ((-not [string]::IsNullOrWhiteSpace($legalReviewId)) -and
            (-not [string]::IsNullOrWhiteSpace($technicalReviewId)) -and
            $externalMaterialApproved)

    $nonClaims = Get-JsonPropertyValue -JsonObject $Review -Name "non_claims"
    foreach ($booleanName in @(
            "environment_ready",
            "native_handles_exposed",
            "cross_backend_inference",
            "external_engine_parity",
            "external_engine_code_used",
            "external_engine_sample_used",
            "external_engine_shader_used",
            "external_engine_asset_used",
            "external_engine_trademark_used",
            "external_engine_ui_expression_used",
            "external_engine_project_import_used",
            "external_engine_api_used",
            "external_engine_compatibility_claim",
            "external_engine_equivalence_claim",
            "external_engine_parity_claim",
            "unity_compatibility",
            "unreal_compatibility",
            "godot_compatibility",
            "renderer_commercial_readiness"
        )) {
        Assert-BooleanProperty -JsonObject $nonClaims -Name $booleanName -Label "non_claims"
    }

    $forbiddenFlags = @(
        "environment_ready",
        "native_handles_exposed",
        "cross_backend_inference",
        "external_engine_parity",
        "external_engine_code_used",
        "external_engine_sample_used",
        "external_engine_shader_used",
        "external_engine_asset_used",
        "external_engine_trademark_used",
        "external_engine_ui_expression_used",
        "external_engine_project_import_used",
        "external_engine_api_used",
        "external_engine_compatibility_claim",
        "external_engine_equivalence_claim",
        "external_engine_parity_claim",
        "unity_compatibility",
        "unreal_compatibility",
        "godot_compatibility",
        "renderer_commercial_readiness"
    )
    $forbiddenNonClaimFlagCount = 0
    foreach ($name in $forbiddenFlags) {
        if ([bool](Get-JsonPropertyValue -JsonObject $nonClaims -Name $name)) {
            $forbiddenNonClaimFlagCount += 1
        }
    }

    $cleanRoomRows = Get-JsonPropertyValue -JsonObject $Review -Name "clean_room_rows"
    $zeroMaterialReview = Get-JsonPropertyValue -JsonObject $cleanRoomRows `
        -Name "external_engine_zero_material_review"
    $cleanReady =
        ([bool](Get-JsonPropertyValue -JsonObject $zeroMaterialReview -Name "ready")) -and
        ($forbiddenSummary.DetectedRows -eq 0) -and
        ($forbiddenNonClaimFlagCount -eq 0) -and
        $externalMaterialHumanApproved

    return [pscustomobject]@{
        Ready = $cleanReady
        ForbiddenDetectedRows = $forbiddenSummary.DetectedRows
        ForbiddenRejectedRows = $forbiddenSummary.RejectedRows
        ExternalEngineShaderUsed = [bool](Get-JsonPropertyValue -JsonObject $nonClaims -Name "external_engine_shader_used")
        ExternalEngineApiUsed = [bool](Get-JsonPropertyValue -JsonObject $nonClaims -Name "external_engine_api_used")
        ExternalEngineCompatibilityClaim = [bool](Get-JsonPropertyValue -JsonObject $nonClaims -Name "external_engine_compatibility_claim")
        ExternalEngineEquivalenceClaim = [bool](Get-JsonPropertyValue -JsonObject $nonClaims -Name "external_engine_equivalence_claim")
        ExternalEngineParityClaim = [bool](Get-JsonPropertyValue -JsonObject $nonClaims -Name "external_engine_parity_claim")
    }
}

function New-CleanRoomLegalArtifact {
    param(
        [Parameter(Mandatory = $true)]$Review,
        [Parameter(Mandatory = $true)]$ReviewSummary
    )

    return [ordered]@{
        schema_version = "GameEngine.RendererCleanRoomLegalArtifact.v1"
        artifact_id = "clean-room-legal"
        validation_recipe = "renderer-clean-room-legal-artifact"
        fixture_only = $false
        ready = [bool]$ReviewSummary.Ready
        source_rows = Get-JsonPropertyValue -JsonObject $Review -Name "source_rows"
        clean_room_rows = Get-JsonPropertyValue -JsonObject $Review -Name "clean_room_rows"
        human_review = Get-JsonPropertyValue -JsonObject $Review -Name "human_review"
        non_claims = Get-JsonPropertyValue -JsonObject $Review -Name "non_claims"
    }
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/commercial-readiness-evidence/."
}

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=renderer-clean-room-legal-artifact"
    Write-Output "renderer_clean_room_legal_artifact_collector_mode=Plan"
    Write-Output "renderer_clean_room_legal_artifact_output_root=$OutputRootRelative"
    Write-Output "renderer_clean_room_legal_artifact_writes_evidence=0"
    Write-Output "renderer_clean_room_legal_artifact_written=0"
    Write-Output "renderer_clean_room_legal_fixture_artifact=0"
    Write-Output "renderer_clean_room_legal_ready=0"
    Write-Output "renderer_clean_room_public_docs_only=0"
    Write-Output "renderer_clean_room_external_engine_zero_material_ready=0"
    Write-Output "renderer_third_party_notices_complete=0"
    Write-Output "renderer_external_engine_forbidden_material_detected_rows=0"
    Write-Output "renderer_external_engine_forbidden_material_rejected_rows=0"
    Write-Output "renderer_external_engine_shader_used=0"
    Write-Output "renderer_external_engine_api_used=0"
    Write-Output "renderer_external_engine_compatibility_claim=0"
    Write-Output "renderer_external_engine_equivalence_claim=0"
    Write-Output "renderer_external_engine_parity_claim=0"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    Write-Output "renderer_environment_ready=0"
    return
}

if ([string]::IsNullOrWhiteSpace($CleanRoomLegalReviewRelative)) {
    Write-Error "clean_room_legal_review_required"
}
if (-not (Test-SafeRepoRelativePath -RelativePath $CleanRoomLegalReviewRelative)) {
    Write-Error "unsafe_relative_path: CleanRoomLegalReviewRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedReviewEvidencePath -RelativePath $CleanRoomLegalReviewRelative)) {
    Write-Error "CleanRoomLegalReviewRelative must be under artifacts/renderer/commercial-readiness-evidence/ or artifacts/renderer/clean-room-legal-review/."
}

$reviewFull = Resolve-RepoRelativePath `
    -RelativePath $CleanRoomLegalReviewRelative `
    -Label "CleanRoomLegalReviewRelative"
$outputRootFull = Resolve-RepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
$artifactFull = Resolve-RepoRelativePath -RelativePath $artifactRelative -Label "clean-room legal artifact"
$review = Read-JsonFile -Path $reviewFull -Label "CleanRoomLegalReviewRelative"
$reviewSummary = Test-CleanRoomLegalReview -Review $review
$artifact = New-CleanRoomLegalArtifact -Review $review -ReviewSummary $reviewSummary
$willWrite = -not $NoWrite.IsPresent

if ($willWrite) {
    $null = New-Item -ItemType Directory -Path $outputRootFull -Force
    $artifact | ConvertTo-Json -Depth 24 |
        Set-Content -LiteralPath $artifactFull -Encoding utf8NoBOM
}

$artifactHash = ""
if (Test-Path -LiteralPath $artifactFull -PathType Leaf) {
    $artifactHash = (Get-FileHash -LiteralPath $artifactFull -Algorithm SHA256).Hash.ToLowerInvariant()
}

Write-Output "validation_recipe=renderer-clean-room-legal-artifact"
Write-Output "renderer_clean_room_legal_artifact_collector_mode=Assemble"
Write-Output "renderer_clean_room_legal_artifact_output_root=$OutputRootRelative"
Write-Output "renderer_clean_room_legal_artifact_path=$artifactRelative"
Write-Output "renderer_clean_room_legal_artifact_hash=$artifactHash"
Write-Output "renderer_clean_room_legal_artifact_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_clean_room_legal_artifact_written=$(ConvertTo-CounterBit ($willWrite -and (Test-Path -LiteralPath $artifactFull -PathType Leaf)))"
Write-Output "renderer_clean_room_legal_fixture_artifact=0"
Write-Output "renderer_clean_room_legal_ready=$(ConvertTo-CounterBit $reviewSummary.Ready)"
Write-Output "renderer_clean_room_public_docs_only=1"
Write-Output "renderer_clean_room_external_engine_zero_material_ready=$(ConvertTo-CounterBit $reviewSummary.Ready)"
Write-Output "renderer_third_party_notices_complete=1"
Write-Output "renderer_external_engine_forbidden_material_detected_rows=$($reviewSummary.ForbiddenDetectedRows)"
Write-Output "renderer_external_engine_forbidden_material_rejected_rows=$($reviewSummary.ForbiddenRejectedRows)"
Write-Output "renderer_external_engine_shader_used=$(ConvertTo-CounterBit $reviewSummary.ExternalEngineShaderUsed)"
Write-Output "renderer_external_engine_api_used=$(ConvertTo-CounterBit $reviewSummary.ExternalEngineApiUsed)"
Write-Output "renderer_external_engine_compatibility_claim=$(ConvertTo-CounterBit $reviewSummary.ExternalEngineCompatibilityClaim)"
Write-Output "renderer_external_engine_equivalence_claim=$(ConvertTo-CounterBit $reviewSummary.ExternalEngineEquivalenceClaim)"
Write-Output "renderer_external_engine_parity_claim=$(ConvertTo-CounterBit $reviewSummary.ExternalEngineParityClaim)"
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"

Write-Information "renderer-clean-room-legal-artifact-collector: ok" -InformationAction Continue
