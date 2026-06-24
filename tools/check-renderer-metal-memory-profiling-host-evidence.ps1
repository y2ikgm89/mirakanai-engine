#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [switch]$RequireReady,
    [switch]$SkipFocusedRendererBuild,
    [string]$ArtifactRootRelative = "artifacts/renderer/metal-memory-profiling-host-evidence",
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

function Get-RendererMetalHostLabel {
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

function Get-RequiredInt64Property {
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
        return [long]$value
    } catch {
        $Diagnostics.Add("invalid_$Name")
        return 0
    }
}

function Resolve-RendererMetalMemoryProfilingArtifactPath {
    param(
        [Parameter(Mandatory = $true)][string]$EvidenceDirectory,
        [Parameter(Mandatory = $true)][string]$ArtifactRootFull,
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    if ([System.IO.Path]::IsPathRooted($RelativePath) -or $RelativePath.Contains("..")) {
        $Diagnostics.Add("unsafe_capture_artifact_path")
        return $null
    }
    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $EvidenceDirectory $RelativePath))
    $rootWithSeparator = $ArtifactRootFull.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        $Diagnostics.Add("capture_artifact_path_escape")
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
        $Diagnostics.Add("invalid_$Name")
    }
}

$schemaPath = Join-Path $root "schemas/renderer-metal-memory-profiling-host-evidence.schema.json"
if (-not (Test-Path -LiteralPath $schemaPath -PathType Leaf)) {
    Write-Error "Missing renderer Metal memory/profiling host evidence schema: $schemaPath"
}
$schemaText = Get-Content -LiteralPath $schemaPath -Raw
foreach ($needle in @(
        "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1",
        "renderer-metal-memory-profiling-host-evidence-v1",
        "Apple-Metal-MTLHeap-2026-06-24",
        "Apple-Metal-MTLResidencySet-2026-06-24",
        "Apple-Metal-MTLCaptureManager-2026-06-24",
        "heap_allocation_ready",
        "residency_set_ready",
        "capture_artifact_hash_sha256",
        "broad_renderer_quality"
    )) {
    if (-not $schemaText.Contains($needle)) {
        Write-Error "Renderer Metal memory/profiling host evidence schema must contain '$needle'."
    }
}

$pwsh = Find-CommandOnCombinedPath "pwsh"
if (-not $pwsh) {
    Write-Error "PowerShell 7 is required for renderer Metal memory/profiling host evidence validation."
}

if (-not $SkipFocusedRendererBuild.IsPresent) {
    Write-Information "renderer-metal-memory-profiling-host-evidence: configuring dev preset..." `
        -InformationAction Continue
    $null = Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $root "tools/cmake.ps1"),
        "--preset",
        "dev"
    )

    Write-Information "renderer-metal-memory-profiling-host-evidence: building focused renderer tests..." `
        -InformationAction Continue
    $null = Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $root "tools/cmake.ps1"),
        "--build",
        "--preset",
        "dev",
        "--target",
        "MK_renderer_tests"
    )

    Write-Information "renderer-metal-memory-profiling-host-evidence: running focused renderer CTest lane..." `
        -InformationAction Continue
    $null = Invoke-CheckedCommand -FilePath $pwsh -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $root "tools/ctest.ps1"),
        "--preset",
        "dev",
        "--output-on-failure",
        "-R",
        "MK_renderer_tests"
    )
}

$hostLabel = Get-RendererMetalHostLabel
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
$memoryReadyRows = 0
$captureReadyRows = 0
$heapRows = 0
$residencySetRows = 0
$residencyCommitRows = 0
$pressureRows = 0
$captureManagerRows = 0
$captureScopeRows = 0
$captureBoundaryRows = 0
$captureArtifactRows = 0
$crossBackendInferenceRows = 0
$nativeHandleRows = 0
$broadBackendParityRows = 0
$broadMetalReadinessRows = 0
$commercialRendererRows = 0
$broadRendererQualityRows = 0
$environmentReadyRows = 0
$externalEngineParityRows = 0

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
    $sourceRows = Get-JsonPropertyValue -JsonObject $evidence -Name "source_rows"
    $memoryRow = Get-JsonPropertyValue -JsonObject $evidence -Name "memory_residency_row"
    $captureRow = Get-JsonPropertyValue -JsonObject $evidence -Name "profiling_capture_row"
    $nonClaims = Get-JsonPropertyValue -JsonObject $evidence -Name "non_claims"

    if ($schemaVersion -cne "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1") {
        $rowDiagnostics.Add("invalid_schema_version")
    }
    if ($claimId -cne "renderer-metal-memory-profiling-host-evidence-v1") {
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

    if ($null -eq $sourceRows) {
        $rowDiagnostics.Add("missing_source_rows")
    } else {
        Assert-SourceRow -SourceRows $sourceRows -Name "heap_documentation_source_id" `
            -Expected "Apple-Metal-MTLHeap-2026-06-24" -Diagnostics $rowDiagnostics
        Assert-SourceRow -SourceRows $sourceRows -Name "residency_set_documentation_source_id" `
            -Expected "Apple-Metal-MTLResidencySet-2026-06-24" -Diagnostics $rowDiagnostics
        Assert-SourceRow -SourceRows $sourceRows -Name "residency_request_documentation_source_id" `
            -Expected "Apple-Metal-MTLResidencySet-requestResidency-2026-06-24" -Diagnostics $rowDiagnostics
        Assert-SourceRow -SourceRows $sourceRows -Name "command_queue_residency_documentation_source_id" `
            -Expected "Apple-Metal-MTLCommandQueue-addResidencySet-2026-06-24" -Diagnostics $rowDiagnostics
        Assert-SourceRow -SourceRows $sourceRows -Name "capture_manager_documentation_source_id" `
            -Expected "Apple-Metal-MTLCaptureManager-2026-06-24" -Diagnostics $rowDiagnostics
        Assert-SourceRow -SourceRows $sourceRows -Name "programmatic_capture_documentation_source_id" `
            -Expected "Apple-Metal-ProgrammaticCapture-2026-06-24" -Diagnostics $rowDiagnostics
    }

    $memoryRowReady = $false
    if ($null -eq $memoryRow) {
        $rowDiagnostics.Add("missing_memory_residency_row")
    } else {
        if ((Get-RequiredStringProperty -JsonObject $memoryRow -Name "proof_row_id" -Diagnostics $rowDiagnostics) -cne "memory_residency") {
            $rowDiagnostics.Add("invalid_memory_proof_row_id")
        }
        if ((Get-RequiredStringProperty -JsonObject $memoryRow -Name "host_validation_recipe_id" `
                    -Diagnostics $rowDiagnostics) -cne "renderer-metal-memory-profiling-host-evidence") {
            $rowDiagnostics.Add("invalid_memory_host_validation_recipe_id")
        }
        $null = Get-RequiredStringProperty -JsonObject $memoryRow -Name "first_party_workload_id" `
            -Diagnostics $rowDiagnostics
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
            if (-not (Test-TrueProperty -JsonObject $memoryRow -Name $requiredTrue)) {
                $rowDiagnostics.Add("missing_$requiredTrue")
            }
        }
        if ((Get-RequiredStringProperty -JsonObject $memoryRow -Name "heap_api_name" -Diagnostics $rowDiagnostics) -cne "MTLHeap") {
            $rowDiagnostics.Add("invalid_heap_api_name")
        }
        if ((Get-RequiredStringProperty -JsonObject $memoryRow -Name "residency_api_name" `
                    -Diagnostics $rowDiagnostics) -cne "MTLResidencySet") {
            $rowDiagnostics.Add("invalid_residency_api_name")
        }
        $heapResourceRows = Get-RequiredInt64Property -JsonObject $memoryRow -Name "heap_resource_rows" `
            -Diagnostics $rowDiagnostics
        $heapAllocatedBytes = Get-RequiredInt64Property -JsonObject $memoryRow -Name "heap_allocated_bytes" `
            -Diagnostics $rowDiagnostics
        $residentBytes = Get-RequiredInt64Property -JsonObject $memoryRow -Name "resident_bytes" `
            -Diagnostics $rowDiagnostics
        $budgetBytes = Get-RequiredInt64Property -JsonObject $memoryRow -Name "budget_bytes" `
            -Diagnostics $rowDiagnostics
        $residencySetAllocationRows = Get-RequiredInt64Property -JsonObject $memoryRow `
            -Name "residency_set_allocation_rows" -Diagnostics $rowDiagnostics
        $pressureSampleRows = Get-RequiredInt64Property -JsonObject $memoryRow -Name "memory_pressure_sample_rows" `
            -Diagnostics $rowDiagnostics
        $budgetStatus = Get-RequiredStringProperty -JsonObject $memoryRow -Name "memory_pressure_budget_status" `
            -Diagnostics $rowDiagnostics
        if ($heapResourceRows -lt 1) { $rowDiagnostics.Add("invalid_heap_resource_rows") }
        if ($heapAllocatedBytes -lt 1) { $rowDiagnostics.Add("invalid_heap_allocated_bytes") }
        if ($residentBytes -lt 1) { $rowDiagnostics.Add("invalid_resident_bytes") }
        if ($budgetBytes -lt 1 -or $budgetBytes -lt $residentBytes) { $rowDiagnostics.Add("invalid_budget_bytes") }
        if ($residencySetAllocationRows -lt 1) { $rowDiagnostics.Add("invalid_residency_set_allocation_rows") }
        if ($pressureSampleRows -lt 1) { $rowDiagnostics.Add("invalid_memory_pressure_sample_rows") }
        if ($budgetStatus -notin @("within_budget", "pressure_observed")) {
            $rowDiagnostics.Add("invalid_memory_pressure_budget_status")
        }
        $memoryRowReady = $heapResourceRows -gt 0 -and $heapAllocatedBytes -gt 0 -and $residentBytes -gt 0 -and
            $budgetBytes -ge $residentBytes -and $residencySetAllocationRows -gt 0 -and $pressureSampleRows -gt 0
        if ($memoryRowReady) {
            $heapRows += 1
            $residencySetRows += 1
            $residencyCommitRows += 1
            $pressureRows += 1
        }
    }

    $captureRowReady = $false
    if ($null -eq $captureRow) {
        $rowDiagnostics.Add("missing_profiling_capture_row")
    } else {
        if ((Get-RequiredStringProperty -JsonObject $captureRow -Name "proof_row_id" `
                    -Diagnostics $rowDiagnostics) -cne "profiling_capture") {
            $rowDiagnostics.Add("invalid_capture_proof_row_id")
        }
        if ((Get-RequiredStringProperty -JsonObject $captureRow -Name "host_validation_recipe_id" `
                    -Diagnostics $rowDiagnostics) -cne "renderer-metal-memory-profiling-host-evidence") {
            $rowDiagnostics.Add("invalid_capture_host_validation_recipe_id")
        }
        $null = Get-RequiredStringProperty -JsonObject $captureRow -Name "first_party_workload_id" `
            -Diagnostics $rowDiagnostics
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
            if (-not (Test-TrueProperty -JsonObject $captureRow -Name $requiredTrue)) {
                $rowDiagnostics.Add("missing_$requiredTrue")
            }
        }
        if ((Get-RequiredStringProperty -JsonObject $captureRow -Name "capture_api_name" `
                    -Diagnostics $rowDiagnostics) -cne "MTLCaptureManager") {
            $rowDiagnostics.Add("invalid_capture_api_name")
        }
        $null = Get-RequiredStringProperty -JsonObject $captureRow -Name "capture_scope_label" `
            -Diagnostics $rowDiagnostics
        $capturePath = Get-RequiredStringProperty -JsonObject $captureRow -Name "capture_artifact_path" `
            -Diagnostics $rowDiagnostics
        $captureHash = Get-RequiredStringProperty -JsonObject $captureRow -Name "capture_artifact_hash_sha256" `
            -Diagnostics $rowDiagnostics
        $deterministicHash = Get-RequiredStringProperty -JsonObject $captureRow `
            -Name "deterministic_capture_hash_sha256" -Diagnostics $rowDiagnostics
        $captureArtifactRowCount = Get-RequiredInt64Property -JsonObject $captureRow -Name "capture_artifact_rows" `
            -Diagnostics $rowDiagnostics
        if (-not (Test-Sha256Text -Value $captureHash) -or -not (Test-Sha256Text -Value $deterministicHash) -or
            $captureHash -cne $deterministicHash) {
            $rowDiagnostics.Add("invalid_capture_hash")
        }
        if ($captureArtifactRowCount -lt 1) {
            $rowDiagnostics.Add("invalid_capture_artifact_rows")
        }
        if (-not [string]::IsNullOrWhiteSpace($capturePath)) {
            $artifactPath = Resolve-RendererMetalMemoryProfilingArtifactPath `
                -EvidenceDirectory $evidenceFile.DirectoryName `
                -ArtifactRootFull $artifactRootFull `
                -RelativePath $capturePath `
                -Diagnostics $rowDiagnostics
            if ($null -eq $artifactPath -or -not (Test-Path -LiteralPath $artifactPath -PathType Leaf)) {
                $missingArtifactRows += 1
                $rowDiagnostics.Add("missing_capture_artifact")
            } else {
                $actualHash = (Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash.ToLowerInvariant()
                if ($actualHash -cne $captureHash) {
                    $missingArtifactRows += 1
                    $rowDiagnostics.Add("capture_artifact_hash_mismatch")
                }
            }
        }
        $captureRowReady = Test-Sha256Text -Value $captureHash -and $captureHash -ceq $deterministicHash -and
            $captureArtifactRowCount -gt 0
        if ($captureRowReady) {
            $captureManagerRows += 1
            $captureScopeRows += 1
            $captureBoundaryRows += 1
            $captureArtifactRows += 1
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
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "native_handles_exposed")) {
            $nativeHandleRows += 1
            $rowDiagnostics.Add("native_handles_exposed")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "broad_backend_parity_ready")) {
            $broadBackendParityRows += 1
            $rowDiagnostics.Add("broad_backend_parity_ready")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "broad_metal_readiness")) {
            $broadMetalReadinessRows += 1
            $rowDiagnostics.Add("broad_metal_readiness")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "commercial_renderer_readiness")) {
            $commercialRendererRows += 1
            $rowDiagnostics.Add("commercial_renderer_readiness")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "broad_renderer_quality")) {
            $broadRendererQualityRows += 1
            $rowDiagnostics.Add("broad_renderer_quality")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "environment_ready")) {
            $environmentReadyRows += 1
            $rowDiagnostics.Add("environment_ready")
        }
        if (-not (Test-FalseProperty -JsonObject $nonClaims -Name "external_engine_api_parity")) {
            $externalEngineParityRows += 1
            $rowDiagnostics.Add("external_engine_api_parity")
        }
    }

    if ($rowDiagnostics.Count -eq 0) {
        $readyRows += 1
        if ($memoryRowReady) {
            $memoryReadyRows += 1
        }
        if ($captureRowReady) {
            $captureReadyRows += 1
        }
    } else {
        $invalidRows += 1
    }
}

$memoryReady = $memoryReadyRows -gt 0 -and $invalidRows -eq 0 -and $missingArtifactRows -eq 0
$captureReady = $captureReadyRows -gt 0 -and $invalidRows -eq 0 -and $missingArtifactRows -eq 0
$ready = $memoryReady -and $captureReady
$hostGated = -not $ready
$status = if ($ready) { "ready" } elseif ($artifactRows -gt 0 -and $invalidRows -gt 0) { "blocked" } else { "host_evidence_required" }

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=renderer-metal-memory-profiling-host-evidence")
$lines.Add("artifact_root=$ArtifactRootRelative")
$lines.Add("renderer_metal_memory_profiling_status=$status")
$lines.Add("renderer_metal_memory_profiling_host=$hostLabel")
$lines.Add("renderer_metal_memory_profiling_host_gated=$(ConvertTo-CounterBit $hostGated)")
$lines.Add("renderer_metal_memory_profiling_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("renderer_metal_memory_profiling_retained_apple_host_evidence=$(ConvertTo-CounterBit $ready)")
$lines.Add("renderer_metal_memory_residency_ready=$(ConvertTo-CounterBit $memoryReady)")
$lines.Add("renderer_metal_profiling_capture_ready=$(ConvertTo-CounterBit $captureReady)")
$lines.Add("renderer_metal_memory_profiling_full_xcode_selected=$(ConvertTo-CounterBit $fullXcodeSelected)")
$lines.Add("renderer_metal_memory_profiling_metal_tool_ready=$(ConvertTo-CounterBit $metalToolReady)")
$lines.Add("renderer_metal_memory_profiling_metallib_tool_ready=$(ConvertTo-CounterBit $metallibToolReady)")
$lines.Add("renderer_metal_memory_profiling_heap_source=Apple-Metal-MTLHeap-2026-06-24")
$lines.Add("renderer_metal_memory_profiling_residency_source=Apple-Metal-MTLResidencySet-2026-06-24")
$lines.Add("renderer_metal_memory_profiling_capture_source=Apple-Metal-MTLCaptureManager-2026-06-24")
$lines.Add("renderer_metal_memory_profiling_artifact_rows=$artifactRows")
$lines.Add("renderer_metal_memory_profiling_ready_rows=$readyRows")
$lines.Add("renderer_metal_memory_profiling_invalid_rows=$invalidRows")
$lines.Add("renderer_metal_memory_profiling_missing_artifacts=$missingArtifactRows")
$lines.Add("renderer_metal_memory_profiling_heap_rows=$heapRows")
$lines.Add("renderer_metal_memory_profiling_residency_set_rows=$residencySetRows")
$lines.Add("renderer_metal_memory_profiling_residency_commit_rows=$residencyCommitRows")
$lines.Add("renderer_metal_memory_profiling_pressure_rows=$pressureRows")
$lines.Add("renderer_metal_memory_profiling_capture_manager_rows=$captureManagerRows")
$lines.Add("renderer_metal_memory_profiling_capture_scope_rows=$captureScopeRows")
$lines.Add("renderer_metal_memory_profiling_capture_boundary_rows=$captureBoundaryRows")
$lines.Add("renderer_metal_memory_profiling_capture_artifact_rows=$captureArtifactRows")
$lines.Add("renderer_metal_memory_profiling_cross_backend_inference=$crossBackendInferenceRows")
$lines.Add("renderer_metal_memory_profiling_native_handles_exposed=$nativeHandleRows")
$lines.Add("renderer_metal_memory_profiling_broad_backend_parity_claims=$broadBackendParityRows")
$lines.Add("renderer_metal_memory_profiling_broad_metal_readiness_claims=$broadMetalReadinessRows")
$lines.Add("renderer_metal_memory_profiling_commercial_renderer_claims=$commercialRendererRows")
$lines.Add("renderer_metal_memory_profiling_broad_renderer_quality_claims=$broadRendererQualityRows")
$lines.Add("renderer_metal_memory_profiling_environment_ready_claims=$environmentReadyRows")
$lines.Add("renderer_metal_memory_profiling_external_engine_api_parity=$externalEngineParityRows")
$lines.Add("renderer_backend_parity_ready=0")
$lines.Add("renderer_metal_broad_readiness=0")
$lines.Add("renderer_commercial_readiness=0")
$lines.Add("renderer_broad_quality_ready=0")
$lines.Add("renderer_environment_ready=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "Renderer Metal memory/profiling readiness is incomplete; retained Apple-host MTLHeap, MTLResidencySet, MTLCaptureManager, and capture artifact evidence is required before renderer_metal_memory_profiling_ready can be 1."
}

Write-Information "renderer-metal-memory-profiling-host-evidence-check: ok" -InformationAction Continue
