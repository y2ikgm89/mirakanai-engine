// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/simd_dispatch.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <utility>

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#define MK_CPU_SIMD_X86_OR_X64 1
#else
#define MK_CPU_SIMD_X86_OR_X64 0
#endif

#if defined(_M_X64) || defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#define MK_CPU_SIMD_COMPILE_SSE2 1
#else
#define MK_CPU_SIMD_COMPILE_SSE2 0
#endif

#if defined(__AVX2__) || (defined(_M_AVX) && _M_AVX >= 2)
#define MK_CPU_SIMD_COMPILE_AVX2 1
#else
#define MK_CPU_SIMD_COMPILE_AVX2 0
#endif

#if MK_CPU_SIMD_X86_OR_X64
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#if MK_CPU_SIMD_COMPILE_AVX2
#include <x86intrin.h>
#endif
#endif
#endif

#if MK_CPU_SIMD_COMPILE_SSE2
#include <xmmintrin.h>
#endif

namespace mirakana {
namespace {

constexpr std::uint32_t kSse2EdxBit = 1U << 26U;
constexpr std::uint32_t kAvxEcxBit = 1U << 28U;
constexpr std::uint32_t kOsxsaveEcxBit = 1U << 27U;
constexpr std::uint32_t kAvx2EbxBit = 1U << 5U;
constexpr bool kReviewedAvx2DispatchLaneEnabled = false;

struct CpuidRegisters {
    std::uint32_t eax{0};
    std::uint32_t ebx{0};
    std::uint32_t ecx{0};
    std::uint32_t edx{0};
};

[[nodiscard]] CpuidRegisters query_cpuid(std::uint32_t leaf, std::uint32_t subleaf = 0) noexcept {
    CpuidRegisters registers;
#if MK_CPU_SIMD_X86_OR_X64
#if defined(_MSC_VER)
    std::array<int, 4> raw{};
    __cpuidex(raw.data(), static_cast<int>(leaf), static_cast<int>(subleaf));
    registers.eax = std::bit_cast<std::uint32_t>(raw[0]);
    registers.ebx = std::bit_cast<std::uint32_t>(raw[1]);
    registers.ecx = std::bit_cast<std::uint32_t>(raw[2]);
    registers.edx = std::bit_cast<std::uint32_t>(raw[3]);
#else
    std::uint32_t eax{};
    std::uint32_t ebx{};
    std::uint32_t ecx{};
    std::uint32_t edx{};
    if (__get_cpuid_count(leaf, subleaf, &eax, &ebx, &ecx, &edx) != 0) {
        registers = CpuidRegisters{.eax = eax, .ebx = ebx, .ecx = ecx, .edx = edx};
    }
#endif
#else
    (void)leaf;
    (void)subleaf;
#endif
    return registers;
}

[[nodiscard]] std::uint64_t query_xcr0() noexcept {
#if MK_CPU_SIMD_X86_OR_X64
#if defined(_MSC_VER)
    return _xgetbv(0);
#elif MK_CPU_SIMD_COMPILE_AVX2
    return _xgetbv(0);
#else
    return 0;
#endif
#else
    return 0;
#endif
}

[[nodiscard]] bool os_supports_avx_registers(const CpuidRegisters& leaf1) noexcept {
    if ((leaf1.ecx & kOsxsaveEcxBit) == 0U || (leaf1.ecx & kAvxEcxBit) == 0U) {
        return false;
    }
    constexpr std::uint64_t xmm_ymm_state_mask = 0x6U;
    return (query_xcr0() & xmm_ymm_state_mask) == xmm_ymm_state_mask;
}

void append_diagnostic(SimdDispatchPolicy& policy, SimdDispatchDiagnosticCode code, std::string message) {
    policy.diagnostic_codes.push_back(code);
    policy.diagnostics.push_back(std::move(message));
}

[[nodiscard]] bool has_diagnostic(const SimdDispatchPolicy& policy, SimdDispatchDiagnosticCode code) noexcept {
    for (const auto diagnostic : policy.diagnostic_codes) {
        if (diagnostic == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool lane_compile_supported(CpuSimdLane lane, const CpuSimdFeatureSet& features) noexcept {
    switch (lane) {
    case CpuSimdLane::scalar:
        return true;
    case CpuSimdLane::sse2:
        return features.sse2_compile_supported;
    case CpuSimdLane::avx2:
        return features.avx2_compile_supported;
    }
    return false;
}

[[nodiscard]] bool lane_runtime_supported(CpuSimdLane lane, const CpuSimdFeatureSet& features) noexcept {
    switch (lane) {
    case CpuSimdLane::scalar:
        return true;
    case CpuSimdLane::sse2:
        return features.sse2_runtime_supported;
    case CpuSimdLane::avx2:
        return features.avx2_runtime_supported;
    }
    return false;
}

void check_lane(SimdDispatchPolicy& policy, CpuSimdLane lane) {
    if (!policy.features.x86_or_x64_host && lane != CpuSimdLane::scalar) {
        append_diagnostic(policy, SimdDispatchDiagnosticCode::non_x86_host,
                          "CPU SIMD dispatch lane requires an x86/x64 host");
    }
    if (!lane_compile_supported(lane, policy.features)) {
        append_diagnostic(policy, SimdDispatchDiagnosticCode::compile_lane_unavailable,
                          "requested CPU SIMD lane is not available in the current compile target");
    }
    if (!lane_runtime_supported(lane, policy.features)) {
        append_diagnostic(policy, SimdDispatchDiagnosticCode::runtime_lane_unavailable,
                          "requested CPU SIMD lane is not available on the current CPU at runtime");
    }
    if (lane == CpuSimdLane::avx2 && !kReviewedAvx2DispatchLaneEnabled) {
        append_diagnostic(policy, SimdDispatchDiagnosticCode::reviewed_target_gate_missing,
                          "AVX2 dispatch requires a reviewed per-target compile configuration before selection");
    }
}

[[nodiscard]] CpuSimdLane select_auto_lane(const CpuSimdFeatureSet& features) noexcept {
    if (features.sse2_compile_supported && features.sse2_runtime_supported) {
        return CpuSimdLane::sse2;
    }
    return CpuSimdLane::scalar;
}

[[nodiscard]] float compute_sse2_dot_product(std::span<const float> lhs, std::span<const float> rhs) noexcept {
#if MK_CPU_SIMD_COMPILE_SSE2
    std::size_t index = 0;
    __m128 sum = _mm_setzero_ps();
    for (; index + 4U <= lhs.size(); index += 4U) {
        const __m128 left = _mm_loadu_ps(lhs.data() + index);
        const __m128 right = _mm_loadu_ps(rhs.data() + index);
        sum = _mm_add_ps(sum, _mm_mul_ps(left, right));
    }

    std::array<float, 4> lanes{};
    _mm_storeu_ps(lanes.data(), sum);
    float result = lanes[0] + lanes[1] + lanes[2] + lanes[3];
    for (; index < lhs.size(); ++index) {
        result += lhs[index] * rhs[index];
    }
    return result;
#else
    return compute_scalar_dot_product(lhs, rhs);
#endif
}

} // namespace

CpuSimdFeatureSet observe_cpu_simd_features() noexcept {
    CpuSimdFeatureSet features;
    features.x86_or_x64_host = MK_CPU_SIMD_X86_OR_X64 != 0;
    features.sse2_compile_supported = MK_CPU_SIMD_COMPILE_SSE2 != 0;
    features.avx2_compile_supported = MK_CPU_SIMD_COMPILE_AVX2 != 0;

#if MK_CPU_SIMD_X86_OR_X64
    const auto leaf0 = query_cpuid(0);
    if (leaf0.eax >= 1U) {
        const auto leaf1 = query_cpuid(1);
        features.sse2_runtime_supported = (leaf1.edx & kSse2EdxBit) != 0U;
        if (leaf0.eax >= 7U) {
            const auto leaf7 = query_cpuid(7);
            features.avx2_runtime_supported = os_supports_avx_registers(leaf1) && (leaf7.ebx & kAvx2EbxBit) != 0U;
        }
    }
#endif

    return features;
}

SimdDispatchPolicy select_simd_dispatch_policy(const SimdDispatchPolicyDesc& desc) {
    SimdDispatchPolicy policy;
    policy.requested_lane = desc.requested_lane;
    policy.features = desc.features.value_or(observe_cpu_simd_features());

    const CpuSimdLane candidate = [&policy] {
        switch (policy.requested_lane) {
        case CpuSimdLaneRequest::auto_select:
            return select_auto_lane(policy.features);
        case CpuSimdLaneRequest::scalar:
            return CpuSimdLane::scalar;
        case CpuSimdLaneRequest::sse2:
            return CpuSimdLane::sse2;
        case CpuSimdLaneRequest::avx2:
            return CpuSimdLane::avx2;
        }
        return CpuSimdLane::scalar;
    }();

    if (candidate != CpuSimdLane::scalar) {
        check_lane(policy, candidate);
    }

    if (!policy.diagnostic_codes.empty()) {
        policy.status = SimdDispatchStatus::unsupported;
        policy.selected_lane = CpuSimdLane::scalar;
        policy.scalar_fallback = true;
        return policy;
    }

    policy.status = SimdDispatchStatus::ready;
    policy.selected_lane = candidate;
    policy.scalar_fallback = candidate == CpuSimdLane::scalar;
    policy.sse2_selected = candidate == CpuSimdLane::sse2;
    policy.avx2_selected = false;
    return policy;
}

float compute_scalar_dot_product(std::span<const float> lhs, std::span<const float> rhs) noexcept {
    float result = 0.0F;
    const auto count = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
    for (std::size_t index = 0; index < count; ++index) {
        result += lhs[index] * rhs[index];
    }
    return result;
}

SimdDotProductEvidence build_simd_dot_product_evidence(std::span<const float> lhs, std::span<const float> rhs,
                                                       const SimdDispatchPolicyDesc& desc) {
    SimdDotProductEvidence evidence;
    evidence.policy = select_simd_dispatch_policy(desc);

    if (lhs.empty() || rhs.empty()) {
        evidence.policy.status = SimdDispatchStatus::invalid_input;
        append_diagnostic(evidence.policy, SimdDispatchDiagnosticCode::empty_input,
                          "SIMD dot product evidence requires non-empty spans");
        evidence.diagnostics = evidence.policy.diagnostics;
        return evidence;
    }
    if (lhs.size() != rhs.size()) {
        evidence.policy.status = SimdDispatchStatus::invalid_input;
        append_diagnostic(evidence.policy, SimdDispatchDiagnosticCode::input_size_mismatch,
                          "SIMD dot product evidence requires equal-size spans");
        evidence.diagnostics = evidence.policy.diagnostics;
        return evidence;
    }

    evidence.input_count = lhs.size();
    switch (evidence.policy.selected_lane) {
    case CpuSimdLane::scalar:
        evidence.result = compute_scalar_dot_product(lhs, rhs);
        break;
    case CpuSimdLane::sse2:
        evidence.result = compute_sse2_dot_product(lhs, rhs);
        break;
    case CpuSimdLane::avx2:
        evidence.result = compute_scalar_dot_product(lhs, rhs);
        break;
    }

    if (evidence.policy.status == SimdDispatchStatus::unsupported &&
        !has_diagnostic(evidence.policy, SimdDispatchDiagnosticCode::compile_lane_unavailable) &&
        !has_diagnostic(evidence.policy, SimdDispatchDiagnosticCode::runtime_lane_unavailable)) {
        append_diagnostic(evidence.policy, SimdDispatchDiagnosticCode::runtime_lane_unavailable,
                          "SIMD dispatch policy fell back to scalar");
    }
    evidence.diagnostics = evidence.policy.diagnostics;
    return evidence;
}

std::string_view cpu_simd_lane_label(CpuSimdLane lane) noexcept {
    switch (lane) {
    case CpuSimdLane::scalar:
        return "scalar";
    case CpuSimdLane::sse2:
        return "sse2";
    case CpuSimdLane::avx2:
        return "avx2";
    }
    return "unknown";
}

std::string_view cpu_simd_lane_request_label(CpuSimdLaneRequest lane) noexcept {
    switch (lane) {
    case CpuSimdLaneRequest::auto_select:
        return "auto_select";
    case CpuSimdLaneRequest::scalar:
        return "scalar";
    case CpuSimdLaneRequest::sse2:
        return "sse2";
    case CpuSimdLaneRequest::avx2:
        return "avx2";
    }
    return "unknown";
}

std::string_view simd_dispatch_status_label(SimdDispatchStatus status) noexcept {
    switch (status) {
    case SimdDispatchStatus::ready:
        return "ready";
    case SimdDispatchStatus::invalid_input:
        return "invalid_input";
    case SimdDispatchStatus::unsupported:
        return "unsupported";
    }
    return "unknown";
}

std::string_view simd_dispatch_diagnostic_code_label(SimdDispatchDiagnosticCode code) noexcept {
    switch (code) {
    case SimdDispatchDiagnosticCode::none:
        return "none";
    case SimdDispatchDiagnosticCode::empty_input:
        return "empty_input";
    case SimdDispatchDiagnosticCode::input_size_mismatch:
        return "input_size_mismatch";
    case SimdDispatchDiagnosticCode::compile_lane_unavailable:
        return "compile_lane_unavailable";
    case SimdDispatchDiagnosticCode::runtime_lane_unavailable:
        return "runtime_lane_unavailable";
    case SimdDispatchDiagnosticCode::reviewed_target_gate_missing:
        return "reviewed_target_gate_missing";
    case SimdDispatchDiagnosticCode::non_x86_host:
        return "non_x86_host";
    }
    return "unknown";
}

} // namespace mirakana
