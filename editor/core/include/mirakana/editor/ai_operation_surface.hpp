// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/workspace.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::editor {

struct EditorAiOperationElementRow {
    std::string id;
    std::string role;
    std::string label;
    bool visible{true};
    bool enabled{true};
};

struct EditorAiOperationDiagnostic {
    std::string code;
    std::string message;
};

struct EditorAiOperationSnapshot {
    std::uint64_t revision{0};
    std::vector<EditorAiOperationElementRow> elements;
    std::vector<EditorAiOperationDiagnostic> diagnostics;
};

struct EditorAiCommandRow {
    std::string id;
    std::string label;
    std::string target_element_id;
    bool enabled{true};
    bool mutates_state{false};
    bool requires_confirmation{false};
};

struct EditorAiCommandCatalog {
    std::uint64_t revision{0};
    std::vector<EditorAiCommandRow> commands;
};

struct EditorAiCommandParameter {
    std::string key;
    std::string value;
};

struct EditorAiCommandRequest {
    std::string command_id;
    std::string target_element_id;
    std::vector<EditorAiCommandParameter> parameters;
    bool user_confirmed{false};
};

struct EditorAiCommandDryRunResult {
    bool accepted{false};
    bool would_mutate{false};
    bool requires_confirmation{false};
    std::vector<EditorAiOperationDiagnostic> diagnostics;
};

struct EditorAiCommandApplyResult {
    bool applied{false};
    std::uint64_t before_revision{0};
    std::uint64_t after_revision{0};
    std::vector<EditorAiOperationDiagnostic> diagnostics;
};

[[nodiscard]] EditorAiOperationSnapshot make_editor_ai_operation_snapshot(const Workspace& workspace);

[[nodiscard]] EditorAiCommandCatalog make_editor_ai_command_catalog(const Workspace& workspace);

[[nodiscard]] EditorAiCommandDryRunResult dry_run_editor_ai_command(const Workspace& workspace,
                                                                    const EditorAiCommandCatalog& catalog,
                                                                    const EditorAiCommandRequest& request);

[[nodiscard]] EditorAiCommandApplyResult apply_editor_ai_command(Workspace& workspace,
                                                                 const EditorAiCommandCatalog& catalog,
                                                                 const EditorAiCommandRequest& request);

} // namespace mirakana::editor
