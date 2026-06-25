#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10C

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Assemble")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/apple-metal-host",

    [string]$AppleMetalHostEvidenceRelative = "",

    [string]$MetalMemoryProfilingHostEvidenceRelative = "",

    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$expectedAppleHostSourceId = "Apple-Metal-Commercial-Host-Bridge-2026-06-25"
$artifactRelative = "$OutputRootRelative/apple-metal-host.json"

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

function Test-AllowedAppleHostEvidencePath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    return $normalizedPath.StartsWith(
        "artifacts/renderer/commercial-readiness-evidence/",
        [System.StringComparison]::Ordinal) -or
        $normalizedPath.StartsWith(
            "artifacts/renderer/apple-metal-commercial-quality-host-evidence/",
            [System.StringComparison]::Ordinal)
}

function Test-AllowedMetalMemoryEvidencePath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    return $normalizedPath.StartsWith(
        "artifacts/renderer/commercial-readiness-evidence/",
        [System.StringComparison]::Ordinal) -or
        $normalizedPath.StartsWith(
            "artifacts/renderer/metal-memory-profiling-host-evidence/",
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

function Assert-StringNotEmpty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $actual = [string](Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name)
    if ([string]::IsNullOrWhiteSpace($actual)) {
        Write-Error "$Label expected non-empty $Name."
    }
}

function Assert-StringArrayContains {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string[]]$ExpectedValues,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $values = @(Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name)
    foreach ($expected in $ExpectedValues) {
        if ($values -cnotcontains $expected) {
            Write-Error "$Label expected $Name to contain $expected."
        }
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

function Assert-ValidationCounter {
    param(
        [Parameter(Mandatory = $true)]$Counters,
        [Parameter(Mandatory = $true)][string]$Name
    )

    Assert-TrueProperty -JsonObject $Counters -Name $Name -Label "validation_counters"
}

function Test-AppleMetalHostEvidence {
    param([Parameter(Mandatory = $true)]$Evidence)

    Assert-ExactJsonProperties -JsonObject $Evidence -Label "Apple Metal host evidence" -ExpectedNames @(
        "schema_version",
        "claim_id",
        "validation_recipe",
        "fixture_only",
        "source_id",
        "source_rows",
        "proof_rows",
        "validation_counters",
        "non_claims"
    )
    Assert-StringProperty -JsonObject $Evidence -Name "schema_version" `
        -Expected "GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1" `
        -Label "Apple Metal host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "claim_id" `
        -Expected "renderer-apple-metal-commercial-quality-artifact-v1" `
        -Label "Apple Metal host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "validation_recipe" `
        -Expected "renderer-metal-apple-host-evidence" `
        -Label "Apple Metal host evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "source_id" `
        -Expected $expectedAppleHostSourceId `
        -Label "Apple Metal host evidence"

    $fixtureOnly = Get-JsonPropertyValue -JsonObject $Evidence -Name "fixture_only"
    if ($fixtureOnly -isnot [bool]) {
        Write-Error "Apple Metal host evidence expected fixture_only to be boolean."
    }
    if ([bool]$fixtureOnly) {
        Write-Error "fixture_artifact_input_rejected: $AppleMetalHostEvidenceRelative"
    }

    $sourceRows = Get-JsonPropertyValue -JsonObject $Evidence -Name "source_rows"
    Assert-ExactJsonProperties -JsonObject $sourceRows -Label "source_rows" -ExpectedNames @(
        "renderer_host_validation_recipe_id",
        "environment_aggregate_validation_recipe_id",
        "visible_package_evidence_status",
        "visible_package_evidence_ready",
        "visible_package_broad_claims"
    )
    Assert-StringProperty -JsonObject $sourceRows -Name "renderer_host_validation_recipe_id" `
        -Expected "renderer-metal-apple-host-evidence" `
        -Label "source_rows"
    Assert-StringProperty -JsonObject $sourceRows -Name "environment_aggregate_validation_recipe_id" `
        -Expected "renderer-metal-environment-aggregate-apple-host-evidence" `
        -Label "source_rows"
    Assert-StringProperty -JsonObject $sourceRows -Name "visible_package_evidence_status" `
        -Expected "ready" `
        -Label "source_rows"
    Assert-TrueProperty -JsonObject $sourceRows -Name "visible_package_evidence_ready" `
        -Label "source_rows"
    Assert-FalseProperty -JsonObject $sourceRows -Name "visible_package_broad_claims" `
        -Label "source_rows"

    $proofRows = Get-JsonPropertyValue -JsonObject $Evidence -Name "proof_rows"
    Assert-ExactJsonProperties -JsonObject $proofRows -Label "proof_rows" -ExpectedNames @(
        "host_toolchain",
        "msl_shader",
        "visible_package",
        "native_handles",
        "cross_backend_inference"
    )

    $hostToolchain = Get-JsonPropertyValue -JsonObject $proofRows -Name "host_toolchain"
    Assert-ExactJsonProperties -JsonObject $hostToolchain -Label "host_toolchain" -ExpectedNames @(
        "ready",
        "xcode_host_ready",
        "metal_tool_ready",
        "metallib_tool_ready",
        "command_line_metal_tools",
        "toolchain_source_id"
    )
    foreach ($requiredTrue in @(
            "ready",
            "xcode_host_ready",
            "metal_tool_ready",
            "metallib_tool_ready",
            "command_line_metal_tools"
        )) {
        Assert-TrueProperty -JsonObject $hostToolchain -Name $requiredTrue -Label "host_toolchain"
    }
    Assert-StringProperty -JsonObject $hostToolchain -Name "toolchain_source_id" `
        -Expected "Apple-Building-Shader-Library-Precompiling-Source-Files-2026-06-25" `
        -Label "host_toolchain"

    $mslShader = Get-JsonPropertyValue -JsonObject $proofRows -Name "msl_shader"
    Assert-ExactJsonProperties -JsonObject $mslShader -Label "msl_shader" -ExpectedNames @(
        "ready",
        "address_spaces",
        "function_constant_attribute",
        "resource_binding_attributes",
        "stage_attributes",
        "msl_source_id"
    )
    Assert-TrueProperty -JsonObject $mslShader -Name "ready" -Label "msl_shader"
    Assert-StringArrayContains -JsonObject $mslShader -Name "address_spaces" `
        -ExpectedValues @("device", "constant", "threadgroup") `
        -Label "msl_shader"
    Assert-StringProperty -JsonObject $mslShader -Name "function_constant_attribute" `
        -Expected "[[function_constant]]" `
        -Label "msl_shader"
    Assert-StringArrayContains -JsonObject $mslShader -Name "resource_binding_attributes" `
        -ExpectedValues @("[[buffer]]", "[[texture]]", "[[sampler]]") `
        -Label "msl_shader"
    Assert-StringArrayContains -JsonObject $mslShader -Name "stage_attributes" `
        -ExpectedValues @("[[vertex]]", "[[fragment]]", "[[kernel]]") `
        -Label "msl_shader"
    Assert-StringProperty -JsonObject $mslShader -Name "msl_source_id" `
        -Expected "Apple-Metal-Shading-Language-Specification-2026-06-25" `
        -Label "msl_shader"

    $visiblePackage = Get-JsonPropertyValue -JsonObject $proofRows -Name "visible_package"
    Assert-ExactJsonProperties -JsonObject $visiblePackage -Label "visible_package" -ExpectedNames @(
        "ready",
        "selected_3d_package",
        "runtime_ui_package",
        "environment_package",
        "generated_game_package",
        "visible_package_rows",
        "deterministic_hash_sha256"
    )
    foreach ($requiredTrue in @(
            "ready",
            "selected_3d_package",
            "runtime_ui_package",
            "environment_package",
            "generated_game_package"
        )) {
        Assert-TrueProperty -JsonObject $visiblePackage -Name $requiredTrue -Label "visible_package"
    }
    Assert-IntegerAtLeast -JsonObject $visiblePackage -Name "visible_package_rows" -Minimum 4 `
        -Label "visible_package"
    Assert-LowerHexSha256 -JsonObject $visiblePackage -Name "deterministic_hash_sha256" `
        -Label "visible_package"

    $nativeHandles = Get-JsonPropertyValue -JsonObject $proofRows -Name "native_handles"
    Assert-ExactJsonProperties -JsonObject $nativeHandles -Label "native_handles" -ExpectedNames @(
        "ready",
        "native_handles_exposed",
        "objective_cxx_boundary_private"
    )
    Assert-TrueProperty -JsonObject $nativeHandles -Name "ready" -Label "native_handles"
    Assert-FalseProperty -JsonObject $nativeHandles -Name "native_handles_exposed" -Label "native_handles"
    Assert-TrueProperty -JsonObject $nativeHandles -Name "objective_cxx_boundary_private" `
        -Label "native_handles"

    $crossBackendInference = Get-JsonPropertyValue -JsonObject $proofRows -Name "cross_backend_inference"
    Assert-ExactJsonProperties -JsonObject $crossBackendInference -Label "cross_backend_inference" `
        -ExpectedNames @(
            "ready",
            "d3d12_inferred",
            "vulkan_inferred"
        )
    Assert-TrueProperty -JsonObject $crossBackendInference -Name "ready" -Label "cross_backend_inference"
    foreach ($requiredFalse in @("d3d12_inferred", "vulkan_inferred")) {
        Assert-FalseProperty -JsonObject $crossBackendInference -Name $requiredFalse `
            -Label "cross_backend_inference"
    }

    $validationCounters = Get-JsonPropertyValue -JsonObject $Evidence -Name "validation_counters"
    Assert-ExactJsonProperties -JsonObject $validationCounters -Label "validation_counters" -ExpectedNames @(
        "renderer_apple_metal_xcode_tools_ready",
        "renderer_apple_metal_msl_shader_ready",
        "renderer_apple_metal_visible_package_ready"
    )
    foreach ($counter in @(
            "renderer_apple_metal_xcode_tools_ready",
            "renderer_apple_metal_msl_shader_ready",
            "renderer_apple_metal_visible_package_ready"
        )) {
        Assert-ValidationCounter -Counters $validationCounters -Name $counter
    }

    $nonClaims = Get-JsonPropertyValue -JsonObject $Evidence -Name "non_claims"
    Assert-ExactJsonProperties -JsonObject $nonClaims -Label "non_claims" -ExpectedNames @(
        "d3d12_inferred",
        "vulkan_inferred",
        "environment_ready",
        "external_engine_parity",
        "native_handles_exposed",
        "metal_objects_public"
    )
    foreach ($requiredFalse in @(
            "d3d12_inferred",
            "vulkan_inferred",
            "environment_ready",
            "external_engine_parity",
            "native_handles_exposed",
            "metal_objects_public"
        )) {
        Assert-FalseProperty -JsonObject $nonClaims -Name $requiredFalse -Label "non_claims"
    }
}

function Test-MetalMemoryProfilingEvidence {
    param([Parameter(Mandatory = $true)]$Evidence)

    Assert-ExactJsonProperties -JsonObject $Evidence -Label "Metal memory/profiling evidence" -ExpectedNames @(
        "schema_version",
        "claim_id",
        "host",
        "source_rows",
        "memory_residency_row",
        "profiling_capture_row",
        "non_claims"
    )
    Assert-StringProperty -JsonObject $Evidence -Name "schema_version" `
        -Expected "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1" `
        -Label "Metal memory/profiling evidence"
    Assert-StringProperty -JsonObject $Evidence -Name "claim_id" `
        -Expected "renderer-metal-memory-profiling-host-evidence-v1" `
        -Label "Metal memory/profiling evidence"

    $hostInfo = Get-JsonPropertyValue -JsonObject $Evidence -Name "host"
    Assert-ExactJsonProperties -JsonObject $hostInfo -Label "host" -ExpectedNames @(
        "platform",
        "macos_version",
        "xcode_version",
        "full_xcode_selected",
        "metal_tool_ready",
        "metallib_tool_ready"
    )
    Assert-StringProperty -JsonObject $hostInfo -Name "platform" -Expected "macos" -Label "host"
    foreach ($requiredTrue in @("full_xcode_selected", "metal_tool_ready", "metallib_tool_ready")) {
        Assert-TrueProperty -JsonObject $hostInfo -Name $requiredTrue -Label "host"
    }

    $sourceRows = Get-JsonPropertyValue -JsonObject $Evidence -Name "source_rows"
    Assert-ExactJsonProperties -JsonObject $sourceRows -Label "source_rows" -ExpectedNames @(
        "heap_documentation_source_id",
        "residency_set_documentation_source_id",
        "residency_request_documentation_source_id",
        "command_queue_residency_documentation_source_id",
        "capture_manager_documentation_source_id",
        "programmatic_capture_documentation_source_id"
    )
    Assert-StringProperty -JsonObject $sourceRows -Name "heap_documentation_source_id" `
        -Expected "Apple-Metal-MTLHeap-2026-06-24" `
        -Label "source_rows"
    Assert-StringProperty -JsonObject $sourceRows -Name "residency_set_documentation_source_id" `
        -Expected "Apple-Metal-MTLResidencySet-2026-06-24" `
        -Label "source_rows"
    Assert-StringProperty -JsonObject $sourceRows -Name "capture_manager_documentation_source_id" `
        -Expected "Apple-Metal-MTLCaptureManager-2026-06-24" `
        -Label "source_rows"

    $memoryResidency = Get-JsonPropertyValue -JsonObject $Evidence -Name "memory_residency_row"
    Assert-ExactJsonProperties -JsonObject $memoryResidency -Label "memory_residency_row" -ExpectedNames @(
        "proof_row_id",
        "host_validation_recipe_id",
        "first_party_workload_id",
        "runtime_ready",
        "command_queue_ready",
        "heap_api_name",
        "heap_allocation_ready",
        "heap_resource_allocation_ready",
        "heap_resource_rows",
        "heap_allocated_bytes",
        "resident_bytes",
        "budget_bytes",
        "residency_api_name",
        "residency_set_ready",
        "residency_set_allocation_rows",
        "residency_request_ready",
        "residency_commit_ready",
        "command_queue_residency_set_committed",
        "residency_pressure_evidence_ready",
        "memory_pressure_sample_rows",
        "memory_pressure_budget_status"
    )
    Assert-StringProperty -JsonObject $memoryResidency -Name "proof_row_id" `
        -Expected "memory_residency" `
        -Label "memory_residency_row"
    Assert-StringProperty -JsonObject $memoryResidency -Name "host_validation_recipe_id" `
        -Expected "renderer-metal-memory-profiling-host-evidence" `
        -Label "memory_residency_row"
    foreach ($requiredTrue in @(
            "runtime_ready",
            "command_queue_ready",
            "heap_allocation_ready",
            "heap_resource_allocation_ready",
            "residency_set_ready",
            "residency_request_ready",
            "residency_commit_ready",
            "command_queue_residency_set_committed",
            "residency_pressure_evidence_ready"
        )) {
        Assert-TrueProperty -JsonObject $memoryResidency -Name $requiredTrue `
            -Label "memory_residency_row"
    }
    Assert-StringProperty -JsonObject $memoryResidency -Name "heap_api_name" `
        -Expected "MTLHeap" `
        -Label "memory_residency_row"
    Assert-StringProperty -JsonObject $memoryResidency -Name "residency_api_name" `
        -Expected "MTLResidencySet" `
        -Label "memory_residency_row"
    foreach ($positiveRow in @(
            "heap_resource_rows",
            "heap_allocated_bytes",
            "resident_bytes",
            "budget_bytes",
            "residency_set_allocation_rows",
            "memory_pressure_sample_rows"
        )) {
        Assert-IntegerAtLeast -JsonObject $memoryResidency -Name $positiveRow -Minimum 1 `
            -Label "memory_residency_row"
    }
    $budgetBytes = [long](Get-JsonPropertyValue -JsonObject $memoryResidency -Name "budget_bytes")
    $residentBytes = [long](Get-JsonPropertyValue -JsonObject $memoryResidency -Name "resident_bytes")
    if ($budgetBytes -lt $residentBytes) {
        Write-Error "memory_residency_row expected budget_bytes >= resident_bytes."
    }

    $profilingCapture = Get-JsonPropertyValue -JsonObject $Evidence -Name "profiling_capture_row"
    Assert-ExactJsonProperties -JsonObject $profilingCapture -Label "profiling_capture_row" -ExpectedNames @(
        "proof_row_id",
        "host_validation_recipe_id",
        "first_party_workload_id",
        "runtime_ready",
        "command_queue_ready",
        "capture_api_name",
        "capture_manager_ready",
        "capture_descriptor_ready",
        "capture_object_ready",
        "capture_scope_ready",
        "capture_scope_label",
        "capture_boundary_ready",
        "capture_started",
        "capture_stopped",
        "command_buffer_captured",
        "capture_artifact_path",
        "capture_artifact_hash_sha256",
        "deterministic_capture_hash_sha256",
        "capture_artifact_rows"
    )
    Assert-StringProperty -JsonObject $profilingCapture -Name "proof_row_id" `
        -Expected "profiling_capture" `
        -Label "profiling_capture_row"
    Assert-StringProperty -JsonObject $profilingCapture -Name "host_validation_recipe_id" `
        -Expected "renderer-metal-memory-profiling-host-evidence" `
        -Label "profiling_capture_row"
    foreach ($requiredTrue in @(
            "runtime_ready",
            "command_queue_ready",
            "capture_manager_ready",
            "capture_descriptor_ready",
            "capture_object_ready",
            "capture_scope_ready",
            "capture_boundary_ready",
            "capture_started",
            "capture_stopped",
            "command_buffer_captured"
        )) {
        Assert-TrueProperty -JsonObject $profilingCapture -Name $requiredTrue `
            -Label "profiling_capture_row"
    }
    Assert-StringProperty -JsonObject $profilingCapture -Name "capture_api_name" `
        -Expected "MTLCaptureManager" `
        -Label "profiling_capture_row"
    Assert-StringNotEmpty -JsonObject $profilingCapture -Name "capture_scope_label" `
        -Label "profiling_capture_row"
    Assert-LowerHexSha256 -JsonObject $profilingCapture -Name "capture_artifact_hash_sha256" `
        -Label "profiling_capture_row"
    Assert-LowerHexSha256 -JsonObject $profilingCapture -Name "deterministic_capture_hash_sha256" `
        -Label "profiling_capture_row"
    Assert-IntegerAtLeast -JsonObject $profilingCapture -Name "capture_artifact_rows" -Minimum 1 `
        -Label "profiling_capture_row"

    $nonClaims = Get-JsonPropertyValue -JsonObject $Evidence -Name "non_claims"
    Assert-ExactJsonProperties -JsonObject $nonClaims -Label "non_claims" -ExpectedNames @(
        "simulator_only_evidence",
        "cross_backend_inference",
        "native_handles_exposed",
        "broad_backend_parity_ready",
        "broad_metal_readiness",
        "commercial_renderer_readiness",
        "broad_renderer_quality",
        "environment_ready",
        "external_engine_api_parity"
    )
    foreach ($requiredFalse in @(
            "simulator_only_evidence",
            "cross_backend_inference",
            "native_handles_exposed",
            "broad_backend_parity_ready",
            "broad_metal_readiness",
            "commercial_renderer_readiness",
            "broad_renderer_quality",
            "environment_ready",
            "external_engine_api_parity"
        )) {
        Assert-FalseProperty -JsonObject $nonClaims -Name $requiredFalse -Label "non_claims"
    }
}

function New-AppleMetalQualityArtifact {
    param(
        [Parameter(Mandatory = $true)]$AppleEvidence,
        [Parameter(Mandatory = $true)]$MetalMemoryEvidence
    )

    $appleProofRows = Get-JsonPropertyValue -JsonObject $AppleEvidence -Name "proof_rows"
    $memoryResidency = Get-JsonPropertyValue -JsonObject $MetalMemoryEvidence -Name "memory_residency_row"
    $profilingCapture = Get-JsonPropertyValue -JsonObject $MetalMemoryEvidence -Name "profiling_capture_row"

    return [ordered]@{
        schema_version = "GameEngine.RendererCommercialQualityCloseout.v1"
        artifact_id = "apple-metal-host"
        validation_recipe = "renderer-metal-apple-host-evidence"
        fixture_only = $false
        ready = $true
        proof_rows = [ordered]@{
            host_toolchain = Get-JsonPropertyValue -JsonObject $appleProofRows -Name "host_toolchain"
            msl_shader = Get-JsonPropertyValue -JsonObject $appleProofRows -Name "msl_shader"
            heap_resources = [ordered]@{
                ready = $true
                api_name = "MTLHeap"
                heap_descriptor_configured = $true
                heap_backed_resources_created = $true
                budgeted_size_bytes = [long](Get-JsonPropertyValue -JsonObject $memoryResidency `
                    -Name "budget_bytes")
            }
            residency_set = [ordered]@{
                ready = $true
                api_name = "MTLResidencySet"
                residency_set_created = $true
                resources_added = $true
                residency_commit_recorded = $true
            }
            capture_manager = [ordered]@{
                ready = $true
                api_name = "MTLCaptureManager"
                capture_scope = [string](Get-JsonPropertyValue -JsonObject $profilingCapture `
                    -Name "capture_scope_label")
                capture_started = $true
                capture_stopped = $true
                capture_artifact_sha256 = [string](Get-JsonPropertyValue -JsonObject $profilingCapture `
                    -Name "deterministic_capture_hash_sha256")
            }
            visible_package = Get-JsonPropertyValue -JsonObject $appleProofRows -Name "visible_package"
            native_handles = Get-JsonPropertyValue -JsonObject $appleProofRows -Name "native_handles"
            cross_backend_inference = Get-JsonPropertyValue -JsonObject $appleProofRows `
                -Name "cross_backend_inference"
        }
        validation_counters = [ordered]@{
            renderer_apple_metal_xcode_tools_ready = $true
            renderer_apple_metal_msl_shader_ready = $true
            renderer_apple_metal_heap_ready = $true
            renderer_apple_metal_residency_set_ready = $true
            renderer_apple_metal_capture_ready = $true
            renderer_apple_metal_visible_package_ready = $true
        }
        non_claims = Get-JsonPropertyValue -JsonObject $AppleEvidence -Name "non_claims"
    }
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/commercial-readiness-evidence/."
}

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=renderer-apple-metal-commercial-quality-artifact"
    Write-Output "renderer_apple_metal_commercial_quality_artifact_collector_mode=Plan"
    Write-Output "renderer_apple_metal_commercial_quality_artifact_output_root=$OutputRootRelative"
    Write-Output "renderer_apple_metal_commercial_quality_artifact_writes_evidence=0"
    Write-Output "renderer_apple_metal_commercial_quality_artifact_written=0"
    Write-Output "renderer_apple_metal_commercial_quality_fixture_artifact=0"
    Write-Output "renderer_apple_metal_commercial_quality_host_gated=1"
    Write-Output "renderer_apple_metal_xcode_tools_ready=0"
    Write-Output "renderer_apple_metal_msl_shader_ready=0"
    Write-Output "renderer_apple_metal_heap_ready=0"
    Write-Output "renderer_apple_metal_residency_set_ready=0"
    Write-Output "renderer_apple_metal_capture_ready=0"
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

if ([string]::IsNullOrWhiteSpace($AppleMetalHostEvidenceRelative)) {
    Write-Error "apple_metal_host_evidence_required"
}
if ([string]::IsNullOrWhiteSpace($MetalMemoryProfilingHostEvidenceRelative)) {
    Write-Error "metal_memory_profiling_host_evidence_required"
}
if (-not (Test-SafeRepoRelativePath -RelativePath $AppleMetalHostEvidenceRelative)) {
    Write-Error "unsafe_relative_path: AppleMetalHostEvidenceRelative must be a safe repo-relative path."
}
if (-not (Test-SafeRepoRelativePath -RelativePath $MetalMemoryProfilingHostEvidenceRelative)) {
    Write-Error "unsafe_relative_path: MetalMemoryProfilingHostEvidenceRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedAppleHostEvidencePath -RelativePath $AppleMetalHostEvidenceRelative)) {
    Write-Error "AppleMetalHostEvidenceRelative must be under artifacts/renderer/commercial-readiness-evidence/ or artifacts/renderer/apple-metal-commercial-quality-host-evidence/."
}
if (-not (Test-AllowedMetalMemoryEvidencePath -RelativePath $MetalMemoryProfilingHostEvidenceRelative)) {
    Write-Error "MetalMemoryProfilingHostEvidenceRelative must be under artifacts/renderer/commercial-readiness-evidence/ or artifacts/renderer/metal-memory-profiling-host-evidence/."
}

$appleHostEvidenceFull = Resolve-RepoRelativePath `
    -RelativePath $AppleMetalHostEvidenceRelative `
    -Label "AppleMetalHostEvidenceRelative"
$metalMemoryEvidenceFull = Resolve-RepoRelativePath `
    -RelativePath $MetalMemoryProfilingHostEvidenceRelative `
    -Label "MetalMemoryProfilingHostEvidenceRelative"
$artifactFull = Resolve-RepoRelativePath -RelativePath $artifactRelative -Label "apple-metal-host artifact"
$outputRootFull = Resolve-RepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
$appleHostEvidence = Read-JsonFile -Path $appleHostEvidenceFull -Label "AppleMetalHostEvidenceRelative"
$metalMemoryEvidence = Read-JsonFile -Path $metalMemoryEvidenceFull `
    -Label "MetalMemoryProfilingHostEvidenceRelative"
Test-AppleMetalHostEvidence -Evidence $appleHostEvidence
Test-MetalMemoryProfilingEvidence -Evidence $metalMemoryEvidence
$artifact = New-AppleMetalQualityArtifact `
    -AppleEvidence $appleHostEvidence `
    -MetalMemoryEvidence $metalMemoryEvidence
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

Write-Output "validation_recipe=renderer-apple-metal-commercial-quality-artifact"
Write-Output "renderer_apple_metal_commercial_quality_artifact_collector_mode=Assemble"
Write-Output "renderer_apple_metal_commercial_quality_artifact_output_root=$OutputRootRelative"
Write-Output "renderer_apple_metal_commercial_quality_artifact_path=$artifactRelative"
Write-Output "renderer_apple_metal_commercial_quality_artifact_hash=$artifactHash"
Write-Output "renderer_apple_metal_commercial_quality_artifact_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_apple_metal_commercial_quality_artifact_written=$(ConvertTo-CounterBit ($willWrite -and (Test-Path -LiteralPath $artifactFull -PathType Leaf)))"
Write-Output "renderer_apple_metal_commercial_quality_fixture_artifact=0"
Write-Output "renderer_apple_metal_commercial_quality_host_gated=0"
Write-Output "renderer_apple_metal_commercial_quality_source_id=$expectedAppleHostSourceId"
Write-Output "renderer_apple_metal_xcode_tools_ready=1"
Write-Output "renderer_apple_metal_msl_shader_ready=1"
Write-Output "renderer_apple_metal_heap_ready=1"
Write-Output "renderer_apple_metal_residency_set_ready=1"
Write-Output "renderer_apple_metal_capture_ready=1"
Write-Output "renderer_apple_metal_visible_package_ready=1"
Write-Output "renderer_apple_metal_native_handles_exposed=0"
Write-Output "renderer_apple_metal_cross_backend_inference=0"
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"

Write-Information "renderer-apple-metal-commercial-quality-artifact-collector: ok" -InformationAction Continue
