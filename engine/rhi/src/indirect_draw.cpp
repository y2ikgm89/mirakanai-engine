// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/indirect_draw.hpp"

#include <bit>
#include <limits>
#include <stdexcept>
#include <vector>

namespace mirakana::rhi {
namespace {

void write_u32(std::array<std::uint8_t, indexed_indirect_draw_command_size_bytes>& bytes, std::size_t offset,
               std::uint32_t value) noexcept {
    bytes[offset + 0U] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    bytes[offset + 2U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    bytes[offset + 3U] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

[[nodiscard]] std::uint32_t read_u32(std::span<const std::uint8_t> bytes, std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset + 0U]) | (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] bool is_aligned_to(std::uint64_t value, std::uint64_t alignment) noexcept {
    return alignment != 0U && value % alignment == 0U;
}

[[nodiscard]] std::uint64_t checked_add_u64(std::uint64_t lhs, std::uint64_t rhs, const char* message) {
    if (lhs > (std::numeric_limits<std::uint64_t>::max)() - rhs) {
        throw std::overflow_error(message);
    }
    return lhs + rhs;
}

[[nodiscard]] std::uint64_t checked_mul_u64(std::uint64_t lhs, std::uint64_t rhs, const char* message) {
    if (lhs != 0U && rhs > (std::numeric_limits<std::uint64_t>::max)() / lhs) {
        throw std::overflow_error(message);
    }
    return lhs * rhs;
}

} // namespace

bool has_indexed_indirect_count_buffer(const IndexedIndirectDrawDesc& desc) noexcept {
    return desc.count_buffer.value != 0;
}

std::array<std::uint8_t, indexed_indirect_draw_command_size_bytes>
encode_indexed_indirect_draw_command(const IndexedIndirectDrawCommand& command) noexcept {
    std::array<std::uint8_t, indexed_indirect_draw_command_size_bytes> bytes{};
    write_u32(bytes, 0U, command.index_count_per_instance);
    write_u32(bytes, 4U, command.instance_count);
    write_u32(bytes, 8U, command.first_index);
    write_u32(bytes, 12U, std::bit_cast<std::uint32_t>(command.vertex_offset));
    write_u32(bytes, 16U, command.first_instance);
    return bytes;
}

IndexedIndirectDrawCommand decode_indexed_indirect_draw_command(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < indexed_indirect_draw_command_size_bytes) {
        throw std::invalid_argument("rhi indexed indirect draw command bytes are too small");
    }

    return IndexedIndirectDrawCommand{
        .index_count_per_instance = read_u32(bytes, 0U),
        .instance_count = read_u32(bytes, 4U),
        .first_index = read_u32(bytes, 8U),
        .vertex_offset = std::bit_cast<std::int32_t>(read_u32(bytes, 12U)),
        .first_instance = read_u32(bytes, 16U),
    };
}

std::uint64_t indexed_indirect_argument_range_end(const IndexedIndirectDrawDesc& desc) {
    if (desc.max_draw_count == 0) {
        throw std::invalid_argument("rhi indexed indirect draw max draw count must be non-zero");
    }
    if (!is_aligned_to(desc.argument_buffer_offset, indexed_indirect_draw_offset_alignment_bytes)) {
        throw std::invalid_argument("rhi indexed indirect draw argument offset must be 4-byte aligned");
    }
    if (desc.command_stride_bytes < indexed_indirect_draw_command_size_bytes ||
        !is_aligned_to(desc.command_stride_bytes, indexed_indirect_draw_offset_alignment_bytes)) {
        throw std::invalid_argument(
            "rhi indexed indirect draw command stride must be 4-byte aligned and at least the command size");
    }

    const auto last_stride_offset =
        checked_mul_u64(static_cast<std::uint64_t>(desc.command_stride_bytes), desc.max_draw_count - 1ULL,
                        "rhi indexed indirect draw argument range overflowed");
    const auto last_command_offset = checked_add_u64(desc.argument_buffer_offset, last_stride_offset,
                                                     "rhi indexed indirect draw argument range overflowed");
    return checked_add_u64(last_command_offset, indexed_indirect_draw_command_size_bytes,
                           "rhi indexed indirect draw argument range overflowed");
}

std::uint32_t decode_indexed_indirect_count_buffer_value(std::span<const std::uint8_t> count_bytes) {
    if (count_bytes.size() < indexed_indirect_draw_count_buffer_size_bytes) {
        throw std::invalid_argument("rhi indexed indirect draw count buffer bytes are too small");
    }
    return read_u32(count_bytes, 0U);
}

std::uint32_t effective_indexed_indirect_draw_count(std::uint32_t count_buffer_value,
                                                    const IndexedIndirectDrawDesc& desc) noexcept {
    return count_buffer_value < desc.max_draw_count ? count_buffer_value : desc.max_draw_count;
}

std::vector<IndexedIndirectDrawCommand>
decode_indexed_indirect_draw_commands(std::span<const std::uint8_t> argument_bytes, const IndexedIndirectDrawDesc& desc,
                                      std::uint32_t draw_count) {
    if (draw_count > desc.max_draw_count) {
        throw std::invalid_argument("rhi indexed indirect draw count must not exceed the maximum draw count");
    }

    std::vector<IndexedIndirectDrawCommand> commands;
    commands.reserve(draw_count);

    for (std::uint32_t draw_index = 0; draw_index < draw_count; ++draw_index) {
        const auto command_offset =
            checked_add_u64(desc.argument_buffer_offset,
                            checked_mul_u64(static_cast<std::uint64_t>(desc.command_stride_bytes), draw_index,
                                            "rhi indexed indirect draw command offset overflowed"),
                            "rhi indexed indirect draw command offset overflowed");
        auto command = decode_indexed_indirect_draw_command(
            argument_bytes.subspan(static_cast<std::size_t>(command_offset), indexed_indirect_draw_command_size_bytes));
        if (command.index_count_per_instance == 0 || command.instance_count == 0) {
            throw std::invalid_argument("rhi indexed indirect draw commands must have non-zero draw counts");
        }
        commands.push_back(command);
    }

    return commands;
}

std::vector<IndexedIndirectDrawCommand>
decode_indexed_indirect_draw_commands(std::span<const std::uint8_t> argument_bytes,
                                      const IndexedIndirectDrawDesc& desc) {
    return decode_indexed_indirect_draw_commands(argument_bytes, desc, desc.max_draw_count);
}

void record_indexed_indirect_draw_stats(RhiStats& stats, std::span<const IndexedIndirectDrawCommand> commands,
                                        const IndexedIndirectDrawDesc& desc, bool count_buffer_used,
                                        std::uint32_t count_buffer_value) noexcept {
    ++stats.indexed_indirect_draw_calls;
    if (count_buffer_used) {
        ++stats.indexed_indirect_count_buffer_reads;
    }
    stats.last_indexed_indirect_max_draw_count = desc.max_draw_count;
    stats.last_indexed_indirect_executed_draw_count = static_cast<std::uint32_t>(commands.size());
    stats.last_indexed_indirect_count_buffer_value = count_buffer_used ? count_buffer_value : desc.max_draw_count;

    for (const auto& command : commands) {
        ++stats.draw_calls;
        ++stats.indexed_draw_calls;
        ++stats.indexed_indirect_commands_executed;
        if (command.instance_count > 1) {
            ++stats.instanced_draw_calls;
            ++stats.instanced_indexed_draw_calls;
            stats.instanced_instances_submitted += command.instance_count;
        }
        stats.indices_submitted +=
            static_cast<std::uint64_t>(command.index_count_per_instance) * command.instance_count;
        stats.last_indexed_draw_index_count = command.index_count_per_instance;
        stats.last_indexed_draw_instance_count = command.instance_count;
        stats.last_indexed_draw_first_index = command.first_index;
        stats.last_indexed_draw_vertex_offset = command.vertex_offset;
        stats.last_indexed_draw_first_instance = command.first_instance;
    }
}

void IRhiCommandList::draw_indexed_indirect(const IndexedIndirectDrawDesc&) {
    throw std::logic_error("rhi indexed indirect draw is unsupported by this command list");
}

} // namespace mirakana::rhi
