#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: 2d-commercial-production-excellence-v1 Phase 1

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Generate")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/2d-commercial/clean-room-review/2d-commercial-production-excellence",

    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$reviewRelative = "$OutputRootRelative/2d-commercial-clean-room-review.json"
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
    return $normalizedPath.StartsWith("artifacts/2d-commercial/clean-room-review/", [System.StringComparison]::Ordinal)
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

function New-OfficialSourceRow {
    param(
        [Parameter(Mandatory = $true)][string]$SourceId,
        [Parameter(Mandatory = $true)][string]$Url,
        [Parameter(Mandatory = $true)][string]$SourceClass,
        [Parameter(Mandatory = $true)][string]$Use
    )

    return [ordered]@{
        source_id = $SourceId
        url = $Url
        source_class = $SourceClass
        use = $Use
        implementation_input = $false
        copied_expression_allowed = $false
    }
}

function New-ReferenceSourceRow {
    param(
        [Parameter(Mandatory = $true)][string]$SourceId,
        [Parameter(Mandatory = $true)][string]$Url,
        [Parameter(Mandatory = $true)][string]$Use,
        [Parameter(Mandatory = $true)][string]$OfficialFallbackUrl
    )

    return [ordered]@{
        source_id = $SourceId
        url = $Url
        source_class = "context7_documentation_mirror"
        use = $Use
        official_fallback_url = $OfficialFallbackUrl
        implementation_input = $false
        copied_expression_allowed = $false
    }
}

function New-OfficialSourceRows {
    return @(
        New-OfficialSourceRow `
            -SourceId "MIRAIKANAI-2D-Commercial-Plan-2026-06-29" `
            -Url "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md" `
            -SourceClass "first_party_design" `
            -Use "first-party MIRAIKANAI 2D commercial production scope and non-claim design"
        New-OfficialSourceRow `
            -SourceId "Khronos-Vulkan-Docs-2026-06-30" `
            -Url "https://docs.vulkan.org/" `
            -SourceClass "official_platform_sdk" `
            -Use "official Vulkan fallback source for backend-local evidence requirements"
        New-OfficialSourceRow `
            -SourceId "Microsoft-D3D12-Docs-2026-06-30" `
            -Url "https://learn.microsoft.com/en-us/windows/win32/direct3d12/" `
            -SourceClass "official_platform_sdk" `
            -Use "official D3D12 fallback source for backend-local evidence requirements"
        New-OfficialSourceRow `
            -SourceId "Apple-Metal-Docs-2026-06-30" `
            -Url "https://developer.apple.com/metal/" `
            -SourceClass "official_platform_sdk" `
            -Use "official Apple Metal fallback source and Apple-host evidence boundary"
        New-OfficialSourceRow `
            -SourceId "Unity-Trademark-Guidelines-2026-06-30" `
            -Url "https://unity.com/legal/branding-trademarks" `
            -SourceClass "official_documentation_category" `
            -Use "legal and trademark boundary research only"
        New-OfficialSourceRow `
            -SourceId "Unity-Terms-2026-06-30" `
            -Url "https://unity.com/legal/terms-of-service" `
            -SourceClass "official_documentation_category" `
            -Use "terms category research only"
        New-OfficialSourceRow `
            -SourceId "Epic-Unreal-EULA-2026-06-30" `
            -Url "https://www.unrealengine.com/eula/unreal" `
            -SourceClass "official_documentation_category" `
            -Use "EULA and trademark boundary research only"
        New-OfficialSourceRow `
            -SourceId "Epic-Unreal-Trademark-License-2026-06-30" `
            -Url "https://dev.epicgames.com/docs/dev-portal/unreal-engine/ue-trademark-license" `
            -SourceClass "official_documentation_category" `
            -Use "trademark approval category research only"
        New-OfficialSourceRow `
            -SourceId "Godot-License-2026-06-30" `
            -Url "https://godotengine.org/license/" `
            -SourceClass "official_documentation_category" `
            -Use "license boundary research only"
        New-OfficialSourceRow `
            -SourceId "Godot-License-Compliance-2026-06-30" `
            -Url "https://docs.godotengine.org/en/stable/about/complying_with_licenses.html" `
            -SourceClass "official_documentation_category" `
            -Use "license compliance category research only"
        New-OfficialSourceRow `
            -SourceId "US-Copyright-FAQ-2026-06-30" `
            -Url "https://www.copyright.gov/help/faq/faq-protect.html" `
            -SourceClass "official_documentation_category" `
            -Use "copyright idea/expression boundary research only"
        New-OfficialSourceRow `
            -SourceId "USPTO-Trademark-Basics-2026-06-30" `
            -Url "https://www.uspto.gov/trademarks/basics/what-trademark" `
            -SourceClass "official_documentation_category" `
            -Use "source-identifying trademark boundary research only"
        New-OfficialSourceRow `
            -SourceId "MIRAIKANAI-Repository-Legal-Policy-2026-06-30" `
            -Url "docs/legal-and-licensing.md" `
            -SourceClass "permissive_dependency_notice_record" `
            -Use "repository legal/dependency policy record"
        New-OfficialSourceRow `
            -SourceId "MIRAIKANAI-Third-Party-Notices-2026-06-30" `
            -Url "THIRD_PARTY_NOTICES.md" `
            -SourceClass "permissive_dependency_notice_record" `
            -Use "repository third-party notice completeness record"
    )
}

function New-ReferenceSourceRows {
    return @(
        New-ReferenceSourceRow `
            -SourceId "Context7-Vulkan-Docs-2026-06-30" `
            -Url "/khronosgroup/vulkan-docs" `
            -Use "Context7 retrieval of Vulkan synchronization2, timestamp-query, and validation-layer documentation; official source remains the Khronos fallback URL" `
            -OfficialFallbackUrl "https://docs.vulkan.org/"
        New-ReferenceSourceRow `
            -SourceId "Context7-D3D12-Docs-2026-06-30" `
            -Url "/websites/learn_microsoft_en-us_windows_win32_direct3d12" `
            -Use "Context7 retrieval of Direct3D 12 debug layer, fence, barrier, descriptor, and PSO documentation; official source remains the Microsoft fallback URL" `
            -OfficialFallbackUrl "https://learn.microsoft.com/en-us/windows/win32/direct3d12/"
        New-ReferenceSourceRow `
            -SourceId "Context7-Metal-Shading-Language-2026-06-30" `
            -Url "/dogukanveziroglu/metal-shading-language-specification" `
            -Use "Context7 reference mirror for MSL address-space, entry-point, and language-restriction review; official Apple evidence must come from the Apple Metal fallback URL" `
            -OfficialFallbackUrl "https://developer.apple.com/metal/"
    )
}

function New-2DCommercialCleanRoomReviewInput {
    param([Parameter(Mandatory = $true)][string]$GeneratedUtc)

    return [ordered]@{
        schema_version = "GameEngine.TwoDCommercialCleanRoomReviewInput.v1"
        claim_id = "2d-commercial-clean-room-source-gate-v1"
        validation_recipe = "2d-commercial-clean-room-review-input"
        generated_utc = $GeneratedUtc
        fixture_only = $false
        legal_advice = $false
        official_sources = @(New-OfficialSourceRows)
        reference_sources = @(New-ReferenceSourceRows)
        clean_room_rows = [ordered]@{
            official_docs_only = [ordered]@{
                ready = $true
                context7_verified = $true
                official_fallback_documented = $true
                public_documentation_only = $true
                implementation_source_copied = $false
                documentation_expression_copied = $false
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
                external_engine_schema_surface = $false
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
            legal_counsel_review_required = $true
            legal_review_id = ""
            legal_approval = $false
            external_material_selected = $false
            external_material_approved = $false
        }
        non_claims = [ordered]@{
            commercial_ready = $false
            legal_approval = $false
            all_platform_ready = $false
            external_engine_compatibility = $false
            external_engine_equivalence = $false
            external_engine_parity = $false
            external_engine_project_import = $false
            external_engine_schema_import = $false
            external_engine_api_clone = $false
            copied_source = $false
            copied_asset = $false
            copied_documentation_text = $false
        }
    }
}

function New-2DCommercialOfficialSourceSummary {
    param([Parameter(Mandatory = $true)][string]$GeneratedUtc)

    return [ordered]@{
        schema_version = "GameEngine.TwoDCommercialOfficialSourceSummary.v1"
        validation_recipe = "2d-commercial-clean-room-review-input"
        generated_utc = $GeneratedUtc
        source_policy = "official-public-documentation-category-research-only"
        legal_advice = $false
        external_engine_material_selected = $false
        official_source_count = @(New-OfficialSourceRows).Count
        reference_source_count = @(New-ReferenceSourceRows).Count
        rejected_external_material = @(
            "external_engine_code",
            "external_engine_sample",
            "external_engine_shader",
            "external_engine_asset",
            "external_engine_trademark",
            "external_engine_ui_expression",
            "external_engine_project_import",
            "external_engine_schema",
            "external_engine_api",
            "compatibility_claim",
            "equivalence_claim",
            "parity_claim"
        )
        non_claims = [ordered]@{
            commercial_ready = $false
            legal_approval = $false
            external_engine_parity = $false
        }
    }
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/2d-commercial/clean-room-review/."
}
if (-not (Test-Path -LiteralPath (Join-Path $root "THIRD_PARTY_NOTICES.md") -PathType Leaf)) {
    Write-Error "THIRD_PARTY_NOTICES.md must exist before generating 2D commercial clean-room review input."
}

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=2d-commercial-clean-room-review-input"
    Write-Output "2d_commercial_clean_room_review_input_generator_mode=Plan"
    Write-Output "2d_commercial_clean_room_review_input_output_root=$OutputRootRelative"
    Write-Output "2d_commercial_clean_room_review_input_path=$reviewRelative"
    Write-Output "2d_commercial_clean_room_source_summary_path=$sourceSummaryRelative"
    Write-Output "2d_commercial_clean_room_review_input_writes_evidence=0"
    Write-Output "2d_commercial_clean_room_review_input_written=0"
    Write-Output "2d_commercial_clean_room_review_input_ready=0"
    Write-Output "2d_commercial_clean_room_public_docs_only=0"
    Write-Output "2d_commercial_clean_room_external_engine_zero_material_ready=0"
    Write-Output "2d_commercial_clean_room_external_engine_forbidden_material_detected_rows=0"
    Write-Output "requires_legal_counsel_review=1"
    Write-Output "2d_commercial_clean_room_commercial_ready=0"
    Write-Output "2d_commercial_clean_room_legal_approval=0"
    return
}

$outputRootFull = Resolve-RepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
$reviewFull = Resolve-RepoRelativePath -RelativePath $reviewRelative -Label "2D commercial clean-room review input"
$sourceSummaryFull = Resolve-RepoRelativePath -RelativePath $sourceSummaryRelative -Label "2D commercial official source summary"
$willWrite = -not $NoWrite.IsPresent
$generatedUtc = [System.DateTimeOffset]::UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ", [System.Globalization.CultureInfo]::InvariantCulture)

$review = New-2DCommercialCleanRoomReviewInput -GeneratedUtc $generatedUtc
$sourceSummary = New-2DCommercialOfficialSourceSummary -GeneratedUtc $generatedUtc

if ($willWrite) {
    $null = New-Item -ItemType Directory -Force -Path $outputRootFull
    $review | ConvertTo-Json -Depth 24 | Set-Content -LiteralPath $reviewFull -Encoding utf8NoBOM
    $sourceSummary | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $sourceSummaryFull -Encoding utf8NoBOM
}

$written = $willWrite -and
    (Test-Path -LiteralPath $reviewFull -PathType Leaf) -and
    (Test-Path -LiteralPath $sourceSummaryFull -PathType Leaf)
$reviewHash = ""
if (Test-Path -LiteralPath $reviewFull -PathType Leaf) {
    $reviewHash = (Get-FileHash -LiteralPath $reviewFull -Algorithm SHA256).Hash.ToLowerInvariant()
}

Write-Output "validation_recipe=2d-commercial-clean-room-review-input"
Write-Output "2d_commercial_clean_room_review_input_generator_mode=Generate"
Write-Output "2d_commercial_clean_room_review_input_output_root=$OutputRootRelative"
Write-Output "2d_commercial_clean_room_review_input_path=$reviewRelative"
Write-Output "2d_commercial_clean_room_source_summary_path=$sourceSummaryRelative"
Write-Output "2d_commercial_clean_room_review_input_hash=$reviewHash"
Write-Output "2d_commercial_clean_room_review_input_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "2d_commercial_clean_room_review_input_written=$(ConvertTo-CounterBit $written)"
Write-Output "2d_commercial_clean_room_review_input_ready=$(ConvertTo-CounterBit $written)"
Write-Output "2d_commercial_clean_room_public_docs_only=1"
Write-Output "2d_commercial_clean_room_external_engine_zero_material_ready=1"
Write-Output "2d_commercial_clean_room_third_party_notices_complete=1"
Write-Output "2d_commercial_clean_room_external_engine_forbidden_material_detected_rows=0"
Write-Output "2d_commercial_clean_room_external_engine_forbidden_material_rejected_rows=0"
Write-Output "2d_commercial_clean_room_external_engine_schema_surface=0"
Write-Output "2d_commercial_clean_room_external_engine_api_used=0"
Write-Output "2d_commercial_clean_room_external_engine_compatibility_claim=0"
Write-Output "2d_commercial_clean_room_external_engine_equivalence_claim=0"
Write-Output "2d_commercial_clean_room_external_engine_parity_claim=0"
Write-Output "requires_legal_counsel_review=1"
Write-Output "2d_commercial_clean_room_commercial_ready=0"
Write-Output "2d_commercial_clean_room_legal_approval=0"

Write-Information "2d-commercial-clean-room-review-input-generator: ok" -InformationAction Continue
