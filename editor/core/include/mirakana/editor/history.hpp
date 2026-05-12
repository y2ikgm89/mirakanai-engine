// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

struct UndoableAction {
    std::string label;
    std::function<void()> redo;
    std::function<void()> undo;
};

class UndoStack {
  public:
    [[nodiscard]] bool execute(UndoableAction action);
    [[nodiscard]] bool undo();
    [[nodiscard]] bool redo();

    [[nodiscard]] bool can_undo() const noexcept;
    [[nodiscard]] bool can_redo() const noexcept;
    [[nodiscard]] std::string_view undo_label() const noexcept;
    [[nodiscard]] std::string_view redo_label() const noexcept;
    [[nodiscard]] std::size_t undo_count() const noexcept;
    [[nodiscard]] std::size_t redo_count() const noexcept;

    void clear() noexcept;

  private:
    [[nodiscard]] static bool valid(const UndoableAction& action) noexcept;

    std::vector<UndoableAction> undo_stack_;
    std::vector<UndoableAction> redo_stack_;
};

} // namespace mirakana::editor
