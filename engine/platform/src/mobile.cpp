// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/mobile.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace mirakana {

namespace {

[[nodiscard]] constexpr std::size_t permission_index(MobilePermissionKind kind) noexcept {
    return static_cast<std::size_t>(kind);
}

[[nodiscard]] constexpr bool is_valid_permission_kind(MobilePermissionKind kind) noexcept {
    return kind != MobilePermissionKind::unknown &&
           permission_index(kind) < permission_index(MobilePermissionKind::count);
}

[[nodiscard]] constexpr bool is_valid_permission_status(MobilePermissionStatus status) noexcept {
    return status != MobilePermissionStatus::unknown;
}

[[nodiscard]] bool is_non_empty_path(const std::string& path) noexcept {
    return !path.empty();
}

} // namespace

bool is_valid_mobile_lifecycle_event(MobileLifecycleEventKind kind) noexcept {
    switch (kind) {
    case MobileLifecycleEventKind::started:
    case MobileLifecycleEventKind::resumed:
    case MobileLifecycleEventKind::paused:
    case MobileLifecycleEventKind::stopped:
    case MobileLifecycleEventKind::low_memory:
    case MobileLifecycleEventKind::back_requested:
    case MobileLifecycleEventKind::destroyed:
        return true;
    case MobileLifecycleEventKind::unknown:
        return false;
    }
    return false;
}

std::optional<LifecycleEventKind> map_mobile_lifecycle_event(MobileLifecycleEventKind kind) noexcept {
    switch (kind) {
    case MobileLifecycleEventKind::started:
        return LifecycleEventKind::will_enter_foreground;
    case MobileLifecycleEventKind::resumed:
        return LifecycleEventKind::did_enter_foreground;
    case MobileLifecycleEventKind::paused:
        return LifecycleEventKind::will_enter_background;
    case MobileLifecycleEventKind::stopped:
        return LifecycleEventKind::did_enter_background;
    case MobileLifecycleEventKind::low_memory:
        return LifecycleEventKind::low_memory;
    case MobileLifecycleEventKind::back_requested:
        return LifecycleEventKind::quit_requested;
    case MobileLifecycleEventKind::destroyed:
        return LifecycleEventKind::terminating;
    case MobileLifecycleEventKind::unknown:
        return std::nullopt;
    }
    return std::nullopt;
}

bool push_mobile_lifecycle_event(VirtualLifecycle& lifecycle, MobileLifecycleEventKind kind) {
    const auto mapped = map_mobile_lifecycle_event(kind);
    if (!mapped.has_value()) {
        return false;
    }

    lifecycle.push(*mapped);
    return true;
}

bool is_valid_mobile_orientation(MobileOrientation orientation) noexcept {
    switch (orientation) {
    case MobileOrientation::portrait:
    case MobileOrientation::portrait_upside_down:
    case MobileOrientation::landscape_left:
    case MobileOrientation::landscape_right:
        return true;
    case MobileOrientation::unknown:
        return false;
    }
    return false;
}

bool is_landscape_mobile_orientation(MobileOrientation orientation) noexcept {
    return orientation == MobileOrientation::landscape_left || orientation == MobileOrientation::landscape_right;
}

WindowExtent oriented_mobile_extent(WindowExtent natural_extent, MobileOrientation orientation) noexcept {
    if (!is_landscape_mobile_orientation(orientation)) {
        return natural_extent;
    }

    return WindowExtent{.width = natural_extent.height, .height = natural_extent.width};
}

bool is_valid_mobile_safe_area(MobileSafeArea safe_area) noexcept {
    return std::isfinite(safe_area.left) && std::isfinite(safe_area.top) && std::isfinite(safe_area.right) &&
           std::isfinite(safe_area.bottom) && safe_area.left >= 0.0F && safe_area.top >= 0.0F &&
           safe_area.right >= 0.0F && safe_area.bottom >= 0.0F;
}

bool is_valid_mobile_viewport_state(const MobileViewportState& state) noexcept {
    return state.pixel_extent.width > 0 && state.pixel_extent.height > 0 &&
           is_valid_mobile_safe_area(state.safe_area) && is_valid_mobile_orientation(state.orientation) &&
           std::isfinite(state.pixel_density) && state.pixel_density > 0.0F;
}

bool is_valid_mobile_storage_roots(const MobileStorageRoots& roots) noexcept {
    return is_non_empty_path(roots.save_data) && is_non_empty_path(roots.cache);
}

bool is_valid_mobile_touch_sample(const MobileTouchSample& sample) noexcept {
    return sample.id != 0 && sample.phase != MobileTouchPhase::unknown && std::isfinite(sample.position.x) &&
           std::isfinite(sample.position.y) && std::isfinite(sample.pressure) && sample.pressure >= 0.0F;
}

std::optional<PointerSample> map_mobile_touch_sample(const MobileTouchSample& sample) noexcept {
    if (!is_valid_mobile_touch_sample(sample)) {
        return std::nullopt;
    }

    return PointerSample{.id = sample.id, .kind = PointerKind::touch, .position = sample.position};
}

void MobilePermissionRegistry::set_status(MobilePermissionKind kind, MobilePermissionStatus status_value) {
    if (!is_valid_permission_kind(kind) || !is_valid_permission_status(status_value)) {
        return;
    }

    if (auto* record = find(kind)) {
        record->status = status_value;
        return;
    }

    permissions_.push_back(MobilePermissionRecord{.kind = kind, .status = status_value});
    sort_permissions();
}

MobilePermissionStatus MobilePermissionRegistry::status(MobilePermissionKind kind) const noexcept {
    if (const auto* record = find(kind)) {
        return record->status;
    }
    return MobilePermissionStatus::unknown;
}

bool MobilePermissionRegistry::granted(MobilePermissionKind kind) const noexcept {
    return status(kind) == MobilePermissionStatus::granted;
}

const std::vector<MobilePermissionRecord>& MobilePermissionRegistry::permissions() const noexcept {
    return permissions_;
}

std::vector<MobilePermissionKind>
MobilePermissionRegistry::missing_permissions(std::initializer_list<MobilePermissionKind> required) const {
    std::vector<MobilePermissionKind> missing;
    for (const auto kind : required) {
        if (!is_valid_permission_kind(kind) || granted(kind)) {
            continue;
        }

        if (!std::ranges::contains(missing, kind)) {
            missing.push_back(kind);
        }
    }

    std::ranges::sort(missing, [](MobilePermissionKind lhs, MobilePermissionKind rhs) {
        return permission_index(lhs) < permission_index(rhs);
    });
    return missing;
}

MobilePermissionRecord* MobilePermissionRegistry::find(MobilePermissionKind kind) noexcept {
    const auto found = std::ranges::find_if(
        permissions_, [kind](const MobilePermissionRecord& record) { return record.kind == kind; });
    return found != permissions_.end() ? &(*found) : nullptr;
}

const MobilePermissionRecord* MobilePermissionRegistry::find(MobilePermissionKind kind) const noexcept {
    const auto found = std::ranges::find_if(
        permissions_, [kind](const MobilePermissionRecord& record) { return record.kind == kind; });
    return found != permissions_.end() ? &(*found) : nullptr;
}

void MobilePermissionRegistry::sort_permissions() noexcept {
    std::ranges::sort(permissions_, [](const MobilePermissionRecord& lhs, const MobilePermissionRecord& rhs) {
        return permission_index(lhs.kind) < permission_index(rhs.kind);
    });
}

} // namespace mirakana
