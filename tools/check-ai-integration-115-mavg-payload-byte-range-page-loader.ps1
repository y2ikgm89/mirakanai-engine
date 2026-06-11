#requires -Version 7.0
#requires -PSEdition Core
# Chapter 115 for check-ai-integration.ps1 static contracts.

$mavgGraphHeaderText = Get-AgentSurfaceText "engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp"
$mavgGraphSourceText = Get-AgentSurfaceText "engine/assets/src/mavg_cluster_graph.cpp"
$platformFilesystemHeaderText = Get-AgentSurfaceText "engine/platform/include/mirakana/platform/filesystem.hpp"
$platformFilesystemSourceText = Get-AgentSurfaceText "engine/platform/src/filesystem.cpp"
$payloadLoaderHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_payload_page_loader.hpp"
$payloadLoaderSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_payload_page_loader.cpp"
$pageStreamingHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp"
$pageStreamingSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_page_streaming.cpp"
$mavgGpuMemoryResidencyHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_gpu_memory_residency.hpp"
$mavgGpuMemoryResidencySourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_gpu_memory_residency.cpp"
$mavgClusterStreamingResidencyCloseoutHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_cluster_streaming_residency_closeout.hpp"
$mavgClusterStreamingResidencyCloseoutSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_cluster_streaming_residency_closeout.cpp"
$mavgClusterStreamingSafePointAdoptionHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_cluster_streaming_safe_point_adoption.hpp"
$mavgClusterStreamingSafePointAdoptionSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_cluster_streaming_safe_point_adoption.cpp"
$payloadLoaderTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_payload_page_loader_tests.cpp"
$pageStreamingTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_page_streaming_tests.cpp"
$mavgGpuMemoryResidencyTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_gpu_memory_residency_tests.cpp"
$mavgClusterStreamingResidencyCloseoutTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_cluster_streaming_residency_closeout_tests.cpp"
$mavgClusterStreamingSafePointAdoptionTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests.cpp"
$graphTestsText = Get-AgentSurfaceText "tests/unit/mavg_cluster_graph_tests.cpp"
$coreTestsText = Get-AgentSurfaceText "tests/unit/core_tests.cpp"
$cmakeText = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeCmakeText = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$runtimeRhiCmakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-payload-byte-range-page-loader-v1.md"
$filesystemPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-payload-filesystem-byte-range-io-v1.md"
$gpuMemoryResidencyPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-gpu-memory-pressure-residency-v1.md"
$clusterStreamingResidencyCloseoutPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-cluster-streaming-residency-closeout-v1.md"
$clusterStreamingSafePointAdoptionPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-cluster-streaming-safe-point-adoption-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$masterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "mavg_cluster_payload_format_v1",
        "invalid_page_byte_range"
    )) {
    Assert-ContainsText $mavgGraphHeaderText $needle "MAVG graph payload page byte-range schema public contract"
    Assert-ContainsText $mavgGraphSourceText $needle "MAVG graph payload page byte-range schema implementation"
}

foreach ($needle in @(
        "RuntimeMavgPayloadPageLoadDesc",
        "RuntimeMavgPayloadFilesystemPageLoadDesc",
        "RuntimeMavgPayloadPageLoadResult",
        "RuntimeMavgPayloadPageRow",
        "RuntimeMavgPayloadPageLoadDiagnosticCode",
        "load_runtime_mavg_payload_pages",
        "load_runtime_mavg_payload_pages_from_filesystem",
        "filesystem_read_byte_count",
        "invoked_file_io",
        "executed_background_worker",
        "executed_direct_storage",
        "touched_gpu_memory_policy",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $payloadLoaderHeaderText $needle "MAVG runtime payload page loader public contract"
}

foreach ($needle in @(
        "load_runtime_mavg_payload_pages_from_filesystem",
        "GameEngine.MavgClusterPayload.v1",
        "payload_starts_with_format",
        "read_binary_range",
        "duplicate_page_request",
        "page_range_out_of_bounds",
        "std::vector<std::byte>"
    )) {
    Assert-ContainsText $payloadLoaderSourceText $needle "MAVG runtime payload page loader implementation"
}

foreach ($needle in @(
        "read_binary_range"
    )) {
    Assert-ContainsText $platformFilesystemHeaderText $needle "MAVG filesystem byte-range public contract"
}

foreach ($needle in @(
        "MemoryFileSystem::read_binary_range",
        "RootedFileSystem::read_binary_range"
    )) {
    Assert-ContainsText $platformFilesystemSourceText $needle "MAVG filesystem byte-range implementation"
}

foreach ($needle in @(
        "memory filesystem reads exact binary byte ranges",
        "rooted filesystem reads exact binary byte ranges without loading text"
    )) {
    Assert-ContainsText $coreTestsText $needle "MAVG filesystem byte-range platform tests"
}

foreach ($needle in @(
        "runtime mavg payload page loader copies requested byte ranges without side effects",
        "runtime mavg payload page loader rejects duplicate missing and out of bounds pages",
        "runtime mavg payload page loader rejects invalid payload format",
        "runtime mavg filesystem payload page loader reads only format prefix and requested byte ranges",
        "runtime mavg filesystem payload page loader rejects range failures without partial rows",
        "MK_REQUIRE(!result.invoked_file_io)",
        "MK_REQUIRE(result.invoked_file_io)",
        "read_text_call_count == 0",
        "MK_REQUIRE(!result.executed_background_worker)",
        "MK_REQUIRE(!result.executed_direct_storage)",
        "MK_REQUIRE(!result.touched_gpu_memory_policy)",
        "MK_REQUIRE(!result.touched_renderer_or_rhi_handles)"
    )) {
    Assert-ContainsText $payloadLoaderTestsText $needle "MAVG runtime payload page loader tests"
}

foreach ($needle in @(
        "mavg cluster graph rejects overlapping page byte ranges",
        "invalid_page_byte_range"
    )) {
    Assert-ContainsText $graphTestsText $needle "MAVG graph byte range validation tests"
}

foreach ($needle in @(
        "MK_runtime_mavg_payload_page_loader_tests",
        "tests/unit/runtime_mavg_payload_page_loader_tests.cpp"
    )) {
    Assert-ContainsText $cmakeText $needle "MAVG runtime payload page loader CMake test target"
}
Assert-ContainsText $runtimeCmakeText "src/mavg_payload_page_loader.cpp" "MAVG runtime payload page loader source registration"

foreach ($surface in @(
        @{ Text = $planText; Label = "implementation plan" },
        @{ Text = $planRegistryText; Label = "plan registry" },
        @{ Text = $currentCapabilitiesText; Label = "current capabilities" },
        @{ Text = $roadmapText; Label = "roadmap" },
        @{ Text = $mavgArchitectureSpecText; Label = "MAVG architecture spec" },
        @{ Text = $masterPlanText; Label = "MAVG master plan" },
        @{ Text = $aiLoopFragmentText; Label = "production loop fragment" },
        @{ Text = $modulesFragmentText; Label = "modules fragment" }
    )) {
    foreach ($needle in @(
            "mavg-payload-byte-range-page-loader-v1",
            "MAVG Payload Byte-Range Page Loader v1",
            "mavg_payload_page_loader.hpp",
            "RuntimeMavgPayloadPageLoadResult",
            "load_runtime_mavg_payload_pages",
            "background streaming",
            "GPU memory pressure",
            "mesh shaders",
            "Metal readiness",
            "Nanite",
            "broad optimization"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG payload byte-range page loader evidence and non-claims"
    }
}

foreach ($surface in @(
        @{ Text = $filesystemPlanText; Label = "filesystem byte-range implementation plan" },
        @{ Text = $planRegistryText; Label = "plan registry" },
        @{ Text = $currentCapabilitiesText; Label = "current capabilities" },
        @{ Text = $roadmapText; Label = "roadmap" },
        @{ Text = $mavgArchitectureSpecText; Label = "MAVG architecture spec" },
        @{ Text = $masterPlanText; Label = "MAVG master plan" },
        @{ Text = $aiLoopFragmentText; Label = "production loop fragment" },
        @{ Text = $modulesFragmentText; Label = "modules fragment" }
    )) {
    foreach ($needle in @(
            "mavg-payload-filesystem-byte-range-io-v1",
            "MAVG Payload Filesystem Byte-Range IO v1",
            "IFileSystem::read_binary_range",
            "load_runtime_mavg_payload_pages_from_filesystem"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG filesystem byte-range evidence"
    }
}

foreach ($needle in @(
        "RuntimeMavgPageStreamingBackgroundLoadDesc",
        "RuntimeMavgPageStreamingBackgroundLoadedRow",
        "RuntimeMavgPageStreamingBackgroundLoadResult",
        "dispatch_runtime_mavg_page_streaming_background_loads",
        "background_load_failed",
        "proved_async_overlap_performance"
    )) {
    Assert-ContainsText $pageStreamingHeaderText $needle "MAVG background streaming dispatch public contract"
}

foreach ($needle in @(
        "JobExecutionBatchDesc",
        "load_runtime_package_candidate_v2",
        "filesystem_mutex",
        "background_dispatch_failed",
        "background_load_failed"
    )) {
    Assert-ContainsText $pageStreamingSourceText $needle "MAVG background streaming dispatch implementation"
}

foreach ($needle in @(
        "runtime mavg page streaming dispatches queued rows through background job workers",
        "runtime mavg page streaming background dispatch reports load failures without safe point mutation",
        "dispatch_runtime_mavg_page_streaming_background_loads",
        "MK_REQUIRE(!dispatch.committed)",
        "MK_REQUIRE(!dispatch.invoked_direct_storage)",
        "MK_REQUIRE(!dispatch.applied_gpu_memory_pressure_policy)",
        "MK_REQUIRE(!dispatch.proved_async_overlap_performance)"
    )) {
    Assert-ContainsText $pageStreamingTestsText $needle "MAVG background streaming dispatch tests"
}

foreach ($surface in @(
        @{ Text = $planRegistryText; Label = "plan registry" },
        @{ Text = $currentCapabilitiesText; Label = "current capabilities" },
        @{ Text = $roadmapText; Label = "roadmap" },
        @{ Text = $mavgArchitectureSpecText; Label = "MAVG architecture spec" },
        @{ Text = $masterPlanText; Label = "MAVG master plan" },
        @{ Text = $aiLoopFragmentText; Label = "production loop fragment" }
    )) {
    foreach ($needle in @(
            "mavg-background-streaming-dispatch-v1",
            "MAVG Background Streaming Dispatch v1",
            "dispatch_runtime_mavg_page_streaming_background_loads",
            "persistent/autonomous background",
            "async-overlap/performance proof",
            "DirectStorage",
            "GPU memory pressure"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG background streaming dispatch evidence and non-claims"
    }
}

foreach ($needle in @(
        "RuntimeMavgGpuMemoryResidencyDesc",
        "RuntimeMavgGpuMemoryResidencyResult",
        "RuntimeMavgGpuMemoryResidencyDiagnosticCode",
        "plan_runtime_mavg_gpu_memory_pressure_residency",
        "RuntimeResourceResidencyBudgetV2",
        "GpuMemoryPolicyPlan",
        "proved_async_overlap_performance",
        "invoked_direct_storage"
    )) {
    Assert-ContainsText $mavgGpuMemoryResidencyHeaderText $needle "MAVG GPU memory pressure residency public contract"
}

foreach ($needle in @(
        "plan_runtime_mavg_page_streaming_automatic_evictions",
        "max_resident_content_bytes",
        "missing_residency_pressure_evidence",
        "missing_package_counter_evidence",
        "mutated_mount_set",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $mavgGpuMemoryResidencySourceText $needle "MAVG GPU memory pressure residency implementation"
}

foreach ($needle in @(
        "runtime rhi mavg gpu memory pressure residency plans protected recency eviction without mutation",
        "runtime rhi mavg gpu memory pressure residency fail closes without policy evidence",
        "plan_runtime_mavg_gpu_memory_pressure_residency",
        "MK_REQUIRE(!result.mutated_mount_set)",
        "MK_REQUIRE(!result.touched_renderer_or_rhi_handles)",
        "missing_residency_pressure_evidence"
    )) {
    Assert-ContainsText $mavgGpuMemoryResidencyTestsText $needle "MAVG GPU memory pressure residency tests"
}

foreach ($needle in @(
        "MK_runtime_rhi_mavg_gpu_memory_residency_tests",
        "tests/unit/runtime_rhi_mavg_gpu_memory_residency_tests.cpp"
    )) {
    Assert-ContainsText $cmakeText $needle "MAVG GPU memory pressure residency CMake test target"
}
Assert-ContainsText $runtimeRhiCmakeText "src/mavg_gpu_memory_residency.cpp" "MAVG GPU memory pressure residency source registration"

foreach ($surface in @(
        @{ Text = $gpuMemoryResidencyPlanText; Label = "GPU memory residency implementation plan" },
        @{ Text = $planRegistryText; Label = "plan registry" },
        @{ Text = $currentCapabilitiesText; Label = "current capabilities" },
        @{ Text = $roadmapText; Label = "roadmap" },
        @{ Text = $mavgArchitectureSpecText; Label = "MAVG architecture spec" },
        @{ Text = $masterPlanText; Label = "MAVG master plan" },
        @{ Text = $aiLoopFragmentText; Label = "production loop fragment" },
        @{ Text = $modulesFragmentText; Label = "modules fragment" }
    )) {
    foreach ($needle in @(
            "mavg-gpu-memory-pressure-residency-v1",
            "MAVG GPU Memory Pressure Residency v1",
            "mavg_gpu_memory_residency.hpp",
            "RuntimeMavgGpuMemoryResidencyResult",
            "plan_runtime_mavg_gpu_memory_pressure_residency",
            "GpuMemoryPolicyPlan",
            "RuntimeResourceResidencyBudgetV2::max_resident_content_bytes",
            "DirectStorage",
            "GPU upload",
            "mesh shaders",
            "Nanite",
            "broad optimization"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG GPU memory pressure residency evidence and non-claims"
    }
}

foreach ($needle in @(
        "RuntimeMavgClusterStreamingResidencyCloseoutDesc",
        "RuntimeMavgClusterStreamingResidencyCloseoutResult",
        "RuntimeMavgClusterStreamingResidencyCloseoutDiagnosticCode",
        "plan_runtime_mavg_cluster_streaming_residency_closeout",
        "payload_loaded_page_count",
        "background_loaded_row_count",
        "deterministic_degradation_row_count",
        "preserved_visible_geometry_without_holes",
        "invoked_gpu_upload",
        "executed_backend"
    )) {
    Assert-ContainsText $mavgClusterStreamingResidencyCloseoutHeaderText $needle "MAVG cluster streaming residency closeout public contract"
}

foreach ($needle in @(
        "plan_runtime_mavg_page_streaming_requests",
        "load_runtime_mavg_payload_pages_from_filesystem",
        "dispatch_runtime_mavg_page_streaming_background_loads",
        "plan_runtime_mavg_gpu_memory_pressure_residency",
        "payload_page_load_failed",
        "background_load_failed",
        "gpu_memory_residency_failed",
        "mutated_mount_set",
        "invoked_direct_storage",
        "proved_async_overlap_performance"
    )) {
    Assert-ContainsText $mavgClusterStreamingResidencyCloseoutSourceText $needle "MAVG cluster streaming residency closeout implementation"
}

foreach ($needle in @(
        "runtime rhi mavg cluster streaming residency closeout composes payload background and memory pressure evidence",
        "runtime rhi mavg cluster streaming residency closeout fail closes payload range errors before mutation",
        "plan_runtime_mavg_cluster_streaming_residency_closeout",
        "MK_REQUIRE(result.preserved_visible_geometry_without_holes)",
        "MK_REQUIRE(!result.invoked_gpu_upload)",
        "payload_page_load_failed"
    )) {
    Assert-ContainsText $mavgClusterStreamingResidencyCloseoutTestsText $needle "MAVG cluster streaming residency closeout tests"
}

foreach ($needle in @(
        "MK_runtime_rhi_mavg_cluster_streaming_residency_closeout_tests",
        "tests/unit/runtime_rhi_mavg_cluster_streaming_residency_closeout_tests.cpp"
    )) {
    Assert-ContainsText $cmakeText $needle "MAVG cluster streaming residency closeout CMake test target"
}
Assert-ContainsText $runtimeRhiCmakeText "src/mavg_cluster_streaming_residency_closeout.cpp" "MAVG cluster streaming residency closeout source registration"

foreach ($surface in @(
        @{ Text = $clusterStreamingResidencyCloseoutPlanText; Label = "cluster streaming residency closeout implementation plan" },
        @{ Text = $planRegistryText; Label = "plan registry" },
        @{ Text = $currentCapabilitiesText; Label = "current capabilities" },
        @{ Text = $roadmapText; Label = "roadmap" },
        @{ Text = $mavgArchitectureSpecText; Label = "MAVG architecture spec" },
        @{ Text = $masterPlanText; Label = "MAVG master plan" },
        @{ Text = $aiLoopFragmentText; Label = "production loop fragment" },
        @{ Text = $modulesFragmentText; Label = "modules fragment" }
    )) {
    foreach ($needle in @(
            "mavg-cluster-streaming-residency-closeout-v1",
            "MAVG Cluster Streaming Residency Closeout v1",
            "mavg_cluster_streaming_residency_closeout.hpp",
            "RuntimeMavgClusterStreamingResidencyCloseoutResult",
            "plan_runtime_mavg_cluster_streaming_residency_closeout",
            "load_runtime_mavg_payload_pages_from_filesystem",
            "dispatch_runtime_mavg_page_streaming_background_loads",
            "plan_runtime_mavg_gpu_memory_pressure_residency",
            "deterministic degradation",
            "DirectStorage",
            "GPU upload",
            "mesh shaders",
            "Nanite",
            "broad optimization"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG cluster streaming residency closeout evidence and non-claims"
    }
}

foreach ($needle in @(
        "RuntimeMavgClusterStreamingSafePointAdoptionDesc",
        "RuntimeMavgClusterStreamingSafePointAdoptionResult",
        "RuntimeMavgClusterStreamingSafePointAdoptionDiagnosticCode",
        "RuntimeMavgClusterStreamingSafePointAdoptionMountRow",
        "execute_runtime_mavg_cluster_streaming_safe_point_adoption",
        "invoked_candidate_load",
        "invoked_gpu_upload",
        "executed_backend",
        "proved_async_overlap_performance"
    )) {
    Assert-ContainsText $mavgClusterStreamingSafePointAdoptionHeaderText $needle "MAVG cluster streaming safe-point adoption public contract"
}

foreach ($needle in @(
        "background_load.loaded_rows",
        "plan_runtime_resident_package_evictions_v2",
        "RuntimeResidentCatalogCacheV2",
        "closeout_failed",
        "missing_loaded_rows",
        "invalid_mount_row",
        "duplicate_mount_row",
        "duplicate_mount_id",
        "mutated_mount_set",
        "invoked_direct_storage",
        "proved_async_overlap_performance"
    )) {
    Assert-ContainsText $mavgClusterStreamingSafePointAdoptionSourceText $needle "MAVG cluster streaming safe-point adoption implementation"
}

foreach ($needle in @(
        "runtime rhi mavg cluster streaming safe point adoption commits loaded rows with reviewed evictions",
        "runtime rhi mavg cluster streaming safe point adoption fails closed before mutation",
        "runtime rhi mavg cluster streaming safe point adoption rejects duplicate mount rows before mutation",
        "runtime rhi mavg cluster streaming safe point adoption keeps projected rows uncommitted on eviction failure",
        "execute_runtime_mavg_cluster_streaming_safe_point_adoption",
        "MK_REQUIRE(!result.invoked_candidate_load)",
        "MK_REQUIRE(result.mutated_mount_set)",
        "closeout_failed",
        "duplicate_mount_row",
        "eviction_plan_failed"
    )) {
    Assert-ContainsText $mavgClusterStreamingSafePointAdoptionTestsText $needle "MAVG cluster streaming safe-point adoption tests"
}

foreach ($needle in @(
        "MK_runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests",
        "tests/unit/runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests.cpp"
    )) {
    Assert-ContainsText $cmakeText $needle "MAVG cluster streaming safe-point adoption CMake test target"
}
Assert-ContainsText $runtimeRhiCmakeText "src/mavg_cluster_streaming_safe_point_adoption.cpp" "MAVG cluster streaming safe-point adoption source registration"

foreach ($surface in @(
        @{ Text = $clusterStreamingSafePointAdoptionPlanText; Label = "cluster streaming safe-point adoption implementation plan" },
        @{ Text = $planRegistryText; Label = "plan registry" },
        @{ Text = $currentCapabilitiesText; Label = "current capabilities" },
        @{ Text = $roadmapText; Label = "roadmap" },
        @{ Text = $mavgArchitectureSpecText; Label = "MAVG architecture spec" },
        @{ Text = $masterPlanText; Label = "MAVG master plan" },
        @{ Text = $aiLoopFragmentText; Label = "production loop fragment" },
        @{ Text = $modulesFragmentText; Label = "modules fragment" }
    )) {
    foreach ($needle in @(
            "mavg-cluster-streaming-safe-point-adoption-v1",
            "MAVG Cluster Streaming Safe Point Adoption v1",
            "mavg_cluster_streaming_safe_point_adoption.hpp",
            "RuntimeMavgClusterStreamingSafePointAdoptionResult",
            "execute_runtime_mavg_cluster_streaming_safe_point_adoption",
            "background_load.loaded_rows",
            "RuntimeResidentPackageMountSetV2",
            "RuntimeResidentCatalogCacheV2",
            "plan_runtime_resident_package_evictions_v2",
            "DirectStorage",
            "GPU upload",
            "backend execution",
            "async-overlap/performance proof",
            "mesh shaders",
            "Nanite",
            "broad optimization"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG cluster streaming safe-point adoption evidence and non-claims"
    }
}

if ($manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must remain the production-completion master plan after MAVG payload byte-range page loader closeout"
}
if ($manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "next-production-gap-selection") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must remain next-production-gap-selection after MAVG payload byte-range page loader closeout"
}
