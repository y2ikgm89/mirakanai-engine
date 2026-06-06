#requires -Version 7.0
#requires -PSEdition Core
# Chapter 104 for check-ai-integration.ps1 static contracts.

$runtimeRhiMavgConventionalHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_conventional_upload.hpp"
$runtimeRhiMavgConventionalSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_conventional_upload.cpp"
$runtimeRhiMavgConventionalTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_conventional_upload_tests.cpp"
$runtimeMavgPageStreamingHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp"
$runtimeMavgPageStreamingSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_page_streaming.cpp"
$runtimeMavgPageStreamingTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_page_streaming_tests.cpp"
$assetsMavgPayloadHeaderText = Get-AgentSurfaceText "engine/assets/include/mirakana/assets/mavg_cluster_payload.hpp"
$runtimeMavgPayloadPagesHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp"
$runtimeMavgPayloadPagesTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_payload_pages_tests.cpp"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"

foreach ($needle in @(
        "RuntimeMavgPageStreamingCandidateRow",
        "RuntimeMavgPageStreamingPlanResult",
        "RuntimeMavgPageStreamingDrainResult",
        "RuntimeMavgResidentPageMountRow",
        "RuntimeMavgPageStreamingSelectedClusterRow",
        "RuntimeMavgPageStreamingEvictionReviewResult",
        "RuntimeMavgPageStreamingDispatchMode",
        "RuntimeMavgPageStreamingDispatchDesc",
        "RuntimeMavgPageStreamingDispatchRow",
        "RuntimeMavgPageStreamingDispatchPlan",
        "plan_runtime_mavg_page_streaming_requests",
        "plan_runtime_mavg_page_streaming_dispatches",
        "review_runtime_mavg_page_streaming_evictions",
        "execute_runtime_mavg_page_streaming_request_safe_point",
        "executed_background_worker",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $runtimeMavgPageStreamingHeaderText $needle "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp"
}
foreach ($needle in @(
        "commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2",
        "plan_runtime_resident_package_evictions_v2",
        "selected_graph_mismatch",
        "missing_page_mount",
        "max_queued_pages",
        "max_dispatch_rows",
        "missing_candidate",
        "invalid_dispatch_row",
        "duplicate_dispatch_mount_id",
        "unsafe_dispatch_mode",
        "safe_point_failed"
    )) {
    Assert-ContainsText $runtimeMavgPageStreamingSourceText $needle "engine/runtime/src/mavg_page_streaming.cpp"
}
foreach ($needle in @(
        "runtime_inferred_frequency",
        "RuntimeMavgPageStreamingFrequencyRow",
        "RuntimeMavgResidentPageFrequencyDesc",
        "RuntimeMavgResidentPageFrequencyResult",
        "infer_runtime_mavg_resident_page_frequencies",
        "inferred_resident_page_frequency",
        "runtime_inferred_frequency_eviction_candidate_count",
        "inferred_frequency_eviction_policy",
        "previous_frequency_rows",
        "frequency_counter_overflow"
    )) {
    Assert-ContainsText $runtimeMavgPageStreamingHeaderText $needle "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp MAVG runtime-inferred frequency evidence"
}
foreach ($needle in @(
        "uses_frequency_eviction_order",
        "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency",
        "copy_frequency_evidence",
        "find_frequency_row",
        "result.inferred_frequency_eviction_policy = true",
        "result.runtime_inferred_frequency_eviction_candidate_count = eviction_candidates.size()",
        "RuntimeMavgPageStreamingDiagnosticCode::frequency_counter_overflow"
    )) {
    Assert-ContainsText $runtimeMavgPageStreamingSourceText $needle "engine/runtime/src/mavg_page_streaming.cpp MAVG runtime-inferred frequency evidence"
}
foreach ($needle in @(
        "runtime mavg page streaming planner coalesces nonresident requests deterministically",
        "runtime mavg page streaming planner applies deterministic max page budget",
        "runtime mavg page streaming dispatch planner builds safe point rows deterministically",
        "runtime mavg page streaming dispatch planner records caller owned background queue without executing worker",
        "runtime mavg page streaming dispatch planner rejects unsafe no safe point mutation boundary",
        "runtime mavg page streaming eviction review protects selected pages and fallback ancestors",
        "runtime mavg page streaming eviction review rejects protected selected eviction candidate",
        "runtime mavg page streaming executes one queued row through reviewed safe point"
    )) {
    Assert-ContainsText $runtimeMavgPageStreamingTestsText $needle "tests/unit/runtime_mavg_page_streaming_tests.cpp"
}
foreach ($needle in @(
        "runtime mavg page streaming runtime inferred frequency policy orders least selected unprotected pages first",
        "runtime mavg page streaming runtime inferred frequency policy evicts cold residents first",
        "runtime mavg page streaming runtime inferred frequency policy rejects counter overflow",
        "runtime mavg page streaming frequency inference drops nonresident rows and deduplicates selected pages",
        "runtime mavg page streaming frequency inference rejects duplicate previous rows"
    )) {
    Assert-ContainsText $runtimeMavgPageStreamingTestsText $needle "tests/unit/runtime_mavg_page_streaming_tests.cpp MAVG runtime-inferred frequency coverage"
}

foreach ($needle in @(
        "MavgClusterPayloadDocument",
        "MavgClusterPayloadPage",
        "MavgClusterPayloadCluster",
        "page_data_hex",
        "mavg_cluster_payload_format_v1",
        "validate_mavg_cluster_payload"
    )) {
    Assert-ContainsText $assetsMavgPayloadHeaderText $needle "engine/assets/include/mirakana/assets/mavg_cluster_payload.hpp"
}
foreach ($needle in @(
        "RuntimeMavgPayloadPageSliceResult",
        "payload_bytes",
        "extract_runtime_mavg_payload_page_slices",
        "RuntimeMavgPayloadPageFileLoadResult",
        "load_runtime_mavg_payload_file_pages",
        "invoked_file_io",
        "used_native_directstorage",
        "mutated_mount_set",
        "executed_background_worker",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesHeaderText $needle "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp"
}
foreach ($needle in @(
        "runtime mavg payload page slices extract requested decoded bytes in request order",
        "runtime mavg payload page slices reject duplicate and unknown page requests before extraction",
        "runtime mavg payload page slices reject invalid payload schema before extraction",
        "runtime mavg payload file pages read selected byte ranges from a filesystem blob",
        "runtime mavg payload file pages fail closed when blob byte range is unavailable"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesTestsText $needle "tests/unit/runtime_mavg_payload_pages_tests.cpp"
}

foreach ($needle in @(
        "RuntimeMavgConventionalMeshUploadDesc",
        "RuntimeMavgConventionalMeshBinding",
        "RuntimeMavgConventionalMeshUploadResult",
        "upload_runtime_mavg_conventional_mesh_binding",
        "executed_gpu_culling",
        "executed_indirect_draw",
        "executed_mesh_shader",
        "touched_native_handles"
    )) {
    Assert-ContainsText $runtimeRhiMavgConventionalHeaderText $needle "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_conventional_upload.hpp"
}
foreach ($needle in @(
        "AssetKind::mavg_cluster_graph",
        "upload_runtime_mesh",
        "wait_for_runtime_uploads_on_queue",
        "package-streaming-not-committed",
        "runtime-resource-not-mavg-cluster-graph",
        "mavg-draw-range-outside-payload"
    )) {
    Assert-ContainsText $runtimeRhiMavgConventionalSourceText $needle "engine/runtime_rhi/src/mavg_conventional_upload.cpp"
}
foreach ($needle in @(
        "mavg conventional upload publishes package visible mesh binding for scene lod planning",
        "mavg conventional upload rejects non committed streaming before creating gpu buffers",
        "mavg conventional upload rejects graph draw ranges outside runtime payload"
    )) {
    Assert-ContainsText $runtimeRhiMavgConventionalTestsText $needle "tests/unit/runtime_rhi_mavg_conventional_upload_tests.cpp"
}
foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" }
    )) {
    foreach ($needle in @("mavg_conventional_upload.hpp", "RuntimeMavgConventionalMeshUploadResult", "upload_runtime_mavg_conventional_mesh_binding", "autonomous background")) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG conventional runtime upload evidence"
    }
    foreach ($needle in @("mavg_page_streaming.hpp", "RuntimeMavgPageStreamingPlanResult", "RuntimeMavgPageStreamingEvictionReviewResult", "RuntimeMavgPageStreamingDispatchPlan", "review_runtime_mavg_page_streaming_evictions", "plan_runtime_mavg_page_streaming_dispatches", "execute_runtime_mavg_page_streaming_request_safe_point", "autonomous background")) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG page streaming queue evidence"
    }
    foreach ($needle in @("mavg-runtime-inferred-frequency-eviction-policy-v1", "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency", "RuntimeMavgPageStreamingFrequencyRow", "RuntimeMavgResidentPageFrequencyDesc", "RuntimeMavgResidentPageFrequencyResult", "infer_runtime_mavg_resident_page_frequencies", "inferred_frequency_eviction_policy", "runtime_inferred_frequency_eviction_candidate_count", "frequency_counter_overflow")) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG runtime-inferred frequency evidence"
    }
    foreach ($needle in @("mavg_cluster_payload.hpp", "MavgClusterPayloadDocument", "page.data_hex", "mavg_payload_pages.hpp", "RuntimeMavgPayloadPageSliceResult", "extract_runtime_mavg_payload_page_slices", "RuntimeMavgPayloadPageFileLoadResult", "load_runtime_mavg_payload_file_pages", "read_byte_range", "used_native_directstorage", "DirectStorage/Win32")) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG page-addressable payload schema evidence"
    }
}
$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
if (@($runtimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime publicHeaders missing mavg_page_streaming.hpp"
}
if (@($runtimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime publicHeaders missing mavg_payload_pages.hpp"
}
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Page Streaming Queue v1",
        "RuntimeMavgPageStreamingCandidateRow",
        "RuntimeMavgPageStreamingPlanResult",
        "RuntimeMavgPageStreamingDrainResult",
        "RuntimeMavgPageStreamingEvictionReviewResult",
        "RuntimeMavgResidentPageMountRow",
    "RuntimeMavgPageStreamingSelectedClusterRow",
    "RuntimeMavgPageStreamingDispatchPlan",
    "plan_runtime_mavg_page_streaming_requests",
    "plan_runtime_mavg_page_streaming_dispatches",
    "review_runtime_mavg_page_streaming_evictions",
    "execute_runtime_mavg_page_streaming_request_safe_point",
    "MAVG Runtime-Inferred Frequency Eviction Policy v1",
    "RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency",
    "RuntimeMavgPageStreamingFrequencyRow",
    "RuntimeMavgResidentPageFrequencyDesc",
    "RuntimeMavgResidentPageFrequencyResult",
    "infer_runtime_mavg_resident_page_frequencies",
    "inferred_frequency_eviction_policy",
    "runtime_inferred_frequency_eviction_candidate_count",
    "frequency_counter_overflow",
    "autonomous background"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime MAVG page streaming queue evidence"
}
foreach ($needle in @(
        "MAVG Page-Addressable Payload Schema v1",
        "mavg_payload_pages.hpp",
        "RuntimeMavgPayloadPageSliceResult",
        "extract_runtime_mavg_payload_page_slices",
        "RuntimeMavgPayloadPageFileLoadResult",
        "load_runtime_mavg_payload_file_pages",
        "payload_bytes",
        "read_byte_range",
        "used_native_directstorage=false",
        "DirectStorage/Win32 async IO"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime MAVG payload schema evidence"
}
$runtimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($runtimeRhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module" }
if (@($runtimeRhiModule[0].publicHeaders) -notcontains "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_conventional_upload.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime_rhi publicHeaders missing mavg_conventional_upload.hpp"
}
$manifestText = ((@($runtimeRhiModule[0].recentEvidence) -join " "), [string]$runtimeRhiModule[0].purpose) -join " "
foreach ($needle in @("MAVG Conventional Runtime Upload Evidence v1", "RuntimeMavgConventionalMeshUploadDesc", "RuntimeMavgConventionalMeshUploadResult", "upload_runtime_mavg_conventional_mesh_binding", "AssetKind::mavg_cluster_graph", "background/page streaming execution")) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json MK_runtime_rhi MAVG conventional runtime upload evidence"
}
