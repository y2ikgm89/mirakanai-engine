// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/scripting_sandbox.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeScriptSandboxPermissionDesc
make_permission(mirakana::runtime::RuntimeScriptSandboxPermissionKind kind, std::string host_api_id = {},
                std::uint32_t source_index = 0U) {
    return mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
        .kind = kind,
        .host_api_id = std::move(host_api_id),
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeScriptSandboxEntrypointDesc
make_entrypoint(std::string entrypoint_id, mirakana::runtime::RuntimeScriptSandboxEntrypointKind kind,
                std::uint64_t instruction_budget, std::uint64_t memory_budget_bytes, std::uint64_t replay_seed,
                std::vector<mirakana::runtime::RuntimeScriptSandboxPermissionDesc> permissions,
                std::uint32_t source_index) {
    return mirakana::runtime::RuntimeScriptSandboxEntrypointDesc{
        .entrypoint_id = std::move(entrypoint_id),
        .kind = kind,
        .instruction_budget = instruction_budget,
        .memory_budget_bytes = memory_budget_bytes,
        .replay_seed = replay_seed,
        .permissions = std::move(permissions),
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeScriptSandboxModuleDesc
make_module(std::string module_id, std::string source_uri,
            std::vector<mirakana::runtime::RuntimeScriptSandboxEntrypointDesc> entrypoints,
            std::uint32_t source_index) {
    return mirakana::runtime::RuntimeScriptSandboxModuleDesc{
        .module_id = std::move(module_id),
        .source_uri = std::move(source_uri),
        .source_index = source_index,
        .entrypoints = std::move(entrypoints),
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeScriptSandboxPlan& plan,
                                           mirakana::runtime::RuntimeScriptSandboxDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] bool has_allowed_permission(const mirakana::runtime::RuntimeScriptSandboxPlan& plan,
                                          std::string_view module_id, std::string_view entrypoint_id,
                                          mirakana::runtime::RuntimeScriptSandboxPermissionKind kind,
                                          std::string_view host_api_id = {}) {
    for (const auto& row : plan.permissions) {
        if (row.module_id == module_id && row.entrypoint_id == entrypoint_id && row.kind == kind &&
            row.host_api_id == host_api_id && row.allowed) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime scripting sandbox plans reviewed permissions and deterministic rows") {
    using EntryKind = mirakana::runtime::RuntimeScriptSandboxEntrypointKind;
    using Permission = mirakana::runtime::RuntimeScriptSandboxPermissionKind;
    using Status = mirakana::runtime::RuntimeScriptSandboxPlanStatus;

    const auto policy = mirakana::runtime::RuntimeScriptSandboxPolicyDesc{
        .modules =
            {
                make_module("ui", "scripts/ui.mkscript",
                            {make_entrypoint("render_hud", EntryKind::update, 120U, 4096U, 42U,
                                             {
                                                 make_permission(Permission::emit_diagnostic, {}, 3U),
                                                 make_permission(Permission::host_api, "hud.draw", 2U),
                                                 make_permission(Permission::read_runtime_state, {}, 1U),
                                             },
                                             4U)},
                            9U),
                make_module("gameplay", "scripts/gameplay.mkscript",
                            {make_entrypoint("tick", EntryKind::update, 200U, 8192U, 7U,
                                             {
                                                 make_permission(Permission::write_runtime_state, {}, 6U),
                                                 make_permission(Permission::host_api, "gameplay.quest", 5U),
                                                 make_permission(Permission::read_save_profile, {}, 4U),
                                             },
                                             2U)},
                            7U),
            },
        .allowed_host_apis = {"gameplay.quest", "hud.draw"},
        .max_instruction_budget_per_entrypoint = 256U,
        .max_memory_budget_bytes_per_entrypoint = 8192U,
    };

    const auto plan = mirakana::runtime::plan_runtime_script_sandbox(policy);

    MK_REQUIRE(plan.status == Status::planned);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.entrypoints.size() == 2U);
    MK_REQUIRE(plan.entrypoints[0].module_id == "gameplay");
    MK_REQUIRE(plan.entrypoints[0].entrypoint_id == "tick");
    MK_REQUIRE(plan.entrypoints[0].allowed_permission_count == 3U);
    MK_REQUIRE(plan.entrypoints[0].denied_permission_count == 0U);
    MK_REQUIRE(plan.entrypoints[0].replay_seed == 7U);
    MK_REQUIRE(plan.entrypoints[1].module_id == "ui");
    MK_REQUIRE(plan.entrypoints[1].entrypoint_id == "render_hud");
    MK_REQUIRE(plan.projected_instruction_budget == 320U);
    MK_REQUIRE(plan.projected_memory_budget_bytes == 12288U);
    MK_REQUIRE(plan.denied_permission_count == 0U);
    MK_REQUIRE(plan.permissions.size() == 6U);
    MK_REQUIRE(has_allowed_permission(plan, "gameplay", "tick", Permission::host_api, "gameplay.quest"));
    MK_REQUIRE(has_allowed_permission(plan, "gameplay", "tick", Permission::write_runtime_state));
    MK_REQUIRE(has_allowed_permission(plan, "ui", "render_hud", Permission::host_api, "hud.draw"));
    MK_REQUIRE(has_allowed_permission(plan, "ui", "render_hud", Permission::read_runtime_state));
}

MK_TEST("runtime scripting sandbox rejects default denied permissions and unsupported host APIs") {
    using Code = mirakana::runtime::RuntimeScriptSandboxDiagnosticCode;
    using EntryKind = mirakana::runtime::RuntimeScriptSandboxEntrypointKind;
    using Permission = mirakana::runtime::RuntimeScriptSandboxPermissionKind;
    using Status = mirakana::runtime::RuntimeScriptSandboxPlanStatus;

    const auto policy = mirakana::runtime::RuntimeScriptSandboxPolicyDesc{
        .modules =
            {
                make_module("unsafe", "scripts/unsafe.mkscript",
                            {make_entrypoint("main", EntryKind::command, 64U, 1024U, 12U,
                                             {
                                                 make_permission(Permission::filesystem_read, {}, 1U),
                                                 make_permission(Permission::filesystem_write, {}, 2U),
                                                 make_permission(Permission::network, {}, 3U),
                                                 make_permission(Permission::process, {}, 4U),
                                                 make_permission(Permission::native_plugin, {}, 5U),
                                                 make_permission(Permission::host_api, {}, 6U),
                                                 make_permission(Permission::host_api, "unreviewed.service", 7U),
                                             },
                                             8U)},
                            9U),
            },
        .allowed_host_apis = {"reviewed.service"},
        .max_instruction_budget_per_entrypoint = 256U,
        .max_memory_budget_bytes_per_entrypoint = 4096U,
    };

    const auto plan = mirakana::runtime::plan_runtime_script_sandbox(policy);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.entrypoints.empty());
    MK_REQUIRE(plan.permissions.empty());
    MK_REQUIRE(diagnostic_count(plan, Code::default_denied_permission) == 5U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_host_api_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::unsupported_host_api_id) == 1U);
}

MK_TEST("runtime scripting sandbox rejects duplicate ids and missing replay seeds") {
    using Code = mirakana::runtime::RuntimeScriptSandboxDiagnosticCode;
    using EntryKind = mirakana::runtime::RuntimeScriptSandboxEntrypointKind;
    using Permission = mirakana::runtime::RuntimeScriptSandboxPermissionKind;
    using Status = mirakana::runtime::RuntimeScriptSandboxPlanStatus;

    const auto policy =
        mirakana::runtime::RuntimeScriptSandboxPolicyDesc{
            .modules =
                {
                    make_module("", "scripts/missing.mkscript",
                                {make_entrypoint("init", EntryKind::init, 32U, 1024U, 1U,
                                                 {make_permission(Permission::emit_diagnostic)}, 1U)},
                                1U),
                    make_module("dupe", "scripts/a.mkscript",
                                {make_entrypoint("tick", EntryKind::update, 32U, 1024U, 1U,
                                                 {make_permission(Permission::emit_diagnostic)}, 2U)},
                                2U),
                    make_module("dupe", "scripts/b.mkscript",
                                {make_entrypoint("tick", EntryKind::update, 32U, 1024U, 1U,
                                                 {make_permission(Permission::emit_diagnostic)}, 3U)},
                                3U),
                    make_module("valid", "scripts/valid.mkscript",
                                {
                                    make_entrypoint("", EntryKind::event, 32U, 1024U, 1U,
                                                    {make_permission(Permission::emit_diagnostic)}, 4U),
                                    make_entrypoint("event", EntryKind::event, 32U, 1024U, 1U,
                                                    {make_permission(Permission::emit_diagnostic)}, 5U),
                                    make_entrypoint("event", EntryKind::event, 32U, 1024U, 0U,
                                                    {make_permission(Permission::emit_diagnostic)}, 6U),
                                },
                                4U),
                },
            .allowed_host_apis = {},
            .max_instruction_budget_per_entrypoint = 256U,
            .max_memory_budget_bytes_per_entrypoint = 4096U,
        };

    const auto plan = mirakana::runtime::plan_runtime_script_sandbox(policy);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.entrypoints.empty());
    MK_REQUIRE(plan.permissions.empty());
    MK_REQUIRE(diagnostic_count(plan, Code::missing_module_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_module_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_entrypoint_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_entrypoint_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_replay_seed) == 1U);
}

MK_TEST("runtime scripting sandbox reports diagnostics in deterministic order") {
    using Code = mirakana::runtime::RuntimeScriptSandboxDiagnosticCode;
    using EntryKind = mirakana::runtime::RuntimeScriptSandboxEntrypointKind;
    using Permission = mirakana::runtime::RuntimeScriptSandboxPermissionKind;
    using Status = mirakana::runtime::RuntimeScriptSandboxPlanStatus;

    const auto policy = mirakana::runtime::RuntimeScriptSandboxPolicyDesc{
        .modules =
            {
                make_module("z_module", "scripts/z.mkscript",
                            {make_entrypoint("z_entry", EntryKind::command, 16U, 512U, 1U,
                                             {make_permission(Permission::host_api, "z.unreviewed", 9U)}, 8U)},
                            7U),
                make_module("a_module", "scripts/a.mkscript",
                            {make_entrypoint("a_entry", EntryKind::command, 0U, 0U, 0U,
                                             {
                                                 make_permission(Permission::network, {}, 3U),
                                                 make_permission(Permission::host_api, {}, 2U),
                                             },
                                             1U)},
                            2U),
            },
        .allowed_host_apis = {},
        .max_instruction_budget_per_entrypoint = 16U,
        .max_memory_budget_bytes_per_entrypoint = 512U,
    };

    const auto plan = mirakana::runtime::plan_runtime_script_sandbox(policy);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(plan.diagnostics.size() == 6U);
    MK_REQUIRE(plan.diagnostics[0].module_id == "a_module");
    MK_REQUIRE(plan.diagnostics[0].entrypoint_id == "a_entry");
    MK_REQUIRE(plan.diagnostics[0].code == Code::missing_replay_seed);
    MK_REQUIRE(plan.diagnostics[1].module_id == "a_module");
    MK_REQUIRE(plan.diagnostics[1].entrypoint_id == "a_entry");
    MK_REQUIRE(plan.diagnostics[1].code == Code::invalid_instruction_budget);
    MK_REQUIRE(plan.diagnostics[2].module_id == "a_module");
    MK_REQUIRE(plan.diagnostics[2].entrypoint_id == "a_entry");
    MK_REQUIRE(plan.diagnostics[2].code == Code::invalid_memory_budget);
    MK_REQUIRE(plan.diagnostics[3].module_id == "a_module");
    MK_REQUIRE(plan.diagnostics[3].entrypoint_id == "a_entry");
    MK_REQUIRE(plan.diagnostics[3].code == Code::missing_host_api_id);
    MK_REQUIRE(plan.diagnostics[4].module_id == "a_module");
    MK_REQUIRE(plan.diagnostics[4].entrypoint_id == "a_entry");
    MK_REQUIRE(plan.diagnostics[4].code == Code::default_denied_permission);
    MK_REQUIRE(plan.diagnostics[5].module_id == "z_module");
    MK_REQUIRE(plan.diagnostics[5].entrypoint_id == "z_entry");
    MK_REQUIRE(plan.diagnostics[5].code == Code::unsupported_host_api_id);
}

MK_TEST("runtime scripting sandbox rejects zero budgets as invalid request") {
    using Code = mirakana::runtime::RuntimeScriptSandboxDiagnosticCode;
    using EntryKind = mirakana::runtime::RuntimeScriptSandboxEntrypointKind;
    using Permission = mirakana::runtime::RuntimeScriptSandboxPermissionKind;
    using Status = mirakana::runtime::RuntimeScriptSandboxPlanStatus;

    const auto policy = mirakana::runtime::RuntimeScriptSandboxPolicyDesc{
        .modules =
            {
                make_module("gameplay", "scripts/gameplay.mkscript",
                            {make_entrypoint("tick", EntryKind::update, 0U, 0U, 42U,
                                             {make_permission(Permission::emit_diagnostic)}, 2U)},
                            1U),
            },
        .allowed_host_apis = {},
        .max_instruction_budget_per_entrypoint = 256U,
        .max_memory_budget_bytes_per_entrypoint = 8192U,
    };

    const auto plan = mirakana::runtime::plan_runtime_script_sandbox(policy);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.entrypoints.empty());
    MK_REQUIRE(plan.permissions.empty());
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_instruction_budget) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_memory_budget) == 1U);
}

MK_TEST("runtime scripting sandbox rejects per entrypoint budget overflow") {
    using Code = mirakana::runtime::RuntimeScriptSandboxDiagnosticCode;
    using EntryKind = mirakana::runtime::RuntimeScriptSandboxEntrypointKind;
    using Permission = mirakana::runtime::RuntimeScriptSandboxPermissionKind;
    using Status = mirakana::runtime::RuntimeScriptSandboxPlanStatus;

    const auto policy = mirakana::runtime::RuntimeScriptSandboxPolicyDesc{
        .modules =
            {
                make_module("gameplay", "scripts/gameplay.mkscript",
                            {make_entrypoint("tick", EntryKind::update, 257U, 8193U, 42U,
                                             {make_permission(Permission::emit_diagnostic)}, 2U)},
                            1U),
            },
        .allowed_host_apis = {},
        .max_instruction_budget_per_entrypoint = 256U,
        .max_memory_budget_bytes_per_entrypoint = 8192U,
    };

    const auto plan = mirakana::runtime::plan_runtime_script_sandbox(policy);

    MK_REQUIRE(plan.status == Status::budget_exceeded);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.entrypoints.empty());
    MK_REQUIRE(plan.permissions.empty());
    MK_REQUIRE(plan.projected_instruction_budget == 257U);
    MK_REQUIRE(plan.projected_memory_budget_bytes == 8193U);
    MK_REQUIRE(diagnostic_count(plan, Code::instruction_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::memory_budget_exceeded) == 1U);
}

MK_TEST("runtime scripting sandbox rejects aggregate budget overflow") {
    using Code = mirakana::runtime::RuntimeScriptSandboxDiagnosticCode;
    using EntryKind = mirakana::runtime::RuntimeScriptSandboxEntrypointKind;
    using Permission = mirakana::runtime::RuntimeScriptSandboxPermissionKind;
    using Status = mirakana::runtime::RuntimeScriptSandboxPlanStatus;

    const auto max_budget = std::numeric_limits<std::uint64_t>::max();
    const auto policy = mirakana::runtime::RuntimeScriptSandboxPolicyDesc{
        .modules =
            {
                make_module("a", "scripts/a.mkscript",
                            {make_entrypoint("tick", EntryKind::update, max_budget, max_budget, 1U,
                                             {make_permission(Permission::emit_diagnostic)}, 1U)},
                            1U),
                make_module("b", "scripts/b.mkscript",
                            {make_entrypoint("tick", EntryKind::update, 1U, 1U, 2U,
                                             {make_permission(Permission::emit_diagnostic)}, 2U)},
                            2U),
            },
        .allowed_host_apis = {},
        .max_instruction_budget_per_entrypoint = 0U,
        .max_memory_budget_bytes_per_entrypoint = 0U,
    };

    const auto plan = mirakana::runtime::plan_runtime_script_sandbox(policy);

    MK_REQUIRE(plan.status == Status::budget_exceeded);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.entrypoints.empty());
    MK_REQUIRE(plan.permissions.empty());
    MK_REQUIRE(plan.projected_instruction_budget == max_budget);
    MK_REQUIRE(plan.projected_memory_budget_bytes == max_budget);
    MK_REQUIRE(diagnostic_count(plan, Code::instruction_budget_exceeded) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::memory_budget_exceeded) == 1U);
}

MK_TEST("runtime scripting sandbox treats empty module set as no modules") {
    using Status = mirakana::runtime::RuntimeScriptSandboxPlanStatus;

    const auto plan =
        mirakana::runtime::plan_runtime_script_sandbox(mirakana::runtime::RuntimeScriptSandboxPolicyDesc{});

    MK_REQUIRE(plan.status == Status::no_modules);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.entrypoints.empty());
    MK_REQUIRE(plan.permissions.empty());
    MK_REQUIRE(plan.diagnostics.empty());
}

int main() {
    return mirakana::test::run_all();
}
