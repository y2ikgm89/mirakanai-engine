#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10R

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Generate")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/apple-metal-commercial-quality-host-evidence/renderer-commercial-readiness",

    [string]$AppleMetalStatusLogRelative = "",

    [switch]$NoWrite,

    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$expectedHostSourceId = "Apple-Metal-Commercial-Host-Bridge-2026-06-25"
$expectedToolchainSourceId = "Apple-Building-Shader-Library-Precompiling-Source-Files-2026-06-25"
$expectedMslSourceId = "Apple-Metal-Shading-Language-Specification-2026-06-25"
$artifactRelative = "$OutputRootRelative/apple-metal-host-evidence.json"
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
        "artifacts/renderer/apple-metal-commercial-quality-host-evidence/",
        [System.StringComparison]::Ordinal)
}

function Test-AllowedInputPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    return $normalizedPath.StartsWith(
        "artifacts/renderer/apple-metal-commercial-quality-host-evidence/",
        [System.StringComparison]::Ordinal) -or
        $normalizedPath.StartsWith(
            "artifacts/renderer/commercial-readiness-evidence/",
            [System.StringComparison]::Ordinal) -or
        $normalizedPath.StartsWith(
            "artifacts/environment/optimization/",
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

function Test-CounterLowerSha256 {
    param(
        [Parameter(Mandatory = $true)]$Counters,
        [Parameter(Mandatory = $true)][string]$Name
    )

    return (Get-CounterString -Counters $Counters -Name $Name) -cmatch "^[0-9a-f]{64}$"
}

function Test-CounterListContainsAll {
    param(
        [Parameter(Mandatory = $true)]$Counters,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string[]]$ExpectedValues
    )

    $values = @((Get-CounterString -Counters $Counters -Name $Name) -split "," |
        Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
    foreach ($expectedValue in @($ExpectedValues)) {
        if ($values -cnotcontains $expectedValue) {
            return $false
        }
    }
    return $true
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
        vulkan_inferred = $false
        environment_ready = $false
        external_engine_parity = $false
        native_handles_exposed = $false
        metal_objects_public = $false
    }
}

function New-BlockedSummary {
    param(
        [Parameter(Mandatory = $true)][string[]]$Blockers,
        [Parameter(Mandatory = $true)][string]$Status
    )

    $reason = "apple_metal_host_evidence_required"
    if (@($Blockers).Count -gt 0) {
        $reason = [string]$Blockers[0]
    }

    return [ordered]@{
        schema_version = "GameEngine.RendererAppleMetalCommercialQualityHostGate.v1"
        validation_recipe = "renderer-apple-metal-commercial-quality-host-evidence"
        source_id = $expectedHostSourceId
        status = $Status
        reason = $reason
        blockers = @($Blockers)
        renderer_apple_metal_commercial_quality_host_evidence_ready = 0
        renderer_apple_metal_commercial_quality_host_evidence_written = 0
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
    Write-Error "OutputRootRelative must be under artifacts/renderer/apple-metal-commercial-quality-host-evidence/."
}

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=renderer-apple-metal-commercial-quality-host-evidence"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_generator_mode=Plan"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_output_root=$OutputRootRelative"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_writes_evidence=0"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_written=0"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_ready=0"
    Write-Output "renderer_apple_metal_xcode_tools_ready=0"
    Write-Output "renderer_apple_metal_msl_shader_ready=0"
    Write-Output "renderer_apple_metal_visible_package_ready=0"
    Write-Output "renderer_apple_metal_native_handles_exposed=0"
    Write-Output "renderer_apple_metal_cross_backend_inference=0"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    Write-Output "renderer_environment_ready=0"
    return
}

if ([string]::IsNullOrWhiteSpace($AppleMetalStatusLogRelative)) {
    if ($RequireReady.IsPresent) {
        Write-Error "apple_metal_status_log_required"
    }
}
elseif (-not (Test-SafeRepoRelativePath -RelativePath $AppleMetalStatusLogRelative)) {
    Write-Error "unsafe_relative_path: AppleMetalStatusLogRelative must be a safe repo-relative path."
}
elseif (-not (Test-AllowedInputPath -RelativePath $AppleMetalStatusLogRelative)) {
    Write-Error "AppleMetalStatusLogRelative must be under approved Apple Metal artifact roots."
}

$blockers = [System.Collections.Generic.List[string]]::new()
$statusText = ""
$counters = @{}

if ([string]::IsNullOrWhiteSpace($AppleMetalStatusLogRelative)) {
    $blockers.Add("apple_metal_status_log_required") | Out-Null
}
else {
    $statusLogFull = Resolve-RepoRelativePath -RelativePath $AppleMetalStatusLogRelative `
        -Label "AppleMetalStatusLogRelative"
    if (-not (Test-Path -LiteralPath $statusLogFull -PathType Leaf)) {
        $blockers.Add("apple_metal_status_log_missing") | Out-Null
    }
    else {
        $statusText = Get-Content -LiteralPath $statusLogFull -Raw
        $counters = ConvertFrom-KeyValueCounterText -Text $statusText
    }
}

$hostSourceReady =
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_commercial_quality_host_source_status" -Expected "ready") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_commercial_quality_host_source_schema" -Expected "GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_commercial_quality_host_source_recipe" -Expected "renderer-metal-apple-host-evidence") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_commercial_quality_host_source_id" -Expected $expectedHostSourceId)
Add-BlockerIfFalse $blockers $hostSourceReady "apple_metal_host_source_rows_required"

$toolchainReady =
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_xcode_tools_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_full_xcode_selected" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_metal_tool_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_metallib_tool_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_command_line_metal_tools" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_toolchain_source_id" -Expected $expectedToolchainSourceId)
Add-BlockerIfFalse $blockers $toolchainReady "apple_metal_toolchain_rows_required"

$mslShaderReady =
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_msl_shader_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_msl_source_id" -Expected $expectedMslSourceId) -and
    (Test-CounterListContainsAll -Counters $counters -Name "renderer_apple_metal_msl_address_spaces" -ExpectedValues @("device", "constant", "threadgroup")) -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_msl_function_constant_attribute" -Expected "[[function_constant]]") -and
    (Test-CounterListContainsAll -Counters $counters -Name "renderer_apple_metal_msl_resource_binding_attributes" -ExpectedValues @("[[buffer]]", "[[texture]]", "[[sampler]]")) -and
    (Test-CounterListContainsAll -Counters $counters -Name "renderer_apple_metal_msl_stage_attributes" -ExpectedValues @("[[vertex]]", "[[fragment]]", "[[kernel]]"))
Add-BlockerIfFalse $blockers $mslShaderReady "apple_metal_msl_shader_rows_required"

$sourceRowsReady =
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_environment_aggregate_recipe" -Expected "renderer-metal-environment-aggregate-apple-host-evidence") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_visible_package_evidence_status" -Expected "ready") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_visible_package_evidence_ready" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_visible_package_broad_claims" -Expected "0")
Add-BlockerIfFalse $blockers $sourceRowsReady "apple_metal_source_visible_package_rows_required"

$visiblePackageReady =
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_visible_package_selected_3d" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_visible_package_runtime_ui" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_visible_package_environment" -Expected "1") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_visible_package_generated_game" -Expected "1") -and
    (Test-CounterIntegerAtLeast -Counters $counters -Name "renderer_apple_metal_visible_package_rows" -Minimum 4) -and
    (Test-CounterLowerSha256 -Counters $counters -Name "renderer_apple_metal_visible_package_hash_sha256")
Add-BlockerIfFalse $blockers $visiblePackageReady "apple_metal_visible_package_rows_required"

$nativeHandlesReady =
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_commercial_quality_native_handles_exposed" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_apple_metal_commercial_quality_cross_backend_inference" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_backend_parity_ready" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_metal_broad_readiness" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_broad_quality_ready" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_commercial_readiness" -Expected "0") -and
    (Test-CounterEquals -Counters $counters -Name "renderer_environment_ready" -Expected "0")
Add-BlockerIfFalse $blockers $nativeHandlesReady "apple_metal_native_handle_non_claim_rows_required"

if ($blockers.Count -ne 0) {
    $status = "host_evidence_required"
    $summaryFull = Resolve-RepoRelativePath -RelativePath $summaryRelative -Label "host gate summary"
    if (-not $NoWrite.IsPresent) {
        $summary = New-BlockedSummary -Blockers $blockers.ToArray() -Status $status
        Write-JsonObject -Path $summaryFull -Value $summary
    }
    Write-Output "validation_recipe=renderer-apple-metal-commercial-quality-host-evidence"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_generator_mode=Generate"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_status=$status"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_output_root=$OutputRootRelative"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_missing_rows=$($blockers.Count)"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_missing_row_names=$(ConvertTo-CounterValue -Value ([string]::Join(',', $blockers.ToArray())))"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_writes_evidence=$(ConvertTo-CounterBit (-not $NoWrite.IsPresent))"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_written=0"
    Write-Output "renderer_apple_metal_commercial_quality_host_evidence_ready=0"
    Write-Output "renderer_apple_metal_xcode_tools_ready=0"
    Write-Output "renderer_apple_metal_msl_shader_ready=0"
    Write-Output "renderer_apple_metal_visible_package_ready=0"
    Write-Output "renderer_apple_metal_native_handles_exposed=0"
    Write-Output "renderer_apple_metal_cross_backend_inference=0"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    Write-Output "renderer_environment_ready=0"
    if ($RequireReady.IsPresent) {
        Write-Error "apple_metal_commercial_quality_host_evidence_not_ready: $([string]::Join(', ', $blockers.ToArray()))"
    }
    return
}

$visiblePackageHash = Get-CounterString -Counters $counters -Name "renderer_apple_metal_visible_package_hash_sha256"
$visiblePackageRows = [long](Get-CounterString -Counters $counters -Name "renderer_apple_metal_visible_package_rows")

$evidence = [ordered]@{
    schema_version = "GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1"
    claim_id = "renderer-apple-metal-commercial-quality-artifact-v1"
    validation_recipe = "renderer-metal-apple-host-evidence"
    fixture_only = $false
    source_id = $expectedHostSourceId
    source_rows = [ordered]@{
        renderer_host_validation_recipe_id = "renderer-metal-apple-host-evidence"
        environment_aggregate_validation_recipe_id = "renderer-metal-environment-aggregate-apple-host-evidence"
        visible_package_evidence_status = "ready"
        visible_package_evidence_ready = $true
        visible_package_broad_claims = $false
    }
    proof_rows = [ordered]@{
        host_toolchain = [ordered]@{
            ready = $true
            xcode_host_ready = $true
            metal_tool_ready = $true
            metallib_tool_ready = $true
            command_line_metal_tools = $true
            toolchain_source_id = $expectedToolchainSourceId
        }
        msl_shader = [ordered]@{
            ready = $true
            address_spaces = @("device", "constant", "threadgroup")
            function_constant_attribute = "[[function_constant]]"
            resource_binding_attributes = @("[[buffer]]", "[[texture]]", "[[sampler]]")
            stage_attributes = @("[[vertex]]", "[[fragment]]", "[[kernel]]")
            msl_source_id = $expectedMslSourceId
        }
        visible_package = [ordered]@{
            ready = $true
            selected_3d_package = $true
            runtime_ui_package = $true
            environment_package = $true
            generated_game_package = $true
            visible_package_rows = $visiblePackageRows
            deterministic_hash_sha256 = $visiblePackageHash
        }
        native_handles = [ordered]@{
            ready = $true
            native_handles_exposed = $false
            objective_cxx_boundary_private = $true
        }
        cross_backend_inference = [ordered]@{
            ready = $true
            d3d12_inferred = $false
            vulkan_inferred = $false
        }
    }
    validation_counters = [ordered]@{
        renderer_apple_metal_xcode_tools_ready = $true
        renderer_apple_metal_msl_shader_ready = $true
        renderer_apple_metal_visible_package_ready = $true
    }
    non_claims = New-NonClaims
}

$artifactFull = Resolve-RepoRelativePath -RelativePath $artifactRelative `
    -Label "Apple Metal commercial quality host evidence artifact"
$willWrite = -not $NoWrite.IsPresent
if ($willWrite) {
    Write-JsonObject -Path $artifactFull -Value $evidence
}

$artifactHash = ""
if (Test-Path -LiteralPath $artifactFull -PathType Leaf) {
    $artifactHash = (Get-FileHash -LiteralPath $artifactFull -Algorithm SHA256).Hash.ToLowerInvariant()
}

Write-Output "validation_recipe=renderer-apple-metal-commercial-quality-host-evidence"
Write-Output "renderer_apple_metal_commercial_quality_host_evidence_generator_mode=Generate"
Write-Output "renderer_apple_metal_commercial_quality_host_evidence_status=ready"
Write-Output "renderer_apple_metal_commercial_quality_host_evidence_output_root=$OutputRootRelative"
Write-Output "renderer_apple_metal_commercial_quality_host_evidence_path=$artifactRelative"
Write-Output "renderer_apple_metal_commercial_quality_host_evidence_hash=$artifactHash"
Write-Output "renderer_apple_metal_commercial_quality_host_evidence_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_apple_metal_commercial_quality_host_evidence_written=$(ConvertTo-CounterBit ($willWrite -and (Test-Path -LiteralPath $artifactFull -PathType Leaf)))"
Write-Output "renderer_apple_metal_commercial_quality_host_evidence_ready=1"
Write-Output "renderer_apple_metal_commercial_quality_host_evidence_source_id=$expectedHostSourceId"
Write-Output "renderer_apple_metal_xcode_tools_ready=1"
Write-Output "renderer_apple_metal_msl_shader_ready=1"
Write-Output "renderer_apple_metal_visible_package_ready=1"
Write-Output "renderer_apple_metal_native_handles_exposed=0"
Write-Output "renderer_apple_metal_cross_backend_inference=0"
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"

Write-Information "renderer-apple-metal-commercial-quality-host-evidence-generator: ok" -InformationAction Continue
