// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/render_backend.hpp"

#include <cstddef>
#include <iterator>
#include <ranges>
#include <span>

namespace mirakana::editor {
namespace {

[[nodiscard]] EditorRenderBackend choose_first_available(EditorRenderBackendAvailability availability,
                                                         std::span<const EditorRenderBackend> order) noexcept {
    const auto found = std::ranges::find_if(order, [availability](EditorRenderBackend candidate) {
        return is_editor_render_backend_available(candidate, availability);
    });
    return found == order.end() ? EditorRenderBackend::null : *found;
}

[[nodiscard]] EditorRenderBackend choose_automatic_backend(EditorRenderBackendAvailability availability,
                                                           EditorRenderBackendHost host) noexcept {
    constexpr EditorRenderBackend windows_order[] = {
        EditorRenderBackend::d3d12,
        EditorRenderBackend::vulkan,
        EditorRenderBackend::metal,
    };
    constexpr EditorRenderBackend linux_order[] = {
        EditorRenderBackend::vulkan,
        EditorRenderBackend::d3d12,
        EditorRenderBackend::metal,
    };
    constexpr EditorRenderBackend apple_order[] = {
        EditorRenderBackend::metal,
        EditorRenderBackend::vulkan,
        EditorRenderBackend::d3d12,
    };

    switch (host) {
    case EditorRenderBackendHost::windows:
        return choose_first_available(availability, windows_order);
    case EditorRenderBackendHost::linux:
    case EditorRenderBackendHost::android:
    case EditorRenderBackendHost::unknown:
        return choose_first_available(availability, linux_order);
    case EditorRenderBackendHost::apple:
        return choose_first_available(availability, apple_order);
    }
    return EditorRenderBackend::null;
}

[[nodiscard]] std::string_view automatic_diagnostic(EditorRenderBackend active) noexcept {
    switch (active) {
    case EditorRenderBackend::d3d12:
        return "Automatic selected Direct3D 12 for this host";
    case EditorRenderBackend::vulkan:
        return "Automatic selected Vulkan for this host";
    case EditorRenderBackend::metal:
        return "Automatic selected Metal for this host";
    case EditorRenderBackend::null:
        return "No native viewport backend available; using NullRhiDevice";
    case EditorRenderBackend::automatic:
        break;
    }
    return "No native viewport backend available; using NullRhiDevice";
}

} // namespace

std::string_view editor_render_backend_id(EditorRenderBackend backend) noexcept {
    switch (backend) {
    case EditorRenderBackend::automatic:
        return "auto";
    case EditorRenderBackend::null:
        return "null";
    case EditorRenderBackend::d3d12:
        return "d3d12";
    case EditorRenderBackend::vulkan:
        return "vulkan";
    case EditorRenderBackend::metal:
        return "metal";
    }
    return "unknown";
}

std::string_view editor_render_backend_label(EditorRenderBackend backend) noexcept {
    switch (backend) {
    case EditorRenderBackend::automatic:
        return "Automatic";
    case EditorRenderBackend::null:
        return "Null";
    case EditorRenderBackend::d3d12:
        return "Direct3D 12";
    case EditorRenderBackend::vulkan:
        return "Vulkan";
    case EditorRenderBackend::metal:
        return "Metal";
    }
    return "Unknown";
}

std::optional<EditorRenderBackend> parse_editor_render_backend(std::string_view id) noexcept {
    if (id == "auto") {
        return EditorRenderBackend::automatic;
    }
    if (id == "null") {
        return EditorRenderBackend::null;
    }
    if (id == "d3d12") {
        return EditorRenderBackend::d3d12;
    }
    if (id == "vulkan") {
        return EditorRenderBackend::vulkan;
    }
    if (id == "metal") {
        return EditorRenderBackend::metal;
    }
    return std::nullopt;
}

EditorRenderBackendHost current_editor_render_backend_host() noexcept {
#if defined(_WIN32)
    return EditorRenderBackendHost::windows;
#elif defined(__APPLE__)
    return EditorRenderBackendHost::apple;
#elif defined(__ANDROID__)
    return EditorRenderBackendHost::android;
#elif defined(__linux__)
    return EditorRenderBackendHost::linux;
#else
    return EditorRenderBackendHost::unknown;
#endif
}

bool is_editor_render_backend_available(EditorRenderBackend backend,
                                        EditorRenderBackendAvailability availability) noexcept {
    switch (backend) {
    case EditorRenderBackend::automatic:
        return true;
    case EditorRenderBackend::null:
        return true;
    case EditorRenderBackend::d3d12:
        return availability.d3d12;
    case EditorRenderBackend::vulkan:
        return availability.vulkan;
    case EditorRenderBackend::metal:
        return availability.metal;
    }
    return false;
}

std::array<EditorRenderBackendDescriptor, 4>
make_editor_render_backend_descriptors(EditorRenderBackendAvailability availability) noexcept {
    return {
        EditorRenderBackendDescriptor{
            .backend = EditorRenderBackend::null,
            .id = "null",
            .label = "Null",
            .available = true,
            .diagnostic = "Headless validation backend",
        },
        EditorRenderBackendDescriptor{
            .backend = EditorRenderBackend::d3d12,
            .id = "d3d12",
            .label = "Direct3D 12",
            .available = availability.d3d12,
            .diagnostic =
                availability.d3d12 ? "Available" : "D3D12 viewport bridge is not enabled in this editor build",
        },
        EditorRenderBackendDescriptor{
            .backend = EditorRenderBackend::vulkan,
            .id = "vulkan",
            .label = "Vulkan",
            .available = availability.vulkan,
            .diagnostic = availability.vulkan ? "Available" : "Vulkan backend is not enabled in this editor build",
        },
        EditorRenderBackendDescriptor{
            .backend = EditorRenderBackend::metal,
            .id = "metal",
            .label = "Metal",
            .available = availability.metal,
            .diagnostic = availability.metal ? "Available" : "Metal backend is not enabled in this editor build",
        },
    };
}

EditorRenderBackendChoice choose_editor_render_backend(EditorRenderBackend requested,
                                                       EditorRenderBackendAvailability availability,
                                                       EditorRenderBackendHost host) noexcept {
    if (requested == EditorRenderBackend::automatic) {
        const auto active = choose_automatic_backend(availability, host);
        return EditorRenderBackendChoice{
            .requested = requested,
            .active = active,
            .exact_match = true,
            .diagnostic = automatic_diagnostic(active),
        };
    }

    if (is_editor_render_backend_available(requested, availability)) {
        return EditorRenderBackendChoice{
            .requested = requested,
            .active = requested,
            .exact_match = true,
            .diagnostic = "Requested backend available",
        };
    }

    return EditorRenderBackendChoice{
        .requested = requested,
        .active = EditorRenderBackend::null,
        .exact_match = false,
        .diagnostic = "Requested backend unavailable; using NullRhiDevice",
    };
}

} // namespace mirakana::editor
