#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10F.1

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Generate")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/d3d12-commercial-quality-host-evidence/renderer-commercial-readiness",

    [string]$PackageSmokeOutputRelative = "",

    [string]$SupplementalHostEvidenceRelative = "",

    [switch]$NoWrite,

    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$expectedSourceId = "Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25"
$artifactRelative = "$OutputRootRelative/d3d12-host-evidence.json"
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
        "artifacts/renderer/d3d12-commercial-quality-host-evidence/",
        [System.StringComparison]::Ordinal)
}

function Test-AllowedInputPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    return $normalizedPath.StartsWith(
        "artifacts/renderer/d3d12-commercial-quality-host-evidence/",
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

function Test-JsonBoolTrue {
    param(
        [AllowNull()]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    return ($value -is [bool]) -and [bool]$value
}

function Test-JsonBoolFalse {
    param(
        [AllowNull()]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    return ($value -is [bool]) -and (-not [bool]$value)
}

function Test-JsonIntegerEquals {
    param(
        [AllowNull()]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][long]$Expected
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    try {
        return ([long]$value) -eq $Expected
    }
    catch {
        return $false
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
        vulkan_inferred = $false
        metal_inferred = $false
        broad_ui_parity = $false
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
        schema_version = "GameEngine.RendererD3d12CommercialQualityHostGate.v1"
        validation_recipe = "renderer-d3d12-commercial-quality-host-evidence"
        source_id = $expectedSourceId
        status = $Status
        blockers = @($Blockers)
        renderer_d3d12_commercial_quality_host_evidence_ready = 0
        renderer_d3d12_commercial_quality_host_evidence_written = 0
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
    Write-Error "OutputRootRelative must be under artifacts/renderer/d3d12-commercial-quality-host-evidence/."
}

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=renderer-d3d12-commercial-quality-host-evidence"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_generator_mode=Plan"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_output_root=$OutputRootRelative"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_writes_evidence=0"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_written=0"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_ready=0"
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

if ([string]::IsNullOrWhiteSpace($PackageSmokeOutputRelative)) {
    if ($RequireReady.IsPresent) {
        Write-Error "d3d12_package_smoke_required"
    }
}
elseif (-not (Test-SafeRepoRelativePath -RelativePath $PackageSmokeOutputRelative)) {
    Write-Error "unsafe_relative_path: PackageSmokeOutputRelative must be a safe repo-relative path."
}
elseif (-not (Test-AllowedInputPath -RelativePath $PackageSmokeOutputRelative)) {
    Write-Error "PackageSmokeOutputRelative must be under approved renderer artifact roots."
}

if ([string]::IsNullOrWhiteSpace($SupplementalHostEvidenceRelative)) {
    if ($RequireReady.IsPresent) {
        Write-Error "d3d12_host_supplement_required"
    }
}
elseif (-not (Test-SafeRepoRelativePath -RelativePath $SupplementalHostEvidenceRelative)) {
    Write-Error "unsafe_relative_path: SupplementalHostEvidenceRelative must be a safe repo-relative path."
}
elseif (-not (Test-AllowedInputPath -RelativePath $SupplementalHostEvidenceRelative)) {
    Write-Error "SupplementalHostEvidenceRelative must be under approved renderer artifact roots."
}

$blockers = [System.Collections.Generic.List[string]]::new()
$packageText = ""
$supplementRaw = ""
$counters = @{}
$supplement = $null

if ([string]::IsNullOrWhiteSpace($PackageSmokeOutputRelative)) {
    $blockers.Add("d3d12_package_smoke_required") | Out-Null
}
else {
    $packageFull = Resolve-RepoRelativePath -RelativePath $PackageSmokeOutputRelative `
        -Label "PackageSmokeOutputRelative"
    if (-not (Test-Path -LiteralPath $packageFull -PathType Leaf)) {
        $blockers.Add("d3d12_package_smoke_missing") | Out-Null
    }
    else {
        $packageText = Get-Content -LiteralPath $packageFull -Raw
        $counters = ConvertFrom-KeyValueCounterText -Text $packageText
    }
}

if ([string]::IsNullOrWhiteSpace($SupplementalHostEvidenceRelative)) {
    $blockers.Add("d3d12_host_supplement_required") | Out-Null
}
else {
    $supplementFull = Resolve-RepoRelativePath -RelativePath $SupplementalHostEvidenceRelative `
        -Label "SupplementalHostEvidenceRelative"
    if (-not (Test-Path -LiteralPath $supplementFull -PathType Leaf)) {
        $blockers.Add("d3d12_host_supplement_missing") | Out-Null
    }
    else {
        $supplementRaw = Get-Content -LiteralPath $supplementFull -Raw
        $supplement = Read-JsonFile -Path $supplementFull -Label "SupplementalHostEvidenceRelative"
        $fixtureOnly = Get-JsonPropertyValue -JsonObject $supplement -Name "fixture_only"
        if ($fixtureOnly -is [bool] -and [bool]$fixtureOnly) {
            Write-Error "fixture_host_supplement_rejected: $SupplementalHostEvidenceRelative"
        }
    }
}

if ($null -ne $supplement) {
    Add-BlockerIfFalse $blockers `
        ([string](Get-JsonPropertyValue -JsonObject $supplement -Name "schema_version") -ceq
            "GameEngine.RendererD3d12CommercialQualityHostSupplement.v1") `
        "d3d12_host_supplement_schema_invalid"
    Add-BlockerIfFalse $blockers `
        ([string](Get-JsonPropertyValue -JsonObject $supplement -Name "validation_recipe") -ceq
            "renderer-d3d12-commercial-quality-host-evidence") `
        "d3d12_host_supplement_recipe_invalid"
    Add-BlockerIfFalse $blockers `
        ([string](Get-JsonPropertyValue -JsonObject $supplement -Name "source_id") -ceq $expectedSourceId) `
        "d3d12_host_supplement_source_invalid"
    Add-BlockerIfFalse $blockers `
        (Test-JsonBoolFalse -JsonObject $supplement -Name "fixture_only") `
        "d3d12_host_supplement_fixture_flag_invalid"

    $proofRows = Get-JsonPropertyValue -JsonObject $supplement -Name "proof_rows"
    $clockCalibration = Get-JsonPropertyValue -JsonObject $proofRows -Name "clock_calibration"
    $debugValidation = Get-JsonPropertyValue -JsonObject $proofRows -Name "debug_validation"
    $residency = Get-JsonPropertyValue -JsonObject $proofRows -Name "residency"
    $unorderedAccessBarrier = Get-JsonPropertyValue -JsonObject $proofRows -Name "unordered_access_barrier"
    $nativeHandles = Get-JsonPropertyValue -JsonObject $proofRows -Name "native_handles"

    Add-BlockerIfFalse $blockers `
        ((Test-JsonBoolTrue -JsonObject $clockCalibration -Name "ready") -and
            ([string](Get-JsonPropertyValue -JsonObject $clockCalibration -Name "api_name") -ceq
                "ID3D12CommandQueue::GetClockCalibration") -and
            (Test-JsonBoolTrue -JsonObject $clockCalibration -Name "cpu_qpc_sample")) `
        "d3d12_clock_calibration_supplement_required"
    Add-BlockerIfFalse $blockers `
        ((Test-JsonBoolTrue -JsonObject $debugValidation -Name "ready") -and
            (Test-JsonBoolTrue -JsonObject $debugValidation -Name "debug_layer_or_gpu_based_validation_clean") -and
            (Test-JsonIntegerEquals -JsonObject $debugValidation -Name "debug_message_count" -Expected 0) -and
            (Test-JsonIntegerEquals -JsonObject $debugValidation -Name "gpu_based_validation_message_count" -Expected 0)) `
        "d3d12_debug_validation_supplement_required"
    Add-BlockerIfFalse $blockers `
        ((Test-JsonBoolTrue -JsonObject $residency -Name "ready") -and
            (Test-JsonBoolTrue -JsonObject $residency -Name "query_video_memory_info_ready") -and
            (Test-JsonBoolTrue -JsonObject $residency -Name "enqueue_make_resident_fence_signaled") -and
            ([string](Get-JsonPropertyValue -JsonObject $residency -Name "residency_api_name") -ceq
                "ID3D12Device3::EnqueueMakeResident") -and
            ([string](Get-JsonPropertyValue -JsonObject $residency -Name "budget_api_name") -ceq
                "IDXGIAdapter3::QueryVideoMemoryInfo")) `
        "d3d12_residency_supplement_required"
    Add-BlockerIfFalse $blockers `
        ((Test-JsonBoolTrue -JsonObject $unorderedAccessBarrier -Name "ready") -and
            ([string](Get-JsonPropertyValue -JsonObject $unorderedAccessBarrier -Name "resource_barrier_api_name") -ceq
                "D3D12_RESOURCE_BARRIER")) `
        "d3d12_unordered_access_barrier_supplement_required"
    Add-BlockerIfFalse $blockers `
        ((Test-JsonBoolTrue -JsonObject $nativeHandles -Name "ready") -and
            (Test-JsonBoolFalse -JsonObject $nativeHandles -Name "native_handles_exposed")) `
        "d3d12_native_handles_supplement_required"

    $nonClaims = Get-JsonPropertyValue -JsonObject $supplement -Name "non_claims"
    foreach ($nonClaim in @(
            "vulkan_inferred",
            "metal_inferred",
            "broad_ui_parity",
            "environment_ready",
            "external_engine_parity",
            "native_handles_exposed"
        )) {
        Add-BlockerIfFalse $blockers (Test-JsonBoolFalse -JsonObject $nonClaims -Name $nonClaim) `
            "d3d12_non_claim_invalid_$nonClaim"
    }
}

$commandAllocatorReady =
    (Test-CounterEquals -Counters $counters -Name "d3d12_framegraph_multiqueue_evidence_ready" -Expected "1") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "framegraph_multiqueue_command_lists_submitted") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "framegraph_multiqueue_submitted_pass_fences") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "framegraph_multiqueue_queue_waits_recorded") -and
    (Test-CounterEquals -Counters $counters -Name "framegraph_multiqueue_graphics_waited_for_copy" -Expected "1") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "d3d12_debug_profiling_execution_framegraph_render_passes_recorded")
Add-BlockerIfFalse $blockers $commandAllocatorReady "d3d12_command_allocator_fence_package_counters_required"

$resourceBarrierReady =
    (Test-CounterPositiveInteger -Counters $counters -Name "framegraph_barrier_steps_executed") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "framegraph_multiqueue_barriers_recorded") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "framegraph_multiqueue_aliasing_barriers_recorded") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_quality_framegraph_barrier_steps_ok" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "postprocess_d3d12_execution_passes_ok" -Expected "1") -and
    ($blockers -notcontains "d3d12_unordered_access_barrier_supplement_required")
Add-BlockerIfFalse $blockers $resourceBarrierReady "d3d12_resource_barrier_package_counters_required"

$timestampReady =
    (Test-CounterEquals -Counters $counters -Name "debug_profiling_policy_backend_profiling_evidence_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "d3d12_debug_profiling_execution_gpu_timestamps_ok" -Expected "1") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "d3d12_debug_profiling_execution_gpu_timestamp_ticks_per_second") -and
    ($blockers -notcontains "d3d12_clock_calibration_supplement_required")
Add-BlockerIfFalse $blockers $timestampReady "d3d12_timestamp_package_counters_required"

$debugValidationReady =
    (Test-CounterEquals -Counters $counters -Name "debug_profiling_policy_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "debug_profiling_policy_diagnostics" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "d3d12_debug_profiling_execution_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "d3d12_debug_profiling_execution_frame_diagnostics_ok" -Expected "1") -and
    ($blockers -notcontains "d3d12_debug_validation_supplement_required")
Add-BlockerIfFalse $blockers $debugValidationReady "d3d12_debug_validation_package_counters_required"

$residencyReady =
    (Test-CounterEquals -Counters $counters -Name "gpu_memory_policy_backend_memory_evidence_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "memory_diagnostics_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "memory_diagnostics_diagnostics" -Expected "0") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "memory_diagnostics_resident_gpu_bytes") -and
    (Test-CounterEquals -Counters $counters -Name "d3d12_gpu_memory_execution_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "d3d12_gpu_memory_execution_budget_ok" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "d3d12_gpu_memory_execution_transient_heap_ok" -Expected "1") -and
    (Test-CounterPositiveInteger -Counters $counters -Name "d3d12_gpu_memory_execution_committed_resources_byte_estimate") -and
    ($blockers -notcontains "d3d12_residency_supplement_required")
Add-BlockerIfFalse $blockers $residencyReady "d3d12_residency_package_counters_required"

$packageReadbackReady =
    (Test-CounterEquals -Counters $counters -Name "renderer_quality_status" -Expected "ready") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_quality_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_quality_diagnostics" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "postprocess_d3d12_execution_status" -Expected "ready") -and
    (Test-CounterEquals -Counters $counters -Name "postprocess_d3d12_execution_ready" -Expected "1")
Add-BlockerIfFalse $blockers $packageReadbackReady "d3d12_package_visible_readback_counters_required"

if ($blockers.Count -ne 0) {
    $status = "host_evidence_required"
    $outputRootFull = Resolve-RepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
    $summaryFull = Resolve-RepoRelativePath -RelativePath $summaryRelative -Label "host gate summary"
    if (-not $NoWrite.IsPresent) {
        $summary = New-BlockedSummary -Blockers $blockers.ToArray() -Status $status
        Write-JsonObject -Path $summaryFull -Value $summary
    }
    Write-Output "validation_recipe=renderer-d3d12-commercial-quality-host-evidence"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_generator_mode=Generate"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_status=$status"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_output_root=$OutputRootRelative"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_missing_rows=$($blockers.Count)"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_missing_row_names=$(ConvertTo-CounterValue -Value ([string]::Join(',', $blockers.ToArray())))"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_writes_evidence=$(ConvertTo-CounterBit (-not $NoWrite.IsPresent))"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_written=0"
    Write-Output "renderer_d3d12_commercial_quality_host_evidence_ready=0"
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
    if ($RequireReady.IsPresent) {
        Write-Error "d3d12_host_evidence_not_ready: $([string]::Join(', ', $blockers.ToArray()))"
    }
    return
}

$proofRows = Get-JsonPropertyValue -JsonObject $supplement -Name "proof_rows"
$debugValidation = Get-JsonPropertyValue -JsonObject $proofRows -Name "debug_validation"
$residency = Get-JsonPropertyValue -JsonObject $proofRows -Name "residency"
$hashInput = [string]::Join("`n", @($packageText.Trim(), $supplementRaw.Trim()))
$readbackHash = Get-LowerSha256ForText -Text $hashInput
$queueFrequencyHz = [long](Get-CounterString -Counters $counters -Name "d3d12_debug_profiling_execution_gpu_timestamp_ticks_per_second")

$evidence = [ordered]@{
    schema_version = "GameEngine.RendererD3d12CommercialQualityHostEvidence.v1"
    claim_id = "renderer-d3d12-commercial-quality-artifact-v1"
    validation_recipe = "renderer-d3d12-quality-evidence"
    fixture_only = $false
    source_id = $expectedSourceId
    proof_rows = [ordered]@{
        command_allocator_list_fence = [ordered]@{
            ready = $true
            command_allocator_reuse_fenced = $true
            command_list_closed_before_execute = $true
            fence_signal_wait_recorded = $true
            fence_api_name = "ID3D12Fence"
        }
        resource_barriers = [ordered]@{
            ready = $true
            render_transition_explicit = $true
            copy_transition_explicit = $true
            unordered_access_barrier_explicit = $true
            readback_transition_explicit = $true
            resource_barrier_api_name = "D3D12_RESOURCE_BARRIER"
        }
        timestamp = [ordered]@{
            ready = $true
            query_type = "D3D12_QUERY_TYPE_TIMESTAMP"
            resolved_query_data = $true
            queue_frequency_hz = $queueFrequencyHz
            clock_calibration = $true
        }
        debug_validation = [ordered]@{
            ready = $true
            debug_layer_or_gpu_based_validation_clean = $true
            debug_message_count = [long](Get-JsonPropertyValue -JsonObject $debugValidation -Name "debug_message_count")
            gpu_based_validation_message_count = [long](Get-JsonPropertyValue -JsonObject $debugValidation -Name "gpu_based_validation_message_count")
        }
        residency = [ordered]@{
            ready = $true
            video_memory_budget_queried = $true
            make_resident_or_budget_recorded = $true
            residency_api_name = [string](Get-JsonPropertyValue -JsonObject $residency -Name "residency_api_name")
            budget_api_name = [string](Get-JsonPropertyValue -JsonObject $residency -Name "budget_api_name")
        }
        package_visible_readback = [ordered]@{
            ready = $true
            deterministic_hash_sha256 = $readbackHash
            readback_counter_rows = 1
            package_counter_id = "renderer_d3d12_package_visible_readback"
        }
        native_handles = [ordered]@{
            ready = $true
            native_handles_exposed = $false
        }
    }
    validation_counters = [ordered]@{
        renderer_d3d12_command_allocator_fence_ready = $true
        renderer_d3d12_resource_barrier_ready = $true
        renderer_d3d12_timestamp_ready = $true
        renderer_d3d12_debug_validation_ready = $true
        renderer_d3d12_residency_ready = $true
        renderer_d3d12_package_readback_ready = $true
    }
    non_claims = New-NonClaims
}

$artifactFull = Resolve-RepoRelativePath -RelativePath $artifactRelative -Label "D3D12 host evidence artifact"
$willWrite = -not $NoWrite.IsPresent
if ($willWrite) {
    Write-JsonObject -Path $artifactFull -Value $evidence
}

$artifactHash = ""
if (Test-Path -LiteralPath $artifactFull -PathType Leaf) {
    $artifactHash = (Get-FileHash -LiteralPath $artifactFull -Algorithm SHA256).Hash.ToLowerInvariant()
}

Write-Output "validation_recipe=renderer-d3d12-commercial-quality-host-evidence"
Write-Output "renderer_d3d12_commercial_quality_host_evidence_generator_mode=Generate"
Write-Output "renderer_d3d12_commercial_quality_host_evidence_status=ready"
Write-Output "renderer_d3d12_commercial_quality_host_evidence_output_root=$OutputRootRelative"
Write-Output "renderer_d3d12_commercial_quality_host_evidence_path=$artifactRelative"
Write-Output "renderer_d3d12_commercial_quality_host_evidence_hash=$artifactHash"
Write-Output "renderer_d3d12_commercial_quality_host_evidence_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_d3d12_commercial_quality_host_evidence_written=$(ConvertTo-CounterBit ($willWrite -and (Test-Path -LiteralPath $artifactFull -PathType Leaf)))"
Write-Output "renderer_d3d12_commercial_quality_host_evidence_ready=1"
Write-Output "renderer_d3d12_commercial_quality_host_evidence_source_id=$expectedSourceId"
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

Write-Information "renderer-d3d12-commercial-quality-host-evidence-generator: ok" -InformationAction Continue
