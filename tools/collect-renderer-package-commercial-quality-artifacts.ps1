#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10D

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Assemble")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/package-commercial-quality",

    [string]$PackageHostEvidenceRelative = "",

    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$expectedSourceId = "GameEngine-Renderer-Package-Commercial-Quality-2026-06-25"
$artifactSpecs = @(
    @{
        Name = "visible_3d"
        ArtifactId = "visible-3d-package"
        FileName = "visible-3d-package.json"
        Schema = "GameEngine.DesktopRuntimePackageEvidence.v1"
        Recipe = "desktop-3d-package"
    },
    @{
        Name = "runtime_ui"
        ArtifactId = "runtime-ui-package"
        FileName = "runtime-ui-package.json"
        Schema = "GameEngine.DesktopRuntimePackageEvidence.v1"
        Recipe = "desktop-runtime-ui-package"
    },
    @{
        Name = "environment"
        ArtifactId = "environment-package"
        FileName = "environment-package.json"
        Schema = "GameEngine.EnvironmentPackageEvidence.v1"
        Recipe = "environment-package"
    },
    @{
        Name = "generated_game"
        ArtifactId = "generated-game-package"
        FileName = "generated-game-package.json"
        Schema = "GameEngine.GeneratedGamePackageEvidence.v1"
        Recipe = "generated-game-package"
    }
)

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

function Test-AllowedHostEvidencePath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    return $normalizedPath.StartsWith(
        "artifacts/renderer/commercial-readiness-evidence/",
        [System.StringComparison]::Ordinal) -or
        $normalizedPath.StartsWith(
            "artifacts/renderer/package-commercial-quality-host-evidence/",
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

function Assert-IntegerAtLeast {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][long]$Minimum,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    try {
        $integerValue = [long]$value
    }
    catch {
        Write-Error "$Label expected integer $Name."
    }
    if ($integerValue -lt $Minimum) {
        Write-Error "$Label expected $Name >= $Minimum."
    }
}

function Assert-LowerHexSha256 {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $value = [string](Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name)
    if ($value -cnotmatch "^[0-9a-f]{64}$") {
        Write-Error "$Label expected lower-case SHA-256 $Name."
    }
}

function Test-CommonPackageNonClaims {
    param([Parameter(Mandatory = $true)]$NonClaims)

    Assert-ExactJsonProperties -JsonObject $NonClaims -Label "package non_claims" -ExpectedNames @(
        "arbitrary_script_execution",
        "package_script_execution",
        "native_handles_exposed",
        "external_engine_parity",
        "environment_ready"
    )
    foreach ($requiredFalse in @(
            "arbitrary_script_execution",
            "package_script_execution",
            "native_handles_exposed",
            "external_engine_parity",
            "environment_ready"
        )) {
        Assert-FalseProperty -JsonObject $NonClaims -Name $requiredFalse -Label "package non_claims"
    }
}

function Test-Visible3dPackageRow {
    param([Parameter(Mandatory = $true)]$Row)

    Assert-StringProperty -JsonObject $Row -Name "validation_recipe" -Expected "desktop-3d-package" `
        -Label "visible_3d"
    Assert-ExactJsonProperties -JsonObject $Row -Label "visible_3d" -ExpectedNames @(
        "validation_recipe",
        "proof_rows",
        "validation_counters",
        "non_claims"
    )
    $proofRows = Get-JsonPropertyValue -JsonObject $Row -Name "proof_rows"
    Assert-ExactJsonProperties -JsonObject $proofRows -Label "visible_3d proof_rows" -ExpectedNames @(
        "material_render",
        "lighting_row",
        "shadow_postprocess",
        "package_visible_readback",
        "manifest_binding"
    )

    $materialRender = Get-JsonPropertyValue -JsonObject $proofRows -Name "material_render"
    foreach ($requiredTrue in @("ready", "pbr_material_row", "texture_binding_row")) {
        Assert-TrueProperty -JsonObject $materialRender -Name $requiredTrue -Label "visible_3d material_render"
    }
    Assert-IntegerAtLeast -JsonObject $materialRender -Name "material_variant_rows" -Minimum 1 `
        -Label "visible_3d material_render"

    $lightingRow = Get-JsonPropertyValue -JsonObject $proofRows -Name "lighting_row"
    foreach ($requiredTrue in @("ready", "direct_light_row", "ambient_light_row", "lighting_readback_nonzero")) {
        Assert-TrueProperty -JsonObject $lightingRow -Name $requiredTrue -Label "visible_3d lighting_row"
    }

    $shadowPostprocess = Get-JsonPropertyValue -JsonObject $proofRows -Name "shadow_postprocess"
    foreach ($requiredTrue in @("ready", "shadow_or_depth_row", "postprocess_row", "tone_mapping_row")) {
        Assert-TrueProperty -JsonObject $shadowPostprocess -Name $requiredTrue `
            -Label "visible_3d shadow_postprocess"
    }

    $readback = Get-JsonPropertyValue -JsonObject $proofRows -Name "package_visible_readback"
    Assert-TrueProperty -JsonObject $readback -Name "ready" -Label "visible_3d package_visible_readback"
    Assert-LowerHexSha256 -JsonObject $readback -Name "deterministic_hash_sha256" `
        -Label "visible_3d package_visible_readback"
    Assert-IntegerAtLeast -JsonObject $readback -Name "readback_counter_rows" -Minimum 1 `
        -Label "visible_3d package_visible_readback"

    $manifestBinding = Get-JsonPropertyValue -JsonObject $proofRows -Name "manifest_binding"
    Assert-TrueProperty -JsonObject $manifestBinding -Name "ready" -Label "visible_3d manifest_binding"
    Assert-TrueProperty -JsonObject $manifestBinding -Name "game_agent_manifest_row" `
        -Label "visible_3d manifest_binding"
    Assert-StringProperty -JsonObject $manifestBinding -Name "validation_recipe_id" `
        -Expected "desktop-3d-package" -Label "visible_3d manifest_binding"
    $packageManifestRow = [string](Get-JsonPropertyValue -JsonObject $manifestBinding -Name "package_manifest_row")
    if ([string]::IsNullOrWhiteSpace($packageManifestRow)) {
        Write-Error "visible_3d manifest_binding expected non-empty package_manifest_row."
    }

    $counters = Get-JsonPropertyValue -JsonObject $Row -Name "validation_counters"
    Assert-ExactJsonProperties -JsonObject $counters -Label "visible_3d validation_counters" -ExpectedNames @(
        "renderer_visible_3d_material_ready",
        "renderer_visible_3d_lighting_ready",
        "renderer_visible_3d_shadow_postprocess_ready",
        "renderer_visible_3d_readback_hash_ready",
        "renderer_package_arbitrary_script_execution",
        "renderer_package_script_execution"
    )
    foreach ($counter in @(
            "renderer_visible_3d_material_ready",
            "renderer_visible_3d_lighting_ready",
            "renderer_visible_3d_shadow_postprocess_ready",
            "renderer_visible_3d_readback_hash_ready"
        )) {
        Assert-TrueProperty -JsonObject $counters -Name $counter -Label "visible_3d validation_counters"
    }
    Assert-FalseProperty -JsonObject $counters -Name "renderer_package_arbitrary_script_execution" `
        -Label "visible_3d validation_counters"
    Assert-FalseProperty -JsonObject $counters -Name "renderer_package_script_execution" `
        -Label "visible_3d validation_counters"
    Test-CommonPackageNonClaims -NonClaims (Get-JsonPropertyValue -JsonObject $Row -Name "non_claims")
}

function Test-RuntimeUiPackageRow {
    param([Parameter(Mandatory = $true)]$Row)

    Assert-StringProperty -JsonObject $Row -Name "validation_recipe" `
        -Expected "desktop-runtime-ui-package" -Label "runtime_ui"
    Assert-ExactJsonProperties -JsonObject $Row -Label "runtime_ui" -ExpectedNames @(
        "validation_recipe",
        "proof_rows",
        "validation_counters",
        "non_claims"
    )
    $proofRows = Get-JsonPropertyValue -JsonObject $Row -Name "proof_rows"
    Assert-ExactJsonProperties -JsonObject $proofRows -Label "runtime_ui proof_rows" -ExpectedNames @(
        "ui_atlas_upload",
        "ui_atlas_readback",
        "renderer_handoff",
        "manifest_binding"
    )

    $upload = Get-JsonPropertyValue -JsonObject $proofRows -Name "ui_atlas_upload"
    foreach ($requiredTrue in @("ready", "atlas_texture_upload_row", "atlas_texture_usage_sampled")) {
        Assert-TrueProperty -JsonObject $upload -Name $requiredTrue -Label "runtime_ui ui_atlas_upload"
    }
    Assert-IntegerAtLeast -JsonObject $upload -Name "upload_counter_rows" -Minimum 1 `
        -Label "runtime_ui ui_atlas_upload"

    $readback = Get-JsonPropertyValue -JsonObject $proofRows -Name "ui_atlas_readback"
    Assert-TrueProperty -JsonObject $readback -Name "ready" -Label "runtime_ui ui_atlas_readback"
    Assert-IntegerAtLeast -JsonObject $readback -Name "readback_counter_rows" -Minimum 1 `
        -Label "runtime_ui ui_atlas_readback"
    Assert-LowerHexSha256 -JsonObject $readback -Name "deterministic_hash_sha256" `
        -Label "runtime_ui ui_atlas_readback"

    $handoff = Get-JsonPropertyValue -JsonObject $proofRows -Name "renderer_handoff"
    foreach ($requiredTrue in @("ready", "retained_upload_handoff_row", "renderer_consumed_ui_atlas_row")) {
        Assert-TrueProperty -JsonObject $handoff -Name $requiredTrue -Label "runtime_ui renderer_handoff"
    }

    $manifestBinding = Get-JsonPropertyValue -JsonObject $proofRows -Name "manifest_binding"
    Assert-TrueProperty -JsonObject $manifestBinding -Name "ready" -Label "runtime_ui manifest_binding"
    Assert-TrueProperty -JsonObject $manifestBinding -Name "game_agent_manifest_row" `
        -Label "runtime_ui manifest_binding"
    Assert-StringProperty -JsonObject $manifestBinding -Name "validation_recipe_id" `
        -Expected "desktop-runtime-ui-package" -Label "runtime_ui manifest_binding"
    $packageManifestRow = [string](Get-JsonPropertyValue -JsonObject $manifestBinding -Name "package_manifest_row")
    if ([string]::IsNullOrWhiteSpace($packageManifestRow)) {
        Write-Error "runtime_ui manifest_binding expected non-empty package_manifest_row."
    }

    $counters = Get-JsonPropertyValue -JsonObject $Row -Name "validation_counters"
    foreach ($counter in @(
            "renderer_runtime_ui_atlas_upload_ready",
            "renderer_runtime_ui_atlas_readback_ready",
            "renderer_runtime_ui_handoff_ready"
        )) {
        Assert-TrueProperty -JsonObject $counters -Name $counter -Label "runtime_ui validation_counters"
    }
    Assert-FalseProperty -JsonObject $counters -Name "renderer_package_arbitrary_script_execution" `
        -Label "runtime_ui validation_counters"
    Assert-FalseProperty -JsonObject $counters -Name "renderer_package_script_execution" `
        -Label "runtime_ui validation_counters"
    Test-CommonPackageNonClaims -NonClaims (Get-JsonPropertyValue -JsonObject $Row -Name "non_claims")
}

function Test-EnvironmentPackageRow {
    param([Parameter(Mandatory = $true)]$Row)

    Assert-StringProperty -JsonObject $Row -Name "validation_recipe" -Expected "environment-package" `
        -Label "environment"
    Assert-ExactJsonProperties -JsonObject $Row -Label "environment" -ExpectedNames @(
        "validation_recipe",
        "proof_rows",
        "validation_counters",
        "non_claims"
    )
    $proofRows = Get-JsonPropertyValue -JsonObject $Row -Name "proof_rows"
    $consumption = Get-JsonPropertyValue -JsonObject $proofRows -Name "environment_renderer_package_consumption"
    Assert-TrueProperty -JsonObject $consumption -Name "ready" `
        -Label "environment environment_renderer_package_consumption"
    Assert-TrueProperty -JsonObject $consumption -Name "environment_package_row_consumed" `
        -Label "environment environment_renderer_package_consumption"
    Assert-IntegerAtLeast -JsonObject $consumption -Name "renderer_environment_rows_consumed_count" -Minimum 1 `
        -Label "environment environment_renderer_package_consumption"
    Assert-FalseProperty -JsonObject $consumption -Name "environment_ready_promoted" `
        -Label "environment environment_renderer_package_consumption"

    $manifestBinding = Get-JsonPropertyValue -JsonObject $proofRows -Name "manifest_binding"
    Assert-TrueProperty -JsonObject $manifestBinding -Name "ready" -Label "environment manifest_binding"
    Assert-TrueProperty -JsonObject $manifestBinding -Name "game_agent_manifest_row" `
        -Label "environment manifest_binding"
    Assert-StringProperty -JsonObject $manifestBinding -Name "validation_recipe_id" `
        -Expected "environment-package" -Label "environment manifest_binding"
    $packageManifestRow = [string](Get-JsonPropertyValue -JsonObject $manifestBinding -Name "package_manifest_row")
    if ([string]::IsNullOrWhiteSpace($packageManifestRow)) {
        Write-Error "environment manifest_binding expected non-empty package_manifest_row."
    }

    $counters = Get-JsonPropertyValue -JsonObject $Row -Name "validation_counters"
    Assert-TrueProperty -JsonObject $counters -Name "renderer_environment_package_consumption_ready" `
        -Label "environment validation_counters"
    Assert-FalseProperty -JsonObject $counters -Name "renderer_environment_ready_promoted" `
        -Label "environment validation_counters"
    Assert-FalseProperty -JsonObject $counters -Name "renderer_package_arbitrary_script_execution" `
        -Label "environment validation_counters"
    Assert-FalseProperty -JsonObject $counters -Name "renderer_package_script_execution" `
        -Label "environment validation_counters"
    Test-CommonPackageNonClaims -NonClaims (Get-JsonPropertyValue -JsonObject $Row -Name "non_claims")
}

function Test-GeneratedGamePackageRow {
    param([Parameter(Mandatory = $true)]$Row)

    Assert-StringProperty -JsonObject $Row -Name "validation_recipe" -Expected "generated-game-package" `
        -Label "generated_game"
    Assert-ExactJsonProperties -JsonObject $Row -Label "generated_game" -ExpectedNames @(
        "validation_recipe",
        "proof_rows",
        "validation_counters",
        "non_claims"
    )
    $proofRows = Get-JsonPropertyValue -JsonObject $Row -Name "proof_rows"
    $output = Get-JsonPropertyValue -JsonObject $proofRows -Name "generated_game_output"
    Assert-TrueProperty -JsonObject $output -Name "ready" -Label "generated_game generated_game_output"
    Assert-TrueProperty -JsonObject $output -Name "generated_game_package_written" `
        -Label "generated_game generated_game_output"
    Assert-IntegerAtLeast -JsonObject $output -Name "output_manifest_rows" -Minimum 1 `
        -Label "generated_game generated_game_output"
    Assert-LowerHexSha256 -JsonObject $output -Name "deterministic_hash_sha256" `
        -Label "generated_game generated_game_output"

    $manifestBinding = Get-JsonPropertyValue -JsonObject $proofRows -Name "manifest_binding"
    Assert-TrueProperty -JsonObject $manifestBinding -Name "ready" -Label "generated_game manifest_binding"
    Assert-TrueProperty -JsonObject $manifestBinding -Name "game_agent_manifest_row" `
        -Label "generated_game manifest_binding"
    Assert-StringProperty -JsonObject $manifestBinding -Name "validation_recipe_id" `
        -Expected "generated-game-package" -Label "generated_game manifest_binding"
    foreach ($name in @("generated_game_manifest_id", "package_manifest_row")) {
        $value = [string](Get-JsonPropertyValue -JsonObject $manifestBinding -Name $name)
        if ([string]::IsNullOrWhiteSpace($value)) {
            Write-Error "generated_game manifest_binding expected non-empty $name."
        }
    }

    $counters = Get-JsonPropertyValue -JsonObject $Row -Name "validation_counters"
    Assert-TrueProperty -JsonObject $counters -Name "renderer_generated_game_package_output_ready" `
        -Label "generated_game validation_counters"
    Assert-TrueProperty -JsonObject $counters -Name "renderer_generated_game_manifest_ready" `
        -Label "generated_game validation_counters"
    Assert-FalseProperty -JsonObject $counters -Name "renderer_package_arbitrary_script_execution" `
        -Label "generated_game validation_counters"
    Assert-FalseProperty -JsonObject $counters -Name "renderer_package_script_execution" `
        -Label "generated_game validation_counters"
    Test-CommonPackageNonClaims -NonClaims (Get-JsonPropertyValue -JsonObject $Row -Name "non_claims")
}

function Test-PackageHostEvidence {
    param([Parameter(Mandatory = $true)]$Evidence)

    Assert-ExactJsonProperties -JsonObject $Evidence -Label "package host evidence" -ExpectedNames @(
        "schema_version",
        "claim_id",
        "validation_recipe",
        "fixture_only",
        "source_id",
        "package_rows",
        "non_claims"
    )
    Assert-StringProperty -JsonObject $Evidence -Name "schema_version" `
        -Expected "GameEngine.RendererPackageCommercialQualityHostEvidence.v1" `
        -Label "package host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "claim_id" `
        -Expected "renderer-package-commercial-quality-artifacts-v1" `
        -Label "package host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "validation_recipe" `
        -Expected "renderer-package-commercial-quality-artifacts" `
        -Label "package host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "source_id" -Expected $expectedSourceId `
        -Label "package host evidence"

    $fixtureOnly = Get-JsonPropertyValue -JsonObject $Evidence -Name "fixture_only"
    if ($fixtureOnly -isnot [bool]) {
        Write-Error "package host evidence expected fixture_only to be boolean."
    }
    if ([bool]$fixtureOnly) {
        Write-Error "fixture_artifact_input_rejected: $PackageHostEvidenceRelative"
    }

    $packageRows = Get-JsonPropertyValue -JsonObject $Evidence -Name "package_rows"
    Assert-ExactJsonProperties -JsonObject $packageRows -Label "package_rows" -ExpectedNames @(
        "visible_3d",
        "runtime_ui",
        "environment",
        "generated_game"
    )
    Test-Visible3dPackageRow -Row (Get-JsonPropertyValue -JsonObject $packageRows -Name "visible_3d")
    Test-RuntimeUiPackageRow -Row (Get-JsonPropertyValue -JsonObject $packageRows -Name "runtime_ui")
    Test-EnvironmentPackageRow -Row (Get-JsonPropertyValue -JsonObject $packageRows -Name "environment")
    Test-GeneratedGamePackageRow -Row (Get-JsonPropertyValue -JsonObject $packageRows -Name "generated_game")

    $nonClaims = Get-JsonPropertyValue -JsonObject $Evidence -Name "non_claims"
    Assert-ExactJsonProperties -JsonObject $nonClaims -Label "package host non_claims" -ExpectedNames @(
        "arbitrary_script_execution",
        "package_script_execution",
        "native_handles_exposed",
        "external_engine_parity",
        "environment_ready",
        "renderer_commercial_readiness"
    )
    foreach ($requiredFalse in @(
            "arbitrary_script_execution",
            "package_script_execution",
            "native_handles_exposed",
            "external_engine_parity",
            "environment_ready",
            "renderer_commercial_readiness"
        )) {
        Assert-FalseProperty -JsonObject $nonClaims -Name $requiredFalse -Label "package host non_claims"
    }
}

function New-PackageArtifact {
    param(
        [Parameter(Mandatory = $true)]$Spec,
        [Parameter(Mandatory = $true)]$PackageRow
    )

    return [ordered]@{
        schema_version = $Spec.Schema
        artifact_id = $Spec.ArtifactId
        validation_recipe = $Spec.Recipe
        fixture_only = $false
        ready = $true
        proof_rows = Get-JsonPropertyValue -JsonObject $PackageRow -Name "proof_rows"
        validation_counters = Get-JsonPropertyValue -JsonObject $PackageRow -Name "validation_counters"
        non_claims = Get-JsonPropertyValue -JsonObject $PackageRow -Name "non_claims"
    }
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/commercial-readiness-evidence/."
}

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=renderer-package-commercial-quality-artifacts"
    Write-Output "renderer_package_commercial_quality_artifacts_collector_mode=Plan"
    Write-Output "renderer_package_commercial_quality_artifacts_output_root=$OutputRootRelative"
    Write-Output "renderer_package_commercial_quality_artifacts_writes_evidence=0"
    Write-Output "renderer_package_commercial_quality_artifacts_written=0"
    Write-Output "renderer_package_commercial_quality_fixture_artifact=0"
    Write-Output "renderer_visible_3d_package_ready=0"
    Write-Output "renderer_runtime_ui_package_ready=0"
    Write-Output "renderer_environment_package_ready=0"
    Write-Output "renderer_generated_game_package_ready=0"
    Write-Output "renderer_package_arbitrary_script_execution=0"
    Write-Output "renderer_package_script_execution=0"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    Write-Output "renderer_environment_ready=0"
    return
}

if ([string]::IsNullOrWhiteSpace($PackageHostEvidenceRelative)) {
    Write-Error "package_host_evidence_required"
}
if (-not (Test-SafeRepoRelativePath -RelativePath $PackageHostEvidenceRelative)) {
    Write-Error "unsafe_relative_path: PackageHostEvidenceRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedHostEvidencePath -RelativePath $PackageHostEvidenceRelative)) {
    Write-Error "PackageHostEvidenceRelative must be under artifacts/renderer/commercial-readiness-evidence/ or artifacts/renderer/package-commercial-quality-host-evidence/."
}

$hostEvidenceFull = Resolve-RepoRelativePath `
    -RelativePath $PackageHostEvidenceRelative `
    -Label "PackageHostEvidenceRelative"
$outputRootFull = Resolve-RepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
$hostEvidence = Read-JsonFile -Path $hostEvidenceFull -Label "PackageHostEvidenceRelative"
Test-PackageHostEvidence -Evidence $hostEvidence
$packageRows = Get-JsonPropertyValue -JsonObject $hostEvidence -Name "package_rows"
$willWrite = -not $NoWrite.IsPresent
$writtenCount = 0

if ($willWrite) {
    $null = New-Item -ItemType Directory -Path $outputRootFull -Force
}

Write-Output "validation_recipe=renderer-package-commercial-quality-artifacts"
Write-Output "renderer_package_commercial_quality_artifacts_collector_mode=Assemble"
Write-Output "renderer_package_commercial_quality_artifacts_output_root=$OutputRootRelative"

foreach ($spec in $artifactSpecs) {
    $artifactRelative = "$OutputRootRelative/$($spec.FileName)"
    $artifactFull = Resolve-RepoRelativePath -RelativePath $artifactRelative -Label "$($spec.Name) package artifact"
    $packageRow = Get-JsonPropertyValue -JsonObject $packageRows -Name $spec.Name
    $artifact = New-PackageArtifact -Spec $spec -PackageRow $packageRow
    if ($willWrite) {
        $artifact | ConvertTo-Json -Depth 16 |
            Set-Content -LiteralPath $artifactFull -Encoding utf8NoBOM
        $writtenCount += 1
    }

    $artifactHash = ""
    if (Test-Path -LiteralPath $artifactFull -PathType Leaf) {
        $artifactHash = (Get-FileHash -LiteralPath $artifactFull -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    Write-Output "renderer_package_commercial_quality_artifacts_$($spec.Name)_path=$artifactRelative"
    Write-Output "renderer_package_commercial_quality_artifacts_$($spec.Name)_hash=$artifactHash"
}

Write-Output "renderer_package_commercial_quality_artifacts_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_package_commercial_quality_artifacts_written=$writtenCount"
Write-Output "renderer_package_commercial_quality_fixture_artifact=0"
Write-Output "renderer_package_commercial_quality_source_id=$expectedSourceId"
Write-Output "renderer_visible_3d_package_ready=1"
Write-Output "renderer_runtime_ui_package_ready=1"
Write-Output "renderer_environment_package_ready=1"
Write-Output "renderer_generated_game_package_ready=1"
Write-Output "renderer_package_arbitrary_script_execution=0"
Write-Output "renderer_package_script_execution=0"
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"

Write-Information "renderer-package-commercial-quality-artifacts-collector: ok" -InformationAction Continue
