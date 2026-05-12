// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/lifecycle.hpp"

namespace mirakana {

const std::vector<LifecycleEvent>& VirtualLifecycle::events() const noexcept {
    return events_;
}

LifecycleState VirtualLifecycle::state() const noexcept {
    return state_;
}

void VirtualLifecycle::push(LifecycleEventKind kind) {
    events_.push_back(LifecycleEvent{.kind = kind, .sequence = next_sequence_++});

    switch (kind) {
    case LifecycleEventKind::quit_requested:
        state_.quit_requested = true;
        break;
    case LifecycleEventKind::terminating:
        state_.terminating = true;
        state_.interactive = false;
        break;
    case LifecycleEventKind::low_memory:
        state_.low_memory = true;
        break;
    case LifecycleEventKind::will_enter_background:
        state_.interactive = false;
        break;
    case LifecycleEventKind::did_enter_background:
        state_.backgrounded = true;
        state_.interactive = false;
        break;
    case LifecycleEventKind::will_enter_foreground:
        state_.backgrounded = false;
        state_.interactive = false;
        break;
    case LifecycleEventKind::did_enter_foreground:
        state_.backgrounded = false;
        state_.interactive = true;
        break;
    }
}

void VirtualLifecycle::begin_frame() {
    events_.clear();
    state_.low_memory = false;
}

void VirtualLifecycle::reset() {
    events_.clear();
    next_sequence_ = 1;
    state_ = LifecycleState{};
}

} // namespace mirakana
