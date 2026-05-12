// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/morph_mesh_cpu_source_bridge.hpp"

#include <cstring>
#include <stdexcept>
#include <vector>

namespace mirakana {
namespace {

void append_le_f32(std::vector<std::uint8_t>& output, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    output.push_back(static_cast<std::uint8_t>(bits & 0xFFU));
    output.push_back(static_cast<std::uint8_t>((bits >> 8U) & 0xFFU));
    output.push_back(static_cast<std::uint8_t>((bits >> 16U) & 0xFFU));
    output.push_back(static_cast<std::uint8_t>((bits >> 24U) & 0xFFU));
}

void append_vec3_bytes(std::vector<std::uint8_t>& output, const Vec3& value) {
    append_le_f32(output, value.x);
    append_le_f32(output, value.y);
    append_le_f32(output, value.z);
}

[[nodiscard]] std::vector<std::uint8_t> pack_vec3_stream(const std::vector<Vec3>& values) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(values.size() * 12U);
    for (const auto& value : values) {
        append_vec3_bytes(bytes, value);
    }
    return bytes;
}

[[nodiscard]] std::vector<Vec3> unpack_vec3_stream(const std::vector<std::uint8_t>& bytes) {
    if ((bytes.size() % 12U) != 0U) {
        throw std::invalid_argument("vec3 byte stream length is invalid");
    }
    std::vector<Vec3> values;
    values.reserve(bytes.size() / 12U);
    for (std::size_t offset = 0; offset < bytes.size(); offset += 12U) {
        Vec3 value{};
        std::memcpy(&value.x, &bytes[offset], sizeof(float));
        std::memcpy(&value.y, &bytes[offset + 4U], sizeof(float));
        std::memcpy(&value.z, &bytes[offset + 8U], sizeof(float));
        values.push_back(value);
    }
    return values;
}

[[nodiscard]] std::vector<float> unpack_f32_weights(const std::vector<std::uint8_t>& bytes) {
    if ((bytes.size() % 4U) != 0U) {
        throw std::invalid_argument("morph target weight byte length is invalid");
    }
    std::vector<float> weights;
    weights.reserve(bytes.size() / 4U);
    for (std::size_t offset = 0; offset < bytes.size(); offset += 4U) {
        float weight = 0.0F;
        std::memcpy(&weight, &bytes[offset], sizeof(float));
        weights.push_back(weight);
    }
    return weights;
}

} // namespace

MorphMeshCpuSourceDocument morph_mesh_cpu_source_document_from_animation_desc(const AnimationMorphMeshCpuDesc& desc) {
    if (!is_valid_animation_morph_mesh_cpu_desc(desc)) {
        throw std::invalid_argument("animation morph mesh CPU description is invalid");
    }
    MorphMeshCpuSourceDocument document;
    document.vertex_count = static_cast<std::uint32_t>(desc.bind_positions.size());
    document.bind_position_bytes = pack_vec3_stream(desc.bind_positions);
    document.bind_normal_bytes = pack_vec3_stream(desc.bind_normals);
    document.bind_tangent_bytes = pack_vec3_stream(desc.bind_tangents);
    document.targets.reserve(desc.targets.size());
    for (const auto& target : desc.targets) {
        MorphMeshCpuTargetSourceDocument row;
        row.position_delta_bytes = pack_vec3_stream(target.position_deltas);
        row.normal_delta_bytes = pack_vec3_stream(target.normal_deltas);
        row.tangent_delta_bytes = pack_vec3_stream(target.tangent_deltas);
        document.targets.push_back(std::move(row));
    }
    document.target_weight_bytes.clear();
    document.target_weight_bytes.reserve(desc.target_weights.size() * 4U);
    for (const auto weight : desc.target_weights) {
        append_le_f32(document.target_weight_bytes, weight);
    }
    if (!is_valid_morph_mesh_cpu_source_document(document)) {
        throw std::invalid_argument("morph mesh CPU source document derived from animation desc is invalid");
    }
    return document;
}

AnimationMorphMeshCpuDesc
animation_morph_mesh_cpu_desc_from_source_document(const MorphMeshCpuSourceDocument& document) {
    if (!is_valid_morph_mesh_cpu_source_document(document)) {
        throw std::invalid_argument("morph mesh CPU source document is invalid");
    }
    AnimationMorphMeshCpuDesc desc;
    desc.bind_positions = unpack_vec3_stream(document.bind_position_bytes);
    desc.bind_normals = unpack_vec3_stream(document.bind_normal_bytes);
    desc.bind_tangents = unpack_vec3_stream(document.bind_tangent_bytes);
    desc.target_weights = unpack_f32_weights(document.target_weight_bytes);
    desc.targets.reserve(document.targets.size());
    for (const auto& target : document.targets) {
        AnimationMorphTargetCpuDesc row;
        row.position_deltas = unpack_vec3_stream(target.position_delta_bytes);
        row.normal_deltas = unpack_vec3_stream(target.normal_delta_bytes);
        row.tangent_deltas = unpack_vec3_stream(target.tangent_delta_bytes);
        desc.targets.push_back(std::move(row));
    }
    if (!is_valid_animation_morph_mesh_cpu_desc(desc)) {
        throw std::invalid_argument("animation morph mesh CPU description derived from source document is invalid");
    }
    return desc;
}

} // namespace mirakana
