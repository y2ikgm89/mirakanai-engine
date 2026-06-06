// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

struct EditorCrossPlatformShellAdapterRow {
    std::string id;
    std::string platform;
    std::string capability;
    std::string status;
    bool first_party{true};
    bool host_gated{true};
    bool unsupported{false};
    bool native_handles_public{false};
};

struct EditorCrossPlatformShellAdapterDesc {
    bool native_handles_exposed{false};
};

struct EditorCrossPlatformShellAdapterPlan {
    std::string cross_platform_status{"host_gated"};
    std::string macos_status{"host_gated"};
    std::string linux_status{"host_gated"};
    std::string android_status{"unsupported"};
    std::string ios_status{"unsupported"};
    std::uint32_t core_contract_rows{0};
    std::uint32_t macos_adapter_rows{0};
    std::uint32_t linux_adapter_rows{0};
    std::uint32_t native_handle_blocked_rows{0};
    bool native_handles_exposed{false};
    std::vector<EditorCrossPlatformShellAdapterRow> rows;
};

[[nodiscard]] EditorCrossPlatformShellAdapterPlan
make_editor_cross_platform_shell_adapter_plan(EditorCrossPlatformShellAdapterDesc desc = {});

[[nodiscard]] const EditorCrossPlatformShellAdapterRow*
find_editor_cross_platform_shell_adapter_row(std::span<const EditorCrossPlatformShellAdapterRow> rows,
                                             std::string_view id) noexcept;

} // namespace mirakana::editor
