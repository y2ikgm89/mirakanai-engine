// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace mirakana::editor {

enum class EditorRenderBackend : std::uint8_t {
    automatic,
    null,
    d3d12,
    vulkan,
    metal,
};

enum class EditorRenderBackendHost : std::uint8_t {
    windows,
    linux,
    android,
    apple,
    unknown,
};

struct EditorRenderBackendAvailability {
    bool d3d12{false};
    bool vulkan{false};
    bool metal{false};
};

struct EditorRenderBackendDescriptor {
    EditorRenderBackend backend{EditorRenderBackend::null};
    std::string_view id;
    std::string_view label;
    bool available{false};
    std::string_view diagnostic;
};

struct EditorRenderBackendChoice {
    EditorRenderBackend requested{EditorRenderBackend::automatic};
    EditorRenderBackend active{EditorRenderBackend::null};
    bool exact_match{false};
    std::string_view diagnostic;
};

[[nodiscard]] std::string_view editor_render_backend_id(EditorRenderBackend backend) noexcept;
[[nodiscard]] std::string_view editor_render_backend_label(EditorRenderBackend backend) noexcept;
[[nodiscard]] std::optional<EditorRenderBackend> parse_editor_render_backend(std::string_view id) noexcept;
[[nodiscard]] EditorRenderBackendHost current_editor_render_backend_host() noexcept;
[[nodiscard]] bool is_editor_render_backend_available(EditorRenderBackend backend,
                                                      EditorRenderBackendAvailability availability) noexcept;
[[nodiscard]] std::array<EditorRenderBackendDescriptor, 4>
make_editor_render_backend_descriptors(EditorRenderBackendAvailability availability) noexcept;
[[nodiscard]] EditorRenderBackendChoice
choose_editor_render_backend(EditorRenderBackend requested, EditorRenderBackendAvailability availability,
                             EditorRenderBackendHost host = current_editor_render_backend_host()) noexcept;

} // namespace mirakana::editor
