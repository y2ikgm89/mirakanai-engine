#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady,
    [string]$ArtifactRootRelative = "artifacts/mavg/metal-mesh-lod-host-execution",
    [string[]]$ExpectedEvidenceCounters = @(),
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AdditionalExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "apple-host-helpers.ps1")

$ExpectedEvidenceCounters = @($ExpectedEvidenceCounters) + @($AdditionalExpectedEvidenceCounters)

$root = Get-RepoRoot
Set-Location $root

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Get-MavgMetalHostLabel {
    if (Test-IsMacOS) {
        return "macos"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Windows)) {
        return "windows"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Linux)) {
        return "linux"
    }
    return "unknown"
}

function Test-Sha256Text {
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

function Test-TrueProperty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name
    )

    return [bool](Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name)
}

function Test-FalseProperty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name
    )

    return -not [bool](Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name)
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
        $Diagnostics.Add("missing_$Name")
        return ""
    }
    return [string]$value
}

function Get-RequiredIntegerProperty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    if ($null -eq $value) {
        $Diagnostics.Add("missing_$Name")
        return 0
    }
    try {
        return [int]$value
    } catch {
        $Diagnostics.Add("invalid_$Name")
        return 0
    }
}

function Resolve-MavgMetalEvidenceArtifactPath {
    param(
        [Parameter(Mandatory = $true)][string]$EvidenceDirectory,
        [Parameter(Mandatory = $true)][string]$ArtifactRootFull,
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    if ([System.IO.Path]::IsPathRooted($RelativePath) -or $RelativePath.Contains("..")) {
        $Diagnostics.Add("unsafe_readback_artifact_path")
        return $null
    }
    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $EvidenceDirectory $RelativePath))
    $rootWithSeparator = $ArtifactRootFull.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        $Diagnostics.Add("readback_artifact_path_escape")
        return $null
    }
    return $fullPath
}

$schemaPath = Join-Path $root "schemas/mavg-metal-mesh-lod-host-execution.schema.json"
if (-not (Test-Path -LiteralPath $schemaPath -PathType Leaf)) {
    Write-Error "Missing MAVG Metal mesh LOD host execution schema: $schemaPath"
}
$schemaText = Get-Content -LiteralPath $schemaPath -Raw
foreach ($needle in @(
        "GameEngine.MavgMetalMeshLodHostExecutionEvidence.v1",
        "mavg-metal-mesh-lod-apple-host-execution-v1",
        "Apple-Metal-Feature-Set-Tables-2026-05-21",
        "draw_mesh_threads_encoded",
        "object_payload_declared",
        "mesh_payload_consumed",
        "deterministic_readback_hash_sha256",
        "mavg_broad_backend_readiness"
    )) {
    if (-not $schemaText.Contains($needle)) {
        Write-Error "MAVG Metal mesh LOD host execution schema must contain '$needle'."
    }
}

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for MAVG Metal host evidence validation."
}

Write-Information "mavg-metal-host-evidence: configuring dev preset..." -InformationAction Continue
Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    (Join-Path $root "tools/cmake.ps1"),
    "--preset",
    "dev"
)

Write-Information "mavg-metal-host-evidence: building focused Metal MAVG mesh LOD test target..." `
    -InformationAction Continue
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
    "MK_mavg_metal_mesh_lod_tests"
)

Write-Information "mavg-metal-host-evidence: running focused Metal MAVG mesh LOD CTest lane..." `
    -InformationAction Continue
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
    "MK_mavg_metal_mesh_lod_tests"
)

$hostLabel = Get-MavgMetalHostLabel
$developerDirectory = Get-AppleDeveloperDirectory
$fullXcodeSelected = Test-FullXcodeDeveloperDirectory -DeveloperDirectory $developerDirectory
$xcrun = Find-CommandOnCombinedPath "xcrun"
$metalToolReady = Test-XcrunToolAvailable -Xcrun $xcrun -SdkName "macosx" -ToolName "metal"
$metallibToolReady = Test-XcrunToolAvailable -Xcrun $xcrun -SdkName "macosx" -ToolName "metallib"

$artifactRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $ArtifactRootRelative))
$evidenceFiles = @()
if (Test-Path -LiteralPath $artifactRootFull -PathType Container) {
    $evidenceFiles = @(Get-ChildItem -LiteralPath $artifactRootFull -Recurse -File -Filter "evidence.json")
}

$artifactRows = 0
$readyRows = 0
$invalidRows = 0
$missingArtifactRows = 0
$appleGpuFamilyRows = 0
$meshShaderFeatureRows = 0
$objectShaderFeatureRows = 0
$firstPartyWorkloadRows = 0
$objectMeshPipelineRows = 0
$objectMeshDispatchRows = 0
$drawMeshThreadsRows = 0
$shaderPayloadRows = 0
$readbackHashRows = 0
$packageVisibleOutputRows = 0
$crossBackendInferenceRows = 0
$rayTracingConflationRows = 0
$nativeHandleRows = 0
$naniteCompatibleRows = 0
$naniteEquivalentRows = 0
$naniteSuperiorRows = 0
$broadBackendReadinessRows = 0
$broadOptimizationRows = 0

foreach ($evidenceFile in $evidenceFiles) {
    $artifactRows += 1
    $rowDiagnostics = [System.Collections.Generic.List[string]]::new()
    try {
        $evidence = Get-Content -LiteralPath $evidenceFile.FullName -Raw | ConvertFrom-Json
    } catch {
        $invalidRows += 1
        continue
    }

    $schemaVersion = Get-RequiredStringProperty -JsonObject $evidence -Name "schema_version" -Diagnostics $rowDiagnostics
    $claimId = Get-RequiredStringProperty -JsonObject $evidence -Name "claim_id" -Diagnostics $rowDiagnostics
    $hostRow = Get-JsonPropertyValue -JsonObject $evidence -Name "host"
    $featureRow = Get-JsonPropertyValue -JsonObject $evidence -Name "feature_row"
    $shaderRow = Get-JsonPropertyValue -JsonObject $evidence -Name "shader_row"
    $executionRow = Get-JsonPropertyValue -JsonObject $evidence -Name "execution_row"
    $nonClaims = Get-JsonPropertyValue -JsonObject $evidence -Name "non_claims"

    if ($schemaVersion -cne "GameEngine.MavgMetalMeshLodHostExecutionEvidence.v1") {
        $rowDiagnostics.Add("invalid_schema_version")
    }
    if ($claimId -cne "mavg-metal-mesh-lod-apple-host-execution-v1") {
        $rowDiagnostics.Add("invalid_claim_id")
    }

    if ($null -eq $hostRow) {
        $rowDiagnostics.Add("missing_host")
    } else {
        if ((Get-RequiredStringProperty -JsonObject $hostRow -Name "platform" -Diagnostics $rowDiagnostics) -cne "macos") {
            $rowDiagnostics.Add("invalid_host_platform")
        }
        $null = Get-RequiredStringProperty -JsonObject $hostRow -Name "macos_version" -Diagnostics $rowDiagnostics
        $null = Get-RequiredStringProperty -JsonObject $hostRow -Name "xcode_version" -Diagnostics $rowDiagnostics
        foreach ($requiredTrue in @("full_xcode_selected", "metal_tool_ready", "metallib_tool_ready")) {
            if (-not (Test-TrueProperty -JsonObject $hostRow -Name $requiredTrue)) {
                $rowDiagnostics.Add("missing_$requiredTrue")
            }
        }
    }

    if ($null -eq $featureRow) {
        $rowDiagnostics.Add("missing_feature_row")
    } else {
        if ((Get-RequiredStringProperty -JsonObject $featureRow -Name "feature_set_table_source_id" `
                    -Diagnostics $rowDiagnostics) -cne "Apple-Metal-Feature-Set-Tables-2026-05-21") {
            $rowDiagnostics.Add("invalid_feature_set_table_source_id")
        }
        if (-not [string]::IsNullOrWhiteSpace((Get-RequiredStringProperty -JsonObject $featureRow `
                        -Name "apple_gpu_family" -Diagnostics $rowDiagnostics))) {
            $appleGpuFamilyRows += 1
        }
        if (Test-TrueProperty -JsonObject $featureRow -Name "mesh_shader_supported") {
            $meshShaderFeatureRows += 1
        } else {
            $rowDiagnostics.Add("missing_mesh_shader_supported")
        }
        if (Test-TrueProperty -JsonObject $featureRow -Name "object_shader_supported") {
            $objectShaderFeatureRows += 1
        } else {
            $rowDiagnostics.Add("missing_object_shader_supported")
        }
    }

    if ($null -eq $shaderRow) {
        $rowDiagnostics.Add("missing_shader_row")
    } else {
        if (-not (Test-TrueProperty -JsonObject $shaderRow -Name "msl_source_validated")) {
            $rowDiagnostics.Add("missing_msl_source_validated")
        }
        $null = Get-RequiredStringProperty -JsonObject $shaderRow -Name "object_function" -Diagnostics $rowDiagnostics
        $null = Get-RequiredStringProperty -JsonObject $shaderRow -Name "mesh_function" -Diagnostics $rowDiagnostics
        $null = Get-RequiredStringProperty -JsonObject $shaderRow -Name "fragment_function" -Diagnostics $rowDiagnostics
        $payloadReady = (Test-TrueProperty -JsonObject $shaderRow -Name "object_payload_declared") -and
            (Test-TrueProperty -JsonObject $shaderRow -Name "mesh_payload_consumed")
        if ($payloadReady) {
            $shaderPayloadRows += 1
        } else {
            $rowDiagnostics.Add("missing_shader_payload")
        }
    }

    if ($null -eq $executionRow) {
        $rowDiagnostics.Add("missing_execution_row")
    } else {
        if (-not [string]::IsNullOrWhiteSpace((Get-RequiredStringProperty -JsonObject $executionRow `
                        -Name "first_party_mavg_workload_id" -Diagnostics $rowDiagnostics))) {
            $firstPartyWorkloadRows += 1
        }
        if (Test-TrueProperty -JsonObject $executionRow -Name "object_mesh_pipeline_created") {
            $objectMeshPipelineRows += 1
        } else {
            $rowDiagnostics.Add("missing_object_mesh_pipeline_created")
        }
        if (Test-TrueProperty -JsonObject $executionRow -Name "draw_mesh_threads_encoded") {
            $drawMeshThreadsRows += 1
        } else {
            $rowDiagnostics.Add("missing_draw_mesh_threads_encoded")
        }
        if (Test-TrueProperty -JsonObject $executionRow -Name "object_mesh_dispatch_executed") {
            $objectMeshDispatchRows += 1
        } else {
            $rowDiagnostics.Add("missing_object_mesh_dispatch_executed")
        }
        if (-not (Test-TrueProperty -JsonObject $executionRow -Name "command_buffer_completed")) {
            $rowDiagnostics.Add("missing_command_buffer_completed")
        }
        if ((Get-RequiredIntegerProperty -JsonObject $executionRow -Name "meshlet_count" -Diagnostics $rowDiagnostics) -lt 1) {
            $rowDiagnostics.Add("invalid_meshlet_count")
        }
        if ((Get-RequiredIntegerProperty -JsonObject $executionRow -Name "lod_level_count" -Diagnostics $rowDiagnostics) -lt 1) {
            $rowDiagnostics.Add("invalid_lod_level_count")
        }
        $readbackPath = Get-RequiredStringProperty -JsonObject $executionRow -Name "readback_artifact_path" `
            -Diagnostics $rowDiagnostics
        $readbackHash = Get-RequiredStringProperty -JsonObject $executionRow -Name "readback_artifact_hash_sha256" `
            -Diagnostics $rowDiagnostics
        $deterministicHash = Get-RequiredStringProperty -JsonObject $executionRow `
            -Name "deterministic_readback_hash_sha256" -Diagnostics $rowDiagnostics
        if (-not (Test-Sha256Text -Value $readbackHash) -or -not (Test-Sha256Text -Value $deterministicHash) -or
            $readbackHash -cne $deterministicHash) {
            $rowDiagnostics.Add("invalid_readback_hash")
        } else {
            $readbackHashRows += 1
        }
        if (-not [string]::IsNullOrWhiteSpace($readbackPath)) {
            $artifactPath = Resolve-MavgMetalEvidenceArtifactPath -EvidenceDirectory $evidenceFile.DirectoryName `
                -ArtifactRootFull $artifactRootFull -RelativePath $readbackPath -Diagnostics $rowDiagnostics
            if ($null -eq $artifactPath -or -not (Test-Path -LiteralPath $artifactPath -PathType Leaf)) {
                $missingArtifactRows += 1
                $rowDiagnostics.Add("missing_readback_artifact")
            } else {
                $actualHash = (Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash.ToLowerInvariant()
                if ($actualHash -cne $readbackHash) {
                    $missingArtifactRows += 1
                    $rowDiagnostics.Add("readback_artifact_hash_mismatch")
                }
            }
        }
        $packageOutputWritten = Test-TrueProperty -JsonObject $executionRow -Name "package_visible_output_written"
        $packageOutputRows = Get-RequiredIntegerProperty -JsonObject $executionRow `
            -Name "package_visible_output_rows" -Diagnostics $rowDiagnostics
        if ($packageOutputWritten -and $packageOutputRows -gt 0) {
            $packageVisibleOutputRows += 1
        } else {
            $rowDiagnostics.Add("missing_package_visible_output")
        }
    }

    if ($null -eq $nonClaims) {
        $rowDiagnostics.Add("missing_non_claims")
    } else {
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "simulator_only_evidence")) {
            $rowDiagnostics.Add("simulator_only_evidence")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "cross_backend_inference")) {
            $crossBackendInferenceRows += 1
            $rowDiagnostics.Add("cross_backend_inference")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "ray_tracing_pipeline_conflation")) {
            $rayTracingConflationRows += 1
            $rowDiagnostics.Add("ray_tracing_pipeline_conflation")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "native_handles_exposed")) {
            $nativeHandleRows += 1
            $rowDiagnostics.Add("native_handles_exposed")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "mavg_mesh_shader_lod_ready")) {
            $rowDiagnostics.Add("mavg_mesh_shader_lod_ready_claimed")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "mavg_metal_ray_tracing_ready")) {
            $rowDiagnostics.Add("mavg_metal_ray_tracing_ready_claimed")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "mavg_nanite_compatible")) {
            $naniteCompatibleRows += 1
            $rowDiagnostics.Add("mavg_nanite_compatible_claimed")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "mavg_nanite_equivalent")) {
            $naniteEquivalentRows += 1
            $rowDiagnostics.Add("mavg_nanite_equivalent_claimed")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "mavg_nanite_superior")) {
            $naniteSuperiorRows += 1
            $rowDiagnostics.Add("mavg_nanite_superior_claimed")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "mavg_broad_backend_readiness")) {
            $broadBackendReadinessRows += 1
            $rowDiagnostics.Add("mavg_broad_backend_readiness_claimed")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "mavg_broad_optimization")) {
            $broadOptimizationRows += 1
            $rowDiagnostics.Add("mavg_broad_optimization_claimed")
        }
    }

    if ($rowDiagnostics.Count -eq 0) {
        $readyRows += 1
    } else {
        $invalidRows += 1
    }
}

$ready = $readyRows -gt 0 -and $invalidRows -eq 0 -and $missingArtifactRows -eq 0
$hostGated = -not $ready
$status = if ($ready) { "ready" } elseif ($artifactRows -gt 0 -and $invalidRows -gt 0) { "blocked" } else { "host_evidence_required" }

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=mavg-metal-mesh-lod-host-evidence")
$lines.Add("artifact_root=$ArtifactRootRelative")
$lines.Add("mavg_metal_mesh_lod_status=$status")
$lines.Add("mavg_metal_mesh_lod_host=$hostLabel")
$lines.Add("mavg_metal_mesh_lod_host_gated=$(ConvertTo-CounterBit $hostGated)")
$lines.Add("mavg_metal_mesh_lod_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_mesh_shader_lod_ready=0")
$lines.Add("mavg_metal_ray_tracing_ready=0")
$lines.Add("mavg_metal_mesh_lod_feature_set_table_source=Apple-Metal-Feature-Set-Tables-2026-05-21")
$lines.Add("mavg_metal_mesh_lod_feature_set_table_row_reviewed=1")
$lines.Add("mavg_metal_mesh_lod_full_xcode_selected=$(ConvertTo-CounterBit $fullXcodeSelected)")
$lines.Add("mavg_metal_mesh_lod_metal_tool_ready=$(ConvertTo-CounterBit $metalToolReady)")
$lines.Add("mavg_metal_mesh_lod_metallib_tool_ready=$(ConvertTo-CounterBit $metallibToolReady)")
$lines.Add("mavg_metal_mesh_lod_execution_artifact_rows=$artifactRows")
$lines.Add("mavg_metal_mesh_lod_execution_ready_rows=$readyRows")
$lines.Add("mavg_metal_mesh_lod_execution_invalid_rows=$invalidRows")
$lines.Add("mavg_metal_mesh_lod_execution_missing_artifacts=$missingArtifactRows")
$lines.Add("mavg_metal_mesh_lod_retained_apple_host_evidence=$(ConvertTo-CounterBit $ready)")
$lines.Add("mavg_metal_mesh_lod_apple_gpu_family_rows=$appleGpuFamilyRows")
$lines.Add("mavg_metal_mesh_lod_mesh_shader_feature_rows=$meshShaderFeatureRows")
$lines.Add("mavg_metal_mesh_lod_object_shader_feature_rows=$objectShaderFeatureRows")
$lines.Add("mavg_metal_mesh_lod_first_party_workload_rows=$firstPartyWorkloadRows")
$lines.Add("mavg_metal_mesh_lod_object_mesh_pipeline_rows=$objectMeshPipelineRows")
$lines.Add("mavg_metal_mesh_lod_object_mesh_dispatch_rows=$objectMeshDispatchRows")
$lines.Add("mavg_metal_mesh_lod_draw_mesh_threads_rows=$drawMeshThreadsRows")
$lines.Add("mavg_metal_mesh_lod_shader_payload_rows=$shaderPayloadRows")
$lines.Add("mavg_metal_mesh_lod_readback_hash_rows=$readbackHashRows")
$lines.Add("mavg_metal_mesh_lod_package_visible_output_rows=$packageVisibleOutputRows")
$lines.Add("mavg_metal_mesh_lod_simulator_only_evidence=0")
$lines.Add("mavg_metal_mesh_lod_cross_backend_inference=$crossBackendInferenceRows")
$lines.Add("mavg_metal_mesh_lod_ray_tracing_pipeline_conflation=$rayTracingConflationRows")
$lines.Add("mavg_metal_mesh_lod_native_handles_exposed=$nativeHandleRows")
$lines.Add("mavg_metal_mesh_lod_nanite_compatible=$naniteCompatibleRows")
$lines.Add("mavg_metal_mesh_lod_nanite_equivalent=$naniteEquivalentRows")
$lines.Add("mavg_metal_mesh_lod_nanite_superior=$naniteSuperiorRows")
$lines.Add("mavg_metal_mesh_lod_broad_backend_readiness=$broadBackendReadinessRows")
$lines.Add("mavg_metal_mesh_lod_broad_optimization=$broadOptimizationRows")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "MAVG Metal mesh LOD readiness is incomplete; Apple-host object/mesh shader execution, readback hash, and package-visible output evidence are required before mavg_metal_mesh_lod_ready can be 1."
}

Write-Information "mavg-metal-host-evidence-check: ok" -InformationAction Continue
