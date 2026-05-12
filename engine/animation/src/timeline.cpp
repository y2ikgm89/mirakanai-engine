// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/animation/timeline.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool is_safe_name(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('=') == std::string_view::npos;
}

[[nodiscard]] bool is_safe_payload(std::string_view value) noexcept {
    return value.find('\n') == std::string_view::npos && value.find('=') == std::string_view::npos;
}

} // namespace

bool is_valid_animation_timeline_desc(const AnimationTimelineDesc& desc) noexcept {
    if (!finite(desc.duration_seconds) || desc.duration_seconds <= 0.0F) {
        return false;
    }

    float previous_time = -1.0F;
    for (const auto& event : desc.events) {
        if (!finite(event.time_seconds) || event.time_seconds < 0.0F || event.time_seconds > desc.duration_seconds ||
            event.time_seconds < previous_time || !is_safe_name(event.name) || !is_safe_payload(event.payload) ||
            !is_safe_name(event.track)) {
            return false;
        }
        previous_time = event.time_seconds;
    }

    return true;
}

bool is_valid_animation_authored_timeline_desc(const AnimationAuthoredTimelineDesc& desc) noexcept {
    if (!finite(desc.duration_seconds) || desc.duration_seconds <= 0.0F || desc.tracks.empty()) {
        return false;
    }

    for (std::size_t index = 0; index < desc.tracks.size(); ++index) {
        const auto& track = desc.tracks[index];
        if (!is_safe_name(track.name) || track.events.empty()) {
            return false;
        }
        for (std::size_t other = index + 1U; other < desc.tracks.size(); ++other) {
            if (track.name == desc.tracks[other].name) {
                return false;
            }
        }

        float previous_time = -1.0F;
        for (const auto& event : track.events) {
            if (!finite(event.time_seconds) || event.time_seconds < 0.0F ||
                event.time_seconds > desc.duration_seconds || event.time_seconds < previous_time ||
                !is_safe_name(event.name) || !is_safe_payload(event.payload)) {
                return false;
            }
            previous_time = event.time_seconds;
        }
    }

    return true;
}

AnimationTimelineDesc build_animation_timeline_from_tracks(const AnimationAuthoredTimelineDesc& desc) {
    if (!is_valid_animation_authored_timeline_desc(desc)) {
        throw std::invalid_argument("animation authored timeline description is invalid");
    }

    std::vector<AnimationTimelineEventDesc> events;
    for (const auto& track : desc.tracks) {
        for (auto event : track.events) {
            event.track = track.name;
            events.push_back(std::move(event));
        }
    }

    std::ranges::sort(events, [](const AnimationTimelineEventDesc& lhs, const AnimationTimelineEventDesc& rhs) {
        if (lhs.time_seconds != rhs.time_seconds) {
            return lhs.time_seconds < rhs.time_seconds;
        }
        if (lhs.track != rhs.track) {
            return lhs.track < rhs.track;
        }
        if (lhs.name != rhs.name) {
            return lhs.name < rhs.name;
        }
        return lhs.payload < rhs.payload;
    });

    return AnimationTimelineDesc{
        .duration_seconds = desc.duration_seconds, .looping = desc.looping, .events = std::move(events)};
}

AnimationTimelinePlayback::AnimationTimelinePlayback(AnimationTimelineDesc desc) : desc_(std::move(desc)) {
    if (!is_valid_animation_timeline_desc(desc_)) {
        throw std::invalid_argument("animation timeline description is invalid");
    }
}

float AnimationTimelinePlayback::time_seconds() const noexcept {
    return time_seconds_;
}

std::uint32_t AnimationTimelinePlayback::loop_index() const noexcept {
    return loop_index_;
}

bool AnimationTimelinePlayback::finished() const noexcept {
    return finished_;
}

void AnimationTimelinePlayback::reset() noexcept {
    time_seconds_ = 0.0F;
    loop_index_ = 0;
    finished_ = false;
}

std::vector<AnimationTimelineEvent> AnimationTimelinePlayback::update(float delta_seconds) {
    if (!finite(delta_seconds) || delta_seconds < 0.0F) {
        throw std::invalid_argument("animation timeline delta seconds is invalid");
    }

    std::vector<AnimationTimelineEvent> events;
    if (delta_seconds == 0.0F || finished_) {
        return events;
    }

    if (!desc_.looping) {
        const auto start = time_seconds_;
        const auto end = std::min(desc_.duration_seconds, time_seconds_ + delta_seconds);
        append_events_in_range(start, end, false, loop_index_, events);
        time_seconds_ = end;
        finished_ = time_seconds_ >= desc_.duration_seconds;
        return events;
    }

    auto remaining_seconds = delta_seconds;
    auto include_start = false;
    while (remaining_seconds > 0.0F) {
        const auto seconds_until_wrap = desc_.duration_seconds - time_seconds_;
        if (remaining_seconds <= seconds_until_wrap) {
            const auto end = time_seconds_ + remaining_seconds;
            append_events_in_range(time_seconds_, end, include_start, loop_index_, events);
            time_seconds_ = end;
            remaining_seconds = 0.0F;
        } else {
            append_events_in_range(time_seconds_, desc_.duration_seconds, include_start, loop_index_, events);
            remaining_seconds -= seconds_until_wrap;
            time_seconds_ = 0.0F;
            ++loop_index_;
            include_start = true;
        }
    }

    if (time_seconds_ >= desc_.duration_seconds) {
        time_seconds_ = 0.0F;
        ++loop_index_;
    }

    return events;
}

void AnimationTimelinePlayback::append_events_in_range(float start_seconds, float end_seconds, bool include_start,
                                                       std::uint32_t loop_index,
                                                       std::vector<AnimationTimelineEvent>& events) const {
    for (const auto& event : desc_.events) {
        const auto after_start =
            include_start ? event.time_seconds >= start_seconds : event.time_seconds > start_seconds;
        if (after_start && event.time_seconds <= end_seconds) {
            events.push_back(AnimationTimelineEvent{.time_seconds = event.time_seconds,
                                                    .name = event.name,
                                                    .payload = event.payload,
                                                    .loop_index = loop_index,
                                                    .track = event.track});
        }
    }
}

} // namespace mirakana
