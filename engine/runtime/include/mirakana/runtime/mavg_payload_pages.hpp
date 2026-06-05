// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/assets/mavg_cluster_payload.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeMavgPayloadPageSliceDiagnosticCode : std::uint8_t {
    missing_graph = 0,
    invalid_graph,
    invalid_payload,
    unknown_page,
    duplicate_requested_page,
    missing_payload_page,
};

struct RuntimeMavgPayloadPageSliceDiagnostic {
    RuntimeMavgPayloadPageSliceDiagnosticCode code{RuntimeMavgPayloadPageSliceDiagnosticCode::missing_graph};
    std::uint32_t page_index{0};
    std::string message;
};

struct RuntimeMavgPayloadPageSliceDesc {
    const MavgClusterGraphDocument* graph{nullptr};
    std::string_view payload_text;
    std::span<const std::uint32_t> page_indices;
};

struct RuntimeMavgPayloadPageSliceRow {
    std::uint32_t page_index{0};
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
    std::vector<std::uint8_t> payload_bytes;
};

struct RuntimeMavgPayloadPageSliceResult {
    std::vector<RuntimeMavgPayloadPageSliceRow> pages;
    std::vector<RuntimeMavgPayloadPageSliceDiagnostic> diagnostics;
    std::size_t requested_page_count{0};
    std::size_t extracted_page_count{0};
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool executed_background_worker{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RuntimeMavgPayloadPageSliceResult
extract_runtime_mavg_payload_page_slices(const RuntimeMavgPayloadPageSliceDesc& desc);

} // namespace mirakana::runtime
