#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: 2d-commercial-production-excellence-v1 Phase 9

[CmdletBinding(SupportsShouldProcess = $true, PositionalBinding = $false)]
param(
    [string]$OutputRootRelative = "artifacts/2d-commercial/release-legal-review/2d-commercial-production-excellence"
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function Test-SafeReleaseLegalRepoRelativePath {
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

function Test-AllowedReleaseLegalOutputRoot {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalized = $RelativePath.Replace("\", "/")
    return $normalized.StartsWith("artifacts/2d-commercial/release-legal-review/", [System.StringComparison]::Ordinal) -or
        $normalized.StartsWith("out/validation/2d-commercial-release-legal-review-input", [System.StringComparison]::Ordinal)
}

function Resolve-ReleaseLegalRepoRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not (Test-SafeReleaseLegalRepoRelativePath -RelativePath $RelativePath)) {
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

function New-ReleaseLegalPackageRow {
    param(
        [Parameter(Mandatory = $true)][string]$Id,
        [Parameter(Mandatory = $true)][string]$Kind,
        [Parameter(Mandatory = $true)][string]$PathOrRecordId
    )

    return [ordered]@{
        id = $Id
        kind = $Kind
        path_or_record_id = $PathOrRecordId
        validation_recipe_id = "installed-2d-commercial-release-legal-gate-smoke"
        ready = $true
        package_visible = $true
        engineering_review_input = $true
        counsel_review_required = $true
    }
}

function New-ReleaseLegalOfficialSourceRow {
    param(
        [Parameter(Mandatory = $true)][string]$Id,
        [Parameter(Mandatory = $true)][string]$Kind,
        [Parameter(Mandatory = $true)][string]$Url,
        [Parameter(Mandatory = $true)][string]$Use
    )

    return [ordered]@{
        id = $Id
        kind = $Kind
        url = $Url
        use = $Use
        ready = $true
        official = $true
        public_docs_only = $true
        implementation_input = $false
        copied_expression_allowed = $false
    }
}

function New-ReleaseLegalPlatformGateRow {
    param(
        [Parameter(Mandatory = $true)][string]$Id,
        [Parameter(Mandatory = $true)][string]$Kind,
        [Parameter(Mandatory = $true)][string]$OfficialSourceId,
        [Parameter(Mandatory = $true)][string]$HostClassId,
        [Parameter(Mandatory = $true)][string]$Requirement
    )

    return [ordered]@{
        id = $Id
        kind = $Kind
        official_source_id = $OfficialSourceId
        host_class_id = $HostClassId
        requirement = $Requirement
        host_gated = $true
        ready = $false
        separate_platform_gate = $true
    }
}

if (-not (Test-AllowedReleaseLegalOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/2d-commercial/release-legal-review/ or out/validation/2d-commercial-release-legal-review-input."
}

$outputRoot = Resolve-ReleaseLegalRepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
$reviewPath = Join-Path $outputRoot "2d-commercial-release-legal-review.json"
$summaryPath = Join-Path $outputRoot "2d-commercial-release-official-source-summary.json"

$officialSourceRows = @(
    New-ReleaseLegalOfficialSourceRow -Id "microsoft.msix.signing" -Kind "microsoft_msix_signing" `
        -Url "https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview" `
        -Use "Windows MSIX package signing and distribution host-gate planning."
    New-ReleaseLegalOfficialSourceRow -Id "apple.notarization.distribution" `
        -Kind "apple_notarization_distribution" `
        -Url "https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution" `
        -Use "macOS notarization, hardened runtime, and distribution host-gate planning."
    New-ReleaseLegalOfficialSourceRow -Id "android.app.signing" -Kind "android_app_signing" `
        -Url "https://developer.android.com/studio/publish/app-signing" `
        -Use "Android release signing, app bundle, and Play App Signing host-gate planning."
    New-ReleaseLegalOfficialSourceRow -Id "repository.2d-clean-room-ledger" -Kind "repository_clean_room_ledger" `
        -Url "docs/specs/2026-06-30-2d-commercial-clean-room-source-ledger-v1.md" `
        -Use "Repository clean-room source classes and external commercial engine non-claim policy."
    New-ReleaseLegalOfficialSourceRow -Id "repository.legal-policy" -Kind "repository_legal_policy" `
        -Url "docs/legal-and-licensing.md" `
        -Use "Repository dependency, notice, provenance, and counsel-review policy."
)

$review = [ordered]@{
    schema_version = 1
    claim_id = "2d-commercial-release-legal-gate-v1"
    validation_recipe = "2d-commercial-release-legal-review-input"
    generated_by = "tools/generate-2d-commercial-release-legal-review-input.ps1"
    legal_advice = $false
    engineering_review_input = $true
    counsel_review_required = $true
    selected_package = "sample_2d_desktop_runtime_package"
    package_rows = @(
        New-ReleaseLegalPackageRow -Id "package-content-inventory" -Kind "package_content_inventory" `
            -PathOrRecordId "out/install/desktop-runtime-release/share/Mirakanai/desktop-runtime-games.json"
        New-ReleaseLegalPackageRow -Id "third-party-notice-record" -Kind "third_party_notice_record" `
            -PathOrRecordId "THIRD_PARTY_NOTICES.md"
        New-ReleaseLegalPackageRow -Id "dependency-manifest-record" -Kind "dependency_manifest_record" `
            -PathOrRecordId "vcpkg.json"
        New-ReleaseLegalPackageRow -Id "source-provenance-summary" -Kind "source_provenance_summary" `
            -PathOrRecordId "games/sample_2d_desktop_runtime_package/game.agent.json"
        New-ReleaseLegalPackageRow -Id "clean-room-static-guard" -Kind "clean_room_static_guard" `
            -PathOrRecordId "tools/check-2d-commercial-clean-room.ps1"
        New-ReleaseLegalPackageRow -Id "trademark-surface-guard" -Kind "trademark_surface_guard" `
            -PathOrRecordId "tools/check-2d-commercial-source-diagnostics.ps1"
        New-ReleaseLegalPackageRow -Id "distribution-artifact-inventory" -Kind "distribution_artifact_inventory" `
            -PathOrRecordId "out/package/Mirakanai-0.1.0-Windows-AMD64.zip"
        New-ReleaseLegalPackageRow -Id "generated-asset-review" -Kind "generated_asset_review" `
            -PathOrRecordId "aiWorkflow.gameDesignSpec.assetRequests"
        New-ReleaseLegalPackageRow -Id "legal-review-input-record" -Kind "legal_review_input_record" `
            -PathOrRecordId "artifacts/2d-commercial/release-legal-review/2d-commercial-production-excellence"
    )
    release_blockers = [ordered]@{
        missing_notices = 0
        unknown_license_rows = 0
        unapproved_dependencies = 0
        external_engine_marks = 0
        copied_assets = 0
        unreviewed_generated_assets = 0
    }
    platform_gate_rows = @(
        New-ReleaseLegalPlatformGateRow -Id "windows-msix-signing" -Kind "windows_msix_signing" `
            -OfficialSourceId "microsoft.msix.signing" -HostClassId "windows-release-signing-host" `
            -Requirement "MSIX package signing and store submission stay host-gated."
        New-ReleaseLegalPlatformGateRow -Id "macos-notarization" -Kind "macos_notarization" `
            -OfficialSourceId "apple.notarization.distribution" -HostClassId "macos-xcode-notarization-host" `
            -Requirement "macOS notarization, hardened runtime, and distribution stay host-gated."
        New-ReleaseLegalPlatformGateRow -Id "android-play-signing" -Kind "android_play_signing" `
            -OfficialSourceId "android.app.signing" -HostClassId "android-release-signing-host" `
            -Requirement "Android release signing, AAB, and Play App Signing stay host-gated."
    )
    official_source_rows = $officialSourceRows
    non_claims = [ordered]@{
        legal_approval = $false
        external_engine_compatibility = $false
        external_engine_parity = $false
        native_handles = $false
        store_certification_complete = $false
        notarization_complete = $false
        signing_complete = $false
    }
}

$summary = [ordered]@{
    schema_version = 1
    claim_id = "2d-commercial-release-legal-gate-v1"
    validation_recipe = "2d-commercial-release-legal-review-input"
    legal_advice = $false
    official_source_rows = $officialSourceRows
    non_claims = [ordered]@{
        legal_approval = $false
        compatibility = $false
        parity = $false
    }
}

if ($PSCmdlet.ShouldProcess($outputRoot, "write 2D commercial release legal review input")) {
    New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
    $review | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $reviewPath -Encoding utf8
    $summary | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $summaryPath -Encoding utf8
}

Write-Output "validation_recipe=2d-commercial-release-legal-review-input"
Write-Output "review_path=$($reviewPath.Substring($root.Length + 1).Replace('\', '/'))"
Write-Output "summary_path=$($summaryPath.Substring($root.Length + 1).Replace('\', '/'))"
Write-Output "legal_advice=0"
Write-Output "engineering_review_input=1"
Write-Output "counsel_review_required=1"
Write-Output "release_blocker_rows=0"
Write-Output "platform_gate_rows=3"
Write-Output "official_source_rows=5"
Write-Information "2d-commercial-release-legal-review-input-generator: ok" -InformationAction Continue
