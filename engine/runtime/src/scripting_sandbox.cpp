// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/scripting_sandbox.hpp"

#include <algorithm>
#include <limits>
#include <string_view>
#include <utility>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) {
    return std::ranges::any_of(ids, [id](const std::string& candidate) { return candidate == id; });
}

[[nodiscard]] bool is_default_denied_permission(RuntimeScriptSandboxPermissionKind kind) {
    switch (kind) {
    case RuntimeScriptSandboxPermissionKind::host_api:
    case RuntimeScriptSandboxPermissionKind::read_runtime_state:
    case RuntimeScriptSandboxPermissionKind::write_runtime_state:
    case RuntimeScriptSandboxPermissionKind::emit_diagnostic:
    case RuntimeScriptSandboxPermissionKind::read_save_profile:
    case RuntimeScriptSandboxPermissionKind::write_save_profile:
        return false;
    case RuntimeScriptSandboxPermissionKind::filesystem_read:
    case RuntimeScriptSandboxPermissionKind::filesystem_write:
    case RuntimeScriptSandboxPermissionKind::network:
    case RuntimeScriptSandboxPermissionKind::process:
    case RuntimeScriptSandboxPermissionKind::native_plugin:
        return true;
    }
    return true;
}

[[nodiscard]] bool is_host_api_allowed(const RuntimeScriptSandboxPolicyDesc& policy, std::string_view host_api_id) {
    return contains_id(policy.allowed_host_apis, host_api_id);
}

void add_diagnostic(RuntimeScriptSandboxPlan& plan, RuntimeScriptSandboxDiagnosticCode code, std::string module_id,
                    std::string entrypoint_id, RuntimeScriptSandboxPermissionKind permission_kind,
                    std::string host_api_id, std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(RuntimeScriptSandboxDiagnostic{
        .code = code,
        .module_id = std::move(module_id),
        .entrypoint_id = std::move(entrypoint_id),
        .permission_kind = permission_kind,
        .host_api_id = std::move(host_api_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

[[nodiscard]] bool is_budget_diagnostic(RuntimeScriptSandboxDiagnosticCode code) {
    switch (code) {
    case RuntimeScriptSandboxDiagnosticCode::instruction_budget_exceeded:
    case RuntimeScriptSandboxDiagnosticCode::memory_budget_exceeded:
        return true;
    case RuntimeScriptSandboxDiagnosticCode::missing_module_id:
    case RuntimeScriptSandboxDiagnosticCode::duplicate_module_id:
    case RuntimeScriptSandboxDiagnosticCode::missing_entrypoint_id:
    case RuntimeScriptSandboxDiagnosticCode::duplicate_entrypoint_id:
    case RuntimeScriptSandboxDiagnosticCode::missing_replay_seed:
    case RuntimeScriptSandboxDiagnosticCode::invalid_instruction_budget:
    case RuntimeScriptSandboxDiagnosticCode::invalid_memory_budget:
    case RuntimeScriptSandboxDiagnosticCode::default_denied_permission:
    case RuntimeScriptSandboxDiagnosticCode::missing_host_api_id:
    case RuntimeScriptSandboxDiagnosticCode::unsupported_host_api_id:
        return false;
    }
    return false;
}

[[nodiscard]] bool has_only_budget_diagnostics(const RuntimeScriptSandboxPlan& plan) {
    return !plan.diagnostics.empty() && std::ranges::all_of(plan.diagnostics, [](const auto& diagnostic) {
        return is_budget_diagnostic(diagnostic.code);
    });
}

void add_budget(RuntimeScriptSandboxPlan& plan, RuntimeScriptSandboxDiagnosticCode overflow_code, std::uint64_t& total,
                std::uint64_t value, std::string module_id, std::string entrypoint_id, std::string message,
                std::uint32_t source_index) {
    constexpr auto max_value = std::numeric_limits<std::uint64_t>::max();
    if (max_value - total < value) {
        total = max_value;
        add_diagnostic(plan, overflow_code, std::move(module_id), std::move(entrypoint_id),
                       RuntimeScriptSandboxPermissionKind::emit_diagnostic, {}, std::move(message), source_index);
        return;
    }
    total += value;
}

void validate_permission(RuntimeScriptSandboxPlan& plan, const RuntimeScriptSandboxPolicyDesc& policy,
                         const RuntimeScriptSandboxModuleDesc& module,
                         const RuntimeScriptSandboxEntrypointDesc& entrypoint,
                         const RuntimeScriptSandboxPermissionDesc& permission) {
    if (is_default_denied_permission(permission.kind)) {
        add_diagnostic(plan, RuntimeScriptSandboxDiagnosticCode::default_denied_permission, module.module_id,
                       entrypoint.entrypoint_id, permission.kind, permission.host_api_id,
                       "script sandbox permission is denied by default and requires a future reviewed adapter",
                       permission.source_index);
        return;
    }

    if (permission.kind != RuntimeScriptSandboxPermissionKind::host_api) {
        return;
    }

    if (permission.host_api_id.empty()) {
        add_diagnostic(plan, RuntimeScriptSandboxDiagnosticCode::missing_host_api_id, module.module_id,
                       entrypoint.entrypoint_id, permission.kind, {}, "script sandbox host API permission needs an id",
                       permission.source_index);
        return;
    }
    if (!is_host_api_allowed(policy, permission.host_api_id)) {
        add_diagnostic(plan, RuntimeScriptSandboxDiagnosticCode::unsupported_host_api_id, module.module_id,
                       entrypoint.entrypoint_id, permission.kind, permission.host_api_id,
                       "script sandbox host API permission is not in the reviewed allow-list", permission.source_index);
    }
}

void validate_policy(RuntimeScriptSandboxPlan& plan, const RuntimeScriptSandboxPolicyDesc& policy) {
    std::vector<std::string> module_ids;
    module_ids.reserve(policy.modules.size());

    for (const auto& module : policy.modules) {
        if (!is_valid_id(module.module_id)) {
            add_diagnostic(plan, RuntimeScriptSandboxDiagnosticCode::missing_module_id, module.module_id, {},
                           RuntimeScriptSandboxPermissionKind::emit_diagnostic, {},
                           "script sandbox module id must be non-empty and path-safe", module.source_index);
        } else if (contains_id(module_ids, module.module_id)) {
            add_diagnostic(plan, RuntimeScriptSandboxDiagnosticCode::duplicate_module_id, module.module_id, {},
                           RuntimeScriptSandboxPermissionKind::emit_diagnostic, {},
                           "script sandbox module ids must be unique", module.source_index);
        } else {
            module_ids.push_back(module.module_id);
        }

        std::vector<std::string> entrypoint_ids;
        entrypoint_ids.reserve(module.entrypoints.size());
        for (const auto& entrypoint : module.entrypoints) {
            add_budget(plan, RuntimeScriptSandboxDiagnosticCode::instruction_budget_exceeded,
                       plan.projected_instruction_budget, entrypoint.instruction_budget, module.module_id,
                       entrypoint.entrypoint_id, "script sandbox aggregate instruction budget overflows uint64_t",
                       entrypoint.source_index);
            add_budget(plan, RuntimeScriptSandboxDiagnosticCode::memory_budget_exceeded,
                       plan.projected_memory_budget_bytes, entrypoint.memory_budget_bytes, module.module_id,
                       entrypoint.entrypoint_id, "script sandbox aggregate memory budget overflows uint64_t",
                       entrypoint.source_index);

            if (!is_valid_id(entrypoint.entrypoint_id)) {
                add_diagnostic(plan, RuntimeScriptSandboxDiagnosticCode::missing_entrypoint_id, module.module_id,
                               entrypoint.entrypoint_id, RuntimeScriptSandboxPermissionKind::emit_diagnostic, {},
                               "script sandbox entrypoint id must be non-empty and path-safe", entrypoint.source_index);
            } else if (contains_id(entrypoint_ids, entrypoint.entrypoint_id)) {
                add_diagnostic(plan, RuntimeScriptSandboxDiagnosticCode::duplicate_entrypoint_id, module.module_id,
                               entrypoint.entrypoint_id, RuntimeScriptSandboxPermissionKind::emit_diagnostic, {},
                               "script sandbox entrypoint ids must be unique per module", entrypoint.source_index);
            } else {
                entrypoint_ids.push_back(entrypoint.entrypoint_id);
            }

            if (entrypoint.replay_seed == 0U) {
                add_diagnostic(plan, RuntimeScriptSandboxDiagnosticCode::missing_replay_seed, module.module_id,
                               entrypoint.entrypoint_id, RuntimeScriptSandboxPermissionKind::emit_diagnostic, {},
                               "script sandbox entrypoints require an explicit non-zero replay seed",
                               entrypoint.source_index);
            }
            if (entrypoint.instruction_budget == 0U) {
                add_diagnostic(plan, RuntimeScriptSandboxDiagnosticCode::invalid_instruction_budget, module.module_id,
                               entrypoint.entrypoint_id, RuntimeScriptSandboxPermissionKind::emit_diagnostic, {},
                               "script sandbox instruction budget must be non-zero", entrypoint.source_index);
            } else if (policy.max_instruction_budget_per_entrypoint > 0U &&
                       entrypoint.instruction_budget > policy.max_instruction_budget_per_entrypoint) {
                add_diagnostic(plan, RuntimeScriptSandboxDiagnosticCode::instruction_budget_exceeded, module.module_id,
                               entrypoint.entrypoint_id, RuntimeScriptSandboxPermissionKind::emit_diagnostic, {},
                               "script sandbox instruction budget exceeds the reviewed per-entrypoint cap",
                               entrypoint.source_index);
            }
            if (entrypoint.memory_budget_bytes == 0U) {
                add_diagnostic(plan, RuntimeScriptSandboxDiagnosticCode::invalid_memory_budget, module.module_id,
                               entrypoint.entrypoint_id, RuntimeScriptSandboxPermissionKind::emit_diagnostic, {},
                               "script sandbox memory budget must be non-zero", entrypoint.source_index);
            } else if (policy.max_memory_budget_bytes_per_entrypoint > 0U &&
                       entrypoint.memory_budget_bytes > policy.max_memory_budget_bytes_per_entrypoint) {
                add_diagnostic(plan, RuntimeScriptSandboxDiagnosticCode::memory_budget_exceeded, module.module_id,
                               entrypoint.entrypoint_id, RuntimeScriptSandboxPermissionKind::emit_diagnostic, {},
                               "script sandbox memory budget exceeds the reviewed per-entrypoint cap",
                               entrypoint.source_index);
            }

            for (const auto& permission : entrypoint.permissions) {
                validate_permission(plan, policy, module, entrypoint, permission);
            }
        }
    }
}

[[nodiscard]] bool permission_allowed(const RuntimeScriptSandboxPolicyDesc& policy,
                                      const RuntimeScriptSandboxPermissionDesc& permission) {
    if (is_default_denied_permission(permission.kind)) {
        return false;
    }
    return permission.kind != RuntimeScriptSandboxPermissionKind::host_api ||
           is_host_api_allowed(policy, permission.host_api_id);
}

void append_entrypoint_row(RuntimeScriptSandboxPlan& plan, const RuntimeScriptSandboxPolicyDesc& policy,
                           const RuntimeScriptSandboxModuleDesc& module,
                           const RuntimeScriptSandboxEntrypointDesc& entrypoint) {
    std::size_t allowed_permissions{0U};
    std::size_t denied_permissions{0U};
    for (const auto& permission : entrypoint.permissions) {
        if (permission_allowed(policy, permission)) {
            ++allowed_permissions;
        } else {
            ++denied_permissions;
        }
    }

    plan.denied_permission_count += denied_permissions;
    plan.entrypoints.push_back(RuntimeScriptSandboxEntrypointPlanRow{
        .module_id = module.module_id,
        .source_uri = module.source_uri,
        .entrypoint_id = entrypoint.entrypoint_id,
        .kind = entrypoint.kind,
        .instruction_budget = entrypoint.instruction_budget,
        .memory_budget_bytes = entrypoint.memory_budget_bytes,
        .replay_seed = entrypoint.replay_seed,
        .allowed_permission_count = allowed_permissions,
        .denied_permission_count = denied_permissions,
        .source_index = entrypoint.source_index,
    });
}

void append_permission_rows(RuntimeScriptSandboxPlan& plan, const RuntimeScriptSandboxPolicyDesc& policy,
                            const RuntimeScriptSandboxModuleDesc& module,
                            const RuntimeScriptSandboxEntrypointDesc& entrypoint) {
    for (const auto& permission : entrypoint.permissions) {
        plan.permissions.push_back(RuntimeScriptSandboxPermissionPlanRow{
            .module_id = module.module_id,
            .entrypoint_id = entrypoint.entrypoint_id,
            .kind = permission.kind,
            .host_api_id = permission.host_api_id,
            .allowed = permission_allowed(policy, permission),
            .source_index = permission.source_index,
        });
    }
}

void sort_rows(RuntimeScriptSandboxPlan& plan) {
    std::ranges::sort(plan.entrypoints, [](const auto& lhs, const auto& rhs) {
        if (lhs.module_id != rhs.module_id) {
            return lhs.module_id < rhs.module_id;
        }
        if (lhs.entrypoint_id != rhs.entrypoint_id) {
            return lhs.entrypoint_id < rhs.entrypoint_id;
        }
        return lhs.source_index < rhs.source_index;
    });

    std::ranges::sort(plan.permissions, [](const auto& lhs, const auto& rhs) {
        if (lhs.module_id != rhs.module_id) {
            return lhs.module_id < rhs.module_id;
        }
        if (lhs.entrypoint_id != rhs.entrypoint_id) {
            return lhs.entrypoint_id < rhs.entrypoint_id;
        }
        if (lhs.kind != rhs.kind) {
            return static_cast<std::uint8_t>(lhs.kind) < static_cast<std::uint8_t>(rhs.kind);
        }
        if (lhs.host_api_id != rhs.host_api_id) {
            return lhs.host_api_id < rhs.host_api_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void sort_diagnostics(RuntimeScriptSandboxPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.module_id != rhs.module_id) {
            return lhs.module_id < rhs.module_id;
        }
        if (lhs.entrypoint_id != rhs.entrypoint_id) {
            return lhs.entrypoint_id < rhs.entrypoint_id;
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.permission_kind != rhs.permission_kind) {
            return static_cast<std::uint8_t>(lhs.permission_kind) < static_cast<std::uint8_t>(rhs.permission_kind);
        }
        return lhs.host_api_id < rhs.host_api_id;
    });
}

} // namespace

bool RuntimeScriptSandboxPlan::succeeded() const noexcept {
    return status == RuntimeScriptSandboxPlanStatus::planned || status == RuntimeScriptSandboxPlanStatus::no_modules;
}

RuntimeScriptSandboxPlan plan_runtime_script_sandbox(const RuntimeScriptSandboxPolicyDesc& policy) {
    RuntimeScriptSandboxPlan plan;

    validate_policy(plan, policy);
    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan);
        plan.status = has_only_budget_diagnostics(plan) ? RuntimeScriptSandboxPlanStatus::budget_exceeded
                                                        : RuntimeScriptSandboxPlanStatus::invalid_request;
        return plan;
    }

    for (const auto& module : policy.modules) {
        for (const auto& entrypoint : module.entrypoints) {
            append_entrypoint_row(plan, policy, module, entrypoint);
            append_permission_rows(plan, policy, module, entrypoint);
        }
    }
    sort_rows(plan);

    plan.status =
        policy.modules.empty() ? RuntimeScriptSandboxPlanStatus::no_modules : RuntimeScriptSandboxPlanStatus::planned;
    return plan;
}

} // namespace mirakana::runtime
