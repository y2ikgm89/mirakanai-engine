#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function Assert-PathUnderDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $pathFull = [System.IO.Path]::GetFullPath($Path)
    $directoryFull = [System.IO.Path]::GetFullPath($Directory).TrimEnd(
        [char[]]@([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
    $directoryPrefix = $directoryFull + [System.IO.Path]::DirectorySeparatorChar
    if ($pathFull -ne $directoryFull -and
        -not $pathFull.StartsWith($directoryPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "$Description escaped expected directory '$directoryFull': $pathFull"
    }
}

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

function Assert-LinePresent {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$ExpectedLine,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Lines.Contains($ExpectedLine)) {
        Write-Error "$Context missing expected line: $ExpectedLine"
    }
}

function Assert-TextAbsent {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if ($Text.Contains($Needle)) {
        Write-Error "$Context contained unexpected text: $Needle"
    }
}

function New-RendererCommercialReadinessEvidenceRow {
    param(
        [Parameter(Mandatory = $true)][string]$EvidenceId,
        [Parameter(Mandatory = $true)][string]$Recipe,
        [Parameter(Mandatory = $true)][string]$SchemaVersion,
        [Parameter(Mandatory = $true)][string]$ArtifactPath,
        [Parameter(Mandatory = $true)][string]$ArtifactHash,
        [Parameter(Mandatory = $true)][string]$Counter,
        [Parameter(Mandatory = $true)][bool]$Ready,
        [Parameter(Mandatory = $true)][string]$DiagnosticCode
    )

    return [ordered]@{
        evidence_id = $EvidenceId
        selected = $true
        ready = $Ready
        host_validation_recipe_id = $Recipe
        retained_artifact_schema_version = $SchemaVersion
        artifact_path = $ArtifactPath
        artifact_hash_sha256 = $ArtifactHash
        validation_counter_id = $Counter
        diagnostic_code = $DiagnosticCode
    }
}

$artifactRootRelative = "artifacts/renderer/commercial-readiness-evidence/metal-memory-full-self-test-$PID"
$artifactRoot = ConvertTo-LocalPath $artifactRootRelative
$approvedArtifactRoot = ConvertTo-LocalPath "artifacts/renderer/commercial-readiness-evidence"
$validator = Join-Path $root "tools/validate-renderer-commercial-readiness-evidence.ps1"

Assert-PathUnderDirectory -Path $artifactRoot -Directory $approvedArtifactRoot `
    -Description "Renderer commercial readiness Metal memory self-test root"

try {
    if (Test-Path -LiteralPath $artifactRoot) {
        Assert-PathUnderDirectory -Path $artifactRoot -Directory $approvedArtifactRoot `
            -Description "Renderer commercial readiness stale Metal memory self-test root"
        Remove-Item -LiteralPath $artifactRoot -Recurse -Force
    }

    $metalMemoryRoot = Join-Path $artifactRoot "metal-memory"
    $null = New-Item -ItemType Directory -Path $metalMemoryRoot -Force

    $captureArtifact = Join-Path $metalMemoryRoot "capture-summary.txt"
    Set-Content -LiteralPath $captureArtifact -Value @("capture row 1", "capture row 2") -Encoding utf8NoBOM
    $captureArtifactHash = (Get-FileHash -LiteralPath $captureArtifact -Algorithm SHA256).Hash.ToLowerInvariant()

    $hostEvidence = [ordered]@{
        schema_version = "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1"
        claim_id = "renderer-metal-memory-profiling-host-evidence-v1"
        host = [ordered]@{
            platform = "macos"
            macos_version = "15.5"
            xcode_version = "16.4"
            full_xcode_selected = $true
            metal_tool_ready = $true
            metallib_tool_ready = $true
        }
        source_rows = [ordered]@{
            heap_documentation_source_id = "Apple-Metal-MTLHeap-2026-06-24"
            residency_set_documentation_source_id = "Apple-Metal-MTLResidencySet-2026-06-24"
            residency_request_documentation_source_id = "Apple-Metal-MTLResidencySet-requestResidency-2026-06-24"
            command_queue_residency_documentation_source_id = "Apple-Metal-MTLCommandQueue-addResidencySet-2026-06-24"
            capture_manager_documentation_source_id = "Apple-Metal-MTLCaptureManager-2026-06-24"
            programmatic_capture_documentation_source_id = "Apple-Metal-ProgrammaticCapture-2026-06-24"
        }
        memory_residency_row = [ordered]@{
            proof_row_id = "memory_residency"
            host_validation_recipe_id = "renderer-metal-memory-profiling-host-evidence"
            first_party_workload_id = "renderer_metal_memory_profiling_apple_host_probe"
            runtime_ready = $true
            command_queue_ready = $true
            heap_api_name = "MTLHeap"
            heap_allocation_ready = $true
            heap_resource_allocation_ready = $true
            heap_resource_rows = 3
            heap_allocated_bytes = 1048576
            resident_bytes = 524288
            budget_bytes = 1048576
            residency_api_name = "MTLResidencySet"
            residency_set_ready = $true
            residency_set_allocation_rows = 2
            residency_request_ready = $true
            residency_commit_ready = $true
            command_queue_residency_set_committed = $true
            residency_pressure_evidence_ready = $true
            memory_pressure_sample_rows = 4
            memory_pressure_budget_status = "within_budget"
        }
        profiling_capture_row = [ordered]@{
            proof_row_id = "profiling_capture"
            host_validation_recipe_id = "renderer-metal-memory-profiling-host-evidence"
            first_party_workload_id = "renderer_metal_memory_profiling_apple_host_probe"
            runtime_ready = $true
            command_queue_ready = $true
            capture_api_name = "MTLCaptureManager"
            capture_manager_ready = $true
            capture_descriptor_ready = $true
            capture_object_ready = $true
            capture_scope_ready = $true
            capture_scope_label = "MK_renderer_metal_memory_profile"
            capture_boundary_ready = $true
            capture_started = $true
            capture_stopped = $true
            command_buffer_captured = $true
            capture_artifact_path = "capture-summary.txt"
            capture_artifact_hash_sha256 = $captureArtifactHash
            deterministic_capture_hash_sha256 = $captureArtifactHash
            capture_artifact_rows = 2
        }
        non_claims = [ordered]@{
            simulator_only_evidence = $false
            cross_backend_inference = $false
            native_handles_exposed = $false
            broad_backend_parity_ready = $false
            broad_metal_readiness = $false
            commercial_renderer_readiness = $false
            broad_renderer_quality = $false
            environment_ready = $false
            external_engine_api_parity = $false
        }
    }
    $hostEvidencePath = Join-Path $metalMemoryRoot "evidence.json"
    $hostEvidence | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $hostEvidencePath -Encoding utf8NoBOM
    $hostEvidenceHash = (Get-FileHash -LiteralPath $hostEvidencePath -Algorithm SHA256).Hash.ToLowerInvariant()
    $missingArtifactHash = "0000000000000000000000000000000000000000000000000000000000000000"

    $commercialEvidence = [ordered]@{
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
            d3d12 = New-RendererCommercialReadinessEvidenceRow -EvidenceId "d3d12" `
                -Recipe "renderer-d3d12-quality-evidence" `
                -SchemaVersion "GameEngine.RendererCommercialQualityCloseout.v1" `
                -ArtifactPath "$artifactRootRelative/missing-d3d12.json" `
                -ArtifactHash $missingArtifactHash `
                -Counter "renderer_d3d12_renderer_quality_ready" `
                -Ready $false `
                -DiagnosticCode "missing_d3d12_quality"
            vulkan_strict = New-RendererCommercialReadinessEvidenceRow -EvidenceId "vulkan_strict" `
                -Recipe "renderer-vulkan-strict-quality-evidence" `
                -SchemaVersion "GameEngine.RendererCommercialQualityCloseout.v1" `
                -ArtifactPath "$artifactRootRelative/missing-vulkan-strict.json" `
                -ArtifactHash $missingArtifactHash `
                -Counter "renderer_vulkan_strict_renderer_quality_ready" `
                -Ready $false `
                -DiagnosticCode "missing_vulkan_strict_quality"
            apple_metal = New-RendererCommercialReadinessEvidenceRow -EvidenceId "apple_metal" `
                -Recipe "renderer-metal-apple-host-evidence" `
                -SchemaVersion "GameEngine.RendererCommercialQualityCloseout.v1" `
                -ArtifactPath "$artifactRootRelative/missing-apple-metal.json" `
                -ArtifactHash $missingArtifactHash `
                -Counter "renderer_apple_metal_renderer_quality_ready" `
                -Ready $false `
                -DiagnosticCode "missing_metal_host_evidence"
        }
        package_rows = [ordered]@{
            visible_3d = New-RendererCommercialReadinessEvidenceRow -EvidenceId "visible_3d" `
                -Recipe "desktop-3d-package" `
                -SchemaVersion "GameEngine.DesktopRuntimePackageEvidence.v1" `
                -ArtifactPath "$artifactRootRelative/missing-visible-3d.json" `
                -ArtifactHash $missingArtifactHash `
                -Counter "renderer_visible_3d_package_ready" `
                -Ready $false `
                -DiagnosticCode "missing_package_evidence"
            runtime_ui = New-RendererCommercialReadinessEvidenceRow -EvidenceId "runtime_ui" `
                -Recipe "desktop-runtime-ui-package" `
                -SchemaVersion "GameEngine.DesktopRuntimePackageEvidence.v1" `
                -ArtifactPath "$artifactRootRelative/missing-runtime-ui.json" `
                -ArtifactHash $missingArtifactHash `
                -Counter "renderer_runtime_ui_package_ready" `
                -Ready $false `
                -DiagnosticCode "missing_package_evidence"
            environment = New-RendererCommercialReadinessEvidenceRow -EvidenceId "environment" `
                -Recipe "environment-package" `
                -SchemaVersion "GameEngine.EnvironmentPackageEvidence.v1" `
                -ArtifactPath "$artifactRootRelative/missing-environment.json" `
                -ArtifactHash $missingArtifactHash `
                -Counter "renderer_environment_package_ready" `
                -Ready $false `
                -DiagnosticCode "missing_package_evidence"
            generated_game = New-RendererCommercialReadinessEvidenceRow -EvidenceId "generated_game" `
                -Recipe "generated-game-package" `
                -SchemaVersion "GameEngine.GeneratedGamePackageEvidence.v1" `
                -ArtifactPath "$artifactRootRelative/missing-generated-game.json" `
                -ArtifactHash $missingArtifactHash `
                -Counter "renderer_generated_game_package_ready" `
                -Ready $false `
                -DiagnosticCode "missing_package_evidence"
        }
        quality_rows = [ordered]@{
            renderer_quality_matrix = New-RendererCommercialReadinessEvidenceRow -EvidenceId "renderer_quality_matrix" `
                -Recipe "renderer-quality-matrix" `
                -SchemaVersion "GameEngine.RendererCommercialQualityCloseout.v1" `
                -ArtifactPath "$artifactRootRelative/missing-renderer-quality-matrix.json" `
                -ArtifactHash $missingArtifactHash `
                -Counter "renderer_quality_matrix_ready" `
                -Ready $false `
                -DiagnosticCode "missing_quality_matrix"
            production_vfx_profiling = New-RendererCommercialReadinessEvidenceRow -EvidenceId "production_vfx_profiling" `
                -Recipe "renderer-production-vfx-profiling" `
                -SchemaVersion "GameEngine.RendererCommercialQualityCloseout.v1" `
                -ArtifactPath "$artifactRootRelative/missing-production-vfx-profiling.json" `
                -ArtifactHash $missingArtifactHash `
                -Counter "renderer_production_vfx_profiling_ready" `
                -Ready $false `
                -DiagnosticCode "missing_vfx_profiling"
        }
        metal_memory_profiling_rows = [ordered]@{
            memory_residency = New-RendererCommercialReadinessEvidenceRow -EvidenceId "memory_residency" `
                -Recipe "renderer-metal-memory-profiling-host-evidence" `
                -SchemaVersion "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1" `
                -ArtifactPath "$artifactRootRelative/metal-memory/evidence.json" `
                -ArtifactHash $hostEvidenceHash `
                -Counter "renderer_metal_memory_profiling_ready" `
                -Ready $true `
                -DiagnosticCode "ready"
            profiling_capture = New-RendererCommercialReadinessEvidenceRow -EvidenceId "profiling_capture" `
                -Recipe "renderer-metal-memory-profiling-host-evidence" `
                -SchemaVersion "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1" `
                -ArtifactPath "$artifactRootRelative/metal-memory/evidence.json" `
                -ArtifactHash $hostEvidenceHash `
                -Counter "renderer_metal_memory_profiling_ready" `
                -Ready $true `
                -DiagnosticCode "ready"
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
            renderer_backend_parity_ready = $false
            renderer_metal_broad_readiness = $false
            renderer_broad_quality_ready = $false
            renderer_commercial_readiness = $false
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

    $commercialEvidence | ConvertTo-Json -Depth 20 |
        Set-Content -LiteralPath (Join-Path $artifactRoot "evidence.json") -Encoding utf8NoBOM

    $validationOutput = @(& pwsh -NoProfile -ExecutionPolicy Bypass -File $validator `
            -ArtifactRootRelative $artifactRootRelative 2>&1)
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Metal memory commercial readiness validation command failed unexpectedly."
    }

    $validationLines = @($validationOutput | ForEach-Object { [string]$_ })
    $validationText = [string]::Join("`n", $validationLines)

    Assert-LinePresent -Lines $validationLines `
        -ExpectedLine "renderer_metal_memory_profiling_ready=1" `
        -Context "Metal memory full host evidence validation"
    Assert-LinePresent -Lines $validationLines `
        -ExpectedLine "renderer_metal_broad_readiness=0" `
        -Context "Metal memory full host evidence validation"
    Assert-LinePresent -Lines $validationLines `
        -ExpectedLine "renderer_commercial_readiness=0" `
        -Context "Metal memory full host evidence validation"
    Assert-TextAbsent -Text $validationText -Needle "invalid_artifact_fixture_flag" `
        -Context "Metal memory full host evidence validation"
    Assert-TextAbsent -Text $validationText -Needle "metal_memory_full_host_evidence_required" `
        -Context "Metal memory full host evidence validation"
} finally {
    if (Test-Path -LiteralPath $artifactRoot) {
        Assert-PathUnderDirectory -Path $artifactRoot -Directory $approvedArtifactRoot `
            -Description "Renderer commercial readiness Metal memory cleanup root"
        Remove-Item -LiteralPath $artifactRoot -Recurse -Force
    }
}

Write-Information "renderer-commercial-readiness-evidence-metal-memory-check: ok" -InformationAction Continue
