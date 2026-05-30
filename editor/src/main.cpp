// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_app.hpp"
#include "native_editor_launch.hpp"

#include <iostream>
#include <string_view>

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
    return app.run();
}
