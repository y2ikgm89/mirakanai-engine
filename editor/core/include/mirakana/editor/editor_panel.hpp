// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>

namespace mirakana::editor {

enum class PanelId : std::uint8_t {
    scene = 0,
    inspector,
    assets,
    console,
    viewport,
    resources,
    ai_commands,
    input_rebinding,
    profiler,
    project_settings,
    timeline,
    environment_settings
};

} // namespace mirakana::editor
