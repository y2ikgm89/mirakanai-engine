#requires -Version 7.0
#requires -PSEdition Core
# Chapter 122 for check-ai-integration.ps1 MAVG selected async-overlap performance proof.

$mavgProofHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_async_overlap_performance_proof.hpp"
$mavgProofSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_async_overlap_performance_proof.cpp"
$mavgProofTestsText = Get-AgentSurfaceText "tests/unit/runtime_rhi_mavg_async_overlap_performance_proof_tests.cpp"
$mavgOverlapHeaderText = Get-AgentSurfaceText "engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_streaming_upload_overlap_evidence.hpp"
$mavgOverlapSourceText = Get-AgentSurfaceText "engine/runtime_rhi/src/mavg_streaming_upload_overlap_evidence.cpp"
$runtimeRhiCMakeText = Get-AgentSurfaceText "engine/runtime_rhi/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$sampleDesktopRuntimeGameText = Get-AgentSurfaceText "games/sample_desktop_runtime_game/main.cpp"
$validatorText = Get-AgentSurfaceText "tools/validate-mavg-async-overlap-performance-proof.ps1"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$productionMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
$mavgMasterPlanText = Get-AgentSurfaceText "docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md"
$mavgProofPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-mavg-async-overlap-performance-proof-v1.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$aiLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "RuntimeMavgAsyncOverlapPerformanceProofDesc",
        "RuntimeMavgAsyncOverlapPerformanceSample",
        "RuntimeMavgAsyncOverlapPerformanceProofResult",
        "RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode",
        "std::span<const RuntimeMavgAsyncOverlapPerformanceSample>",
        "prove_runtime_mavg_async_overlap_performance",
        "has_runtime_mavg_async_overlap_performance_proof_diagnostic",
        "bool touched_native_handles{false};",
        "bool used_gpu_directstorage_destination{false};",
        "bool used_gdeflate{false};",
        "bool executed_mesh_shader{false};",
        "bool claimed_metal_readiness{false};",
        "bool claimed_nanite_equivalence{false};",
        "bool claimed_broad_optimization{false};"
    )) {
    Assert-ContainsText $mavgProofHeaderText $needle "mavg_async_overlap_performance_proof.hpp public contract"
}

foreach ($needle in @(
        "RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::overlap_evidence_not_ready",
        "RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::sample_workload_mismatch",
        "RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::sample_artifact_mismatch",
        "RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::sample_row_count_mismatch",
        "RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::invalid_sample_ticks",
        "RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::sample_missing_overlap",
        "RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::insufficient_speedup",
        "result.claimed_speedup = true;",
        "result.proved_async_overlap_performance = true;",
        "if (!result.diagnostics.empty())"
    )) {
    Assert-ContainsText $mavgProofSourceText $needle "mavg_async_overlap_performance_proof.cpp fail-closed implementation"
}

foreach ($needle in @(
        "AssetId graph_asset;",
        "std::uint64_t timeline_id{0};"
    )) {
    Assert-ContainsText $mavgOverlapHeaderText $needle "mavg_streaming_upload_overlap_evidence.hpp retained proof identity"
}
foreach ($needle in @(
        "result.graph_asset = desc.graph_asset;",
        "result.timeline_id = desc.background_load_window.timeline_id;"
    )) {
    Assert-ContainsText $mavgOverlapSourceText $needle "mavg_streaming_upload_overlap_evidence.cpp retained proof identity"
}

foreach ($needle in @(
        "runtime rhi mavg async overlap performance proof promotes selected measured samples only",
        "runtime rhi mavg async overlap performance proof fails closed without ready overlap evidence",
        "runtime rhi mavg async overlap performance proof rejects invalid sample evidence",
        "runtime rhi mavg async overlap performance proof rejects insufficient speedup",
        "serial_p95_tick_count == 140U",
        "overlapped_p95_tick_count == 105U",
        "speedup_basis_points == 2500U",
        "!result.claimed_speedup"
    )) {
    Assert-ContainsText $mavgProofTestsText $needle "MK_runtime_rhi_mavg_async_overlap_performance_proof_tests coverage"
}

Assert-ContainsText $runtimeRhiCMakeText "src/mavg_async_overlap_performance_proof.cpp" "engine/runtime_rhi/CMakeLists.txt proof source registration"
Assert-ContainsText $rootCMakeText "MK_runtime_rhi_mavg_async_overlap_performance_proof_tests" "root CMake proof test target"

foreach ($needle in @(
        "--require-mavg-async-overlap-performance-proof",
        "mavg_async_overlap_performance_proof_status=",
        "mavg_async_overlap_performance_proof_ready=",
        "mavg_async_overlap_performance_proof_samples=",
        "mavg_async_overlap_performance_proof_overlapped_samples=",
        "mavg_async_overlap_performance_proof_serial_p95_ticks=",
        "mavg_async_overlap_performance_proof_overlapped_p95_ticks=",
        "mavg_async_overlap_performance_proof_speedup_basis_points=",
        "mavg_async_overlap_performance_proof_native_handles_exposed=",
        "mavg_async_overlap_performance_proof_gpu_directstorage_destinations=",
        "mavg_async_overlap_performance_proof_gdeflate=",
        "mavg_async_overlap_performance_proof_mesh_shader_execution=",
        "mavg_async_overlap_performance_proof_metal_readiness=",
        "mavg_async_overlap_performance_proof_nanite_equivalence=",
        "mavg_async_overlap_performance_proof_broad_optimization="
    )) {
    Assert-ContainsText $sampleDesktopRuntimeGameText $needle "sample_desktop_runtime_game proof package counters"
}

foreach ($needle in @(
        "validation_recipe=mavg-async-overlap-performance-proof",
        "MK_runtime_rhi_mavg_async_overlap_performance_proof_tests|MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests",
        "sample_desktop_runtime_game",
        "--require-mavg-async-overlap-performance-proof",
        '$proofReadyCounter -eq "1"',
        '$sampleCount -eq "10"',
        '$overlappedSampleCount -eq "5"',
        '$serialP95Ticks -eq "140"',
        '$overlappedP95Ticks -eq "105"',
        '$speedupBasisPoints -eq "2500"',
        '$broadOptimization -eq "0"',
        'mavg_async_overlap_performance_proof_ready=$(ConvertTo-CounterBit $ready)',
        'mavg_async_overlap_performance_proof_samples=$sampleCount',
        'mavg_async_overlap_performance_proof_broad_optimization=$broadOptimization'
    )) {
    Assert-ContainsText $validatorText $needle "tools/validate-mavg-async-overlap-performance-proof.ps1"
}

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $productionMasterPlanText; Label = "production completion master plan" },
        @{ Text = $mavgMasterPlanText; Label = "MAVG master plan" },
        @{ Text = $mavgProofPlanText; Label = "MAVG async overlap performance proof plan" },
        @{ Text = $modulesFragmentText; Label = "engine/agent/manifest.fragments/004-modules.json" },
        @{ Text = $aiLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" }
    )) {
    foreach ($needle in @(
            "mavg-async-overlap-performance-proof-v1",
            "selected measured-sample async-overlap performance proof",
            "RuntimeMavgAsyncOverlapPerformanceProofResult",
            "mavg_async_overlap_performance_proof_ready=1",
            "speedup 2500 basis points"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) proof claim"
    }
    foreach ($needle in @(
            "GPU DirectStorage destinations",
            "GDeflate",
            "mesh shaders",
            "Metal readiness",
            "Nanite",
            "native handles",
            "broad optimization"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) proof non-claims"
    }
}

$recommendedPlanText = $manifest.aiOperableProductionLoop.recommendedNextPlan | ConvertTo-Json -Depth 8
foreach ($needle in @(
        "mavg-async-overlap-performance-proof-v1",
        "RuntimeMavgAsyncOverlapPerformanceProofResult",
        "mavg_async_overlap_performance_proof_ready=1",
        "serial p95 140",
        "overlapped p95 105",
        "speedup 2500 basis points",
        "broad MAVG backend readiness"
    )) {
    Assert-ContainsText $recommendedPlanText $needle "engine/agent/manifest.json recommendedNextPlan proof evidence"
}

$runtimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($runtimeRhiModule.Count -ne 1) { Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module" }
$runtimeRhiManifestText = ((@($runtimeRhiModule[0].publicHeaders) -join " "),
    (@($runtimeRhiModule[0].recentEvidence) -join " "),
    [string]$runtimeRhiModule[0].purpose) -join " "
foreach ($needle in @(
        "mavg_async_overlap_performance_proof.hpp",
        "RuntimeMavgAsyncOverlapPerformanceProofDesc",
        "RuntimeMavgAsyncOverlapPerformanceProofResult",
        "prove_runtime_mavg_async_overlap_performance",
        "mavg_async_overlap_performance_proof_ready=1",
        "broad MAVG backend readiness"
    )) {
    Assert-ContainsText $runtimeRhiManifestText $needle "engine/agent/manifest.json MK_runtime_rhi proof evidence"
}
