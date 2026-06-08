// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/indirect_draw.hpp"

#include <bit>
#include <stdexcept>

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

void IRhiCommandList::draw_indexed_indirect(const IndexedIndirectDrawDesc&) {
    throw std::logic_error("rhi indexed indirect draw is unsupported by this command list");
}

} // namespace mirakana::rhi
