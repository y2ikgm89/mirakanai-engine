// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
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

enum class RuntimeScriptExecutionStatus : std::uint8_t {
    completed = 0,
    invalid_request,
    budget_exceeded,
    adapter_failed,
};

enum class RuntimeScriptExecutionDiagnosticCode : std::uint8_t {
    missing_plan = 0,
    policy_not_planned,
    missing_entrypoint,
    replay_seed_mismatch,
    invalid_instruction_budget,
    invalid_memory_budget,
    instruction_budget_exceeded,
    memory_budget_exceeded,
    denied_permission,
    adapter_failed,
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

struct RuntimeScriptExecutionStats {
    std::uint64_t instructions_consumed{0U};
    std::uint64_t memory_bytes_touched{0U};
    std::size_t host_api_call_count{0U};
};

struct RuntimeScriptHostApiCall {
    std::string host_api_id;
    std::string payload;
    std::uint32_t source_index{0U};
};

struct RuntimeScriptAdapterResult {
    bool completed{false};
    RuntimeScriptExecutionStats stats;
    std::vector<RuntimeScriptHostApiCall> host_api_calls;
    std::vector<std::string> output_rows;
    std::string diagnostic_message;
};

struct RuntimeScriptExecutionRequest {
    const RuntimeScriptSandboxPlan* plan{nullptr};
    std::string module_id;
    std::string entrypoint_id;
    std::string input_event_id;
    std::uint64_t instruction_budget{0U};
    std::uint64_t memory_budget_bytes{0U};
    std::uint64_t replay_seed{0U};
};

struct RuntimeScriptExecutionDiagnostic {
    RuntimeScriptExecutionDiagnosticCode code{RuntimeScriptExecutionDiagnosticCode::missing_plan};
    std::string module_id;
    std::string entrypoint_id;
    std::string message;
};

struct RuntimeScriptExecutionResult {
    RuntimeScriptExecutionStatus status{RuntimeScriptExecutionStatus::invalid_request};
    std::vector<RuntimeScriptExecutionDiagnostic> diagnostics;
    RuntimeScriptExecutionStats stats;
    std::vector<RuntimeScriptHostApiCall> host_api_calls;
    std::vector<std::string> output_rows;
    std::uint64_t replay_signature{0U};
    bool dispatched{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

class IRuntimeScriptExecutionAdapter {
  public:
    virtual ~IRuntimeScriptExecutionAdapter() = default;

    [[nodiscard]] virtual RuntimeScriptAdapterResult
    execute(const RuntimeScriptExecutionRequest& request, const RuntimeScriptSandboxEntrypointPlanRow& entrypoint,
            std::span<const RuntimeScriptSandboxPermissionPlanRow> permissions) = 0;
};

/// Plans reviewed, deterministic script-module sandbox metadata for generated gameplay.
/// This does not interpret scripts, load code, open files, access the network, spawn processes, bind native plugins,
/// mutate package state, expose native handles, or grant unreviewed host APIs.
[[nodiscard]] RuntimeScriptSandboxPlan plan_runtime_script_sandbox(const RuntimeScriptSandboxPolicyDesc& policy);

/// Dispatches one reviewed script entrypoint through a caller-owned adapter.
/// This does not provide a script interpreter, open files, access sockets, spawn processes, bind native plugins,
/// mutate packages, expose native handles, or grant permissions beyond the reviewed sandbox plan rows.
[[nodiscard]] RuntimeScriptExecutionResult
execute_runtime_script_entrypoint(const RuntimeScriptExecutionRequest& request,
                                  IRuntimeScriptExecutionAdapter& adapter);

} // namespace mirakana::runtime
