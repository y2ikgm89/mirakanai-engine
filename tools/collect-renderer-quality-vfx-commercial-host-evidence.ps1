#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10T

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Collect")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/quality-vfx-commercial-host-evidence/current-run",

    [string]$Generated3dStatusLogRelative = "",

    [string]$PackageHostEvidenceRelative = "",

    [string]$D3d12HostEvidenceRelative = "",

    [string]$VulkanStrictHostEvidenceRelative = "",

    [string]$AppleMetalHostEvidenceRelative = "",

    [string]$MetalMemoryProfilingHostEvidenceRelative = "",

    [switch]$NoWrite,

    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$expectedSourceId = "GameEngine-Renderer-Quality-Vfx-Profiling-2026-06-25"
$artifactRelative = "$OutputRootRelative/quality-vfx-host-evidence.json"
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
        "artifacts/renderer/quality-vfx-commercial-host-evidence/",
        [System.StringComparison]::Ordinal)
}

function Test-AllowedInputPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    foreach ($prefix in @(
            "artifacts/renderer/quality-vfx-commercial-host-evidence/",
            "artifacts/renderer/commercial-readiness-evidence/",
            "artifacts/renderer/package-commercial-quality-host-evidence/",
            "artifacts/renderer/d3d12-commercial-quality-host-evidence/",
            "artifacts/renderer/vulkan-strict-commercial-quality-host-evidence/",
            "artifacts/renderer/apple-metal-commercial-quality-host-evidence/",
            "artifacts/renderer/metal-memory-profiling-host-artifacts/"
        )) {
        if ($normalizedPath.StartsWith($prefix, [System.StringComparison]::Ordinal)) {
            return $true
        }
    }
    return $false
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

function Read-JsonFileOrNull {
    param([Parameter(Mandatory = $true)][string]$Path)

    try {
        return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    }
    catch {
        return $null
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

function Write-JsonObject {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $parent -Force
    }
    $Value | ConvertTo-Json -Depth 24 | Set-Content -LiteralPath $Path -Encoding utf8NoBOM
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

function Read-StatusEvidenceOrNull {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$ExpectedTarget,
        [Parameter(Mandatory = $true)][string]$Label,
        [System.Collections.Generic.List[string]]$Blockers
    )

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        $Blockers.Add("$($Label)_required") | Out-Null
        return $null
    }
    if (-not (Test-SafeRepoRelativePath -RelativePath $RelativePath)) {
        Write-Error "unsafe_relative_path: $Label must be a safe repo-relative status log path."
    }
    if (-not (Test-AllowedInputPath -RelativePath $RelativePath)) {
        Write-Error "$Label must be under an approved renderer artifact root."
    }

    $path = Resolve-RepoRelativePath -RelativePath $RelativePath -Label $Label
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        $Blockers.Add("$($Label)_missing") | Out-Null
        return $null
    }

    $lines = @(Get-Content -LiteralPath $path)
    $targetPrefix = "$ExpectedTarget status="
    $statusLine = [string]($lines | Where-Object {
            [string]$_ -like "$targetPrefix*"
        } | Select-Object -Last 1)
    if ([string]::IsNullOrWhiteSpace($statusLine)) {
        $Blockers.Add("$($Label)_status_line_missing") | Out-Null
        return $null
    }

    $values = @{}
    foreach ($match in [regex]::Matches($statusLine, "(^|\s)([A-Za-z0-9_.-]+)=([^\s]+)")) {
        $values[[string]$match.Groups[2].Value] = [string]$match.Groups[3].Value
    }

    return [pscustomobject]@{
        relative_path = $RelativePath
        status_line = $statusLine
        values = $values
    }
}

function Test-StatusValueEquals {
    param(
        [AllowNull()]$Evidence,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Expected
    )

    if ($null -eq $Evidence -or -not $Evidence.values.ContainsKey($Name)) {
        return $false
    }
    return [string]$Evidence.values[$Name] -ceq $Expected
}

function Test-StatusPositiveInteger {
    param(
        [AllowNull()]$Evidence,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if ($null -eq $Evidence -or -not $Evidence.values.ContainsKey($Name)) {
        return $false
    }
    $parsed = [UInt64]0
    return [UInt64]::TryParse([string]$Evidence.values[$Name], [ref]$parsed) -and $parsed -gt 0
}

function Read-RetainedEvidenceOrNull {
    param(
        [string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Label,
        [Parameter(Mandatory = $true)][string]$ExpectedSchemaVersion,
        [Parameter(Mandatory = $true)][string]$ExpectedClaimId,
        [System.Collections.Generic.List[string]]$Blockers
    )

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        $Blockers.Add("$($Label)_required") | Out-Null
        return $null
    }
    if (-not (Test-SafeRepoRelativePath -RelativePath $RelativePath)) {
        Write-Error "unsafe_relative_path: $Label must be a safe repo-relative JSON path."
    }
    if (-not (Test-AllowedInputPath -RelativePath $RelativePath)) {
        Write-Error "$Label must be under an approved renderer artifact root."
    }

    $path = Resolve-RepoRelativePath -RelativePath $RelativePath -Label $Label
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        $Blockers.Add("$($Label)_missing") | Out-Null
        return $null
    }
    $json = Read-JsonFileOrNull -Path $path
    if ($null -eq $json) {
        $Blockers.Add("$($Label)_invalid_json") | Out-Null
        return $null
    }
    if ([string](Get-JsonPropertyValue -JsonObject $json -Name "schema_version") -cne $ExpectedSchemaVersion) {
        $Blockers.Add("$($Label)_schema_version_mismatch") | Out-Null
        return $null
    }
    if ([string](Get-JsonPropertyValue -JsonObject $json -Name "claim_id") -cne $ExpectedClaimId) {
        $Blockers.Add("$($Label)_claim_id_mismatch") | Out-Null
        return $null
    }
    $fixtureOnly = Get-JsonPropertyValue -JsonObject $json -Name "fixture_only"
    if ($fixtureOnly -isnot [bool]) {
        $Blockers.Add("$($Label)_fixture_only_type_mismatch") | Out-Null
        return $null
    }
    if ([bool]$fixtureOnly) {
        $Blockers.Add("$($Label)_fixture_only_rejected") | Out-Null
        return $null
    }
    return $json
}

function New-CommonNonClaims {
    return [ordered]@{
        broad_renderer_quality = $false
        renderer_commercial_readiness = $false
        external_engine_parity = $false
        native_handles_exposed = $false
    }
}

function Write-BlockedSummaryAndCounters {
    param(
        [Parameter(Mandatory = $true)][string[]]$Blockers,
        [Parameter(Mandatory = $true)][string]$Status
    )

    $summaryFull = Resolve-RepoRelativePath -RelativePath $summaryRelative -Label "quality/VFX host gate summary"
    if (-not $NoWrite.IsPresent) {
        Write-JsonObject -Path $summaryFull -Value ([ordered]@{
                schema_version = "GameEngine.RendererQualityVfxCommercialHostGate.v1"
                validation_recipe = "renderer-quality-vfx-commercial-host-evidence"
                source_id = $expectedSourceId
                status = $Status
                reason = if ($Blockers.Count -gt 0) { [string]$Blockers[0] } else { $Status }
                blockers = @($Blockers)
                renderer_quality_vfx_commercial_host_evidence_ready = 0
                renderer_quality_vfx_commercial_host_evidence_written = 0
                renderer_backend_parity_ready = 0
                renderer_metal_broad_readiness = 0
                renderer_broad_quality_ready = 0
                renderer_commercial_readiness = 0
                renderer_environment_ready = 0
            })
    }

    Write-Output "validation_recipe=renderer-quality-vfx-commercial-host-evidence"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_mode=Collect"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_status=$Status"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_output_root=$OutputRootRelative"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_missing_rows=$($Blockers.Count)"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_missing_row_names=$(ConvertTo-CounterValue -Value ([string]::Join(',', $Blockers)))"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_writes_evidence=$(ConvertTo-CounterBit (-not $NoWrite.IsPresent))"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_written=0"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_ready=0"
    Write-Output "renderer_quality_matrix_ready=0"
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

    if ($RequireReady.IsPresent) {
        Write-Error "quality_vfx_host_evidence_not_ready: $([string]::Join(', ', $Blockers))"
    }
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/quality-vfx-commercial-host-evidence/."
}

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=renderer-quality-vfx-commercial-host-evidence"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_mode=Plan"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_output_root=$OutputRootRelative"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_writes_evidence=0"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_written=0"
    Write-Output "renderer_quality_vfx_commercial_host_evidence_ready=0"
    Write-Output "renderer_quality_matrix_ready=0"
    Write-Output "rendering_vfx_profiling_reviewed=0"
    Write-Output "renderer_production_vfx_profiling_ready=0"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    Write-Output "renderer_environment_ready=0"
    return
}

$blockers = [System.Collections.Generic.List[string]]::new()
$generated3dEvidence = Read-StatusEvidenceOrNull `
    -RelativePath $Generated3dStatusLogRelative `
    -ExpectedTarget "sample_generated_desktop_runtime_3d_package" `
    -Label "generated_3d_status_log" `
    -Blockers $blockers

$null = Read-RetainedEvidenceOrNull `
    -RelativePath $PackageHostEvidenceRelative `
    -Label "package_host_evidence" `
    -ExpectedSchemaVersion "GameEngine.RendererPackageCommercialQualityHostEvidence.v1" `
    -ExpectedClaimId "renderer-package-commercial-quality-artifacts-v1" `
    -Blockers $blockers
$null = Read-RetainedEvidenceOrNull `
    -RelativePath $D3d12HostEvidenceRelative `
    -Label "d3d12_host_evidence" `
    -ExpectedSchemaVersion "GameEngine.RendererD3d12CommercialQualityHostEvidence.v1" `
    -ExpectedClaimId "renderer-d3d12-commercial-quality-artifact-v1" `
    -Blockers $blockers
$null = Read-RetainedEvidenceOrNull `
    -RelativePath $VulkanStrictHostEvidenceRelative `
    -Label "vulkan_strict_host_evidence" `
    -ExpectedSchemaVersion "GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1" `
    -ExpectedClaimId "renderer-vulkan-strict-commercial-quality-artifact-v1" `
    -Blockers $blockers
$null = Read-RetainedEvidenceOrNull `
    -RelativePath $AppleMetalHostEvidenceRelative `
    -Label "apple_metal_host_evidence" `
    -ExpectedSchemaVersion "GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1" `
    -ExpectedClaimId "renderer-apple-metal-commercial-quality-artifact-v1" `
    -Blockers $blockers
$null = Read-RetainedEvidenceOrNull `
    -RelativePath $MetalMemoryProfilingHostEvidenceRelative `
    -Label "metal_memory_profiling_host_evidence" `
    -ExpectedSchemaVersion "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1" `
    -ExpectedClaimId "renderer-metal-memory-profiling-host-evidence-v1" `
    -Blockers $blockers

foreach ($requiredZero in @(
        "renderer_quality_matrix_gpu_command_side_effects",
        "renderer_quality_matrix_native_capture_side_effects",
        "renderer_quality_matrix_crash_upload_side_effects",
        "renderer_production_vfx_native_capture_side_effects",
        "renderer_production_vfx_crash_upload_side_effects",
        "renderer_backend_parity_ready",
        "renderer_metal_broad_readiness",
        "renderer_broad_quality_ready",
        "renderer_commercial_readiness",
        "environment_ready",
        "native_handles_exposed",
        "external_engine_parity"
    )) {
    Add-BlockerIfFalse $blockers `
        (Test-StatusValueEquals -Evidence $generated3dEvidence -Name $requiredZero -Expected "0") `
        "$($requiredZero)_must_be_zero"
}

foreach ($requiredOne in @(
        "renderer_quality_matrix_reviewed",
        "renderer_quality_matrix_d3d12_ready",
        "renderer_quality_matrix_vulkan_strict_ready",
        "rendering_vfx_profiling_reviewed",
        "rendering_vfx_profiling_d3d12_host_evidence_ready",
        "rendering_vfx_profiling_vulkan_strict_host_evidence_ready",
        "rendering_vfx_profiling_debug_policy_ready",
        "rendering_vfx_profiling_debug_cpu_profile_zone_evidence_ready",
        "rendering_vfx_profiling_debug_trace_capture_handoff_evidence_ready",
        "rendering_vfx_profiling_debug_package_counter_evidence_ready",
        "rendering_vfx_profiling_memory_policy_ready",
        "rendering_vfx_profiling_memory_budget_evidence_ready",
        "rendering_vfx_profiling_memory_residency_pressure_evidence_ready",
        "rendering_vfx_profiling_memory_package_counter_evidence_ready"
    )) {
    Add-BlockerIfFalse $blockers `
        (Test-StatusValueEquals -Evidence $generated3dEvidence -Name $requiredOne -Expected "1") `
        "$($requiredOne)_required"
}

Add-BlockerIfFalse $blockers `
    (Test-StatusValueEquals -Evidence $generated3dEvidence -Name "renderer_quality_matrix_status" -Expected "host_evidence_required") `
    "renderer_quality_matrix_host_gated_status_required"
Add-BlockerIfFalse $blockers `
    (Test-StatusValueEquals -Evidence $generated3dEvidence -Name "renderer_quality_matrix_general_renderer_quality_ready" -Expected "0") `
    "renderer_quality_matrix_general_quality_nonclaim_required"
Add-BlockerIfFalse $blockers `
    (Test-StatusValueEquals -Evidence $generated3dEvidence -Name "renderer_quality_matrix_diagnostics" -Expected "0") `
    "renderer_quality_matrix_clean_diagnostics_required"
Add-BlockerIfFalse $blockers `
    (Test-StatusValueEquals -Evidence $generated3dEvidence -Name "rendering_vfx_profiling_diagnostics" -Expected "0") `
    "rendering_vfx_profiling_clean_diagnostics_required"
Add-BlockerIfFalse $blockers `
    (Test-StatusPositiveInteger -Evidence $generated3dEvidence -Name "renderer_quality_matrix_replay_hash") `
    "renderer_quality_matrix_replay_hash_required"
Add-BlockerIfFalse $blockers `
    (Test-StatusPositiveInteger -Evidence $generated3dEvidence -Name "rendering_vfx_profiling_replay_hash") `
    "rendering_vfx_profiling_replay_hash_required"

if ($blockers.Count -ne 0) {
    Write-BlockedSummaryAndCounters -Blockers $blockers.ToArray() -Status "host_evidence_required"
    return
}

$qualityMatrixHash = Get-LowerSha256ForText -Text "renderer_quality_matrix`n$($generated3dEvidence.status_line)`n$D3d12HostEvidenceRelative`n$VulkanStrictHostEvidenceRelative`n$AppleMetalHostEvidenceRelative"
$vfxHash = Get-LowerSha256ForText -Text "production_vfx_profiling`n$($generated3dEvidence.status_line)`n$PackageHostEvidenceRelative`n$MetalMemoryProfilingHostEvidenceRelative"

$hostEvidence = [ordered]@{
    schema_version = "GameEngine.RendererQualityVfxCommercialHostEvidence.v1"
    claim_id = "renderer-quality-vfx-commercial-artifacts-v1"
    validation_recipe = "renderer-quality-vfx-commercial-artifacts"
    fixture_only = $false
    source_id = $expectedSourceId
    quality_rows = [ordered]@{
        renderer_quality_matrix = [ordered]@{
            validation_recipe = "renderer-quality-matrix"
            proof_rows = [ordered]@{
                matrix_status = [ordered]@{
                    ready = $true
                    renderer_quality_matrix_status = "host_evidence_required"
                    d3d12_renderer_quality_ready = $true
                    vulkan_strict_renderer_quality_ready = $true
                    apple_metal_host_evidence_supplied = $true
                    general_renderer_quality_claim = $false
                }
                side_effect_policy = [ordered]@{
                    ready = $true
                    gpu_command_side_effects = 0
                    native_capture_side_effects = 0
                    crash_upload_side_effects = 0
                }
                replay = [ordered]@{
                    ready = $true
                    deterministic_replay_hash_sha256 = $qualityMatrixHash
                }
                diagnostics = [ordered]@{
                    ready = $true
                    diagnostic_error_count = 0
                }
            }
            validation_counters = [ordered]@{
                renderer_quality_matrix_ready = $true
                renderer_quality_matrix_general_renderer_quality_claim = $false
                renderer_quality_matrix_gpu_command_side_effects = 0
                renderer_quality_matrix_native_capture_side_effects = 0
                renderer_quality_matrix_crash_upload_side_effects = 0
            }
            non_claims = [ordered]@{
                broad_renderer_quality = $false
                renderer_commercial_readiness = $false
                external_engine_parity = $false
                native_handles_exposed = $false
                gpu_command_side_effects = $false
                native_capture_side_effects = $false
                crash_upload_side_effects = $false
            }
        }
        production_vfx_profiling = [ordered]@{
            validation_recipe = "renderer-production-vfx-profiling"
            proof_rows = [ordered]@{
                vfx_profiling_review = [ordered]@{
                    ready = $true
                    rendering_vfx_profiling_reviewed = $true
                    d3d12_renderer_quality_ready = $true
                    vulkan_strict_renderer_quality_ready = $true
                    apple_metal_host_evidence_supplied = $true
                }
                debug_policy = [ordered]@{
                    ready = $true
                    debug_capture_policy_recorded = $true
                    debug_upload_policy_recorded = $true
                    crash_upload_policy_recorded = $true
                }
                memory_policy = [ordered]@{
                    ready = $true
                    memory_residency_policy_recorded = $true
                    metal_memory_profiling_evidence_supplied = $true
                }
                package_counters = [ordered]@{
                    ready = $true
                    visible_3d_package_ready = $true
                    runtime_ui_package_ready = $true
                    environment_package_ready = $true
                    generated_game_package_ready = $true
                }
                replay = [ordered]@{
                    ready = $true
                    deterministic_replay_hash_sha256 = $vfxHash
                }
                side_effect_policy = [ordered]@{
                    ready = $true
                    native_capture_side_effects = 0
                    crash_upload_side_effects = 0
                    retained_official_profiler_artifact_selected = $false
                }
            }
            validation_counters = [ordered]@{
                renderer_production_vfx_profiling_ready = $true
                rendering_vfx_profiling_reviewed = $true
                renderer_production_vfx_native_capture_side_effects = 0
                renderer_production_vfx_crash_upload_side_effects = 0
            }
            non_claims = [ordered]@{
                broad_renderer_quality = $false
                renderer_commercial_readiness = $false
                external_engine_parity = $false
                native_handles_exposed = $false
                native_capture_side_effects = $false
                crash_upload_side_effects = $false
                retained_official_profiler_artifact_selected = $false
            }
        }
    }
    non_claims = [ordered]@{
        broad_renderer_quality = $false
        renderer_commercial_readiness = $false
        external_engine_parity = $false
        native_handles_exposed = $false
        gpu_command_side_effects = $false
        native_capture_side_effects = $false
        crash_upload_side_effects = $false
    }
}

$artifactFull = Resolve-RepoRelativePath -RelativePath $artifactRelative -Label "quality/VFX host evidence artifact"
$willWrite = -not $NoWrite.IsPresent
if ($willWrite) {
    Write-JsonObject -Path $artifactFull -Value $hostEvidence
}

$artifactHash = ""
if (Test-Path -LiteralPath $artifactFull -PathType Leaf) {
    $artifactHash = (Get-FileHash -LiteralPath $artifactFull -Algorithm SHA256).Hash.ToLowerInvariant()
}

Write-Output "validation_recipe=renderer-quality-vfx-commercial-host-evidence"
Write-Output "renderer_quality_vfx_commercial_host_evidence_mode=Collect"
Write-Output "renderer_quality_vfx_commercial_host_evidence_status=ready"
Write-Output "renderer_quality_vfx_commercial_host_evidence_output_root=$OutputRootRelative"
Write-Output "renderer_quality_vfx_commercial_host_evidence_path=$artifactRelative"
Write-Output "renderer_quality_vfx_commercial_host_evidence_hash=$artifactHash"
Write-Output "renderer_quality_vfx_commercial_host_evidence_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_quality_vfx_commercial_host_evidence_written=$(ConvertTo-CounterBit ($willWrite -and (Test-Path -LiteralPath $artifactFull -PathType Leaf)))"
Write-Output "renderer_quality_vfx_commercial_host_evidence_ready=1"
Write-Output "renderer_quality_vfx_commercial_host_evidence_source_id=$expectedSourceId"
Write-Output "renderer_quality_matrix_ready=1"
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

Write-Information "renderer-quality-vfx-commercial-host-evidence-collector: ok" -InformationAction Continue
