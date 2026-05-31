// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/ui.hpp"

#include <cstdint>
#include <string>

namespace mirakana::editor {

class NativeEditorApp;

struct FirstPartyEditorDocument {
    ui::UiDocument document;
    ui::LayoutResult layout;
    ui::RendererSubmission renderer_submission;
    std::uint32_t panel_root_count{0};
    bool native_handles_exposed{false};
};

struct FirstPartyEditorShellSmokeCounters {
    std::string ui{"first_party"};
    std::string backend{"d3d12"};
    std::uint32_t panel_count{0};
    bool imgui_enabled{false};
    bool sdl3_enabled{false};
    bool viewport_native_handles_exposed{false};
    bool material_preview_native_handles_exposed{false};
};

[[nodiscard]] FirstPartyEditorDocument make_first_party_editor_document(const NativeEditorApp& app);

[[nodiscard]] FirstPartyEditorShellSmokeCounters
make_first_party_editor_shell_smoke_counters(const NativeEditorApp& app, const FirstPartyEditorDocument& document);

} // namespace mirakana::editor
