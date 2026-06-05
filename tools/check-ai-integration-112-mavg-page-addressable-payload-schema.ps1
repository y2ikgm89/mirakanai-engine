#requires -Version 7.0
#requires -PSEdition Core
# Chapter 112 for check-ai-integration.ps1 static contracts.

$assetsMavgPayloadHeaderText = Get-AgentSurfaceText "engine/assets/include/mirakana/assets/mavg_cluster_payload.hpp"
$assetsMavgPayloadSourceText = Get-AgentSurfaceText "engine/assets/src/mavg_cluster_payload.cpp"
$toolsMavgCookSourceText = Get-AgentSurfaceText "engine/tools/asset/mavg_cluster_cook.cpp"
$mavgPayloadTestsText = Get-AgentSurfaceText "tests/unit/mavg_cluster_payload_tests.cpp"
$runtimeMavgPayloadPagesHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp"
$runtimeMavgPayloadPagesSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_payload_pages.cpp"
$runtimeMavgPayloadPagesTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_payload_pages_tests.cpp"
$toolsMavgCookTestsText = Get-AgentSurfaceText "tests/unit/tools_mavg_cluster_cook_tests.cpp"
$mavgPayloadPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-page-addressable-payload-schema-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "MavgClusterPayloadDocument",
        "MavgClusterPayloadPage",
        "MavgClusterPayloadCluster",
        "page_data_hex",
        "MavgClusterPayloadDiagnosticCode",
        "overlapping_page_range",
        "cluster_page_mismatch",
        "cluster_draw_range_mismatch",
        "mavg_cluster_payload_format_v1",
        "deserialize_mavg_cluster_payload_document",
        "serialize_mavg_cluster_payload_document",
        "validate_mavg_cluster_payload"
    )) {
    Assert-ContainsText $assetsMavgPayloadHeaderText $needle "engine/assets/include/mirakana/assets/mavg_cluster_payload.hpp public API"
}

foreach ($needle in @(
        "GameEngine.MavgClusterPayload.v1",
        "vertex.data_hex",
        "index.data_hex",
        "page.data_hex",
        "page.count",
        "cluster.count",
        "missing_graph_page",
        "overlapping_page_range",
        "cluster_page_mismatch",
        "cluster_draw_range_mismatch"
    )) {
    Assert-ContainsText $assetsMavgPayloadSourceText $needle "engine/assets/src/mavg_cluster_payload.cpp schema implementation"
}

foreach ($needle in @(
        "RuntimeMavgPayloadPageSliceDesc",
        "RuntimeMavgPayloadPageSliceRow",
        "RuntimeMavgPayloadPageSliceResult",
        "RuntimeMavgPayloadPageSliceDiagnosticCode",
        "payload_bytes",
        "invoked_file_io",
        "mutated_mount_set",
        "executed_background_worker",
        "touched_renderer_or_rhi_handles",
        "extract_runtime_mavg_payload_page_slices"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesHeaderText $needle "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp public API"
}

foreach ($needle in @(
        "deserialize_mavg_cluster_payload_document",
        "validate_mavg_cluster_payload",
        "duplicate_requested_page",
        "unknown_page",
        "missing_payload_page",
        "decode_hex_bytes",
        "payload.page_data_hex"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesSourceText $needle "engine/runtime/src/mavg_payload_pages.cpp implementation"
}

foreach ($needle in @(
        "serialize_mavg_cluster_payload_document",
        "MavgClusterPayloadDocument",
        "MavgClusterPayloadPage",
        "MavgClusterPayloadCluster",
        "page_data_hex"
    )) {
    Assert-ContainsText $toolsMavgCookSourceText $needle "engine/tools/asset/mavg_cluster_cook.cpp payload schema cook"
}

foreach ($needle in @(
        "mavg cluster payload validates page addressable byte ranges against graph",
        "mavg cluster payload rejects duplicate overlapping and out of range pages",
        "mavg cluster payload rejects cluster page and draw range mismatches"
    )) {
    Assert-ContainsText $mavgPayloadTestsText $needle "tests/unit/mavg_cluster_payload_tests.cpp coverage"
}

foreach ($needle in @(
        "runtime mavg payload page slices extract requested decoded bytes in request order",
        "runtime mavg payload page slices reject duplicate and unknown page requests before extraction",
        "runtime mavg payload page slices reject invalid payload schema before extraction"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesTestsText $needle "tests/unit/runtime_mavg_payload_pages_tests.cpp coverage"
}

foreach ($needle in @(
        "page.count=2",
        "page.data_hex=",
        "page.0.index=0",
        "page.0.byte_offset=0",
        "page.0.byte_size=128",
        "page.1.index=1",
        "page.1.byte_offset=128",
        "cluster.0.page=0",
        "cluster.3.page=1"
    )) {
    Assert-ContainsText $toolsMavgCookTestsText $needle "tests/unit/tools_mavg_cluster_cook_tests.cpp payload cook coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgPayloadPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-page-addressable-payload-schema-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-page-addressable-payload-schema-v1",
            "mavg_cluster_payload.hpp",
            "MavgClusterPayloadDocument",
            "MavgClusterPayloadPage",
            "validate_mavg_cluster_payload",
            "page.data_hex",
            "mavg_payload_pages.hpp",
            "RuntimeMavgPayloadPageSliceResult",
            "extract_runtime_mavg_payload_page_slices",
            "DirectStorage/Win32",
            "autonomous background",
            "async",
            "automatic eviction policy",
            "GPU memory pressure",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG page-addressable payload evidence"
    }
}

foreach ($needle in @(
        "mavg-page-addressable-payload-schema-v1",
        "GameEngine.MavgClusterPayload.v1",
        "page.data_hex",
        "no-IO runtime page slice extraction",
        "autonomous worker execution",
        "automatic eviction policy",
        "GPU memory pressure integration"
    )) {
    Assert-ContainsText $mavgRuntimeLodPlanText $needle "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md active child"
}

foreach ($needle in @(
        "mavg-page-addressable-payload-schema-v1",
        "docs/superpowers/plans/2026-06-05-mavg-page-addressable-payload-schema-v1.md",
        "MAVG Page-Addressable Payload Schema v1",
        "mavg_cluster_payload.hpp",
        "MavgClusterPayloadDocument",
        "MavgClusterPayloadPage",
        "page.data_hex",
        "mavg_payload_pages.hpp",
        "RuntimeMavgPayloadPageSliceResult",
        "extract_runtime_mavg_payload_page_slices",
        "mavg-package-streaming-residency-dispatch-v1",
        "RuntimeMavgPageStreamingDispatchPlan",
        "plan_runtime_mavg_page_streaming_dispatches",
        "RuntimeMavgPageStreamingDrainDesc",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG payload active pointer"
}

foreach ($needle in @(
        "MAVG Page-Addressable Payload Schema v1",
        "mavg_cluster_payload.hpp",
        "MavgClusterPayloadDocument",
        "MavgClusterPayloadPage",
        "page.data_hex",
        "mavg_payload_pages.hpp",
        "RuntimeMavgPayloadPageSliceResult",
        "extract_runtime_mavg_payload_page_slices",
        "payload_bytes",
        "DirectStorage/Win32 async IO"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MAVG payload module evidence"
}

$assetsModule = @($manifest.modules | Where-Object { $_.name -eq "MK_assets" })
if ($assetsModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_assets module" }
if (@($assetsModule[0].publicHeaders) -notcontains "engine/assets/include/mirakana/assets/mavg_cluster_payload.hpp") {
    Write-Error "engine/agent/manifest.json MK_assets publicHeaders missing mavg_cluster_payload.hpp"
}
$assetsManifestText = ((@($assetsModule[0].recentEvidence) -join " "), [string]$assetsModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Page-Addressable Payload Schema v1",
        "MavgClusterPayloadDocument",
        "MavgClusterPayloadPage",
        "MavgClusterPayloadCluster",
        "validate_mavg_cluster_payload",
        "GameEngine.MavgClusterPayload.v1",
        "page.data_hex",
        "Nanite compatibility/equivalence/superiority"
    )) {
    Assert-ContainsText $assetsManifestText $needle "engine/agent/manifest.json MK_assets MAVG payload evidence"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
if (@($runtimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime publicHeaders missing mavg_payload_pages.hpp"
}
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Page-Addressable Payload Schema v1",
        "RuntimeMavgPayloadPageSliceResult",
        "extract_runtime_mavg_payload_page_slices",
        "payload_bytes",
        "zero file IO",
        "resident mount mutation",
        "background worker execution",
        "renderer/RHI handle",
        "DirectStorage/Win32 async IO"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime MAVG payload evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
$productionLoopText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence),
    ([string]$productionLoop.recommendedNextPlan.completedContext),
    ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
        "MAVG Page-Addressable Payload Schema v1",
        "mavg-page-addressable-payload-schema-v1",
        "RuntimeMavgPayloadPageSliceResult",
        "extract_runtime_mavg_payload_page_slices",
        "page.data_hex"
    )) {
    Assert-ContainsText $productionLoopText $needle "engine/agent/manifest.json retained MAVG page-addressable payload prerequisite"
}
