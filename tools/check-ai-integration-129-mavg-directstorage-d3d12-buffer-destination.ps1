#requires -Version 7.0
#requires -PSEdition Core
# Chapter 129 for check-ai-integration.ps1 static contracts.

$runtimePublicHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp"
$win32PublicHeaderText = Get-AgentSurfaceText "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp"
$directStorageSourceText = Get-AgentSurfaceText "engine/runtime_host/win32/src/win32_mavg_directstorage_payload_io.cpp"
$runtimeTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_payload_pages_tests.cpp"
$directStorageSdkTestsText = Get-AgentSurfaceText "tests/unit/runtime_host_win32_directstorage_sdk_tests.cpp"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-directstorage-d3d12-buffer-destination-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$validationRecipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "use_directstorage_d3d12_buffer_destination",
        "used_directstorage_resource_destination",
        "directstorage_resource_destination_request_count",
        "directstorage_resource_destination_bytes",
        "std::span<std::uint8_t> destination_memory",
        "signal_fence_after_requests"
    )) {
    Assert-ContainsText $runtimePublicHeaderText $needle "mavg_payload_pages.hpp DirectStorage D3D12 buffer destination first-party evidence contract"
}

foreach ($needle in @(
        "ID3D12Resource",
        "ID3D12Device",
        "IDStorage",
        "DSTORAGE_",
        "#include <d3d12.h>",
        "#include <dstorage.h>",
        "#include <windows.h>",
        "HANDLE",
        "ComPtr"
    )) {
    Assert-DoesNotContainText $runtimePublicHeaderText $needle "mavg_payload_pages.hpp must remain native-symbol free"
    Assert-DoesNotContainText $win32PublicHeaderText $needle "win32_mavg_payload_io.hpp must keep DirectStorage/D3D12 handles private"
}

foreach ($needle in @(
        "Microsoft::WRL::ComPtr<ID3D12Resource>",
        "create_resource_destination",
        "CreateCommittedResource",
        "D3D12_RESOURCE_STATE_COPY_DEST",
        "use_directstorage_d3d12_buffer_destination",
        "DSTORAGE_REQUEST_DESTINATION_BUFFER",
        "storage_request.Destination.Buffer.Resource",
        "storage_request.Destination.Buffer.Offset",
        "storage_request.Destination.Buffer.Size",
        "queue_desc.Device = use_resource_destination ?",
        "used_directstorage_resource_destination = true",
        "directstorage_resource_destination_request_count",
        "directstorage_resource_destination_bytes"
    )) {
    Assert-ContainsText $directStorageSourceText $needle "win32_mavg_directstorage_payload_io.cpp private D3D12 buffer resource destination proof"
}

foreach ($needle in @(
        "runtime mavg payload native io dispatch propagates directstorage d3d12 buffer destination evidence",
        "use_directstorage_d3d12_buffer_destination = true",
        "used_directstorage_resource_destination",
        "directstorage_resource_destination_request_count == 2U",
        "directstorage_resource_destination_bytes == 128U",
        "last_destination_memory_bytes == 0U",
        "!result.touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $runtimeTestsText $needle "MK_runtime_mavg_payload_pages_tests D3D12 buffer destination contract propagation"
}

foreach ($needle in @(
        "run_directstorage_d3d12_buffer_destination_execution_test",
        "use_directstorage_d3d12_buffer_destination = true",
        "used_directstorage_resource_destination",
        "directstorage_resource_destination_request_count == 2U",
        "directstorage_resource_destination_bytes == 8U",
        "status.native_fence_completed_value >= status.native_fence_signal_value",
        "!status.touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $directStorageSdkTestsText $needle "MK_runtime_host_win32_directstorage_sdk_tests optional D3D12 buffer destination coverage"
}

foreach ($surface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-06-06-mavg-directstorage-d3d12-buffer-destination-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $validationRecipesFragmentText; Label = "engine/agent/manifest.fragments/009-validationRecipes.json" },
        @{ Text = $aiLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" }
    )) {
    foreach ($needle in @(
            "MAVG DirectStorage D3D12 Buffer Destination v1",
            "use_directstorage_d3d12_buffer_destination",
            "DSTORAGE_REQUEST_DESTINATION_BUFFER",
            "DSTORAGE_DESTINATION_BUFFER",
            "DSTORAGE_QUEUE_DESC::Device",
            "ID3D12Resource",
            "used_directstorage_resource_destination",
            "directstorage_resource_destination_request_count",
            "directstorage_resource_destination_bytes"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG DirectStorage D3D12 buffer destination evidence"
    }
    foreach ($needle in @(
            "no default DirectStorage dependency",
            "GPU decompression",
            "full page-to-resource residency",
            "async-overlap/performance",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG DirectStorage D3D12 buffer destination non-claims"
    }
}

foreach ($surface in @(
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" }
    )) {
    foreach ($needle in @(
            "MAVG DirectStorage D3D12 Buffer Destination v1",
            "private",
            "GPU decompression",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG DirectStorage D3D12 buffer destination high-level tracking"
    }
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-directstorage-d3d12-buffer-destination-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG DirectStorage D3D12 Buffer Destination v1"
}

$directStorageModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_host_win32_directstorage" })
if ($directStorageModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_host_win32_directstorage module" }
$directStorageManifestText = ((@($directStorageModule[0].dependencies) -join " "), (@($directStorageModule[0].publicHeaders) -join " "), (@($directStorageModule[0].recentEvidence) -join " "), [string]$directStorageModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG DirectStorage D3D12 Buffer Destination v1",
        "DSTORAGE_REQUEST_DESTINATION_BUFFER",
        "non-null DSTORAGE_QUEUE_DESC::Device",
        "private ID3D12Resource buffer",
        "used_directstorage_resource_destination",
        "directstorage_resource_destination_bytes",
        "not full page-to-resource residency integration",
        "not GPU decompression"
    )) {
    Assert-ContainsText $directStorageManifestText $needle "engine/agent/manifest.json MK_runtime_host_win32_directstorage D3D12 buffer destination evidence"
}
