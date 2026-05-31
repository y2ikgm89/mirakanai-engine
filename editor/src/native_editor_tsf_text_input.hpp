// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/ui.hpp"

#include <memory>
#include <optional>

namespace mirakana::editor {

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

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] std::unique_ptr<NativeEditorTsfTextServicesAdapter> make_native_editor_tsf_text_services_adapter();

} // namespace mirakana::editor
