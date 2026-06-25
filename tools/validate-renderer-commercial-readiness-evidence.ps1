#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [switch]$RequireReady,
    [string]$ArtifactRootRelative = "",
    [string[]]$ExpectedEvidenceCounters = @(),
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AdditionalExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$ExpectedEvidenceCounters = @($ExpectedEvidenceCounters) + @($AdditionalExpectedEvidenceCounters)

$root = Get-RepoRoot
Set-Location $root

# Mirrors the value-only plan_renderer_commercial_readiness_promotion gate by consuming retained artifacts.
# Retained rows come from renderer-commercial-quality-closeout and tools/check-renderer-metal-memory-profiling-host-evidence.ps1 output.
# Fixture roots emit fixture_only=1 and cannot be mistaken for retained commercial host artifacts.
function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Add-RendererReadinessDiagnostic {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    if (-not $Diagnostics.Contains($Name)) {
        $Diagnostics.Add($Name) | Out-Null
    }
}

function Join-RendererReadinessDiagnosticValue {
    param([string[]]$Value = @())

    $usableValue = @($Value | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
    if ($usableValue.Count -eq 0) {
        return "none"
    }

    return ($usableValue -join "+")
}

function Test-LowerHexSha256Text {
    param([AllowNull()][string]$Value)

    return -not [string]::IsNullOrWhiteSpace($Value) -and $Value -cmatch "^[0-9a-f]{64}$"
}

function Get-JsonPropertyValue {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
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
        [Parameter(Mandatory = $true)][string]$Label,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    if ($null -eq $JsonObject) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "missing_$Label"
        return
    }

    $actualNames = @($JsonObject.PSObject.Properties.Name)
    foreach ($expected in $ExpectedNames) {
        if ($actualNames -notcontains $expected) {
            Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "missing_${Label}_$expected"
        }
    }
    foreach ($actual in $actualNames) {
        if ($ExpectedNames -notcontains $actual) {
            Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "unexpected_${Label}_$actual"
        }
    }
}

function Get-RequiredStringProperty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "missing_$Name"
        return ""
    }
    return [string]$value
}

function Test-RequiredTrueProperty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    if ($true -ne [bool]$value) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "missing_$Name"
        return $false
    }
    return $true
}

function Test-RequiredFalseProperty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    if ($false -ne [bool]$value) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "forbidden_$Name"
        return $false
    }
    return $true
}

function Resolve-RendererCommercialReadinessArtifactRoot {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        return $null
    }
    $normalizedPath = $RelativePath.Replace("\", "/")
    if ([System.IO.Path]::IsPathRooted($RelativePath) -or $normalizedPath.Contains("..")) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "unsafe_artifact_root"
        return $null
    }
    if (-not ($normalizedPath.StartsWith("artifacts/renderer/commercial-readiness-evidence/",
                [System.StringComparison]::Ordinal) -or
            $normalizedPath -eq "artifacts/renderer/commercial-readiness-evidence" -or
            $normalizedPath.StartsWith("tests/fixtures/renderer/commercial-readiness-evidence/",
                [System.StringComparison]::Ordinal) -or
            $normalizedPath -eq "tests/fixtures/renderer/commercial-readiness-evidence")) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "unapproved_artifact_root"
        return $null
    }

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    $rootWithSeparator = $root.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "artifact_root_escape"
        return $null
    }
    return $fullPath
}

function Resolve-RendererCommercialReadinessEvidenceFile {
    param(
        [Parameter(Mandatory = $true)][string]$ArtifactRootFull,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    if (-not (Test-Path -LiteralPath $ArtifactRootFull -PathType Container)) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "artifact_root_missing"
        return $null
    }

    $directEvidence = Join-Path $ArtifactRootFull "evidence.json"
    if (Test-Path -LiteralPath $directEvidence -PathType Leaf) {
        return $directEvidence
    }

    $readyEvidence = Join-Path (Join-Path $ArtifactRootFull "ready") "evidence.json"
    if (Test-Path -LiteralPath $readyEvidence -PathType Leaf) {
        return $readyEvidence
    }

    Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "evidence_json_missing"
    return $null
}

function Resolve-RendererCommercialReadinessArtifactPath {
    param(
        [Parameter(Mandatory = $true)][string]$ArtifactRootFull,
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    $normalizedPath = $RelativePath.Replace("\", "/")
    if ([System.IO.Path]::IsPathRooted($RelativePath) -or $normalizedPath.Contains("..") -or
        $normalizedPath.Contains(":")) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "unsafe_artifact_path"
        return $null
    }

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    $artifactRootWithSeparator = $ArtifactRootFull.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not ($fullPath -eq $ArtifactRootFull -or
            $fullPath.StartsWith($artifactRootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase))) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "artifact_path_escape"
        return $null
    }
    return $fullPath
}

function Assert-SourceRow {
    param(
        [Parameter(Mandatory = $true)]$SourceRows,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    $actual = Get-RequiredStringProperty -JsonObject $SourceRows -Name $Name -Diagnostics $Diagnostics
    if ($actual -cne $Expected) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "invalid_$Name"
    }
}

function Read-RendererCommercialReadinessJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    try {
        return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    } catch {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "invalid_json"
        return $null
    }
}

function Test-RendererReadinessFixtureContracts {
    param(
        [Parameter(Mandatory = $true)][string]$FixtureRoot,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    if (-not (Test-Path -LiteralPath $FixtureRoot -PathType Container)) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "fixture_root_missing"
        return 0
    }

    $fixtureRows = 0
    foreach ($fixture in @(Get-ChildItem -LiteralPath $FixtureRoot -Recurse -File -Filter "evidence.json")) {
        $fixtureRows += 1
        $fixtureDiagnostics = [System.Collections.Generic.List[string]]::new()
        $fixtureJson = Read-RendererCommercialReadinessJson -Path $fixture.FullName -Diagnostics $fixtureDiagnostics
        if ($null -eq $fixtureJson) {
            Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "fixture_invalid_json"
            continue
        }
        if ((Get-RequiredStringProperty -JsonObject $fixtureJson -Name "schema_version" `
                    -Diagnostics $fixtureDiagnostics) -cne "GameEngine.RendererCommercialReadinessEvidence.v1") {
            Add-RendererReadinessDiagnostic -Diagnostics $fixtureDiagnostics -Name "invalid_fixture_schema_version"
        }
        if ((Get-RequiredStringProperty -JsonObject $fixtureJson -Name "claim_id" `
                    -Diagnostics $fixtureDiagnostics) -cne "renderer-commercial-readiness-evidence-promotion-v1") {
            Add-RendererReadinessDiagnostic -Diagnostics $fixtureDiagnostics -Name "invalid_fixture_claim_id"
        }
        if ($fixtureDiagnostics.Count -ne 0) {
            Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "fixture_contract_invalid"
        }
    }
    return $fixtureRows
}

function ConvertFrom-RendererReadinessKeyValueLines {
    param([string[]]$Lines = @())

    $values = @{}
    foreach ($line in @($Lines)) {
        $text = [string]$line
        $separator = $text.IndexOf("=")
        if ($separator -le 0) {
            continue
        }
        $key = $text.Substring(0, $separator)
        $value = $text.Substring($separator + 1)
        $values[$key] = $value
    }
    return $values
}

function Test-RendererCommercialReadinessD3d12Artifact {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$D3d12ProofRows,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    $artifactDiagnostics = [System.Collections.Generic.List[string]]::new()
    $artifact = Read-RendererCommercialReadinessJson -Path $Path -Diagnostics $artifactDiagnostics
    if ($null -eq $artifact) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "invalid_d3d12_artifact_json"
        return $false
    }

    Assert-ExactJsonProperties -JsonObject $artifact -Label "d3d12_artifact" -Diagnostics $artifactDiagnostics `
        -ExpectedNames @(
            "schema_version",
            "artifact_id",
            "validation_recipe",
            "fixture_only",
            "ready",
            "proof_rows",
            "validation_counters",
            "non_claims"
        )

    if ((Get-RequiredStringProperty -JsonObject $artifact -Name "schema_version" `
                -Diagnostics $artifactDiagnostics) -cne "GameEngine.RendererCommercialQualityCloseout.v1") {
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_d3d12_artifact_schema"
    }
    if ((Get-RequiredStringProperty -JsonObject $artifact -Name "artifact_id" `
                -Diagnostics $artifactDiagnostics) -cne "d3d12-quality") {
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_d3d12_artifact_id"
    }
    if ((Get-RequiredStringProperty -JsonObject $artifact -Name "validation_recipe" `
                -Diagnostics $artifactDiagnostics) -cne "renderer-d3d12-quality-evidence") {
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_d3d12_artifact_recipe"
    }
    $null = Test-RequiredTrueProperty -JsonObject $artifact -Name "ready" -Diagnostics $artifactDiagnostics

    $proofRows = Get-JsonPropertyValue -JsonObject $artifact -Name "proof_rows"
    $commandAllocatorListFence = Get-JsonPropertyValue -JsonObject $proofRows -Name "command_allocator_list_fence"
    $resourceBarriers = Get-JsonPropertyValue -JsonObject $proofRows -Name "resource_barriers"
    $timestamp = Get-JsonPropertyValue -JsonObject $proofRows -Name "timestamp"
    $debugValidation = Get-JsonPropertyValue -JsonObject $proofRows -Name "debug_validation"
    $residency = Get-JsonPropertyValue -JsonObject $proofRows -Name "residency"
    $packageVisibleReadback = Get-JsonPropertyValue -JsonObject $proofRows -Name "package_visible_readback"
    $nativeHandles = Get-JsonPropertyValue -JsonObject $proofRows -Name "native_handles"

    $commandAllocatorFenceReady =
        (Test-RequiredTrueProperty -JsonObject $commandAllocatorListFence -Name "ready" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $commandAllocatorListFence -Name "command_allocator_reuse_fenced" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $commandAllocatorListFence -Name "command_list_closed_before_execute" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $commandAllocatorListFence -Name "fence_signal_wait_recorded" `
            -Diagnostics $artifactDiagnostics) -and
        ((Get-RequiredStringProperty -JsonObject $commandAllocatorListFence -Name "fence_api_name" `
                -Diagnostics $artifactDiagnostics) -cne "")
    if ((Get-RequiredStringProperty -JsonObject $commandAllocatorListFence -Name "fence_api_name" `
                -Diagnostics $artifactDiagnostics) -cne "ID3D12Fence") {
        $commandAllocatorFenceReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_d3d12_fence_api"
    }
    $D3d12ProofRows["command_allocator_fence"] = $commandAllocatorFenceReady

    $resourceBarrierReady =
        (Test-RequiredTrueProperty -JsonObject $resourceBarriers -Name "ready" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $resourceBarriers -Name "render_transition_explicit" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $resourceBarriers -Name "copy_transition_explicit" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $resourceBarriers -Name "unordered_access_barrier_explicit" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $resourceBarriers -Name "readback_transition_explicit" `
            -Diagnostics $artifactDiagnostics)
    if ((Get-RequiredStringProperty -JsonObject $resourceBarriers -Name "resource_barrier_api_name" `
                -Diagnostics $artifactDiagnostics) -cne "D3D12_RESOURCE_BARRIER") {
        $resourceBarrierReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_d3d12_resource_barrier_api"
    }
    $D3d12ProofRows["resource_barrier"] = $resourceBarrierReady

    $timestampReady =
        (Test-RequiredTrueProperty -JsonObject $timestamp -Name "ready" -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $timestamp -Name "resolved_query_data" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $timestamp -Name "clock_calibration" `
            -Diagnostics $artifactDiagnostics)
    if ((Get-RequiredStringProperty -JsonObject $timestamp -Name "query_type" `
                -Diagnostics $artifactDiagnostics) -cne "D3D12_QUERY_TYPE_TIMESTAMP") {
        $timestampReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_d3d12_timestamp_query"
    }
    $queueFrequency = Get-JsonPropertyValue -JsonObject $timestamp -Name "queue_frequency_hz"
    if ($null -eq $queueFrequency -or [long]$queueFrequency -le 0) {
        $timestampReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "missing_d3d12_queue_frequency"
    }
    $D3d12ProofRows["timestamp"] = $timestampReady

    $debugValidationReady =
        (Test-RequiredTrueProperty -JsonObject $debugValidation -Name "ready" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $debugValidation -Name "debug_layer_or_gpu_based_validation_clean" `
            -Diagnostics $artifactDiagnostics)
    foreach ($zeroField in @("debug_message_count", "gpu_based_validation_message_count")) {
        $value = Get-JsonPropertyValue -JsonObject $debugValidation -Name $zeroField
        if ($null -eq $value -or [long]$value -ne 0) {
            $debugValidationReady = $false
            Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "dirty_d3d12_$zeroField"
        }
    }
    $D3d12ProofRows["debug_validation"] = $debugValidationReady

    $residencyReady =
        (Test-RequiredTrueProperty -JsonObject $residency -Name "ready" -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $residency -Name "video_memory_budget_queried" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $residency -Name "make_resident_or_budget_recorded" `
            -Diagnostics $artifactDiagnostics)
    if ((Get-RequiredStringProperty -JsonObject $residency -Name "residency_api_name" `
                -Diagnostics $artifactDiagnostics) -cne "ID3D12Device3::EnqueueMakeResident") {
        $residencyReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_d3d12_residency_api"
    }
    if ((Get-RequiredStringProperty -JsonObject $residency -Name "budget_api_name" `
                -Diagnostics $artifactDiagnostics) -cne "IDXGIAdapter3::QueryVideoMemoryInfo") {
        $residencyReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_d3d12_budget_api"
    }
    $D3d12ProofRows["residency"] = $residencyReady

    $packageReadbackReady =
        (Test-RequiredTrueProperty -JsonObject $packageVisibleReadback -Name "ready" `
            -Diagnostics $artifactDiagnostics)
    $packageReadbackHash = Get-RequiredStringProperty -JsonObject $packageVisibleReadback `
        -Name "deterministic_hash_sha256" -Diagnostics $artifactDiagnostics
    if (-not (Test-LowerHexSha256Text -Value $packageReadbackHash)) {
        $packageReadbackReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_d3d12_package_readback_hash"
    }
    $readbackCounterRows = Get-JsonPropertyValue -JsonObject $packageVisibleReadback -Name "readback_counter_rows"
    if ($null -eq $readbackCounterRows -or [long]$readbackCounterRows -le 0) {
        $packageReadbackReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "missing_d3d12_package_readback_rows"
    }
    $D3d12ProofRows["package_readback"] = $packageReadbackReady

    $nativeHandleReady =
        (Test-RequiredTrueProperty -JsonObject $nativeHandles -Name "ready" -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredFalseProperty -JsonObject $nativeHandles -Name "native_handles_exposed" `
            -Diagnostics $artifactDiagnostics)
    $D3d12ProofRows["native_handles_exposed"] =
        [bool](Get-JsonPropertyValue -JsonObject $nativeHandles -Name "native_handles_exposed")
    $D3d12ProofRows["native_handles"] = $nativeHandleReady

    $nonClaims = Get-JsonPropertyValue -JsonObject $artifact -Name "non_claims"
    $nonClaimsReady = $true
    foreach ($requiredFalse in @(
            "vulkan_inferred",
            "metal_inferred",
            "broad_ui_parity",
            "environment_ready",
            "external_engine_parity",
            "native_handles_exposed"
        )) {
        $nonClaimsReady = (Test-RequiredFalseProperty -JsonObject $nonClaims -Name $requiredFalse `
                -Diagnostics $artifactDiagnostics) -and $nonClaimsReady
    }

    foreach ($artifactDiagnostic in $artifactDiagnostics) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name $artifactDiagnostic
    }

    return $commandAllocatorFenceReady -and $resourceBarrierReady -and $timestampReady -and
        $debugValidationReady -and $residencyReady -and $packageReadbackReady -and
        $nativeHandleReady -and $nonClaimsReady
}

function Test-RendererCommercialReadinessVulkanArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$VulkanProofRows,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    $artifactDiagnostics = [System.Collections.Generic.List[string]]::new()
    $artifact = Read-RendererCommercialReadinessJson -Path $Path -Diagnostics $artifactDiagnostics
    if ($null -eq $artifact) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "invalid_vulkan_artifact_json"
        return $false
    }

    Assert-ExactJsonProperties -JsonObject $artifact -Label "vulkan_artifact" -Diagnostics $artifactDiagnostics `
        -ExpectedNames @(
            "schema_version",
            "artifact_id",
            "validation_recipe",
            "fixture_only",
            "ready",
            "proof_rows",
            "validation_counters",
            "non_claims"
        )

    if ((Get-RequiredStringProperty -JsonObject $artifact -Name "schema_version" `
                -Diagnostics $artifactDiagnostics) -cne "GameEngine.RendererCommercialQualityCloseout.v1") {
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_vulkan_artifact_schema"
    }
    if ((Get-RequiredStringProperty -JsonObject $artifact -Name "artifact_id" `
                -Diagnostics $artifactDiagnostics) -cne "vulkan-strict-quality") {
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_vulkan_artifact_id"
    }
    if ((Get-RequiredStringProperty -JsonObject $artifact -Name "validation_recipe" `
                -Diagnostics $artifactDiagnostics) -cne "renderer-vulkan-strict-quality-evidence") {
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_vulkan_artifact_recipe"
    }
    $null = Test-RequiredTrueProperty -JsonObject $artifact -Name "ready" -Diagnostics $artifactDiagnostics

    $proofRows = Get-JsonPropertyValue -JsonObject $artifact -Name "proof_rows"
    $synchronization2 = Get-JsonPropertyValue -JsonObject $proofRows -Name "synchronization2"
    $validationLayer = Get-JsonPropertyValue -JsonObject $proofRows -Name "validation_layer"
    $synchronizationValidation = Get-JsonPropertyValue -JsonObject $proofRows -Name "synchronization_validation"
    $memoryBinding = Get-JsonPropertyValue -JsonObject $proofRows -Name "memory_binding"
    $timestampQuery = Get-JsonPropertyValue -JsonObject $proofRows -Name "timestamp_query"
    $spirvShaderValidation = Get-JsonPropertyValue -JsonObject $proofRows -Name "spirv_shader_validation"
    $packageVisibleReadback = Get-JsonPropertyValue -JsonObject $proofRows -Name "package_visible_readback"
    $nativeHandles = Get-JsonPropertyValue -JsonObject $proofRows -Name "native_handles"

    $synchronization2Ready =
        (Test-RequiredTrueProperty -JsonObject $synchronization2 -Name "ready" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $synchronization2 -Name "vk_cmd_pipeline_barrier2_recorded" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $synchronization2 -Name "vk_dependency_info_recorded" `
            -Diagnostics $artifactDiagnostics)
    if ((Get-RequiredStringProperty -JsonObject $synchronization2 -Name "api_name" `
                -Diagnostics $artifactDiagnostics) -cne "vkCmdPipelineBarrier2") {
        $synchronization2Ready = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_vulkan_synchronization2_api"
    }
    if ((Get-RequiredStringProperty -JsonObject $synchronization2 -Name "structure_name" `
                -Diagnostics $artifactDiagnostics) -cne "VkDependencyInfo") {
        $synchronization2Ready = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_vulkan_dependency_info"
    }
    $VulkanProofRows["synchronization2"] = $synchronization2Ready

    $validationLayerReady =
        (Test-RequiredTrueProperty -JsonObject $validationLayer -Name "ready" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $validationLayer -Name "validation_log_clean" `
            -Diagnostics $artifactDiagnostics)
    if ((Get-RequiredStringProperty -JsonObject $validationLayer -Name "layer_name" `
                -Diagnostics $artifactDiagnostics) -cne "VK_LAYER_KHRONOS_validation") {
        $validationLayerReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_vulkan_validation_layer"
    }
    if ([long](Get-JsonPropertyValue -JsonObject $validationLayer -Name "validation_error_count") -ne 0) {
        $validationLayerReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "dirty_vulkan_validation_log"
    }
    $VulkanProofRows["validation_layer"] = $validationLayerReady

    $syncValidationReady =
        (Test-RequiredTrueProperty -JsonObject $synchronizationValidation -Name "ready" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $synchronizationValidation -Name "sync_validation_enabled" `
            -Diagnostics $artifactDiagnostics)
    if ([long](Get-JsonPropertyValue -JsonObject $synchronizationValidation -Name "sync_validation_error_count") -ne 0) {
        $syncValidationReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "dirty_vulkan_sync_validation"
    }
    $VulkanProofRows["sync_validation"] = $syncValidationReady

    $memoryBindingReady =
        (Test-RequiredTrueProperty -JsonObject $memoryBinding -Name "ready" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $memoryBinding -Name "buffer_memory_bound" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $memoryBinding -Name "image_memory_bound" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $memoryBinding -Name "vuid_constraints_checked" `
            -Diagnostics $artifactDiagnostics)
    if (-not (Get-RequiredStringProperty -JsonObject $memoryBinding -Name "vuid_reference" `
                -Diagnostics $artifactDiagnostics).Contains("VUID")) {
        $memoryBindingReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "missing_vulkan_memory_binding_vuid"
    }
    $VulkanProofRows["memory_binding"] = $memoryBindingReady

    $timestampReady =
        (Test-RequiredTrueProperty -JsonObject $timestampQuery -Name "ready" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $timestampQuery -Name "query_pool_timestamp" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $timestampQuery -Name "timestamps_resolved" `
            -Diagnostics $artifactDiagnostics)
    if ([long](Get-JsonPropertyValue -JsonObject $timestampQuery -Name "timestamp_period_ns") -le 0) {
        $timestampReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "missing_vulkan_timestamp_period"
    }
    $VulkanProofRows["timestamp"] = $timestampReady

    $shaderValidationReady =
        (Test-RequiredTrueProperty -JsonObject $spirvShaderValidation -Name "ready" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $spirvShaderValidation -Name "spirv_val_ready" `
            -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredTrueProperty -JsonObject $spirvShaderValidation -Name "shader_modules_validated" `
            -Diagnostics $artifactDiagnostics)
    if ([long](Get-JsonPropertyValue -JsonObject $spirvShaderValidation -Name "validation_error_count") -ne 0) {
        $shaderValidationReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "dirty_vulkan_spirv_validation"
    }
    $VulkanProofRows["shader_validation"] = $shaderValidationReady

    $packageReadbackReady =
        (Test-RequiredTrueProperty -JsonObject $packageVisibleReadback -Name "ready" `
            -Diagnostics $artifactDiagnostics)
    $packageReadbackHash = Get-RequiredStringProperty -JsonObject $packageVisibleReadback `
        -Name "deterministic_hash_sha256" -Diagnostics $artifactDiagnostics
    if (-not (Test-LowerHexSha256Text -Value $packageReadbackHash)) {
        $packageReadbackReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "invalid_vulkan_package_readback_hash"
    }
    if ([long](Get-JsonPropertyValue -JsonObject $packageVisibleReadback -Name "readback_counter_rows") -le 0) {
        $packageReadbackReady = $false
        Add-RendererReadinessDiagnostic -Diagnostics $artifactDiagnostics -Name "missing_vulkan_package_readback_rows"
    }
    $VulkanProofRows["package_readback"] = $packageReadbackReady

    $nativeHandleReady =
        (Test-RequiredTrueProperty -JsonObject $nativeHandles -Name "ready" -Diagnostics $artifactDiagnostics) -and
        (Test-RequiredFalseProperty -JsonObject $nativeHandles -Name "native_handles_exposed" `
            -Diagnostics $artifactDiagnostics)
    $VulkanProofRows["native_handles_exposed"] =
        [bool](Get-JsonPropertyValue -JsonObject $nativeHandles -Name "native_handles_exposed")
    $VulkanProofRows["native_handles"] = $nativeHandleReady

    $nonClaims = Get-JsonPropertyValue -JsonObject $artifact -Name "non_claims"
    $nonClaimsReady = $true
    foreach ($requiredFalse in @(
            "d3d12_inferred",
            "metal_inferred",
            "debugging_only_full_pipeline_barrier",
            "environment_ready",
            "external_engine_parity",
            "native_handles_exposed"
        )) {
        $nonClaimsReady = (Test-RequiredFalseProperty -JsonObject $nonClaims -Name $requiredFalse `
                -Diagnostics $artifactDiagnostics) -and $nonClaimsReady
    }

    foreach ($artifactDiagnostic in $artifactDiagnostics) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name $artifactDiagnostic
    }

    return $synchronization2Ready -and $validationLayerReady -and $syncValidationReady -and
        $memoryBindingReady -and $timestampReady -and $shaderValidationReady -and
        $packageReadbackReady -and $nativeHandleReady -and $nonClaimsReady
}

function Invoke-RendererCommercialQualityCloseoutFromEvidence {
    param(
        [Parameter(Mandatory = $true)]$RowReady,
        [Parameter(Mandatory = $true)][bool]$CleanRoomReady,
        [Parameter(Mandatory = $true)][bool]$ThirdPartyNoticeReady,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    $pwsh = Find-CommandOnCombinedPath "pwsh"
    if (-not $pwsh) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "pwsh_missing"
        return @{}
    }

    $scriptPath = Join-Path $root "tools/validate-renderer-commercial-quality-closeout.ps1"
    $arguments = @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $scriptPath,
        "-RequireReady",
        "-BackendParityReady"
    )

    $rowSwitches = @(
        @{ Name = "d3d12"; SwitchName = "-D3d12Ready" },
        @{ Name = "vulkan_strict"; SwitchName = "-VulkanStrictReady" },
        @{ Name = "apple_metal"; SwitchName = "-AppleMetalReady" },
        @{ Name = "renderer_quality_matrix"; SwitchName = "-RendererQualityMatrixReady" },
        @{ Name = "production_vfx_profiling"; SwitchName = "-ProductionVfxProfilingReady" },
        @{ Name = "metal_memory_profiling"; SwitchName = "-MetalMemoryProfilingReady" },
        @{ Name = "visible_3d"; SwitchName = "-Visible3dPackageReady" },
        @{ Name = "runtime_ui"; SwitchName = "-RuntimeUiPackageReady" },
        @{ Name = "environment"; SwitchName = "-EnvironmentPackageReady" },
        @{ Name = "generated_game"; SwitchName = "-GeneratedGamePackageReady" }
    )

    foreach ($rowSwitch in $rowSwitches) {
        if ([bool]$RowReady[$rowSwitch.Name]) {
            $arguments += $rowSwitch.SwitchName
        }
    }
    $arguments += "-StaticGuardsReady"
    if (-not $CleanRoomReady) {
        $arguments += "-CleanRoomSourceReviewMissing"
    }
    if (-not $ThirdPartyNoticeReady) {
        $arguments += "-ThirdPartyNoticesIncomplete"
    }

    $closeoutOutput = @(& $pwsh @arguments 2>&1)
    if ($LASTEXITCODE -ne 0) {
        Add-RendererReadinessDiagnostic -Diagnostics $Diagnostics -Name "renderer-commercial-quality-closeout_failed"
    }
    return ConvertFrom-RendererReadinessKeyValueLines -Lines $closeoutOutput
}

$diagnostics = [System.Collections.Generic.List[string]]::new()
$schemaPath = Join-Path $root "schemas/renderer-commercial-readiness-evidence.schema.json"
if (-not (Test-Path -LiteralPath $schemaPath -PathType Leaf)) {
    Write-Error "Missing renderer commercial readiness evidence schema: $schemaPath"
}

$schemaText = Get-Content -LiteralPath $schemaPath -Raw
foreach ($needle in @(
        "GameEngine.RendererCommercialReadinessEvidence.v1",
        "renderer-commercial-readiness-evidence-promotion-v1",
        "Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25",
        "Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25",
        "Apple-Metal-Framework-Memory-Capture-2026-06-25",
        "Apple-Metal-Shading-Language-Specification-2026-06-25",
        "Unity-Legal-Terms-2026-06-25",
        "Epic-Unreal-Engine-EULA-Trademark-2026-06-25",
        "Godot-Trademark-Licensing-2026-06-25",
        "renderer_backend_parity_ready",
        "renderer_metal_broad_readiness",
        "renderer_broad_quality_ready",
        "renderer_commercial_readiness"
    )) {
    if (-not $schemaText.Contains($needle)) {
        Write-Error "Renderer commercial readiness evidence schema must contain '$needle'."
    }
}

$fixtureRoot = Join-Path $root "tests/fixtures/renderer/commercial-readiness-evidence"
$fixtureRows = Test-RendererReadinessFixtureContracts -FixtureRoot $fixtureRoot -Diagnostics $diagnostics

$artifactRootProvided = -not [string]::IsNullOrWhiteSpace($ArtifactRootRelative)
$artifactRootFull = $null
if ($artifactRootProvided) {
    $artifactRootFull = Resolve-RendererCommercialReadinessArtifactRoot `
        -RelativePath $ArtifactRootRelative `
        -Diagnostics $diagnostics
}
$fixtureOnly = $artifactRootProvided -and
    $ArtifactRootRelative.Replace("\", "/").StartsWith("tests/fixtures/renderer/commercial-readiness-evidence",
        [System.StringComparison]::Ordinal)

if ($RequireReady.IsPresent -and -not $artifactRootProvided) {
    Add-RendererReadinessDiagnostic -Diagnostics $diagnostics -Name "require_ready_without_artifact_root"
}

$evidenceFile = $null
if ($null -ne $artifactRootFull) {
    $evidenceFile = Resolve-RendererCommercialReadinessEvidenceFile `
        -ArtifactRootFull $artifactRootFull `
        -Diagnostics $diagnostics
}

$rowReady = @{
    d3d12 = $false
    vulkan_strict = $false
    apple_metal = $false
    visible_3d = $false
    runtime_ui = $false
    environment = $false
    generated_game = $false
    renderer_quality_matrix = $false
    production_vfx_profiling = $false
    memory_residency = $false
    profiling_capture = $false
    metal_memory_profiling = $false
}
$d3d12ProofRows = @{
    command_allocator_fence = $false
    resource_barrier = $false
    timestamp = $false
    debug_validation = $false
    residency = $false
    package_readback = $false
    native_handles = $false
    native_handles_exposed = $false
}
$vulkanProofRows = @{
    synchronization2 = $false
    validation_layer = $false
    sync_validation = $false
    memory_binding = $false
    timestamp = $false
    shader_validation = $false
    package_readback = $false
    native_handles = $false
    native_handles_exposed = $false
}

$artifactRows = 0
$readyRows = 0
$invalidRows = 0
$missingArtifactRows = 0
$hashMismatchRows = 0
$forbiddenMaterialRejectedRows = 0
$externalEngineDetectedRows = 0
$sourceRowCount = 0
$closeoutValues = @{}

if ($null -ne $evidenceFile) {
    $evidenceDiagnostics = [System.Collections.Generic.List[string]]::new()
    $evidence = Read-RendererCommercialReadinessJson -Path $evidenceFile -Diagnostics $evidenceDiagnostics
    if ($null -ne $evidence) {
        Assert-ExactJsonProperties -JsonObject $evidence -Label "evidence" -Diagnostics $evidenceDiagnostics `
            -ExpectedNames @(
                "schema_version",
                "claim_id",
                "source_rows",
                "backend_rows",
                "package_rows",
                "quality_rows",
                "metal_memory_profiling_rows",
                "clean_room_rows",
                "expected_counters",
                "non_claims"
            )

        if ((Get-RequiredStringProperty -JsonObject $evidence -Name "schema_version" `
                    -Diagnostics $evidenceDiagnostics) -cne "GameEngine.RendererCommercialReadinessEvidence.v1") {
            Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name "invalid_schema_version"
        }
        if ((Get-RequiredStringProperty -JsonObject $evidence -Name "claim_id" `
                    -Diagnostics $evidenceDiagnostics) -cne "renderer-commercial-readiness-evidence-promotion-v1") {
            Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name "invalid_claim_id"
        }

        $sourceRows = Get-JsonPropertyValue -JsonObject $evidence -Name "source_rows"
        if ($null -eq $sourceRows) {
            Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name "missing_source_rows"
        } else {
            $sourceRowCount = @($sourceRows.PSObject.Properties.Name).Count
            Assert-SourceRow -SourceRows $sourceRows -Name "d3d12_documentation_source_id" `
                -Expected "Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25" `
                -Diagnostics $evidenceDiagnostics
            Assert-SourceRow -SourceRows $sourceRows -Name "vulkan_documentation_source_id" `
                -Expected "Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25" `
                -Diagnostics $evidenceDiagnostics
            Assert-SourceRow -SourceRows $sourceRows -Name "metal_framework_source_id" `
                -Expected "Apple-Metal-Framework-Memory-Capture-2026-06-25" `
                -Diagnostics $evidenceDiagnostics
            Assert-SourceRow -SourceRows $sourceRows -Name "metal_shading_language_source_id" `
                -Expected "Apple-Metal-Shading-Language-Specification-2026-06-25" `
                -Diagnostics $evidenceDiagnostics
            Assert-SourceRow -SourceRows $sourceRows -Name "unity_legal_source_id" `
                -Expected "Unity-Legal-Terms-2026-06-25" -Diagnostics $evidenceDiagnostics
            Assert-SourceRow -SourceRows $sourceRows -Name "unreal_legal_source_id" `
                -Expected "Epic-Unreal-Engine-EULA-Trademark-2026-06-25" -Diagnostics $evidenceDiagnostics
            Assert-SourceRow -SourceRows $sourceRows -Name "godot_legal_source_id" `
                -Expected "Godot-Trademark-Licensing-2026-06-25" -Diagnostics $evidenceDiagnostics
            Assert-SourceRow -SourceRows $sourceRows -Name "json_schema_source_id" `
                -Expected "JSON-Schema-Draft-2020-12-2026-06-25" -Diagnostics $evidenceDiagnostics
            Assert-SourceRow -SourceRows $sourceRows -Name "context7_review_source_id" `
                -Expected "Context7-Renderer-Commercial-Readiness-Docs-2026-06-25" `
                -Diagnostics $evidenceDiagnostics
        }

        $rowSpecs = @(
            @{ Group = "backend_rows"; Name = "d3d12"; EvidenceId = "d3d12"; Counter = "renderer_d3d12_renderer_quality_ready"; Recipe = "renderer-d3d12-quality-evidence"; Missing = "d3d12_renderer_quality_evidence_required" },
            @{ Group = "backend_rows"; Name = "vulkan_strict"; EvidenceId = "vulkan_strict"; Counter = "renderer_vulkan_strict_renderer_quality_ready"; Recipe = "renderer-vulkan-strict-quality-evidence"; Missing = "vulkan_strict_renderer_quality_evidence_required" },
            @{ Group = "backend_rows"; Name = "apple_metal"; EvidenceId = "apple_metal"; Counter = "renderer_apple_metal_renderer_quality_ready"; Recipe = "renderer-metal-apple-host-evidence"; Missing = "apple_metal_renderer_quality_evidence_required" },
            @{ Group = "package_rows"; Name = "visible_3d"; EvidenceId = "visible_3d"; Counter = "renderer_visible_3d_package_ready"; Recipe = "desktop-3d-package"; Missing = "visible_3d_package_evidence_required" },
            @{ Group = "package_rows"; Name = "runtime_ui"; EvidenceId = "runtime_ui"; Counter = "renderer_runtime_ui_package_ready"; Recipe = "desktop-runtime-ui-package"; Missing = "runtime_ui_package_evidence_required" },
            @{ Group = "package_rows"; Name = "environment"; EvidenceId = "environment"; Counter = "renderer_environment_package_ready"; Recipe = "environment-package"; Missing = "environment_package_evidence_required" },
            @{ Group = "package_rows"; Name = "generated_game"; EvidenceId = "generated_game"; Counter = "renderer_generated_game_package_ready"; Recipe = "generated-game-package"; Missing = "generated_game_package_evidence_required" },
            @{ Group = "quality_rows"; Name = "renderer_quality_matrix"; EvidenceId = "renderer_quality_matrix"; Counter = "renderer_quality_matrix_ready"; Recipe = "renderer-quality-matrix"; Missing = "renderer_quality_matrix_evidence_required" },
            @{ Group = "quality_rows"; Name = "production_vfx_profiling"; EvidenceId = "production_vfx_profiling"; Counter = "renderer_production_vfx_profiling_ready"; Recipe = "renderer-production-vfx-profiling"; Missing = "production_vfx_profiling_evidence_required" },
            @{ Group = "metal_memory_profiling_rows"; Name = "memory_residency"; EvidenceId = "memory_residency"; Counter = "renderer_metal_memory_profiling_ready"; Recipe = "renderer-metal-memory-profiling-host-evidence"; Missing = "metal_memory_residency_evidence_required" },
            @{ Group = "metal_memory_profiling_rows"; Name = "profiling_capture"; EvidenceId = "profiling_capture"; Counter = "renderer_metal_memory_profiling_ready"; Recipe = "renderer-metal-memory-profiling-host-evidence"; Missing = "metal_profiling_capture_evidence_required" }
        )

        foreach ($rowSpec in $rowSpecs) {
            $group = Get-JsonPropertyValue -JsonObject $evidence -Name $rowSpec.Group
            $row = Get-JsonPropertyValue -JsonObject $group -Name $rowSpec.Name
            if ($null -eq $row) {
                Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name $rowSpec.Missing
                $invalidRows += 1
                continue
            }

            $artifactRows += 1
            Assert-ExactJsonProperties -JsonObject $row -Label $rowSpec.Name -Diagnostics $evidenceDiagnostics `
                -ExpectedNames @(
                    "evidence_id",
                    "selected",
                    "ready",
                    "host_validation_recipe_id",
                    "retained_artifact_schema_version",
                    "artifact_path",
                    "artifact_hash_sha256",
                    "validation_counter_id",
                    "diagnostic_code"
                )

            if ((Get-RequiredStringProperty -JsonObject $row -Name "evidence_id" `
                        -Diagnostics $evidenceDiagnostics) -cne $rowSpec.EvidenceId) {
                Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name "invalid_$($rowSpec.Name)_evidence_id"
            }
            $null = Test-RequiredTrueProperty -JsonObject $row -Name "selected" -Diagnostics $evidenceDiagnostics
            $rowIsReady = [bool](Get-JsonPropertyValue -JsonObject $row -Name "ready")
            if ((Get-RequiredStringProperty -JsonObject $row -Name "host_validation_recipe_id" `
                        -Diagnostics $evidenceDiagnostics) -cne $rowSpec.Recipe) {
                Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name "invalid_$($rowSpec.Name)_recipe"
            }
            if ((Get-RequiredStringProperty -JsonObject $row -Name "validation_counter_id" `
                        -Diagnostics $evidenceDiagnostics) -cne $rowSpec.Counter) {
                Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name "invalid_$($rowSpec.Name)_counter"
            }

            $artifactPath = Get-RequiredStringProperty -JsonObject $row -Name "artifact_path" `
                -Diagnostics $evidenceDiagnostics
            $artifactHash = Get-RequiredStringProperty -JsonObject $row -Name "artifact_hash_sha256" `
                -Diagnostics $evidenceDiagnostics
            if (-not (Test-LowerHexSha256Text -Value $artifactHash)) {
                Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name "invalid_$($rowSpec.Name)_hash"
            }

            $resolvedArtifactPath = $null
            if (-not [string]::IsNullOrWhiteSpace($artifactPath)) {
                $resolvedArtifactPath = Resolve-RendererCommercialReadinessArtifactPath `
                    -ArtifactRootFull $artifactRootFull `
                    -RelativePath $artifactPath `
                    -Diagnostics $evidenceDiagnostics
                if ($null -eq $resolvedArtifactPath -or
                    -not (Test-Path -LiteralPath $resolvedArtifactPath -PathType Leaf)) {
                    $missingArtifactRows += 1
                    Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name "missing_artifact"
                } else {
                    $actualHash = (Get-FileHash -LiteralPath $resolvedArtifactPath -Algorithm SHA256).Hash.ToLowerInvariant()
                    if ($actualHash -cne $artifactHash) {
                        $hashMismatchRows += 1
                        Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name "artifact_hash_mismatch"
                    }
                }
            }

            $artifactSpecificReady = $true
            if ($rowSpec.Name -eq "d3d12") {
                if ($null -eq $resolvedArtifactPath -or
                    -not (Test-Path -LiteralPath $resolvedArtifactPath -PathType Leaf)) {
                    $artifactSpecificReady = $false
                    Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics `
                        -Name "missing_d3d12_quality_artifact"
                } else {
                    $artifactSpecificReady = Test-RendererCommercialReadinessD3d12Artifact `
                        -Path $resolvedArtifactPath `
                        -D3d12ProofRows $d3d12ProofRows `
                        -Diagnostics $evidenceDiagnostics
                }
            }
            if ($rowSpec.Name -eq "vulkan_strict") {
                if ($null -eq $resolvedArtifactPath -or
                    -not (Test-Path -LiteralPath $resolvedArtifactPath -PathType Leaf)) {
                    $artifactSpecificReady = $false
                    Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics `
                        -Name "missing_vulkan_strict_quality_artifact"
                } else {
                    $artifactSpecificReady = Test-RendererCommercialReadinessVulkanArtifact `
                        -Path $resolvedArtifactPath `
                        -VulkanProofRows $vulkanProofRows `
                        -Diagnostics $evidenceDiagnostics
                }
            }

            $diagnosticCode = Get-RequiredStringProperty -JsonObject $row -Name "diagnostic_code" `
                -Diagnostics $evidenceDiagnostics
            if ($rowIsReady -and $diagnosticCode -cne "ready") {
                Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name "ready_row_without_ready_diagnostic"
            }
            if ($rowIsReady -and $artifactSpecificReady) {
                $readyRows += 1
                $rowReady[$rowSpec.Name] = $true
            } else {
                Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name $rowSpec.Missing
            }
        }

        $rowReady["metal_memory_profiling"] =
            ([bool]$rowReady["memory_residency"] -and [bool]$rowReady["profiling_capture"])

        $cleanRoomRows = Get-JsonPropertyValue -JsonObject $evidence -Name "clean_room_rows"
        $officialDocs = Get-JsonPropertyValue -JsonObject $cleanRoomRows -Name "official_docs_only"
        $legalReview = Get-JsonPropertyValue -JsonObject $cleanRoomRows -Name "legal_review"
        $thirdPartyNotices = Get-JsonPropertyValue -JsonObject $cleanRoomRows -Name "third_party_notices"
        $forbiddenMaterialRows = @(Get-JsonPropertyValue -JsonObject $cleanRoomRows -Name "forbidden_material_rows")

        $officialDocsReady = $true
        foreach ($requiredTrue in @(
                "ready",
                "public_documentation_only",
                "context7_verified",
                "official_fallback_documented",
                "external_engine_source_review_complete"
            )) {
            $officialDocsReady = (Test-RequiredTrueProperty -JsonObject $officialDocs -Name $requiredTrue `
                    -Diagnostics $evidenceDiagnostics) -and $officialDocsReady
        }

        $legalReviewReady = $true
        foreach ($requiredTrue in @(
                "ready",
                "unity_terms_reviewed",
                "unreal_eula_trademark_reviewed",
                "godot_trademark_reviewed"
            )) {
            $legalReviewReady = (Test-RequiredTrueProperty -JsonObject $legalReview -Name $requiredTrue `
                    -Diagnostics $evidenceDiagnostics) -and $legalReviewReady
        }
        foreach ($requiredFalse in @(
                "unity_compatibility",
                "unreal_compatibility",
                "godot_compatibility",
                "compatibility_claims",
                "equivalence_claims",
                "parity_claims"
            )) {
            $legalReviewReady = (Test-RequiredFalseProperty -JsonObject $legalReview -Name $requiredFalse `
                    -Diagnostics $evidenceDiagnostics) -and $legalReviewReady
        }

        $thirdPartyNoticeReady = $true
        foreach ($requiredTrue in @("ready", "complete")) {
            $thirdPartyNoticeReady = (Test-RequiredTrueProperty -JsonObject $thirdPartyNotices -Name $requiredTrue `
                    -Diagnostics $evidenceDiagnostics) -and $thirdPartyNoticeReady
        }
        $noticesPath = Get-RequiredStringProperty -JsonObject $thirdPartyNotices -Name "notices_path" `
            -Diagnostics $evidenceDiagnostics
        if ($noticesPath -cne "THIRD_PARTY_NOTICES.md" -or
            -not (Test-Path -LiteralPath (Join-Path $root $noticesPath) -PathType Leaf)) {
            $thirdPartyNoticeReady = $false
            Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics -Name "third_party_notices_required"
        }

        foreach ($forbiddenRow in @($forbiddenMaterialRows)) {
            if ($null -eq $forbiddenRow) {
                continue
            }
            $detected = [bool](Get-JsonPropertyValue -JsonObject $forbiddenRow -Name "detected")
            $rejected = [bool](Get-JsonPropertyValue -JsonObject $forbiddenRow -Name "rejected")
            $usedInEngine = [bool](Get-JsonPropertyValue -JsonObject $forbiddenRow -Name "used_in_engine")
            if ($detected) {
                $externalEngineDetectedRows += 1
            }
            if ($detected -and $rejected -and -not $usedInEngine) {
                $forbiddenMaterialRejectedRows += 1
                Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics `
                    -Name "external_engine_material_rejected"
            } elseif ($detected -or $usedInEngine) {
                Add-RendererReadinessDiagnostic -Diagnostics $evidenceDiagnostics `
                    -Name "external_engine_material_used"
            }
        }

        $nonClaims = Get-JsonPropertyValue -JsonObject $evidence -Name "non_claims"
        $nonClaimsReady = $true
        foreach ($requiredFalse in @(
                "environment_ready",
                "native_handles_exposed",
                "cross_backend_inference",
                "external_engine_parity",
                "external_engine_code_used",
                "external_engine_sample_used",
                "external_engine_asset_used",
                "external_engine_trademark_used",
                "external_engine_ui_expression_used",
                "unity_compatibility",
                "unreal_compatibility",
                "godot_compatibility"
            )) {
            $nonClaimsReady = (Test-RequiredFalseProperty -JsonObject $nonClaims -Name $requiredFalse `
                    -Diagnostics $evidenceDiagnostics) -and $nonClaimsReady
        }

        $cleanRoomReady = $officialDocsReady -and $legalReviewReady -and $nonClaimsReady
        $closeoutValues = Invoke-RendererCommercialQualityCloseoutFromEvidence `
            -RowReady $rowReady `
            -CleanRoomReady $cleanRoomReady `
            -ThirdPartyNoticeReady $thirdPartyNoticeReady `
            -Diagnostics $evidenceDiagnostics

        if ($evidenceDiagnostics.Count -eq 0 -or
            ($evidenceDiagnostics.Count -eq 1 -and $forbiddenMaterialRejectedRows -gt 0)) {
            $invalidRows = $invalidRows
        } else {
            $invalidRows += 1
        }

        foreach ($diagnostic in $evidenceDiagnostics) {
            Add-RendererReadinessDiagnostic -Diagnostics $diagnostics -Name $diagnostic
        }
    }
}

$rendererBackendParityReady = ([string]$closeoutValues["renderer_backend_parity_ready"]) -eq "1"
$rendererMetalBroadReadiness = ([string]$closeoutValues["renderer_metal_broad_readiness"]) -eq "1"
$rendererBroadQualityReady = ([string]$closeoutValues["renderer_broad_quality_ready"]) -eq "1"
$qualityCloseoutReady = ([string]$closeoutValues["renderer_commercial_quality_closeout_ready"]) -eq "1"
$rendererCommercialReadiness = $qualityCloseoutReady -and $rendererBackendParityReady -and
    $rendererMetalBroadReadiness -and $rendererBroadQualityReady -and
    ($forbiddenMaterialRejectedRows -eq 0) -and ($externalEngineDetectedRows -eq 0) -and
    ($missingArtifactRows -eq 0) -and ($hashMismatchRows -eq 0) -and ($invalidRows -eq 0)

$evidenceReady = $rendererCommercialReadiness

if (-not $artifactRootProvided) {
    Add-RendererReadinessDiagnostic -Diagnostics $diagnostics -Name "artifact_root_required"
}

$status = if ($evidenceReady) {
    "ready"
} elseif ($RequireReady.IsPresent) {
    "blocked"
} else {
    "host_evidence_required"
}

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=renderer-commercial-readiness-evidence")
$lines.Add("artifact_root=$ArtifactRootRelative")
$lines.Add("fixture_only=$(ConvertTo-CounterBit $fixtureOnly)")
$lines.Add("renderer_commercial_readiness_value_api=plan_renderer_commercial_readiness_promotion")
$lines.Add("renderer_commercial_readiness_evidence_status=$status")
$lines.Add("renderer_commercial_readiness_evidence_ready=$(ConvertTo-CounterBit $evidenceReady)")
$lines.Add("renderer_commercial_readiness_evidence_require_ready=$(ConvertTo-CounterBit $RequireReady)")
$lines.Add("renderer_commercial_readiness_evidence_schema_ready=1")
$lines.Add("renderer_commercial_readiness_evidence_fixture_rows=$fixtureRows")
$lines.Add("renderer_commercial_readiness_evidence_source_rows=$sourceRowCount")
$lines.Add("renderer_commercial_readiness_evidence_artifact_rows=$artifactRows")
$lines.Add("renderer_commercial_readiness_evidence_ready_rows=$readyRows")
$lines.Add("renderer_commercial_readiness_evidence_invalid_rows=$invalidRows")
$lines.Add("renderer_commercial_readiness_evidence_missing_artifacts=$missingArtifactRows")
$lines.Add("renderer_commercial_readiness_evidence_hash_mismatches=$hashMismatchRows")
$lines.Add("renderer_external_engine_forbidden_material_detected_rows=$externalEngineDetectedRows")
$lines.Add("renderer_external_engine_forbidden_material_rejected_rows=$forbiddenMaterialRejectedRows")
$lines.Add("renderer_d3d12_command_allocator_fence_ready=$(ConvertTo-CounterBit $($d3d12ProofRows["command_allocator_fence"]))")
$lines.Add("renderer_d3d12_resource_barrier_ready=$(ConvertTo-CounterBit $($d3d12ProofRows["resource_barrier"]))")
$lines.Add("renderer_d3d12_timestamp_ready=$(ConvertTo-CounterBit $($d3d12ProofRows["timestamp"]))")
$lines.Add("renderer_d3d12_debug_validation_ready=$(ConvertTo-CounterBit $($d3d12ProofRows["debug_validation"]))")
$lines.Add("renderer_d3d12_residency_ready=$(ConvertTo-CounterBit $($d3d12ProofRows["residency"]))")
$lines.Add("renderer_d3d12_package_readback_ready=$(ConvertTo-CounterBit $($d3d12ProofRows["package_readback"]))")
$lines.Add("renderer_d3d12_native_handles_exposed=$(ConvertTo-CounterBit $($d3d12ProofRows["native_handles_exposed"]))")
$lines.Add("renderer_vulkan_synchronization2_ready=$(ConvertTo-CounterBit $($vulkanProofRows["synchronization2"]))")
$lines.Add("renderer_vulkan_validation_layer_ready=$(ConvertTo-CounterBit $($vulkanProofRows["validation_layer"]))")
$lines.Add("renderer_vulkan_sync_validation_ready=$(ConvertTo-CounterBit $($vulkanProofRows["sync_validation"]))")
$lines.Add("renderer_vulkan_memory_binding_ready=$(ConvertTo-CounterBit $($vulkanProofRows["memory_binding"]))")
$lines.Add("renderer_vulkan_timestamp_ready=$(ConvertTo-CounterBit $($vulkanProofRows["timestamp"]))")
$lines.Add("renderer_vulkan_shader_validation_ready=$(ConvertTo-CounterBit $($vulkanProofRows["shader_validation"]))")
$lines.Add("renderer_vulkan_package_readback_ready=$(ConvertTo-CounterBit $($vulkanProofRows["package_readback"]))")
$lines.Add("renderer_vulkan_native_handles_exposed=$(ConvertTo-CounterBit $($vulkanProofRows["native_handles_exposed"]))")
$lines.Add("renderer_d3d12_renderer_quality_ready=$(ConvertTo-CounterBit $($rowReady["d3d12"]))")
$lines.Add("renderer_vulkan_strict_renderer_quality_ready=$(ConvertTo-CounterBit $($rowReady["vulkan_strict"]))")
$lines.Add("renderer_apple_metal_renderer_quality_ready=$(ConvertTo-CounterBit $($rowReady["apple_metal"]))")
$lines.Add("renderer_quality_matrix_ready=$(ConvertTo-CounterBit $($rowReady["renderer_quality_matrix"]))")
$lines.Add("renderer_production_vfx_profiling_ready=$(ConvertTo-CounterBit $($rowReady["production_vfx_profiling"]))")
$lines.Add("renderer_metal_memory_profiling_ready=$(ConvertTo-CounterBit $($rowReady["metal_memory_profiling"]))")
$lines.Add("renderer_visible_3d_package_ready=$(ConvertTo-CounterBit $($rowReady["visible_3d"]))")
$lines.Add("renderer_runtime_ui_package_ready=$(ConvertTo-CounterBit $($rowReady["runtime_ui"]))")
$lines.Add("renderer_environment_package_ready=$(ConvertTo-CounterBit $($rowReady["environment"]))")
$lines.Add("renderer_generated_game_package_ready=$(ConvertTo-CounterBit $($rowReady["generated_game"]))")
$lines.Add("renderer_static_guards_ready=1")
$lines.Add("renderer_commercial_quality_closeout_status=$([string]$closeoutValues["renderer_commercial_quality_closeout_status"])")
$lines.Add("renderer_commercial_quality_closeout_ready=$(ConvertTo-CounterBit $qualityCloseoutReady)")
$lines.Add("renderer_backend_parity_ready=$(ConvertTo-CounterBit $rendererBackendParityReady)")
$lines.Add("renderer_metal_broad_readiness=$(ConvertTo-CounterBit $rendererMetalBroadReadiness)")
$lines.Add("renderer_broad_quality_ready=$(ConvertTo-CounterBit $rendererBroadQualityReady)")
$lines.Add("renderer_commercial_readiness=$(ConvertTo-CounterBit $rendererCommercialReadiness)")
$lines.Add("renderer_environment_ready=0")
$lines.Add("renderer_commercial_readiness_evidence_blocker=$(Join-RendererReadinessDiagnosticValue $diagnostics.ToArray())")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $evidenceReady) {
    Write-Error "Renderer commercial readiness evidence is not ready: $($diagnostics -join ', ')"
}

Write-Information "renderer-commercial-readiness-evidence: ok" -InformationAction Continue
