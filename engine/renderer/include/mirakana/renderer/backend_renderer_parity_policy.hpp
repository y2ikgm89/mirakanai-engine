// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <cstdint>

namespace mirakana {

enum class BackendRendererParityDiagnosticCode : std::uint8_t {
    none = 0,
    cross_backend_proof_transfer,
};

struct BackendRendererParityProofDesc {
    rhi::BackendKind selected_backend{rhi::BackendKind::null};
    rhi::BackendKind proof_backend{rhi::BackendKind::null};
};

[[nodiscard]] bool
backend_renderer_parity_proof_matches_selected_backend(const BackendRendererParityProofDesc& desc) noexcept;

[[nodiscard]] const char* backend_renderer_parity_diagnostic_message(BackendRendererParityDiagnosticCode code) noexcept;

} // namespace mirakana
