#requires -Version 7.0
#requires -PSEdition Core
# Chapter 115 for check-ai-integration.ps1 static contracts.

$mavgGraphHeaderText = Get-AgentSurfaceText "engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp"
$mavgGraphSourceText = Get-AgentSurfaceText "engine/assets/src/mavg_cluster_graph.cpp"
$payloadLoaderHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_payload_page_loader.hpp"
$payloadLoaderSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_payload_page_loader.cpp"
$payloadLoaderTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_payload_page_loader_tests.cpp"
$graphTestsText = Get-AgentSurfaceText "tests/unit/mavg_cluster_graph_tests.cpp"
$cmakeText = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeCmakeText = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-11-mavg-payload-byte-range-page-loader-v1.md"
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
        "RuntimeMavgPayloadPageLoadResult",
        "RuntimeMavgPayloadPageRow",
        "RuntimeMavgPayloadPageLoadDiagnosticCode",
        "load_runtime_mavg_payload_pages",
        "invoked_file_io",
        "executed_background_worker",
        "executed_direct_storage",
        "touched_gpu_memory_policy",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $payloadLoaderHeaderText $needle "MAVG runtime payload page loader public contract"
}

foreach ($needle in @(
        "GameEngine.MavgClusterPayload.v1",
        "payload_starts_with_format",
        "duplicate_page_request",
        "page_range_out_of_bounds",
        "std::vector<std::byte>"
    )) {
    Assert-ContainsText $payloadLoaderSourceText $needle "MAVG runtime payload page loader implementation"
}

foreach ($needle in @(
        "runtime mavg payload page loader copies requested byte ranges without side effects",
        "runtime mavg payload page loader rejects duplicate missing and out of bounds pages",
        "runtime mavg payload page loader rejects invalid payload format",
        "MK_REQUIRE(!result.invoked_file_io)",
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
            "filesystem byte-range",
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

if ($manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must remain the production-completion master plan after MAVG payload byte-range page loader closeout"
}
if ($manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "next-production-gap-selection") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must remain next-production-gap-selection after MAVG payload byte-range page loader closeout"
}
