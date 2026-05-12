// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/platform/lifecycle.hpp"
#include "mirakana/platform/window.hpp"

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

namespace mirakana {

enum class MobileLifecycleEventKind {
    unknown = 0,
    started,
    resumed,
    paused,
    stopped,
    low_memory,
    back_requested,
    destroyed,
};

enum class MobileOrientation {
    unknown = 0,
    portrait,
    portrait_upside_down,
    landscape_left,
    landscape_right,
};

struct MobileSafeArea {
    float left{0.0F};
    float top{0.0F};
    float right{0.0F};
    float bottom{0.0F};
};

struct MobileViewportState {
    WindowExtent pixel_extent{};
    MobileSafeArea safe_area{};
    MobileOrientation orientation{MobileOrientation::unknown};
    float pixel_density{1.0F};
};

enum class MobileTouchPhase {
    unknown = 0,
    pressed,
    moved,
    released,
    canceled,
};

struct MobileTouchSample {
    PointerId id{0};
    MobileTouchPhase phase{MobileTouchPhase::unknown};
    Vec2 position{};
    float pressure{0.0F};
};

enum class MobilePermissionKind {
    unknown = 0,
    storage,
    microphone,
    notifications,
    count,
};

enum class MobilePermissionStatus {
    unknown = 0,
    not_determined,
    granted,
    denied,
    restricted,
};

struct MobilePermissionRecord {
    MobilePermissionKind kind{MobilePermissionKind::unknown};
    MobilePermissionStatus status{MobilePermissionStatus::unknown};
};

struct MobileStorageRoots {
    std::string save_data;
    std::string cache;
    std::string shared;
};

[[nodiscard]] bool is_valid_mobile_lifecycle_event(MobileLifecycleEventKind kind) noexcept;
[[nodiscard]] std::optional<LifecycleEventKind> map_mobile_lifecycle_event(MobileLifecycleEventKind kind) noexcept;
bool push_mobile_lifecycle_event(VirtualLifecycle& lifecycle, MobileLifecycleEventKind kind);

[[nodiscard]] bool is_valid_mobile_orientation(MobileOrientation orientation) noexcept;
[[nodiscard]] bool is_landscape_mobile_orientation(MobileOrientation orientation) noexcept;
[[nodiscard]] WindowExtent oriented_mobile_extent(WindowExtent natural_extent, MobileOrientation orientation) noexcept;

[[nodiscard]] bool is_valid_mobile_safe_area(MobileSafeArea safe_area) noexcept;
[[nodiscard]] bool is_valid_mobile_viewport_state(const MobileViewportState& state) noexcept;
[[nodiscard]] bool is_valid_mobile_storage_roots(const MobileStorageRoots& roots) noexcept;

[[nodiscard]] bool is_valid_mobile_touch_sample(const MobileTouchSample& sample) noexcept;
[[nodiscard]] std::optional<PointerSample> map_mobile_touch_sample(const MobileTouchSample& sample) noexcept;

class MobilePermissionRegistry final {
  public:
    void set_status(MobilePermissionKind kind, MobilePermissionStatus status);

    [[nodiscard]] MobilePermissionStatus status(MobilePermissionKind kind) const noexcept;
    [[nodiscard]] bool granted(MobilePermissionKind kind) const noexcept;
    [[nodiscard]] const std::vector<MobilePermissionRecord>& permissions() const noexcept;
    [[nodiscard]] std::vector<MobilePermissionKind>
    missing_permissions(std::initializer_list<MobilePermissionKind> required) const;

  private:
    [[nodiscard]] MobilePermissionRecord* find(MobilePermissionKind kind) noexcept;
    [[nodiscard]] const MobilePermissionRecord* find(MobilePermissionKind kind) const noexcept;
    void sort_permissions() noexcept;

    std::vector<MobilePermissionRecord> permissions_;
};

} // namespace mirakana
