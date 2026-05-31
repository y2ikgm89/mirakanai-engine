// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::editor {

struct NativeEditorTextAtlasHandoffDesc {
    std::string text{"Mirakanai"};
    std::string font_family{"Segoe UI"};
    float pixel_size{18.0F};
    float max_width{512.0F};
};

struct NativeEditorTextAtlasHandoffEvidenceRow {
    std::string id;
    std::string status;
    bool adapter_invoked{false};
    bool glyphs_ready{false};
    bool fallback_used{false};
    bool host_evidence_required{false};
    bool host_evidence_available{false};
    bool unsupported{false};
    bool native_handles_public{false};
};

struct NativeEditorTextAtlasHandoffEvidence {
    std::string status{"not_evaluated"};
    bool text_shaping_adapter_invoked{false};
    bool font_rasterizer_adapter_invoked{false};
    bool glyphs_ready{false};
    bool fallback_used{false};
    bool atlas_handoff_ready{false};
    bool native_handles_exposed{false};
    std::uint32_t adapter_invoked_rows{0};
    std::uint32_t glyphs_ready_rows{0};
    std::uint32_t fallback_rows{0};
    std::uint32_t fallback_used_rows{0};
    std::uint32_t fallback_not_used_rows{0};
    std::uint32_t host_gated_rows{0};
    std::uint32_t unsupported_rows{0};
    std::uint32_t shaped_glyph_count{0};
    std::uint32_t rasterized_glyph_count{0};
    std::uint32_t zero_ink_glyph_count{0};
    std::vector<NativeEditorTextAtlasHandoffEvidenceRow> rows;
};

[[nodiscard]] NativeEditorTextAtlasHandoffEvidence
make_native_editor_directwrite_text_atlas_handoff_evidence(const NativeEditorTextAtlasHandoffDesc& desc);

} // namespace mirakana::editor
