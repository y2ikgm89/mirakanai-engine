#requires -Version 7.0
#requires -PSEdition Core
# Chapter 113 for check-ai-integration.ps1 static contracts.

$platformFilesystemHeaderText = Get-AgentSurfaceText "engine/platform/include/mirakana/platform/filesystem.hpp"
$platformFilesystemSourceText = Get-AgentSurfaceText "engine/platform/src/filesystem.cpp"
$coreTestsText = Get-AgentSurfaceText "tests/unit/core_tests.cpp"
$runtimeMavgPayloadPagesHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp"
$runtimeMavgPayloadPagesSourceText = Get-AgentSurfaceText "engine/runtime/src/mavg_payload_pages.cpp"
$runtimeMavgPayloadPagesTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_payload_pages_tests.cpp"
$mavgByteRangePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-payload-byte-range-file-io-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "std::span<const std::uint8_t>",
        "read_bytes",
        "read_byte_range",
        "write_bytes",
        "MemoryFileSystem",
        "RootedFileSystem"
    )) {
    Assert-ContainsText $platformFilesystemHeaderText $needle "engine/platform/include/mirakana/platform/filesystem.hpp byte-range API"
}

foreach ($needle in @(
        "bytes_from_text",
        "text_from_bytes",
        "slice_bytes",
        "checked_file_size",
        "MemoryFileSystem::read_byte_range",
        "RootedFileSystem::read_byte_range",
        "RootedFileSystem::write_bytes",
        "std::ios::binary"
    )) {
    Assert-ContainsText $platformFilesystemSourceText $needle "engine/platform/src/filesystem.cpp byte-range implementation"
}

foreach ($needle in @(
        "memory filesystem reads exact binary byte ranges",
        "rooted filesystem reads exact binary byte ranges under a root",
        "read_byte_range",
        "write_bytes"
    )) {
    Assert-ContainsText $coreTestsText $needle "tests/unit/core_tests.cpp byte-range filesystem coverage"
}

foreach ($needle in @(
        "RuntimeMavgPayloadPageFileLoadDiagnosticCode",
        "RuntimeMavgPayloadPageFileLoadDesc",
        "RuntimeMavgPayloadPageFileLoadRow",
        "RuntimeMavgPayloadPageFileLoadResult",
        "payload_file_read_failed",
        "invoked_file_io",
        "used_native_directstorage",
        "mutated_mount_set",
        "executed_background_worker",
        "touched_renderer_or_rhi_handles",
        "load_runtime_mavg_payload_file_pages"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesHeaderText $needle "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp file load API"
}

foreach ($needle in @(
        "validate_mavg_cluster_graph",
        "deserialize_mavg_cluster_payload_document",
        "validate_mavg_cluster_payload",
        "duplicate_requested_page",
        "unknown_page",
        "missing_payload_page",
        "payload_file_read_failed",
        "read_byte_range",
        "used_native_directstorage = false"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesSourceText $needle "engine/runtime/src/mavg_payload_pages.cpp file load implementation"
}

foreach ($needle in @(
        "runtime mavg payload file pages read selected byte ranges from a filesystem blob",
        "runtime mavg payload file pages fail closed when blob byte range is unavailable",
        "RuntimeMavgPayloadPageFileLoadDesc",
        "payload_file_read_failed",
        "used_native_directstorage"
    )) {
    Assert-ContainsText $runtimeMavgPayloadPagesTestsText $needle "tests/unit/runtime_mavg_payload_pages_tests.cpp file load coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" },
        @{ Text = $mavgByteRangePlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-payload-byte-range-file-io-v1.md" }
    )) {
    foreach ($needle in @(
            "mavg-payload-byte-range-file-io-v1",
            "IFileSystem",
            "read_byte_range",
            "RuntimeMavgPayloadPageFileLoadResult",
            "load_runtime_mavg_payload_file_pages",
            "used_native_directstorage=false",
            "DirectStorage",
            "async-overlap/performance",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG byte-range file IO evidence"
    }
}

foreach ($needle in @(
        "mavg-payload-byte-range-file-io-v1",
        "docs/superpowers/plans/2026-06-05-mavg-payload-byte-range-file-io-v1.md",
        "MAVG Payload Byte-Range File IO v1",
        "IFileSystem read_bytes",
        "write_bytes",
        "read_byte_range",
        "RuntimeMavgPayloadPageFileLoadResult",
        "RuntimeMavgPayloadPageFileLoadDesc",
        "RuntimeMavgPayloadPageFileLoadRow",
        "load_runtime_mavg_payload_file_pages",
        "used_native_directstorage=false",
        "native DirectStorage queues/fences/status arrays",
        "async-overlap/performance",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json MAVG byte-range active pointer"
}

foreach ($needle in @(
        "MAVG Payload Byte-Range File IO v1",
        "IFileSystem read_bytes",
        "write_bytes",
        "read_byte_range",
        "MemoryFileSystem and RootedFileSystem exact byte-range coverage",
        "RuntimeMavgPayloadPageFileLoadDesc",
        "RuntimeMavgPayloadPageFileLoadRow",
        "RuntimeMavgPayloadPageFileLoadResult",
        "load_runtime_mavg_payload_file_pages",
        "used_native_directstorage=false",
        "native DirectStorage queues/fences/status arrays",
        "async-overlap/performance"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MAVG byte-range module evidence"
}

$platformModule = @($manifest.modules | Where-Object { $_.name -eq "MK_platform" })
if ($platformModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_platform module" }
if (@($platformModule[0].publicHeaders) -notcontains "engine/platform/include/mirakana/platform/filesystem.hpp") {
    Write-Error "engine/agent/manifest.json MK_platform publicHeaders missing filesystem.hpp"
}
$platformManifestText = ((@($platformModule[0].recentEvidence) -join " "), [string]$platformModule[0].purpose) -join " "
foreach ($needle in @(
        "IFileSystem read_bytes",
        "write_bytes",
        "read_byte_range",
        "MemoryFileSystem and RootedFileSystem byte-range coverage"
    )) {
    Assert-ContainsText $platformManifestText $needle "engine/agent/manifest.json MK_platform byte-range evidence"
}

$runtimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($runtimeModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module" }
if (@($runtimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime publicHeaders missing mavg_payload_pages.hpp"
}
$runtimeManifestText = ((@($runtimeModule[0].recentEvidence) -join " "), [string]$runtimeModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG Payload Byte-Range File IO v1",
        "RuntimeMavgPayloadPageFileLoadResult",
        "RuntimeMavgPayloadPageFileLoadDesc",
        "RuntimeMavgPayloadPageFileLoadRow",
        "load_runtime_mavg_payload_file_pages",
        "invoked_file_io=true",
        "used_native_directstorage=false",
        "no native DirectStorage queues/fences/status arrays"
    )) {
    Assert-ContainsText $runtimeManifestText $needle "engine/agent/manifest.json MK_runtime byte-range evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if ([string]$productionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-05-mavg-payload-byte-range-file-io-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must select MAVG Payload Byte-Range File IO v1"
}
if ([string]$productionLoop.recommendedNextPlan.id -ne "mavg-payload-byte-range-file-io-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must select mavg-payload-byte-range-file-io-v1"
}
