#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$EvidenceRoot = "artifacts/performance/optional-gpu-compute-review",

    [string[]]$RequiredCandidateIds = @(),

    [switch]$RequireReady,

    [string[]]$ExpectedEvidenceCounters = @(),

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AdditionalExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root
$ExpectedEvidenceCounters = @($ExpectedEvidenceCounters) + @($AdditionalExpectedEvidenceCounters)

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Get-HostLabel {
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Windows)) {
        return "windows"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Linux)) {
        return "linux"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::OSX)) {
        return "macos"
    }
    return "unknown"
}

function Get-JsonPropertyValue {
    param(
        [Parameter(Mandatory = $true)][object]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $property = $JsonObject.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }
    return $property.Value
}

function ConvertTo-PropertyName {
    param([Parameter(Mandatory = $true)][string]$Name)

    return $Name.ToLowerInvariant().Replace("/", "_").Replace("-", "_").Replace(" ", "_")
}

function Test-NonEmptyEvidenceField {
    param(
        [Parameter(Mandatory = $true)][object]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $directValue = Get-JsonPropertyValue -JsonObject $JsonObject -Name $Name
    if (-not [string]::IsNullOrWhiteSpace([string]$directValue)) {
        return $true
    }

    $normalizedValue = Get-JsonPropertyValue -JsonObject $JsonObject -Name (ConvertTo-PropertyName $Name)
    return -not [string]::IsNullOrWhiteSpace([string]$normalizedValue)
}

function Add-Diagnostic {
    param(
        [Parameter(Mandatory = $true)]$Diagnostics,
        [Parameter(Mandatory = $true)][string]$Message
    )

    $Diagnostics.Add($Message) | Out-Null
}

function Test-SafeEvidencePath {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$EvidenceRootFull
    )

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        return $false
    }
    if ($RelativePath.Contains("\")) {
        return $false
    }
    if ($RelativePath.StartsWith("/", [System.StringComparison]::Ordinal)) {
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

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    $rootWithSeparator = $EvidenceRootFull.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    return $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)
}

function Test-CommandAvailable {
    param([Parameter(Mandatory = $true)][string]$Command)

    return $null -ne (Find-CommandOnCombinedPath $Command)
}

function Get-StringSet {
    param([object[]]$Values)

    $set = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($value in @($Values)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$value)) {
            $null = $set.Add([string]$value)
        }
    }
    return $set
}

$manifest = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.json") -Raw | ConvertFrom-Json
$review = $manifest.aiOperableProductionLoop.optionalGpuComputeReview
$candidateRows = @($review.candidateRows)
$candidateIds = @($candidateRows | ForEach-Object { [string]$_.id })
if ($RequiredCandidateIds.Count -eq 0) {
    $RequiredCandidateIds = $candidateIds
}

$knownCandidates = @{}
foreach ($candidate in $candidateRows) {
    $knownCandidates[[string]$candidate.id] = $candidate
}
$knownClassifications = Get-StringSet @($review.classifications | ForEach-Object { [string]$_ })
$requiredEvidenceFields = @($review.requiredEvidenceFields | ForEach-Object { [string]$_ })

$evidenceRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $EvidenceRoot))
$evidenceFiles = @()
if (Test-Path -LiteralPath $evidenceRootFull -PathType Container) {
    $evidenceFiles = @(Get-ChildItem -LiteralPath $evidenceRootFull -Recurse -File -Filter "evidence.json")
}

$validRows = 0
$readyRows = 0
$diagnosticCount = 0
$invalidJsonCount = 0
$pathEscapeCount = 0
$profilerArtifactRows = 0
$dependencyInstallGateRows = 0
$fallbackRows = 0
$synchronizationEvidenceRows = 0
$readyCandidates = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)

foreach ($evidenceFile in $evidenceFiles) {
    $rowDiagnostics = [System.Collections.Generic.List[string]]::new()
    try {
        $evidence = Get-Content -LiteralPath $evidenceFile.FullName -Raw | ConvertFrom-Json
    } catch {
        $invalidJsonCount += 1
        $diagnosticCount += 1
        continue
    }

    $candidateId = [string](Get-JsonPropertyValue -JsonObject $evidence -Name "candidate_id")
    if (-not $knownCandidates.ContainsKey($candidateId)) {
        Add-Diagnostic $rowDiagnostics "unknown_candidate_id"
    }

    $classification = [string](Get-JsonPropertyValue -JsonObject $evidence -Name "classification")
    if (-not $knownClassifications.Contains($classification)) {
        Add-Diagnostic $rowDiagnostics "unknown_classification"
    }
    if ($knownCandidates.ContainsKey($candidateId) -and
        [string]$knownCandidates[$candidateId].classification -ne $classification) {
        Add-Diagnostic $rowDiagnostics "classification_mismatch"
    }

    foreach ($field in $requiredEvidenceFields) {
        if (-not (Test-NonEmptyEvidenceField -JsonObject $evidence -Name $field)) {
            Add-Diagnostic $rowDiagnostics "missing_evidence_field:$(ConvertTo-PropertyName $field)"
        }
    }

    foreach ($counterName in @(
            "default_validation_dependency",
            "vcpkg_feature_added",
            "cmake_linkage_added",
            "runtime_dependency_added",
            "public_native_handles_exposed",
            "broad_gpu_compute_claim",
            "async_overlap_claim",
            "cross_vendor_parity_claim",
            "cross_backend_parity_claim",
            "broad_optimization_claim"
        )) {
        $value = [string](Get-JsonPropertyValue -JsonObject $evidence -Name $counterName)
        if ($value -ne "0") {
            Add-Diagnostic $rowDiagnostics "forbidden_side_effect:$counterName"
        }
    }

    $profilerArtifacts = @(Get-JsonPropertyValue -JsonObject $evidence -Name "profiler_artifacts")
    if ($classification -ne "non_goal" -and $profilerArtifacts.Count -eq 0) {
        Add-Diagnostic $rowDiagnostics "missing_profiler_artifacts"
    }
    foreach ($artifact in $profilerArtifacts) {
        $profilerArtifactRows += 1
        $relativePath = [string](Get-JsonPropertyValue -JsonObject $artifact -Name "path")
        if (-not (Test-SafeEvidencePath -RelativePath $relativePath -EvidenceRootFull $evidenceRootFull)) {
            $pathEscapeCount += 1
            Add-Diagnostic $rowDiagnostics "unsafe_profiler_artifact_path"
            continue
        }
        $artifactFullPath = [System.IO.Path]::GetFullPath((Join-Path $root $relativePath))
        if (-not (Test-Path -LiteralPath $artifactFullPath -PathType Leaf)) {
            Add-Diagnostic $rowDiagnostics "missing_profiler_artifact_file"
        }
    }

    if (Test-NonEmptyEvidenceField -JsonObject $evidence -Name "dependency install gate") {
        $dependencyInstallGateRows += 1
    }
    if (Test-NonEmptyEvidenceField -JsonObject $evidence -Name "scalar or RHI fallback") {
        $fallbackRows += 1
    }
    if ((Test-NonEmptyEvidenceField -JsonObject $evidence -Name "synchronization") -and
        (Test-NonEmptyEvidenceField -JsonObject $evidence -Name "stream/event usage")) {
        $synchronizationEvidenceRows += 1
    }

    if ($rowDiagnostics.Count -eq 0) {
        $validRows += 1
        $readyRows += 1
        $null = $readyCandidates.Add($candidateId)
    } else {
        $diagnosticCount += $rowDiagnostics.Count
    }
}

$missingCandidateCount = 0
foreach ($candidateId in $RequiredCandidateIds) {
    if (-not $readyCandidates.Contains($candidateId)) {
        $missingCandidateCount += 1
    }
}

$ready = $evidenceFiles.Count -gt 0 -and
    $diagnosticCount -eq 0 -and
    $invalidJsonCount -eq 0 -and
    $pathEscapeCount -eq 0 -and
    $missingCandidateCount -eq 0
$status = if ($ready) { "ready" } else { "host-gated" }

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=host-optional-gpu-compute-review")
$lines.Add("optional_gpu_compute_review_status=$status")
$lines.Add("optional_gpu_compute_review_host=$(Get-HostLabel)")
$lines.Add("optional_gpu_compute_review_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("optional_gpu_compute_review_host_gated=$(ConvertTo-CounterBit (-not $ready))")
$lines.Add("optional_gpu_compute_review_evidence_rows=$($evidenceFiles.Count)")
$lines.Add("optional_gpu_compute_review_valid_rows=$validRows")
$lines.Add("optional_gpu_compute_review_ready_rows=$readyRows")
$lines.Add("optional_gpu_compute_review_required_candidate_rows=$($RequiredCandidateIds.Count)")
$lines.Add("optional_gpu_compute_review_ready_candidate_rows=$($readyCandidates.Count)")
$lines.Add("optional_gpu_compute_review_missing_candidate_rows=$missingCandidateCount")
$lines.Add("optional_gpu_compute_review_profiler_artifact_rows=$profilerArtifactRows")
$lines.Add("optional_gpu_compute_review_dependency_install_gate_rows=$dependencyInstallGateRows")
$lines.Add("optional_gpu_compute_review_fallback_rows=$fallbackRows")
$lines.Add("optional_gpu_compute_review_synchronization_evidence_rows=$synchronizationEvidenceRows")
$lines.Add("optional_gpu_compute_review_diagnostic_count=$diagnosticCount")
$lines.Add("optional_gpu_compute_review_invalid_json_count=$invalidJsonCount")
$lines.Add("optional_gpu_compute_review_path_escape_count=$pathEscapeCount")
$lines.Add("optional_gpu_compute_review_dxc_ready=$(ConvertTo-CounterBit (Test-CommandAvailable 'dxc'))")
$lines.Add("optional_gpu_compute_review_spirv_val_ready=$(ConvertTo-CounterBit (Test-CommandAvailable 'spirv-val'))")
$lines.Add("optional_gpu_compute_review_pix_ready=$(ConvertTo-CounterBit (Test-CommandAvailable 'pix'))")
$lines.Add("optional_gpu_compute_review_nvcc_ready=$(ConvertTo-CounterBit (Test-CommandAvailable 'nvcc'))")
$lines.Add("optional_gpu_compute_review_hipcc_ready=$(ConvertTo-CounterBit (Test-CommandAvailable 'hipcc'))")
$lines.Add("optional_gpu_compute_review_sycl_ls_ready=$(ConvertTo-CounterBit (Test-CommandAvailable 'sycl-ls'))")
$lines.Add("optional_gpu_compute_review_default_validation_dependency=0")
$lines.Add("optional_gpu_compute_review_vcpkg_feature_added=0")
$lines.Add("optional_gpu_compute_review_cmake_linkage_added=0")
$lines.Add("optional_gpu_compute_review_runtime_dependency_added=0")
$lines.Add("optional_gpu_compute_review_public_native_handles_exposed=0")
$lines.Add("optional_gpu_compute_review_broad_gpu_compute_claim=0")
$lines.Add("optional_gpu_compute_review_async_overlap_claim=0")
$lines.Add("optional_gpu_compute_review_cross_vendor_parity_claim=0")
$lines.Add("optional_gpu_compute_review_cross_backend_parity_claim=0")
$lines.Add("optional_gpu_compute_review_broad_optimization_claim=0")

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "Optional GPU compute review readiness is incomplete; attach candidate classifications, profiler artifacts where required, synchronization proof, dependency/install gates, and fallback evidence before optional_gpu_compute_review_ready can be 1."
}

Write-Information "optional-gpu-compute-review-evidence-check: ok" -InformationAction Continue
