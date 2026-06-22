#requires -Version 7.0
#requires -PSEdition Core
# Chapter 123 for check-ai-integration.ps1 MAVG advanced backend evidence.

$mavgAdvancedHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_advanced_backend_evidence.hpp"
$mavgAdvancedSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_advanced_backend_evidence.cpp"
$mavgAdvancedTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_advanced_backend_evidence_tests.cpp"
$mavgAutonomousSchedulerHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_autonomous_streaming_scheduler.hpp"
$mavgAutonomousSchedulerSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_autonomous_streaming_scheduler.cpp"
$mavgAutonomousSchedulerTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_autonomous_streaming_scheduler_tests.cpp"
$mavgAutonomousSchedulerValidationText = Get-AgentSurfaceText "tools/validate-mavg-autonomous-streaming-scheduler.ps1"
$runtimeRhiCMakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$mavgAdvancedSpecText = Get-AgentSurfaceText "docs/specs/2026-06-21-mavg-advanced-backend-evidence-v1.md"
$mavgAdvancedPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "MavgAdvancedBackendEvidenceDiagnosticCode",
        "MavgAdvancedBackendEvidenceDesc",
        "MavgAdvancedBackendEvidenceResult",
        "evaluate_mavg_advanced_backend_evidence",
        "source_gate_date_yyyy_mm_dd",
        "source_gate_current_date_yyyy_mm_dd",
        "context7_vulkan_docs_ready",
        "context7_cmake_ready",
        "context7_vcpkg_ready",
        "official_d3d12_mesh_shader_ready",
        "official_vulkan_mesh_shader_ready",
        "official_apple_metal_ready",
        "official_directstorage_ready",
        "official_nanite_docs_ready",
        "official_profiler_docs_ready",
        "mavg_mesh_shader_lod_d3d12_ready",
        "mavg_mesh_shader_lod_vulkan_ready",
        "mavg_metal_mesh_lod_ready",
        "mavg_package_visible_backend_readiness_ready",
        "mavg_autonomous_streaming_scheduler_ready",
        "mavg_async_overlap_measured_performance_ready",
        "mavg_deformation_integration_ready",
        "mavg_ray_tracing_integration_ready",
        "mavg_broad_cpu_gpu_memory_optimization_ready",
        "mavg_nanite_comparison_report_ready",
        "bool mavg_nanite_compatible{false};",
        "bool mavg_nanite_equivalent{false};",
        "bool mavg_nanite_superior{false};",
        "request_current_active_plan_mutation"
    )) {
    Assert-ContainsText $mavgAdvancedHeaderText $needle "mavg_advanced_backend_evidence.hpp public contract"
}

foreach ($needle in @(
        "days_in_month",
        "result.source_gate_ready = result.missing_source_gate_count == 0U",
        "missing_context7_source",
        "missing_official_source",
        "missing_official_source_doc_date",
        "mavg_mesh_shader_lod_ready",
        "mavg_nanite_product_claim",
        "currentActivePlan",
        "result.mavg_advanced_backend_evidence_ready"
    )) {
    Assert-ContainsText $mavgAdvancedSourceText $needle "mavg_advanced_backend_evidence.cpp fail-closed implementation"
}

foreach ($needle in @(
        "fails closed when matrix is empty",
        "accepts fresh sources but keeps missing tasks unclaimed",
        "rejects stale source gates",
        "rejects invalid calendar source gate dates",
        "rejects missing official source rows",
        "succeeds only when every required row is ready",
        "blocks nanite compatibility equivalence and superiority claims",
        "rejects current active plan mutation requests"
    )) {
    Assert-ContainsText $mavgAdvancedTestsText $needle "MK_runtime_rhi_mavg_advanced_backend_evidence_tests coverage"
}

Assert-ContainsText $runtimeRhiCMakeText "src/mavg_advanced_backend_evidence.cpp" "engine/runtime_rhi/CMakeLists.txt advanced evidence source registration"
Assert-ContainsText $rootCMakeText "MK_runtime_rhi_mavg_advanced_backend_evidence_tests" "root CMake advanced evidence test target"

foreach ($needle in @(
        "RuntimeMavgAutonomousStreamingSchedulerState",
        "RuntimeMavgAutonomousStreamingSchedulerDesc",
        "RuntimeMavgAutonomousStreamingSchedulerResult",
        "tick_runtime_mavg_autonomous_streaming_scheduler",
        "RuntimeMavgAutonomousStreamingIoBackendKind",
        "RuntimeMavgAutonomousStreamingSafePointPolicy",
        "mavg_autonomous_streaming_scheduler_ready",
        "exposed_native_handles",
        "proved_async_overlap_performance"
    )) {
    Assert-ContainsText $mavgAutonomousSchedulerHeaderText $needle "mavg_autonomous_streaming_scheduler.hpp public contract"
}

foreach ($needle in @(
        "select_mavg_lod_clusters",
        "plan_runtime_mavg_page_streaming_requests",
        "load_runtime_mavg_payload_pages_from_filesystem",
        "load_runtime_mavg_payload_pages_from_direct_storage",
        "tick_runtime_mavg_page_streaming_background_service",
        "plan_runtime_mavg_gpu_memory_pressure_residency",
        "execute_runtime_mavg_cluster_streaming_safe_point_adoption",
        "result.exposed_native_handles = false",
        "result.proved_async_overlap_performance = false"
    )) {
    Assert-ContainsText $mavgAutonomousSchedulerSourceText $needle "mavg_autonomous_streaming_scheduler.cpp implementation"
}

foreach ($needle in @(
        "selects dispatches adopts and evicts without page requests",
        "keeps pending rows and coalesces duplicates across frames",
        "responds to camera movement and page heat priority",
        "routes payload reads through directstorage executor",
        "fails closed on directstorage executor failure",
        "cancels selected pages before io",
        "preserves safe point atomicity on invalid mount rows"
    )) {
    Assert-ContainsText $mavgAutonomousSchedulerTestsText $needle "MK_runtime_rhi_mavg_autonomous_streaming_scheduler_tests coverage"
}

foreach ($needle in @(
        "MK_runtime_rhi_mavg_autonomous_streaming_scheduler_tests",
        "mavg_autonomous_streaming_scheduler_ready=1",
        "mavg_autonomous_streaming_scheduler_native_handles_exposed=0",
        "mavg_autonomous_streaming_scheduler_async_overlap_performance_proof=0",
        "mavg_autonomous_streaming_scheduler_broad_backend_readiness=0"
    )) {
    Assert-ContainsText $mavgAutonomousSchedulerValidationText $needle "validate-mavg-autonomous-streaming-scheduler.ps1 counters"
}

Assert-ContainsText $runtimeRhiCMakeText "src/mavg_autonomous_streaming_scheduler.cpp" "engine/runtime_rhi/CMakeLists.txt autonomous scheduler source registration"
Assert-ContainsText $rootCMakeText "MK_runtime_rhi_mavg_autonomous_streaming_scheduler_tests" "root CMake autonomous scheduler test target"

foreach ($surface in @(
        @{ Text = $mavgAdvancedSpecText; Label = "MAVG advanced evidence spec" },
        @{ Text = $mavgAdvancedPlanText; Label = "MAVG advanced evidence plan" },
        @{ Text = $planRegistryText; Label = "plan registry" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" }
    )) {
    foreach ($needle in @(
            "mavg-advanced-backend-evidence-closeout-v1",
            "mavg_advanced_backend_evidence.hpp",
            "MavgAdvancedBackendEvidenceDesc",
            "MavgAdvancedBackendEvidenceResult",
            "evaluate_mavg_advanced_backend_evidence",
            "mavg_mesh_shader_lod_ready",
            "mavg_package_visible_backend_readiness_ready",
            "mavg_autonomous_streaming_scheduler_ready",
            "mavg_async_overlap_measured_performance_ready",
            "mavg_deformation_integration_ready",
            "mavg_ray_tracing_integration_ready",
            "mavg_broad_cpu_gpu_memory_optimization_ready",
            "mavg_nanite_comparison_report_ready",
            "mavg-autonomous-streaming-scheduler-v1",
            "RuntimeMavgAutonomousStreamingSchedulerResult",
            "mavg_autonomous_streaming_scheduler_ready=1"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) advanced evidence claim matrix"
    }
    foreach ($needle in @(
            "Nanite compatibility",
            "equivalence",
            "superiority",
            "currentActivePlan",
            "native handles",
            "cross-backend inference",
            "broad readiness"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) advanced evidence non-claims"
    }
    foreach ($forbiddenNeedle in @(
            "mavg_nanite_compatible=1",
            "mavg_nanite_equivalent=1",
            "mavg_nanite_superior=1"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden Nanite claim"
    }
}

$runtimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($runtimeRhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module" }
$runtimeRhiManifestText = ((@($runtimeRhiModule[0].publicHeaders) -join " "),
    (@($runtimeRhiModule[0].recentEvidence) -join " "),
    [string]$runtimeRhiModule[0].purpose) -join " "
foreach ($needle in @(
        "mavg_advanced_backend_evidence.hpp",
        "MavgAdvancedBackendEvidenceDesc",
        "MavgAdvancedBackendEvidenceResult",
        "evaluate_mavg_advanced_backend_evidence",
        "mavg_nanite_compatible",
        "mavg_nanite_equivalent",
        "mavg_nanite_superior",
        "currentActivePlan",
        "mavg_autonomous_streaming_scheduler_ready",
        "RuntimeMavgAutonomousStreamingSchedulerResult"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi advanced evidence"
}
