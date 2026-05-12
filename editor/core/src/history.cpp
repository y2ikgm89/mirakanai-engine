// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/history.hpp"

#include <utility>

namespace mirakana::editor {

bool UndoStack::execute(UndoableAction action) {
    if (!valid(action)) {
        return false;
    }

    action.redo();
    undo_stack_.push_back(std::move(action));
    redo_stack_.clear();
    return true;
}

bool UndoStack::undo() {
    if (!can_undo()) {
        return false;
    }

    auto action = std::move(undo_stack_.back());
    undo_stack_.pop_back();
    action.undo();
    redo_stack_.push_back(std::move(action));
    return true;
}

bool UndoStack::redo() {
    if (!can_redo()) {
        return false;
    }

    auto action = std::move(redo_stack_.back());
    redo_stack_.pop_back();
    action.redo();
    undo_stack_.push_back(std::move(action));
    return true;
}

bool UndoStack::can_undo() const noexcept {
    return !undo_stack_.empty();
}

bool UndoStack::can_redo() const noexcept {
    return !redo_stack_.empty();
}

std::string_view UndoStack::undo_label() const noexcept {
    return can_undo() ? std::string_view(undo_stack_.back().label) : std::string_view();
}

std::string_view UndoStack::redo_label() const noexcept {
    return can_redo() ? std::string_view(redo_stack_.back().label) : std::string_view();
}

std::size_t UndoStack::undo_count() const noexcept {
    return undo_stack_.size();
}

std::size_t UndoStack::redo_count() const noexcept {
    return redo_stack_.size();
}

void UndoStack::clear() noexcept {
    undo_stack_.clear();
    redo_stack_.clear();
}

bool UndoStack::valid(const UndoableAction& action) noexcept {
    return !action.label.empty() && static_cast<bool>(action.redo) && static_cast<bool>(action.undo);
}

} // namespace mirakana::editor
