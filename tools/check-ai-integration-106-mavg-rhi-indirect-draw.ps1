#requires -Version 7.0
#requires -PSEdition Core
# Chapter 106 for check-ai-integration.ps1 static contracts.

$rhiHeaderText = Get-AgentSurfaceText "engine/rhi/include/mirakana/rhi/rhi.hpp"
$indirectDrawHeaderText = Get-AgentSurfaceText "engine/rhi/include/mirakana/rhi/indirect_draw.hpp"
$indirectDrawSourceText = Get-AgentSurfaceText "engine/rhi/src/indirect_draw.cpp"
$nullRhiText = Get-AgentSurfaceText "engine/rhi/src/null_rhi.cpp"
$vulkanBackendHeaderText = Get-AgentSurfaceText "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp"
$vulkanBackendSourceText = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_backend.cpp"
$rhiTestsText = Get-AgentSurfaceText "tests/unit/rhi_tests.cpp"
$mavgArchitectureSpecText = Get-AgentSurfaceText "docs/specs/2026-06-05-mavg-architecture-v1.md"
$mavgRhiPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-rhi-indirect-draw-v1.md"
$mavgRuntimeLodPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"

foreach ($needle in @(
        "IndexedIndirectDrawCommand",
        "IndexedIndirectDrawDesc",
        "indexed_indirect_draw_command_word_count",
        "indexed_indirect_draw_command_size_bytes",
        "indexed_indirect_draw_command_stride_bytes",
        "indexed_indirect_draw_offset_alignment_bytes",
        "indexed_indirect_draw_count_buffer_size_bytes",
        "encode_indexed_indirect_draw_command",
        "decode_indexed_indirect_draw_command",
        "has_indexed_indirect_count_buffer"
    )) {
    Assert-ContainsText $indirectDrawHeaderText $needle "engine/rhi/include/mirakana/rhi/indirect_draw.hpp public contract"
}

foreach ($needle in @(
        "std::bit_cast",
        "write_u32",
        "read_u32",
        "rhi indexed indirect draw is unsupported by this command list",
        "draw_indexed_indirect"
    )) {
    Assert-ContainsText $indirectDrawSourceText $needle "engine/rhi/src/indirect_draw.cpp deterministic encoding/default path"
}

foreach ($needle in @(
        "indirect = 1U << 6U",
        "indexed_indirect_draw_calls",
        "indexed_indirect_commands_executed",
        "indexed_indirect_count_buffer_reads",
        "last_indexed_indirect_max_draw_count",
        "last_indexed_indirect_executed_draw_count",
        "last_indexed_indirect_count_buffer_value",
        "draw_indexed_indirect"
    )) {
    Assert-ContainsText $rhiHeaderText $needle "engine/rhi/include/mirakana/rhi/rhi.hpp indirect draw API"
}

foreach ($needle in @(
        "indexed_indirect_argument_range_end",
        "read_u32_le",
        "BufferUsage::indirect",
        "has_indexed_indirect_count_buffer",
        "count_buffer_value",
        "indexed_indirect_count_buffer_reads",
        "indexed_indirect_commands_executed",
        "draw_indexed_indirect"
    )) {
    Assert-ContainsText $nullRhiText $needle "engine/rhi/src/null_rhi.cpp Null RHI indirect draw execution"
}

foreach ($needle in @(
        "indirect",
        "VulkanBufferUsagePlan"
    )) {
    Assert-ContainsText $vulkanBackendHeaderText $needle "engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp indirect usage"
}

foreach ($needle in @(
        "vulkan_buffer_usage_indirect_buffer_bit",
        "BufferUsage::indirect",
        "usage.indirect",
        "plan.usage.indirect"
    )) {
    Assert-ContainsText $vulkanBackendSourceText $needle "engine/rhi/vulkan/src/vulkan_backend.cpp indirect usage mapping"
}

foreach ($needle in @(
        "null rhi executes indexed indirect draw from argument and count buffers",
        "null rhi rejects indexed indirect draw buffers with invalid usage or range",
        "IndexedIndirectDrawDesc",
        "BufferUsage::indirect",
        "draw_indexed_indirect"
    )) {
    Assert-ContainsText $rhiTestsText $needle "tests/unit/rhi_tests.cpp RHI indirect draw coverage"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgArchitectureSpecText; Label = "docs/specs/2026-06-05-mavg-architecture-v1.md" },
        @{ Text = $mavgRhiPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-rhi-indirect-draw-v1.md" },
        @{ Text = $mavgRuntimeLodPlanText; Label = "docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md" }
    )) {
    foreach ($needle in @("mavg-rhi-indirect-draw-v1", "indirect_draw.hpp", "IndexedIndirectDrawDesc", "BufferUsage::indirect", "Null RHI")) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG RHI indirect draw evidence"
    }
    foreach ($needle in @("D3D12", "ExecuteIndirect", "Vulkan indirect draw execution", "Nanite")) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG RHI indirect draw non-claim evidence"
    }
}

foreach ($needle in @(
        "mavg-rhi-indirect-draw-v1",
        "docs/superpowers/plans/2026-06-05-mavg-rhi-indirect-draw-v1.md",
        "IndexedIndirectDrawCommand",
        "IndexedIndirectDrawDesc",
        "BufferUsage::indirect",
        "D3D12 ExecuteIndirect",
        "Vulkan indirect draw execution"
    )) {
    Assert-ContainsText $aiLoopFragmentText $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
}

foreach ($needle in @(
        "engine/rhi/include/mirakana/rhi/indirect_draw.hpp",
        "MAVG RHI Indirect Draw v1",
        "IndexedIndirectDrawDesc",
        "BufferUsage::indirect",
        "Null RHI",
        "D3D12 ExecuteIndirect",
        "Vulkan indirect draw execution",
        "Nanite equivalence/superiority"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json MK_rhi evidence"
}

$rhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($rhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module" }
if (@($rhiModule[0].publicHeaders) -notcontains "engine/rhi/include/mirakana/rhi/indirect_draw.hpp") {
    Write-Error "engine/agent/manifest.json MK_rhi publicHeaders missing indirect_draw.hpp"
}
$rhiManifestText = ((@($rhiModule[0].recentEvidence) -join " "), [string]$rhiModule[0].purpose) -join " "
foreach ($needle in @(
        "MAVG RHI Indirect Draw v1",
        "IndexedIndirectDrawCommand",
        "IndexedIndirectDrawDesc",
        "BufferUsage::indirect",
        "IRhiCommandList::draw_indexed_indirect",
        "Null RHI",
        "VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT",
        "D3D12 ExecuteIndirect",
        "Vulkan indirect draw execution",
        "Nanite equivalence/superiority"
    )) {
    Assert-ContainsText $rhiManifestText $needle "engine/agent/manifest.json MK_rhi MAVG RHI indirect draw evidence"
}

if ($manifest.aiOperableProductionLoop.currentActivePlan -ne "docs/superpowers/plans/2026-06-05-mavg-rhi-indirect-draw-v1.md") {
    Write-Error "engine/agent/manifest.json currentActivePlan must point at mavg-rhi-indirect-draw-v1"
}
if ($manifest.aiOperableProductionLoop.recommendedNextPlan.id -ne "mavg-rhi-indirect-draw-v1") {
    Write-Error "engine/agent/manifest.json recommendedNextPlan.id must be mavg-rhi-indirect-draw-v1"
}
