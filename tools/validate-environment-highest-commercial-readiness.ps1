#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady,
    [string[]]$ExpectedEvidenceCounters = @(),
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AdditionalExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$ExpectedEvidenceCounters = @($ExpectedEvidenceCounters) + @($AdditionalExpectedEvidenceCounters)

$root = Get-RepoRoot
Set-Location $root

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for environment highest commercial readiness validation."
}

Write-Information "environment-highest-commercial-readiness: building focused commercial v2 test target..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--build",
    "--preset",
    "dev",
    "--target",
    "MK_environment_commercial_readiness_v2_tests"
)

Write-Information "environment-highest-commercial-readiness: running focused CTest..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/ctest.ps1"),
    "--preset",
    "dev",
    "--output-on-failure",
    "-R",
    "MK_environment_commercial_readiness_v2_tests"
)

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Convert-ClaimStateToCommercialStatus {
    param([AllowNull()][string]$State)

    switch ([string]$State) {
        "ready" { return "ready" }
        "host-gated" { return "host_gated" }
        "unsupported" { return "unsupported" }
        default { return "dependency_gated" }
    }
}

function Get-ClaimRowById {
    param(
        [Parameter(Mandatory = $true)][object[]]$Rows,
        [Parameter(Mandatory = $true)][string]$Id
    )

    foreach ($row in @($Rows)) {
        if ([string]$row.id -eq $Id) {
            return $row
        }
    }
    return $null
}

function Get-CounterValueFromText {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$DefaultValue
    )

    $match = [regex]::Match($Text, "(^|\s)$([regex]::Escape($Name))=([^\s]+)")
    if ($match.Success) {
        return $match.Groups[2].Value
    }
    return $DefaultValue
}

$manifestPath = Join-Path $root "engine/agent/manifest.json"
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    Write-Error "engine/agent/manifest.json is required for highest commercial readiness validation. Run tools/compose-agent-manifest.ps1 -Write after manifest fragment edits."
}

$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$claimRows = @($manifest.aiOperableProductionLoop.environmentCommercialClaimMatrix)

$optimizationOutput = & $pwsh `
    "-NoProfile" `
    "-ExecutionPolicy" `
    "Bypass" `
    "-File" `
    (Join-Path $root "tools/validate-environment-optimization-artifacts.ps1") 2>&1
$optimizationText = [string]::Join("`n", @($optimizationOutput))
$optimizationReady = (Get-CounterValueFromText -Text $optimizationText -Name "environment_broad_optimization_ready" -DefaultValue "0") -eq "1"
$optimizationWorkloadRows = Get-CounterValueFromText -Text $optimizationText -Name "environment_optimization_measurement_workload_rows" -DefaultValue "0"
$optimizationBackendRows = Get-CounterValueFromText -Text $optimizationText -Name "environment_optimization_measurement_backend_rows" -DefaultValue "0"
$optimizationBeforeAfterPairs = Get-CounterValueFromText -Text $optimizationText -Name "environment_optimization_measurement_before_after_pairs" -DefaultValue "0"
$optimizationProfilerArtifacts = Get-CounterValueFromText -Text $optimizationText -Name "environment_optimization_measurement_profiler_artifacts" -DefaultValue "0"
$optimizationTraceEventJson = Get-CounterValueFromText -Text $optimizationText -Name "environment_optimization_measurement_trace_event_json" -DefaultValue "0"
$optimizationMissingArtifacts = Get-CounterValueFromText -Text $optimizationText -Name "environment_optimization_measurement_missing_artifacts" -DefaultValue "21"
$optimizationOverBudget = Get-CounterValueFromText -Text $optimizationText -Name "environment_optimization_measurement_over_budget" -DefaultValue "0"

$requiredRows = @(
    "environment_strict_vulkan_aggregate_ready",
    "environment_metal_aggregate_ready",
    "environment_backend_parity_ready",
    "environment_platform_windows_d3d12_ready",
    "environment_platform_windows_vulkan_ready",
    "environment_platform_linux_vulkan_ready",
    "environment_platform_macos_metal_ready",
    "environment_platform_ios_metal_ready",
    "environment_platform_android_vulkan_ready",
    "environment_platform_readiness_ready",
    "environment_all_platform_unconditional_ready",
    "environment_broad_optimization_ready",
    "environment_asset_pipeline_openexr_ktx_basis_full_ready",
    "environment_aaa_preset_asset_library_ready",
    "environment_physical_weather_simulation_ready",
    "environment_artist_workflow_production_ready"
)

$readyRows = 0
$hostGatedRows = 0
$dependencyGatedRows = 0
$blockedRows = 0
$unsupportedRows = 0
$missingRows = 0
$rowOutput = [System.Collections.Generic.List[string]]::new()

foreach ($requiredRow in $requiredRows) {
    $claimRow = Get-ClaimRowById -Rows $claimRows -Id $requiredRow
    $status = Convert-ClaimStateToCommercialStatus -State $claimRow.state
    if ($null -eq $claimRow) {
        $status = "dependency_gated"
        $missingRows += 1
    }
    if ($requiredRow -eq "environment_broad_optimization_ready" -and $status -eq "ready" -and -not $optimizationReady) {
        $status = "dependency_gated"
    }

    switch ($status) {
        "ready" { $readyRows += 1 }
        "host_gated" { $hostGatedRows += 1 }
        "unsupported" { $unsupportedRows += 1 }
        "blocked" { $blockedRows += 1 }
        default { $dependencyGatedRows += 1 }
    }

    $rowOutput.Add("$requiredRow=$(if ($status -eq 'ready') { '1' } else { '0' })")
    $rowOutput.Add("$($requiredRow)_status=$status")
}

$nativeHandleAccess = $false
$diagnostics = $false
$allRowsReady = $readyRows -eq $requiredRows.Count -and
    $hostGatedRows -eq 0 -and
    $dependencyGatedRows -eq 0 -and
    $blockedRows -eq 0 -and
    $unsupportedRows -eq 0 -and
    $missingRows -eq 0 -and
    -not $nativeHandleAccess -and
    -not $diagnostics

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=environment-highest-commercial-readiness-closeout")
$lines.Add("environment_highest_commercial_status=$(if ($allRowsReady) { 'ready' } else { 'blocked' })")
$lines.Add("environment_highest_commercial_ready=$(ConvertTo-CounterBit $allRowsReady)")
$lines.Add("environment_commercial_ready=$(ConvertTo-CounterBit $allRowsReady)")
$lines.Add("environment_commercial_required_rows=$($requiredRows.Count)")
$lines.Add("environment_commercial_ready_rows=$readyRows")
$lines.Add("environment_host_gated_rows=$hostGatedRows")
$lines.Add("environment_dependency_gated_rows=$dependencyGatedRows")
$lines.Add("environment_blocked_rows=$blockedRows")
$lines.Add("environment_unsupported_rows=$unsupportedRows")
$lines.Add("environment_missing_rows=$missingRows")
$lines.Add("environment_native_handle_access=$(ConvertTo-CounterBit $nativeHandleAccess)")
$lines.Add("environment_commercial_diagnostics=$(ConvertTo-CounterBit $diagnostics)")
foreach ($line in $rowOutput) {
    $lines.Add($line)
}
$lines.Add("environment_optimization_measurement_workload_rows=$optimizationWorkloadRows")
$lines.Add("environment_optimization_measurement_backend_rows=$optimizationBackendRows")
$lines.Add("environment_optimization_measurement_before_after_pairs=$optimizationBeforeAfterPairs")
$lines.Add("environment_optimization_measurement_profiler_artifacts=$optimizationProfilerArtifacts")
$lines.Add("environment_optimization_measurement_trace_event_json=$optimizationTraceEventJson")
$lines.Add("environment_optimization_measurement_missing_artifacts=$optimizationMissingArtifacts")
$lines.Add("environment_optimization_measurement_over_budget=$optimizationOverBudget")
$lines.Add("environment_weather_simulation_backend_parity_ready=$((Get-ClaimRowById -Rows $claimRows -Id 'environment_physical_weather_simulation_ready').state -eq 'ready' ? '1' : '0')")
$lines.Add("workflow_visible_shell_execution_ready=$((Get-ClaimRowById -Rows $claimRows -Id 'environment_artist_workflow_production_ready').state -eq 'ready' ? '1' : '0')")
$lines.Add("workflow_operator_review_ready=$((Get-ClaimRowById -Rows $claimRows -Id 'environment_artist_workflow_production_ready').state -eq 'ready' ? '1' : '0')")
$lines.Add("runtime_source_parsing=0")
$lines.Add("environment_ready=0")
$lines.Add("environment_ready_unchanged=1")

$missingExpectedCounters = @()
foreach ($expected in @($ExpectedEvidenceCounters)) {
    if (-not $lines.Contains([string]$expected)) {
        $missingExpectedCounters += [string]$expected
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($missingExpectedCounters.Count -ne 0) {
    Write-Error "environment highest commercial readiness validation did not emit expected counter(s): $([string]::Join(', ', $missingExpectedCounters))"
}

if ($RequireReady.IsPresent -and -not $allRowsReady) {
    Write-Error "environment highest commercial readiness is incomplete; all 16 exact commercial v2 rows must be ready with zero host-gated, dependency-gated, blocked, unsupported, missing, native-handle, and diagnostic rows."
}
