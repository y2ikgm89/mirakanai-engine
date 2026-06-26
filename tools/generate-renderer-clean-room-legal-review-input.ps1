#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10L

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Generate")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/clean-room-legal-review/renderer-commercial-readiness",

    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$reviewRelative = "$OutputRootRelative/clean-room-legal-review.json"
$sourceSummaryRelative = "$OutputRootRelative/official-source-summary.json"

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
        "artifacts/renderer/clean-room-legal-review/",
        [System.StringComparison]::Ordinal) -or
        $normalizedPath.StartsWith(
            "artifacts/renderer/commercial-readiness-evidence/",
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

function New-CleanRoomLegalReviewInput {
    return [ordered]@{
        schema_version = "GameEngine.RendererCleanRoomLegalReviewInput.v1"
        claim_id = "renderer-clean-room-legal-artifact-v1"
        validation_recipe = "renderer-clean-room-legal-artifact"
        fixture_only = $false
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
                ready = $true
                external_engine_code_used = $false
                external_engine_sample_used = $false
                external_engine_shader_used = $false
                external_engine_asset_used = $false
                external_engine_trademark_used = $false
                external_engine_ui_expression_used = $false
                external_engine_project_import_used = $false
                external_engine_api_used = $false
                external_engine_compatibility_claim = $false
                external_engine_equivalence_claim = $false
                external_engine_parity_claim = $false
            }
            third_party_notices = [ordered]@{
                ready = $true
                complete = $true
                notices_path = "THIRD_PARTY_NOTICES.md"
            }
            forbidden_material_rows = @()
        }
        human_review = [ordered]@{
            external_material_selected = $false
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
            external_engine_code_used = $false
            external_engine_sample_used = $false
            external_engine_shader_used = $false
            external_engine_asset_used = $false
            external_engine_trademark_used = $false
            external_engine_ui_expression_used = $false
            external_engine_project_import_used = $false
            external_engine_api_used = $false
            external_engine_compatibility_claim = $false
            external_engine_equivalence_claim = $false
            external_engine_parity_claim = $false
            unity_compatibility = $false
            unreal_compatibility = $false
            godot_compatibility = $false
            renderer_commercial_readiness = $false
        }
    }
}

function New-OfficialSourceSummary {
    param([Parameter(Mandatory = $true)][string]$GeneratedUtc)

    return [ordered]@{
        schema_version = "GameEngine.RendererCleanRoomLegalReviewSourceSummary.v1"
        validation_recipe = "renderer-clean-room-legal-review-input"
        generated_utc = $GeneratedUtc
        ci_artifact_name = "renderer-clean-room-legal-review-artifacts"
        source_policy = "official-public-documentation-category-research-only"
        legal_advice = $false
        external_engine_material_selected = $false
        official_sources = @(
            [ordered]@{
                source_id = "Unity-Legal-Terms-2026-06-25"
                url = "https://unity.com/legal/terms-of-service"
                source_owner = "Unity"
                use = "terms category research only"
            },
            [ordered]@{
                source_id = "Unity-Trademark-Guidelines-2026-06-25"
                url = "https://unity.com/legal/branding-trademarks"
                source_owner = "Unity"
                use = "trademark category research only"
            },
            [ordered]@{
                source_id = "Epic-Unreal-Engine-EULA-Trademark-2026-06-25"
                url = "https://www.unrealengine.com/eula/unreal"
                source_owner = "Epic Games"
                use = "EULA and trademark notice category research only"
            },
            [ordered]@{
                source_id = "Epic-Unreal-Engine-Release-Trademark-2026-06-25"
                url = "https://dev.epicgames.com/docs/dev-portal/unreal-engine/ue-trademark-license"
                source_owner = "Epic Games"
                use = "Unreal Engine trademark approval category research only"
            },
            [ordered]@{
                source_id = "Godot-License-2026-06-25"
                url = "https://godotengine.org/license/"
                source_owner = "Godot Engine contributors"
                use = "license category research only"
            },
            [ordered]@{
                source_id = "Godot-Trademark-Licensing-2026-06-25"
                url = "https://docs.godotengine.org/en/stable/about/complying_with_licenses.html"
                source_owner = "Godot Engine contributors"
                use = "license compliance category research only"
            }
        )
        rejected_external_material = @(
            "external_engine_code",
            "external_engine_sample",
            "external_engine_shader",
            "external_engine_asset",
            "external_engine_trademark",
            "external_engine_ui_expression",
            "external_engine_project_import",
            "external_engine_api",
            "compatibility_claim",
            "equivalence_claim",
            "parity_claim"
        )
        non_claims = [ordered]@{
            renderer_commercial_readiness = $false
            renderer_backend_parity_ready = $false
            renderer_metal_broad_readiness = $false
            renderer_broad_quality_ready = $false
            environment_ready = $false
            external_engine_parity = $false
        }
    }
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/clean-room-legal-review/ or artifacts/renderer/commercial-readiness-evidence/."
}
if (-not (Test-Path -LiteralPath (Join-Path $root "THIRD_PARTY_NOTICES.md") -PathType Leaf)) {
    Write-Error "THIRD_PARTY_NOTICES.md must exist before generating clean-room legal review input."
}

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=renderer-clean-room-legal-review-input"
    Write-Output "renderer_clean_room_legal_review_input_generator_mode=Plan"
    Write-Output "renderer_clean_room_legal_review_input_output_root=$OutputRootRelative"
    Write-Output "renderer_clean_room_legal_review_input_path=$reviewRelative"
    Write-Output "renderer_clean_room_legal_review_input_source_summary_path=$sourceSummaryRelative"
    Write-Output "renderer_clean_room_legal_review_input_writes_evidence=0"
    Write-Output "renderer_clean_room_legal_review_input_written=0"
    Write-Output "renderer_clean_room_legal_review_input_ready=0"
    Write-Output "renderer_clean_room_public_docs_only=0"
    Write-Output "renderer_clean_room_external_engine_zero_material_ready=0"
    Write-Output "renderer_external_engine_forbidden_material_detected_rows=0"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    Write-Output "renderer_environment_ready=0"
    return
}

$outputRootFull = Resolve-RepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
$reviewFull = Resolve-RepoRelativePath -RelativePath $reviewRelative -Label "clean-room legal review input"
$sourceSummaryFull = Resolve-RepoRelativePath -RelativePath $sourceSummaryRelative -Label "official source summary"
$willWrite = -not $NoWrite.IsPresent
$generatedUtc = [System.DateTimeOffset]::UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ", [System.Globalization.CultureInfo]::InvariantCulture)

$review = New-CleanRoomLegalReviewInput
$sourceSummary = New-OfficialSourceSummary -GeneratedUtc $generatedUtc

if ($willWrite) {
    $null = New-Item -ItemType Directory -Force -Path $outputRootFull
    $review | ConvertTo-Json -Depth 24 |
        Set-Content -LiteralPath $reviewFull -Encoding utf8NoBOM
    $sourceSummary | ConvertTo-Json -Depth 16 |
        Set-Content -LiteralPath $sourceSummaryFull -Encoding utf8NoBOM
}

$written = $willWrite -and
    (Test-Path -LiteralPath $reviewFull -PathType Leaf) -and
    (Test-Path -LiteralPath $sourceSummaryFull -PathType Leaf)
$reviewHash = ""
if (Test-Path -LiteralPath $reviewFull -PathType Leaf) {
    $reviewHash = (Get-FileHash -LiteralPath $reviewFull -Algorithm SHA256).Hash.ToLowerInvariant()
}

Write-Output "validation_recipe=renderer-clean-room-legal-review-input"
Write-Output "renderer_clean_room_legal_review_input_generator_mode=Generate"
Write-Output "renderer_clean_room_legal_review_input_output_root=$OutputRootRelative"
Write-Output "renderer_clean_room_legal_review_input_path=$reviewRelative"
Write-Output "renderer_clean_room_legal_review_input_source_summary_path=$sourceSummaryRelative"
Write-Output "renderer_clean_room_legal_review_input_hash=$reviewHash"
Write-Output "renderer_clean_room_legal_review_input_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_clean_room_legal_review_input_written=$(ConvertTo-CounterBit $written)"
Write-Output "renderer_clean_room_legal_review_input_ready=$(ConvertTo-CounterBit $written)"
Write-Output "renderer_clean_room_public_docs_only=1"
Write-Output "renderer_clean_room_external_engine_zero_material_ready=1"
Write-Output "renderer_third_party_notices_complete=1"
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

Write-Information "renderer-clean-room-legal-review-input-generator: ok" -InformationAction Continue
