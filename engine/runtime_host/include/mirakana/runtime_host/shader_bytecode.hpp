// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/filesystem.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class DesktopShaderBytecodeLoadStatus : std::uint8_t {
    ready = 0,
    missing,
    empty,
    read_failed,
    invalid_request,
};

struct DesktopShaderBytecodeBlob {
    std::string entry_point;
    std::vector<std::uint8_t> bytecode;
};

struct DesktopShaderBytecodeLoadDesc {
    IFileSystem* filesystem{nullptr};
    std::string vertex_path;
    std::string fragment_path;
    std::string vertex_entry_point{"vs_main"};
    std::string fragment_entry_point{"ps_main"};
};

struct DesktopShaderBytecodeLoadResult {
    DesktopShaderBytecodeLoadStatus status{DesktopShaderBytecodeLoadStatus::invalid_request};
    DesktopShaderBytecodeBlob vertex_shader;
    DesktopShaderBytecodeBlob fragment_shader;
    std::string diagnostic;

    [[nodiscard]] bool ready() const noexcept;
};

[[nodiscard]] DesktopShaderBytecodeLoadResult
load_desktop_shader_bytecode_pair(const DesktopShaderBytecodeLoadDesc& desc);
[[nodiscard]] std::string_view
desktop_shader_bytecode_load_status_name(DesktopShaderBytecodeLoadStatus status) noexcept;

} // namespace mirakana
