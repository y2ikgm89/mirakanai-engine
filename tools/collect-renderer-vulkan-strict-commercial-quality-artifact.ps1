#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10B

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Assemble")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/vulkan-strict-quality",

    [string]$VulkanStrictHostEvidenceRelative = "",

    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$expectedSourceId = "Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25"
$artifactRelative = "$OutputRootRelative/vulkan-strict-quality.json"

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
            "artifacts/renderer/vulkan-strict-commercial-quality-host-evidence/",
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

function Assert-StringContains {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $actual = [string](Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name)
    if (-not $actual.Contains($Needle, [System.StringComparison]::Ordinal)) {
        Write-Error "$Label expected $Name to contain $Needle."
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

function Assert-NumberGreaterThan {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][double]$Minimum,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    try {
        $numberValue = [double]$value
    }
    catch {
        Write-Error "$Label expected numeric $Name."
    }
    if (-not [double]::IsFinite($numberValue) -or $numberValue -le $Minimum) {
        Write-Error "$Label expected $Name > $Minimum."
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

function Assert-ValidationCounter {
    param(
        [Parameter(Mandatory = $true)]$Counters,
        [Parameter(Mandatory = $true)][string]$Name
    )

    Assert-TrueProperty -JsonObject $Counters -Name $Name -Label "validation_counters"
}

function Test-VulkanStrictHostEvidence {
    param([Parameter(Mandatory = $true)]$Evidence)

    Assert-ExactJsonProperties -JsonObject $Evidence -Label "strict Vulkan host evidence" -ExpectedNames @(
        "schema_version",
        "claim_id",
        "validation_recipe",
        "fixture_only",
        "source_id",
        "proof_rows",
        "validation_counters",
        "non_claims"
    )
    Assert-StringProperty -JsonObject $Evidence -Name "schema_version" `
        -Expected "GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1" `
        -Label "strict Vulkan host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "claim_id" `
        -Expected "renderer-vulkan-strict-commercial-quality-artifact-v1" `
        -Label "strict Vulkan host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "validation_recipe" `
        -Expected "renderer-vulkan-strict-quality-evidence" `
        -Label "strict Vulkan host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "source_id" `
        -Expected $expectedSourceId `
        -Label "strict Vulkan host evidence"

    $fixtureOnly = Get-JsonPropertyValue -JsonObject $Evidence -Name "fixture_only"
    if ($fixtureOnly -isnot [bool]) {
        Write-Error "strict Vulkan host evidence expected fixture_only to be boolean."
    }
    if ([bool]$fixtureOnly) {
        Write-Error "fixture_artifact_input_rejected: $VulkanStrictHostEvidenceRelative"
    }

    $proofRows = Get-JsonPropertyValue -JsonObject $Evidence -Name "proof_rows"
    Assert-ExactJsonProperties -JsonObject $proofRows -Label "proof_rows" -ExpectedNames @(
        "synchronization2",
        "validation_layer",
        "synchronization_validation",
        "memory_binding",
        "timestamp_query",
        "spirv_shader_validation",
        "package_visible_readback",
        "native_handles"
    )

    $synchronization2 = Get-JsonPropertyValue -JsonObject $proofRows -Name "synchronization2"
    Assert-ExactJsonProperties -JsonObject $synchronization2 -Label "synchronization2" -ExpectedNames @(
        "ready",
        "vk_cmd_pipeline_barrier2_recorded",
        "vk_dependency_info_recorded",
        "api_name",
        "structure_name"
    )
    foreach ($requiredTrue in @("ready", "vk_cmd_pipeline_barrier2_recorded", "vk_dependency_info_recorded")) {
        Assert-TrueProperty -JsonObject $synchronization2 -Name $requiredTrue -Label "synchronization2"
    }
    Assert-StringProperty -JsonObject $synchronization2 -Name "api_name" `
        -Expected "vkCmdPipelineBarrier2" `
        -Label "synchronization2"
    Assert-StringProperty -JsonObject $synchronization2 -Name "structure_name" `
        -Expected "VkDependencyInfo" `
        -Label "synchronization2"

    $validationLayer = Get-JsonPropertyValue -JsonObject $proofRows -Name "validation_layer"
    Assert-ExactJsonProperties -JsonObject $validationLayer -Label "validation_layer" -ExpectedNames @(
        "ready",
        "layer_name",
        "validation_log_clean",
        "validation_error_count"
    )
    foreach ($requiredTrue in @("ready", "validation_log_clean")) {
        Assert-TrueProperty -JsonObject $validationLayer -Name $requiredTrue -Label "validation_layer"
    }
    Assert-StringProperty -JsonObject $validationLayer -Name "layer_name" `
        -Expected "VK_LAYER_KHRONOS_validation" `
        -Label "validation_layer"
    Assert-IntegerEquals -JsonObject $validationLayer -Name "validation_error_count" -Expected 0 `
        -Label "validation_layer"

    $synchronizationValidation = Get-JsonPropertyValue -JsonObject $proofRows `
        -Name "synchronization_validation"
    Assert-ExactJsonProperties -JsonObject $synchronizationValidation -Label "synchronization_validation" `
        -ExpectedNames @(
            "ready",
            "sync_validation_enabled",
            "sync_validation_error_count"
        )
    foreach ($requiredTrue in @("ready", "sync_validation_enabled")) {
        Assert-TrueProperty -JsonObject $synchronizationValidation -Name $requiredTrue `
            -Label "synchronization_validation"
    }
    Assert-IntegerEquals -JsonObject $synchronizationValidation -Name "sync_validation_error_count" `
        -Expected 0 `
        -Label "synchronization_validation"

    $memoryBinding = Get-JsonPropertyValue -JsonObject $proofRows -Name "memory_binding"
    Assert-ExactJsonProperties -JsonObject $memoryBinding -Label "memory_binding" -ExpectedNames @(
        "ready",
        "buffer_memory_bound",
        "image_memory_bound",
        "vuid_constraints_checked",
        "vuid_reference"
    )
    foreach ($requiredTrue in @("ready", "buffer_memory_bound", "image_memory_bound", "vuid_constraints_checked")) {
        Assert-TrueProperty -JsonObject $memoryBinding -Name $requiredTrue -Label "memory_binding"
    }
    Assert-StringContains -JsonObject $memoryBinding -Name "vuid_reference" -Needle "VUID" `
        -Label "memory_binding"

    $timestampQuery = Get-JsonPropertyValue -JsonObject $proofRows -Name "timestamp_query"
    Assert-ExactJsonProperties -JsonObject $timestampQuery -Label "timestamp_query" -ExpectedNames @(
        "ready",
        "query_pool_timestamp",
        "timestamps_resolved",
        "timestamp_period_ns"
    )
    foreach ($requiredTrue in @("ready", "query_pool_timestamp", "timestamps_resolved")) {
        Assert-TrueProperty -JsonObject $timestampQuery -Name $requiredTrue -Label "timestamp_query"
    }
    Assert-NumberGreaterThan -JsonObject $timestampQuery -Name "timestamp_period_ns" -Minimum 0 `
        -Label "timestamp_query"

    $spirvShaderValidation = Get-JsonPropertyValue -JsonObject $proofRows -Name "spirv_shader_validation"
    Assert-ExactJsonProperties -JsonObject $spirvShaderValidation -Label "spirv_shader_validation" `
        -ExpectedNames @(
            "ready",
            "spirv_val_ready",
            "shader_modules_validated",
            "validation_error_count"
        )
    foreach ($requiredTrue in @("ready", "spirv_val_ready", "shader_modules_validated")) {
        Assert-TrueProperty -JsonObject $spirvShaderValidation -Name $requiredTrue `
            -Label "spirv_shader_validation"
    }
    Assert-IntegerEquals -JsonObject $spirvShaderValidation -Name "validation_error_count" `
        -Expected 0 `
        -Label "spirv_shader_validation"

    $packageVisibleReadback = Get-JsonPropertyValue -JsonObject $proofRows -Name "package_visible_readback"
    Assert-ExactJsonProperties -JsonObject $packageVisibleReadback -Label "package_visible_readback" `
        -ExpectedNames @(
            "ready",
            "deterministic_hash_sha256",
            "readback_counter_rows",
            "package_counter_id"
        )
    Assert-TrueProperty -JsonObject $packageVisibleReadback -Name "ready" -Label "package_visible_readback"
    Assert-LowerHexSha256 -JsonObject $packageVisibleReadback -Name "deterministic_hash_sha256" `
        -Label "package_visible_readback"
    Assert-IntegerAtLeast -JsonObject $packageVisibleReadback -Name "readback_counter_rows" -Minimum 1 `
        -Label "package_visible_readback"
    Assert-StringProperty -JsonObject $packageVisibleReadback -Name "package_counter_id" `
        -Expected "renderer_vulkan_package_visible_readback" `
        -Label "package_visible_readback"

    $nativeHandles = Get-JsonPropertyValue -JsonObject $proofRows -Name "native_handles"
    Assert-ExactJsonProperties -JsonObject $nativeHandles -Label "native_handles" -ExpectedNames @(
        "ready",
        "native_handles_exposed"
    )
    Assert-TrueProperty -JsonObject $nativeHandles -Name "ready" -Label "native_handles"
    Assert-FalseProperty -JsonObject $nativeHandles -Name "native_handles_exposed" -Label "native_handles"

    $validationCounters = Get-JsonPropertyValue -JsonObject $Evidence -Name "validation_counters"
    Assert-ExactJsonProperties -JsonObject $validationCounters -Label "validation_counters" -ExpectedNames @(
        "renderer_vulkan_synchronization2_ready",
        "renderer_vulkan_validation_layer_ready",
        "renderer_vulkan_sync_validation_ready",
        "renderer_vulkan_memory_binding_ready",
        "renderer_vulkan_timestamp_ready",
        "renderer_vulkan_shader_validation_ready",
        "renderer_vulkan_package_readback_ready"
    )
    foreach ($counter in @(
            "renderer_vulkan_synchronization2_ready",
            "renderer_vulkan_validation_layer_ready",
            "renderer_vulkan_sync_validation_ready",
            "renderer_vulkan_memory_binding_ready",
            "renderer_vulkan_timestamp_ready",
            "renderer_vulkan_shader_validation_ready",
            "renderer_vulkan_package_readback_ready"
        )) {
        Assert-ValidationCounter -Counters $validationCounters -Name $counter
    }

    $nonClaims = Get-JsonPropertyValue -JsonObject $Evidence -Name "non_claims"
    Assert-ExactJsonProperties -JsonObject $nonClaims -Label "non_claims" -ExpectedNames @(
        "d3d12_inferred",
        "metal_inferred",
        "debugging_only_full_pipeline_barrier",
        "environment_ready",
        "external_engine_parity",
        "native_handles_exposed"
    )
    foreach ($requiredFalse in @(
            "d3d12_inferred",
            "metal_inferred",
            "debugging_only_full_pipeline_barrier",
            "environment_ready",
            "external_engine_parity",
            "native_handles_exposed"
        )) {
        Assert-FalseProperty -JsonObject $nonClaims -Name $requiredFalse -Label "non_claims"
    }
}

function New-VulkanStrictQualityArtifact {
    param([Parameter(Mandatory = $true)]$Evidence)

    $proofRows = Get-JsonPropertyValue -JsonObject $Evidence -Name "proof_rows"
    return [ordered]@{
        schema_version = "GameEngine.RendererCommercialQualityCloseout.v1"
        artifact_id = "vulkan-strict-quality"
        validation_recipe = "renderer-vulkan-strict-quality-evidence"
        fixture_only = $false
        ready = $true
        proof_rows = [ordered]@{
            synchronization2 = Get-JsonPropertyValue -JsonObject $proofRows -Name "synchronization2"
            validation_layer = Get-JsonPropertyValue -JsonObject $proofRows -Name "validation_layer"
            synchronization_validation = Get-JsonPropertyValue -JsonObject $proofRows `
                -Name "synchronization_validation"
            memory_binding = Get-JsonPropertyValue -JsonObject $proofRows -Name "memory_binding"
            timestamp_query = Get-JsonPropertyValue -JsonObject $proofRows -Name "timestamp_query"
            spirv_shader_validation = Get-JsonPropertyValue -JsonObject $proofRows `
                -Name "spirv_shader_validation"
            package_visible_readback = Get-JsonPropertyValue -JsonObject $proofRows `
                -Name "package_visible_readback"
            native_handles = Get-JsonPropertyValue -JsonObject $proofRows -Name "native_handles"
        }
        validation_counters = Get-JsonPropertyValue -JsonObject $Evidence -Name "validation_counters"
        non_claims = Get-JsonPropertyValue -JsonObject $Evidence -Name "non_claims"
    }
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/commercial-readiness-evidence/."
}

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=renderer-vulkan-strict-commercial-quality-artifact"
    Write-Output "renderer_vulkan_strict_commercial_quality_artifact_collector_mode=Plan"
    Write-Output "renderer_vulkan_strict_commercial_quality_artifact_output_root=$OutputRootRelative"
    Write-Output "renderer_vulkan_strict_commercial_quality_artifact_writes_evidence=0"
    Write-Output "renderer_vulkan_strict_commercial_quality_artifact_written=0"
    Write-Output "renderer_vulkan_strict_commercial_quality_fixture_artifact=0"
    Write-Output "renderer_vulkan_synchronization2_ready=0"
    Write-Output "renderer_vulkan_validation_layer_ready=0"
    Write-Output "renderer_vulkan_sync_validation_ready=0"
    Write-Output "renderer_vulkan_memory_binding_ready=0"
    Write-Output "renderer_vulkan_timestamp_ready=0"
    Write-Output "renderer_vulkan_shader_validation_ready=0"
    Write-Output "renderer_vulkan_package_readback_ready=0"
    Write-Output "renderer_vulkan_native_handles_exposed=0"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    Write-Output "renderer_environment_ready=0"
    return
}

if ([string]::IsNullOrWhiteSpace($VulkanStrictHostEvidenceRelative)) {
    Write-Error "vulkan_strict_host_evidence_required"
}
if (-not (Test-SafeRepoRelativePath -RelativePath $VulkanStrictHostEvidenceRelative)) {
    Write-Error "unsafe_relative_path: VulkanStrictHostEvidenceRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedHostEvidencePath -RelativePath $VulkanStrictHostEvidenceRelative)) {
    Write-Error "VulkanStrictHostEvidenceRelative must be under artifacts/renderer/commercial-readiness-evidence/ or artifacts/renderer/vulkan-strict-commercial-quality-host-evidence/."
}

$hostEvidenceFull = Resolve-RepoRelativePath `
    -RelativePath $VulkanStrictHostEvidenceRelative `
    -Label "VulkanStrictHostEvidenceRelative"
$artifactFull = Resolve-RepoRelativePath -RelativePath $artifactRelative -Label "vulkan-strict-quality artifact"
$outputRootFull = Resolve-RepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
$hostEvidence = Read-JsonFile -Path $hostEvidenceFull -Label "VulkanStrictHostEvidenceRelative"
Test-VulkanStrictHostEvidence -Evidence $hostEvidence
$artifact = New-VulkanStrictQualityArtifact -Evidence $hostEvidence
$willWrite = -not $NoWrite.IsPresent

if ($willWrite) {
    $null = New-Item -ItemType Directory -Path $outputRootFull -Force
    $artifact | ConvertTo-Json -Depth 16 |
        Set-Content -LiteralPath $artifactFull -Encoding utf8NoBOM
}

$artifactHash = ""
if (Test-Path -LiteralPath $artifactFull -PathType Leaf) {
    $artifactHash = (Get-FileHash -LiteralPath $artifactFull -Algorithm SHA256).Hash.ToLowerInvariant()
}

Write-Output "validation_recipe=renderer-vulkan-strict-commercial-quality-artifact"
Write-Output "renderer_vulkan_strict_commercial_quality_artifact_collector_mode=Assemble"
Write-Output "renderer_vulkan_strict_commercial_quality_artifact_output_root=$OutputRootRelative"
Write-Output "renderer_vulkan_strict_commercial_quality_artifact_path=$artifactRelative"
Write-Output "renderer_vulkan_strict_commercial_quality_artifact_hash=$artifactHash"
Write-Output "renderer_vulkan_strict_commercial_quality_artifact_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_vulkan_strict_commercial_quality_artifact_written=$(ConvertTo-CounterBit ($willWrite -and (Test-Path -LiteralPath $artifactFull -PathType Leaf)))"
Write-Output "renderer_vulkan_strict_commercial_quality_fixture_artifact=0"
Write-Output "renderer_vulkan_strict_commercial_quality_source_id=$expectedSourceId"
Write-Output "renderer_vulkan_synchronization2_ready=1"
Write-Output "renderer_vulkan_validation_layer_ready=1"
Write-Output "renderer_vulkan_sync_validation_ready=1"
Write-Output "renderer_vulkan_memory_binding_ready=1"
Write-Output "renderer_vulkan_timestamp_ready=1"
Write-Output "renderer_vulkan_shader_validation_ready=1"
Write-Output "renderer_vulkan_package_readback_ready=1"
Write-Output "renderer_vulkan_native_handles_exposed=0"
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"

Write-Information "renderer-vulkan-strict-commercial-quality-artifact-collector: ok" -InformationAction Continue
