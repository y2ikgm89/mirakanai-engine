// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeScriptSandboxPermissionKind : std::uint8_t {
    host_api = 0,
    read_runtime_state,
    write_runtime_state,
    emit_diagnostic,
    read_save_profile,
    write_save_profile,
    filesystem_read,
    filesystem_write,
    network,
    process,
    native_plugin,
};

enum class RuntimeScriptSandboxEntrypointKind : std::uint8_t {
    init = 0,
    update,
    event,
    command,
};

enum class RuntimeScriptSandboxPlanStatus : std::uint8_t {
    planned = 0,
    no_modules,
    invalid_request,
    budget_exceeded,
};

enum class RuntimeScriptSandboxDiagnosticCode : std::uint8_t {
    missing_module_id = 0,
    duplicate_module_id,
    missing_entrypoint_id,
    duplicate_entrypoint_id,
    missing_replay_seed,
    invalid_instruction_budget,
    invalid_memory_budget,
    instruction_budget_exceeded,
    memory_budget_exceeded,
    default_denied_permission,
    missing_host_api_id,
    unsupported_host_api_id,
};

struct RuntimeScriptSandboxPermissionDesc {
    RuntimeScriptSandboxPermissionKind kind{RuntimeScriptSandboxPermissionKind::emit_diagnostic};
    std::string host_api_id;
    std::uint32_t source_index{0U};
};

struct RuntimeScriptSandboxEntrypointDesc {
    std::string entrypoint_id;
    RuntimeScriptSandboxEntrypointKind kind{RuntimeScriptSandboxEntrypointKind::update};
    std::uint64_t instruction_budget{0U};
    std::uint64_t memory_budget_bytes{0U};
    std::uint64_t replay_seed{0U};
    std::vector<RuntimeScriptSandboxPermissionDesc> permissions;
    std::uint32_t source_index{0U};
};

struct RuntimeScriptSandboxModuleDesc {
    std::string module_id;
    std::string source_uri;
    std::uint32_t source_index{0U};
    std::vector<RuntimeScriptSandboxEntrypointDesc> entrypoints;
};

struct RuntimeScriptSandboxPolicyDesc {
    std::vector<RuntimeScriptSandboxModuleDesc> modules;
    std::vector<std::string> allowed_host_apis;
    std::uint64_t max_instruction_budget_per_entrypoint{0U};
    std::uint64_t max_memory_budget_bytes_per_entrypoint{0U};
};

struct RuntimeScriptSandboxDiagnostic {
    RuntimeScriptSandboxDiagnosticCode code{RuntimeScriptSandboxDiagnosticCode::missing_module_id};
    std::string module_id;
    std::string entrypoint_id;
    RuntimeScriptSandboxPermissionKind permission_kind{RuntimeScriptSandboxPermissionKind::emit_diagnostic};
    std::string host_api_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct RuntimeScriptSandboxEntrypointPlanRow {
    std::string module_id;
    std::string source_uri;
    std::string entrypoint_id;
    RuntimeScriptSandboxEntrypointKind kind{RuntimeScriptSandboxEntrypointKind::update};
    std::uint64_t instruction_budget{0U};
    std::uint64_t memory_budget_bytes{0U};
    std::uint64_t replay_seed{0U};
    std::size_t allowed_permission_count{0U};
    std::size_t denied_permission_count{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeScriptSandboxPermissionPlanRow {
    std::string module_id;
    std::string entrypoint_id;
    RuntimeScriptSandboxPermissionKind kind{RuntimeScriptSandboxPermissionKind::emit_diagnostic};
    std::string host_api_id;
    bool allowed{false};
    std::uint32_t source_index{0U};
};

struct RuntimeScriptSandboxPlan {
    RuntimeScriptSandboxPlanStatus status{RuntimeScriptSandboxPlanStatus::invalid_request};
    std::vector<RuntimeScriptSandboxDiagnostic> diagnostics;
    std::vector<RuntimeScriptSandboxEntrypointPlanRow> entrypoints;
    std::vector<RuntimeScriptSandboxPermissionPlanRow> permissions;
    std::uint64_t projected_instruction_budget{0U};
    std::uint64_t projected_memory_budget_bytes{0U};
    std::size_t denied_permission_count{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans reviewed, deterministic script-module sandbox metadata for generated gameplay.
/// This does not interpret scripts, load code, open files, access the network, spawn processes, bind native plugins,
/// mutate package state, expose native handles, or grant unreviewed host APIs.
[[nodiscard]] RuntimeScriptSandboxPlan plan_runtime_script_sandbox(const RuntimeScriptSandboxPolicyDesc& policy);

} // namespace mirakana::runtime
