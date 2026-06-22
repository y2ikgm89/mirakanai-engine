#requires -Version 7.0
#requires -PSEdition Core
# Chapter 130 for check-ai-integration.ps1 MAVG broad CPU/GPU/memory optimization evidence gate.

$headerText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_broad_optimization_evidence.hpp"
$sourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_broad_optimization_evidence.cpp"
$testsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_broad_optimization_evidence_tests.cpp"
$schemaText = Get-AgentSurfaceText "schemas/mavg-broad-optimization-artifacts.schema.json"
$validatorText = Get-AgentSurfaceText "tools/validate-mavg-broad-optimization.ps1"
$runtimeRhiCMakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$commandsFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgAdvancedPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md"

foreach ($needle in @(
        "RuntimeMavgBroadOptimizationEvidenceDesc",
        "RuntimeMavgBroadOptimizationEvidenceRow",
        "RuntimeMavgBroadOptimizationMetrics",
        "RuntimeMavgBroadOptimizationEvidenceResult",
        "RuntimeMavgBroadOptimizationDiagnosticCode",
        "evaluate_runtime_mavg_broad_optimization_evidence",
        "has_runtime_mavg_broad_optimization_diagnostic",
        "runtime_mavg_broad_optimization_status_label",
        "profiler_tool_support_source_id",
        "profiler_capture_overhead_basis_points",
        "profiler_tool_eol_or_unsupported",
        "profiler_tool_backend_vendor_mismatch",
        "bool mavg_broad_cpu_gpu_memory_optimization_ready{false};",
        "bool mavg_package_visible_backend_readiness_ready{false};",
        "bool mavg_nanite_compatible{false};",
        "bool mavg_nanite_equivalent{false};",
        "bool mavg_nanite_superior{false};"
    )) {
    Assert-ContainsText $headerText $needle "mavg_broad_optimization_evidence.hpp public contract"
}

foreach ($needle in @(
        "constexpr std::array<RequiredClass, 14>",
        "RuntimeMavgBroadOptimizationProfilerTool::intel_gpa",
        "is_eol_or_unsupported_profiler_tool",
        "is_profiler_tool_allowed_for_backend_vendor",
        "RuntimeMavgBroadOptimizationProfilerTool::pix_timing_capture",
        "RuntimeMavgBroadOptimizationProfilerTool::nsight_graphics_gpu_trace",
        "RuntimeMavgBroadOptimizationProfilerTool::radeon_gpu_profiler",
        "RuntimeMavgBroadOptimizationProfilerTool::apple_metal_tools",
        "MAVG broad optimization cannot substitute unsupported or end-of-life profiler evidence",
        "MAVG broad optimization profiler evidence must match the backend",
        "official tool support row",
        "result.mavg_broad_cpu_gpu_memory_optimization_ready = true"
    )) {
    Assert-ContainsText $sourceText $needle "mavg_broad_optimization_evidence.cpp fail-closed implementation"
}

foreach ($needle in @(
        "runtime rhi mavg broad optimization evidence host gates complete matrix with eol intel vulkan profiler",
        "runtime rhi mavg broad optimization evidence rejects profiler backend vendor mismatch",
        "runtime rhi mavg broad optimization evidence records profiler support source and overhead",
        "runtime rhi mavg broad optimization evidence blocks synthetic native and broad claims",
        "RuntimeMavgBroadOptimizationProfilerTool::intel_gpa",
        "profiler_tool_backend_vendor_mismatch",
        "mavg_nanite_superior"
    )) {
    Assert-ContainsText $testsText $needle "MK_runtime_rhi_mavg_broad_optimization_evidence_tests coverage"
}

foreach ($needle in @(
        "GameEngine.MavgBroadOptimizationArtifacts.v1",
        "mavg-broad-optimization-evidence-v1",
        "profiler_tool_support_source_id",
        "profiler_capture_overhead_basis_points",
        "pix_timing_capture",
        "nsight_graphics_gpu_trace",
        "radeon_gpu_profiler",
        "intel_gpa",
        "apple_metal_tools",
        "claims_broad_backend_readiness"
    )) {
    Assert-ContainsText $schemaText $needle "mavg-broad-optimization-artifacts schema"
}

foreach ($needle in @(
        "validation_recipe=mavg-broad-optimization",
        "MK_runtime_rhi_mavg_broad_optimization_evidence_tests",
        'mavg_broad_cpu_gpu_memory_optimization_status=$status',
        'mavg_broad_cpu_gpu_memory_optimization_ready=$(ConvertTo-CounterBit $ready)',
        'mavg_broad_optimization_required_rows=$requiredRows',
        "mavg_broad_optimization_eol_or_unsupported_profiler_rows=0",
        "mavg_broad_optimization_profiler_backend_vendor_mismatch_rows=0",
        "mavg_broad_optimization_intel_gpa_eol=1",
        "mavg_broad_optimization_intel_vulkan_supported_profiler_rows=0",
        "mavg_broad_optimization_package_visible_backend_readiness=0",
        "mavg_broad_optimization_environment_ready=0",
        "mavg_broad_optimization_renderer_ready=0",
        "mavg_broad_optimization_nanite_superior=0",
        "MAVG broad CPU/GPU/memory optimization is incomplete"
    )) {
    Assert-ContainsText $validatorText $needle "tools/validate-mavg-broad-optimization.ps1"
}

Assert-ContainsText $runtimeRhiCMakeText "src/mavg_broad_optimization_evidence.cpp" "engine/runtime_rhi/CMakeLists.txt MAVG broad optimization source registration"
Assert-ContainsText $rootCMakeText "MK_runtime_rhi_mavg_broad_optimization_evidence_tests" "root CMake MAVG broad optimization test target"
Assert-ContainsText $commandsFragmentText "mavgBroadOptimizationCheck" "manifest commands MAVG broad optimization command"

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgAdvancedPlanText; Label = "MAVG advanced evidence plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" }
    )) {
    foreach ($needle in @(
            "mavg-broad-optimization-evidence-v1",
            "mavg_broad_optimization_evidence.hpp",
            "RuntimeMavgBroadOptimizationEvidenceDesc",
            "RuntimeMavgBroadOptimizationEvidenceRow",
            "RuntimeMavgBroadOptimizationMetrics",
            "evaluate_runtime_mavg_broad_optimization_evidence",
            "GameEngine.MavgBroadOptimizationArtifacts.v1",
            "tools/validate-mavg-broad-optimization.ps1",
            "mavg_broad_cpu_gpu_memory_optimization_status=host_evidence_required",
            "mavg_broad_cpu_gpu_memory_optimization_ready=0",
            "mavg_broad_optimization_required_rows=14",
            "mavg_broad_optimization_intel_gpa_eol=1",
            "mavg_broad_optimization_intel_vulkan_supported_profiler_rows=0",
            "official-profiler",
            "capture",
            "Nanite",
            "environment",
            "renderer"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG broad optimization host gate"
    }
    foreach ($forbiddenNeedle in @(
            "mavg_broad_cpu_gpu_memory_optimization_ready=1",
            "mavg_broad_cpu_gpu_memory_optimization_status=ready",
            "mavg_broad_optimization_package_visible_backend_readiness=1",
            "mavg_broad_optimization_environment_ready=1",
            "mavg_broad_optimization_renderer_ready=1",
            "mavg_broad_optimization_nanite_compatible=1",
            "mavg_broad_optimization_nanite_equivalent=1",
            "mavg_broad_optimization_nanite_superior=1"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden MAVG broad optimization ready claim"
    }
}

foreach ($needle in @(
        "PIX",
        "Nsight",
        "RGP",
        "Intel GPA",
        "Apple Metal tools",
        "EOL",
        "Vulkan Intel"
    )) {
    Assert-ContainsText $mavgAdvancedPlanText $needle "MAVG advanced plan official profiler policy"
}

$manifest = $manifestText | ConvertFrom-Json
$runtimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($runtimeRhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module" }
$runtimeRhiManifestText = ((@($runtimeRhiModule[0].publicHeaders) -join " "),
    (@($runtimeRhiModule[0].recentEvidence) -join " "),
    [string]$runtimeRhiModule[0].purpose) -join " "
foreach ($needle in @(
        "mavg_broad_optimization_evidence.hpp",
        "RuntimeMavgBroadOptimizationEvidenceDesc",
        "mavg_broad_cpu_gpu_memory_optimization_ready=0",
        "mavg_broad_optimization_intel_gpa_eol=1",
        "mavg_broad_optimization_intel_vulkan_supported_profiler_rows=0"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi MAVG broad optimization evidence"
}
