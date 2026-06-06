#requires -Version 7.0
#requires -PSEdition Core
# Chapter 130 for check-ai-integration.ps1 static contracts.

$runtimeHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp"
$win32HeaderText = Get-AgentSurfaceText "engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp"
$directStorageSourceText = Get-AgentSurfaceText "engine/runtime_host/win32/src/win32_mavg_directstorage_payload_io.cpp"
$d3d12PrivateHeaderText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_directstorage_private.hpp"
$d3d12SourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_backend.cpp"
$runtimeTestsText = Get-AgentSurfaceText "tests/unit/runtime_mavg_payload_pages_tests.cpp"
$d3d12TestsText = Get-AgentSurfaceText "tests/unit/d3d12_rhi_tests.cpp"
$directStorageSdkTestsText = Get-AgentSurfaceText "tests/unit/runtime_host_win32_directstorage_sdk_tests.cpp"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-directstorage-rhi-resource-destination-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$mavgSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$validationRecipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "used_directstorage_caller_owned_rhi_resource_destination",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $runtimeHeaderText $needle "mavg_payload_pages.hpp caller-owned RHI DirectStorage destination evidence"
}

foreach ($needle in @(
        "directstorage_rhi_device",
        "directstorage_rhi_destination_buffer",
        "rhi::IRhiDevice*",
        "rhi::BufferHandle"
    )) {
    Assert-ContainsText $win32HeaderText $needle "win32_mavg_payload_io.hpp first-party RHI destination descriptor fields"
}

foreach ($needle in @(
        "ID3D12Resource",
        "ID3D12Device",
        "IDStorage",
        "DSTORAGE_",
        "#include <d3d12.h>",
        "#include <dstorage.h>",
        "#include <windows.h>",
        "ComPtr"
    )) {
    Assert-DoesNotContainText $win32HeaderText $needle "win32_mavg_payload_io.hpp must keep DirectStorage/D3D12/COM symbols private"
}

foreach ($needle in @(
        "D3d12DirectStorageBufferDestination",
        "D3d12DirectStorageBufferDestinationStatus",
        "resolve_directstorage_buffer_destination",
        "Microsoft::WRL::ComPtr<ID3D12Device>",
        "Microsoft::WRL::ComPtr<ID3D12Resource>",
        "exposed_native_handles"
    )) {
    Assert-ContainsText $d3d12PrivateHeaderText $needle "d3d12_directstorage_private.hpp private caller-owned destination resolver contract"
}

foreach ($needle in @(
        "resolve_directstorage_device_context_buffer_destination",
        "resolve_directstorage_buffer_destination(IRhiDevice& device",
        "D3D12_HEAP_TYPE_DEFAULT",
        "GetHeapProperties",
        "GetDevice",
        "copy_destination_compatible",
        "default_heap_resource",
        "unsupported_backend"
    )) {
    Assert-ContainsText $d3d12SourceText $needle "d3d12_backend.cpp private DirectStorage buffer destination resolver"
}

foreach ($needle in @(
        "resolve_caller_owned_rhi_resource_destination",
        "directstorage_rhi_device",
        "directstorage_rhi_destination_buffer",
        "resolve_directstorage_buffer_destination",
        "used_directstorage_caller_owned_rhi_resource_destination",
        "resource_destination_device",
        "queue_desc.Device = use_resource_destination ? submission.resource_destination_device.Get() : nullptr",
        "create_completion_fence(result, resource_destination_device.Get()",
        "result.touched_renderer_or_rhi_handles = used_caller_owned_rhi_resource_destination",
        "result.touched_renderer_or_rhi_handles = submission.used_directstorage_caller_owned_rhi_resource_destination"
    )) {
    Assert-ContainsText $directStorageSourceText $needle "win32_mavg_directstorage_payload_io.cpp caller-owned RHI resource destination execution"
}

foreach ($needle in @(
        "runtime mavg payload native io dispatch propagates caller owned rhi destination evidence",
        "simulate_caller_owned_rhi_resource_destination",
        "used_directstorage_caller_owned_rhi_resource_destination",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $runtimeTestsText $needle "MK_runtime_mavg_payload_pages_tests caller-owned RHI destination propagation"
}

foreach ($needle in @(
        "d3d12 directstorage private resolver exposes caller owned buffer destination internally",
        "d3d12 directstorage private resolver rejects unsupported unknown and incompatible buffers",
        "resolve_directstorage_buffer_destination",
        "D3d12DirectStorageBufferDestinationStatus::ready",
        "D3d12DirectStorageBufferDestinationStatus::unsupported_backend",
        "D3d12DirectStorageBufferDestinationStatus::invalid_buffer",
        "D3d12DirectStorageBufferDestinationStatus::incompatible_buffer"
    )) {
    Assert-ContainsText $d3d12TestsText $needle "MK_d3d12_rhi_tests private DirectStorage resolver coverage"
}

foreach ($needle in @(
        "run_directstorage_caller_owned_rhi_buffer_destination_execution_test",
        "run_directstorage_caller_owned_rhi_buffer_destination_rejection_test",
        "directstorage_rhi_device = device.get()",
        "directstorage_rhi_destination_buffer = destination",
        "used_directstorage_caller_owned_rhi_resource_destination",
        "touched_renderer_or_rhi_handles"
    )) {
    Assert-ContainsText $directStorageSdkTestsText $needle "MK_runtime_host_win32_directstorage_sdk_tests caller-owned RHI destination coverage"
}

foreach ($surface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-06-06-mavg-directstorage-rhi-resource-destination-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $mavgSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $validationRecipesFragmentText; Label = "engine/agent/manifest.fragments/009-validationRecipes.json" },
        @{ Text = $aiLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" }
    )) {
    foreach ($needle in @(
            "MAVG DirectStorage RHI Resource Destination v1",
            "directstorage_rhi_device",
            "directstorage_rhi_destination_buffer",
            "resolve_directstorage_buffer_destination",
            "used_directstorage_caller_owned_rhi_resource_destination"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG DirectStorage RHI destination evidence"
    }
    foreach ($needle in @(
            "GPU decompression",
            "page-to-resource",
            "async-overlap/performance",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG DirectStorage RHI destination non-claims"
    }
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-directstorage-rhi-resource-destination-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG DirectStorage RHI Resource Destination v1"
}

$directStorageModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_host_win32_directstorage" })
if ($directStorageModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_host_win32_directstorage module" }
$directStorageManifestText = ((@($directStorageModule[0].dependencies) -join " "), (@($directStorageModule[0].publicHeaders) -join " "), (@($directStorageModule[0].recentEvidence) -join " "), [string]$directStorageModule[0].purpose) -join " "
foreach ($needle in @(
        "MK_rhi",
        "MK_rhi_d3d12",
        "MAVG DirectStorage RHI Resource Destination v1",
        "directstorage_rhi_device",
        "directstorage_rhi_destination_buffer",
        "resolve_directstorage_buffer_destination",
        "used_directstorage_caller_owned_rhi_resource_destination",
        "touched_renderer_or_rhi_handles=true",
        "not GPU decompression",
        "not full page-to-resource residency integration"
    )) {
    Assert-ContainsText $directStorageManifestText $needle "engine/agent/manifest.json MK_runtime_host_win32_directstorage RHI destination evidence"
}
