// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/simd_dispatch.hpp"

#include "simd_dispatch_detail.hpp"

#include <array>
#include <cstddef>

#if defined(MK_CORE_REVIEWED_AVX2_TARGET) &&                                                                           \
    (defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__))
#define MK_CORE_AVX2_TARGET_AVAILABLE 1
#include <immintrin.h>
#else
#define MK_CORE_AVX2_TARGET_AVAILABLE 0
#endif

namespace mirakana::detail {

[[nodiscard]] float compute_avx2_dot_product(std::span<const float> lhs, std::span<const float> rhs) noexcept {
#if MK_CORE_AVX2_TARGET_AVAILABLE
    std::size_t index = 0;
    __m256 sum = _mm256_setzero_ps();
    for (; index + 8U <= lhs.size(); index += 8U) {
        const __m256 left = _mm256_loadu_ps(lhs.data() + index);
        const __m256 right = _mm256_loadu_ps(rhs.data() + index);
        sum = _mm256_add_ps(sum, _mm256_mul_ps(left, right));
    }

    std::array<float, 8> lanes{};
    _mm256_storeu_ps(lanes.data(), sum);
    float result = 0.0F;
    for (const auto lane : lanes) {
        result += lane;
    }
    for (; index < lhs.size(); ++index) {
        result += lhs[index] * rhs[index];
    }
    return result;
#else
    float result = 0.0F;
    const auto count = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
    for (std::size_t index = 0; index < count; ++index) {
        result += lhs[index] * rhs[index];
    }
    return result;
#endif
}

} // namespace mirakana::detail
