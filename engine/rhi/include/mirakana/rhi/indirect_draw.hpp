// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace mirakana::rhi {

inline constexpr std::uint32_t indexed_indirect_draw_command_word_count = 5;
inline constexpr std::uint32_t indexed_indirect_draw_command_size_bytes =
    indexed_indirect_draw_command_word_count * sizeof(std::uint32_t);
inline constexpr std::uint32_t indexed_indirect_draw_command_stride_bytes = indexed_indirect_draw_command_size_bytes;
inline constexpr std::uint32_t indexed_indirect_draw_offset_alignment_bytes = sizeof(std::uint32_t);
inline constexpr std::uint32_t indexed_indirect_draw_count_buffer_size_bytes = sizeof(std::uint32_t);

struct IndexedIndirectDrawCommand {
    std::uint32_t index_count_per_instance{0};
    std::uint32_t instance_count{0};
    std::uint32_t first_index{0};
    std::int32_t vertex_offset{0};
    std::uint32_t first_instance{0};
};

struct IndexedIndirectDrawDesc {
    BufferHandle argument_buffer;
    std::uint64_t argument_buffer_offset{0};
    std::uint32_t command_stride_bytes{indexed_indirect_draw_command_stride_bytes};
    std::uint32_t max_draw_count{0};
    BufferHandle count_buffer;
    std::uint64_t count_buffer_offset{0};
};

[[nodiscard]] bool has_indexed_indirect_count_buffer(const IndexedIndirectDrawDesc& desc) noexcept;

[[nodiscard]] std::array<std::uint8_t, indexed_indirect_draw_command_size_bytes>
encode_indexed_indirect_draw_command(const IndexedIndirectDrawCommand& command) noexcept;

[[nodiscard]] IndexedIndirectDrawCommand decode_indexed_indirect_draw_command(std::span<const std::uint8_t> bytes);

} // namespace mirakana::rhi
