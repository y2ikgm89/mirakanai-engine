// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct AnimationTimelineEventDesc {
    float time_seconds{0.0F};
    std::string name;
    std::string payload;
    std::string track{"default"};
};

struct AnimationTimelineDesc {
    float duration_seconds{0.0F};
    bool looping{false};
    std::vector<AnimationTimelineEventDesc> events;
};

struct AnimationTimelineEventTrackDesc {
    std::string name;
    std::vector<AnimationTimelineEventDesc> events;
};

struct AnimationAuthoredTimelineDesc {
    float duration_seconds{0.0F};
    bool looping{false};
    std::vector<AnimationTimelineEventTrackDesc> tracks;
};

struct AnimationTimelineEvent {
    float time_seconds{0.0F};
    std::string name;
    std::string payload;
    std::uint32_t loop_index{0};
    std::string track{"default"};
};

[[nodiscard]] bool is_valid_animation_timeline_desc(const AnimationTimelineDesc& desc) noexcept;
[[nodiscard]] bool is_valid_animation_authored_timeline_desc(const AnimationAuthoredTimelineDesc& desc) noexcept;
[[nodiscard]] AnimationTimelineDesc build_animation_timeline_from_tracks(const AnimationAuthoredTimelineDesc& desc);

class AnimationTimelinePlayback {
  public:
    explicit AnimationTimelinePlayback(AnimationTimelineDesc desc);

    [[nodiscard]] float time_seconds() const noexcept;
    [[nodiscard]] std::uint32_t loop_index() const noexcept;
    [[nodiscard]] bool finished() const noexcept;

    void reset() noexcept;
    [[nodiscard]] std::vector<AnimationTimelineEvent> update(float delta_seconds);

  private:
    void append_events_in_range(float start_seconds, float end_seconds, bool include_start, std::uint32_t loop_index,
                                std::vector<AnimationTimelineEvent>& events) const;

    AnimationTimelineDesc desc_;
    float time_seconds_{0.0F};
    std::uint32_t loop_index_{0};
    bool finished_{false};
};

} // namespace mirakana
