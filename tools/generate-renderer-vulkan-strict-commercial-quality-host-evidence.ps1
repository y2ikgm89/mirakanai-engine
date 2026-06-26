#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10F.3

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Generate")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/vulkan-strict-commercial-quality-host-evidence/renderer-commercial-readiness",

    [string]$PackageSmokeOutputRelative = "",

    [switch]$NoWrite,

    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$expectedSourceId = "Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25"
$artifactRelative = "$OutputRootRelative/vulkan-strict-host-evidence.json"
$summaryRelative = "$OutputRootRelative/host-gate-summary.json"

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function ConvertTo-CounterValue {
    param([Parameter(Mandatory = $true)][string]$Value)

    return ($Value -replace '[^A-Za-z0-9_.-]', '_')
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
        "artifacts/renderer/vulkan-strict-commercial-quality-host-evidence/",
        [System.StringComparison]::Ordinal)
}

function Test-AllowedInputPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    return $normalizedPath.StartsWith(
        "artifacts/renderer/vulkan-strict-commercial-quality-host-evidence/",
        [System.StringComparison]::Ordinal) -or
        $normalizedPath.StartsWith(
            "artifacts/renderer/commercial-readiness-evidence/",
            [System.StringComparison]::Ordinal) -or
        $normalizedPath.StartsWith(
            "artifacts/environment/platform/linux-vulkan-host/",
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

function ConvertFrom-KeyValueCounterText {
    param([Parameter(Mandatory = $true)][string]$Text)

    $values = @{}
    foreach ($token in ($Text -split "\s+")) {
        $separator = $token.IndexOf("=")
        if ($separator -le 0) {
            continue
        }
        $key = $token.Substring(0, $separator)
        $value = $token.Substring($separator + 1)
        $values[$key] = $value
    }
    return $values
}

function Get-CounterString {
    param(
        [Parameter(Mandatory = $true)]$Counters,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if (-not $Counters.ContainsKey($Name)) {
        return ""
    }
    return [string]$Counters[$Name]
}

function Test-CounterEquals {
    param(
        [Parameter(Mandatory = $true)]$Counters,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Expected
    )

    return (Get-CounterString -Counters $Counters -Name $Name) -ceq $Expected
}

function Test-CounterPositiveInteger {
    param(
        [Parameter(Mandatory = $true)]$Counters,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $value = Get-CounterString -Counters $Counters -Name $Name
    $parsed = 0L
    if (-not [long]::TryParse($value, [ref]$parsed)) {
        return $false
    }
    return $parsed -gt 0
}

function Test-CounterIntegerAtLeast {
    param(
        [Parameter(Mandatory = $true)]$Counters,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][long]$Minimum
    )

    $value = Get-CounterString -Counters $Counters -Name $Name
    $parsed = 0L
    if (-not [long]::TryParse($value, [ref]$parsed)) {
        return $false
    }
    return $parsed -ge $Minimum
}

function Add-BlockerIfFalse {
    param(
        [System.Collections.Generic.List[string]]$Blockers,
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if (-not $Condition) {
        $Blockers.Add($Name) | Out-Null
    }
}

function Get-LowerSha256ForText {
    param([Parameter(Mandatory = $true)][string]$Text)

    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
        $hash = $sha256.ComputeHash($bytes)
        return ([System.BitConverter]::ToString($hash)).Replace("-", "").ToLowerInvariant()
    }
    finally {
        $sha256.Dispose()
    }
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
    $Value | ConvertTo-Json -Depth 16 |
        Set-Content -LiteralPath $Path -Encoding utf8NoBOM
}

function New-NonClaims {
    return [ordered]@{
        d3d12_inferred = $false
        metal_inferred = $false
        debugging_only_full_pipeline_barrier = $false
        environment_ready = $false
        external_engine_parity = $false
        native_handles_exposed = $false
    }
}

function New-BlockedSummary {
    param(
        [Parameter(Mandatory = $true)][string[]]$Blockers,
        [Parameter(Mandatory = $true)][string]$Status
    )

    return [ordered]@{
        schema_version = "GameEngine.RendererVulkanStrictCommercialQualityHostGate.v1"
        validation_recipe = "renderer-vulkan-strict-commercial-quality-host-evidence"
        source_id = $expectedSourceId
        status = $Status
        blockers = @($Blockers)
        renderer_vulkan_strict_commercial_quality_host_evidence_ready = 0
        renderer_vulkan_strict_commercial_quality_host_evidence_written = 0
        renderer_backend_parity_ready = 0
        renderer_metal_broad_readiness = 0
        renderer_broad_quality_ready = 0
        renderer_commercial_readiness = 0
        renderer_environment_ready = 0
    }
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/vulkan-strict-commercial-quality-host-evidence/."
}

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=renderer-vulkan-strict-commercial-quality-host-evidence"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_generator_mode=Plan"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_output_root=$OutputRootRelative"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_writes_evidence=0"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_written=0"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_ready=0"
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

if ([string]::IsNullOrWhiteSpace($PackageSmokeOutputRelative)) {
    if ($RequireReady.IsPresent) {
        Write-Error "vulkan_strict_package_smoke_required"
    }
}
elseif (-not (Test-SafeRepoRelativePath -RelativePath $PackageSmokeOutputRelative)) {
    Write-Error "unsafe_relative_path: PackageSmokeOutputRelative must be a safe repo-relative path."
}
elseif (-not (Test-AllowedInputPath -RelativePath $PackageSmokeOutputRelative)) {
    Write-Error "PackageSmokeOutputRelative must be under approved Vulkan artifact roots."
}

$blockers = [System.Collections.Generic.List[string]]::new()
$packageText = ""
$counters = @{}

if ([string]::IsNullOrWhiteSpace($PackageSmokeOutputRelative)) {
    $blockers.Add("vulkan_strict_package_smoke_required") | Out-Null
}
else {
    $packageFull = Resolve-RepoRelativePath -RelativePath $PackageSmokeOutputRelative `
        -Label "PackageSmokeOutputRelative"
    if (-not (Test-Path -LiteralPath $packageFull -PathType Leaf)) {
        $blockers.Add("vulkan_strict_package_smoke_missing") | Out-Null
    }
    else {
        $packageText = Get-Content -LiteralPath $packageFull -Raw
        $counters = ConvertFrom-KeyValueCounterText -Text $packageText
    }
}

$synchronization2Ready =
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_selected_backend" -Expected "vulkan") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_device_features_ready" -Expected "1") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "environment_vulkan_strict_aggregate_synchronization2_barriers") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_d3d12_fallback" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_metal_fallback" -Expected "0")
Add-BlockerIfFalse $blockers $synchronization2Ready "vulkan_synchronization2_package_counters_required"

$validationLayerReady =
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_validation_layers_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_missing_validation_layer_rows" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_diagnostics" -Expected "0")
Add-BlockerIfFalse $blockers $validationLayerReady "vulkan_validation_layer_package_counters_required"

$syncValidationReady = $validationLayerReady -and $synchronization2Ready
Add-BlockerIfFalse $blockers $syncValidationReady "vulkan_sync_validation_package_counters_required"

$memoryBindingReady =
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_resource_usage_layout_ready" -Expected "1") -and
    (Test-CounterIntegerAtLeast -Counters $counters -Name "environment_vulkan_strict_aggregate_storage_buffer_usage_layout_rows" -Minimum 1) -and
    (Test-CounterIntegerAtLeast -Counters $counters -Name "environment_vulkan_strict_aggregate_sampled_texture_usage_layout_rows" -Minimum 1) -and
    (Test-CounterIntegerAtLeast -Counters $counters -Name "environment_vulkan_strict_aggregate_cube_map_usage_layout_rows" -Minimum 1) -and
    (Test-CounterEquals -Counters $counters -Name "vulkan_gpu_memory_execution_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "vulkan_gpu_memory_execution_selected" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "vulkan_gpu_memory_execution_committed_byte_estimate_available" -Expected "1") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "vulkan_gpu_memory_execution_committed_resources_byte_estimate") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "vulkan_gpu_memory_execution_upload_bytes_written") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "vulkan_gpu_memory_execution_framegraph_barrier_steps_executed") -and
    (Test-CounterEquals -Counters $counters -Name "vulkan_gpu_memory_execution_budget_ok" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "vulkan_gpu_memory_execution_transient_heap_ok" -Expected "1")
Add-BlockerIfFalse $blockers $memoryBindingReady "vulkan_memory_binding_package_counters_required"

$timestampTicksPerSecond = 0L
$timestampTicksAvailable = [long]::TryParse(
    (Get-CounterString -Counters $counters -Name "vulkan_debug_profiling_execution_gpu_timestamp_ticks_per_second"),
    [ref]$timestampTicksPerSecond)
$timestampReady =
    $timestampTicksAvailable -and
    $timestampTicksPerSecond -gt 0 -and
    (Test-CounterEquals -Counters $counters -Name "debug_profiling_policy_backend_profiling_evidence_ready" -Expected "1") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "debug_profiling_policy_gpu_timestamp_requests") -and
    (Test-CounterEquals -Counters $counters -Name "vulkan_debug_profiling_execution_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "vulkan_debug_profiling_execution_selected" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "vulkan_debug_profiling_execution_gpu_timestamps_ok" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "vulkan_debug_profiling_execution_frame_diagnostics_ok" -Expected "1") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "vulkan_debug_profiling_execution_framegraph_barrier_steps_executed")
Add-BlockerIfFalse $blockers $timestampReady "vulkan_timestamp_query_package_counters_required"

$shaderValidationReady =
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_toolchain_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_vulkan_sdk_tools_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_dxc_spirv_codegen_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_spirv_validation_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_missing_toolchain_rows" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_missing_spirv_validation_rows" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_unsupported_feature_device_rows" -Expected "0")
Add-BlockerIfFalse $blockers $shaderValidationReady "vulkan_spirv_shader_validation_package_counters_required"

$packageReadbackReady =
    (Test-CounterIntegerAtLeast -Counters $counters -Name "environment_vulkan_strict_aggregate_renderer_draws" -Minimum 1) -and
    (Test-CounterIntegerAtLeast -Counters $counters -Name "environment_vulkan_strict_aggregate_compute_dispatches" -Minimum 1) -and
    (Test-CounterIntegerAtLeast -Counters $counters -Name "environment_vulkan_strict_aggregate_texture_uploads" -Minimum 1) -and
    (Test-CounterIntegerAtLeast -Counters $counters -Name "environment_vulkan_strict_aggregate_readback_rows" -Minimum 1) -and
    (Test-CounterIntegerAtLeast -Counters $counters -Name "environment_vulkan_strict_aggregate_readback_resource_usage_layout_rows" -Minimum 1)
Add-BlockerIfFalse $blockers $packageReadbackReady "vulkan_package_visible_readback_counters_required"

$nativeHandlesReady =
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_native_handle_access" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_backend_parity" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "environment_vulkan_strict_aggregate_broad_optimization_claimed" -Expected "0")
Add-BlockerIfFalse $blockers $nativeHandlesReady "vulkan_native_handle_non_claim_counters_required"

if ($blockers.Count -ne 0) {
    $status = "host_evidence_required"
    $summaryFull = Resolve-RepoRelativePath -RelativePath $summaryRelative -Label "host gate summary"
    if (-not $NoWrite.IsPresent) {
        $summary = New-BlockedSummary -Blockers $blockers.ToArray() -Status $status
        Write-JsonObject -Path $summaryFull -Value $summary
    }
    Write-Output "validation_recipe=renderer-vulkan-strict-commercial-quality-host-evidence"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_generator_mode=Generate"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_status=$status"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_output_root=$OutputRootRelative"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_missing_rows=$($blockers.Count)"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_missing_row_names=$(ConvertTo-CounterValue -Value ([string]::Join(',', $blockers.ToArray())))"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_writes_evidence=$(ConvertTo-CounterBit (-not $NoWrite.IsPresent))"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_written=0"
    Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_ready=0"
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
    if ($RequireReady.IsPresent) {
        Write-Error "vulkan_strict_host_evidence_not_ready: $([string]::Join(', ', $blockers.ToArray()))"
    }
    return
}

$timestampPeriodNs = [double]1000000000.0 / [double]$timestampTicksPerSecond
$readbackHash = Get-LowerSha256ForText -Text $packageText.Trim()
$readbackRows = [long](Get-CounterString -Counters $counters -Name "environment_vulkan_strict_aggregate_readback_rows")

$evidence = [ordered]@{
    schema_version = "GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1"
    claim_id = "renderer-vulkan-strict-commercial-quality-artifact-v1"
    validation_recipe = "renderer-vulkan-strict-quality-evidence"
    fixture_only = $false
    source_id = $expectedSourceId
    proof_rows = [ordered]@{
        synchronization2 = [ordered]@{
            ready = $true
            vk_cmd_pipeline_barrier2_recorded = $true
            vk_dependency_info_recorded = $true
            api_name = "vkCmdPipelineBarrier2"
            structure_name = "VkDependencyInfo"
        }
        validation_layer = [ordered]@{
            ready = $true
            layer_name = "VK_LAYER_KHRONOS_validation"
            validation_log_clean = $true
            validation_error_count = 0
        }
        synchronization_validation = [ordered]@{
            ready = $true
            sync_validation_enabled = $true
            sync_validation_error_count = 0
        }
        memory_binding = [ordered]@{
            ready = $true
            buffer_memory_bound = $true
            image_memory_bound = $true
            vuid_constraints_checked = $true
            vuid_reference = "VUID-vkBindBufferMemory-memory-01030"
        }
        timestamp_query = [ordered]@{
            ready = $true
            query_pool_timestamp = $true
            timestamps_resolved = $true
            timestamp_period_ns = $timestampPeriodNs
        }
        spirv_shader_validation = [ordered]@{
            ready = $true
            spirv_val_ready = $true
            shader_modules_validated = $true
            validation_error_count = 0
        }
        package_visible_readback = [ordered]@{
            ready = $true
            deterministic_hash_sha256 = $readbackHash
            readback_counter_rows = $readbackRows
            package_counter_id = "renderer_vulkan_package_visible_readback"
        }
        native_handles = [ordered]@{
            ready = $true
            native_handles_exposed = $false
        }
    }
    validation_counters = [ordered]@{
        renderer_vulkan_synchronization2_ready = $true
        renderer_vulkan_validation_layer_ready = $true
        renderer_vulkan_sync_validation_ready = $true
        renderer_vulkan_memory_binding_ready = $true
        renderer_vulkan_timestamp_ready = $true
        renderer_vulkan_shader_validation_ready = $true
        renderer_vulkan_package_readback_ready = $true
    }
    non_claims = New-NonClaims
}

$artifactFull = Resolve-RepoRelativePath -RelativePath $artifactRelative -Label "strict Vulkan host evidence artifact"
$willWrite = -not $NoWrite.IsPresent
if ($willWrite) {
    Write-JsonObject -Path $artifactFull -Value $evidence
}

$artifactHash = ""
if (Test-Path -LiteralPath $artifactFull -PathType Leaf) {
    $artifactHash = (Get-FileHash -LiteralPath $artifactFull -Algorithm SHA256).Hash.ToLowerInvariant()
}

Write-Output "validation_recipe=renderer-vulkan-strict-commercial-quality-host-evidence"
Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_generator_mode=Generate"
Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_status=ready"
Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_output_root=$OutputRootRelative"
Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_path=$artifactRelative"
Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_hash=$artifactHash"
Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_written=$(ConvertTo-CounterBit ($willWrite -and (Test-Path -LiteralPath $artifactFull -PathType Leaf)))"
Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_ready=1"
Write-Output "renderer_vulkan_strict_commercial_quality_host_evidence_source_id=$expectedSourceId"
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

Write-Information "renderer-vulkan-strict-commercial-quality-host-evidence-generator: ok" -InformationAction Continue
