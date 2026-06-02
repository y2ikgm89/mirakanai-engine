// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class CpuSimdLane : std::uint8_t { scalar, sse2, avx2 };

enum class CpuSimdLaneRequest : std::uint8_t { auto_select, scalar, sse2, avx2 };

enum class SimdDispatchStatus : std::uint8_t { ready, invalid_input, unsupported };

enum class SimdDispatchDiagnosticCode : std::uint8_t {
    none,
    empty_input,
    input_size_mismatch,
    compile_lane_unavailable,
    runtime_lane_unavailable,
    reviewed_target_gate_missing,
    non_x86_host
};

struct CpuSimdFeatureSet {
    bool x86_or_x64_host{false};
    bool sse2_compile_supported{false};
    bool sse2_runtime_supported{false};
    bool avx2_compile_supported{false};
    bool avx2_runtime_supported{false};
};

struct SimdDispatchPolicyDesc {
    CpuSimdLaneRequest requested_lane{CpuSimdLaneRequest::auto_select};
    std::optional<CpuSimdFeatureSet> features;
};

struct SimdDispatchPolicy {
    SimdDispatchStatus status{SimdDispatchStatus::unsupported};
    CpuSimdLaneRequest requested_lane{CpuSimdLaneRequest::auto_select};
    CpuSimdLane selected_lane{CpuSimdLane::scalar};
    CpuSimdFeatureSet features;
    bool scalar_fallback{true};
    bool sse2_selected{false};
    bool avx2_selected{false};
    bool gpu_async_overlap_applied{false};
    bool cuda_path_used{false};
    bool hip_path_used{false};
    bool sycl_path_used{false};
    std::vector<SimdDispatchDiagnosticCode> diagnostic_codes;
    std::vector<std::string> diagnostics;

    [[nodiscard]] bool ready() const noexcept {
        return status == SimdDispatchStatus::ready;
    }
};

struct SimdDotProductEvidence {
    SimdDispatchPolicy policy;
    std::uint64_t input_count{0};
    float result{0.0F};
    bool span_inputs_used{true};
    bool raw_pointers_retained{false};
    std::vector<std::string> diagnostics;
};

[[nodiscard]] CpuSimdFeatureSet observe_cpu_simd_features() noexcept;
[[nodiscard]] SimdDispatchPolicy select_simd_dispatch_policy(const SimdDispatchPolicyDesc& desc);
[[nodiscard]] float compute_scalar_dot_product(std::span<const float> lhs, std::span<const float> rhs) noexcept;
[[nodiscard]] SimdDotProductEvidence build_simd_dot_product_evidence(std::span<const float> lhs,
                                                                     std::span<const float> rhs,
                                                                     const SimdDispatchPolicyDesc& desc = {});

[[nodiscard]] std::string_view cpu_simd_lane_label(CpuSimdLane lane) noexcept;
[[nodiscard]] std::string_view cpu_simd_lane_request_label(CpuSimdLaneRequest lane) noexcept;
[[nodiscard]] std::string_view simd_dispatch_status_label(SimdDispatchStatus status) noexcept;
[[nodiscard]] std::string_view simd_dispatch_diagnostic_code_label(SimdDispatchDiagnosticCode code) noexcept;

} // namespace mirakana
