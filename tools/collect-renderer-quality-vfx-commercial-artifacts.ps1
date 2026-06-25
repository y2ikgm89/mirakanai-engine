#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10D

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Assemble")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/quality-vfx-commercial",

    [string]$QualityVfxHostEvidenceRelative = "",

    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$expectedSourceId = "GameEngine-Renderer-Quality-Vfx-Profiling-2026-06-25"
$artifactSpecs = @(
    @{
        Name = "renderer_quality_matrix"
        ArtifactId = "renderer-quality-matrix"
        FileName = "renderer-quality-matrix.json"
        Recipe = "renderer-quality-matrix"
    },
    @{
        Name = "production_vfx_profiling"
        ArtifactId = "production-vfx-profiling"
        FileName = "production-vfx-profiling.json"
        Recipe = "renderer-production-vfx-profiling"
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
            "artifacts/renderer/quality-vfx-commercial-host-evidence/",
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

function Assert-IntegerEquals {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][long]$Expected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    try {
        $integerValue = [long]$value
    }
    catch {
        Write-Error "$Label expected integer $Name."
    }
    if ($integerValue -ne $Expected) {
        Write-Error "$Label expected $Name=$Expected."
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

function Test-CommonQualityNonClaims {
    param(
        [Parameter(Mandatory = $true)]$NonClaims,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($requiredFalse in @(
            "broad_renderer_quality",
            "renderer_commercial_readiness",
            "external_engine_parity",
            "native_handles_exposed"
        )) {
        Assert-FalseProperty -JsonObject $NonClaims -Name $requiredFalse -Label $Label
    }
}

function Test-RendererQualityMatrixRow {
    param([Parameter(Mandatory = $true)]$Row)

    Assert-StringProperty -JsonObject $Row -Name "validation_recipe" -Expected "renderer-quality-matrix" `
        -Label "renderer_quality_matrix"
    Assert-ExactJsonProperties -JsonObject $Row -Label "renderer_quality_matrix" -ExpectedNames @(
        "validation_recipe",
        "proof_rows",
        "validation_counters",
        "non_claims"
    )

    $proofRows = Get-JsonPropertyValue -JsonObject $Row -Name "proof_rows"
    Assert-ExactJsonProperties -JsonObject $proofRows -Label "renderer_quality_matrix proof_rows" `
        -ExpectedNames @(
            "matrix_status",
            "side_effect_policy",
            "replay",
            "diagnostics"
        )

    $matrixStatus = Get-JsonPropertyValue -JsonObject $proofRows -Name "matrix_status"
    Assert-TrueProperty -JsonObject $matrixStatus -Name "ready" -Label "renderer_quality_matrix matrix_status"
    Assert-StringProperty -JsonObject $matrixStatus -Name "renderer_quality_matrix_status" `
        -Expected "host_evidence_required" -Label "renderer_quality_matrix matrix_status"
    foreach ($requiredTrue in @(
            "d3d12_renderer_quality_ready",
            "vulkan_strict_renderer_quality_ready",
            "apple_metal_host_evidence_supplied"
        )) {
        Assert-TrueProperty -JsonObject $matrixStatus -Name $requiredTrue `
            -Label "renderer_quality_matrix matrix_status"
    }
    Assert-FalseProperty -JsonObject $matrixStatus -Name "general_renderer_quality_claim" `
        -Label "renderer_quality_matrix matrix_status"

    $sideEffectPolicy = Get-JsonPropertyValue -JsonObject $proofRows -Name "side_effect_policy"
    Assert-TrueProperty -JsonObject $sideEffectPolicy -Name "ready" `
        -Label "renderer_quality_matrix side_effect_policy"
    foreach ($zeroCounter in @(
            "gpu_command_side_effects",
            "native_capture_side_effects",
            "crash_upload_side_effects"
        )) {
        Assert-IntegerEquals -JsonObject $sideEffectPolicy -Name $zeroCounter -Expected 0 `
            -Label "renderer_quality_matrix side_effect_policy"
    }

    $replay = Get-JsonPropertyValue -JsonObject $proofRows -Name "replay"
    Assert-TrueProperty -JsonObject $replay -Name "ready" -Label "renderer_quality_matrix replay"
    Assert-LowerHexSha256 -JsonObject $replay -Name "deterministic_replay_hash_sha256" `
        -Label "renderer_quality_matrix replay"

    $diagnostics = Get-JsonPropertyValue -JsonObject $proofRows -Name "diagnostics"
    Assert-TrueProperty -JsonObject $diagnostics -Name "ready" -Label "renderer_quality_matrix diagnostics"
    Assert-IntegerEquals -JsonObject $diagnostics -Name "diagnostic_error_count" -Expected 0 `
        -Label "renderer_quality_matrix diagnostics"

    $counters = Get-JsonPropertyValue -JsonObject $Row -Name "validation_counters"
    Assert-TrueProperty -JsonObject $counters -Name "renderer_quality_matrix_ready" `
        -Label "renderer_quality_matrix validation_counters"
    Assert-FalseProperty -JsonObject $counters -Name "renderer_quality_matrix_general_renderer_quality_claim" `
        -Label "renderer_quality_matrix validation_counters"
    foreach ($zeroCounter in @(
            "renderer_quality_matrix_gpu_command_side_effects",
            "renderer_quality_matrix_native_capture_side_effects",
            "renderer_quality_matrix_crash_upload_side_effects"
        )) {
        Assert-IntegerEquals -JsonObject $counters -Name $zeroCounter -Expected 0 `
            -Label "renderer_quality_matrix validation_counters"
    }

    $nonClaims = Get-JsonPropertyValue -JsonObject $Row -Name "non_claims"
    Assert-ExactJsonProperties -JsonObject $nonClaims -Label "renderer_quality_matrix non_claims" `
        -ExpectedNames @(
            "broad_renderer_quality",
            "renderer_commercial_readiness",
            "external_engine_parity",
            "native_handles_exposed",
            "gpu_command_side_effects",
            "native_capture_side_effects",
            "crash_upload_side_effects"
        )
    Test-CommonQualityNonClaims -NonClaims $nonClaims -Label "renderer_quality_matrix non_claims"
    foreach ($requiredFalse in @(
            "gpu_command_side_effects",
            "native_capture_side_effects",
            "crash_upload_side_effects"
        )) {
        Assert-FalseProperty -JsonObject $nonClaims -Name $requiredFalse `
            -Label "renderer_quality_matrix non_claims"
    }
}

function Test-ProductionVfxProfilingRow {
    param([Parameter(Mandatory = $true)]$Row)

    Assert-StringProperty -JsonObject $Row -Name "validation_recipe" `
        -Expected "renderer-production-vfx-profiling" -Label "production_vfx_profiling"
    Assert-ExactJsonProperties -JsonObject $Row -Label "production_vfx_profiling" -ExpectedNames @(
        "validation_recipe",
        "proof_rows",
        "validation_counters",
        "non_claims"
    )

    $proofRows = Get-JsonPropertyValue -JsonObject $Row -Name "proof_rows"
    Assert-ExactJsonProperties -JsonObject $proofRows -Label "production_vfx_profiling proof_rows" `
        -ExpectedNames @(
            "vfx_profiling_review",
            "debug_policy",
            "memory_policy",
            "package_counters",
            "replay",
            "side_effect_policy"
        )

    $review = Get-JsonPropertyValue -JsonObject $proofRows -Name "vfx_profiling_review"
    foreach ($requiredTrue in @(
            "ready",
            "rendering_vfx_profiling_reviewed",
            "d3d12_renderer_quality_ready",
            "vulkan_strict_renderer_quality_ready",
            "apple_metal_host_evidence_supplied"
        )) {
        Assert-TrueProperty -JsonObject $review -Name $requiredTrue `
            -Label "production_vfx_profiling review"
    }

    $debugPolicy = Get-JsonPropertyValue -JsonObject $proofRows -Name "debug_policy"
    foreach ($requiredTrue in @(
            "ready",
            "debug_capture_policy_recorded",
            "debug_upload_policy_recorded",
            "crash_upload_policy_recorded"
        )) {
        Assert-TrueProperty -JsonObject $debugPolicy -Name $requiredTrue `
            -Label "production_vfx_profiling debug_policy"
    }

    $memoryPolicy = Get-JsonPropertyValue -JsonObject $proofRows -Name "memory_policy"
    foreach ($requiredTrue in @(
            "ready",
            "memory_residency_policy_recorded",
            "metal_memory_profiling_evidence_supplied"
        )) {
        Assert-TrueProperty -JsonObject $memoryPolicy -Name $requiredTrue `
            -Label "production_vfx_profiling memory_policy"
    }

    $packageCounters = Get-JsonPropertyValue -JsonObject $proofRows -Name "package_counters"
    foreach ($requiredTrue in @(
            "ready",
            "visible_3d_package_ready",
            "runtime_ui_package_ready",
            "environment_package_ready",
            "generated_game_package_ready"
        )) {
        Assert-TrueProperty -JsonObject $packageCounters -Name $requiredTrue `
            -Label "production_vfx_profiling package_counters"
    }

    $replay = Get-JsonPropertyValue -JsonObject $proofRows -Name "replay"
    Assert-TrueProperty -JsonObject $replay -Name "ready" -Label "production_vfx_profiling replay"
    Assert-LowerHexSha256 -JsonObject $replay -Name "deterministic_replay_hash_sha256" `
        -Label "production_vfx_profiling replay"

    $sideEffectPolicy = Get-JsonPropertyValue -JsonObject $proofRows -Name "side_effect_policy"
    Assert-TrueProperty -JsonObject $sideEffectPolicy -Name "ready" `
        -Label "production_vfx_profiling side_effect_policy"
    Assert-IntegerEquals -JsonObject $sideEffectPolicy -Name "native_capture_side_effects" -Expected 0 `
        -Label "production_vfx_profiling side_effect_policy"
    Assert-IntegerEquals -JsonObject $sideEffectPolicy -Name "crash_upload_side_effects" -Expected 0 `
        -Label "production_vfx_profiling side_effect_policy"
    Assert-FalseProperty -JsonObject $sideEffectPolicy -Name "retained_official_profiler_artifact_selected" `
        -Label "production_vfx_profiling side_effect_policy"

    $counters = Get-JsonPropertyValue -JsonObject $Row -Name "validation_counters"
    Assert-TrueProperty -JsonObject $counters -Name "renderer_production_vfx_profiling_ready" `
        -Label "production_vfx_profiling validation_counters"
    Assert-TrueProperty -JsonObject $counters -Name "rendering_vfx_profiling_reviewed" `
        -Label "production_vfx_profiling validation_counters"
    Assert-IntegerEquals -JsonObject $counters -Name "renderer_production_vfx_native_capture_side_effects" `
        -Expected 0 -Label "production_vfx_profiling validation_counters"
    Assert-IntegerEquals -JsonObject $counters -Name "renderer_production_vfx_crash_upload_side_effects" `
        -Expected 0 -Label "production_vfx_profiling validation_counters"

    $nonClaims = Get-JsonPropertyValue -JsonObject $Row -Name "non_claims"
    Assert-ExactJsonProperties -JsonObject $nonClaims -Label "production_vfx_profiling non_claims" `
        -ExpectedNames @(
            "broad_renderer_quality",
            "renderer_commercial_readiness",
            "external_engine_parity",
            "native_handles_exposed",
            "native_capture_side_effects",
            "crash_upload_side_effects",
            "retained_official_profiler_artifact_selected"
        )
    Test-CommonQualityNonClaims -NonClaims $nonClaims -Label "production_vfx_profiling non_claims"
    foreach ($requiredFalse in @(
            "native_capture_side_effects",
            "crash_upload_side_effects",
            "retained_official_profiler_artifact_selected"
        )) {
        Assert-FalseProperty -JsonObject $nonClaims -Name $requiredFalse `
            -Label "production_vfx_profiling non_claims"
    }
}

function Test-QualityVfxHostEvidence {
    param([Parameter(Mandatory = $true)]$Evidence)

    Assert-ExactJsonProperties -JsonObject $Evidence -Label "quality/VFX host evidence" -ExpectedNames @(
        "schema_version",
        "claim_id",
        "validation_recipe",
        "fixture_only",
        "source_id",
        "quality_rows",
        "non_claims"
    )
    Assert-StringProperty -JsonObject $Evidence -Name "schema_version" `
        -Expected "GameEngine.RendererQualityVfxCommercialHostEvidence.v1" `
        -Label "quality/VFX host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "claim_id" `
        -Expected "renderer-quality-vfx-commercial-artifacts-v1" `
        -Label "quality/VFX host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "validation_recipe" `
        -Expected "renderer-quality-vfx-commercial-artifacts" `
        -Label "quality/VFX host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "source_id" -Expected $expectedSourceId `
        -Label "quality/VFX host evidence"

    $fixtureOnly = Get-JsonPropertyValue -JsonObject $Evidence -Name "fixture_only"
    if ($fixtureOnly -isnot [bool]) {
        Write-Error "quality/VFX host evidence expected fixture_only to be boolean."
    }
    if ([bool]$fixtureOnly) {
        Write-Error "fixture_artifact_input_rejected: $QualityVfxHostEvidenceRelative"
    }

    $qualityRows = Get-JsonPropertyValue -JsonObject $Evidence -Name "quality_rows"
    Assert-ExactJsonProperties -JsonObject $qualityRows -Label "quality_rows" -ExpectedNames @(
        "renderer_quality_matrix",
        "production_vfx_profiling"
    )
    Test-RendererQualityMatrixRow -Row (Get-JsonPropertyValue -JsonObject $qualityRows -Name "renderer_quality_matrix")
    Test-ProductionVfxProfilingRow -Row (Get-JsonPropertyValue -JsonObject $qualityRows -Name "production_vfx_profiling")

    $nonClaims = Get-JsonPropertyValue -JsonObject $Evidence -Name "non_claims"
    Assert-ExactJsonProperties -JsonObject $nonClaims -Label "quality/VFX host non_claims" -ExpectedNames @(
        "broad_renderer_quality",
        "renderer_commercial_readiness",
        "external_engine_parity",
        "native_handles_exposed",
        "gpu_command_side_effects",
        "native_capture_side_effects",
        "crash_upload_side_effects"
    )
    Test-CommonQualityNonClaims -NonClaims $nonClaims -Label "quality/VFX host non_claims"
    foreach ($requiredFalse in @(
            "gpu_command_side_effects",
            "native_capture_side_effects",
            "crash_upload_side_effects"
        )) {
        Assert-FalseProperty -JsonObject $nonClaims -Name $requiredFalse -Label "quality/VFX host non_claims"
    }
}

function New-QualityVfxArtifact {
    param(
        [Parameter(Mandatory = $true)]$Spec,
        [Parameter(Mandatory = $true)]$QualityRow
    )

    return [ordered]@{
        schema_version = "GameEngine.RendererCommercialQualityCloseout.v1"
        artifact_id = $Spec.ArtifactId
        validation_recipe = $Spec.Recipe
        fixture_only = $false
        ready = $true
        proof_rows = Get-JsonPropertyValue -JsonObject $QualityRow -Name "proof_rows"
        validation_counters = Get-JsonPropertyValue -JsonObject $QualityRow -Name "validation_counters"
        non_claims = Get-JsonPropertyValue -JsonObject $QualityRow -Name "non_claims"
    }
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/commercial-readiness-evidence/."
}

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=renderer-quality-vfx-commercial-artifacts"
    Write-Output "renderer_quality_vfx_commercial_artifacts_collector_mode=Plan"
    Write-Output "renderer_quality_vfx_commercial_artifacts_output_root=$OutputRootRelative"
    Write-Output "renderer_quality_vfx_commercial_artifacts_writes_evidence=0"
    Write-Output "renderer_quality_vfx_commercial_artifacts_written=0"
    Write-Output "renderer_quality_vfx_commercial_fixture_artifact=0"
    Write-Output "renderer_quality_matrix_ready=0"
    Write-Output "renderer_quality_matrix_status=host_evidence_required"
    Write-Output "rendering_vfx_profiling_reviewed=0"
    Write-Output "renderer_production_vfx_profiling_ready=0"
    Write-Output "renderer_quality_matrix_gpu_command_side_effects=0"
    Write-Output "renderer_quality_matrix_native_capture_side_effects=0"
    Write-Output "renderer_quality_matrix_crash_upload_side_effects=0"
    Write-Output "renderer_production_vfx_native_capture_side_effects=0"
    Write-Output "renderer_production_vfx_crash_upload_side_effects=0"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    Write-Output "renderer_environment_ready=0"
    return
}

if ([string]::IsNullOrWhiteSpace($QualityVfxHostEvidenceRelative)) {
    Write-Error "quality_vfx_host_evidence_required"
}
if (-not (Test-SafeRepoRelativePath -RelativePath $QualityVfxHostEvidenceRelative)) {
    Write-Error "unsafe_relative_path: QualityVfxHostEvidenceRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedHostEvidencePath -RelativePath $QualityVfxHostEvidenceRelative)) {
    Write-Error "QualityVfxHostEvidenceRelative must be under artifacts/renderer/commercial-readiness-evidence/ or artifacts/renderer/quality-vfx-commercial-host-evidence/."
}

$hostEvidenceFull = Resolve-RepoRelativePath `
    -RelativePath $QualityVfxHostEvidenceRelative `
    -Label "QualityVfxHostEvidenceRelative"
$outputRootFull = Resolve-RepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
$hostEvidence = Read-JsonFile -Path $hostEvidenceFull -Label "QualityVfxHostEvidenceRelative"
Test-QualityVfxHostEvidence -Evidence $hostEvidence
$qualityRows = Get-JsonPropertyValue -JsonObject $hostEvidence -Name "quality_rows"
$willWrite = -not $NoWrite.IsPresent
$writtenCount = 0

if ($willWrite) {
    $null = New-Item -ItemType Directory -Path $outputRootFull -Force
}

Write-Output "validation_recipe=renderer-quality-vfx-commercial-artifacts"
Write-Output "renderer_quality_vfx_commercial_artifacts_collector_mode=Assemble"
Write-Output "renderer_quality_vfx_commercial_artifacts_output_root=$OutputRootRelative"

foreach ($spec in $artifactSpecs) {
    $artifactRelative = "$OutputRootRelative/$($spec.FileName)"
    $artifactFull = Resolve-RepoRelativePath -RelativePath $artifactRelative -Label "$($spec.Name) quality/VFX artifact"
    $qualityRow = Get-JsonPropertyValue -JsonObject $qualityRows -Name $spec.Name
    $artifact = New-QualityVfxArtifact -Spec $spec -QualityRow $qualityRow
    if ($willWrite) {
        $artifact | ConvertTo-Json -Depth 16 |
            Set-Content -LiteralPath $artifactFull -Encoding utf8NoBOM
        $writtenCount += 1
    }

    $artifactHash = ""
    if (Test-Path -LiteralPath $artifactFull -PathType Leaf) {
        $artifactHash = (Get-FileHash -LiteralPath $artifactFull -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    Write-Output "renderer_quality_vfx_commercial_artifacts_$($spec.Name)_path=$artifactRelative"
    Write-Output "renderer_quality_vfx_commercial_artifacts_$($spec.Name)_hash=$artifactHash"
}

Write-Output "renderer_quality_vfx_commercial_artifacts_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_quality_vfx_commercial_artifacts_written=$writtenCount"
Write-Output "renderer_quality_vfx_commercial_fixture_artifact=0"
Write-Output "renderer_quality_vfx_commercial_source_id=$expectedSourceId"
Write-Output "renderer_quality_matrix_ready=1"
Write-Output "renderer_quality_matrix_status=host_evidence_required"
Write-Output "rendering_vfx_profiling_reviewed=1"
Write-Output "renderer_production_vfx_profiling_ready=1"
Write-Output "renderer_quality_matrix_gpu_command_side_effects=0"
Write-Output "renderer_quality_matrix_native_capture_side_effects=0"
Write-Output "renderer_quality_matrix_crash_upload_side_effects=0"
Write-Output "renderer_production_vfx_native_capture_side_effects=0"
Write-Output "renderer_production_vfx_crash_upload_side_effects=0"
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"

Write-Information "renderer-quality-vfx-commercial-artifacts-collector: ok" -InformationAction Continue
