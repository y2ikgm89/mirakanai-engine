#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Assemble")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/assembled",

    [string]$D3d12ArtifactRelative = "",
    [string]$VulkanStrictArtifactRelative = "",
    [string]$AppleMetalArtifactRelative = "",
    [string]$Visible3dPackageArtifactRelative = "",
    [string]$RuntimeUiPackageArtifactRelative = "",
    [string]$EnvironmentPackageArtifactRelative = "",
    [string]$GeneratedGamePackageArtifactRelative = "",
    [string]$RendererQualityMatrixArtifactRelative = "",
    [string]$ProductionVfxProfilingArtifactRelative = "",

    [string]$MetalMemoryHostEvidenceRelative = "",
    [string]$MetalMemoryResidencyArtifactRelative = "",
    [string]$MetalProfilingCaptureArtifactRelative = "",

    [switch]$OfficialDocsOnlyReviewReady,
    [switch]$LegalReviewReady,
    [switch]$ExternalEngineZeroMaterialReviewReady,
    [switch]$ThirdPartyNoticesComplete,
    [switch]$AllowFixtureArtifactsForSelfTest,
    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

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

function ConvertTo-RepoRelativePath {
    param([Parameter(Mandatory = $true)][string]$FullPath)

    $rootFull = [System.IO.Path]::GetFullPath($root).TrimEnd([System.IO.Path]::DirectorySeparatorChar)
    $normalizedFullPath = [System.IO.Path]::GetFullPath($FullPath)
    return ([System.IO.Path]::GetRelativePath($rootFull, $normalizedFullPath) -replace "\\", "/")
}

function Resolve-RepoRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not (Test-SafeRepoRelativePath -RelativePath $RelativePath)) {
        Write-Error "$Label must be a safe repo-relative path without absolute, drive-qualified, colon, backslash, or '..' segments."
    }
    return [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
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

function Assert-JsonStringProperty {
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

function Copy-ArtifactFile {
    param(
        [Parameter(Mandatory = $true)][string]$SourceFull,
        [Parameter(Mandatory = $true)][string]$DestinationRelative
    )

    $destinationFull = Resolve-RepoRelativePath -RelativePath $DestinationRelative -Label "DestinationRelative"
    $destinationDirectory = [System.IO.Path]::GetDirectoryName($destinationFull)
    if (-not [string]::IsNullOrWhiteSpace($destinationDirectory)) {
        $null = New-Item -ItemType Directory -Path $destinationDirectory -Force
    }
    Copy-Item -LiteralPath $SourceFull -Destination $destinationFull -Force
    return $destinationFull
}

function Copy-NestedMetalCaptureArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$SourceEvidenceFull,
        [Parameter(Mandatory = $true)]$EvidenceJson,
        [Parameter(Mandatory = $true)][string]$OutputEvidenceRelative
    )

    $captureRow = Get-JsonPropertyValue -JsonObject $EvidenceJson -Name "profiling_capture_row"
    $captureRelative = [string](Get-JsonPropertyValue -JsonObject $captureRow -Name "capture_artifact_path")
    if ([string]::IsNullOrWhiteSpace($captureRelative)) {
        Write-Error "MetalMemoryHostEvidenceRelative must contain profiling_capture_row.capture_artifact_path."
    }
    if (-not (Test-SafeRepoRelativePath -RelativePath $captureRelative)) {
        Write-Error "Metal capture artifact path must be safe and relative: $captureRelative"
    }

    $sourceDirectory = [System.IO.Path]::GetDirectoryName($SourceEvidenceFull)
    $sourceCaptureFull = [System.IO.Path]::GetFullPath((Join-Path $sourceDirectory $captureRelative))
    $sourceDirectoryWithSeparator = $sourceDirectory.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $sourceCaptureFull.StartsWith($sourceDirectoryWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "Metal capture artifact must resolve under the source evidence directory."
    }
    if (-not (Test-Path -LiteralPath $sourceCaptureFull -PathType Leaf)) {
        Write-Error "Metal capture artifact does not exist: $sourceCaptureFull"
    }

    $outputEvidenceDirectory = Split-Path -Parent $OutputEvidenceRelative
    $destinationRelative = "$outputEvidenceDirectory/$captureRelative"
    $null = Copy-ArtifactFile -SourceFull $sourceCaptureFull -DestinationRelative $destinationRelative
}

function New-RendererEvidenceRow {
    param(
        [Parameter(Mandatory = $true)][string]$EvidenceId,
        [Parameter(Mandatory = $true)][string]$Recipe,
        [Parameter(Mandatory = $true)][string]$SchemaVersion,
        [Parameter(Mandatory = $true)][string]$ArtifactPath,
        [Parameter(Mandatory = $true)][string]$ArtifactHash,
        [Parameter(Mandatory = $true)][string]$Counter
    )

    return [ordered]@{
        evidence_id = $EvidenceId
        selected = $true
        ready = $true
        host_validation_recipe_id = $Recipe
        retained_artifact_schema_version = $SchemaVersion
        artifact_path = $ArtifactPath
        artifact_hash_sha256 = $ArtifactHash
        validation_counter_id = $Counter
        diagnostic_code = "ready"
    }
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/commercial-readiness-evidence/."
}

$requiredArtifactRows = 11
$willWrite = $Mode -eq "Assemble" -and -not $NoWrite.IsPresent

if ($Mode -eq "Plan") {
    Write-Output "renderer_commercial_readiness_evidence_collector_mode=Plan"
    Write-Output "renderer_commercial_readiness_evidence_collector_writes_evidence=0"
    Write-Output "renderer_commercial_readiness_evidence_collector_required_artifact_rows=$requiredArtifactRows"
    Write-Output "renderer_commercial_readiness_evidence_collector_commercial_renderer=0"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    return
}

foreach ($reviewSwitch in @(
        @{ Ready = [bool]$OfficialDocsOnlyReviewReady; Name = "official_docs_review_required" },
        @{ Ready = [bool]$LegalReviewReady; Name = "legal_review_required" },
        @{ Ready = [bool]$ExternalEngineZeroMaterialReviewReady; Name = "external_engine_zero_material_review_required" },
        @{ Ready = [bool]$ThirdPartyNoticesComplete; Name = "third_party_notices_required" }
    )) {
    if (-not [bool]$reviewSwitch.Ready) {
        Write-Error $reviewSwitch.Name
    }
}

$outputRootFull = Resolve-RepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
if ($willWrite) {
    $null = New-Item -ItemType Directory -Path $outputRootFull -Force
}

$fixtureArtifactCount = 0
$copiedArtifactRows = @{}
$rowSpecs = @(
    @{
        Name = "d3d12"
        Input = $D3d12ArtifactRelative
        Output = "$OutputRootRelative/d3d12-quality.json"
        Schema = "GameEngine.RendererCommercialQualityCloseout.v1"
        ArtifactId = "d3d12-quality"
        Recipe = "renderer-d3d12-quality-evidence"
        Counter = "renderer_d3d12_renderer_quality_ready"
    },
    @{
        Name = "vulkan_strict"
        Input = $VulkanStrictArtifactRelative
        Output = "$OutputRootRelative/vulkan-strict-quality.json"
        Schema = "GameEngine.RendererCommercialQualityCloseout.v1"
        ArtifactId = "vulkan-strict-quality"
        Recipe = "renderer-vulkan-strict-quality-evidence"
        Counter = "renderer_vulkan_strict_renderer_quality_ready"
    },
    @{
        Name = "apple_metal"
        Input = $AppleMetalArtifactRelative
        Output = "$OutputRootRelative/apple-metal-host.json"
        Schema = "GameEngine.RendererCommercialQualityCloseout.v1"
        ArtifactId = "apple-metal-host"
        Recipe = "renderer-metal-apple-host-evidence"
        Counter = "renderer_apple_metal_renderer_quality_ready"
    },
    @{
        Name = "visible_3d"
        Input = $Visible3dPackageArtifactRelative
        Output = "$OutputRootRelative/visible-3d-package.json"
        Schema = "GameEngine.DesktopRuntimePackageEvidence.v1"
        ArtifactId = "visible-3d-package"
        Recipe = "desktop-3d-package"
        Counter = "renderer_visible_3d_package_ready"
    },
    @{
        Name = "runtime_ui"
        Input = $RuntimeUiPackageArtifactRelative
        Output = "$OutputRootRelative/runtime-ui-package.json"
        Schema = "GameEngine.DesktopRuntimePackageEvidence.v1"
        ArtifactId = "runtime-ui-package"
        Recipe = "desktop-runtime-ui-package"
        Counter = "renderer_runtime_ui_package_ready"
    },
    @{
        Name = "environment"
        Input = $EnvironmentPackageArtifactRelative
        Output = "$OutputRootRelative/environment-package.json"
        Schema = "GameEngine.EnvironmentPackageEvidence.v1"
        ArtifactId = "environment-package"
        Recipe = "environment-package"
        Counter = "renderer_environment_package_ready"
    },
    @{
        Name = "generated_game"
        Input = $GeneratedGamePackageArtifactRelative
        Output = "$OutputRootRelative/generated-game-package.json"
        Schema = "GameEngine.GeneratedGamePackageEvidence.v1"
        ArtifactId = "generated-game-package"
        Recipe = "generated-game-package"
        Counter = "renderer_generated_game_package_ready"
    },
    @{
        Name = "renderer_quality_matrix"
        Input = $RendererQualityMatrixArtifactRelative
        Output = "$OutputRootRelative/renderer-quality-matrix.json"
        Schema = "GameEngine.RendererCommercialQualityCloseout.v1"
        ArtifactId = "renderer-quality-matrix"
        Recipe = "renderer-quality-matrix"
        Counter = "renderer_quality_matrix_ready"
    },
    @{
        Name = "production_vfx_profiling"
        Input = $ProductionVfxProfilingArtifactRelative
        Output = "$OutputRootRelative/production-vfx-profiling.json"
        Schema = "GameEngine.RendererCommercialQualityCloseout.v1"
        ArtifactId = "production-vfx-profiling"
        Recipe = "renderer-production-vfx-profiling"
        Counter = "renderer_production_vfx_profiling_ready"
    }
)

foreach ($rowSpec in $rowSpecs) {
    if ([string]::IsNullOrWhiteSpace([string]$rowSpec.Input)) {
        Write-Error "$($rowSpec.Name)_artifact_required"
    }

    $sourceFull = Resolve-RepoRelativePath -RelativePath $rowSpec.Input -Label "$($rowSpec.Name) artifact"
    $json = Read-JsonFile -Path $sourceFull -Label "$($rowSpec.Name) artifact"
    Assert-JsonStringProperty -JsonObject $json -Name "schema_version" -Expected $rowSpec.Schema `
        -Label $rowSpec.Name
    Assert-JsonStringProperty -JsonObject $json -Name "artifact_id" -Expected $rowSpec.ArtifactId `
        -Label $rowSpec.Name
    Assert-JsonStringProperty -JsonObject $json -Name "validation_recipe" -Expected $rowSpec.Recipe `
        -Label $rowSpec.Name

    $fixtureOnlyValue = Get-JsonPropertyValue -JsonObject $json -Name "fixture_only"
    if ([bool]$fixtureOnlyValue) {
        $fixtureArtifactCount += 1
        if (-not $AllowFixtureArtifactsForSelfTest) {
            Write-Error "fixture_artifact_input_rejected: $($rowSpec.Input)"
        }
    }

    $destinationFull = $sourceFull
    if ($willWrite) {
        $destinationFull = Copy-ArtifactFile -SourceFull $sourceFull -DestinationRelative $rowSpec.Output
    }
    $hash = (Get-FileHash -LiteralPath $destinationFull -Algorithm SHA256).Hash.ToLowerInvariant()
    $copiedArtifactRows[$rowSpec.Name] = New-RendererEvidenceRow `
        -EvidenceId $rowSpec.Name `
        -Recipe $rowSpec.Recipe `
        -SchemaVersion $rowSpec.Schema `
        -ArtifactPath $rowSpec.Output `
        -ArtifactHash $hash `
        -Counter $rowSpec.Counter
    Write-Output "renderer_commercial_readiness_evidence_collector_$($rowSpec.Name)_hash=$hash"
}

$metalRows = @{}
$usingFullMetalHostEvidence = -not [string]::IsNullOrWhiteSpace($MetalMemoryHostEvidenceRelative)
if ($usingFullMetalHostEvidence) {
    $metalSourceFull = Resolve-RepoRelativePath `
        -RelativePath $MetalMemoryHostEvidenceRelative `
        -Label "MetalMemoryHostEvidenceRelative"
    $metalJson = Read-JsonFile -Path $metalSourceFull -Label "MetalMemoryHostEvidenceRelative"
    Assert-JsonStringProperty -JsonObject $metalJson -Name "schema_version" `
        -Expected "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1" `
        -Label "MetalMemoryHostEvidenceRelative"
    Assert-JsonStringProperty -JsonObject $metalJson -Name "claim_id" `
        -Expected "renderer-metal-memory-profiling-host-evidence-v1" `
        -Label "MetalMemoryHostEvidenceRelative"

    $metalOutputRelative = "$OutputRootRelative/metal-memory-profiling-host-evidence/evidence.json"
    $metalDestinationFull = $metalSourceFull
    if ($willWrite) {
        $metalDestinationFull = Copy-ArtifactFile `
            -SourceFull $metalSourceFull `
            -DestinationRelative $metalOutputRelative
        Copy-NestedMetalCaptureArtifact `
            -SourceEvidenceFull $metalSourceFull `
            -EvidenceJson $metalJson `
            -OutputEvidenceRelative $metalOutputRelative
    }
    $metalHash = (Get-FileHash -LiteralPath $metalDestinationFull -Algorithm SHA256).Hash.ToLowerInvariant()
    foreach ($metalRow in @(
            @{ Name = "memory_residency"; Counter = "renderer_metal_memory_profiling_ready" },
            @{ Name = "profiling_capture"; Counter = "renderer_metal_memory_profiling_ready" }
        )) {
        $metalRows[$metalRow.Name] = New-RendererEvidenceRow `
            -EvidenceId $metalRow.Name `
            -Recipe "renderer-metal-memory-profiling-host-evidence" `
            -SchemaVersion "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1" `
            -ArtifactPath $metalOutputRelative `
            -ArtifactHash $metalHash `
            -Counter $metalRow.Counter
    }
    Write-Output "renderer_commercial_readiness_evidence_collector_metal_memory_host_hash=$metalHash"
} else {
    foreach ($metalFixtureSpec in @(
            @{
                Name = "memory_residency"
                Input = $MetalMemoryResidencyArtifactRelative
                Output = "$OutputRootRelative/metal-memory-residency.json"
                ArtifactId = "metal-memory-residency"
            },
            @{
                Name = "profiling_capture"
                Input = $MetalProfilingCaptureArtifactRelative
                Output = "$OutputRootRelative/metal-profiling-capture.json"
                ArtifactId = "metal-profiling-capture"
            }
        )) {
        if ([string]::IsNullOrWhiteSpace([string]$metalFixtureSpec.Input)) {
            Write-Error "$($metalFixtureSpec.Name)_artifact_required"
        }
        if (-not $AllowFixtureArtifactsForSelfTest) {
            Write-Error "metal_memory_host_evidence_required"
        }

        $sourceFull = Resolve-RepoRelativePath `
            -RelativePath $metalFixtureSpec.Input `
            -Label "$($metalFixtureSpec.Name) fixture artifact"
        $json = Read-JsonFile -Path $sourceFull -Label "$($metalFixtureSpec.Name) fixture artifact"
        Assert-JsonStringProperty -JsonObject $json -Name "schema_version" `
            -Expected "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1" `
            -Label $metalFixtureSpec.Name
        Assert-JsonStringProperty -JsonObject $json -Name "artifact_id" `
            -Expected $metalFixtureSpec.ArtifactId `
            -Label $metalFixtureSpec.Name
        Assert-JsonStringProperty -JsonObject $json -Name "validation_recipe" `
            -Expected "renderer-metal-memory-profiling-host-evidence" `
            -Label $metalFixtureSpec.Name

        $fixtureOnlyValue = Get-JsonPropertyValue -JsonObject $json -Name "fixture_only"
        if ([bool]$fixtureOnlyValue) {
            $fixtureArtifactCount += 1
        } else {
            Write-Error "split metal memory artifacts are reserved for fixture self-tests."
        }

        $destinationFull = $sourceFull
        if ($willWrite) {
            $destinationFull = Copy-ArtifactFile `
                -SourceFull $sourceFull `
                -DestinationRelative $metalFixtureSpec.Output
        }
        $hash = (Get-FileHash -LiteralPath $destinationFull -Algorithm SHA256).Hash.ToLowerInvariant()
        $metalRows[$metalFixtureSpec.Name] = New-RendererEvidenceRow `
            -EvidenceId $metalFixtureSpec.Name `
            -Recipe "renderer-metal-memory-profiling-host-evidence" `
            -SchemaVersion "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1" `
            -ArtifactPath $metalFixtureSpec.Output `
            -ArtifactHash $hash `
            -Counter "renderer_metal_memory_profiling_ready"
        Write-Output "renderer_commercial_readiness_evidence_collector_$($metalFixtureSpec.Name)_hash=$hash"
    }
}

$evidence = [ordered]@{
    schema_version = "GameEngine.RendererCommercialReadinessEvidence.v1"
    claim_id = "renderer-commercial-readiness-evidence-promotion-v1"
    source_rows = [ordered]@{
        d3d12_documentation_source_id = "Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25"
        vulkan_documentation_source_id = "Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25"
        metal_framework_source_id = "Apple-Metal-Framework-Memory-Capture-2026-06-25"
        metal_shading_language_source_id = "Apple-Metal-Shading-Language-Specification-2026-06-25"
        unity_legal_source_id = "Unity-Legal-Terms-2026-06-25"
        unreal_legal_source_id = "Epic-Unreal-Engine-EULA-Trademark-2026-06-25"
        godot_legal_source_id = "Godot-Trademark-Licensing-2026-06-25"
        json_schema_source_id = "JSON-Schema-Draft-2020-12-2026-06-25"
        context7_review_source_id = "Context7-Renderer-Commercial-Readiness-Docs-2026-06-25"
    }
    backend_rows = [ordered]@{
        d3d12 = $copiedArtifactRows["d3d12"]
        vulkan_strict = $copiedArtifactRows["vulkan_strict"]
        apple_metal = $copiedArtifactRows["apple_metal"]
    }
    package_rows = [ordered]@{
        visible_3d = $copiedArtifactRows["visible_3d"]
        runtime_ui = $copiedArtifactRows["runtime_ui"]
        environment = $copiedArtifactRows["environment"]
        generated_game = $copiedArtifactRows["generated_game"]
    }
    quality_rows = [ordered]@{
        renderer_quality_matrix = $copiedArtifactRows["renderer_quality_matrix"]
        production_vfx_profiling = $copiedArtifactRows["production_vfx_profiling"]
    }
    metal_memory_profiling_rows = [ordered]@{
        memory_residency = $metalRows["memory_residency"]
        profiling_capture = $metalRows["profiling_capture"]
    }
    clean_room_rows = [ordered]@{
        official_docs_only = [ordered]@{
            ready = $true
            public_documentation_only = $true
            context7_verified = $true
            official_fallback_documented = $true
            external_engine_source_review_complete = $true
        }
        legal_review = [ordered]@{
            ready = $true
            unity_terms_reviewed = $true
            unreal_eula_trademark_reviewed = $true
            godot_trademark_reviewed = $true
            unity_compatibility = $false
            unreal_compatibility = $false
            godot_compatibility = $false
            compatibility_claims = $false
            equivalence_claims = $false
            parity_claims = $false
        }
        external_engine_zero_material_review = [ordered]@{
            ready = $true
            external_engine_code_used = $false
            external_engine_sample_used = $false
            external_engine_shader_used = $false
            external_engine_asset_used = $false
            external_engine_trademark_used = $false
            external_engine_ui_expression_used = $false
            external_engine_project_import_used = $false
            external_engine_api_used = $false
            external_engine_compatibility_claim = $false
            external_engine_equivalence_claim = $false
            external_engine_parity_claim = $false
        }
        third_party_notices = [ordered]@{
            ready = $true
            complete = $true
            notices_path = "THIRD_PARTY_NOTICES.md"
        }
        forbidden_material_rows = @()
    }
    expected_counters = [ordered]@{
        renderer_backend_parity_ready = $true
        renderer_metal_broad_readiness = $true
        renderer_broad_quality_ready = $true
        renderer_commercial_readiness = $true
    }
    non_claims = [ordered]@{
        environment_ready = $false
        native_handles_exposed = $false
        cross_backend_inference = $false
        external_engine_parity = $false
        external_engine_code_used = $false
        external_engine_sample_used = $false
        external_engine_shader_used = $false
        external_engine_asset_used = $false
        external_engine_trademark_used = $false
        external_engine_ui_expression_used = $false
        external_engine_project_import_used = $false
        external_engine_api_used = $false
        external_engine_compatibility_claim = $false
        external_engine_equivalence_claim = $false
        external_engine_parity_claim = $false
        unity_compatibility = $false
        unreal_compatibility = $false
        godot_compatibility = $false
    }
}

$evidencePathRelative = "$OutputRootRelative/evidence.json"
if ($willWrite) {
    $evidenceJson = $evidence | ConvertTo-Json -Depth 24
    Set-Content -LiteralPath (Resolve-RepoRelativePath -RelativePath $evidencePathRelative -Label "evidence.json") `
        -Encoding utf8NoBOM `
        -Value $evidenceJson
}

$realPromotionCandidate = $fixtureArtifactCount -eq 0 -and
    [bool]$OfficialDocsOnlyReviewReady -and
    [bool]$LegalReviewReady -and
    [bool]$ExternalEngineZeroMaterialReviewReady -and
    [bool]$ThirdPartyNoticesComplete

Write-Output "renderer_commercial_readiness_evidence_collector_mode=Assemble"
Write-Output "renderer_commercial_readiness_evidence_collector_output_root=$OutputRootRelative"
Write-Output "renderer_commercial_readiness_evidence_collector_evidence_json=$evidencePathRelative"
Write-Output "renderer_commercial_readiness_evidence_collector_writes_evidence=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_commercial_readiness_evidence_collector_written=$(ConvertTo-CounterBit $willWrite)"
Write-Output "renderer_commercial_readiness_evidence_collector_artifact_rows=$requiredArtifactRows"
Write-Output "renderer_commercial_readiness_evidence_collector_fixture_artifacts=$fixtureArtifactCount"
Write-Output "renderer_commercial_readiness_evidence_collector_real_promotion_candidate=$(ConvertTo-CounterBit $realPromotionCandidate)"
Write-Output "renderer_commercial_readiness_evidence_collector_native_handles_exposed=0"
Write-Output "renderer_commercial_readiness_evidence_collector_cross_backend_inference=0"
Write-Output "renderer_commercial_readiness_evidence_collector_external_engine_parity=0"
Write-Output "renderer_commercial_readiness_evidence_collector_external_engine_material_rows=0"
Write-Output "renderer_commercial_readiness_evidence_collector_commercial_renderer=0"
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
