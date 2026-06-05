// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/ui.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mirakana::editor {

struct NativeEditorTsfTextStoreEvidence {
    std::string status{"not_ready"};
    bool text_store_ready{false};
    std::uint32_t sink_advisory_rows{0};
    std::uint32_t document_lock_rows{0};
    std::uint32_t request_lock_rows{0};
    std::uint32_t selection_read_rows{0};
    std::uint32_t selection_write_rows{0};
    std::uint32_t text_read_rows{0};
    std::uint32_t text_replace_rows{0};
    std::uint32_t insert_text_at_selection_rows{0};
    std::uint32_t text_ext_rows{0};
    std::uint32_t screen_ext_rows{0};
    bool candidate_ui_host_owned{true};
    std::uint32_t reconversion_diagnostic_rows{0};
    bool native_handles_exposed{false};
    std::vector<std::string> diagnostics;
};

class NativeEditorTsfTextServicesAdapter final : public ui::IPlatformIntegrationAdapter, public ui::IImeAdapter {
  public:
    NativeEditorTsfTextServicesAdapter();
    ~NativeEditorTsfTextServicesAdapter() override;

    NativeEditorTsfTextServicesAdapter(const NativeEditorTsfTextServicesAdapter&) = delete;
    NativeEditorTsfTextServicesAdapter& operator=(const NativeEditorTsfTextServicesAdapter&) = delete;
    NativeEditorTsfTextServicesAdapter(NativeEditorTsfTextServicesAdapter&&) noexcept;
    NativeEditorTsfTextServicesAdapter& operator=(NativeEditorTsfTextServicesAdapter&&) noexcept;

    void begin_text_input(const ui::PlatformTextInputRequest& request) override;
    void end_text_input(const ui::ElementId& target) override;
    void update_composition(const ui::ImeComposition& composition) override;

    [[nodiscard]] const std::optional<ui::PlatformTextInputRequest>& active_request() const noexcept;
    [[nodiscard]] bool native_handles_exposed() const noexcept;
    [[nodiscard]] bool tsf_thread_manager_ready() const noexcept;
    [[nodiscard]] bool tsf_document_manager_ready() const noexcept;
    [[nodiscard]] NativeEditorTsfTextStoreEvidence tsf_text_store_evidence() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] std::unique_ptr<NativeEditorTsfTextServicesAdapter> make_native_editor_tsf_text_services_adapter();

} // namespace mirakana::editor
