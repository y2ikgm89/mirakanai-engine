// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/time.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace mirakana {

FixedTimestep::FixedTimestep(double fixed_delta_seconds, double max_frame_delta_seconds)
    : fixed_delta_seconds_(fixed_delta_seconds), max_frame_delta_seconds_(max_frame_delta_seconds) {
    if (fixed_delta_seconds_ <= 0.0) {
        throw std::invalid_argument("fixed delta must be greater than zero");
    }
    if (max_frame_delta_seconds_ < fixed_delta_seconds_) {
        throw std::invalid_argument("max frame delta must be at least fixed delta");
    }
}

FrameStep FixedTimestep::begin_frame(double real_delta_seconds) {
    const double clamped_delta = std::clamp(real_delta_seconds, 0.0, max_frame_delta_seconds_);
    accumulator_seconds_ += clamped_delta;

    std::uint32_t fixed_steps = 0;
    while (accumulator_seconds_ >= fixed_delta_seconds_) {
        accumulator_seconds_ -= fixed_delta_seconds_;
        ++fixed_steps;
    }

    return FrameStep{.frame_delta_seconds = clamped_delta,
                     .fixed_steps = fixed_steps,
                     .interpolation_alpha = accumulator_seconds_ / fixed_delta_seconds_};
}

double FixedTimestep::fixed_delta_seconds() const noexcept {
    return fixed_delta_seconds_;
}

double FixedTimestep::accumulator_seconds() const noexcept {
    return accumulator_seconds_;
}

void FixedTimestep::reset() noexcept {
    accumulator_seconds_ = 0.0;
}

} // namespace mirakana
