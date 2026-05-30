// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_app.hpp"
#include "native_editor_launch.hpp"
#include "win32_imgui_d3d12_host.hpp"

#include <iostream>
#include <string_view>

namespace {

[[nodiscard]] std::string_view adapter_name(mirakana::editor::Win32ImguiD3d12AdapterKind adapter) noexcept {
    switch (adapter) {
    case mirakana::editor::Win32ImguiD3d12AdapterKind::hardware:
        return "hardware";
    case mirakana::editor::Win32ImguiD3d12AdapterKind::warp:
        return "warp";
    case mirakana::editor::Win32ImguiD3d12AdapterKind::none:
        break;
    }
    return "none";
}

} // namespace

int main(int argc, char** argv) {
    const auto launch = mirakana::editor::parse_native_editor_launch(argc, argv);
    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    if (!validation.valid) {
        std::cerr << validation.diagnostic << '\n'
                  << mirakana::editor::native_editor_launch_usage(argc > 0 && argv != nullptr && argv[0] != nullptr
                                                                      ? std::string_view{argv[0]}
                                                                      : std::string_view{"MK_editor"})
                  << '\n';
        return mirakana::editor::native_editor_launch_usage_error_exit_code();
    }

    mirakana::editor::NativeEditorApp app{launch.options};
    mirakana::editor::Win32ImguiD3d12Host host{mirakana::editor::Win32ImguiD3d12HostDesc{.launch = launch.options}};
    const auto result = host.run(app);
    if (launch.options.smoke_frames > 0) {
        std::cout << (result.succeeded ? "editor_shell_status=ready" : "editor_shell_status=failed") << '\n'
                  << "editor_shell_backend=d3d12\n"
                  << "editor_shell_sdl3=0\n"
                  << "editor_shell_frames=" << result.frames_rendered << '\n'
                  << "editor_shell_panels=" << app.panels_rendered_last_frame() << '\n'
                  << "editor_shell_resizes=" << result.resize_count << '\n'
                  << "editor_shell_adapter=" << adapter_name(result.adapter_kind) << '\n';
    }
    if (!result.succeeded && !result.diagnostic.empty()) {
        std::cerr << result.diagnostic << '\n';
    }
    return result.exit_code;
}
