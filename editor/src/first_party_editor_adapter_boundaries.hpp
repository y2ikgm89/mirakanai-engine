// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::editor {

#if defined(_WIN32)
inline constexpr bool kFirstPartyEditorDirectWriteTextFontAdapterImplemented = true;
inline constexpr bool kFirstPartyEditorTsfTextServicesAdapterImplemented = true;
inline constexpr bool kFirstPartyEditorUiaAccessibilityBridgeImplemented = true;
#else
inline constexpr bool kFirstPartyEditorDirectWriteTextFontAdapterImplemented = false;
inline constexpr bool kFirstPartyEditorTsfTextServicesAdapterImplemented = false;
inline constexpr bool kFirstPartyEditorUiaAccessibilityBridgeImplemented = false;
#endif

enum class FirstPartyEditorAdapterBoundary : std::uint8_t {
    text_shaping,
    font_rasterization,
    ime_text_services,
    accessibility_bridge,
    image_decoding,
};

struct FirstPartyEditorAdapterBoundaryRow {
    FirstPartyEditorAdapterBoundary boundary{FirstPartyEditorAdapterBoundary::text_shaping};
    std::string id;
    std::string official_source_family;
    bool implemented{false};
    bool native_handles_public{false};
};

[[nodiscard]] inline std::vector<FirstPartyEditorAdapterBoundaryRow> first_party_editor_required_adapter_boundaries() {
    return {
        FirstPartyEditorAdapterBoundaryRow{
            .boundary = FirstPartyEditorAdapterBoundary::text_shaping,
            .id = "editor.adapter.text_shaping",
            .official_source_family = "Windows SDK DirectWrite private text layout adapter",
            .implemented = kFirstPartyEditorDirectWriteTextFontAdapterImplemented,
        },
        FirstPartyEditorAdapterBoundaryRow{
            .boundary = FirstPartyEditorAdapterBoundary::font_rasterization,
            .id = "editor.adapter.font_rasterization",
            .official_source_family = "Windows SDK DirectWrite private glyph rasterization adapter",
            .implemented = kFirstPartyEditorDirectWriteTextFontAdapterImplemented,
        },
        FirstPartyEditorAdapterBoundaryRow{
            .boundary = FirstPartyEditorAdapterBoundary::ime_text_services,
            .id = "editor.adapter.ime_text_services",
            .official_source_family = "Text Services Framework or platform text services adapter",
            .implemented = kFirstPartyEditorTsfTextServicesAdapterImplemented,
        },
        FirstPartyEditorAdapterBoundaryRow{
            .boundary = FirstPartyEditorAdapterBoundary::accessibility_bridge,
            .id = "editor.adapter.accessibility_bridge",
            .official_source_family = "UI Automation, NSAccessibility, AT-SPI, Android, or iOS accessibility adapter",
            .implemented = kFirstPartyEditorUiaAccessibilityBridgeImplemented,
        },
        FirstPartyEditorAdapterBoundaryRow{
            .boundary = FirstPartyEditorAdapterBoundary::image_decoding,
            .id = "editor.adapter.image_decoding",
            .official_source_family = "audited source image codec adapter",
        },
    };
}

} // namespace mirakana::editor
