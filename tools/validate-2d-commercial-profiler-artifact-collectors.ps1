#requires -Version 7.0
#requires -PSEdition Core

# Supported synthetic contract smoke:
# pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-commercial-profiler-artifact-collectors.ps1 -SyntheticSmoke
# Expected ready counters for the synthetic smoke contract include:
# profiler_artifact_collector_status=ready
# profiler_artifact_collector_required_tool_rows=5
# profiler_artifact_collector_ready_rows=5
# profiler_artifact_collector_retained_artifact_rows=5
# profiler_artifact_collector_hash_mismatch_rows=0
# profiler_artifact_collector_external_engine_claim_rows=0
# profiler_artifact_collector_legal_approval_claim_rows=0

[CmdletBinding()]
param(
    [string]$ArtifactRootRelative = "",

    [switch]$SyntheticSmoke,

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

$schemaId = "GameEngine.2DCommercialProfilerArtifactCollector.v1"
$claimId = "2d-commercial-profiler-artifact-collectors-v1"
$manifestFileName = "profiler-artifact-collector.json"

$requiredToolRows = @(
    [pscustomobject]@{
        id = "windows-wpr-wpa"
        backend = "d3d12"
        host = "windows"
        artifact = "wpa-summary.txt"
        sourceId = "microsoft.windows-performance-toolkit"
        sourceUrl = "https://learn.microsoft.com/en-us/windows-hardware/test/wpt/"
    },
    [pscustomobject]@{
        id = "pix-on-windows"
        backend = "d3d12"
        host = "windows"
        artifact = "pix-capture-summary.txt"
        sourceId = "microsoft.pix-on-windows"
        sourceUrl = "https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/general/pix-overview"
    },
    [pscustomobject]@{
        id = "vulkan-timestamp-debug-utils"
        backend = "vulkan"
        host = "vulkan-strict"
        artifact = "timestamp-debug-utils.json"
        sourceId = "khronos.vulkan-timestamp-debug-utils"
        sourceUrl = "https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html"
    },
    [pscustomobject]@{
        id = "apple-xctrace-metal-capture"
        backend = "metal"
        host = "macos-apple-metal"
        artifact = "xctrace-toc.xml"
        sourceId = "apple.xctrace-metal-capture"
        sourceUrl = "https://developer.apple.com/documentation/xcode/recording-performance-data"
    },
    [pscustomobject]@{
        id = "linux-perf"
        backend = "linux-perf"
        host = "linux"
        artifact = "perf-report.txt"
        sourceId = "linux.perf"
        sourceUrl = "https://perf.wiki.kernel.org/index.php/Main_Page"
    }
)

if ([string]::IsNullOrWhiteSpace($ArtifactRootRelative)) {
    if ($SyntheticSmoke.IsPresent) {
        $ArtifactRootRelative = "out/host-artifacts/2d-commercial-profiler-artifact-collectors/synthetic"
    } else {
        $ArtifactRootRelative = "artifacts/performance/2d-commercial-profiler-artifact-collectors"
    }
}

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
    param([AllowNull()][string]$RelativePath)

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

function Test-PathUnderRoot {
    param(
        [Parameter(Mandatory = $true)][string]$FullPath,
        [Parameter(Mandatory = $true)][string]$RootFull
    )

    $normalizedRoot = [System.IO.Path]::GetFullPath($RootFull).TrimEnd(
        [char[]]@([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
    $prefix = $normalizedRoot + [System.IO.Path]::DirectorySeparatorChar
    $normalizedPath = [System.IO.Path]::GetFullPath($FullPath)
    return $normalizedPath -eq $normalizedRoot -or
        $normalizedPath.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)
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

function Test-Sha256Text {
    param([AllowNull()][string]$Value)

    return -not [string]::IsNullOrWhiteSpace($Value) -and $Value -cmatch "^[0-9a-f]{64}$"
}

function Add-Diagnostic {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics,
        [Parameter(Mandatory = $true)][string]$Code
    )

    $Diagnostics.Add($Code) | Out-Null
}

function Test-FalseClaim {
    param(
        [AllowNull()]$Claims,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $value = Get-JsonPropertyValue -JsonObject $Claims -Name $Name
    return -not [bool]$value
}

function New-SyntheticArtifactText {
    param([Parameter(Mandatory = $true)]$ToolRow)

    return @(
        "schema=$schemaId",
        "claim_id=$claimId",
        "tool_id=$($ToolRow.id)",
        "backend_id=$($ToolRow.backend)",
        "host_class_id=$($ToolRow.host)",
        "workload_id=sample_2d_commercial_performance_regression",
        "official_source_id=$($ToolRow.sourceId)",
        "official_source_url=$($ToolRow.sourceUrl)",
        "synthetic_fixture=1",
        "external_engine_compatibility=0",
        "legal_approval=0"
    ) -join "`n"
}

function Write-SyntheticSmokeFixture {
    param(
        [Parameter(Mandatory = $true)][string]$ArtifactRootRelativeValue,
        [Parameter(Mandatory = $true)][string]$ArtifactRootFull
    )

    if (-not (Test-SafeRepoRelativePath -RelativePath $ArtifactRootRelativeValue)) {
        Write-Error "ArtifactRootRelative must be a safe repository-relative path."
    }
    if (-not (Test-PathUnderRoot -FullPath $ArtifactRootFull -RootFull (Join-Path $root "out/host-artifacts"))) {
        Write-Error "SyntheticSmoke ArtifactRootRelative must stay under out/host-artifacts."
    }

    $null = New-Item -ItemType Directory -Force -Path $ArtifactRootFull
    $rows = [System.Collections.Generic.List[object]]::new()
    foreach ($toolRow in $requiredToolRows) {
        $toolDirectoryRelative = "$ArtifactRootRelativeValue/$($toolRow.id)"
        $toolDirectoryFull = [System.IO.Path]::GetFullPath((Join-Path $root $toolDirectoryRelative))
        $null = New-Item -ItemType Directory -Force -Path $toolDirectoryFull
        $artifactRelative = "$toolDirectoryRelative/$($toolRow.artifact)"
        $artifactFull = [System.IO.Path]::GetFullPath((Join-Path $root $artifactRelative))
        Set-Content -LiteralPath $artifactFull -Value (New-SyntheticArtifactText -ToolRow $toolRow) -Encoding utf8NoBOM
        $hash = (Get-FileHash -LiteralPath $artifactFull -Algorithm SHA256).Hash.ToLowerInvariant()

        $rows.Add([ordered]@{
                id = "$($toolRow.id)-sample-2d"
                tool_id = $toolRow.id
                host_class_id = $toolRow.host
                backend_id = $toolRow.backend
                workload_id = "sample_2d_commercial_performance_regression"
                artifact_path = $artifactRelative
                artifact_sha256 = $hash
                official_source_id = $toolRow.sourceId
                official_source_url = $toolRow.sourceUrl
                ready = $true
                retained_artifact = $true
                official_tool = $true
                public_docs_only = $true
                command_line_reviewed = $true
                host_owned_capture = $true
                package_counter_linked = $true
                synthetic_fixture = $true
                claims = [ordered]@{
                    broad_optimization = $false
                    cross_vendor_parity = $false
                    cross_backend_parity = $false
                    public_native_handles = $false
                    external_engine_compatibility = $false
                    legal_approval = $false
                }
            }) | Out-Null
    }

    $manifest = [ordered]@{
        schema = $schemaId
        claim_id = $claimId
        validation_recipe = "2d-commercial-profiler-artifact-collectors"
        artifact_root = $ArtifactRootRelativeValue
        capture_policy = "host-owned-official-tools"
        rows = @($rows)
    }
    $manifest | ConvertTo-Json -Depth 8 |
        Set-Content -LiteralPath (Join-Path $ArtifactRootFull $manifestFileName) -Encoding utf8NoBOM
}

function Test-CollectorRow {
    param(
        [Parameter(Mandatory = $true)]$Row,
        [Parameter(Mandatory = $true)]$RequiredTools,
        [Parameter(Mandatory = $true)][string]$ArtifactRootFull
    )

    $diagnostics = [System.Collections.Generic.List[string]]::new()
    $toolId = [string](Get-JsonPropertyValue -JsonObject $Row -Name "tool_id")
    if (-not $RequiredTools.Contains($toolId)) {
        Add-Diagnostic $diagnostics "unknown_tool_id"
    }

    foreach ($field in @("id", "host_class_id", "backend_id", "workload_id", "artifact_path", "artifact_sha256",
            "official_source_id", "official_source_url")) {
        if ([string]::IsNullOrWhiteSpace([string](Get-JsonPropertyValue -JsonObject $Row -Name $field))) {
            Add-Diagnostic $diagnostics "missing_$field"
        }
    }

    foreach ($flag in @("ready", "retained_artifact", "official_tool", "public_docs_only",
            "command_line_reviewed", "host_owned_capture", "package_counter_linked")) {
        if (-not [bool](Get-JsonPropertyValue -JsonObject $Row -Name $flag)) {
            Add-Diagnostic $diagnostics "missing_$flag"
        }
    }

    $claims = Get-JsonPropertyValue -JsonObject $Row -Name "claims"
    if ($null -eq $claims) {
        Add-Diagnostic $diagnostics "missing_claims"
    }
    foreach ($claim in @("broad_optimization", "cross_vendor_parity", "cross_backend_parity",
            "public_native_handles", "external_engine_compatibility", "legal_approval")) {
        if (-not (Test-FalseClaim -Claims $claims -Name $claim)) {
            Add-Diagnostic $diagnostics $claim
        }
    }

    $artifactRelative = [string](Get-JsonPropertyValue -JsonObject $Row -Name "artifact_path")
    $artifactFull = ""
    if (-not (Test-SafeRepoRelativePath -RelativePath $artifactRelative)) {
        Add-Diagnostic $diagnostics "unsafe_artifact_path"
    } else {
        $artifactFull = [System.IO.Path]::GetFullPath((Join-Path $root $artifactRelative))
        if (-not (Test-PathUnderRoot -FullPath $artifactFull -RootFull $ArtifactRootFull)) {
            Add-Diagnostic $diagnostics "artifact_path_escape"
        } elseif (-not (Test-Path -LiteralPath $artifactFull -PathType Leaf)) {
            Add-Diagnostic $diagnostics "missing_artifact_file"
        } else {
            $expectedHash = [string](Get-JsonPropertyValue -JsonObject $Row -Name "artifact_sha256")
            if (-not (Test-Sha256Text -Value $expectedHash)) {
                Add-Diagnostic $diagnostics "invalid_artifact_sha256"
            } else {
                $actualHash = (Get-FileHash -LiteralPath $artifactFull -Algorithm SHA256).Hash.ToLowerInvariant()
                if ($actualHash -cne $expectedHash) {
                    Add-Diagnostic $diagnostics "artifact_hash_mismatch"
                }
            }
        }
    }

    return [pscustomobject]@{
        ToolId = $toolId
        Diagnostics = @($diagnostics)
        Ready = $diagnostics.Count -eq 0
        RetainedArtifact = [bool](Get-JsonPropertyValue -JsonObject $Row -Name "retained_artifact")
        OfficialTool = [bool](Get-JsonPropertyValue -JsonObject $Row -Name "official_tool")
        PublicDocsOnly = [bool](Get-JsonPropertyValue -JsonObject $Row -Name "public_docs_only")
        PackageCounterLinked = [bool](Get-JsonPropertyValue -JsonObject $Row -Name "package_counter_linked")
    }
}

$artifactRootFull = [System.IO.Path]::GetFullPath((Join-Path $root $ArtifactRootRelative))
if (-not (Test-SafeRepoRelativePath -RelativePath $ArtifactRootRelative)) {
    Write-Error "ArtifactRootRelative must be a safe repository-relative path."
}

if ($SyntheticSmoke.IsPresent) {
    Write-SyntheticSmokeFixture -ArtifactRootRelativeValue $ArtifactRootRelative -ArtifactRootFull $artifactRootFull
}

$manifestFiles = @()
if (Test-Path -LiteralPath $artifactRootFull -PathType Container) {
    $manifestFiles = @(Get-ChildItem -LiteralPath $artifactRootFull -Recurse -File -Filter $manifestFileName)
}

$requiredToolSet = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
foreach ($toolRow in $requiredToolRows) {
    $null = $requiredToolSet.Add([string]$toolRow.id)
}
$seenToolSet = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)

$manifestRows = 0
$artifactRows = 0
$readyRows = 0
$retainedArtifactRows = 0
$officialToolRows = 0
$publicDocsRows = 0
$packageCounterLinkedRows = 0
$diagnosticRows = 0
$invalidJsonRows = 0
$duplicateToolRows = 0
$missingArtifactRows = 0
$hashMismatchRows = 0
$unsafePathRows = 0
$externalEngineClaimRows = 0
$legalApprovalClaimRows = 0
$broadOptimizationClaimRows = 0
$publicNativeHandleRows = 0
$crossBackendParityClaimRows = 0
$crossVendorParityClaimRows = 0

foreach ($manifestFile in $manifestFiles) {
    $manifestRows += 1
    try {
        $manifest = Get-Content -LiteralPath $manifestFile.FullName -Raw | ConvertFrom-Json
    } catch {
        $invalidJsonRows += 1
        continue
    }

    if ([string](Get-JsonPropertyValue -JsonObject $manifest -Name "schema") -cne $schemaId -or
        [string](Get-JsonPropertyValue -JsonObject $manifest -Name "claim_id") -cne $claimId) {
        $diagnosticRows += 1
        continue
    }

    foreach ($row in @((Get-JsonPropertyValue -JsonObject $manifest -Name "rows"))) {
        $artifactRows += 1
        $rowResult = Test-CollectorRow -Row $row -RequiredTools $requiredToolSet -ArtifactRootFull $artifactRootFull
        if (-not [string]::IsNullOrWhiteSpace($rowResult.ToolId)) {
            if (-not $seenToolSet.Add($rowResult.ToolId)) {
                $duplicateToolRows += 1
            }
        }
        if ($rowResult.RetainedArtifact) { $retainedArtifactRows += 1 }
        if ($rowResult.OfficialTool) { $officialToolRows += 1 }
        if ($rowResult.PublicDocsOnly) { $publicDocsRows += 1 }
        if ($rowResult.PackageCounterLinked) { $packageCounterLinkedRows += 1 }
        if ($rowResult.Ready) { $readyRows += 1 }

        foreach ($diagnostic in @($rowResult.Diagnostics)) {
            $diagnosticRows += 1
            switch ($diagnostic) {
                "missing_artifact_file" { $missingArtifactRows += 1 }
                "artifact_hash_mismatch" { $hashMismatchRows += 1 }
                "unsafe_artifact_path" { $unsafePathRows += 1 }
                "artifact_path_escape" { $unsafePathRows += 1 }
                "external_engine_compatibility" { $externalEngineClaimRows += 1 }
                "legal_approval" { $legalApprovalClaimRows += 1 }
                "broad_optimization" { $broadOptimizationClaimRows += 1 }
                "public_native_handles" { $publicNativeHandleRows += 1 }
                "cross_backend_parity" { $crossBackendParityClaimRows += 1 }
                "cross_vendor_parity" { $crossVendorParityClaimRows += 1 }
            }
        }
    }
}

$missingToolRows = 0
foreach ($toolRow in $requiredToolRows) {
    if (-not $seenToolSet.Contains([string]$toolRow.id)) {
        $missingToolRows += 1
    }
}

$ready = $manifestRows -gt 0 -and
    $readyRows -eq $requiredToolRows.Count -and
    $seenToolSet.Count -eq $requiredToolRows.Count -and
    $missingToolRows -eq 0 -and
    $duplicateToolRows -eq 0 -and
    $diagnosticRows -eq 0 -and
    $invalidJsonRows -eq 0
$status = if ($ready) { "ready" } elseif ($manifestRows -eq 0) { "host_evidence_required" } else { "blocked" }

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=2d-commercial-profiler-artifact-collectors")
$lines.Add("schema=$schemaId")
$lines.Add("claim_id=$claimId")
$lines.Add("artifact_root=$ArtifactRootRelative")
$lines.Add("profiler_artifact_collector_status=$status")
$lines.Add("profiler_artifact_collector_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("profiler_artifact_collector_synthetic_fixture=$(ConvertTo-CounterBit $SyntheticSmoke.IsPresent)")
$lines.Add("profiler_artifact_collector_manifest_rows=$manifestRows")
$lines.Add("profiler_artifact_collector_artifact_rows=$artifactRows")
$lines.Add("profiler_artifact_collector_required_tool_rows=$($requiredToolRows.Count)")
$lines.Add("profiler_artifact_collector_seen_tool_rows=$($seenToolSet.Count)")
$lines.Add("profiler_artifact_collector_missing_tool_rows=$missingToolRows")
$lines.Add("profiler_artifact_collector_duplicate_tool_rows=$duplicateToolRows")
$lines.Add("profiler_artifact_collector_ready_rows=$readyRows")
$lines.Add("profiler_artifact_collector_retained_artifact_rows=$retainedArtifactRows")
$lines.Add("profiler_artifact_collector_official_tool_rows=$officialToolRows")
$lines.Add("profiler_artifact_collector_public_docs_rows=$publicDocsRows")
$lines.Add("profiler_artifact_collector_package_counter_linked_rows=$packageCounterLinkedRows")
$lines.Add("profiler_artifact_collector_diagnostic_rows=$diagnosticRows")
$lines.Add("profiler_artifact_collector_invalid_json_rows=$invalidJsonRows")
$lines.Add("profiler_artifact_collector_missing_artifact_rows=$missingArtifactRows")
$lines.Add("profiler_artifact_collector_hash_mismatch_rows=$hashMismatchRows")
$lines.Add("profiler_artifact_collector_unsafe_path_rows=$unsafePathRows")
$lines.Add("profiler_artifact_collector_external_engine_claim_rows=$externalEngineClaimRows")
$lines.Add("profiler_artifact_collector_legal_approval_claim_rows=$legalApprovalClaimRows")
$lines.Add("profiler_artifact_collector_broad_optimization_claim_rows=$broadOptimizationClaimRows")
$lines.Add("profiler_artifact_collector_public_native_handle_rows=$publicNativeHandleRows")
$lines.Add("profiler_artifact_collector_cross_backend_parity_claim_rows=$crossBackendParityClaimRows")
$lines.Add("profiler_artifact_collector_cross_vendor_parity_claim_rows=$crossVendorParityClaimRows")
foreach ($toolRow in $requiredToolRows) {
    $lines.Add("profiler_artifact_collector_tool_$($toolRow.id)=$(ConvertTo-CounterBit ($seenToolSet.Contains([string]$toolRow.id)))")
}

foreach ($expected in $ExpectedEvidenceCounters) {
    if (-not $lines.Contains($expected)) {
        Write-Error "Expected evidence counter not emitted: $expected"
    }
}

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady.IsPresent -and -not $ready) {
    Write-Error "2D commercial profiler artifact collectors are incomplete; provide retained official-tool artifact rows for Windows WPR/WPA, PIX on Windows, Vulkan timestamp/debug-utils, Apple xctrace/Metal capture, and Linux perf before profiler_artifact_collector_ready can be 1."
}

Write-Information "2d-commercial-profiler-artifact-collectors: ok" -InformationAction Continue
