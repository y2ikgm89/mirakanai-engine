// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>

namespace mirakana {

struct FrameStep {
    double frame_delta_seconds;
    std::uint32_t fixed_steps;
    double interpolation_alpha;
};

class FixedTimestep {
  public:
    FixedTimestep(double fixed_delta_seconds, double max_frame_delta_seconds);

    [[nodiscard]] FrameStep begin_frame(double real_delta_seconds);
    [[nodiscard]] double fixed_delta_seconds() const noexcept;
    [[nodiscard]] double accumulator_seconds() const noexcept;

    void reset() noexcept;

  private:
    double fixed_delta_seconds_;
    double max_frame_delta_seconds_;
    double accumulator_seconds_{0.0};
};

} // namespace mirakana
