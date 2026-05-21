// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/backend_renderer_parity_policy.hpp"

namespace mirakana {

bool backend_renderer_parity_proof_matches_selected_backend(const BackendRendererParityProofDesc& desc) noexcept {
    return desc.selected_backend != rhi::BackendKind::null && desc.selected_backend == desc.proof_backend;
}

const char* backend_renderer_parity_diagnostic_message(const BackendRendererParityDiagnosticCode code) noexcept {
    switch (code) {
    case BackendRendererParityDiagnosticCode::none:
        return "none";
    case BackendRendererParityDiagnosticCode::cross_backend_proof_transfer:
        return "cross_backend_proof_transfer";
    }
    return "unknown";
}

} // namespace mirakana
