#requires -Version 7.0
#requires -PSEdition Core
# Chapter 126 for check-ai-integration.ps1 static contracts.

$rhiHeaderText = Get-AgentSurfaceText "engine/rhi/include/mirakana/rhi/rhi.hpp"
$nullRhiText = Get-AgentSurfaceText "engine/rhi/src/null_rhi.cpp"
$d3d12HeaderText = Get-AgentSurfaceText "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
$d3d12SourceText = Get-AgentSurfaceText "engine/rhi/d3d12/src/d3d12_backend.cpp"
$vulkanSourceText = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_backend.cpp"
$rhiTestsText = Get-AgentSurfaceText "tests/unit/rhi_tests.cpp"
$d3d12TestsText = Get-AgentSurfaceText "tests/unit/d3d12_rhi_tests.cpp"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgResidencyPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-06-mavg-d3d12-residency-action-execution-v1.md"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "RhiResidencyActionKind",
        "RhiResidencyResourceKind",
        "RhiResidencyActionStatus",
        "RhiResidencyResourceRef",
        "RhiResidencyActionDesc",
        "RhiResidencyActionResult",
        "std::span<const RhiResidencyResourceRef> resources",
        "invoked_native_make_resident",
        "invoked_native_evict",
        "exposed_native_handles",
        "enforced_allocator_budget",
        "native_error_code",
        "rhi_residency_action_kind_id",
        "rhi_residency_resource_kind_id",
        "rhi_residency_action_status_id",
        "virtual RhiResidencyActionResult execute_residency_action"
    )) {
    Assert-ContainsText $rhiHeaderText $needle "rhi.hpp residency action public contract"
}

foreach ($needle in @(
        "RhiResidencyActionResult NullRhiDevice::execute_residency_action",
        "valid_residency_action_kind",
        "owns_buffer(resource.buffer)",
        "owns_texture(resource.texture)",
        "RhiResidencyActionStatus::invalid_request",
        "RhiResidencyActionStatus::invalid_resource",
        "RhiResidencyActionStatus::succeeded",
        "result.acted_resource_count = result.requested_resource_count"
    )) {
    Assert-ContainsText $nullRhiText $needle "null_rhi.cpp deterministic residency action validation"
}

foreach ($needle in @(
        "RhiResidencyActionResult execute_residency_action",
        "std::span<const NativeResourceHandle> resources"
    )) {
    Assert-ContainsText $d3d12HeaderText $needle "d3d12_backend.hpp residency action backend-private declaration"
}

foreach ($needle in @(
        "RhiResidencyActionResult DeviceContext::execute_residency_action",
        "std::vector<Microsoft::WRL::ComPtr<ID3D12Pageable>> pageables",
        "std::vector<ID3D12Pageable*> pageable_pointers",
        "impl_->resource_is_placed(resource_handle)",
        "resource->QueryInterface(IID_PPV_ARGS(&pageable))",
        "impl_->device->MakeResident",
        "impl_->device->Evict",
        "result.native_error_code",
        "execute_residency_action(const RhiResidencyActionDesc& desc) override",
        "buffer_handles_.at(resource.buffer.value - 1U)",
        "texture_handles_.at(resource.texture.value - 1U)",
        "buffer_resident_",
        "texture_resident_",
        "observed_resources_resident",
        "d3d12 rhi residency evict requires completed resource-use fences",
        "d3d12 rhi command list references nonresident resources",
        "d3d12 rhi buffer is not resident",
        "d3d12 rhi texture is not resident"
    )) {
    Assert-ContainsText $d3d12SourceText $needle "d3d12_backend.cpp committed-resource residency action execution"
}

foreach ($needle in @(
        "execute_residency_action(const RhiResidencyActionDesc& desc) override",
        "IRhiDevice::execute_residency_action(desc)",
        "vulkan rhi residency action is unsupported until backend-local parity lands"
    )) {
    Assert-ContainsText $vulkanSourceText $needle "vulkan_backend.cpp residency action fail-closed behavior"
}

foreach ($needle in @(
        "null rhi residency action validates buffer and texture rows",
        "null rhi residency action rejects invalid resource rows before action",
        "RhiResidencyActionKind::make_resident",
        "RhiResidencyActionKind::evict",
        "RhiResidencyActionStatus::succeeded",
        "RhiResidencyActionStatus::invalid_resource",
        "!result.exposed_native_handles",
        "!result.enforced_allocator_budget"
    )) {
    Assert-ContainsText $rhiTestsText $needle "MK_rhi_tests residency action coverage"
}

foreach ($needle in @(
        "d3d12 rhi residency action executes make resident and evict for committed resources",
        "d3d12 rhi residency action rejects unknown resources before native calls",
        "d3d12 rhi residency action tracks evicted resources at use sites",
        "d3d12 rhi residency action rejects transient placed textures before native calls",
        "evicted.invoked_native_evict",
        "made_resident.invoked_native_make_resident",
        "rejected_recording",
        "rejected_submit",
        "!result.invoked_native_make_resident",
        "!result.invoked_native_evict",
        "!result.exposed_native_handles",
        "!result.enforced_allocator_budget"
    )) {
    Assert-ContainsText $d3d12TestsText $needle "MK_d3d12_rhi_tests residency action coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgResidencyPlanText; Label = "docs/superpowers/plans/2026-06-06-mavg-d3d12-residency-action-execution-v1.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgMasterPlanText; Label = "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md" }
    )) {
    foreach ($needle in @(
            "MAVG D3D12 Residency Action Execution v1",
            "mavg-d3d12-residency-action-execution-v1",
            "RhiResidencyActionDesc",
            "RhiResidencyActionResult",
            "IRhiDevice::execute_residency_action",
            "ID3D12Device::MakeResident",
            "ID3D12Device::Evict"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) D3D12 residency action evidence"
    }
    foreach ($needle in @(
            "DirectStorage",
            "allocator",
            "Vulkan/Metal",
            "Nanite"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) D3D12 residency action non-claims"
    }
}

foreach ($needle in @(
        "MAVG D3D12 Residency Action Execution v1",
        "RhiResidencyActionDesc",
        "RhiResidencyResourceRef",
        "RhiResidencyActionResult",
        "IRhiDevice::execute_residency_action",
        "ID3D12Device::MakeResident",
        "ID3D12Device::Evict",
        "invoked_native_make_resident",
        "invoked_native_evict",
        "native_error_code",
        "evicted-resource",
        "placed",
        "exposed_native_handles=false",
        "enforced_allocator_budget=false",
        "no DirectStorage file IO",
        "no command-list residency-set scheduling",
        "no Vulkan/Metal",
        "no Nanite compatibility/equivalence/superiority claim"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json D3D12 residency action evidence"
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json D3D12 residency action evidence"
}

$productionLoop = $manifest.aiOperableProductionLoop
if (@($productionLoop.recommendedNextPlan.retainedCompletedPlanPaths) -notcontains "docs/superpowers/plans/2026-06-06-mavg-d3d12-residency-action-execution-v1.md") {
    Write-Error "engine/agent/manifest.json retainedCompletedPlanPaths must retain MAVG D3D12 Residency Action Execution v1"
}

$recommendedPlanText = (([string]$productionLoop.recommendedNextPlan.retainedCompletedPlanEvidence), ([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
foreach ($needle in @(
        "MAVG D3D12 Residency Action Execution v1",
        "RhiResidencyActionDesc",
        "RhiResidencyActionResult",
        "IRhiDevice::execute_residency_action",
        "ID3D12Device::MakeResident",
        "ID3D12Device::Evict",
        "exposed_native_handles=false",
        "enforced_allocator_budget=false",
        "no full MAVG page-to-resource residency service",
        "unsupportedProductionGaps = []"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan D3D12 residency action evidence"
}

$rhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($rhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module" }
$rhiManifestText = ((@($rhiModule[0].publicHeaders) -join " "), (@($rhiModule[0].recentEvidence) -join " "), [string]$rhiModule[0].purpose) -join " "
foreach ($needle in @(
        "RhiResidencyActionDesc",
        "RhiResidencyActionResult",
        "IRhiDevice::execute_residency_action",
        "ID3D12Device::MakeResident",
        "ID3D12Device::Evict",
        "no DirectStorage file IO",
        "no Nanite compatibility/equivalence/superiority"
    )) {
    Assert-ContainsText $rhiManifestText $needle "engine/agent/manifest.json MK_rhi D3D12 residency action evidence"
}

$d3d12Module = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi_d3d12" })
if ($d3d12Module.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi_d3d12 module" }
$d3d12ManifestText = ((@($d3d12Module[0].publicHeaders) -join " "), (@($d3d12Module[0].recentEvidence) -join " "), [string]$d3d12Module[0].purpose) -join " "
foreach ($needle in @(
        "DeviceContext::execute_residency_action",
        "D3d12RhiDevice::execute_residency_action",
        "ID3D12Pageable",
        "ID3D12Device::MakeResident",
        "ID3D12Device::Evict",
        "no public native handles",
        "no allocator/GPU budget enforcement"
    )) {
    Assert-ContainsText $d3d12ManifestText $needle "engine/agent/manifest.json MK_rhi_d3d12 D3D12 residency action evidence"
}
