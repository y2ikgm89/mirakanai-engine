// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/material.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/asset_runtime.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

inline constexpr std::uint64_t runtime_material_uniform_buffer_size_bytes = material_factor_uniform_size_bytes;
inline constexpr std::uint64_t runtime_material_uniform_buffer_allocation_size_bytes = 256;
inline constexpr std::uint64_t runtime_scene_pbr_frame_uniform_size_bytes = 256;
inline constexpr std::uint32_t runtime_mesh_position_vertex_stride_bytes = 12;
inline constexpr std::uint32_t runtime_mesh_tangent_space_vertex_stride_bytes = 48;
/// Interleaved GPU skinning vertex layout: `POSITION` (12) + `NORMAL` (12) + `TEXCOORD0` (8) + `TANGENT` (16) +
/// `BLENDINDICES` `uint16x4` (8) + `BLENDWEIGHT` `float32x4` (16) - must match
/// `GameEngine.CookedSkinnedMesh.v1` cook.
inline constexpr std::uint32_t runtime_skinned_mesh_vertex_stride_bytes = 72;
inline constexpr std::uint32_t runtime_skinned_mesh_joint_matrix_bytes = 64;
/// Maximum joints per draw for D3D12 root constant buffer sizing (`joint_count * 64` aligned to 256 bytes).
inline constexpr std::uint32_t runtime_skinned_mesh_max_joints = 256;
/// GPU morph POSITION delta stream stride: one little-endian float3 per vertex per target.
inline constexpr std::uint32_t runtime_morph_position_delta_stride_bytes = 12;
/// GPU morph NORMAL delta stream stride: one little-endian float3 per vertex per target.
inline constexpr std::uint32_t runtime_morph_normal_delta_stride_bytes = 12;
/// GPU morph TANGENT delta stream stride: one little-endian float3 per vertex per target.
inline constexpr std::uint32_t runtime_morph_tangent_delta_stride_bytes = 12;
/// Per-draw morph weight uniform allocation. D3D12 constant buffer views require 256-byte alignment.
inline constexpr std::uint64_t runtime_morph_weight_uniform_buffer_allocation_size_bytes = 256;

enum class RuntimeMeshVertexLayout : std::uint8_t {
    unknown,
    position,
    position_normal_uv_tangent,
    skinned_mesh_tangent_space
};

struct RuntimeTextureUploadOptions {
    rhi::TextureUsage usage{rhi::TextureUsage::shader_resource | rhi::TextureUsage::copy_destination};
    rhi::QueueKind queue{rhi::QueueKind::graphics};
    bool wait_for_completion{true};
};

struct RuntimeTextureUploadResult {
    rhi::TextureHandle texture;
    rhi::BufferHandle upload_buffer;
    rhi::TextureDesc texture_desc;
    rhi::BufferTextureCopyRegion copy_region;
    std::uint64_t uploaded_bytes{0};
    const rhi::IRhiDevice* owner_device{nullptr};
    bool copy_recorded{false};
    std::string diagnostic;
    rhi::FenceValue submitted_fence{};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct RuntimeMeshUploadOptions {
    rhi::BufferUsage vertex_usage{rhi::BufferUsage::vertex | rhi::BufferUsage::copy_destination};
    rhi::BufferUsage index_usage{rhi::BufferUsage::index | rhi::BufferUsage::copy_destination};
    rhi::QueueKind queue{rhi::QueueKind::graphics};
    std::uint32_t vertex_stride{runtime_mesh_position_vertex_stride_bytes};
    rhi::IndexFormat index_format{rhi::IndexFormat::uint32};
    bool derive_vertex_layout_from_payload{true};
    bool wait_for_completion{true};
};

struct RuntimeMeshVertexLayoutDesc {
    RuntimeMeshVertexLayout layout{RuntimeMeshVertexLayout::unknown};
    std::uint32_t vertex_stride{0};
    std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> vertex_attributes;
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct RuntimeMeshUploadResult {
    rhi::BufferHandle vertex_buffer;
    rhi::BufferHandle index_buffer;
    rhi::BufferHandle vertex_upload_buffer;
    rhi::BufferHandle index_upload_buffer;
    rhi::BufferDesc vertex_buffer_desc;
    rhi::BufferDesc index_buffer_desc;
    rhi::BufferCopyRegion vertex_copy_region;
    rhi::BufferCopyRegion index_copy_region;
    std::uint32_t vertex_stride{0};
    rhi::IndexFormat index_format{rhi::IndexFormat::unknown};
    const rhi::IRhiDevice* owner_device{nullptr};
    std::uint32_t vertex_count{0};
    std::uint32_t index_count{0};
    std::uint64_t uploaded_vertex_bytes{0};
    std::uint64_t uploaded_index_bytes{0};
    bool copy_recorded{false};
    std::string diagnostic;
    rhi::FenceValue submitted_fence{};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct RuntimeSkinnedMeshUploadOptions {
    rhi::BufferUsage vertex_usage{rhi::BufferUsage::vertex | rhi::BufferUsage::copy_destination};
    rhi::BufferUsage index_usage{rhi::BufferUsage::index | rhi::BufferUsage::copy_destination};
    rhi::BufferUsage palette_usage{rhi::BufferUsage::uniform | rhi::BufferUsage::copy_destination};
    rhi::QueueKind queue{rhi::QueueKind::graphics};
    std::uint32_t vertex_stride{runtime_skinned_mesh_vertex_stride_bytes};
    rhi::IndexFormat index_format{rhi::IndexFormat::uint32};
    bool wait_for_completion{true};
};

struct RuntimeSkinnedMeshUploadResult {
    rhi::BufferHandle vertex_buffer;
    rhi::BufferHandle index_buffer;
    rhi::BufferHandle joint_palette_buffer;
    rhi::BufferHandle vertex_upload_buffer;
    rhi::BufferHandle index_upload_buffer;
    rhi::BufferHandle joint_palette_upload_buffer;
    rhi::BufferDesc vertex_buffer_desc;
    rhi::BufferDesc index_buffer_desc;
    rhi::BufferDesc joint_palette_buffer_desc;
    rhi::BufferCopyRegion vertex_copy_region;
    rhi::BufferCopyRegion index_copy_region;
    rhi::BufferCopyRegion joint_palette_copy_region;
    std::uint32_t vertex_stride{runtime_skinned_mesh_vertex_stride_bytes};
    rhi::IndexFormat index_format{rhi::IndexFormat::uint32};
    std::uint32_t vertex_count{0};
    std::uint32_t index_count{0};
    std::uint32_t joint_count{0};
    std::uint64_t joint_palette_uniform_allocation_bytes{0};
    const rhi::IRhiDevice* owner_device{nullptr};
    std::uint64_t uploaded_vertex_bytes{0};
    std::uint64_t uploaded_index_bytes{0};
    std::uint64_t uploaded_joint_palette_bytes{0};
    bool copy_recorded{false};
    std::string diagnostic;
    rhi::FenceValue submitted_fence{};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct RuntimeMorphMeshUploadOptions {
    rhi::BufferUsage position_delta_usage{rhi::BufferUsage::storage | rhi::BufferUsage::copy_destination};
    rhi::BufferUsage normal_delta_usage{rhi::BufferUsage::storage | rhi::BufferUsage::copy_destination};
    rhi::BufferUsage tangent_delta_usage{rhi::BufferUsage::storage | rhi::BufferUsage::copy_destination};
    rhi::BufferUsage weight_usage{rhi::BufferUsage::uniform | rhi::BufferUsage::copy_destination};
    rhi::QueueKind queue{rhi::QueueKind::graphics};
    bool wait_for_completion{true};
};

struct RuntimeMorphMeshUploadResult {
    rhi::BufferHandle position_delta_buffer;
    rhi::BufferHandle normal_delta_buffer;
    rhi::BufferHandle tangent_delta_buffer;
    rhi::BufferHandle morph_weight_buffer;
    rhi::BufferHandle position_delta_upload_buffer;
    rhi::BufferHandle normal_delta_upload_buffer;
    rhi::BufferHandle tangent_delta_upload_buffer;
    rhi::BufferHandle morph_weight_upload_buffer;
    rhi::BufferDesc position_delta_buffer_desc;
    rhi::BufferDesc normal_delta_buffer_desc;
    rhi::BufferDesc tangent_delta_buffer_desc;
    rhi::BufferDesc morph_weight_buffer_desc;
    rhi::BufferCopyRegion position_delta_copy_region;
    rhi::BufferCopyRegion normal_delta_copy_region;
    rhi::BufferCopyRegion tangent_delta_copy_region;
    rhi::BufferCopyRegion morph_weight_copy_region;
    std::uint32_t vertex_count{0};
    std::uint32_t target_count{0};
    std::uint64_t uploaded_position_delta_bytes{0};
    std::uint64_t uploaded_normal_delta_bytes{0};
    std::uint64_t uploaded_tangent_delta_bytes{0};
    std::uint64_t uploaded_weight_bytes{0};
    std::uint64_t morph_weight_uniform_allocation_bytes{0};
    const rhi::IRhiDevice* owner_device{nullptr};
    bool copy_recorded{false};
    std::string diagnostic;
    rhi::FenceValue submitted_fence{};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct RuntimeMorphMeshComputeBindingOptions {
    rhi::DescriptorSetLayoutHandle descriptor_set_layout;
    rhi::BufferUsage output_position_usage{rhi::BufferUsage::storage | rhi::BufferUsage::copy_source};
    rhi::BufferUsage output_normal_usage{rhi::BufferUsage::none};
    rhi::BufferUsage output_tangent_usage{rhi::BufferUsage::none};
    std::uint32_t output_slot_count{1};
    bool create_descriptor_set_layout{true};
};

struct RuntimeMorphMeshComputeOutputSlot {
    rhi::BufferHandle output_position_buffer;
    rhi::BufferHandle output_normal_buffer;
    rhi::BufferHandle output_tangent_buffer;
    rhi::BufferDesc output_position_buffer_desc;
    rhi::BufferDesc output_normal_buffer_desc;
    rhi::BufferDesc output_tangent_buffer_desc;
    std::uint64_t output_position_bytes{0};
    std::uint64_t output_normal_bytes{0};
    std::uint64_t output_tangent_bytes{0};
};

struct RuntimeMorphMeshComputeBinding {
    rhi::DescriptorSetLayoutHandle descriptor_set_layout;
    rhi::DescriptorSetHandle descriptor_set;
    rhi::BufferHandle output_position_buffer;
    rhi::BufferHandle output_normal_buffer;
    rhi::BufferHandle output_tangent_buffer;
    rhi::BufferDesc output_position_buffer_desc;
    rhi::BufferDesc output_normal_buffer_desc;
    rhi::BufferDesc output_tangent_buffer_desc;
    std::vector<RuntimeMorphMeshComputeOutputSlot> output_slots;
    std::uint32_t vertex_count{0};
    std::uint32_t target_count{0};
    std::uint64_t output_position_bytes{0};
    std::uint64_t output_normal_bytes{0};
    std::uint64_t output_tangent_bytes{0};
    const rhi::IRhiDevice* owner_device{nullptr};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct RuntimeMaterialTextureResource {
    MaterialTextureSlot slot{MaterialTextureSlot::unknown};
    rhi::TextureHandle texture;
    const rhi::IRhiDevice* owner_device{nullptr};
};

struct RuntimeMaterialGpuBinding {
    rhi::DescriptorSetLayoutHandle descriptor_set_layout;
    rhi::DescriptorSetHandle descriptor_set;
    rhi::BufferHandle uniform_buffer;
    rhi::BufferHandle uniform_upload_buffer;
    /// Optional upload-heap uniform used for `scene_pbr_frame` when no shared buffer was supplied.
    rhi::BufferHandle scene_pbr_frame_buffer{};
    std::vector<rhi::SamplerHandle> samplers;
    std::vector<rhi::DescriptorWrite> writes;
    std::uint64_t factor_bytes_uploaded{0};
    const rhi::IRhiDevice* owner_device{nullptr};
    bool factor_copy_recorded{false};
    std::string diagnostic;
    rhi::FenceValue submitted_fence{};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct RuntimeMaterialDescriptorSetLayoutDescResult {
    rhi::DescriptorSetLayoutDesc desc;
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct RuntimeMaterialGpuBindingOptions {
    rhi::DescriptorSetLayoutHandle descriptor_set_layout;
    bool create_descriptor_set_layout{true};
    rhi::BufferHandle shared_scene_pbr_frame_uniform{};
};

[[nodiscard]] RuntimeTextureUploadResult upload_runtime_texture(rhi::IRhiDevice& device,
                                                                const runtime::RuntimeTexturePayload& payload,
                                                                const RuntimeTextureUploadOptions& options = {});

[[nodiscard]] RuntimeMeshUploadResult upload_runtime_mesh(rhi::IRhiDevice& device,
                                                          const runtime::RuntimeMeshPayload& payload,
                                                          const RuntimeMeshUploadOptions& options = {});

[[nodiscard]] MeshGpuBinding make_runtime_mesh_gpu_binding(const RuntimeMeshUploadResult& upload) noexcept;

[[nodiscard]] RuntimeSkinnedMeshUploadResult
upload_runtime_skinned_mesh(rhi::IRhiDevice& device, const runtime::RuntimeSkinnedMeshPayload& payload,
                            const RuntimeSkinnedMeshUploadOptions& options = {});

[[nodiscard]] SkinnedMeshGpuBinding
make_runtime_skinned_mesh_gpu_binding(const RuntimeSkinnedMeshUploadResult& upload) noexcept;

[[nodiscard]] RuntimeMorphMeshUploadResult
upload_runtime_morph_mesh_cpu(rhi::IRhiDevice& device, const runtime::RuntimeMorphMeshCpuPayload& payload,
                              const RuntimeMorphMeshUploadOptions& options = {});

[[nodiscard]] MorphMeshGpuBinding
make_runtime_morph_mesh_gpu_binding(const RuntimeMorphMeshUploadResult& upload) noexcept;

[[nodiscard]] RuntimeMorphMeshComputeBinding
create_runtime_morph_mesh_compute_binding(rhi::IRhiDevice& device, const RuntimeMeshUploadResult& mesh_upload,
                                          const RuntimeMorphMeshUploadResult& morph_upload,
                                          const RuntimeMorphMeshComputeBindingOptions& options = {});

[[nodiscard]] MeshGpuBinding
make_runtime_compute_morph_output_mesh_gpu_binding(const RuntimeMeshUploadResult& mesh_upload,
                                                   const RuntimeMorphMeshComputeBinding& compute_binding,
                                                   std::uint32_t output_slot_index = 0) noexcept;

[[nodiscard]] MeshGpuBinding
make_runtime_compute_morph_tangent_frame_output_mesh_gpu_binding(const RuntimeMeshUploadResult& mesh_upload,
                                                                 const RuntimeMorphMeshComputeBinding& compute_binding,
                                                                 std::uint32_t output_slot_index = 0) noexcept;

[[nodiscard]] SkinnedMeshGpuBinding
make_runtime_compute_morph_skinned_mesh_gpu_binding(const RuntimeSkinnedMeshUploadResult& skinned_upload,
                                                    const RuntimeMorphMeshComputeBinding& compute_binding,
                                                    std::uint32_t output_slot_index = 0) noexcept;

/// Allocates a per-mesh descriptor set that binds `upload.joint_palette_buffer` as a vertex-visible uniform buffer at
/// binding 0. When `shared_joint_descriptor_set_layout_inout.value == 0`, creates a shared layout and assigns it;
/// otherwise reuses the provided layout for additional skinned meshes in the same scene palette.
[[nodiscard]] std::string
attach_skinned_mesh_joint_descriptor_set(rhi::IRhiDevice& device, const RuntimeSkinnedMeshUploadResult& upload,
                                         SkinnedMeshGpuBinding& out,
                                         rhi::DescriptorSetLayoutHandle& shared_joint_descriptor_set_layout_inout);

/// Allocates a per-mesh descriptor set that binds morph POSITION deltas as a vertex-visible storage buffer at binding
/// 0 and morph weights as a vertex-visible uniform buffer at binding 1. Optional NORMAL and TANGENT deltas are bound
/// as vertex-visible storage buffers at bindings 2 and 3 when present. Reuses `shared_layout_inout` when provided.
[[nodiscard]] std::string attach_morph_mesh_descriptor_set(rhi::IRhiDevice& device,
                                                           const RuntimeMorphMeshUploadResult& upload,
                                                           MorphMeshGpuBinding& out,
                                                           rhi::DescriptorSetLayoutHandle& shared_layout_inout);

[[nodiscard]] RuntimeMeshVertexLayoutDesc
make_runtime_skinned_mesh_vertex_layout_desc(const runtime::RuntimeSkinnedMeshPayload& payload);

[[nodiscard]] RuntimeMeshVertexLayoutDesc
make_runtime_mesh_vertex_layout_desc(const runtime::RuntimeMeshPayload& payload);

[[nodiscard]] RuntimeMeshUploadOptions make_runtime_mesh_upload_options(const runtime::RuntimeMeshPayload& payload,
                                                                        RuntimeMeshUploadOptions options = {});

[[nodiscard]] RuntimeMaterialDescriptorSetLayoutDescResult
make_runtime_material_descriptor_set_layout_desc(const MaterialPipelineBindingMetadata& metadata);

[[nodiscard]] RuntimeMaterialGpuBinding create_runtime_material_gpu_binding(
    rhi::IRhiDevice& device, const MaterialPipelineBindingMetadata& metadata, const MaterialFactors& factors,
    const std::vector<RuntimeMaterialTextureResource>& textures, const RuntimeMaterialGpuBindingOptions& options = {});

} // namespace mirakana::runtime_rhi
