// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/shader_bytecode.hpp"

#include <exception>
#include <string>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool has_required_text(std::string_view text) noexcept {
    return !text.empty();
}

[[nodiscard]] std::vector<std::uint8_t> to_bytes(const std::string& text) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(text.size());
    for (const char value : text) {
        bytes.push_back(static_cast<std::uint8_t>(value));
    }
    return bytes;
}

[[nodiscard]] DesktopShaderBytecodeLoadResult make_result(DesktopShaderBytecodeLoadStatus status,
                                                          std::string diagnostic) {
    return DesktopShaderBytecodeLoadResult{
        .status = status,
        .vertex_shader = {},
        .fragment_shader = {},
        .diagnostic = std::move(diagnostic),
    };
}

[[nodiscard]] bool read_bytecode_text(IFileSystem& filesystem, const std::string& path, std::string_view stage_name,
                                      std::string& output, DesktopShaderBytecodeLoadResult& failure) {
    try {
        output = filesystem.read_text(path);
        return true;
    } catch (const std::exception& error) {
        failure = make_result(DesktopShaderBytecodeLoadStatus::read_failed,
                              "desktop " + std::string{stage_name} + " shader bytecode read failed: " + path + ": " +
                                  error.what());
        return false;
    } catch (...) {
        failure = make_result(DesktopShaderBytecodeLoadStatus::read_failed,
                              "desktop " + std::string{stage_name} + " shader bytecode read failed: " + path +
                                  ": unknown error");
        return false;
    }
}

} // namespace

bool DesktopShaderBytecodeLoadResult::ready() const noexcept {
    return status == DesktopShaderBytecodeLoadStatus::ready;
}

DesktopShaderBytecodeLoadResult load_desktop_shader_bytecode_pair(const DesktopShaderBytecodeLoadDesc& desc) {
    if (desc.filesystem == nullptr) {
        return make_result(DesktopShaderBytecodeLoadStatus::invalid_request,
                           "desktop shader bytecode load requires a filesystem");
    }
    if (!has_required_text(desc.vertex_path) || !has_required_text(desc.fragment_path)) {
        return make_result(DesktopShaderBytecodeLoadStatus::invalid_request,
                           "desktop shader bytecode paths must not be empty");
    }
    if (!has_required_text(desc.vertex_entry_point) || !has_required_text(desc.fragment_entry_point)) {
        return make_result(DesktopShaderBytecodeLoadStatus::invalid_request,
                           "desktop shader bytecode entry points must not be empty");
    }

    if (!desc.filesystem->exists(desc.vertex_path)) {
        return make_result(DesktopShaderBytecodeLoadStatus::missing,
                           "desktop vertex shader bytecode is missing: " + desc.vertex_path);
    }
    if (!desc.filesystem->exists(desc.fragment_path)) {
        return make_result(DesktopShaderBytecodeLoadStatus::missing,
                           "desktop fragment shader bytecode is missing: " + desc.fragment_path);
    }

    std::string vertex;
    std::string fragment;
    DesktopShaderBytecodeLoadResult read_failure;
    if (!read_bytecode_text(*desc.filesystem, desc.vertex_path, "vertex", vertex, read_failure)) {
        return read_failure;
    }
    if (!read_bytecode_text(*desc.filesystem, desc.fragment_path, "fragment", fragment, read_failure)) {
        return read_failure;
    }
    if (vertex.empty()) {
        return make_result(DesktopShaderBytecodeLoadStatus::empty,
                           "desktop vertex shader bytecode is empty: " + desc.vertex_path);
    }
    if (fragment.empty()) {
        return make_result(DesktopShaderBytecodeLoadStatus::empty,
                           "desktop fragment shader bytecode is empty: " + desc.fragment_path);
    }

    return DesktopShaderBytecodeLoadResult{
        .status = DesktopShaderBytecodeLoadStatus::ready,
        .vertex_shader =
            DesktopShaderBytecodeBlob{.entry_point = desc.vertex_entry_point, .bytecode = to_bytes(vertex)},
        .fragment_shader =
            DesktopShaderBytecodeBlob{.entry_point = desc.fragment_entry_point, .bytecode = to_bytes(fragment)},
        .diagnostic = {},
    };
}

std::string_view desktop_shader_bytecode_load_status_name(DesktopShaderBytecodeLoadStatus status) noexcept {
    switch (status) {
    case DesktopShaderBytecodeLoadStatus::ready:
        return "ready";
    case DesktopShaderBytecodeLoadStatus::missing:
        return "missing";
    case DesktopShaderBytecodeLoadStatus::empty:
        return "empty";
    case DesktopShaderBytecodeLoadStatus::read_failed:
        return "read_failed";
    case DesktopShaderBytecodeLoadStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

} // namespace mirakana
