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

function Write-JsonObject {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][object]$Value
    )

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $parent -Force
    }
    $json = $Value | ConvertTo-Json -Depth 16
    Set-Content -LiteralPath $Path -Value $json -Encoding utf8NoBOM
}

function New-CleanRoomLegalReview {
    param(
        [Parameter(Mandatory = $true)][bool]$FixtureOnly,
        [Parameter(Mandatory = $true)][bool]$ForbiddenMaterial
    )

    $forbiddenRows = @()
    if ($ForbiddenMaterial) {
        $forbiddenRows = foreach ($materialKind in @(
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
            [ordered]@{
                material_kind = $materialKind
                detected = $true
                rejected = $true
                used_in_engine = $false
                diagnostic_code = "external_engine_material_rejected"
            }
        }
    }

    return [ordered]@{
        schema_version = "GameEngine.RendererCleanRoomLegalReviewInput.v1"
        claim_id = "renderer-clean-room-legal-artifact-v1"
        validation_recipe = "renderer-clean-room-legal-artifact"
        fixture_only = $FixtureOnly
        source_rows = [ordered]@{
            unity_terms_source_id = "Unity-Legal-Terms-2026-06-25"
            unity_trademark_source_id = "Unity-Trademark-Guidelines-2026-06-25"
            unreal_eula_source_id = "Epic-Unreal-Engine-EULA-Trademark-2026-06-25"
            unreal_release_trademark_source_id = "Epic-Unreal-Engine-Release-Trademark-2026-06-25"
            godot_license_source_id = "Godot-License-2026-06-25"
            godot_trademark_source_id = "Godot-Trademark-Licensing-2026-06-25"
        }
        clean_room_rows = [ordered]@{
            official_docs_only = [ordered]@{
                ready = $true
                public_documentation_only = $true
                context7_verified = $true
                official_fallback_documented = $true
                external_engine_source_review_complete = $true
            }
            legal_review = [ordered]@{
                ready = $true
                unity_terms_reviewed = $true
                unreal_eula_trademark_reviewed = $true
                godot_trademark_reviewed = $true
                unity_compatibility = $false
                unreal_compatibility = $false
                godot_compatibility = $false
                compatibility_claims = $false
                equivalence_claims = $false
                parity_claims = $false
            }
            external_engine_zero_material_review = [ordered]@{
                ready = -not $ForbiddenMaterial
                external_engine_code_used = $ForbiddenMaterial
                external_engine_sample_used = $ForbiddenMaterial
                external_engine_shader_used = $ForbiddenMaterial
                external_engine_asset_used = $ForbiddenMaterial
                external_engine_trademark_used = $ForbiddenMaterial
                external_engine_ui_expression_used = $ForbiddenMaterial
                external_engine_project_import_used = $false
                external_engine_api_used = $ForbiddenMaterial
                external_engine_compatibility_claim = $ForbiddenMaterial
                external_engine_equivalence_claim = $ForbiddenMaterial
                external_engine_parity_claim = $ForbiddenMaterial
            }
            third_party_notices = [ordered]@{
                ready = $true
                complete = $true
                notices_path = "THIRD_PARTY_NOTICES.md"
            }
            forbidden_material_rows = $forbiddenRows
        }
        human_review = [ordered]@{
            external_material_selected = $ForbiddenMaterial
            legal_review_required_for_external_material = $true
            technical_review_required_for_external_material = $true
            legal_review_id = ""
            technical_review_id = ""
            external_material_approved = $false
        }
        non_claims = [ordered]@{
            environment_ready = $false
            native_handles_exposed = $false
            cross_backend_inference = $false
            external_engine_parity = $false
            external_engine_code_used = $ForbiddenMaterial
            external_engine_sample_used = $ForbiddenMaterial
            external_engine_shader_used = $ForbiddenMaterial
            external_engine_asset_used = $ForbiddenMaterial
            external_engine_trademark_used = $ForbiddenMaterial
            external_engine_ui_expression_used = $ForbiddenMaterial
            external_engine_project_import_used = $false
            external_engine_api_used = $ForbiddenMaterial
            external_engine_compatibility_claim = $ForbiddenMaterial
            external_engine_equivalence_claim = $ForbiddenMaterial
            external_engine_parity_claim = $ForbiddenMaterial
            unity_compatibility = $false
            unreal_compatibility = $false
            godot_compatibility = $false
            renderer_commercial_readiness = $false
        }
    }
}

$producerScript = Join-Path $root "tools/collect-renderer-clean-room-legal-artifact.ps1"
if (-not (Test-Path -LiteralPath $producerScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-clean-room-legal-artifact.ps1 must exist for Task 10E clean-room legal artifact production."
}

$collectorScript = Join-Path $root "tools/collect-renderer-commercial-readiness-evidence.ps1"
if (-not (Test-Path -LiteralPath $collectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-commercial-readiness-evidence.ps1 must exist for Task 10 retained artifact assembly."
}

$fixtureRoot = "tests/fixtures/renderer/commercial-readiness-evidence/ready"
$evidenceRootRelative = "artifacts/renderer/commercial-readiness-evidence/clean-room-legal-contract-$PID"
$inputRootRelative = "$evidenceRootRelative/input"
$producerOutputRootRelative = "$evidenceRootRelative/producer-output"
$badOutputRootRelative = "$evidenceRootRelative/bad-output"
$collectorOutputRootRelative = "$evidenceRootRelative/collector-output"
$realInputRelative = "$inputRootRelative/clean-room-legal-review.json"
$fixtureInputRelative = "$inputRootRelative/clean-room-legal-fixture-review.json"
$forbiddenInputRelative = "$inputRootRelative/clean-room-legal-forbidden-review.json"
$evidenceRootPath = ConvertTo-LocalPath $evidenceRootRelative

try {
    if (Test-Path -LiteralPath $evidenceRootPath) {
        Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
    }

    Write-JsonObject -Path (ConvertTo-LocalPath $realInputRelative) `
        -Value (New-CleanRoomLegalReview -FixtureOnly $false -ForbiddenMaterial $false)
    Write-JsonObject -Path (ConvertTo-LocalPath $fixtureInputRelative) `
        -Value (New-CleanRoomLegalReview -FixtureOnly $true -ForbiddenMaterial $false)
    Write-JsonObject -Path (ConvertTo-LocalPath $forbiddenInputRelative) `
        -Value (New-CleanRoomLegalReview -FixtureOnly $false -ForbiddenMaterial $true)

    $planLines = @(& $producerScript -Mode Plan -OutputRootRelative $producerOutputRootRelative)
    foreach ($expectedLine in @(
            "renderer_clean_room_legal_artifact_collector_mode=Plan",
            "renderer_clean_room_legal_artifact_written=0",
            "renderer_clean_room_legal_ready=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $planLines $expectedLine "clean-room legal artifact producer Plan mode"
    }

    $unsafeRejected = $false
    try {
        $null = & $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -CleanRoomLegalReviewRelative "../unsafe.json" 2>&1
    }
    catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "clean-room legal artifact producer must reject unsafe relative paths."
    }

    $fixtureRejected = $false
    try {
        $null = & $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -CleanRoomLegalReviewRelative $fixtureInputRelative 2>&1
    }
    catch {
        $fixtureRejected = [string]$_.Exception.Message -like "*fixture_artifact_input_rejected*"
    }
    if (-not $fixtureRejected) {
        Write-Error "clean-room legal artifact producer must reject fixture-only review evidence."
    }

    $forbiddenLines = @(& $producerScript `
            -Mode Assemble `
            -OutputRootRelative $badOutputRootRelative `
            -CleanRoomLegalReviewRelative $forbiddenInputRelative)
    foreach ($expectedLine in @(
            "renderer_clean_room_legal_artifact_collector_mode=Assemble",
            "renderer_clean_room_legal_artifact_written=1",
            "renderer_clean_room_legal_ready=0",
            "renderer_external_engine_forbidden_material_detected_rows=10",
            "renderer_external_engine_forbidden_material_rejected_rows=10",
            "renderer_external_engine_shader_used=1",
            "renderer_external_engine_api_used=1",
            "renderer_external_engine_compatibility_claim=1",
            "renderer_external_engine_equivalence_claim=1",
            "renderer_external_engine_parity_claim=1",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $forbiddenLines $expectedLine "clean-room legal artifact forbidden material path"
    }

    $assembleLines = @(& $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -CleanRoomLegalReviewRelative $realInputRelative)
    foreach ($expectedLine in @(
            "renderer_clean_room_legal_artifact_collector_mode=Assemble",
            "renderer_clean_room_legal_artifact_written=1",
            "renderer_clean_room_legal_fixture_artifact=0",
            "renderer_clean_room_legal_ready=1",
            "renderer_clean_room_public_docs_only=1",
            "renderer_clean_room_external_engine_zero_material_ready=1",
            "renderer_third_party_notices_complete=1",
            "renderer_external_engine_forbidden_material_detected_rows=0",
            "renderer_external_engine_shader_used=0",
            "renderer_external_engine_api_used=0",
            "renderer_external_engine_compatibility_claim=0",
            "renderer_external_engine_equivalence_claim=0",
            "renderer_external_engine_parity_claim=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $assembleLines $expectedLine "clean-room legal artifact producer Assemble mode"
    }

    $artifactPath = ConvertTo-LocalPath "$producerOutputRootRelative/clean-room-legal.json"
    if (-not (Test-Path -LiteralPath $artifactPath -PathType Leaf)) {
        Write-Error "clean-room legal artifact producer did not write clean-room-legal.json."
    }
    $artifact = Get-Content -LiteralPath $artifactPath -Raw | ConvertFrom-Json
    if ([string]$artifact.schema_version -ne "GameEngine.RendererCleanRoomLegalArtifact.v1") {
        Write-Error "clean-room legal artifact schema_version mismatch."
    }
    if ([string]$artifact.artifact_id -ne "clean-room-legal") {
        Write-Error "clean-room legal artifact_id mismatch."
    }
    if ([bool]$artifact.fixture_only) {
        Write-Error "clean-room legal artifact must not be fixture_only when review evidence is retained real evidence."
    }
    if (-not [bool]$artifact.ready) {
        Write-Error "clean-room legal artifact must be ready for clean first-party review evidence."
    }
    if ([bool]$artifact.non_claims.external_engine_parity) {
        Write-Error "clean-room legal artifact must not claim external-engine parity."
    }

    $collectorArguments = @{
        OutputRootRelative = $collectorOutputRootRelative
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
        CleanRoomLegalArtifactRelative = "$producerOutputRootRelative/clean-room-legal.json"
    }
    $collectorLines = @(& $collectorScript -Mode Assemble @collectorArguments -AllowFixtureArtifactsForSelfTest)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_evidence_collector_fixture_artifacts=11",
            "renderer_commercial_readiness_evidence_collector_real_promotion_candidate=0",
            "renderer_clean_room_legal_ready=1",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $collectorLines $expectedLine `
            "commercial readiness collector with retained clean-room legal artifact"
    }

    $validationLines = @(& (Join-Path $PSScriptRoot "validate-renderer-commercial-readiness-evidence.ps1") `
            -ArtifactRootRelative $collectorOutputRootRelative)
    $validationValues = ConvertFrom-KeyValueLines -Lines $validationLines
    if ([string]$validationValues["renderer_clean_room_legal_ready"] -ne "1") {
        Write-Error "retained clean-room legal artifact must validate as renderer_clean_room_legal_ready."
    }
    if ([string]$validationValues["renderer_external_engine_forbidden_material_detected_rows"] -ne "0") {
        Write-Error "clean-room legal artifact must not introduce forbidden external-engine material."
    }
    if ([string]$validationValues["renderer_commercial_readiness"] -ne "0") {
        Write-Error "clean-room legal producer must not promote renderer_commercial_readiness by itself."
    }
}
finally {
    if (Test-Path -LiteralPath $evidenceRootPath) {
        Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
    }
}

Write-Information "renderer-clean-room-legal-artifact-check: ok" -InformationAction Continue
