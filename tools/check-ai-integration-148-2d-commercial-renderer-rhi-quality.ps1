#requires -Version 7.0
#requires -PSEdition Core
# Chapter 148 for check-ai-integration.ps1 2D commercial renderer/RHI quality gate contract.

$headerText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/two_d_commercial_renderer_rhi_quality.hpp"
$sourceText = Get-AgentSurfaceText "engine/renderer/src/two_d_commercial_renderer_rhi_quality.cpp"
$testsText = Get-AgentSurfaceText "tests/unit/two_d_commercial_renderer_rhi_quality_tests.cpp"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$rendererCMakeText = Get-AgentSurfaceText "engine/renderer/CMakeLists.txt"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"

foreach ($needle in @(
        "TwoDCommercialRendererRhiQualityOfficialSourceKind",
        "TwoDCommercialRendererRhiQualityOfficialSourceRow",
        "TwoDCommercialRendererRhiQualityEvidenceRow",
        "TwoDCommercialRendererRhiQualityDesc",
        "TwoDCommercialRendererRhiQualityResult",
        "evaluate_2d_commercial_renderer_rhi_quality",
        "microsoft_direct3d12",
        "khronos_vulkan",
        "apple_metal",
        "repository_legal_policy",
        "selected_d3d12_renderer_rhi_quality_ready",
        "vulkan_strict_renderer_rhi_quality_ready",
        "metal_renderer_rhi_quality_ready",
        "broad_backend_parity_ready",
        "broad_renderer_rhi_quality_ready",
        "public_native_handles_exposed",
        "request_cross_backend_inference",
        "external_engine_compatibility_claim",
        "legal_approval_claim"
    )) {
    Assert-ContainsText $headerText $needle "2D commercial renderer/RHI quality public header"
}

foreach ($needle in @(
        "2026-07-01",
        "2d-commercial-renderer-rhi-quality",
        "missing_d3d12_evidence",
        "missing_sprite_batch_budget",
        "stale_official_source",
        "public_native_handle_access",
        "cross_backend_inference",
        "external_engine_claim",
        "legal_approval_claim",
        "broad_backend_claim",
        "result.vulkan_strict_renderer_rhi_quality_ready = false",
        "result.metal_renderer_rhi_quality_ready = false",
        "result.broad_backend_parity_ready = false",
        "result.broad_renderer_rhi_quality_ready = false"
    )) {
    Assert-ContainsText $sourceText $needle "2D commercial renderer/RHI quality source"
}

foreach ($needle in @(
        "Microsoft-D3D12-Direct3D12-Command-Fence-Barrier-Descriptor-2026-07-01",
        "Khronos-Vulkan-Synchronization2-Timestamp-Validation-2026-07-01",
        "Apple-Metal-Shading-Language-Resource-Binding-2026-07-01",
        "MIRAIKANAI-Legal-Clean-Room-Policy-2026-07-01",
        "selected D3D12 proof without broad backend claims",
        "missing D3D12 official patterns host gated",
        "requires sprite throughput budget evidence",
        "rejects native handle and cross backend inference rows",
        "rejects external engine and legal approval claims",
        "rejects stale official source rows"
    )) {
    Assert-ContainsText $testsText $needle "2D commercial renderer/RHI quality tests"
}

foreach ($needle in @(
        "MK_2d_renderer_rhi_quality_tests",
        "MK_two_d_commercial_renderer_rhi_quality_tests",
        "tests/unit/two_d_commercial_renderer_rhi_quality_tests.cpp"
    )) {
    Assert-ContainsText $rootCMakeText $needle "2D commercial renderer/RHI quality root CMake target"
}

Assert-ContainsText $rendererCMakeText "src/two_d_commercial_renderer_rhi_quality.cpp" "2D commercial renderer/RHI quality renderer CMake source"
Assert-ContainsText $modulesFragmentText "two_d_commercial_renderer_rhi_quality.hpp" "modules fragment 2D commercial renderer/RHI quality header"
Assert-ContainsText $manifestText "two_d_commercial_renderer_rhi_quality.hpp" "composed manifest 2D commercial renderer/RHI quality header"

foreach ($surface in @(
        @{ Text = $modulesFragmentText; Label = "modules fragment" },
        @{ Text = $manifestText; Label = "composed manifest" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planText; Label = "2D commercial production excellence plan" }
    )) {
    foreach ($needle in @(
            "2D Commercial Renderer/RHI Quality Gate v1",
            "evaluate_2d_commercial_renderer_rhi_quality",
            "selected D3D12",
            "Microsoft D3D12",
            "Khronos Vulkan",
            "Apple Metal",
            "repository legal policy",
            "command allocator/list/fence",
            "descriptor heap",
            "PSO reuse",
            "resource barrier",
            "debug validation",
            "timestamp/PIX",
            "package readback",
            "sprite throughput",
            "atlas residency/upload scheduling",
            "frame pacing",
            "claim-control",
            "clean-room",
            "public native handles",
            "cross-backend Vulkan/Metal inference",
            "Unity/Unreal Engine/Godot",
            "legal-approval claims",
            "broad renderer"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) 2D commercial renderer/RHI quality gate"
    }
}

foreach ($needle in @(
        "2D Commercial Renderer/RHI Quality Gate v1",
        "selected-D3D12",
        "Vulkan/Metal/broad renderer readiness remains unpromoted"
    )) {
    Assert-ContainsText $planRegistryText $needle "docs/superpowers/plans/README.md 2D commercial renderer/RHI quality gate index"
}

foreach ($forbiddenNeedle in @(
        "2D Commercial Renderer/RHI Quality Gate v1 closes Phase 6",
        "renderer_commercial_readiness=1",
        "broad renderer commercial readiness is ready",
        "Vulkan/Metal readiness is ready",
        "legal approval is complete"
    )) {
    Assert-DoesNotContainText $currentCapabilitiesText $forbiddenNeedle "docs/current-capabilities.md 2D renderer/RHI forbidden ready claim"
    Assert-DoesNotContainText $roadmapText $forbiddenNeedle "docs/roadmap.md 2D renderer/RHI forbidden ready claim"
    Assert-DoesNotContainText $planText $forbiddenNeedle "2D commercial production excellence forbidden ready claim"
}
