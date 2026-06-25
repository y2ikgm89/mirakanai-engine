#requires -Version 7.0
#requires -PSEdition Core
# Chapter 74 for check-json-contracts.ps1 renderer commercial readiness evidence bundle schema.

$schemaText = Get-JsonContractSurfaceText "schemas/renderer-commercial-readiness-evidence.schema.json"
$readyFixtureText = Get-JsonContractSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/ready/evidence.json"
$missingMetalFixtureText = Get-JsonContractSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/missing_metal/evidence.json"
$externalEngineRejectedFixtureText = Get-JsonContractSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/external_engine_rejected/evidence.json"
$schema = $schemaText | ConvertFrom-Json
$readyFixture = $readyFixtureText | ConvertFrom-Json
$missingMetalFixture = $missingMetalFixtureText | ConvertFrom-Json
$externalEngineRejectedFixture = $externalEngineRejectedFixtureText | ConvertFrom-Json

function Assert-RendererCommercialReadinessPropertySet {
    param(
        [Parameter(Mandatory = $true)] [object] $Object,
        [Parameter(Mandatory = $true)] [string[]] $ExpectedProperties,
        [Parameter(Mandatory = $true)] [string] $Label
    )

    $actualProperties = @($Object.PSObject.Properties.Name)
    foreach ($expectedProperty in $ExpectedProperties) {
        if ($actualProperties -notcontains $expectedProperty) {
            Write-Error "$Label missing required property: $expectedProperty"
        }
    }
    foreach ($actualProperty in $actualProperties) {
        if ($ExpectedProperties -notcontains $actualProperty) {
            Write-Error "$Label has unexpected property: $actualProperty"
        }
    }
}

function Get-RendererCommercialReadinessEvidenceRows {
    param([Parameter(Mandatory = $true)] [object] $Fixture)

    return @(
        $Fixture.backend_rows.d3d12,
        $Fixture.backend_rows.vulkan_strict,
        $Fixture.backend_rows.apple_metal,
        $Fixture.package_rows.visible_3d,
        $Fixture.package_rows.runtime_ui,
        $Fixture.package_rows.environment,
        $Fixture.package_rows.generated_game,
        $Fixture.quality_rows.renderer_quality_matrix,
        $Fixture.quality_rows.production_vfx_profiling,
        $Fixture.metal_memory_profiling_rows.memory_residency,
        $Fixture.metal_memory_profiling_rows.profiling_capture
    )
}

foreach ($needle in @(
        "https://json-schema.org/draft/2020-12/schema",
        "GameEngine.RendererCommercialReadinessEvidence.v1",
        "renderer-commercial-readiness-evidence-promotion-v1",
        "Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25",
        "Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25",
        "Apple-Metal-Framework-Memory-Capture-2026-06-25",
        "Apple-Metal-Shading-Language-Specification-2026-06-25",
        "Unity-Legal-Terms-2026-06-25",
        "Epic-Unreal-Engine-EULA-Trademark-2026-06-25",
        "Godot-Trademark-Licensing-2026-06-25",
        "JSON-Schema-Draft-2020-12-2026-06-25",
        "additionalProperties",
        "artifact_path",
        "artifact_hash_sha256",
        "^[0-9a-f]{64}$",
        "^(artifacts/renderer/commercial-readiness-evidence|tests/fixtures/renderer/commercial-readiness-evidence)/",
        "d3d12",
        "vulkan_strict",
        "apple_metal",
        "visible_3d",
        "runtime_ui",
        "environment",
        "generated_game",
        "renderer_quality_matrix",
        "production_vfx_profiling",
        "memory_residency",
        "profiling_capture",
        "official_docs_only",
        "third_party_notices",
        "forbidden_material_rows",
        "external_engine_code_used",
        "external_engine_sample_used",
        "external_engine_asset_used",
        "external_engine_trademark_used",
        "external_engine_ui_expression_used",
        "unity_compatibility",
        "unreal_compatibility",
        "godot_compatibility",
        "renderer_commercial_readiness"
    )) {
    Assert-ContainsText $schemaText $needle "renderer commercial readiness evidence schema"
}

if ($schema.properties.schema_version.const -ne "GameEngine.RendererCommercialReadinessEvidence.v1") {
    Write-Error "renderer commercial readiness evidence schema_version must be const v1"
}
if ($schema.properties.claim_id.const -ne "renderer-commercial-readiness-evidence-promotion-v1") {
    Write-Error "renderer commercial readiness evidence claim_id must be const promotion v1"
}
foreach ($topLevelProperty in @("source_rows", "backend_rows", "package_rows", "quality_rows", "metal_memory_profiling_rows", "clean_room_rows", "expected_counters", "non_claims")) {
    if (@($schema.required) -notcontains $topLevelProperty) {
        Write-Error "renderer commercial readiness evidence schema missing required top-level property: $topLevelProperty"
    }
}
foreach ($fixture in @(
        @{ Text = $readyFixtureText; Label = "ready fixture"; RequiredNeedle = '"renderer_commercial_readiness": true' },
        @{ Text = $missingMetalFixtureText; Label = "missing Metal fixture"; RequiredNeedle = "missing_metal_memory_profiling" },
        @{ Text = $externalEngineRejectedFixtureText; Label = "external engine rejected fixture"; RequiredNeedle = "external_engine_material_rejected" }
    )) {
    foreach ($needle in @(
            "GameEngine.RendererCommercialReadinessEvidence.v1",
            "renderer-commercial-readiness-evidence-promotion-v1",
            "d3d12",
            "vulkan_strict",
            "apple_metal",
            "visible_3d",
            "runtime_ui",
            "environment",
            "generated_game",
            "renderer_quality_matrix",
            "production_vfx_profiling",
            "memory_residency",
            "profiling_capture",
            "native_handles_exposed",
            "cross_backend_inference",
            "external_engine_parity"
        )) {
        Assert-ContainsText $fixture.Text $needle "renderer commercial readiness evidence $($fixture.Label)"
    }
    Assert-ContainsText $fixture.Text $fixture.RequiredNeedle "renderer commercial readiness evidence $($fixture.Label)"
}

foreach ($fixture in @(
        @{ Object = $readyFixture; Label = "ready fixture" },
        @{ Object = $missingMetalFixture; Label = "missing Metal fixture" },
        @{ Object = $externalEngineRejectedFixture; Label = "external engine rejected fixture" }
    )) {
    Assert-RendererCommercialReadinessPropertySet $fixture.Object.backend_rows @("d3d12", "vulkan_strict", "apple_metal") "$($fixture.Label) backend_rows"
    Assert-RendererCommercialReadinessPropertySet $fixture.Object.package_rows @("visible_3d", "runtime_ui", "environment", "generated_game") "$($fixture.Label) package_rows"
    Assert-RendererCommercialReadinessPropertySet $fixture.Object.quality_rows @("renderer_quality_matrix", "production_vfx_profiling") "$($fixture.Label) quality_rows"
    Assert-RendererCommercialReadinessPropertySet $fixture.Object.metal_memory_profiling_rows @("memory_residency", "profiling_capture") "$($fixture.Label) metal_memory_profiling_rows"

    foreach ($row in Get-RendererCommercialReadinessEvidenceRows $fixture.Object) {
        if ($true -ne [bool]$row.selected) {
            Write-Error "$($fixture.Label) row '$($row.evidence_id)' must stay selected"
        }
        if ([string]$row.artifact_path -notmatch '^(artifacts/renderer/commercial-readiness-evidence|tests/fixtures/renderer/commercial-readiness-evidence)/(?!.*\.\.)[a-z0-9_./-]+$') {
            Write-Error "$($fixture.Label) row '$($row.evidence_id)' has an unapproved artifact_path: $($row.artifact_path)"
        }
        if ([string]$row.artifact_path -match '^[A-Za-z]:|^/|\\|\.\.') {
            Write-Error "$($fixture.Label) row '$($row.evidence_id)' artifact_path must reject absolute, Windows, backslash, and parent-traversal paths"
        }
        if ([string]$row.artifact_hash_sha256 -notmatch '^[0-9a-f]{64}$') {
            Write-Error "$($fixture.Label) row '$($row.evidence_id)' must use a lowercase SHA-256 artifact hash"
        }
    }
}

if ($true -ne [bool]$readyFixture.expected_counters.renderer_commercial_readiness) {
    Write-Error "ready fixture must expect renderer_commercial_readiness=true"
}
if ($false -ne [bool]$missingMetalFixture.expected_counters.renderer_metal_broad_readiness -or
    $false -ne [bool]$missingMetalFixture.expected_counters.renderer_commercial_readiness) {
    Write-Error "missing Metal fixture must keep broad Metal and commercial readiness false"
}
if ($false -ne [bool]$externalEngineRejectedFixture.expected_counters.renderer_commercial_readiness) {
    Write-Error "external engine rejected fixture must keep renderer_commercial_readiness=false"
}
$externalEngineRejectedRows = @($externalEngineRejectedFixture.clean_room_rows.forbidden_material_rows)
if ($externalEngineRejectedRows.Count -ne 1) {
    Write-Error "external engine rejected fixture must carry exactly one forbidden material row"
}
if ($externalEngineRejectedRows[0].diagnostic_code -ne "external_engine_material_rejected" -or
    $true -ne [bool]$externalEngineRejectedRows[0].rejected -or
    $false -ne [bool]$externalEngineRejectedRows[0].used_in_engine) {
    Write-Error "external engine rejected fixture must reject forbidden material without using it in engine evidence"
}
