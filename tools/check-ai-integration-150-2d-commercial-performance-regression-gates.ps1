#requires -Version 7.0
#requires -PSEdition Core

# Chapter 150 for check-ai-integration.ps1 static contracts.
# Runtime 2D Commercial Performance Regression Gates v1 package/manifest/plan alignment.

$performanceHeader = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/two_d_commercial_performance_regression_gate.hpp"
$performanceSource = Get-AgentSurfaceText "engine/runtime/src/two_d_commercial_performance_regression_gate.cpp"
$performanceTests = Get-AgentSurfaceText "tests/unit/runtime_2d_commercial_performance_regression_gate_tests.cpp"
$rootCMake = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeCMake = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$sample2dMain = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample2dManifest = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$validatorScript = Get-AgentSurfaceText "tools/validate-2d-commercial-performance-regression-gates.ps1"
$validationRecipesManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$gameCodeGuidanceManifest = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$composedManifest = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilities = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmap = Get-AgentSurfaceText "docs/roadmap.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md"

foreach ($needle in @(
        "Runtime2DCommercialPerformanceWorkloadKind",
        "Runtime2DCommercialPerformanceMetricKind",
        "Runtime2DCommercialPerformanceOfficialSourceKind",
        "Runtime2DCommercialPerformanceWorkloadRow",
        "Runtime2DCommercialPerformanceMetricRow",
        "Runtime2DCommercialPerformanceOfficialSourceRow",
        "Runtime2DCommercialPerformanceRegressionGateDesc",
        "Runtime2DCommercialPerformanceRegressionGateResult",
        "evaluate_runtime_2d_commercial_performance_regression_gate",
        "microsoft_d3d12",
        "microsoft_wpt",
        "microsoft_pix",
        "khronos_vulkan_timestamp_debug_utils",
        "apple_xctrace_metal_capture",
        "linux_perf",
        "legal_approval_claim"
    )) {
    Assert-ContainsText $performanceHeader $needle "two_d_commercial_performance_regression_gate.hpp"
}

foreach ($needle in @(
        "kRequiredWorkloadKinds",
        "kRequiredMetricKinds",
        "kRequiredOfficialSourceKinds",
        "host_threshold_not_ready",
        "over_budget",
        "broad_optimization_claim",
        "cross_vendor_parity_claim",
        "cross_backend_parity_claim",
        "external_engine_compatibility_claim",
        "legal_approval_claim"
    )) {
    Assert-ContainsText $performanceSource $needle "two_d_commercial_performance_regression_gate.cpp"
}

foreach ($needle in @(
        "runtime 2d commercial performance regression gate accepts selected package evidence",
        "runtime 2d commercial performance regression gate rejects missing workload and source rows",
        "runtime 2d commercial performance regression gate rejects over budget and broad claims",
        "host_gated_profiler_artifact_rows == 5U",
        "official_source_rows == 7U"
    )) {
    Assert-ContainsText $performanceTests $needle "runtime_2d_commercial_performance_regression_gate_tests.cpp"
}

foreach ($needle in @(
        "src/two_d_commercial_performance_regression_gate.cpp",
        "tests/unit/runtime_2d_commercial_performance_regression_gate_tests.cpp",
        "MK_runtime_2d_commercial_performance_regression_gate_tests"
    )) {
    Assert-ContainsText ($runtimeCMake + "`n" + $rootCMake) $needle "2d commercial performance regression CMake registration"
}

foreach ($needle in @(
        "mirakana/runtime/two_d_commercial_performance_regression_gate.hpp",
        "--require-2d-commercial-performance-regression-gate",
        "make_2d_commercial_performance_workload_rows",
        "make_2d_commercial_performance_metric_rows",
        "make_2d_commercial_performance_official_source_rows",
        "validate_2d_commercial_performance_regression_package_evidence",
        "2d_commercial_performance_regression_ready=",
        "2d_commercial_performance_workload_rows=",
        "2d_commercial_performance_metric_rows=",
        "2d_commercial_performance_official_source_rows=",
        "2d_commercial_performance_over_budget_rows=",
        "2d_commercial_performance_broad_optimization_claim_rows=",
        "2d_commercial_performance_cross_vendor_parity_claim_rows=",
        "2d_commercial_performance_cross_backend_parity_claim_rows=",
        "2d_commercial_performance_external_engine_claim_rows=",
        "2d_commercial_performance_legal_approval_claim_rows=",
        "required_2d_commercial_performance_regression_gate_unavailable"
    )) {
    Assert-ContainsText $sample2dMain $needle "sample_2d_desktop_runtime_package/main.cpp"
}

foreach ($needle in @(
        "installed-2d-commercial-performance-regression-smoke",
        "performance-regression-gate-2d",
        "commercialPerformanceRegression2D",
        "evaluate_runtime_2d_commercial_performance_regression_gate",
        "2d_commercial_performance_regression_ready=1",
        "2d_commercial_performance_workload_rows=8",
        "2d_commercial_performance_metric_rows=11",
        "2d_commercial_performance_official_source_rows=7",
        "2d_commercial_performance_cpu_frame_p50_us=15800",
        "2d_commercial_performance_cpu_frame_p95_us=16000",
        "2d_commercial_performance_cpu_frame_p99_us=16000",
        "2d_commercial_performance_over_budget_rows=0",
        "external commercial engine compatibility",
        "legal approval"
    )) {
    Assert-ContainsText $sample2dManifest $needle "sample_2d_desktop_runtime_package/game.agent.json"
}
if ($sample2dManifest.Contains("performanceRegressionGate2D")) {
    Write-Error "sample_2d_desktop_runtime_package/game.agent.json must use commercialPerformanceRegression2D without compatibility aliases"
}

foreach ($needle in @(
        "2d_commercial_performance_regression_status",
        "2d_commercial_performance_regression_ready",
        "2d_commercial_performance_workload_rows",
        "2d_commercial_performance_metric_rows",
        "2d_commercial_performance_official_source_rows",
        "2d_commercial_performance_cpu_frame_p50_us",
        "2d_commercial_performance_cpu_frame_p95_us",
        "2d_commercial_performance_cpu_frame_p99_us",
        "2d_commercial_performance_over_budget_rows",
        "package-desktop-runtime.ps1",
        "installed-2d-commercial-performance-regression-smoke",
        "performance-regression-gate-2d"
    )) {
    Assert-ContainsText $validatorScript $needle "tools/validate-2d-commercial-performance-regression-gates.ps1"
}

foreach ($needle in @(
        "desktop-runtime-2d-commercial-performance-regression-gates",
        "tools/validate-2d-commercial-performance-regression-gates.ps1 -RequireReady",
        "Runtime2DCommercialPerformanceRegressionGateDesc",
        "zero broad optimization",
        "zero legal-approval claims"
    )) {
    Assert-ContainsText $validationRecipesManifest $needle "engine/agent/manifest.fragments/009-validationRecipes.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json validation recipes"
}

foreach ($needle in @(
        "twoDCommercialPerformanceRegressionPhase8Evidence",
        "2d-commercial-performance-regression-gates-v1",
        "Runtime2DCommercialPerformanceWorkloadKind",
        "Microsoft Direct3D 12, Windows Performance Toolkit/WPA, PIX on Windows",
        "zero PGO/LTO default-lane"
    )) {
    Assert-ContainsText $productionLoopManifest $needle "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json production loop"
}

foreach ($needle in @(
        "current2DCommercialPerformanceRegressionPhase8",
        "evaluate_runtime_2d_commercial_performance_regression_gate",
        "--require-2d-commercial-performance-regression-gate",
        "public Microsoft Direct3D 12, WPT/WPA, PIX on Windows",
        "real retained profiler capture artifacts"
    )) {
    Assert-ContainsText $gameCodeGuidanceManifest $needle "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
    Assert-ContainsText $composedManifest $needle "engine/agent/manifest.json game code guidance"
}

foreach ($needle in @(
        "2D Commercial Performance Regression Gates v1",
        "tools/validate-2d-commercial-performance-regression-gates.ps1",
        "https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-graphics",
        "https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/general/pix-overview",
        "https://learn.microsoft.com/en-us/windows-hardware/test/wpt/",
        "https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html",
        "https://developer.apple.com/metal/tools/",
        "https://perf.wiki.kernel.org/index.php/Main_Page",
        "zero over-budget",
        "Unity/Unreal/Godot compatibility"
    )) {
    Assert-ContainsText ($currentCapabilities + "`n" + $roadmap + "`n" + $planText) $needle "2d commercial performance regression docs and plan"
}
