// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/window.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace mirakana {

bool is_valid_display_rect(DisplayRect rect) noexcept {
    return rect.width > 0 && rect.height > 0;
}

bool is_valid_display_info(const DisplayInfo& info) noexcept {
    return info.id != 0 && is_valid_display_rect(info.bounds) && is_valid_display_rect(info.usable_bounds) &&
           std::isfinite(info.content_scale) && info.content_scale > 0.0F;
}

bool is_valid_window_display_state(WindowDisplayState state) noexcept {
    return state.display_id != 0 && std::isfinite(state.content_scale) && state.content_scale > 0.0F &&
           std::isfinite(state.pixel_density) && state.pixel_density > 0.0F && is_valid_display_rect(state.safe_area);
}

namespace {

[[nodiscard]] bool is_valid_window_extent(WindowExtent extent) noexcept {
    return extent.width > 0 && extent.height > 0;
}

[[nodiscard]] std::uint64_t usable_area(const DisplayInfo& display) noexcept {
    return static_cast<std::uint64_t>(display.usable_bounds.width) *
           static_cast<std::uint64_t>(display.usable_bounds.height);
}

[[nodiscard]] bool better_tie_breaker(const DisplayInfo& candidate, const DisplayInfo& current) noexcept {
    return candidate.id < current.id;
}

} // namespace

std::optional<DisplayInfo> select_display(const std::vector<DisplayInfo>& displays, DisplaySelectionRequest request) {
    std::optional<DisplayInfo> selected;

    for (const auto& display : displays) {
        if (!is_valid_display_info(display)) {
            continue;
        }

        if (request.policy == DisplaySelectionPolicy::specific) {
            if (display.id == request.display_id) {
                return display;
            }
            continue;
        }

        if (!selected.has_value()) {
            selected = display;
            continue;
        }

        switch (request.policy) {
        case DisplaySelectionPolicy::primary:
            if ((!selected->primary && display.primary) ||
                (selected->primary == display.primary && better_tie_breaker(display, *selected))) {
                selected = display;
            }
            break;
        case DisplaySelectionPolicy::specific:
            break;
        case DisplaySelectionPolicy::highest_content_scale:
            if (display.content_scale > selected->content_scale ||
                (display.content_scale == selected->content_scale && better_tie_breaker(display, *selected))) {
                selected = display;
            }
            break;
        case DisplaySelectionPolicy::largest_usable_area:
            if (usable_area(display) > usable_area(*selected) ||
                (usable_area(display) == usable_area(*selected) && better_tie_breaker(display, *selected))) {
                selected = display;
            }
            break;
        }
    }

    return selected;
}

std::optional<WindowPlacement> plan_window_placement(const std::vector<DisplayInfo>& displays,
                                                     WindowPlacementRequest request) {
    if (!is_valid_window_extent(request.extent)) {
        return std::nullopt;
    }

    const auto display = select_display(displays, request.display);
    if (!display.has_value() && request.policy != WindowPlacementPolicy::absolute) {
        return std::nullopt;
    }

    WindowPosition position = request.position;
    if (display.has_value()) {
        switch (request.policy) {
        case WindowPlacementPolicy::centered: {
            const auto& bounds = display->usable_bounds;
            position = WindowPosition{
                .x = static_cast<std::int32_t>(
                    bounds.x +
                    ((static_cast<std::int64_t>(bounds.width) - static_cast<std::int64_t>(request.extent.width)) / 2)),
                .y = static_cast<std::int32_t>(bounds.y + ((static_cast<std::int64_t>(bounds.height) -
                                                            static_cast<std::int64_t>(request.extent.height)) /
                                                           2)),
            };
            break;
        }
        case WindowPlacementPolicy::top_left:
            position = WindowPosition{.x = display->usable_bounds.x, .y = display->usable_bounds.y};
            break;
        case WindowPlacementPolicy::absolute:
            break;
        }
    }

    return WindowPlacement{
        .position = position, .extent = request.extent, .display_id = display.has_value() ? display->id : DisplayId{0}};
}

HeadlessWindow::HeadlessWindow(WindowDesc desc)
    : title_(std::move(desc.title)), extent_(desc.extent), position_(desc.position) {
    if (title_.empty()) {
        throw std::invalid_argument("window title must not be empty");
    }
    if (!is_valid_window_extent(extent_)) {
        throw std::invalid_argument("window extent must be non-zero");
    }
}

std::string_view HeadlessWindow::title() const noexcept {
    return title_;
}

WindowExtent HeadlessWindow::extent() const noexcept {
    return extent_;
}

WindowPosition HeadlessWindow::position() const noexcept {
    return position_;
}

bool HeadlessWindow::is_open() const noexcept {
    return open_;
}

void HeadlessWindow::resize(WindowExtent extent) {
    if (!is_valid_window_extent(extent)) {
        throw std::invalid_argument("window extent must be non-zero");
    }
    extent_ = extent;
}

void HeadlessWindow::move(WindowPosition position) {
    position_ = position;
}

void HeadlessWindow::apply_placement(WindowPlacement placement) {
    resize(placement.extent);
    move(placement.position);
}

void HeadlessWindow::request_close() noexcept {
    open_ = false;
}

} // namespace mirakana
