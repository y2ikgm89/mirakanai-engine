// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

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

[[nodiscard]] std::uint64_t indexed_indirect_argument_range_end(const IndexedIndirectDrawDesc& desc);

// Decodes the 32-bit little-endian draw count from a CPU-readable upload count buffer slice. When a count buffer is
// supplied, this value (clamped to the maximum draw count) determines how many indexed indirect draws execute.
[[nodiscard]] std::uint32_t decode_indexed_indirect_count_buffer_value(std::span<const std::uint8_t> count_bytes);

// Returns `min(count_buffer_value, desc.max_draw_count)`, the effective number of indexed indirect draws executed when
// a count buffer is supplied. Backends clamp the supplied count to the maximum draw count for deterministic execution.
[[nodiscard]] std::uint32_t effective_indexed_indirect_draw_count(std::uint32_t count_buffer_value,
                                                                  const IndexedIndirectDrawDesc& desc) noexcept;

[[nodiscard]] std::vector<IndexedIndirectDrawCommand>
decode_indexed_indirect_draw_commands(std::span<const std::uint8_t> argument_bytes,
                                      const IndexedIndirectDrawDesc& desc);

// Bounded variant that decodes exactly `draw_count` commands (which must not exceed `desc.max_draw_count`). Used when a
// count buffer limits execution below `max_draw_count`.
[[nodiscard]] std::vector<IndexedIndirectDrawCommand>
decode_indexed_indirect_draw_commands(std::span<const std::uint8_t> argument_bytes, const IndexedIndirectDrawDesc& desc,
                                      std::uint32_t draw_count);

void record_indexed_indirect_draw_stats(RhiStats& stats, std::span<const IndexedIndirectDrawCommand> commands,
                                        const IndexedIndirectDrawDesc& desc, bool count_buffer_used = false,
                                        std::uint32_t count_buffer_value = 0) noexcept;

} // namespace mirakana::rhi
