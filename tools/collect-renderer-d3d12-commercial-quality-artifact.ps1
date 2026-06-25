#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10A

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Assemble")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/d3d12-quality",

    [string]$D3d12HostEvidenceRelative = "",

    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$expectedSourceId = "Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25"
$artifactRelative = "$OutputRootRelative/d3d12-quality.json"

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
            "artifacts/renderer/d3d12-commercial-quality-host-evidence/",
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

function Test-D3d12HostEvidence {
    param([Parameter(Mandatory = $true)]$Evidence)

    Assert-ExactJsonProperties -JsonObject $Evidence -Label "D3D12 host evidence" -ExpectedNames @(
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
        -Expected "GameEngine.RendererD3d12CommercialQualityHostEvidence.v1" `
        -Label "D3D12 host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "claim_id" `
        -Expected "renderer-d3d12-commercial-quality-artifact-v1" `
        -Label "D3D12 host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "validation_recipe" `
        -Expected "renderer-d3d12-quality-evidence" `
        -Label "D3D12 host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "source_id" `
        -Expected $expectedSourceId `
        -Label "D3D12 host evidence"

    $fixtureOnly = Get-JsonPropertyValue -JsonObject $Evidence -Name "fixture_only"
    if ($fixtureOnly -isnot [bool]) {
        Write-Error "D3D12 host evidence expected fixture_only to be boolean."
    }
    if ([bool]$fixtureOnly) {
        Write-Error "fixture_artifact_input_rejected: $D3d12HostEvidenceRelative"
    }

    $proofRows = Get-JsonPropertyValue -JsonObject $Evidence -Name "proof_rows"
    Assert-ExactJsonProperties -JsonObject $proofRows -Label "proof_rows" -ExpectedNames @(
        "command_allocator_list_fence",
        "resource_barriers",
        "timestamp",
        "debug_validation",
        "residency",
        "package_visible_readback",
        "native_handles"
    )

    $commandAllocatorListFence = Get-JsonPropertyValue -JsonObject $proofRows -Name "command_allocator_list_fence"
    Assert-ExactJsonProperties -JsonObject $commandAllocatorListFence -Label "command_allocator_list_fence" `
        -ExpectedNames @(
            "ready",
            "command_allocator_reuse_fenced",
            "command_list_closed_before_execute",
            "fence_signal_wait_recorded",
            "fence_api_name"
        )
    foreach ($requiredTrue in @(
            "ready",
            "command_allocator_reuse_fenced",
            "command_list_closed_before_execute",
            "fence_signal_wait_recorded"
        )) {
        Assert-TrueProperty -JsonObject $commandAllocatorListFence -Name $requiredTrue `
            -Label "command_allocator_list_fence"
    }
    Assert-StringProperty -JsonObject $commandAllocatorListFence -Name "fence_api_name" `
        -Expected "ID3D12Fence" `
        -Label "command_allocator_list_fence"

    $resourceBarriers = Get-JsonPropertyValue -JsonObject $proofRows -Name "resource_barriers"
    Assert-ExactJsonProperties -JsonObject $resourceBarriers -Label "resource_barriers" -ExpectedNames @(
        "ready",
        "render_transition_explicit",
        "copy_transition_explicit",
        "unordered_access_barrier_explicit",
        "readback_transition_explicit",
        "resource_barrier_api_name"
    )
    foreach ($requiredTrue in @(
            "ready",
            "render_transition_explicit",
            "copy_transition_explicit",
            "unordered_access_barrier_explicit",
            "readback_transition_explicit"
        )) {
        Assert-TrueProperty -JsonObject $resourceBarriers -Name $requiredTrue -Label "resource_barriers"
    }
    Assert-StringProperty -JsonObject $resourceBarriers -Name "resource_barrier_api_name" `
        -Expected "D3D12_RESOURCE_BARRIER" `
        -Label "resource_barriers"

    $timestamp = Get-JsonPropertyValue -JsonObject $proofRows -Name "timestamp"
    Assert-ExactJsonProperties -JsonObject $timestamp -Label "timestamp" -ExpectedNames @(
        "ready",
        "query_type",
        "resolved_query_data",
        "queue_frequency_hz",
        "clock_calibration"
    )
    foreach ($requiredTrue in @("ready", "resolved_query_data", "clock_calibration")) {
        Assert-TrueProperty -JsonObject $timestamp -Name $requiredTrue -Label "timestamp"
    }
    Assert-StringProperty -JsonObject $timestamp -Name "query_type" `
        -Expected "D3D12_QUERY_TYPE_TIMESTAMP" `
        -Label "timestamp"
    Assert-IntegerAtLeast -JsonObject $timestamp -Name "queue_frequency_hz" -Minimum 1 -Label "timestamp"

    $debugValidation = Get-JsonPropertyValue -JsonObject $proofRows -Name "debug_validation"
    Assert-ExactJsonProperties -JsonObject $debugValidation -Label "debug_validation" -ExpectedNames @(
        "ready",
        "debug_layer_or_gpu_based_validation_clean",
        "debug_message_count",
        "gpu_based_validation_message_count"
    )
    foreach ($requiredTrue in @("ready", "debug_layer_or_gpu_based_validation_clean")) {
        Assert-TrueProperty -JsonObject $debugValidation -Name $requiredTrue -Label "debug_validation"
    }
    Assert-IntegerEquals -JsonObject $debugValidation -Name "debug_message_count" -Expected 0 `
        -Label "debug_validation"
    Assert-IntegerEquals -JsonObject $debugValidation -Name "gpu_based_validation_message_count" `
        -Expected 0 `
        -Label "debug_validation"

    $residency = Get-JsonPropertyValue -JsonObject $proofRows -Name "residency"
    Assert-ExactJsonProperties -JsonObject $residency -Label "residency" -ExpectedNames @(
        "ready",
        "video_memory_budget_queried",
        "make_resident_or_budget_recorded",
        "residency_api_name",
        "budget_api_name"
    )
    foreach ($requiredTrue in @("ready", "video_memory_budget_queried", "make_resident_or_budget_recorded")) {
        Assert-TrueProperty -JsonObject $residency -Name $requiredTrue -Label "residency"
    }
    Assert-StringProperty -JsonObject $residency -Name "residency_api_name" `
        -Expected "ID3D12Device3::EnqueueMakeResident" `
        -Label "residency"
    Assert-StringProperty -JsonObject $residency -Name "budget_api_name" `
        -Expected "IDXGIAdapter3::QueryVideoMemoryInfo" `
        -Label "residency"

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
        -Expected "renderer_d3d12_package_visible_readback" `
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
        "renderer_d3d12_command_allocator_fence_ready",
        "renderer_d3d12_resource_barrier_ready",
        "renderer_d3d12_timestamp_ready",
        "renderer_d3d12_debug_validation_ready",
        "renderer_d3d12_residency_ready",
        "renderer_d3d12_package_readback_ready"
    )
    foreach ($counter in @(
            "renderer_d3d12_command_allocator_fence_ready",
            "renderer_d3d12_resource_barrier_ready",
            "renderer_d3d12_timestamp_ready",
            "renderer_d3d12_debug_validation_ready",
            "renderer_d3d12_residency_ready",
            "renderer_d3d12_package_readback_ready"
        )) {
        Assert-ValidationCounter -Counters $validationCounters -Name $counter
    }

    $nonClaims = Get-JsonPropertyValue -JsonObject $Evidence -Name "non_claims"
    Assert-ExactJsonProperties -JsonObject $nonClaims -Label "non_claims" -ExpectedNames @(
        "vulkan_inferred",
        "metal_inferred",
        "broad_ui_parity",
        "environment_ready",
        "external_engine_parity",
        "native_handles_exposed"
    )
    foreach ($requiredFalse in @(
            "vulkan_inferred",
            "metal_inferred",
            "broad_ui_parity",
            "environment_ready",
            "external_engine_parity",
            "native_handles_exposed"
        )) {
        Assert-FalseProperty -JsonObject $nonClaims -Name $requiredFalse -Label "non_claims"
    }
}

function New-D3d12QualityArtifact {
    param([Parameter(Mandatory = $true)]$Evidence)

    $proofRows = Get-JsonPropertyValue -JsonObject $Evidence -Name "proof_rows"
    return [ordered]@{
        schema_version = "GameEngine.RendererCommercialQualityCloseout.v1"
        artifact_id = "d3d12-quality"
        validation_recipe = "renderer-d3d12-quality-evidence"
        fixture_only = $false
        ready = $true
        proof_rows = [ordered]@{
            command_allocator_list_fence = Get-JsonPropertyValue -JsonObject $proofRows `
                -Name "command_allocator_list_fence"
            resource_barriers = Get-JsonPropertyValue -JsonObject $proofRows -Name "resource_barriers"
            timestamp = Get-JsonPropertyValue -JsonObject $proofRows -Name "timestamp"
            debug_validation = Get-JsonPropertyValue -JsonObject $proofRows -Name "debug_validation"
            residency = Get-JsonPropertyValue -JsonObject $proofRows -Name "residency"
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
    Write-Output "validation_recipe=renderer-d3d12-commercial-quality-artifact"
    Write-Output "renderer_d3d12_commercial_quality_artifact_collector_mode=Plan"
    Write-Output "renderer_d3d12_commercial_quality_artifact_output_root=$OutputRootRelative"
    Write-Output "renderer_d3d12_commercial_quality_artifact_writes_evidence=0"
    Write-Output "renderer_d3d12_commercial_quality_artifact_written=0"
    Write-Output "renderer_d3d12_commercial_quality_fixture_artifact=0"
    Write-Output "renderer_d3d12_command_allocator_fence_ready=0"
    Write-Output "renderer_d3d12_resource_barrier_ready=0"
    Write-Output "renderer_d3d12_timestamp_ready=0"
    Write-Output "renderer_d3d12_debug_validation_ready=0"
    Write-Output "renderer_d3d12_residency_ready=0"
    Write-Output "renderer_d3d12_package_readback_ready=0"
    Write-Output "renderer_d3d12_native_handles_exposed=0"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    Write-Output "renderer_environment_ready=0"
    return
}

if ([string]::IsNullOrWhiteSpace($D3d12HostEvidenceRelative)) {
    Write-Error "d3d12_host_evidence_required"
}
if (-not (Test-SafeRepoRelativePath -RelativePath $D3d12HostEvidenceRelative)) {
    Write-Error "unsafe_relative_path: D3d12HostEvidenceRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedHostEvidencePath -RelativePath $D3d12HostEvidenceRelative)) {
    Write-Error "D3d12HostEvidenceRelative must be under artifacts/renderer/commercial-readiness-evidence/ or artifacts/renderer/d3d12-commercial-quality-host-evidence/."
}

$hostEvidenceFull = Resolve-RepoRelativePath `
    -RelativePath $D3d12HostEvidenceRelative `
    -Label "D3d12HostEvidenceRelative"
$artifactFull = Resolve-RepoRelativePath -RelativePath $artifactRelative -Label "d3d12-quality artifact"
$outputRootFull = Resolve-RepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
$hostEvidence = Read-JsonFile -Path $hostEvidenceFull -Label "D3d12HostEvidenceRelative"
Test-D3d12HostEvidence -Evidence $hostEvidence
$artifact = New-D3d12QualityArtifact -Evidence $hostEvidence
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

Write-Output "validation_recipe=renderer-d3d12-commercial-quality-artifact"
Write-Output "renderer_d3d12_commercial_quality_artifact_collector_mode=Assemble"
Write-Output "renderer_d3d12_commercial_quality_artifact_output_root=$OutputRootRelative"
Write-Output "renderer_d3d12_commercial_quality_artifact_path=$artifactRelative"
Write-Output "renderer_d3d12_commercial_quality_artifact_hash=$artifactHash"
Write-Output "renderer_d3d12_commercial_quality_artifact_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_d3d12_commercial_quality_artifact_written=$(ConvertTo-CounterBit ($willWrite -and (Test-Path -LiteralPath $artifactFull -PathType Leaf)))"
Write-Output "renderer_d3d12_commercial_quality_fixture_artifact=0"
Write-Output "renderer_d3d12_commercial_quality_source_id=$expectedSourceId"
Write-Output "renderer_d3d12_command_allocator_fence_ready=1"
Write-Output "renderer_d3d12_resource_barrier_ready=1"
Write-Output "renderer_d3d12_timestamp_ready=1"
Write-Output "renderer_d3d12_debug_validation_ready=1"
Write-Output "renderer_d3d12_residency_ready=1"
Write-Output "renderer_d3d12_package_readback_ready=1"
Write-Output "renderer_d3d12_native_handles_exposed=0"
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"

Write-Information "renderer-d3d12-commercial-quality-artifact-collector: ok" -InformationAction Continue
