// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <vector>

namespace mirakana {

enum class LifecycleEventKind : std::uint8_t {
    quit_requested,
    terminating,
    low_memory,
    will_enter_background,
    did_enter_background,
    will_enter_foreground,
    did_enter_foreground,
};

struct LifecycleEvent {
    LifecycleEventKind kind{LifecycleEventKind::quit_requested};
    std::uint64_t sequence{0};
};

struct LifecycleState {
    bool quit_requested{false};
    bool terminating{false};
    bool low_memory{false};
    bool backgrounded{false};
    bool interactive{true};
};

class VirtualLifecycle final {
  public:
    [[nodiscard]] const std::vector<LifecycleEvent>& events() const noexcept;
    [[nodiscard]] LifecycleState state() const noexcept;

    void push(LifecycleEventKind kind);
    void begin_frame();
    void reset();

  private:
    std::vector<LifecycleEvent> events_;
    std::uint64_t next_sequence_{1};
    LifecycleState state_{};
};

} // namespace mirakana
