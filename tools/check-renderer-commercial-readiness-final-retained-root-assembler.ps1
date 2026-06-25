#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

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

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
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
    $json = $Value | ConvertTo-Json -Depth 24
    Set-Content -LiteralPath $Path -Value $json -Encoding utf8NoBOM
}

function Remove-TestRoot {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $fullPath = [System.IO.Path]::GetFullPath((ConvertTo-LocalPath $RelativePath))
    $allowedRoot = [System.IO.Path]::GetFullPath(
        (ConvertTo-LocalPath "artifacts/renderer/commercial-readiness-evidence")).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($allowedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "Refusing to remove test root outside artifacts/renderer/commercial-readiness-evidence: $fullPath"
    }
    if (Test-Path -LiteralPath $fullPath) {
        Remove-Item -LiteralPath $fullPath -Recurse -Force
    }
}

function New-D3d12HostEvidence {
    return [ordered]@{
        schema_version = "GameEngine.RendererD3d12CommercialQualityHostEvidence.v1"
        claim_id = "renderer-d3d12-commercial-quality-artifact-v1"
        validation_recipe = "renderer-d3d12-quality-evidence"
        fixture_only = $false
        source_id = "Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25"
        proof_rows = [ordered]@{
            command_allocator_list_fence = [ordered]@{
                ready = $true
                command_allocator_reuse_fenced = $true
                command_list_closed_before_execute = $true
                fence_signal_wait_recorded = $true
                fence_api_name = "ID3D12Fence"
            }
            resource_barriers = [ordered]@{
                ready = $true
                render_transition_explicit = $true
                copy_transition_explicit = $true
                unordered_access_barrier_explicit = $true
                readback_transition_explicit = $true
                resource_barrier_api_name = "D3D12_RESOURCE_BARRIER"
            }
            timestamp = [ordered]@{
                ready = $true
                query_type = "D3D12_QUERY_TYPE_TIMESTAMP"
                resolved_query_data = $true
                queue_frequency_hz = 1000000000
                clock_calibration = $true
            }
            debug_validation = [ordered]@{
                ready = $true
                debug_layer_or_gpu_based_validation_clean = $true
                debug_message_count = 0
                gpu_based_validation_message_count = 0
            }
            residency = [ordered]@{
                ready = $true
                video_memory_budget_queried = $true
                make_resident_or_budget_recorded = $true
                residency_api_name = "ID3D12Device3::EnqueueMakeResident"
                budget_api_name = "IDXGIAdapter3::QueryVideoMemoryInfo"
            }
            package_visible_readback = [ordered]@{
                ready = $true
                deterministic_hash_sha256 = "4c5d0a311d81e9fb91938f124890cb8456b744a6129da1b2ce2f6ff3954494f2"
                readback_counter_rows = 1
                package_counter_id = "renderer_d3d12_package_visible_readback"
            }
            native_handles = [ordered]@{
                ready = $true
                native_handles_exposed = $false
            }
        }
        validation_counters = [ordered]@{
            renderer_d3d12_command_allocator_fence_ready = $true
            renderer_d3d12_resource_barrier_ready = $true
            renderer_d3d12_timestamp_ready = $true
            renderer_d3d12_debug_validation_ready = $true
            renderer_d3d12_residency_ready = $true
            renderer_d3d12_package_readback_ready = $true
        }
        non_claims = [ordered]@{
            vulkan_inferred = $false
            metal_inferred = $false
            broad_ui_parity = $false
            environment_ready = $false
            external_engine_parity = $false
            native_handles_exposed = $false
        }
    }
}

function New-VulkanHostEvidence {
    return [ordered]@{
        schema_version = "GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1"
        claim_id = "renderer-vulkan-strict-commercial-quality-artifact-v1"
        validation_recipe = "renderer-vulkan-strict-quality-evidence"
        fixture_only = $false
        source_id = "Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25"
        proof_rows = [ordered]@{
            synchronization2 = [ordered]@{
                ready = $true
                vk_cmd_pipeline_barrier2_recorded = $true
                vk_dependency_info_recorded = $true
                api_name = "vkCmdPipelineBarrier2"
                structure_name = "VkDependencyInfo"
            }
            validation_layer = [ordered]@{
                ready = $true
                layer_name = "VK_LAYER_KHRONOS_validation"
                validation_log_clean = $true
                validation_error_count = 0
            }
            synchronization_validation = [ordered]@{
                ready = $true
                sync_validation_enabled = $true
                sync_validation_error_count = 0
            }
            memory_binding = [ordered]@{
                ready = $true
                buffer_memory_bound = $true
                image_memory_bound = $true
                vuid_constraints_checked = $true
                vuid_reference = "VUID-vkBindBufferMemory-memory-01030"
            }
            timestamp_query = [ordered]@{
                ready = $true
                query_pool_timestamp = $true
                timestamps_resolved = $true
                timestamp_period_ns = 0.5
            }
            spirv_shader_validation = [ordered]@{
                ready = $true
                spirv_val_ready = $true
                shader_modules_validated = $true
                validation_error_count = 0
            }
            package_visible_readback = [ordered]@{
                ready = $true
                deterministic_hash_sha256 = "7f86d7be87b48d9a93f56d4a2f072c435f96a8364eb88c1a91fa6bda38f8d94b"
                readback_counter_rows = 1
                package_counter_id = "renderer_vulkan_package_visible_readback"
            }
            native_handles = [ordered]@{
                ready = $true
                native_handles_exposed = $false
            }
        }
        validation_counters = [ordered]@{
            renderer_vulkan_synchronization2_ready = $true
            renderer_vulkan_validation_layer_ready = $true
            renderer_vulkan_sync_validation_ready = $true
            renderer_vulkan_memory_binding_ready = $true
            renderer_vulkan_timestamp_ready = $true
            renderer_vulkan_shader_validation_ready = $true
            renderer_vulkan_package_readback_ready = $true
        }
        non_claims = [ordered]@{
            d3d12_inferred = $false
            metal_inferred = $false
            debugging_only_full_pipeline_barrier = $false
            environment_ready = $false
            external_engine_parity = $false
            native_handles_exposed = $false
        }
    }
}

function New-AppleMetalHostEvidence {
    return [ordered]@{
        schema_version = "GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1"
        claim_id = "renderer-apple-metal-commercial-quality-artifact-v1"
        validation_recipe = "renderer-metal-apple-host-evidence"
        fixture_only = $false
        source_id = "Apple-Metal-Commercial-Host-Bridge-2026-06-25"
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
                toolchain_source_id = "Apple-Building-Shader-Library-Precompiling-Source-Files-2026-06-25"
            }
            msl_shader = [ordered]@{
                ready = $true
                address_spaces = @("device", "constant", "threadgroup")
                function_constant_attribute = "[[function_constant]]"
                resource_binding_attributes = @("[[buffer]]", "[[texture]]", "[[sampler]]")
                stage_attributes = @("[[vertex]]", "[[fragment]]", "[[kernel]]")
                msl_source_id = "Apple-Metal-Shading-Language-Specification-2026-06-25"
            }
            visible_package = [ordered]@{
                ready = $true
                selected_3d_package = $true
                runtime_ui_package = $true
                environment_package = $true
                generated_game_package = $true
                visible_package_rows = 4
                deterministic_hash_sha256 = "0d4d4e443582ab659a52bfb21fcab6f9aed7f1fcd29882a595c9f5a4f6b8b77a"
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
        non_claims = [ordered]@{
            d3d12_inferred = $false
            vulkan_inferred = $false
            environment_ready = $false
            external_engine_parity = $false
            native_handles_exposed = $false
            metal_objects_public = $false
        }
    }
}

function New-PackageHostEvidence {
    return [ordered]@{
        schema_version = "GameEngine.RendererPackageCommercialQualityHostEvidence.v1"
        claim_id = "renderer-package-commercial-quality-artifacts-v1"
        validation_recipe = "renderer-package-commercial-quality-artifacts"
        fixture_only = $false
        source_id = "GameEngine-Renderer-Package-Commercial-Quality-2026-06-25"
        package_rows = [ordered]@{
            visible_3d = [ordered]@{
                validation_recipe = "desktop-3d-package"
                proof_rows = [ordered]@{
                    material_render = [ordered]@{
                        ready = $true
                        pbr_material_row = $true
                        texture_binding_row = $true
                        material_variant_rows = 1
                    }
                    lighting_row = [ordered]@{
                        ready = $true
                        direct_light_row = $true
                        ambient_light_row = $true
                        lighting_readback_nonzero = $true
                    }
                    shadow_postprocess = [ordered]@{
                        ready = $true
                        shadow_or_depth_row = $true
                        postprocess_row = $true
                        tone_mapping_row = $true
                    }
                    package_visible_readback = [ordered]@{
                        ready = $true
                        deterministic_hash_sha256 = "e55076775a63e556bece9ce3bb2dfe4f74641c4bbb48e5606f3323e9813a25b7"
                        readback_counter_rows = 1
                    }
                    manifest_binding = [ordered]@{
                        ready = $true
                        game_agent_manifest_row = $true
                        validation_recipe_id = "desktop-3d-package"
                        package_manifest_row = "sample_generated_desktop_runtime_3d_package"
                    }
                }
                validation_counters = [ordered]@{
                    renderer_visible_3d_material_ready = $true
                    renderer_visible_3d_lighting_ready = $true
                    renderer_visible_3d_shadow_postprocess_ready = $true
                    renderer_visible_3d_readback_hash_ready = $true
                    renderer_package_arbitrary_script_execution = $false
                    renderer_package_script_execution = $false
                }
                non_claims = [ordered]@{
                    arbitrary_script_execution = $false
                    package_script_execution = $false
                    native_handles_exposed = $false
                    external_engine_parity = $false
                    environment_ready = $false
                }
            }
            runtime_ui = [ordered]@{
                validation_recipe = "desktop-runtime-ui-package"
                proof_rows = [ordered]@{
                    ui_atlas_upload = [ordered]@{
                        ready = $true
                        atlas_texture_upload_row = $true
                        atlas_texture_usage_sampled = $true
                        upload_counter_rows = 1
                    }
                    ui_atlas_readback = [ordered]@{
                        ready = $true
                        readback_counter_rows = 1
                        deterministic_hash_sha256 = "8a6cf68c576f8f89bd1f2d7df9d35f7c80dbd6f5866222737345e222b7fb5c8d"
                    }
                    renderer_handoff = [ordered]@{
                        ready = $true
                        retained_upload_handoff_row = $true
                        renderer_consumed_ui_atlas_row = $true
                    }
                    manifest_binding = [ordered]@{
                        ready = $true
                        game_agent_manifest_row = $true
                        validation_recipe_id = "desktop-runtime-ui-package"
                        package_manifest_row = "sample_2d_desktop_runtime_package"
                    }
                }
                validation_counters = [ordered]@{
                    renderer_runtime_ui_atlas_upload_ready = $true
                    renderer_runtime_ui_atlas_readback_ready = $true
                    renderer_runtime_ui_handoff_ready = $true
                    renderer_package_arbitrary_script_execution = $false
                    renderer_package_script_execution = $false
                }
                non_claims = [ordered]@{
                    arbitrary_script_execution = $false
                    package_script_execution = $false
                    native_handles_exposed = $false
                    external_engine_parity = $false
                    environment_ready = $false
                }
            }
            environment = [ordered]@{
                validation_recipe = "environment-package"
                proof_rows = [ordered]@{
                    environment_renderer_package_consumption = [ordered]@{
                        ready = $true
                        environment_package_row_consumed = $true
                        renderer_environment_rows_consumed_count = 4
                        environment_ready_promoted = $false
                    }
                    manifest_binding = [ordered]@{
                        ready = $true
                        game_agent_manifest_row = $true
                        validation_recipe_id = "environment-package"
                        package_manifest_row = "environment_renderer_package"
                    }
                }
                validation_counters = [ordered]@{
                    renderer_environment_package_consumption_ready = $true
                    renderer_environment_ready_promoted = $false
                    renderer_package_arbitrary_script_execution = $false
                    renderer_package_script_execution = $false
                }
                non_claims = [ordered]@{
                    arbitrary_script_execution = $false
                    package_script_execution = $false
                    native_handles_exposed = $false
                    external_engine_parity = $false
                    environment_ready = $false
                }
            }
            generated_game = [ordered]@{
                validation_recipe = "generated-game-package"
                proof_rows = [ordered]@{
                    generated_game_output = [ordered]@{
                        ready = $true
                        generated_game_package_written = $true
                        output_manifest_rows = 1
                        deterministic_hash_sha256 = "b8ec9f6e45f0c36bbf4c5b9bfd31e2c8ad4754e6f2e53f4419bd116080e24f3a"
                    }
                    manifest_binding = [ordered]@{
                        ready = $true
                        game_agent_manifest_row = $true
                        validation_recipe_id = "generated-game-package"
                        generated_game_manifest_id = "generated_game_studio_v1_package"
                        package_manifest_row = "generated_game_package_output"
                    }
                }
                validation_counters = [ordered]@{
                    renderer_generated_game_package_output_ready = $true
                    renderer_generated_game_manifest_ready = $true
                    renderer_package_arbitrary_script_execution = $false
                    renderer_package_script_execution = $false
                }
                non_claims = [ordered]@{
                    arbitrary_script_execution = $false
                    package_script_execution = $false
                    native_handles_exposed = $false
                    external_engine_parity = $false
                    environment_ready = $false
                }
            }
        }
        non_claims = [ordered]@{
            arbitrary_script_execution = $false
            package_script_execution = $false
            native_handles_exposed = $false
            external_engine_parity = $false
            environment_ready = $false
            renderer_commercial_readiness = $false
        }
    }
}

function New-QualityVfxHostEvidence {
    return [ordered]@{
        schema_version = "GameEngine.RendererQualityVfxCommercialHostEvidence.v1"
        claim_id = "renderer-quality-vfx-commercial-artifacts-v1"
        validation_recipe = "renderer-quality-vfx-commercial-artifacts"
        fixture_only = $false
        source_id = "GameEngine-Renderer-Quality-Vfx-Profiling-2026-06-25"
        quality_rows = [ordered]@{
            renderer_quality_matrix = [ordered]@{
                validation_recipe = "renderer-quality-matrix"
                proof_rows = [ordered]@{
                    matrix_status = [ordered]@{
                        ready = $true
                        renderer_quality_matrix_status = "host_evidence_required"
                        d3d12_renderer_quality_ready = $true
                        vulkan_strict_renderer_quality_ready = $true
                        apple_metal_host_evidence_supplied = $true
                        general_renderer_quality_claim = $false
                    }
                    side_effect_policy = [ordered]@{
                        ready = $true
                        gpu_command_side_effects = 0
                        native_capture_side_effects = 0
                        crash_upload_side_effects = 0
                    }
                    replay = [ordered]@{
                        ready = $true
                        deterministic_replay_hash_sha256 = "11efb7abda26952d0c620bd171949269bdf4d0fc14e4d35a7e2a2ec1ba6bc99f"
                    }
                    diagnostics = [ordered]@{
                        ready = $true
                        diagnostic_error_count = 0
                    }
                }
                validation_counters = [ordered]@{
                    renderer_quality_matrix_ready = $true
                    renderer_quality_matrix_general_renderer_quality_claim = $false
                    renderer_quality_matrix_gpu_command_side_effects = 0
                    renderer_quality_matrix_native_capture_side_effects = 0
                    renderer_quality_matrix_crash_upload_side_effects = 0
                }
                non_claims = [ordered]@{
                    broad_renderer_quality = $false
                    renderer_commercial_readiness = $false
                    external_engine_parity = $false
                    native_handles_exposed = $false
                    gpu_command_side_effects = $false
                    native_capture_side_effects = $false
                    crash_upload_side_effects = $false
                }
            }
            production_vfx_profiling = [ordered]@{
                validation_recipe = "renderer-production-vfx-profiling"
                proof_rows = [ordered]@{
                    vfx_profiling_review = [ordered]@{
                        ready = $true
                        rendering_vfx_profiling_reviewed = $true
                        d3d12_renderer_quality_ready = $true
                        vulkan_strict_renderer_quality_ready = $true
                        apple_metal_host_evidence_supplied = $true
                    }
                    debug_policy = [ordered]@{
                        ready = $true
                        debug_capture_policy_recorded = $true
                        debug_upload_policy_recorded = $true
                        crash_upload_policy_recorded = $true
                    }
                    memory_policy = [ordered]@{
                        ready = $true
                        memory_residency_policy_recorded = $true
                        metal_memory_profiling_evidence_supplied = $true
                    }
                    package_counters = [ordered]@{
                        ready = $true
                        visible_3d_package_ready = $true
                        runtime_ui_package_ready = $true
                        environment_package_ready = $true
                        generated_game_package_ready = $true
                    }
                    replay = [ordered]@{
                        ready = $true
                        deterministic_replay_hash_sha256 = "2282db5ec41d758f4b3ee0a43c05cc3e0a353ae7f7d15d508f8c98032dcc27b2"
                    }
                    side_effect_policy = [ordered]@{
                        ready = $true
                        native_capture_side_effects = 0
                        crash_upload_side_effects = 0
                        retained_official_profiler_artifact_selected = $false
                    }
                }
                validation_counters = [ordered]@{
                    renderer_production_vfx_profiling_ready = $true
                    rendering_vfx_profiling_reviewed = $true
                    renderer_production_vfx_native_capture_side_effects = 0
                    renderer_production_vfx_crash_upload_side_effects = 0
                }
                non_claims = [ordered]@{
                    broad_renderer_quality = $false
                    renderer_commercial_readiness = $false
                    external_engine_parity = $false
                    native_handles_exposed = $false
                    native_capture_side_effects = $false
                    crash_upload_side_effects = $false
                    retained_official_profiler_artifact_selected = $false
                }
            }
        }
        non_claims = [ordered]@{
            broad_renderer_quality = $false
            renderer_commercial_readiness = $false
            external_engine_parity = $false
            native_handles_exposed = $false
            gpu_command_side_effects = $false
            native_capture_side_effects = $false
            crash_upload_side_effects = $false
        }
    }
}

function New-CleanRoomLegalReview {
    return [ordered]@{
        schema_version = "GameEngine.RendererCleanRoomLegalReviewInput.v1"
        claim_id = "renderer-clean-room-legal-artifact-v1"
        validation_recipe = "renderer-clean-room-legal-artifact"
        fixture_only = $false
        source_rows = [ordered]@{
            unity_terms_source_id = "Unity-Legal-Terms-2026-06-25"
            unity_trademark_source_id = "Unity-Trademark-Guidelines-2026-06-25"
            unreal_eula_source_id = "Epic-Unreal-Engine-EULA-Trademark-2026-06-25"
            unreal_release_trademark_source_id = "Epic-Unreal-Engine-Release-Trademark-2026-06-25"
            godot_license_source_id = "Godot-License-2026-06-25"
            godot_trademark_source_id = "Godot-Trademark-Licensing-2026-06-25"
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
        human_review = [ordered]@{
            external_material_selected = $false
            legal_review_required_for_external_material = $true
            technical_review_required_for_external_material = $true
            legal_review_id = ""
            technical_review_id = ""
            external_material_approved = $false
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
            renderer_commercial_readiness = $false
        }
    }
}

$assemblerScript = Join-Path $root "tools/assemble-renderer-commercial-readiness-final-retained-root.ps1"
if (-not (Test-Path -LiteralPath $assemblerScript -PathType Leaf)) {
    Write-Error "tools/assemble-renderer-commercial-readiness-final-retained-root.ps1 must exist."
}

$evidenceRootRelative = "artifacts/renderer/commercial-readiness-evidence/final-retained-assembler-contract-$PID"
$inputRootRelative = "$evidenceRootRelative/input"
$outputRootRelative = "$evidenceRootRelative/final-retained"
$evidenceRootPath = ConvertTo-LocalPath $evidenceRootRelative

try {
    Remove-TestRoot -RelativePath $evidenceRootRelative

    $planLines = @(& $assemblerScript -Mode Plan -OutputRootRelative $outputRootRelative)
    foreach ($expectedLine in @(
            "validation_recipe=renderer-commercial-readiness-final-retained-root-assembler",
            "renderer_commercial_readiness_final_assembler_mode=Plan",
            "renderer_commercial_readiness_final_assembler_required_host_inputs=7",
            "renderer_commercial_readiness_final_assembler_required_final_files=12",
            "renderer_commercial_readiness_final_assembler_ready=0",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $planLines $expectedLine "final retained root assembler Plan mode"
    }

    $missingRejected = $false
    try {
        $null = & $assemblerScript -Mode Assemble -OutputRootRelative $outputRootRelative -RequireReady 2>&1
    }
    catch {
        $missingRejected = [string]$_.Exception.Message -like "*d3d12_host_evidence_required*" -and
            [string]$_.Exception.Message -like "*clean_room_legal_review_required*"
    }
    if (-not $missingRejected) {
        Write-Error "Final retained root assembler must reject missing host inputs with -RequireReady."
    }

    $unsafeRejected = $false
    try {
        $null = & $assemblerScript -Mode Plan -OutputRootRelative "../unsafe" 2>&1
    }
    catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "Final retained root assembler must reject unsafe output roots."
    }

    $d3d12Input = "$inputRootRelative/d3d12/d3d12-host-evidence.json"
    $vulkanInput = "$inputRootRelative/vulkan/vulkan-host-evidence.json"
    $appleInput = "$inputRootRelative/apple/apple-host-evidence.json"
    $metalInput = "$inputRootRelative/metal-memory/evidence.json"
    $packageInput = "$inputRootRelative/package/package-host-evidence.json"
    $qualityInput = "$inputRootRelative/quality-vfx/quality-vfx-host-evidence.json"
    $legalInput = "$inputRootRelative/legal/clean-room-legal-review.json"

    Write-JsonObject -Path (ConvertTo-LocalPath $d3d12Input) -Value (New-D3d12HostEvidence)
    Write-JsonObject -Path (ConvertTo-LocalPath $vulkanInput) -Value (New-VulkanHostEvidence)
    Write-JsonObject -Path (ConvertTo-LocalPath $appleInput) -Value (New-AppleMetalHostEvidence)
    Write-JsonObject -Path (ConvertTo-LocalPath $packageInput) -Value (New-PackageHostEvidence)
    Write-JsonObject -Path (ConvertTo-LocalPath $qualityInput) -Value (New-QualityVfxHostEvidence)
    Write-JsonObject -Path (ConvertTo-LocalPath $legalInput) -Value (New-CleanRoomLegalReview)

    $metalInputFull = ConvertTo-LocalPath $metalInput
    $metalInputParent = Split-Path -Parent $metalInputFull
    $null = New-Item -ItemType Directory -Path $metalInputParent -Force
    Copy-Item -LiteralPath (ConvertTo-LocalPath "tests/fixtures/renderer/metal-memory-profiling-host-evidence/ready/evidence.json") `
        -Destination $metalInputFull `
        -Force
    Copy-Item -LiteralPath (ConvertTo-LocalPath "tests/fixtures/renderer/metal-memory-profiling-host-evidence/ready/capture-summary.txt") `
        -Destination (Join-Path $metalInputParent "capture-summary.txt") `
        -Force

    $assembleLines = @(& $assemblerScript `
            -Mode Assemble `
            -OutputRootRelative $outputRootRelative `
            -D3d12HostEvidenceRelative $d3d12Input `
            -VulkanStrictHostEvidenceRelative $vulkanInput `
            -AppleMetalHostEvidenceRelative $appleInput `
            -MetalMemoryProfilingHostEvidenceRelative $metalInput `
            -PackageHostEvidenceRelative $packageInput `
            -QualityVfxHostEvidenceRelative $qualityInput `
            -CleanRoomLegalReviewRelative $legalInput `
            -RequireReady)

    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_assembler_status=ready",
            "renderer_commercial_readiness_final_assembler_ready=1",
            "renderer_commercial_readiness_final_preflight_ready=1",
            "renderer_commercial_readiness_final_preflight_missing_files=0",
            "renderer_commercial_readiness=1",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $assembleLines $expectedLine "final retained root assembler Assemble mode"
    }

    foreach ($expectedFile in @(
            "evidence.json",
            "d3d12-quality.json",
            "vulkan-strict-quality.json",
            "apple-metal-host.json",
            "visible-3d-package.json",
            "runtime-ui-package.json",
            "environment-package.json",
            "generated-game-package.json",
            "renderer-quality-matrix.json",
            "production-vfx-profiling.json",
            "metal-memory-profiling-host-evidence/evidence.json",
            "clean-room-legal.json"
        )) {
        if (-not (Test-Path -LiteralPath (ConvertTo-LocalPath "$outputRootRelative/$expectedFile") -PathType Leaf)) {
            Write-Error "Final retained root assembler did not write expected file: $expectedFile"
        }
    }
}
finally {
    Remove-TestRoot -RelativePath $evidenceRootRelative
}

Write-Information "renderer-commercial-readiness-final-retained-root-assembler-check: ok" -InformationAction Continue
