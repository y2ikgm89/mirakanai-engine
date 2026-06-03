// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <span>

namespace mirakana::detail {

[[nodiscard]] float compute_avx2_dot_product(std::span<const float> lhs, std::span<const float> rhs) noexcept;

} // namespace mirakana::detail
