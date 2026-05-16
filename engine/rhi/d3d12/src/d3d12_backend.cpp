// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/d3d12/d3d12_backend.hpp"

#include "mirakana/rhi/gpu_debug.hpp"
#include "mirakana/rhi/memory_diagnostics.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d12.h>
#include <d3dcommon.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::rhi::d3d12 {
namespace {

[[nodiscard]] bool adapter_supports_d3d12(IUnknown* adapter) noexcept {
    return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr));
}

[[nodiscard]] bool enable_debug_layer() noexcept {
    Microsoft::WRL::ComPtr<ID3D12Debug> debug;
    if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        return false;
    }
    debug->EnableDebugLayer();
    return true;
}

[[nodiscard]] bool find_hardware_adapter(IDXGIFactory6* factory,
                                         Microsoft::WRL::ComPtr<IDXGIAdapter1>& selected) noexcept {
    for (std::uint32_t adapter_index = 0;; ++adapter_index) {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        const HRESULT result = factory->EnumAdapters1(adapter_index, &adapter);
        if (result == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(result)) {
            continue;
        }

        DXGI_ADAPTER_DESC1 desc{};
        if (FAILED(adapter->GetDesc1(&desc))) {
            continue;
        }
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0U) {
            continue;
        }
        if (!adapter_supports_d3d12(adapter.Get())) {
            continue;
        }

        selected = adapter;
        return true;
    }

    return false;
}

[[nodiscard]] bool find_warp_adapter(IDXGIFactory6* factory, Microsoft::WRL::ComPtr<IDXGIAdapter>& selected) noexcept {
    if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&selected)))) {
        return false;
    }
    return adapter_supports_d3d12(selected.Get());
}

[[nodiscard]] IUnknown* select_adapter(IDXGIFactory6* factory, bool prefer_warp,
                                       Microsoft::WRL::ComPtr<IDXGIAdapter1>& hardware_adapter,
                                       Microsoft::WRL::ComPtr<IDXGIAdapter>& warp_adapter, bool& used_warp) noexcept {
    used_warp = false;
    if (!prefer_warp && find_hardware_adapter(factory, hardware_adapter)) {
        return hardware_adapter.Get();
    }
    if (find_warp_adapter(factory, warp_adapter)) {
        used_warp = true;
        return warp_adapter.Get();
    }
    if (find_hardware_adapter(factory, hardware_adapter)) {
        return hardware_adapter.Get();
    }
    return nullptr;
}

[[nodiscard]] DXGI_FORMAT to_dxgi_format(Format format) noexcept {
    switch (format) {
    case Format::rgba8_unorm:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case Format::bgra8_unorm:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    case Format::depth24_stencil8:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case Format::unknown:
        break;
    }
    return DXGI_FORMAT_UNKNOWN;
}

[[nodiscard]] bool is_d3d12_color_render_format(Format format) noexcept {
    return format == Format::rgba8_unorm || format == Format::bgra8_unorm;
}

[[nodiscard]] bool is_d3d12_depth_stencil_format(Format format) noexcept {
    return format == Format::depth24_stencil8;
}

[[nodiscard]] DXGI_FORMAT to_dxgi_vertex_format(VertexFormat format) noexcept {
    switch (format) {
    case VertexFormat::float32x2:
        return DXGI_FORMAT_R32G32_FLOAT;
    case VertexFormat::float32x3:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case VertexFormat::float32x4:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case VertexFormat::uint16x4:
        return DXGI_FORMAT_R16G16B16A16_UINT;
    case VertexFormat::unknown:
        break;
    }
    return DXGI_FORMAT_UNKNOWN;
}

[[nodiscard]] std::uint32_t vertex_format_size(VertexFormat format) noexcept {
    switch (format) {
    case VertexFormat::float32x2:
        return 8;
    case VertexFormat::float32x3:
        return 12;
    case VertexFormat::float32x4:
        return 16;
    case VertexFormat::uint16x4:
        return 8;
    case VertexFormat::unknown:
        break;
    }
    return 0;
}

[[nodiscard]] const char* semantic_name(VertexSemantic semantic) noexcept {
    switch (semantic) {
    case VertexSemantic::position:
        return "POSITION";
    case VertexSemantic::normal:
        return "NORMAL";
    case VertexSemantic::tangent:
        return "TANGENT";
    case VertexSemantic::texcoord:
    case VertexSemantic::custom:
        return "TEXCOORD";
    case VertexSemantic::color:
        return "COLOR";
    case VertexSemantic::joint_indices:
        return "BLENDINDICES";
    case VertexSemantic::joint_weights:
        return "BLENDWEIGHT";
    }
    return "TEXCOORD";
}

[[nodiscard]] D3D12_INPUT_CLASSIFICATION input_classification(VertexInputRate rate) noexcept {
    return rate == VertexInputRate::instance ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
                                             : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
}

[[nodiscard]] Format from_dxgi_format(DXGI_FORMAT format) noexcept {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return Format::rgba8_unorm;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return Format::bgra8_unorm;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
        return Format::depth24_stencil8;
    default:
        break;
    }
    return Format::unknown;
}

[[nodiscard]] DXGI_FORMAT to_dxgi_index_format(IndexFormat format) noexcept {
    switch (format) {
    case IndexFormat::uint16:
        return DXGI_FORMAT_R16_UINT;
    case IndexFormat::uint32:
        return DXGI_FORMAT_R32_UINT;
    case IndexFormat::unknown:
        break;
    }
    return DXGI_FORMAT_UNKNOWN;
}

[[nodiscard]] D3D12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE type) noexcept {
    D3D12_HEAP_PROPERTIES properties{};
    properties.Type = type;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;
    return properties;
}

[[nodiscard]] D3D12_RESOURCE_FLAGS committed_buffer_flags(BufferUsage usage) noexcept {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    if (has_flag(usage, BufferUsage::storage)) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    return flags;
}

[[nodiscard]] D3D12_RESOURCE_DESC buffer_resource_desc(std::uint64_t size_bytes,
                                                       D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) noexcept {
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = size_bytes;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = flags;
    return desc;
}

[[nodiscard]] DXGI_FORMAT d3d12_texture_resource_format(Format format, TextureUsage usage) noexcept {
    if (format == Format::depth24_stencil8 && has_flag(usage, TextureUsage::shader_resource)) {
        return DXGI_FORMAT_R24G8_TYPELESS;
    }
    return to_dxgi_format(format);
}

[[nodiscard]] D3D12_RESOURCE_DESC texture_resource_desc(Extent2D extent, Format format,
                                                        TextureUsage usage = TextureUsage::none) noexcept {
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = extent.width;
    desc.Height = extent.height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = d3d12_texture_resource_format(format, usage);
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    return desc;
}

[[nodiscard]] bool valid_resource_ownership_desc(const ResourceOwnershipDesc& desc) noexcept {
    return desc.upload_buffer_size_bytes > 0 && desc.readback_buffer_size_bytes > 0 && desc.texture_extent.width > 0 &&
           desc.texture_extent.height > 0 && to_dxgi_format(desc.texture_format) != DXGI_FORMAT_UNKNOWN;
}

[[nodiscard]] bool valid_buffer_desc(const BufferDesc& desc) noexcept {
    return desc.size_bytes > 0 && desc.usage != BufferUsage::none;
}

void validate_buffer_copy_region(const BufferDesc& source, const BufferDesc& destination,
                                 const BufferCopyRegion& region) {
    if (region.size_bytes == 0) {
        throw std::invalid_argument("d3d12 rhi buffer copy size must be non-zero");
    }
    if (region.source_offset > source.size_bytes || region.size_bytes > source.size_bytes - region.source_offset) {
        throw std::invalid_argument("d3d12 rhi buffer copy source range is outside the source buffer");
    }
    if (region.destination_offset > destination.size_bytes ||
        region.size_bytes > destination.size_bytes - region.destination_offset) {
        throw std::invalid_argument("d3d12 rhi buffer copy destination range is outside the destination buffer");
    }
}

void validate_buffer_texture_region(const TextureDesc& texture, const BufferTextureCopyRegion& region) {
    if (region.texture_extent.width == 0 || region.texture_extent.height == 0 || region.texture_extent.depth == 0) {
        throw std::invalid_argument("d3d12 rhi buffer texture copy extent must be non-zero");
    }
    if (region.texture_offset.x > texture.extent.width ||
        region.texture_extent.width > texture.extent.width - region.texture_offset.x) {
        throw std::invalid_argument("d3d12 rhi buffer texture copy width is outside the texture");
    }
    if (region.texture_offset.y > texture.extent.height ||
        region.texture_extent.height > texture.extent.height - region.texture_offset.y) {
        throw std::invalid_argument("d3d12 rhi buffer texture copy height is outside the texture");
    }
    if (region.texture_offset.z > texture.extent.depth ||
        region.texture_extent.depth > texture.extent.depth - region.texture_offset.z) {
        throw std::invalid_argument("d3d12 rhi buffer texture copy depth is outside the texture");
    }
}

[[nodiscard]] std::uint64_t checked_mul_u64(std::uint64_t lhs, std::uint64_t rhs, const char* message) {
    if (lhs != 0 && rhs > (std::numeric_limits<std::uint64_t>::max)() / lhs) {
        throw std::overflow_error(message);
    }
    return lhs * rhs;
}

[[nodiscard]] std::uint64_t checked_add_u64(std::uint64_t lhs, std::uint64_t rhs, const char* message) {
    if (rhs > (std::numeric_limits<std::uint64_t>::max)() - lhs) {
        throw std::overflow_error(message);
    }
    return lhs + rhs;
}

struct D3d12LinearTextureFootprint {
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed{};
    std::uint64_t required_bytes{0};
};

[[nodiscard]] D3d12LinearTextureFootprint d3d12_linear_texture_footprint(Format format,
                                                                         const BufferTextureCopyRegion& region) {
    const auto dxgi_format = to_dxgi_format(format);
    if (dxgi_format == DXGI_FORMAT_UNKNOWN) {
        throw std::invalid_argument("d3d12 rhi buffer texture copy format is unsupported");
    }
    if (region.buffer_offset % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT != 0) {
        throw std::invalid_argument("d3d12 rhi buffer texture copy offset must be 512-byte aligned");
    }

    const auto texel_bytes = static_cast<std::uint64_t>(bytes_per_texel(format));
    const auto row_length = region.buffer_row_length == 0 ? region.texture_extent.width : region.buffer_row_length;
    const auto image_height =
        region.buffer_image_height == 0 ? region.texture_extent.height : region.buffer_image_height;
    if (row_length < region.texture_extent.width) {
        throw std::invalid_argument("d3d12 rhi buffer texture copy row length is smaller than the copy width");
    }
    if (image_height < region.texture_extent.height) {
        throw std::invalid_argument("d3d12 rhi buffer texture copy image height is smaller than the copy height");
    }

    const auto row_pitch =
        checked_mul_u64(row_length, texel_bytes, "d3d12 rhi buffer texture copy row pitch overflowed");
    if (row_pitch == 0 || row_pitch > (std::numeric_limits<UINT>::max)() ||
        row_pitch % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT != 0) {
        throw std::invalid_argument("d3d12 rhi buffer texture copy row pitch must be 256-byte aligned");
    }

    const auto image_pitch =
        checked_mul_u64(image_height, row_pitch, "d3d12 rhi buffer texture copy image pitch overflowed");
    const auto copy_bytes =
        checked_mul_u64(region.texture_extent.depth, image_pitch, "d3d12 rhi buffer texture copy footprint overflowed");
    const auto required_bytes =
        checked_add_u64(region.buffer_offset, copy_bytes, "d3d12 rhi buffer texture copy footprint overflowed");

    D3d12LinearTextureFootprint footprint;
    footprint.placed.Offset = region.buffer_offset;
    footprint.placed.Footprint.Format = dxgi_format;
    footprint.placed.Footprint.Width = region.texture_extent.width;
    footprint.placed.Footprint.Height = region.texture_extent.height;
    footprint.placed.Footprint.Depth = region.texture_extent.depth;
    footprint.placed.Footprint.RowPitch = static_cast<UINT>(row_pitch);
    footprint.required_bytes = required_bytes;
    return footprint;
}

[[nodiscard]] bool valid_texture_desc(const TextureDesc& desc) noexcept {
    if (desc.extent.width == 0 || desc.extent.height == 0 || desc.extent.depth == 0 || desc.format == Format::unknown ||
        desc.usage == TextureUsage::none || to_dxgi_format(desc.format) == DXGI_FORMAT_UNKNOWN) {
        return false;
    }
    if (has_flag(desc.usage, TextureUsage::render_target) && !is_d3d12_color_render_format(desc.format)) {
        return false;
    }
    if (has_flag(desc.usage, TextureUsage::depth_stencil)) {
        constexpr auto sampled_depth_usage = TextureUsage::depth_stencil | TextureUsage::shader_resource;
        return is_d3d12_depth_stencil_format(desc.format) && desc.extent.depth == 1 &&
               (desc.usage == TextureUsage::depth_stencil || desc.usage == sampled_depth_usage);
    }
    return !is_d3d12_depth_stencil_format(desc.format);
}

[[nodiscard]] bool valid_sampler_desc(SamplerDesc desc) noexcept {
    const auto valid_filter = [](SamplerFilter filter) noexcept {
        return filter == SamplerFilter::nearest || filter == SamplerFilter::linear;
    };
    const auto valid_address = [](SamplerAddressMode mode) noexcept {
        return mode == SamplerAddressMode::repeat || mode == SamplerAddressMode::clamp_to_edge;
    };
    return valid_filter(desc.min_filter) && valid_filter(desc.mag_filter) && valid_address(desc.address_u) &&
           valid_address(desc.address_v) && valid_address(desc.address_w);
}

[[nodiscard]] D3D12_HEAP_FLAGS committed_texture_heap_flags(TextureUsage usage) noexcept {
    if (has_flag(usage, TextureUsage::shared)) {
        return D3D12_HEAP_FLAG_SHARED;
    }
    return D3D12_HEAP_FLAG_NONE;
}

[[nodiscard]] D3D12_HEAP_FLAGS placed_texture_heap_flags(TextureUsage usage) noexcept {
    if (has_flag(usage, TextureUsage::render_target) || has_flag(usage, TextureUsage::depth_stencil)) {
        return D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
    }
    return D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
}

[[nodiscard]] bool valid_swapchain_desc(const NativeSwapchainDesc& desc) noexcept {
    const auto format = to_dxgi_format(desc.swapchain.format);
    return desc.window.value != nullptr && desc.swapchain.extent.width > 0 && desc.swapchain.extent.height > 0 &&
           desc.swapchain.buffer_count >= 2 && desc.swapchain.buffer_count <= 16 &&
           (format == DXGI_FORMAT_R8G8B8A8_UNORM || format == DXGI_FORMAT_B8G8R8A8_UNORM);
}

[[nodiscard]] bool valid_descriptor_heap_kind(NativeDescriptorHeapKind kind) noexcept {
    switch (kind) {
    case NativeDescriptorHeapKind::cbv_srv_uav:
    case NativeDescriptorHeapKind::sampler:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_descriptor_heap_desc(const NativeDescriptorHeapDesc& desc) noexcept {
    return valid_descriptor_heap_kind(desc.kind) && desc.capacity > 0;
}

[[nodiscard]] HWND to_hwnd(NativeWindowHandle handle) noexcept {
    return static_cast<HWND>(handle.value);
}

[[nodiscard]] D3D12_DESCRIPTOR_HEAP_TYPE descriptor_heap_type(NativeDescriptorHeapKind kind) noexcept {
    switch (kind) {
    case NativeDescriptorHeapKind::cbv_srv_uav:
        return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    case NativeDescriptorHeapKind::sampler:
        return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    }
    return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
}

[[nodiscard]] D3D12_HEAP_TYPE committed_buffer_heap_type(BufferUsage usage) noexcept {
    if (has_flag(usage, BufferUsage::storage)) {
        return D3D12_HEAP_TYPE_DEFAULT;
    }
    if (has_flag(usage, BufferUsage::copy_source)) {
        return D3D12_HEAP_TYPE_UPLOAD;
    }
    if (usage == BufferUsage::copy_destination) {
        return D3D12_HEAP_TYPE_READBACK;
    }
    return D3D12_HEAP_TYPE_DEFAULT;
}

[[nodiscard]] D3D12_RESOURCE_STATES committed_buffer_initial_state(D3D12_HEAP_TYPE heap_type) noexcept {
    if (heap_type == D3D12_HEAP_TYPE_UPLOAD) {
        return D3D12_RESOURCE_STATE_GENERIC_READ;
    }
    if (heap_type == D3D12_HEAP_TYPE_READBACK) {
        return D3D12_RESOURCE_STATE_COPY_DEST;
    }
    return D3D12_RESOURCE_STATE_COMMON;
}

[[nodiscard]] D3D12_RESOURCE_STATES committed_texture_initial_state(TextureUsage usage) noexcept {
    if (has_flag(usage, TextureUsage::copy_destination)) {
        return D3D12_RESOURCE_STATE_COPY_DEST;
    }
    if (has_flag(usage, TextureUsage::copy_source)) {
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    if (has_flag(usage, TextureUsage::render_target)) {
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    if (has_flag(usage, TextureUsage::depth_stencil)) {
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    return D3D12_RESOURCE_STATE_COMMON;
}

[[nodiscard]] ResourceState initial_texture_state(TextureUsage usage) noexcept {
    if (has_flag(usage, TextureUsage::copy_destination)) {
        return ResourceState::copy_destination;
    }
    if (has_flag(usage, TextureUsage::copy_source)) {
        return ResourceState::copy_source;
    }
    if (has_flag(usage, TextureUsage::render_target)) {
        return ResourceState::render_target;
    }
    if (has_flag(usage, TextureUsage::depth_stencil)) {
        return ResourceState::depth_write;
    }
    if (has_flag(usage, TextureUsage::present)) {
        return ResourceState::present;
    }
    if (has_flag(usage, TextureUsage::shader_resource)) {
        return ResourceState::shader_read;
    }
    return ResourceState::undefined;
}

[[nodiscard]] D3D12_RESOURCE_FLAGS committed_texture_flags(TextureUsage usage) noexcept {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    if (has_flag(usage, TextureUsage::render_target)) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (has_flag(usage, TextureUsage::depth_stencil)) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (has_flag(usage, TextureUsage::storage)) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    return flags;
}

[[nodiscard]] D3D12_RESOURCE_STATES to_d3d12_resource_state(ResourceState state) noexcept {
    switch (state) {
    case ResourceState::copy_source:
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case ResourceState::copy_destination:
        return D3D12_RESOURCE_STATE_COPY_DEST;
    case ResourceState::shader_read:
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case ResourceState::render_target:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case ResourceState::depth_write:
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case ResourceState::present:
        return D3D12_RESOURCE_STATE_PRESENT;
    case ResourceState::undefined:
        break;
    }
    return D3D12_RESOURCE_STATE_COMMON;
}

[[nodiscard]] bool valid_shader_stage(ShaderStage stage) noexcept {
    switch (stage) {
    case ShaderStage::vertex:
    case ShaderStage::fragment:
    case ShaderStage::compute:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_shader_bytecode_desc(const NativeShaderBytecodeDesc& desc) noexcept {
    return valid_shader_stage(desc.stage) && desc.bytecode != nullptr && desc.bytecode_size > 0 &&
           desc.bytecode_size <= static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)());
}

[[nodiscard]] D3D12_PRIMITIVE_TOPOLOGY_TYPE to_d3d12_primitive_topology_type(PrimitiveTopology topology) noexcept {
    switch (topology) {
    case PrimitiveTopology::triangle_list:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case PrimitiveTopology::line_list:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    }
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

[[nodiscard]] D3D_PRIMITIVE_TOPOLOGY to_d3d_primitive_topology(PrimitiveTopology topology) noexcept {
    switch (topology) {
    case PrimitiveTopology::triangle_list:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case PrimitiveTopology::line_list:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    }
    return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

[[nodiscard]] bool valid_resource_transition(ResourceState before, ResourceState after) noexcept {
    return before != ResourceState::undefined && after != ResourceState::undefined && before != after;
}

[[nodiscard]] bool valid_descriptor_type(DescriptorType type) noexcept {
    switch (type) {
    case DescriptorType::uniform_buffer:
    case DescriptorType::storage_buffer:
    case DescriptorType::sampled_texture:
    case DescriptorType::storage_texture:
    case DescriptorType::sampler:
        return true;
    }
    return false;
}

[[nodiscard]] D3D12_DESCRIPTOR_RANGE_TYPE descriptor_range_type(DescriptorType type) noexcept {
    switch (type) {
    case DescriptorType::uniform_buffer:
        return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case DescriptorType::storage_buffer:
    case DescriptorType::storage_texture:
        return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    case DescriptorType::sampled_texture:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case DescriptorType::sampler:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    }
    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
}

[[nodiscard]] NativeDescriptorHeapKind descriptor_heap_kind_for_type(DescriptorType type) noexcept {
    if (type == DescriptorType::sampler) {
        return NativeDescriptorHeapKind::sampler;
    }
    return NativeDescriptorHeapKind::cbv_srv_uav;
}

[[nodiscard]] bool descriptor_type_matches_heap(DescriptorType type, NativeDescriptorHeapKind kind) noexcept {
    return descriptor_heap_kind_for_type(type) == kind;
}

[[nodiscard]] bool descriptor_set_has_heap_kind(const DescriptorSetLayoutDesc& layout,
                                                NativeDescriptorHeapKind kind) noexcept {
    return std::ranges::any_of(
        layout.bindings, [kind](const auto& binding) { return descriptor_type_matches_heap(binding.type, kind); });
}

[[nodiscard]] D3D12_FILTER to_d3d12_filter(SamplerDesc desc) noexcept {
    if (desc.min_filter == SamplerFilter::nearest && desc.mag_filter == SamplerFilter::nearest) {
        return D3D12_FILTER_MIN_MAG_MIP_POINT;
    }
    if (desc.min_filter == SamplerFilter::nearest && desc.mag_filter == SamplerFilter::linear) {
        return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
    }
    if (desc.min_filter == SamplerFilter::linear && desc.mag_filter == SamplerFilter::nearest) {
        return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    }
    return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
}

[[nodiscard]] D3D12_TEXTURE_ADDRESS_MODE to_d3d12_address_mode(SamplerAddressMode mode) noexcept {
    switch (mode) {
    case SamplerAddressMode::repeat:
        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case SamplerAddressMode::clamp_to_edge:
        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    }
    return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
}

[[nodiscard]] D3D12_SAMPLER_DESC to_d3d12_sampler_desc(SamplerDesc desc) noexcept {
    D3D12_SAMPLER_DESC sampler{};
    sampler.Filter = to_d3d12_filter(desc);
    sampler.AddressU = to_d3d12_address_mode(desc.address_u);
    sampler.AddressV = to_d3d12_address_mode(desc.address_v);
    sampler.AddressW = to_d3d12_address_mode(desc.address_w);
    sampler.MipLODBias = 0.0F;
    sampler.MaxAnisotropy = 1;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor[0] = 0.0F;
    sampler.BorderColor[1] = 0.0F;
    sampler.BorderColor[2] = 0.0F;
    sampler.BorderColor[3] = 0.0F;
    sampler.MinLOD = 0.0F;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    return sampler;
}

[[nodiscard]] bool visibility_has(ShaderStageVisibility value, ShaderStageVisibility flag) noexcept {
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0U;
}

[[nodiscard]] D3D12_SHADER_VISIBILITY shader_visibility_for_table(const DescriptorSetLayoutDesc& layout,
                                                                  NativeDescriptorHeapKind kind) noexcept {
    std::uint32_t combined = 0;
    for (const auto& binding : layout.bindings) {
        if (descriptor_type_matches_heap(binding.type, kind)) {
            combined |= static_cast<std::uint32_t>(binding.stages);
        }
    }

    if (combined == static_cast<std::uint32_t>(ShaderStageVisibility::vertex)) {
        return D3D12_SHADER_VISIBILITY_VERTEX;
    }
    if (combined == static_cast<std::uint32_t>(ShaderStageVisibility::fragment)) {
        return D3D12_SHADER_VISIBILITY_PIXEL;
    }

    return D3D12_SHADER_VISIBILITY_ALL;
}

[[nodiscard]] bool valid_descriptor_set_layout_desc(const DescriptorSetLayoutDesc& desc) noexcept {
    if (desc.bindings.empty()) {
        return false;
    }

    for (const auto& binding : desc.bindings) {
        if (!valid_descriptor_type(binding.type) || binding.count == 0 || !has_stage_visibility(binding.stages)) {
            return false;
        }
        const auto duplicates = std::ranges::count_if(
            desc.bindings, [&binding](const DescriptorBindingDesc& other) { return other.binding == binding.binding; });
        if (duplicates > 1) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool valid_root_signature_desc(const NativeRootSignatureDesc& desc) noexcept {
    if (desc.push_constant_bytes > 256 || desc.push_constant_bytes % 4U != 0U) {
        return false;
    }
    for (const auto& descriptor_set : desc.descriptor_sets) {
        if (!valid_descriptor_set_layout_desc(descriptor_set)) {
            return false;
        }
        for (const auto& binding : descriptor_set.bindings) {
            if (visibility_has(binding.stages, ShaderStageVisibility::compute) &&
                binding.stages != ShaderStageVisibility::compute) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] bool valid_rhi_pipeline_layout_desc(const PipelineLayoutDesc& desc) noexcept {
    return desc.push_constant_bytes <= 256 && desc.push_constant_bytes % 4U == 0U;
}

[[nodiscard]] bool valid_vertex_input_desc(const std::vector<VertexBufferLayoutDesc>& vertex_buffers,
                                           const std::vector<VertexAttributeDesc>& vertex_attributes) noexcept {
    for (const auto& layout : vertex_buffers) {
        if (layout.stride == 0) {
            return false;
        }
        const auto duplicates =
            std::ranges::count_if(vertex_buffers, [&layout](const VertexBufferLayoutDesc& candidate) {
                return candidate.binding == layout.binding;
            });
        if (duplicates > 1) {
            return false;
        }
    }

    for (const auto& attribute : vertex_attributes) {
        if (to_dxgi_vertex_format(attribute.format) == DXGI_FORMAT_UNKNOWN) {
            return false;
        }
        const auto layout = std::ranges::find_if(vertex_buffers, [&attribute](const VertexBufferLayoutDesc& candidate) {
            return candidate.binding == attribute.binding;
        });
        if (layout == vertex_buffers.end()) {
            return false;
        }
        const auto duplicates =
            std::ranges::count_if(vertex_attributes, [&attribute](const VertexAttributeDesc& candidate) {
                return candidate.location == attribute.location;
            });
        if (duplicates > 1) {
            return false;
        }
        const auto size = vertex_format_size(attribute.format);
        if (size == 0 || attribute.offset > layout->stride || size > layout->stride - attribute.offset) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool valid_rhi_compare_op(CompareOp op) noexcept {
    switch (op) {
    case CompareOp::never:
    case CompareOp::less:
    case CompareOp::equal:
    case CompareOp::less_equal:
    case CompareOp::greater:
    case CompareOp::not_equal:
    case CompareOp::greater_equal:
    case CompareOp::always:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_depth_state_for_format(Format depth_format, DepthStencilStateDesc depth_state) noexcept {
    if (!valid_rhi_compare_op(depth_state.depth_compare)) {
        return false;
    }
    if (depth_state.depth_write_enabled && !depth_state.depth_test_enabled) {
        return false;
    }
    if (depth_format != Format::unknown && !is_d3d12_depth_stencil_format(depth_format)) {
        return false;
    }
    if ((depth_state.depth_test_enabled || depth_state.depth_write_enabled) && depth_format == Format::unknown) {
        return false;
    }
    return true;
}

[[nodiscard]] bool valid_rhi_depth_state_desc(const GraphicsPipelineDesc& desc) noexcept {
    return valid_depth_state_for_format(desc.depth_format, desc.depth_state);
}

[[nodiscard]] bool valid_rhi_graphics_pipeline_desc(const GraphicsPipelineDesc& desc) noexcept {
    return desc.layout.value != 0 && desc.vertex_shader.value != 0 && desc.fragment_shader.value != 0 &&
           is_d3d12_color_render_format(desc.color_format) && valid_rhi_depth_state_desc(desc) &&
           to_d3d12_primitive_topology_type(desc.topology) != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED &&
           valid_vertex_input_desc(desc.vertex_buffers, desc.vertex_attributes);
}

[[nodiscard]] bool valid_rhi_compute_pipeline_desc(const ComputePipelineDesc& desc) noexcept {
    return desc.layout.value != 0 && desc.compute_shader.value != 0;
}

[[nodiscard]] D3D12_BLEND_DESC default_blend_desc() noexcept {
    D3D12_BLEND_DESC desc{};
    desc.AlphaToCoverageEnable = FALSE;
    desc.IndependentBlendEnable = FALSE;
    for (auto& target : desc.RenderTarget) {
        target.BlendEnable = FALSE;
        target.LogicOpEnable = FALSE;
        target.SrcBlend = D3D12_BLEND_ONE;
        target.DestBlend = D3D12_BLEND_ZERO;
        target.BlendOp = D3D12_BLEND_OP_ADD;
        target.SrcBlendAlpha = D3D12_BLEND_ONE;
        target.DestBlendAlpha = D3D12_BLEND_ZERO;
        target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        target.LogicOp = D3D12_LOGIC_OP_NOOP;
        target.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    return desc;
}

[[nodiscard]] D3D12_RASTERIZER_DESC default_rasterizer_desc() noexcept {
    D3D12_RASTERIZER_DESC desc{};
    desc.FillMode = D3D12_FILL_MODE_SOLID;
    desc.CullMode = D3D12_CULL_MODE_NONE;
    desc.FrontCounterClockwise = FALSE;
    desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    desc.DepthClipEnable = TRUE;
    desc.MultisampleEnable = FALSE;
    desc.AntialiasedLineEnable = FALSE;
    desc.ForcedSampleCount = 0;
    desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    return desc;
}

[[nodiscard]] D3D12_DEPTH_STENCILOP_DESC disabled_stencil_op_desc() noexcept {
    D3D12_DEPTH_STENCILOP_DESC desc{};
    desc.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    desc.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    desc.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    desc.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    return desc;
}

[[nodiscard]] D3D12_DEPTH_STENCIL_DESC disabled_depth_stencil_desc() noexcept {
    D3D12_DEPTH_STENCIL_DESC desc{};
    desc.DepthEnable = FALSE;
    desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    desc.StencilEnable = FALSE;
    desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    desc.FrontFace = disabled_stencil_op_desc();
    desc.BackFace = disabled_stencil_op_desc();
    return desc;
}

[[nodiscard]] D3D12_COMPARISON_FUNC to_d3d12_comparison_func(CompareOp op) noexcept {
    switch (op) {
    case CompareOp::never:
        return D3D12_COMPARISON_FUNC_NEVER;
    case CompareOp::less:
        return D3D12_COMPARISON_FUNC_LESS;
    case CompareOp::equal:
        return D3D12_COMPARISON_FUNC_EQUAL;
    case CompareOp::less_equal:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case CompareOp::greater:
        return D3D12_COMPARISON_FUNC_GREATER;
    case CompareOp::not_equal:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case CompareOp::greater_equal:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case CompareOp::always:
        return D3D12_COMPARISON_FUNC_ALWAYS;
    }
    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
}

[[nodiscard]] D3D12_DEPTH_STENCIL_DESC to_d3d12_depth_stencil_desc(DepthStencilStateDesc depth_state) noexcept {
    auto desc = disabled_depth_stencil_desc();
    if (!depth_state.depth_test_enabled) {
        return desc;
    }

    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = depth_state.depth_write_enabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = to_d3d12_comparison_func(depth_state.depth_compare);
    return desc;
}

[[nodiscard]] bool valid_queue_kind(QueueKind queue) noexcept {
    switch (queue) {
    case QueueKind::graphics:
    case QueueKind::compute:
    case QueueKind::copy:
        return true;
    }
    return false;
}

void record_queue_submit(DeviceContextStats& stats, QueueKind queue, FenceValue fence) noexcept {
    ++stats.queue_event_sequence;
    stats.last_submitted_fence_queue = fence.queue;
    switch (queue) {
    case QueueKind::graphics:
        ++stats.graphics_queue_submits;
        stats.last_graphics_submitted_fence_value = fence.value;
        stats.last_graphics_submit_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::compute:
        ++stats.compute_queue_submits;
        stats.last_compute_submitted_fence_value = fence.value;
        stats.last_compute_submit_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::copy:
        ++stats.copy_queue_submits;
        stats.last_copy_submitted_fence_value = fence.value;
        stats.last_copy_submit_sequence = stats.queue_event_sequence;
        return;
    }
}

void record_queue_submit(RhiStats& stats, QueueKind queue, FenceValue fence) noexcept {
    ++stats.queue_event_sequence;
    stats.last_submitted_fence_queue = fence.queue;
    switch (queue) {
    case QueueKind::graphics:
        ++stats.graphics_queue_submits;
        stats.last_graphics_submitted_fence_value = fence.value;
        stats.last_graphics_submit_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::compute:
        ++stats.compute_queue_submits;
        stats.last_compute_submitted_fence_value = fence.value;
        stats.last_compute_submit_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::copy:
        ++stats.copy_queue_submits;
        stats.last_copy_submitted_fence_value = fence.value;
        stats.last_copy_submit_sequence = stats.queue_event_sequence;
        return;
    }
}

void record_queue_wait(DeviceContextStats& stats, QueueKind queue, FenceValue fence) noexcept {
    ++stats.queue_event_sequence;
    stats.last_queue_wait_fence_value = fence.value;
    stats.last_queue_wait_fence_queue = fence.queue;
    stats.last_queue_wait_sequence = stats.queue_event_sequence;
    switch (queue) {
    case QueueKind::graphics:
        stats.last_graphics_queue_wait_fence_value = fence.value;
        stats.last_graphics_queue_wait_fence_queue = fence.queue;
        stats.last_graphics_queue_wait_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::compute:
        stats.last_compute_queue_wait_fence_value = fence.value;
        stats.last_compute_queue_wait_fence_queue = fence.queue;
        stats.last_compute_queue_wait_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::copy:
        stats.last_copy_queue_wait_fence_value = fence.value;
        stats.last_copy_queue_wait_fence_queue = fence.queue;
        stats.last_copy_queue_wait_sequence = stats.queue_event_sequence;
        return;
    }
}

void record_queue_wait(RhiStats& stats, QueueKind queue, FenceValue fence) noexcept {
    ++stats.queue_event_sequence;
    stats.last_queue_wait_fence_value = fence.value;
    stats.last_queue_wait_fence_queue = fence.queue;
    stats.last_queue_wait_sequence = stats.queue_event_sequence;
    switch (queue) {
    case QueueKind::graphics:
        stats.last_graphics_queue_wait_fence_value = fence.value;
        stats.last_graphics_queue_wait_fence_queue = fence.queue;
        stats.last_graphics_queue_wait_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::compute:
        stats.last_compute_queue_wait_fence_value = fence.value;
        stats.last_compute_queue_wait_fence_queue = fence.queue;
        stats.last_compute_queue_wait_sequence = stats.queue_event_sequence;
        return;
    case QueueKind::copy:
        stats.last_copy_queue_wait_fence_value = fence.value;
        stats.last_copy_queue_wait_fence_queue = fence.queue;
        stats.last_copy_queue_wait_sequence = stats.queue_event_sequence;
        return;
    }
}

[[nodiscard]] D3D12_COMMAND_LIST_TYPE command_list_type(QueueKind queue) noexcept {
    switch (queue) {
    case QueueKind::graphics:
        return D3D12_COMMAND_LIST_TYPE_DIRECT;
    case QueueKind::compute:
        return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    case QueueKind::copy:
        return D3D12_COMMAND_LIST_TYPE_COPY;
    }
    return D3D12_COMMAND_LIST_TYPE_DIRECT;
}

class Win32Event final {
  public:
    Win32Event(const Win32Event&) = delete;
    Win32Event& operator=(const Win32Event&) = delete;

    Win32Event() noexcept : handle_(CreateEventW(nullptr, FALSE, FALSE, nullptr)) {}

    ~Win32Event() {
        if (handle_ != nullptr) {
            CloseHandle(handle_);
        }
    }

    [[nodiscard]] HANDLE get() const noexcept {
        return handle_;
    }

  private:
    HANDLE handle_{nullptr};
};

} // namespace

// PIX / GPU-based validation listings: `ID3D12Object::SetName` (Microsoft D3D12 guidance). Placed outside the
// file-local anonymous namespace so nested `DeviceContext::Impl` inline member definitions can call them.
static void d3d12_set_object_name(ID3D12Object* object, const wchar_t* name) noexcept {
    if (object == nullptr || name == nullptr) {
        return;
    }
    (void)object->SetName(name);
}

static void d3d12_set_object_name_fmt(ID3D12Object* object, const wchar_t* format, ...) noexcept {
    if (object == nullptr || format == nullptr) {
        return;
    }
    std::array<wchar_t, 256> buffer{};
    std::va_list args{};
    va_start(args, format);
    (void)_vsnwprintf_s(buffer.data(), buffer.size(), _TRUNCATE, format, args);
    va_end(args);
    (void)object->SetName(buffer.data());
}

struct PlacedResourceStateUpdate {
    NativeResourceHandle before;
    NativeResourceHandle after;
};

struct CommandListRecord {
    QueueKind queue{QueueKind::graphics};
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
    Microsoft::WRL::ComPtr<ID3D12QueryHeap> submitted_timing_query_heap;
    Microsoft::WRL::ComPtr<ID3D12Resource> submitted_timing_readback;
    FenceValue submitted_fence;
    QueueClockCalibration submitted_timing_calibration_before;
    NativeRootSignatureHandle graphics_root_signature;
    NativeRootSignatureHandle compute_root_signature;
    NativeDescriptorHeapBinding descriptor_heaps;
    NativeGraphicsPipelineHandle graphics_pipeline;
    NativeComputePipelineHandle compute_pipeline;
    std::vector<PlacedResourceStateUpdate> placed_resource_state_updates;
    bool render_target_set{false};
    bool closed{false};
    bool submitted{false};
    bool submitted_timing_begin_recorded{false};
    bool submitted_timing_resolved{false};
    std::uint64_t submitted_timing_frequency{0};
    SubmittedCommandCalibratedTimingStatus submitted_timing_status{SubmittedCommandCalibratedTimingStatus::unsupported};
    std::string_view submitted_timing_diagnostic;
    std::uint32_t gpu_debug_scope_depth{0};
};

namespace {

constexpr UINT k_submitted_command_timestamp_count = 2U;
constexpr std::uint64_t k_submitted_command_timestamp_readback_size =
    sizeof(std::uint64_t) * k_submitted_command_timestamp_count;

void reset_submitted_command_timing_state(CommandListRecord& record) noexcept {
    record.submitted_timing_calibration_before = QueueClockCalibration{};
    record.submitted_timing_begin_recorded = false;
    record.submitted_timing_resolved = false;
    if (record.submitted_timing_query_heap != nullptr && record.submitted_timing_readback != nullptr) {
        record.submitted_timing_status = SubmittedCommandCalibratedTimingStatus::not_submitted;
        record.submitted_timing_diagnostic = "d3d12 submitted command calibrated timing pending submission";
    }
}

bool record_submitted_command_timing_begin(CommandListRecord& record) noexcept {
    if (record.list == nullptr || record.submitted_timing_query_heap == nullptr ||
        record.submitted_timing_readback == nullptr) {
        return false;
    }

    record.list->EndQuery(record.submitted_timing_query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
    record.submitted_timing_begin_recorded = true;
    record.submitted_timing_resolved = false;
    record.submitted_timing_status = SubmittedCommandCalibratedTimingStatus::not_submitted;
    record.submitted_timing_diagnostic = "d3d12 submitted command calibrated timing pending submission";
    return true;
}

bool resolve_submitted_command_timing(CommandListRecord& record) noexcept {
    if (!record.submitted_timing_begin_recorded || record.list == nullptr ||
        record.submitted_timing_query_heap == nullptr || record.submitted_timing_readback == nullptr) {
        return false;
    }

    record.list->EndQuery(record.submitted_timing_query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
    record.list->ResolveQueryData(record.submitted_timing_query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0,
                                  k_submitted_command_timestamp_count, record.submitted_timing_readback.Get(), 0);
    record.submitted_timing_resolved = true;
    return true;
}

} // namespace

struct SwapchainRecord {
    HWND hwnd{nullptr};
    SwapchainDesc desc;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapchain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> back_buffers;
    std::uint32_t rtv_descriptor_size{0};
    /// Matches `NativeSwapchainHandle::value` (1-based). Used for stable SetName labels on back buffers and RTV heaps.
    std::uint32_t native_handle_value{0};
};

struct ResourceRenderTargetViewRecord {
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
    std::uint32_t descriptor_size{0};
};

struct ResourceDepthStencilViewRecord {
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
    std::uint32_t descriptor_size{0};
};

struct DescriptorHeapRecord {
    NativeDescriptorHeapKind kind{NativeDescriptorHeapKind::cbv_srv_uav};
    std::uint32_t capacity{0};
    bool shader_visible{false};
    std::uint32_t descriptor_size{0};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
};

struct RootSignatureRecord {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
    NativeRootSignatureInfo info;
    std::vector<NativeDescriptorHeapKind> table_heap_kinds;
};

struct ShaderRecord {
    ShaderStage stage{ShaderStage::vertex};
    std::vector<std::uint8_t> bytecode;
};

struct GraphicsPipelineRecord {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
    NativeGraphicsPipelineInfo info;
    NativeShaderHandle vertex_shader;
    NativeShaderHandle fragment_shader;
};

struct ComputePipelineRecord {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
    NativeComputePipelineInfo info;
    NativeShaderHandle compute_shader;
};

struct DeviceContext::Impl {
    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandQueue>, 3> command_queues;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    std::array<Microsoft::WRL::ComPtr<ID3D12Fence>, 3> queue_fences;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resources;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Heap>> resource_heaps;
    std::vector<std::uint64_t> resource_alias_group_ids;
    std::vector<bool> resource_placed_active;
    std::vector<ResourceRenderTargetViewRecord> resource_rtvs;
    std::vector<ResourceDepthStencilViewRecord> resource_dsvs;
    std::vector<SwapchainRecord> swapchains;
    std::vector<DescriptorHeapRecord> descriptor_heaps;
    std::vector<RootSignatureRecord> root_signatures;
    std::vector<ShaderRecord> shaders;
    std::vector<GraphicsPipelineRecord> graphics_pipelines;
    std::vector<ComputePipelineRecord> compute_pipelines;
    std::vector<CommandListRecord> command_lists;
    DeviceContextStats stats;
    std::uint64_t next_fence_value{0};
    std::array<std::uint64_t, 3> next_queue_fence_values{};
    std::uint64_t next_resource_alias_group_id{0};
    FenceValue last_submitted_fence;
    bool used_warp{false};
    bool debug_layer_enabled{false};

    [[nodiscard]] static Microsoft::WRL::ComPtr<ID3D12CommandQueue>&
    queue_command_queue(std::array<Microsoft::WRL::ComPtr<ID3D12CommandQueue>, 3>& queues, QueueKind queue) noexcept {
        switch (queue) {
        case QueueKind::graphics:
            return queues[0];
        case QueueKind::compute:
            return queues[1];
        case QueueKind::copy:
            return queues[2];
        }
        return queues[0];
    }

    [[nodiscard]] static Microsoft::WRL::ComPtr<ID3D12Fence>&
    queue_fence(std::array<Microsoft::WRL::ComPtr<ID3D12Fence>, 3>& fences, QueueKind queue) noexcept {
        switch (queue) {
        case QueueKind::graphics:
            return fences[0];
        case QueueKind::compute:
            return fences[1];
        case QueueKind::copy:
            return fences[2];
        }
        return fences[0];
    }

    [[nodiscard]] static std::uint64_t& queue_fence_value(std::array<std::uint64_t, 3>& values,
                                                          QueueKind queue) noexcept {
        switch (queue) {
        case QueueKind::graphics:
            return values[0];
        case QueueKind::compute:
            return values[1];
        case QueueKind::copy:
            return values[2];
        }
        return values[0];
    }

    [[nodiscard]] static const std::uint64_t& queue_fence_value(const std::array<std::uint64_t, 3>& values,
                                                                QueueKind queue) noexcept {
        switch (queue) {
        case QueueKind::graphics:
            return values[0];
        case QueueKind::compute:
            return values[1];
        case QueueKind::copy:
            return values[2];
        }
        return values[0];
    }

    [[nodiscard]] ID3D12CommandQueue* ensure_queue(QueueKind queue) {
        if (device == nullptr || !valid_queue_kind(queue)) {
            return nullptr;
        }

        auto& queue_com_ptr = queue_command_queue(command_queues, queue);
        if (queue_com_ptr != nullptr) {
            return queue_com_ptr.Get();
        }

        D3D12_COMMAND_QUEUE_DESC queue_desc{};
        queue_desc.Type = command_list_type(queue);
        queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queue_desc.NodeMask = 0;
        if (FAILED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue_com_ptr)))) {
            return nullptr;
        }

        if (queue == QueueKind::compute) {
            d3d12_set_object_name(queue_com_ptr.Get(), L"GameEngine.RHI.D3D12.ComputeQueue");
        } else if (queue == QueueKind::copy) {
            d3d12_set_object_name(queue_com_ptr.Get(), L"GameEngine.RHI.D3D12.CopyQueue");
        }

        return queue_com_ptr.Get();
    }

    [[nodiscard]] ID3D12Fence* ensure_fence(QueueKind queue) {
        if (device == nullptr || !valid_queue_kind(queue)) {
            return nullptr;
        }

        auto& queue_fence_slot = queue_fence(queue_fences, queue);
        if (queue_fence_slot != nullptr) {
            return queue_fence_slot.Get();
        }

        if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&queue_fence_slot)))) {
            return nullptr;
        }

        if (queue == QueueKind::compute) {
            d3d12_set_object_name(queue_fence_slot.Get(), L"GameEngine.RHI.D3D12.ComputeFence");
        } else if (queue == QueueKind::copy) {
            d3d12_set_object_name(queue_fence_slot.Get(), L"GameEngine.RHI.D3D12.CopyFence");
        }

        return queue_fence_slot.Get();
    }

    [[nodiscard]] ID3D12Resource* resource_record(NativeResourceHandle handle) noexcept {
        if (handle.value == 0 || handle.value > resources.size()) {
            return nullptr;
        }
        return resources[handle.value - 1U].Get();
    }

    [[nodiscard]] const ID3D12Resource* resource_record(NativeResourceHandle handle) const noexcept {
        if (handle.value == 0 || handle.value > resources.size()) {
            return nullptr;
        }
        return resources[handle.value - 1U].Get();
    }

    [[nodiscard]] bool resource_is_placed(NativeResourceHandle handle) const noexcept {
        if (handle.value == 0 || handle.value > resource_heaps.size()) {
            return false;
        }
        return resource_heaps[handle.value - 1U] != nullptr;
    }

    [[nodiscard]] bool resources_share_placed_alias_group(NativeResourceHandle lhs,
                                                          NativeResourceHandle rhs) const noexcept {
        if (lhs.value == 0 || rhs.value == 0 || lhs.value > resource_heaps.size() ||
            rhs.value > resource_heaps.size() || lhs.value > resource_alias_group_ids.size() ||
            rhs.value > resource_alias_group_ids.size()) {
            return false;
        }
        const std::uint64_t lhs_group = resource_alias_group_ids[lhs.value - 1U];
        const std::uint64_t rhs_group = resource_alias_group_ids[rhs.value - 1U];
        ID3D12Heap* lhs_heap = resource_heaps[lhs.value - 1U].Get();
        ID3D12Heap* rhs_heap = resource_heaps[rhs.value - 1U].Get();
        return lhs_group != 0 && lhs_group == rhs_group && lhs_heap != nullptr && lhs_heap == rhs_heap;
    }

    [[nodiscard]] bool resource_is_placed_active(NativeResourceHandle handle) const noexcept {
        if (handle.value == 0 || handle.value > resource_placed_active.size()) {
            return false;
        }
        return resource_placed_active[handle.value - 1U];
    }

    void set_resource_placed_active(NativeResourceHandle handle, bool active) noexcept {
        if (handle.value == 0 || handle.value > resource_placed_active.size()) {
            return;
        }
        resource_placed_active[handle.value - 1U] = active;
    }

    [[nodiscard]] ResourceRenderTargetViewRecord* resource_rtv_record(NativeResourceHandle handle) noexcept {
        if (handle.value == 0 || handle.value > resource_rtvs.size()) {
            return nullptr;
        }
        auto& record = resource_rtvs[handle.value - 1U];
        if (record.heap == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] ResourceDepthStencilViewRecord* resource_dsv_record(NativeResourceHandle handle) noexcept {
        if (handle.value == 0 || handle.value > resource_dsvs.size()) {
            return nullptr;
        }
        auto& record = resource_dsvs[handle.value - 1U];
        if (record.heap == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] CommandListRecord* command_list(NativeCommandListHandle handle) noexcept {
        if (handle.value == 0 || handle.value > command_lists.size()) {
            return nullptr;
        }
        auto& record = command_lists[handle.value - 1U];
        if (record.allocator == nullptr || record.list == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] SwapchainRecord* swapchain_record(NativeSwapchainHandle handle) noexcept {
        if (handle.value == 0 || handle.value > swapchains.size()) {
            return nullptr;
        }
        auto& record = swapchains[handle.value - 1U];
        if (record.swapchain == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] const SwapchainRecord* swapchain_record(NativeSwapchainHandle handle) const noexcept {
        if (handle.value == 0 || handle.value > swapchains.size()) {
            return nullptr;
        }
        const auto& record = swapchains[handle.value - 1U];
        if (record.swapchain == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] DescriptorHeapRecord* descriptor_heap_record(NativeDescriptorHeapHandle handle) noexcept {
        if (handle.value == 0 || handle.value > descriptor_heaps.size()) {
            return nullptr;
        }
        auto& record = descriptor_heaps[handle.value - 1U];
        if (record.heap == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] const DescriptorHeapRecord* descriptor_heap_record(NativeDescriptorHeapHandle handle) const noexcept {
        if (handle.value == 0 || handle.value > descriptor_heaps.size()) {
            return nullptr;
        }
        const auto& record = descriptor_heaps[handle.value - 1U];
        if (record.heap == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] RootSignatureRecord* root_signature_record(NativeRootSignatureHandle handle) noexcept {
        if (handle.value == 0 || handle.value > root_signatures.size()) {
            return nullptr;
        }
        auto& record = root_signatures[handle.value - 1U];
        if (record.root_signature == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] const RootSignatureRecord* root_signature_record(NativeRootSignatureHandle handle) const noexcept {
        if (handle.value == 0 || handle.value > root_signatures.size()) {
            return nullptr;
        }
        const auto& record = root_signatures[handle.value - 1U];
        if (record.root_signature == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] ShaderRecord* shader_record(NativeShaderHandle handle) noexcept {
        if (handle.value == 0 || handle.value > shaders.size()) {
            return nullptr;
        }
        auto& record = shaders[handle.value - 1U];
        if (record.bytecode.empty()) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] const ShaderRecord* shader_record(NativeShaderHandle handle) const noexcept {
        if (handle.value == 0 || handle.value > shaders.size()) {
            return nullptr;
        }
        const auto& record = shaders[handle.value - 1U];
        if (record.bytecode.empty()) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] GraphicsPipelineRecord* graphics_pipeline_record(NativeGraphicsPipelineHandle handle) noexcept {
        if (handle.value == 0 || handle.value > graphics_pipelines.size()) {
            return nullptr;
        }
        auto& record = graphics_pipelines[handle.value - 1U];
        if (record.pipeline_state == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] const GraphicsPipelineRecord*
    graphics_pipeline_record(NativeGraphicsPipelineHandle handle) const noexcept {
        if (handle.value == 0 || handle.value > graphics_pipelines.size()) {
            return nullptr;
        }
        const auto& record = graphics_pipelines[handle.value - 1U];
        if (record.pipeline_state == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] ComputePipelineRecord* compute_pipeline_record(NativeComputePipelineHandle handle) noexcept {
        if (handle.value == 0 || handle.value > compute_pipelines.size()) {
            return nullptr;
        }
        auto& record = compute_pipelines[handle.value - 1U];
        if (record.pipeline_state == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] const ComputePipelineRecord*
    compute_pipeline_record(NativeComputePipelineHandle handle) const noexcept {
        if (handle.value == 0 || handle.value > compute_pipelines.size()) {
            return nullptr;
        }
        const auto& record = compute_pipelines[handle.value - 1U];
        if (record.pipeline_state == nullptr) {
            return nullptr;
        }
        return &record;
    }

    [[nodiscard]] bool refresh_back_buffers(SwapchainRecord& record) {
        record.back_buffers.clear();
        record.back_buffers.reserve(record.desc.buffer_count);

        for (std::uint32_t index = 0; index < record.desc.buffer_count; ++index) {
            Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
            if (FAILED(record.swapchain->GetBuffer(index, IID_PPV_ARGS(&buffer)))) {
                record.back_buffers.clear();
                return false;
            }
            record.back_buffers.push_back(buffer);
            d3d12_set_object_name_fmt(buffer.Get(), L"GameEngine.RHI.D3D12.Swapchain%u.BackBuffer%u",
                                      static_cast<unsigned>(record.native_handle_value), static_cast<unsigned>(index));
            ++stats.swapchain_back_buffers_created;
        }

        return true;
    }

    [[nodiscard]] bool refresh_render_target_views(SwapchainRecord& record) {
        if (device == nullptr || record.back_buffers.size() != record.desc.buffer_count) {
            return false;
        }

        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heap_desc.NumDescriptors = record.desc.buffer_count;
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heap_desc.NodeMask = 0;
        if (FAILED(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&record.rtv_heap)))) {
            return false;
        }

        d3d12_set_object_name_fmt(record.rtv_heap.Get(), L"GameEngine.RHI.D3D12.Swapchain%u.RtvDescriptorHeap",
                                  static_cast<unsigned>(record.native_handle_value));
        ++stats.render_target_view_heaps_created;
        record.rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_CPU_DESCRIPTOR_HANDLE rtv_cpu = record.rtv_heap->GetCPUDescriptorHandleForHeapStart();
        for (std::uint32_t index = 0; index < record.desc.buffer_count; ++index) {
            device->CreateRenderTargetView(record.back_buffers[index].Get(), nullptr, rtv_cpu);
            rtv_cpu.ptr += static_cast<SIZE_T>(record.rtv_descriptor_size);
            ++stats.render_target_views_created;
        }

        return true;
    }

    [[nodiscard]] bool create_resource_render_target_view(ID3D12Resource* resource,
                                                          ResourceRenderTargetViewRecord& record) {
        if (device == nullptr || resource == nullptr) {
            return false;
        }

        const auto desc = resource->GetDesc();
        if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER ||
            (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == 0) {
            return false;
        }

        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heap_desc.NumDescriptors = 1;
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heap_desc.NodeMask = 0;
        if (FAILED(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&record.heap)))) {
            return false;
        }

        ++stats.render_target_view_heaps_created;
        record.descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        device->CreateRenderTargetView(resource, nullptr, record.heap->GetCPUDescriptorHandleForHeapStart());
        ++stats.render_target_views_created;
        return true;
    }

    [[nodiscard]] bool create_resource_depth_stencil_view(ID3D12Resource* resource,
                                                          ResourceDepthStencilViewRecord& record) const {
        if (device == nullptr || resource == nullptr) {
            return false;
        }

        const auto desc = resource->GetDesc();
        if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
            (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) == 0 ||
            (desc.Format != DXGI_FORMAT_D24_UNORM_S8_UINT && desc.Format != DXGI_FORMAT_R24G8_TYPELESS) ||
            desc.DepthOrArraySize != 1) {
            return false;
        }

        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        heap_desc.NumDescriptors = 1;
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heap_desc.NodeMask = 0;
        if (FAILED(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&record.heap)))) {
            return false;
        }

        record.descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        D3D12_DEPTH_STENCIL_VIEW_DESC view{};
        view.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        view.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        view.Flags = D3D12_DSV_FLAG_NONE;
        view.Texture2D.MipSlice = 0;
        device->CreateDepthStencilView(resource, &view, record.heap->GetCPUDescriptorHandleForHeapStart());
        return true;
    }
};

DeviceContext::DeviceContext(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

DeviceContext::DeviceContext(DeviceContext&&) noexcept = default;

DeviceContext& DeviceContext::operator=(DeviceContext&&) noexcept = default;

DeviceContext::~DeviceContext() = default;

std::unique_ptr<DeviceContext> DeviceContext::create(const DeviceBootstrapDesc& desc) {
    auto impl = std::make_unique<Impl>();
    if (desc.enable_debug_layer) {
        impl->debug_layer_enabled = enable_debug_layer();
    }

    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&impl->factory)))) {
        return nullptr;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter1> hardware_adapter;
    Microsoft::WRL::ComPtr<IDXGIAdapter> warp_adapter;
    IUnknown* selected_adapter =
        select_adapter(impl->factory.Get(), desc.prefer_warp, hardware_adapter, warp_adapter, impl->used_warp);
    if (selected_adapter == nullptr) {
        return nullptr;
    }

    if (FAILED(D3D12CreateDevice(selected_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&impl->device)))) {
        return nullptr;
    }

    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.NodeMask = 0;
    if (FAILED(impl->device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&impl->command_queue)))) {
        return nullptr;
    }
    DeviceContext::Impl::queue_command_queue(impl->command_queues, QueueKind::graphics) = impl->command_queue;

    if (FAILED(impl->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&impl->fence)))) {
        return nullptr;
    }
    DeviceContext::Impl::queue_fence(impl->queue_fences, QueueKind::graphics) = impl->fence;

    d3d12_set_object_name(impl->device.Get(), L"GameEngine.RHI.D3D12.Device");
    d3d12_set_object_name(impl->command_queue.Get(), L"GameEngine.RHI.D3D12.GraphicsQueue");
    d3d12_set_object_name(impl->fence.Get(), L"GameEngine.RHI.D3D12.GraphicsFence");

    return std::unique_ptr<DeviceContext>(new DeviceContext(std::move(impl)));
}

bool DeviceContext::valid() const noexcept {
    return impl_ != nullptr && impl_->device != nullptr && impl_->command_queue != nullptr && impl_->fence != nullptr;
}

BackendKind DeviceContext::backend_kind() noexcept {
    return BackendKind::d3d12;
}

bool DeviceContext::used_warp() const noexcept {
    return impl_ != nullptr && impl_->used_warp;
}

bool DeviceContext::debug_layer_enabled() const noexcept {
    return impl_ != nullptr && impl_->debug_layer_enabled;
}

DeviceContextStats DeviceContext::stats() const noexcept {
    if (impl_ == nullptr) {
        return DeviceContextStats{};
    }
    return impl_->stats;
}

NativeResourceHandle DeviceContext::create_committed_buffer(const BufferDesc& desc) {
    if (!valid() || !valid_buffer_desc(desc)) {
        return NativeResourceHandle{};
    }

    const auto heap_type = committed_buffer_heap_type(desc.usage);
    const auto heap = heap_properties(heap_type);
    const auto resource_desc = buffer_resource_desc(desc.size_bytes, committed_buffer_flags(desc.usage));

    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    if (FAILED(impl_->device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &resource_desc,
                                                      committed_buffer_initial_state(heap_type), nullptr,
                                                      IID_PPV_ARGS(&resource)))) {
        return NativeResourceHandle{};
    }

    d3d12_set_object_name_fmt(resource.Get(), L"GameEngine.RHI.D3D12.Buffer%u",
                              static_cast<unsigned>(impl_->resources.size() + 1U));

    impl_->resources.push_back(resource);
    impl_->resource_heaps.emplace_back();
    impl_->resource_alias_group_ids.push_back(0);
    impl_->resource_placed_active.push_back(true);
    impl_->resource_rtvs.push_back(ResourceRenderTargetViewRecord{});
    impl_->resource_dsvs.push_back(ResourceDepthStencilViewRecord{});
    ++impl_->stats.committed_buffers_created;
    ++impl_->stats.committed_resources_alive;
    return NativeResourceHandle{static_cast<std::uint32_t>(impl_->resources.size())};
}

NativeResourceHandle DeviceContext::create_committed_texture(const TextureDesc& desc) {
    if (!valid() || !valid_texture_desc(desc)) {
        return NativeResourceHandle{};
    }

    auto resource_desc = texture_resource_desc(Extent2D{.width = desc.extent.width, .height = desc.extent.height},
                                               desc.format, desc.usage);
    resource_desc.DepthOrArraySize = static_cast<UINT16>(desc.extent.depth);
    resource_desc.Flags = committed_texture_flags(desc.usage);
    const auto heap = heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_CLEAR_VALUE clear_value{};
    const D3D12_CLEAR_VALUE* optimized_clear_value = nullptr;
    if (has_flag(desc.usage, TextureUsage::depth_stencil)) {
        clear_value.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clear_value.DepthStencil.Depth = 1.0F;
        clear_value.DepthStencil.Stencil = 0;
        optimized_clear_value = &clear_value;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    if (FAILED(impl_->device->CreateCommittedResource(&heap, committed_texture_heap_flags(desc.usage), &resource_desc,
                                                      committed_texture_initial_state(desc.usage),
                                                      optimized_clear_value, IID_PPV_ARGS(&resource)))) {
        return NativeResourceHandle{};
    }

    d3d12_set_object_name_fmt(resource.Get(), L"GameEngine.RHI.D3D12.Texture%u",
                              static_cast<unsigned>(impl_->resources.size() + 1U));

    ResourceRenderTargetViewRecord rtv;
    if (has_flag(desc.usage, TextureUsage::render_target) &&
        !impl_->create_resource_render_target_view(resource.Get(), rtv)) {
        return NativeResourceHandle{};
    }

    ResourceDepthStencilViewRecord dsv;
    if (has_flag(desc.usage, TextureUsage::depth_stencil) &&
        !impl_->create_resource_depth_stencil_view(resource.Get(), dsv)) {
        return NativeResourceHandle{};
    }

    impl_->resources.push_back(resource);
    impl_->resource_heaps.emplace_back();
    impl_->resource_alias_group_ids.push_back(0);
    impl_->resource_placed_active.push_back(true);
    impl_->resource_rtvs.push_back(std::move(rtv));
    impl_->resource_dsvs.push_back(std::move(dsv));
    ++impl_->stats.committed_textures_created;
    ++impl_->stats.committed_resources_alive;
    return NativeResourceHandle{static_cast<std::uint32_t>(impl_->resources.size())};
}

NativeResourceHandle DeviceContext::create_placed_texture(const TextureDesc& desc) {
    if (!valid() || !valid_texture_desc(desc) || has_flag(desc.usage, TextureUsage::shared) ||
        has_flag(desc.usage, TextureUsage::present)) {
        return NativeResourceHandle{};
    }

    auto resource_desc = texture_resource_desc(Extent2D{.width = desc.extent.width, .height = desc.extent.height},
                                               desc.format, desc.usage);
    resource_desc.DepthOrArraySize = static_cast<UINT16>(desc.extent.depth);
    resource_desc.Flags = committed_texture_flags(desc.usage);

    const D3D12_RESOURCE_ALLOCATION_INFO allocation = impl_->device->GetResourceAllocationInfo(0, 1, &resource_desc);
    if (allocation.SizeInBytes == 0 || allocation.SizeInBytes == (std::numeric_limits<std::uint64_t>::max)()) {
        return NativeResourceHandle{};
    }

    D3D12_HEAP_DESC heap_desc{};
    heap_desc.SizeInBytes = allocation.SizeInBytes;
    heap_desc.Properties = heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    heap_desc.Alignment = allocation.Alignment;
    heap_desc.Flags = placed_texture_heap_flags(desc.usage);

    Microsoft::WRL::ComPtr<ID3D12Heap> heap;
    if (FAILED(impl_->device->CreateHeap(&heap_desc, IID_PPV_ARGS(&heap)))) {
        return NativeResourceHandle{};
    }

    D3D12_CLEAR_VALUE clear_value{};
    const D3D12_CLEAR_VALUE* optimized_clear_value = nullptr;
    if (has_flag(desc.usage, TextureUsage::depth_stencil)) {
        clear_value.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clear_value.DepthStencil.Depth = 1.0F;
        clear_value.DepthStencil.Stencil = 0;
        optimized_clear_value = &clear_value;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    if (FAILED(impl_->device->CreatePlacedResource(heap.Get(), 0, &resource_desc,
                                                   committed_texture_initial_state(desc.usage), optimized_clear_value,
                                                   IID_PPV_ARGS(&resource)))) {
        return NativeResourceHandle{};
    }

    d3d12_set_object_name_fmt(heap.Get(), L"GameEngine.RHI.D3D12.TransientTextureHeap%u",
                              static_cast<unsigned>(impl_->resources.size() + 1U));
    d3d12_set_object_name_fmt(resource.Get(), L"GameEngine.RHI.D3D12.PlacedTexture%u",
                              static_cast<unsigned>(impl_->resources.size() + 1U));

    ResourceRenderTargetViewRecord rtv;
    if (has_flag(desc.usage, TextureUsage::render_target) &&
        !impl_->create_resource_render_target_view(resource.Get(), rtv)) {
        return NativeResourceHandle{};
    }

    ResourceDepthStencilViewRecord dsv;
    if (has_flag(desc.usage, TextureUsage::depth_stencil) &&
        !impl_->create_resource_depth_stencil_view(resource.Get(), dsv)) {
        return NativeResourceHandle{};
    }

    impl_->resources.push_back(resource);
    impl_->resource_heaps.push_back(std::move(heap));
    impl_->resource_alias_group_ids.push_back(0);
    impl_->resource_placed_active.push_back(false);
    impl_->resource_rtvs.push_back(std::move(rtv));
    impl_->resource_dsvs.push_back(std::move(dsv));
    ++impl_->stats.placed_texture_heaps_created;
    ++impl_->stats.placed_textures_created;
    ++impl_->stats.placed_resources_alive;
    return NativeResourceHandle{static_cast<std::uint32_t>(impl_->resources.size())};
}

std::vector<NativeResourceHandle> DeviceContext::create_placed_texture_alias_group(const TextureDesc& desc,
                                                                                   std::size_t texture_count) {
    if (!valid() || !valid_texture_desc(desc) || texture_count < 2 ||
        texture_count > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)()) ||
        has_flag(desc.usage, TextureUsage::shared) || has_flag(desc.usage, TextureUsage::present)) {
        return {};
    }
    if (impl_->resources.size() >
        static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)()) - texture_count) {
        return {};
    }

    auto resource_desc = texture_resource_desc(Extent2D{.width = desc.extent.width, .height = desc.extent.height},
                                               desc.format, desc.usage);
    resource_desc.DepthOrArraySize = static_cast<UINT16>(desc.extent.depth);
    resource_desc.Flags = committed_texture_flags(desc.usage);

    const D3D12_RESOURCE_ALLOCATION_INFO allocation = impl_->device->GetResourceAllocationInfo(0, 1, &resource_desc);
    if (allocation.SizeInBytes == 0 || allocation.SizeInBytes == (std::numeric_limits<std::uint64_t>::max)()) {
        return {};
    }

    D3D12_HEAP_DESC heap_desc{};
    heap_desc.SizeInBytes = allocation.SizeInBytes;
    heap_desc.Properties = heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    heap_desc.Alignment = allocation.Alignment;
    heap_desc.Flags = placed_texture_heap_flags(desc.usage);

    Microsoft::WRL::ComPtr<ID3D12Heap> heap;
    if (FAILED(impl_->device->CreateHeap(&heap_desc, IID_PPV_ARGS(&heap)))) {
        return {};
    }

    D3D12_CLEAR_VALUE clear_value{};
    const D3D12_CLEAR_VALUE* optimized_clear_value = nullptr;
    if (has_flag(desc.usage, TextureUsage::depth_stencil)) {
        clear_value.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clear_value.DepthStencil.Depth = 1.0F;
        clear_value.DepthStencil.Stencil = 0;
        optimized_clear_value = &clear_value;
    }

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resources;
    std::vector<ResourceRenderTargetViewRecord> rtvs;
    std::vector<ResourceDepthStencilViewRecord> dsvs;
    resources.reserve(texture_count);
    rtvs.reserve(texture_count);
    dsvs.reserve(texture_count);

    for (std::size_t i = 0; i < texture_count; ++i) {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        if (FAILED(impl_->device->CreatePlacedResource(heap.Get(), 0, &resource_desc,
                                                       committed_texture_initial_state(desc.usage),
                                                       optimized_clear_value, IID_PPV_ARGS(&resource)))) {
            return {};
        }

        ResourceRenderTargetViewRecord rtv;
        if (has_flag(desc.usage, TextureUsage::render_target) &&
            !impl_->create_resource_render_target_view(resource.Get(), rtv)) {
            return {};
        }

        ResourceDepthStencilViewRecord dsv;
        if (has_flag(desc.usage, TextureUsage::depth_stencil) &&
            !impl_->create_resource_depth_stencil_view(resource.Get(), dsv)) {
            return {};
        }

        resources.push_back(std::move(resource));
        rtvs.push_back(std::move(rtv));
        dsvs.push_back(std::move(dsv));
    }

    d3d12_set_object_name_fmt(heap.Get(), L"GameEngine.RHI.D3D12.TransientTextureAliasHeap%u",
                              static_cast<unsigned>(impl_->resources.size() + 1U));

    const std::uint64_t alias_group_id = ++impl_->next_resource_alias_group_id;
    std::vector<NativeResourceHandle> handles;
    handles.reserve(resources.size());
    for (std::size_t i = 0; i < resources.size(); ++i) {
        d3d12_set_object_name_fmt(resources[i].Get(), L"GameEngine.RHI.D3D12.PlacedAliasTexture%u",
                                  static_cast<unsigned>(impl_->resources.size() + 1U));
        impl_->resources.push_back(std::move(resources[i]));
        impl_->resource_heaps.push_back(heap);
        impl_->resource_alias_group_ids.push_back(alias_group_id);
        impl_->resource_placed_active.push_back(false);
        impl_->resource_rtvs.push_back(std::move(rtvs[i]));
        impl_->resource_dsvs.push_back(std::move(dsvs[i]));
        handles.push_back(NativeResourceHandle{static_cast<std::uint32_t>(impl_->resources.size())});
    }

    ++impl_->stats.placed_texture_heaps_created;
    ++impl_->stats.placed_texture_alias_groups_created;
    impl_->stats.placed_textures_created += static_cast<std::uint64_t>(handles.size());
    impl_->stats.placed_resources_alive += static_cast<std::uint64_t>(handles.size());
    return handles;
}

bool DeviceContext::activate_placed_texture(NativeCommandListHandle commands, NativeResourceHandle texture) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    ID3D12Resource* texture_resource = impl_->resource_record(texture);
    if (command_record == nullptr || texture_resource == nullptr || command_record->closed) {
        return false;
    }
    if (!impl_->resource_is_placed(texture)) {
        return true;
    }

    const auto desc = texture_resource->GetDesc();
    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        return false;
    }
    bool pending_active = impl_->resource_is_placed_active(texture);
    for (const auto& update : command_record->placed_resource_state_updates) {
        if (update.before.value == texture.value) {
            pending_active = false;
        }
        if (update.after.value == texture.value) {
            pending_active = true;
        }
    }
    if (pending_active) {
        return true;
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Aliasing.pResourceBefore = nullptr;
    barrier.Aliasing.pResourceAfter = texture_resource;

    command_record->list->ResourceBarrier(1, &barrier);
    command_record->placed_resource_state_updates.push_back(PlacedResourceStateUpdate{
        .before = NativeResourceHandle{},
        .after = texture,
    });
    ++impl_->stats.placed_resource_activation_barriers;
    return true;
}

NativeSwapchainHandle DeviceContext::create_swapchain_for_window(const NativeSwapchainDesc& desc) {
    if (!valid() || !valid_swapchain_desc(desc)) {
        return NativeSwapchainHandle{};
    }

    ID3D12CommandQueue* direct_queue = impl_->ensure_queue(QueueKind::graphics);
    if (direct_queue == nullptr) {
        return NativeSwapchainHandle{};
    }

    DXGI_SWAP_CHAIN_DESC1 swapchain_desc{};
    swapchain_desc.Width = desc.swapchain.extent.width;
    swapchain_desc.Height = desc.swapchain.extent.height;
    swapchain_desc.Format = to_dxgi_format(desc.swapchain.format);
    swapchain_desc.Stereo = FALSE;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = desc.swapchain.buffer_count;
    swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchain_desc.Flags = 0;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain1;
    if (FAILED(impl_->factory->CreateSwapChainForHwnd(direct_queue, to_hwnd(desc.window), &swapchain_desc, nullptr,
                                                      nullptr, &swapchain1))) {
        return NativeSwapchainHandle{};
    }

    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapchain3;
    if (FAILED(swapchain1.As(&swapchain3))) {
        return NativeSwapchainHandle{};
    }

    (void)impl_->factory->MakeWindowAssociation(to_hwnd(desc.window), DXGI_MWA_NO_ALT_ENTER);

    auto& record = impl_->swapchains.emplace_back();
    record.hwnd = to_hwnd(desc.window);
    record.desc = desc.swapchain;
    record.swapchain = swapchain3;
    record.native_handle_value = static_cast<std::uint32_t>(impl_->swapchains.size());

    if (!impl_->refresh_back_buffers(record) || !impl_->refresh_render_target_views(record)) {
        impl_->swapchains.pop_back();
        return NativeSwapchainHandle{};
    }

    ++impl_->stats.swapchains_created;
    impl_->stats.swapchains_alive = impl_->swapchains.size();
    return NativeSwapchainHandle{static_cast<std::uint32_t>(impl_->swapchains.size())};
}

bool DeviceContext::present_swapchain(NativeSwapchainHandle handle) {
    if (!valid()) {
        return false;
    }

    SwapchainRecord* record = impl_->swapchain_record(handle);
    if (record == nullptr) {
        return false;
    }

    const UINT sync_interval = record->desc.vsync ? 1U : 0U;
    if (FAILED(record->swapchain->Present(sync_interval, 0))) {
        return false;
    }

    ++impl_->stats.swapchain_presents;
    return true;
}

bool DeviceContext::resize_swapchain(NativeSwapchainHandle handle, Extent2D extent) {
    if (!valid() || extent.width == 0 || extent.height == 0) {
        return false;
    }

    SwapchainRecord* record = impl_->swapchain_record(handle);
    if (record == nullptr) {
        return false;
    }

    record->back_buffers.clear();
    record->rtv_heap.Reset();
    if (FAILED(record->swapchain->ResizeBuffers(record->desc.buffer_count, extent.width, extent.height,
                                                to_dxgi_format(record->desc.format), 0))) {
        return false;
    }

    record->desc.extent = extent;
    if (!impl_->refresh_back_buffers(*record) || !impl_->refresh_render_target_views(*record)) {
        return false;
    }

    ++impl_->stats.swapchain_resizes;
    return true;
}

NativeSwapchainInfo DeviceContext::swapchain_info(NativeSwapchainHandle handle) const noexcept {
    if (!valid()) {
        return NativeSwapchainInfo{};
    }

    const SwapchainRecord* record = impl_->swapchain_record(handle);
    if (record == nullptr) {
        return NativeSwapchainInfo{};
    }

    return NativeSwapchainInfo{
        .valid = true,
        .extent = record->desc.extent,
        .format = record->desc.format,
        .buffer_count = record->desc.buffer_count,
        .current_back_buffer = record->swapchain->GetCurrentBackBufferIndex(),
        .render_target_view_count = static_cast<std::uint32_t>(record->back_buffers.size()),
    };
}

NativeDescriptorHeapHandle DeviceContext::create_descriptor_heap(const NativeDescriptorHeapDesc& desc) {
    if (!valid() || !valid_descriptor_heap_desc(desc)) {
        return NativeDescriptorHeapHandle{};
    }

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
    heap_desc.Type = descriptor_heap_type(desc.kind);
    heap_desc.NumDescriptors = desc.capacity;
    heap_desc.Flags = desc.shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.NodeMask = 0;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
    if (FAILED(impl_->device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)))) {
        return NativeDescriptorHeapHandle{};
    }

    const auto heap_index = static_cast<unsigned>(impl_->descriptor_heaps.size() + 1U);
    if (desc.kind == NativeDescriptorHeapKind::cbv_srv_uav) {
        d3d12_set_object_name_fmt(heap.Get(), L"GameEngine.RHI.D3D12.DescriptorHeap.CbvSrvUav%u", heap_index);
    } else {
        d3d12_set_object_name_fmt(heap.Get(), L"GameEngine.RHI.D3D12.DescriptorHeap.Sampler%u", heap_index);
    }

    impl_->descriptor_heaps.push_back(DescriptorHeapRecord{
        .kind = desc.kind,
        .capacity = desc.capacity,
        .shader_visible = desc.shader_visible,
        .descriptor_size = impl_->device->GetDescriptorHandleIncrementSize(heap_desc.Type),
        .heap = heap,
    });
    ++impl_->stats.descriptor_heaps_created;
    if (desc.shader_visible) {
        ++impl_->stats.shader_visible_descriptor_heaps_created;
        impl_->stats.shader_visible_descriptors_reserved += desc.capacity;
    }

    return NativeDescriptorHeapHandle{static_cast<std::uint32_t>(impl_->descriptor_heaps.size())};
}

NativeDescriptorHeapInfo DeviceContext::descriptor_heap_info(NativeDescriptorHeapHandle handle) const noexcept {
    if (!valid()) {
        return NativeDescriptorHeapInfo{};
    }

    const DescriptorHeapRecord* record = impl_->descriptor_heap_record(handle);
    if (record == nullptr) {
        return NativeDescriptorHeapInfo{};
    }

    return NativeDescriptorHeapInfo{
        .valid = true,
        .kind = record->kind,
        .capacity = record->capacity,
        .shader_visible = record->shader_visible,
        .descriptor_size = record->descriptor_size,
    };
}

bool DeviceContext::write_descriptor(const NativeDescriptorWriteDesc& desc) {
    if (!valid()) {
        return false;
    }

    DescriptorHeapRecord* heap = impl_->descriptor_heap_record(desc.heap);
    if (heap == nullptr || !valid_descriptor_type(desc.type) || desc.descriptor_index >= heap->capacity) {
        return false;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE destination = heap->heap->GetCPUDescriptorHandleForHeapStart();
    destination.ptr += static_cast<SIZE_T>(desc.descriptor_index) * static_cast<SIZE_T>(heap->descriptor_size);

    if (desc.type == DescriptorType::sampler) {
        if (heap->kind != NativeDescriptorHeapKind::sampler || !valid_sampler_desc(desc.sampler)) {
            return false;
        }

        const auto sampler = to_d3d12_sampler_desc(desc.sampler);
        impl_->device->CreateSampler(&sampler, destination);
        ++impl_->stats.descriptor_views_written;
        return true;
    }

    ID3D12Resource* resource = impl_->resource_record(desc.resource);
    if (resource == nullptr || heap->kind != NativeDescriptorHeapKind::cbv_srv_uav) {
        return false;
    }

    const auto resource_desc = resource->GetDesc();
    switch (desc.type) {
    case DescriptorType::uniform_buffer: {
        if (resource_desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
            resource_desc.Width > static_cast<UINT64>(std::numeric_limits<UINT>::max()) - 255ULL) {
            return false;
        }

        const auto aligned_size = (resource_desc.Width + 255ULL) & ~255ULL;
        D3D12_CONSTANT_BUFFER_VIEW_DESC view{};
        view.BufferLocation = resource->GetGPUVirtualAddress();
        view.SizeInBytes = static_cast<UINT>(aligned_size);
        if (view.BufferLocation == 0 || view.SizeInBytes == 0) {
            return false;
        }

        impl_->device->CreateConstantBufferView(&view, destination);
        break;
    }
    case DescriptorType::storage_buffer: {
        if (resource_desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
            (resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0 || resource_desc.Width == 0 ||
            resource_desc.Width % 4ULL != 0ULL ||
            resource_desc.Width / 4ULL > static_cast<UINT64>(std::numeric_limits<UINT>::max())) {
            return false;
        }

        D3D12_UNORDERED_ACCESS_VIEW_DESC view{};
        view.Format = DXGI_FORMAT_R32_TYPELESS;
        view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        view.Buffer.FirstElement = 0;
        view.Buffer.NumElements = static_cast<UINT>(resource_desc.Width / 4ULL);
        view.Buffer.StructureByteStride = 0;
        view.Buffer.CounterOffsetInBytes = 0;
        view.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        impl_->device->CreateUnorderedAccessView(resource, nullptr, &view, destination);
        break;
    }
    case DescriptorType::sampled_texture: {
        const bool color_srv = is_d3d12_color_render_format(from_dxgi_format(resource_desc.Format));
        const bool sampled_depth_srv = resource_desc.Format == DXGI_FORMAT_R24G8_TYPELESS &&
                                       (resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0;
        if (resource_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || (!color_srv && !sampled_depth_srv)) {
            return false;
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC view{};
        view.Format = sampled_depth_srv ? DXGI_FORMAT_R24_UNORM_X8_TYPELESS : resource_desc.Format;
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view.Texture2D.MostDetailedMip = 0;
        view.Texture2D.MipLevels = resource_desc.MipLevels;
        view.Texture2D.PlaneSlice = 0;
        view.Texture2D.ResourceMinLODClamp = 0.0F;
        impl_->device->CreateShaderResourceView(resource, &view, destination);
        break;
    }
    case DescriptorType::storage_texture: {
        if (resource_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
            !is_d3d12_color_render_format(from_dxgi_format(resource_desc.Format)) ||
            (resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0) {
            return false;
        }

        D3D12_UNORDERED_ACCESS_VIEW_DESC view{};
        view.Format = resource_desc.Format;
        view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        view.Texture2D.MipSlice = 0;
        view.Texture2D.PlaneSlice = 0;
        impl_->device->CreateUnorderedAccessView(resource, nullptr, &view, destination);
        break;
    }
    case DescriptorType::sampler:
        return false;
    }

    ++impl_->stats.descriptor_views_written;
    return true;
}

NativeRootSignatureHandle DeviceContext::create_root_signature(const NativeRootSignatureDesc& desc) {
    if (!valid() || !valid_root_signature_desc(desc)) {
        return NativeRootSignatureHandle{};
    }

    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> ranges_by_table;
    std::vector<D3D12_ROOT_PARAMETER> root_parameters;
    ranges_by_table.reserve(desc.descriptor_sets.size());
    root_parameters.reserve(desc.descriptor_sets.size() + (desc.push_constant_bytes > 0 ? 1U : 0U));

    std::vector<NativeDescriptorHeapKind> table_heap_kinds;
    std::uint32_t descriptor_range_count = 0;
    for (std::uint32_t set_index = 0; set_index < desc.descriptor_sets.size(); ++set_index) {
        const auto& descriptor_set = desc.descriptor_sets[set_index];
        for (const auto kind : {NativeDescriptorHeapKind::cbv_srv_uav, NativeDescriptorHeapKind::sampler}) {
            if (!descriptor_set_has_heap_kind(descriptor_set, kind)) {
                continue;
            }

            auto& ranges = ranges_by_table.emplace_back();
            ranges.reserve(descriptor_set.bindings.size());

            for (const auto& binding : descriptor_set.bindings) {
                if (!descriptor_type_matches_heap(binding.type, kind)) {
                    continue;
                }

                D3D12_DESCRIPTOR_RANGE range{};
                range.RangeType = descriptor_range_type(binding.type);
                range.NumDescriptors = binding.count;
                range.BaseShaderRegister = binding.binding;
                range.RegisterSpace = set_index;
                range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                ranges.push_back(range);
                ++descriptor_range_count;
            }

            D3D12_ROOT_PARAMETER parameter{};
            parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            parameter.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(ranges.size());
            parameter.DescriptorTable.pDescriptorRanges = ranges.data();
            parameter.ShaderVisibility = shader_visibility_for_table(descriptor_set, kind);
            root_parameters.push_back(parameter);
            table_heap_kinds.push_back(kind);
        }
    }

    if (desc.push_constant_bytes > 0) {
        D3D12_ROOT_PARAMETER parameter{};
        parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        parameter.Constants.ShaderRegister = 0;
        parameter.Constants.RegisterSpace = 1023;
        parameter.Constants.Num32BitValues = desc.push_constant_bytes / 4U;
        parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        root_parameters.push_back(parameter);
    }

    D3D12_ROOT_SIGNATURE_DESC root_desc{};
    root_desc.NumParameters = static_cast<UINT>(root_parameters.size());
    root_desc.pParameters = root_parameters.empty() ? nullptr : root_parameters.data();
    root_desc.NumStaticSamplers = 0;
    root_desc.pStaticSamplers = nullptr;
    root_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> serialized;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    if (FAILED(D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors))) {
        return NativeRootSignatureHandle{};
    }

    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
    if (FAILED(impl_->device->CreateRootSignature(0, serialized->GetBufferPointer(), serialized->GetBufferSize(),
                                                  IID_PPV_ARGS(&root_signature)))) {
        return NativeRootSignatureHandle{};
    }

    d3d12_set_object_name_fmt(root_signature.Get(), L"GameEngine.RHI.D3D12.RootSignature%u",
                              static_cast<unsigned>(impl_->root_signatures.size() + 1U));

    impl_->root_signatures.push_back(RootSignatureRecord{
        .root_signature = root_signature,
        .info =
            NativeRootSignatureInfo{
                .valid = true,
                .descriptor_table_count = static_cast<std::uint32_t>(table_heap_kinds.size()),
                .descriptor_range_count = descriptor_range_count,
                .push_constant_bytes = desc.push_constant_bytes,
            },
        .table_heap_kinds = std::move(table_heap_kinds),
    });
    ++impl_->stats.root_signatures_created;
    impl_->stats.descriptor_tables_created += impl_->root_signatures.back().info.descriptor_table_count;
    impl_->stats.descriptor_ranges_created += descriptor_range_count;

    return NativeRootSignatureHandle{static_cast<std::uint32_t>(impl_->root_signatures.size())};
}

NativeRootSignatureInfo DeviceContext::root_signature_info(NativeRootSignatureHandle handle) const noexcept {
    if (!valid()) {
        return NativeRootSignatureInfo{};
    }

    const RootSignatureRecord* record = impl_->root_signature_record(handle);
    if (record == nullptr) {
        return NativeRootSignatureInfo{};
    }

    return record->info;
}

NativeShaderHandle DeviceContext::create_shader_module(const NativeShaderBytecodeDesc& desc) {
    if (!valid() || !valid_shader_bytecode_desc(desc)) {
        return NativeShaderHandle{};
    }

    ShaderRecord record;
    record.stage = desc.stage;
    record.bytecode.resize(static_cast<std::size_t>(desc.bytecode_size));
    std::memcpy(record.bytecode.data(), desc.bytecode, record.bytecode.size());

    impl_->shaders.push_back(std::move(record));
    ++impl_->stats.shader_modules_created;
    impl_->stats.shader_bytecode_bytes_owned += desc.bytecode_size;
    return NativeShaderHandle{static_cast<std::uint32_t>(impl_->shaders.size())};
}

NativeShaderInfo DeviceContext::shader_info(NativeShaderHandle handle) const noexcept {
    if (!valid()) {
        return NativeShaderInfo{};
    }

    const ShaderRecord* record = impl_->shader_record(handle);
    if (record == nullptr) {
        return NativeShaderInfo{};
    }

    return NativeShaderInfo{
        .valid = true,
        .stage = record->stage,
        .bytecode_size = static_cast<std::uint64_t>(record->bytecode.size()),
    };
}

NativeGraphicsPipelineHandle DeviceContext::create_graphics_pipeline(const NativeGraphicsPipelineDesc& desc) {
    if (!valid() || !is_d3d12_color_render_format(desc.color_format) ||
        to_d3d12_primitive_topology_type(desc.topology) == D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED ||
        !valid_depth_state_for_format(desc.depth_format, desc.depth_state) ||
        !valid_vertex_input_desc(desc.vertex_buffers, desc.vertex_attributes)) {
        return NativeGraphicsPipelineHandle{};
    }

    RootSignatureRecord* root_signature = impl_->root_signature_record(desc.root_signature);
    ShaderRecord* vertex_shader = impl_->shader_record(desc.vertex_shader);
    ShaderRecord* fragment_shader = impl_->shader_record(desc.fragment_shader);
    if (root_signature == nullptr || vertex_shader == nullptr || fragment_shader == nullptr) {
        return NativeGraphicsPipelineHandle{};
    }
    if (vertex_shader->stage != ShaderStage::vertex || fragment_shader->stage != ShaderStage::fragment) {
        return NativeGraphicsPipelineHandle{};
    }
    std::vector<D3D12_INPUT_ELEMENT_DESC> input_elements;
    input_elements.reserve(desc.vertex_attributes.size());
    for (const auto& attribute : desc.vertex_attributes) {
        const auto layout =
            std::ranges::find_if(desc.vertex_buffers, [&attribute](const VertexBufferLayoutDesc& candidate) {
                return candidate.binding == attribute.binding;
            });
        if (layout == desc.vertex_buffers.end()) {
            return NativeGraphicsPipelineHandle{};
        }
        input_elements.push_back(D3D12_INPUT_ELEMENT_DESC{
            semantic_name(attribute.semantic),
            attribute.semantic_index,
            to_dxgi_vertex_format(attribute.format),
            attribute.binding,
            attribute.offset,
            input_classification(layout->input_rate),
            layout->input_rate == VertexInputRate::instance ? 1U : 0U,
        });
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc{};
    pipeline_desc.pRootSignature = root_signature->root_signature.Get();
    pipeline_desc.VS = D3D12_SHADER_BYTECODE{
        vertex_shader->bytecode.data(),
        vertex_shader->bytecode.size(),
    };
    pipeline_desc.PS = D3D12_SHADER_BYTECODE{
        fragment_shader->bytecode.data(),
        fragment_shader->bytecode.size(),
    };
    pipeline_desc.BlendState = default_blend_desc();
    pipeline_desc.SampleMask = (std::numeric_limits<UINT>::max)();
    pipeline_desc.RasterizerState = default_rasterizer_desc();
    pipeline_desc.DepthStencilState = to_d3d12_depth_stencil_desc(desc.depth_state);
    pipeline_desc.InputLayout = D3D12_INPUT_LAYOUT_DESC{
        input_elements.empty() ? nullptr : input_elements.data(),
        static_cast<UINT>(input_elements.size()),
    };
    pipeline_desc.PrimitiveTopologyType = to_d3d12_primitive_topology_type(desc.topology);
    pipeline_desc.NumRenderTargets = 1;
    pipeline_desc.RTVFormats[0] = to_dxgi_format(desc.color_format);
    pipeline_desc.DSVFormat = to_dxgi_format(desc.depth_format);
    pipeline_desc.SampleDesc.Count = 1;
    pipeline_desc.SampleDesc.Quality = 0;
    pipeline_desc.NodeMask = 0;
    pipeline_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
    if (FAILED(impl_->device->CreateGraphicsPipelineState(&pipeline_desc, IID_PPV_ARGS(&pipeline_state)))) {
        return NativeGraphicsPipelineHandle{};
    }

    d3d12_set_object_name_fmt(pipeline_state.Get(), L"GameEngine.RHI.D3D12.GraphicsPso%u",
                              static_cast<unsigned>(impl_->graphics_pipelines.size() + 1U));

    impl_->graphics_pipelines.push_back(GraphicsPipelineRecord{
        .pipeline_state = pipeline_state,
        .info =
            NativeGraphicsPipelineInfo{
                .valid = true,
                .root_signature = desc.root_signature,
                .color_format = desc.color_format,
                .depth_format = desc.depth_format,
                .topology = desc.topology,
                .vertex_buffer_layout_count = static_cast<std::uint32_t>(desc.vertex_buffers.size()),
                .vertex_attribute_count = static_cast<std::uint32_t>(desc.vertex_attributes.size()),
            },
        .vertex_shader = desc.vertex_shader,
        .fragment_shader = desc.fragment_shader,
    });
    ++impl_->stats.graphics_pipelines_created;
    return NativeGraphicsPipelineHandle{static_cast<std::uint32_t>(impl_->graphics_pipelines.size())};
}

NativeGraphicsPipelineInfo DeviceContext::graphics_pipeline_info(NativeGraphicsPipelineHandle handle) const noexcept {
    if (!valid()) {
        return NativeGraphicsPipelineInfo{};
    }

    const GraphicsPipelineRecord* record = impl_->graphics_pipeline_record(handle);
    if (record == nullptr) {
        return NativeGraphicsPipelineInfo{};
    }

    return record->info;
}

NativeComputePipelineHandle DeviceContext::create_compute_pipeline(const NativeComputePipelineDesc& desc) {
    if (!valid()) {
        return NativeComputePipelineHandle{};
    }

    RootSignatureRecord* root_signature = impl_->root_signature_record(desc.root_signature);
    ShaderRecord* compute_shader = impl_->shader_record(desc.compute_shader);
    if (root_signature == nullptr || compute_shader == nullptr || compute_shader->stage != ShaderStage::compute) {
        return NativeComputePipelineHandle{};
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc{};
    pipeline_desc.pRootSignature = root_signature->root_signature.Get();
    pipeline_desc.CS = D3D12_SHADER_BYTECODE{
        compute_shader->bytecode.data(),
        compute_shader->bytecode.size(),
    };
    pipeline_desc.NodeMask = 0;
    pipeline_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
    if (FAILED(impl_->device->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&pipeline_state)))) {
        return NativeComputePipelineHandle{};
    }

    d3d12_set_object_name_fmt(pipeline_state.Get(), L"GameEngine.RHI.D3D12.ComputePso%u",
                              static_cast<unsigned>(impl_->compute_pipelines.size() + 1U));

    impl_->compute_pipelines.push_back(ComputePipelineRecord{
        .pipeline_state = pipeline_state,
        .info =
            NativeComputePipelineInfo{
                .valid = true,
                .root_signature = desc.root_signature,
            },
        .compute_shader = desc.compute_shader,
    });
    ++impl_->stats.compute_pipelines_created;
    return NativeComputePipelineHandle{static_cast<std::uint32_t>(impl_->compute_pipelines.size())};
}

NativeComputePipelineInfo DeviceContext::compute_pipeline_info(NativeComputePipelineHandle handle) const noexcept {
    if (!valid()) {
        return NativeComputePipelineInfo{};
    }

    const ComputePipelineRecord* record = impl_->compute_pipeline_record(handle);
    if (record == nullptr) {
        return NativeComputePipelineInfo{};
    }

    return record->info;
}

bool DeviceContext::transition_swapchain_back_buffer(NativeCommandListHandle commands, NativeSwapchainHandle swapchain,
                                                     ResourceState before, ResourceState after) {
    if (!valid() || !valid_resource_transition(before, after)) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    SwapchainRecord* swapchain_record = impl_->swapchain_record(swapchain);
    if (command_record == nullptr || swapchain_record == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::graphics) {
        return false;
    }

    const auto back_buffer_index = swapchain_record->swapchain->GetCurrentBackBufferIndex();
    if (back_buffer_index >= swapchain_record->back_buffers.size()) {
        return false;
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapchain_record->back_buffers[back_buffer_index].Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = to_d3d12_resource_state(before);
    barrier.Transition.StateAfter = to_d3d12_resource_state(after);

    command_record->list->ResourceBarrier(1, &barrier);
    ++impl_->stats.swapchain_back_buffer_transitions;
    return true;
}

bool DeviceContext::transition_texture(NativeCommandListHandle commands, NativeResourceHandle texture,
                                       ResourceState before, ResourceState after) {
    if (!valid() || !valid_resource_transition(before, after)) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    ID3D12Resource* texture_resource = impl_->resource_record(texture);
    if (command_record == nullptr || texture_resource == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::graphics) {
        return false;
    }

    const auto desc = texture_resource->GetDesc();
    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        return false;
    }
    if (!activate_placed_texture(commands, texture)) {
        return false;
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = texture_resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = to_d3d12_resource_state(before);
    barrier.Transition.StateAfter = to_d3d12_resource_state(after);

    command_record->list->ResourceBarrier(1, &barrier);
    ++impl_->stats.texture_transitions;
    return true;
}

bool DeviceContext::texture_aliasing_barrier(NativeCommandListHandle commands, NativeResourceHandle before,
                                             NativeResourceHandle after) {
    if (!valid() || before.value == 0 || after.value == 0 || before.value == after.value) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    ID3D12Resource* before_resource = impl_->resource_record(before);
    ID3D12Resource* after_resource = impl_->resource_record(after);
    if (command_record == nullptr || before_resource == nullptr || after_resource == nullptr ||
        command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::graphics) {
        return false;
    }

    const auto before_desc = before_resource->GetDesc();
    const auto after_desc = after_resource->GetDesc();
    if (before_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER ||
        after_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        return false;
    }

    if (impl_->resources_share_placed_alias_group(before, after)) {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Aliasing.pResourceBefore = before_resource;
        barrier.Aliasing.pResourceAfter = after_resource;

        command_record->list->ResourceBarrier(1, &barrier);
        command_record->placed_resource_state_updates.push_back(PlacedResourceStateUpdate{
            .before = before,
            .after = after,
        });
        ++impl_->stats.texture_aliasing_barriers;
        ++impl_->stats.placed_resource_aliasing_barriers;
        return true;
    }

    // Public RHI calls require concrete texture handles, but committed resources cannot be used
    // as non-null D3D12 aliasing-barrier resources. Record a conservative backend-private
    // null-resource aliasing barrier so the command has native synchronization evidence without
    // exposing public wildcard/null handles or claiming placed-resource alias execution.
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Aliasing.pResourceBefore = nullptr;
    barrier.Aliasing.pResourceAfter = nullptr;

    command_record->list->ResourceBarrier(1, &barrier);
    ++impl_->stats.texture_aliasing_barriers;
    ++impl_->stats.null_resource_aliasing_barriers;
    return true;
}

bool DeviceContext::clear_swapchain_back_buffer(NativeCommandListHandle commands, NativeSwapchainHandle swapchain,
                                                float red, float green, float blue, float alpha) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    SwapchainRecord* swapchain_record = impl_->swapchain_record(swapchain);
    if (command_record == nullptr || swapchain_record == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::graphics || swapchain_record->rtv_heap == nullptr) {
        return false;
    }

    const auto back_buffer_index = swapchain_record->swapchain->GetCurrentBackBufferIndex();
    if (back_buffer_index >= swapchain_record->back_buffers.size()) {
        return false;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE handle = swapchain_record->rtv_heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(back_buffer_index) * static_cast<SIZE_T>(swapchain_record->rtv_descriptor_size);

    const std::array<float, 4> color{red, green, blue, alpha};
    command_record->list->ClearRenderTargetView(handle, color.data(), 0, nullptr);
    ++impl_->stats.swapchain_back_buffer_clears;
    return true;
}

bool DeviceContext::clear_texture_render_target(NativeCommandListHandle commands, NativeResourceHandle texture,
                                                float red, float green, float blue, float alpha) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    ID3D12Resource* texture_resource = impl_->resource_record(texture);
    ResourceRenderTargetViewRecord* rtv_record = impl_->resource_rtv_record(texture);
    if (command_record == nullptr || texture_resource == nullptr || rtv_record == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::graphics) {
        return false;
    }

    const auto desc = texture_resource->GetDesc();
    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        return false;
    }
    if (!activate_placed_texture(commands, texture)) {
        return false;
    }

    const std::array<float, 4> color{red, green, blue, alpha};
    command_record->list->ClearRenderTargetView(rtv_record->heap->GetCPUDescriptorHandleForHeapStart(), color.data(), 0,
                                                nullptr);
    ++impl_->stats.texture_render_target_clears;
    return true;
}

bool DeviceContext::clear_texture_depth_stencil(NativeCommandListHandle commands, NativeResourceHandle texture,
                                                float depth) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    ID3D12Resource* texture_resource = impl_->resource_record(texture);
    ResourceDepthStencilViewRecord* dsv_record = impl_->resource_dsv_record(texture);
    if (command_record == nullptr || texture_resource == nullptr || dsv_record == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::graphics) {
        return false;
    }

    const auto desc = texture_resource->GetDesc();
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
        (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) == 0) {
        return false;
    }
    if (!activate_placed_texture(commands, texture)) {
        return false;
    }

    command_record->list->ClearDepthStencilView(dsv_record->heap->GetCPUDescriptorHandleForHeapStart(),
                                                D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
    return true;
}

bool DeviceContext::set_descriptor_heaps(NativeCommandListHandle commands, const NativeDescriptorHeapBinding& binding) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    if (command_record == nullptr || command_record->closed || command_record->queue == QueueKind::copy) {
        return false;
    }

    std::vector<ID3D12DescriptorHeap*> heaps;
    heaps.reserve(2);

    if (binding.cbv_srv_uav.value != 0) {
        DescriptorHeapRecord* heap = impl_->descriptor_heap_record(binding.cbv_srv_uav);
        if (heap == nullptr || heap->kind != NativeDescriptorHeapKind::cbv_srv_uav || !heap->shader_visible) {
            return false;
        }
        heaps.push_back(heap->heap.Get());
    }

    if (binding.sampler.value != 0) {
        DescriptorHeapRecord* heap = impl_->descriptor_heap_record(binding.sampler);
        if (heap == nullptr || heap->kind != NativeDescriptorHeapKind::sampler || !heap->shader_visible) {
            return false;
        }
        heaps.push_back(heap->heap.Get());
    }

    if (heaps.empty()) {
        return false;
    }

    command_record->list->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
    command_record->descriptor_heaps = binding;
    impl_->stats.descriptor_heaps_bound += static_cast<std::uint32_t>(heaps.size());
    return true;
}

bool DeviceContext::set_graphics_root_signature(NativeCommandListHandle commands,
                                                NativeRootSignatureHandle root_signature) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    RootSignatureRecord* signature_record = impl_->root_signature_record(root_signature);
    if (command_record == nullptr || signature_record == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::graphics) {
        return false;
    }

    command_record->list->SetGraphicsRootSignature(signature_record->root_signature.Get());
    command_record->graphics_root_signature = root_signature;
    ++impl_->stats.root_signatures_bound;
    return true;
}

bool DeviceContext::set_graphics_descriptor_table(NativeCommandListHandle commands,
                                                  NativeRootSignatureHandle root_signature,
                                                  std::uint32_t root_parameter_index, NativeDescriptorHeapHandle heap,
                                                  std::uint32_t descriptor_index) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    RootSignatureRecord* signature_record = impl_->root_signature_record(root_signature);
    DescriptorHeapRecord* heap_record = impl_->descriptor_heap_record(heap);
    if (command_record == nullptr || signature_record == nullptr || heap_record == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::graphics || !heap_record->shader_visible) {
        return false;
    }
    if (root_parameter_index >= signature_record->table_heap_kinds.size() ||
        signature_record->table_heap_kinds.at(root_parameter_index) != heap_record->kind) {
        return false;
    }
    const auto bound_heap = heap_record->kind == NativeDescriptorHeapKind::sampler
                                ? command_record->descriptor_heaps.sampler
                                : command_record->descriptor_heaps.cbv_srv_uav;
    if (command_record->graphics_root_signature.value != root_signature.value || bound_heap.value != heap.value) {
        return false;
    }
    if (root_parameter_index >= signature_record->info.descriptor_table_count ||
        descriptor_index >= heap_record->capacity) {
        return false;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = heap_record->heap->GetGPUDescriptorHandleForHeapStart();
    gpu_handle.ptr += static_cast<UINT64>(descriptor_index) * heap_record->descriptor_size;
    command_record->list->SetGraphicsRootDescriptorTable(root_parameter_index, gpu_handle);

    ++impl_->stats.descriptor_tables_bound;
    return true;
}

bool DeviceContext::set_compute_root_signature(NativeCommandListHandle commands,
                                               NativeRootSignatureHandle root_signature) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    RootSignatureRecord* signature_record = impl_->root_signature_record(root_signature);
    if (command_record == nullptr || signature_record == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::compute) {
        return false;
    }

    command_record->list->SetComputeRootSignature(signature_record->root_signature.Get());
    command_record->compute_root_signature = root_signature;
    ++impl_->stats.root_signatures_bound;
    return true;
}

bool DeviceContext::set_compute_descriptor_table(NativeCommandListHandle commands,
                                                 NativeRootSignatureHandle root_signature,
                                                 std::uint32_t root_parameter_index, NativeDescriptorHeapHandle heap,
                                                 std::uint32_t descriptor_index) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    RootSignatureRecord* signature_record = impl_->root_signature_record(root_signature);
    DescriptorHeapRecord* heap_record = impl_->descriptor_heap_record(heap);
    if (command_record == nullptr || signature_record == nullptr || heap_record == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::compute || !heap_record->shader_visible) {
        return false;
    }
    if (root_parameter_index >= signature_record->table_heap_kinds.size() ||
        signature_record->table_heap_kinds.at(root_parameter_index) != heap_record->kind) {
        return false;
    }
    const auto bound_heap = heap_record->kind == NativeDescriptorHeapKind::sampler
                                ? command_record->descriptor_heaps.sampler
                                : command_record->descriptor_heaps.cbv_srv_uav;
    if (command_record->compute_root_signature.value != root_signature.value || bound_heap.value != heap.value) {
        return false;
    }
    if (root_parameter_index >= signature_record->info.descriptor_table_count ||
        descriptor_index >= heap_record->capacity) {
        return false;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = heap_record->heap->GetGPUDescriptorHandleForHeapStart();
    gpu_handle.ptr += static_cast<UINT64>(descriptor_index) * heap_record->descriptor_size;
    command_record->list->SetComputeRootDescriptorTable(root_parameter_index, gpu_handle);

    ++impl_->stats.descriptor_tables_bound;
    return true;
}

bool DeviceContext::set_swapchain_render_target(NativeCommandListHandle commands, NativeSwapchainHandle swapchain,
                                                NativeResourceHandle depth) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    SwapchainRecord* swapchain_record = impl_->swapchain_record(swapchain);
    if (command_record == nullptr || swapchain_record == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::graphics || swapchain_record->rtv_heap == nullptr) {
        return false;
    }

    const auto back_buffer_index = swapchain_record->swapchain->GetCurrentBackBufferIndex();
    if (back_buffer_index >= swapchain_record->back_buffers.size()) {
        return false;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapchain_record->rtv_heap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += static_cast<SIZE_T>(back_buffer_index) * static_cast<SIZE_T>(swapchain_record->rtv_descriptor_size);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
    const D3D12_CPU_DESCRIPTOR_HANDLE* dsv_ptr = nullptr;
    if (depth.value != 0) {
        ID3D12Resource* depth_resource = impl_->resource_record(depth);
        ResourceDepthStencilViewRecord* dsv_record = impl_->resource_dsv_record(depth);
        if (depth_resource == nullptr || dsv_record == nullptr) {
            return false;
        }
        const auto depth_desc = depth_resource->GetDesc();
        if (depth_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
            depth_desc.Width != swapchain_record->desc.extent.width ||
            depth_desc.Height != swapchain_record->desc.extent.height) {
            return false;
        }
        if (!activate_placed_texture(commands, depth)) {
            return false;
        }
        dsv = dsv_record->heap->GetCPUDescriptorHandleForHeapStart();
        dsv_ptr = &dsv;
    }

    D3D12_VIEWPORT viewport{};
    viewport.TopLeftX = 0.0F;
    viewport.TopLeftY = 0.0F;
    viewport.Width = static_cast<float>(swapchain_record->desc.extent.width);
    viewport.Height = static_cast<float>(swapchain_record->desc.extent.height);
    viewport.MinDepth = 0.0F;
    viewport.MaxDepth = 1.0F;

    D3D12_RECT scissor{};
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = static_cast<LONG>(swapchain_record->desc.extent.width);
    scissor.bottom = static_cast<LONG>(swapchain_record->desc.extent.height);

    command_record->list->OMSetRenderTargets(1, &rtv, FALSE, dsv_ptr);
    command_record->list->RSSetViewports(1, &viewport);
    command_record->list->RSSetScissorRects(1, &scissor);
    command_record->render_target_set = true;
    ++impl_->stats.swapchain_render_targets_set;
    return true;
}

bool DeviceContext::set_texture_render_target(NativeCommandListHandle commands, NativeResourceHandle texture,
                                              NativeResourceHandle depth) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    ID3D12Resource* texture_resource = impl_->resource_record(texture);
    ResourceRenderTargetViewRecord* rtv_record = impl_->resource_rtv_record(texture);
    if (command_record == nullptr || texture_resource == nullptr || rtv_record == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::graphics) {
        return false;
    }

    const auto desc = texture_resource->GetDesc();
    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER || desc.Width == 0 || desc.Height == 0) {
        return false;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
    const D3D12_CPU_DESCRIPTOR_HANDLE* dsv_ptr = nullptr;
    if (depth.value != 0) {
        ID3D12Resource* depth_resource = impl_->resource_record(depth);
        ResourceDepthStencilViewRecord* dsv_record = impl_->resource_dsv_record(depth);
        if (depth_resource == nullptr || dsv_record == nullptr) {
            return false;
        }
        const auto depth_desc = depth_resource->GetDesc();
        if (depth_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || depth_desc.Width != desc.Width ||
            depth_desc.Height != desc.Height) {
            return false;
        }
        if (!activate_placed_texture(commands, depth)) {
            return false;
        }
        dsv = dsv_record->heap->GetCPUDescriptorHandleForHeapStart();
        dsv_ptr = &dsv;
    }
    if (!activate_placed_texture(commands, texture)) {
        return false;
    }

    D3D12_VIEWPORT viewport{};
    viewport.TopLeftX = 0.0F;
    viewport.TopLeftY = 0.0F;
    viewport.Width = static_cast<float>(desc.Width);
    viewport.Height = static_cast<float>(desc.Height);
    viewport.MinDepth = 0.0F;
    viewport.MaxDepth = 1.0F;

    D3D12_RECT scissor{};
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = static_cast<LONG>(desc.Width);
    scissor.bottom = static_cast<LONG>(desc.Height);

    const auto rtv = rtv_record->heap->GetCPUDescriptorHandleForHeapStart();
    command_record->list->OMSetRenderTargets(1, &rtv, FALSE, dsv_ptr);
    command_record->list->RSSetViewports(1, &viewport);
    command_record->list->RSSetScissorRects(1, &scissor);
    command_record->render_target_set = true;
    ++impl_->stats.texture_render_targets_set;
    return true;
}

bool DeviceContext::bind_graphics_pipeline(NativeCommandListHandle commands, NativeGraphicsPipelineHandle pipeline) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    GraphicsPipelineRecord* pipeline_record = impl_->graphics_pipeline_record(pipeline);
    if (command_record == nullptr || pipeline_record == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::graphics) {
        return false;
    }
    if (command_record->graphics_root_signature.value != pipeline_record->info.root_signature.value) {
        return false;
    }

    command_record->list->SetPipelineState(pipeline_record->pipeline_state.Get());
    command_record->list->IASetPrimitiveTopology(to_d3d_primitive_topology(pipeline_record->info.topology));
    command_record->graphics_pipeline = pipeline;
    ++impl_->stats.graphics_pipelines_bound;
    return true;
}

bool DeviceContext::bind_compute_pipeline(NativeCommandListHandle commands, NativeComputePipelineHandle pipeline) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    ComputePipelineRecord* pipeline_record = impl_->compute_pipeline_record(pipeline);
    if (command_record == nullptr || pipeline_record == nullptr || command_record->closed) {
        return false;
    }
    if (command_record->queue != QueueKind::compute) {
        return false;
    }
    if (command_record->compute_root_signature.value != pipeline_record->info.root_signature.value) {
        return false;
    }

    command_record->list->SetPipelineState(pipeline_record->pipeline_state.Get());
    command_record->compute_pipeline = pipeline;
    ++impl_->stats.compute_pipelines_bound;
    return true;
}

bool DeviceContext::bind_vertex_buffer(NativeCommandListHandle commands, NativeResourceHandle buffer,
                                       std::uint64_t offset, std::uint32_t stride, std::uint32_t slot) {
    if (!valid() || stride == 0) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    ID3D12Resource* resource = impl_->resource_record(buffer);
    if (command_record == nullptr || resource == nullptr || command_record->closed ||
        command_record->queue != QueueKind::graphics) {
        return false;
    }
    if (command_record->graphics_pipeline.value == 0 || !command_record->render_target_set) {
        return false;
    }

    const auto desc = resource->GetDesc();
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || offset >= desc.Width ||
        desc.Width - offset > (std::numeric_limits<UINT>::max)()) {
        return false;
    }

    D3D12_VERTEX_BUFFER_VIEW view{};
    view.BufferLocation = resource->GetGPUVirtualAddress() + offset;
    view.SizeInBytes = static_cast<UINT>(desc.Width - offset);
    view.StrideInBytes = stride;
    command_record->list->IASetVertexBuffers(slot, 1, &view);
    ++impl_->stats.vertex_buffer_bindings;
    return true;
}

bool DeviceContext::bind_index_buffer(NativeCommandListHandle commands, NativeResourceHandle buffer,
                                      std::uint64_t offset, IndexFormat format) {
    if (!valid()) {
        return false;
    }

    const auto dxgi_format = to_dxgi_index_format(format);
    if (dxgi_format == DXGI_FORMAT_UNKNOWN) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    ID3D12Resource* resource = impl_->resource_record(buffer);
    if (command_record == nullptr || resource == nullptr || command_record->closed ||
        command_record->queue != QueueKind::graphics) {
        return false;
    }
    if (command_record->graphics_pipeline.value == 0 || !command_record->render_target_set) {
        return false;
    }

    const auto desc = resource->GetDesc();
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || offset >= desc.Width ||
        desc.Width - offset > (std::numeric_limits<UINT>::max)()) {
        return false;
    }

    D3D12_INDEX_BUFFER_VIEW view{};
    view.BufferLocation = resource->GetGPUVirtualAddress() + offset;
    view.SizeInBytes = static_cast<UINT>(desc.Width - offset);
    view.Format = dxgi_format;
    command_record->list->IASetIndexBuffer(&view);
    ++impl_->stats.index_buffer_bindings;
    return true;
}

bool DeviceContext::draw(NativeCommandListHandle commands, std::uint32_t vertex_count, std::uint32_t instance_count) {
    if (!valid() || vertex_count == 0 || instance_count == 0) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    if (command_record == nullptr || command_record->closed || command_record->queue != QueueKind::graphics) {
        return false;
    }
    if (command_record->graphics_pipeline.value == 0 || !command_record->render_target_set) {
        return false;
    }

    command_record->list->DrawInstanced(vertex_count, instance_count, 0, 0);
    ++impl_->stats.draw_calls;
    impl_->stats.vertices_submitted += static_cast<std::uint64_t>(vertex_count) * instance_count;
    return true;
}

bool DeviceContext::draw_indexed(NativeCommandListHandle commands, std::uint32_t index_count,
                                 std::uint32_t instance_count) {
    if (!valid() || index_count == 0 || instance_count == 0) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    if (command_record == nullptr || command_record->closed || command_record->queue != QueueKind::graphics) {
        return false;
    }
    if (command_record->graphics_pipeline.value == 0 || !command_record->render_target_set) {
        return false;
    }

    command_record->list->DrawIndexedInstanced(index_count, instance_count, 0, 0, 0);
    ++impl_->stats.draw_calls;
    ++impl_->stats.indexed_draw_calls;
    impl_->stats.indices_submitted += static_cast<std::uint64_t>(index_count) * instance_count;
    return true;
}

bool DeviceContext::dispatch(NativeCommandListHandle commands, std::uint32_t group_count_x, std::uint32_t group_count_y,
                             std::uint32_t group_count_z) {
    if (!valid() || group_count_x == 0 || group_count_y == 0 || group_count_z == 0) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    if (command_record == nullptr || command_record->closed || command_record->queue != QueueKind::compute) {
        return false;
    }
    if (command_record->compute_pipeline.value == 0) {
        return false;
    }

    command_record->list->Dispatch(group_count_x, group_count_y, group_count_z);
    ++impl_->stats.compute_dispatches;
    impl_->stats.compute_workgroups_x += group_count_x;
    impl_->stats.compute_workgroups_y += group_count_y;
    impl_->stats.compute_workgroups_z += group_count_z;
    return true;
}

bool DeviceContext::set_viewport(NativeCommandListHandle commands, const mirakana::rhi::ViewportDesc& viewport) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    if (command_record == nullptr || command_record->closed || command_record->queue != QueueKind::graphics) {
        return false;
    }
    if (!command_record->render_target_set) {
        return false;
    }
    if (!std::isfinite(viewport.x) || !std::isfinite(viewport.y) || !std::isfinite(viewport.width) ||
        !std::isfinite(viewport.height) || !std::isfinite(viewport.min_depth) || !std::isfinite(viewport.max_depth)) {
        return false;
    }
    if (viewport.width <= 0.0F || viewport.height <= 0.0F) {
        return false;
    }

    D3D12_VIEWPORT vp{};
    vp.TopLeftX = viewport.x;
    vp.TopLeftY = viewport.y;
    vp.Width = viewport.width;
    vp.Height = viewport.height;
    vp.MinDepth = viewport.min_depth;
    vp.MaxDepth = viewport.max_depth;
    command_record->list->RSSetViewports(1, &vp);
    return true;
}

bool DeviceContext::set_scissor(NativeCommandListHandle commands, const mirakana::rhi::ScissorRectDesc& scissor) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    if (command_record == nullptr || command_record->closed || command_record->queue != QueueKind::graphics) {
        return false;
    }
    if (!command_record->render_target_set) {
        return false;
    }
    if (scissor.width == 0U || scissor.height == 0U) {
        return false;
    }

    D3D12_RECT rect{};
    rect.left = static_cast<LONG>(scissor.x);
    rect.top = static_cast<LONG>(scissor.y);
    rect.right = static_cast<LONG>(static_cast<std::uint64_t>(scissor.x) + static_cast<std::uint64_t>(scissor.width));
    rect.bottom = static_cast<LONG>(static_cast<std::uint64_t>(scissor.y) + static_cast<std::uint64_t>(scissor.height));
    command_record->list->RSSetScissorRects(1, &rect);
    return true;
}

bool DeviceContext::copy_buffer(NativeCommandListHandle commands, NativeResourceHandle source,
                                NativeResourceHandle destination, const BufferCopyRegion& region) {
    if (!valid() || region.size_bytes == 0) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    ID3D12Resource* source_resource = impl_->resource_record(source);
    ID3D12Resource* destination_resource = impl_->resource_record(destination);
    if (command_record == nullptr || source_resource == nullptr || destination_resource == nullptr ||
        command_record->closed) {
        return false;
    }

    const auto source_desc = source_resource->GetDesc();
    const auto destination_desc = destination_resource->GetDesc();
    if (source_desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
        destination_desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || region.source_offset > source_desc.Width ||
        region.size_bytes > source_desc.Width - region.source_offset ||
        region.destination_offset > destination_desc.Width ||
        region.size_bytes > destination_desc.Width - region.destination_offset) {
        return false;
    }

    command_record->list->CopyBufferRegion(destination_resource, region.destination_offset, source_resource,
                                           region.source_offset, region.size_bytes);
    ++impl_->stats.buffer_copies;
    impl_->stats.bytes_copied += region.size_bytes;
    return true;
}

bool DeviceContext::copy_buffer_to_texture(NativeCommandListHandle commands, NativeResourceHandle source,
                                           NativeResourceHandle destination, const BufferTextureCopyRegion& region) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    ID3D12Resource* source_resource = impl_->resource_record(source);
    ID3D12Resource* destination_resource = impl_->resource_record(destination);
    if (command_record == nullptr || source_resource == nullptr || destination_resource == nullptr ||
        command_record->closed) {
        return false;
    }

    const auto source_desc = source_resource->GetDesc();
    const auto destination_desc = destination_resource->GetDesc();
    if (source_desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
        destination_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        return false;
    }
    if (!activate_placed_texture(commands, destination)) {
        return false;
    }

    const auto footprint = d3d12_linear_texture_footprint(from_dxgi_format(destination_desc.Format), region);
    if (footprint.required_bytes > source_desc.Width) {
        return false;
    }

    D3D12_TEXTURE_COPY_LOCATION source_location{};
    source_location.pResource = source_resource;
    source_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    source_location.PlacedFootprint = footprint.placed;

    D3D12_TEXTURE_COPY_LOCATION destination_location{};
    destination_location.pResource = destination_resource;
    destination_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destination_location.SubresourceIndex = 0;

    command_record->list->CopyTextureRegion(&destination_location, region.texture_offset.x, region.texture_offset.y,
                                            region.texture_offset.z, &source_location, nullptr);
    ++impl_->stats.buffer_texture_copies;
    return true;
}

bool DeviceContext::copy_texture_to_buffer(NativeCommandListHandle commands, NativeResourceHandle source,
                                           NativeResourceHandle destination, const BufferTextureCopyRegion& region) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* command_record = impl_->command_list(commands);
    ID3D12Resource* source_resource = impl_->resource_record(source);
    ID3D12Resource* destination_resource = impl_->resource_record(destination);
    if (command_record == nullptr || source_resource == nullptr || destination_resource == nullptr ||
        command_record->closed) {
        return false;
    }

    const auto source_desc = source_resource->GetDesc();
    const auto destination_desc = destination_resource->GetDesc();
    if (source_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
        destination_desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
        return false;
    }
    if (!activate_placed_texture(commands, source)) {
        return false;
    }

    const auto footprint = d3d12_linear_texture_footprint(from_dxgi_format(source_desc.Format), region);
    if (footprint.required_bytes > destination_desc.Width) {
        return false;
    }

    D3D12_TEXTURE_COPY_LOCATION source_location{};
    source_location.pResource = source_resource;
    source_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    source_location.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION destination_location{};
    destination_location.pResource = destination_resource;
    destination_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    destination_location.PlacedFootprint = footprint.placed;

    D3D12_BOX source_box{};
    source_box.left = region.texture_offset.x;
    source_box.top = region.texture_offset.y;
    source_box.front = region.texture_offset.z;
    source_box.right = region.texture_offset.x + region.texture_extent.width;
    source_box.bottom = region.texture_offset.y + region.texture_extent.height;
    source_box.back = region.texture_offset.z + region.texture_extent.depth;

    command_record->list->CopyTextureRegion(&destination_location, 0, 0, 0, &source_location, &source_box);
    ++impl_->stats.texture_buffer_copies;
    return true;
}

bool DeviceContext::write_buffer(NativeResourceHandle buffer, std::uint64_t offset,
                                 std::span<const std::uint8_t> bytes) {
    if (!valid() || bytes.empty()) {
        return false;
    }

    ID3D12Resource* resource = impl_->resource_record(buffer);
    if (resource == nullptr) {
        return false;
    }

    const auto desc = resource->GetDesc();
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || offset > desc.Width ||
        bytes.size() > desc.Width - offset) {
        return false;
    }

    const D3D12_RANGE read_range{0, 0};
    void* mapped = nullptr;
    if (FAILED(resource->Map(0, &read_range, &mapped)) || mapped == nullptr) {
        return false;
    }

    const auto mapped_bytes =
        std::span<std::uint8_t>{static_cast<std::uint8_t*>(mapped), static_cast<std::size_t>(desc.Width)};
    const auto source_bytes = std::as_bytes(std::span{bytes});
    std::memcpy(mapped_bytes.subspan(static_cast<std::size_t>(offset), source_bytes.size()).data(), source_bytes.data(),
                source_bytes.size());
    const D3D12_RANGE written_range{
        static_cast<SIZE_T>(offset),
        static_cast<SIZE_T>(offset + bytes.size()),
    };
    resource->Unmap(0, &written_range);
    return true;
}

std::vector<std::uint8_t> DeviceContext::read_buffer(NativeResourceHandle buffer, std::uint64_t offset,
                                                     std::uint64_t size_bytes) const {
    if (!valid() || size_bytes == 0) {
        return {};
    }

    ID3D12Resource* resource = impl_->resource_record(buffer);
    if (resource == nullptr) {
        return {};
    }

    const auto desc = resource->GetDesc();
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER || offset > desc.Width || size_bytes > desc.Width - offset) {
        return {};
    }

    const D3D12_RANGE read_range{
        static_cast<SIZE_T>(offset),
        static_cast<SIZE_T>(offset + size_bytes),
    };
    void* mapped = nullptr;
    if (FAILED(resource->Map(0, &read_range, &mapped)) || mapped == nullptr) {
        return {};
    }

    const std::span<const std::uint8_t> mapped_data{
        static_cast<const std::uint8_t*>(mapped),
        static_cast<std::size_t>(desc.Width),
    };
    const std::span<const std::uint8_t> bytes =
        mapped_data.subspan(static_cast<std::size_t>(offset), static_cast<std::size_t>(size_bytes));
    std::vector<std::uint8_t> result(bytes.begin(), bytes.end());
    const D3D12_RANGE written_range{0, 0};
    resource->Unmap(0, &written_range);
    return result;
}

D3d12SharedTextureExportResult DeviceContext::export_shared_texture(NativeResourceHandle texture,
                                                                    const TextureDesc& desc) {
    D3d12SharedTextureExportResult result{};
    result.extent = Extent2D{.width = desc.extent.width, .height = desc.extent.height};
    result.format = desc.format;

    if (!valid()) {
        result.device_unavailable = true;
        return result;
    }
    if (!has_flag(desc.usage, TextureUsage::shared)) {
        result.texture_not_shareable = true;
        return result;
    }

    ID3D12Resource* resource = impl_->resource_record(texture);
    if (resource == nullptr) {
        result.invalid_texture = true;
        return result;
    }

    HANDLE shared_handle = nullptr;
    const HRESULT create_result =
        impl_->device->CreateSharedHandle(resource, nullptr, GENERIC_ALL, nullptr, &shared_handle);
    if (FAILED(create_result) || shared_handle == nullptr) {
        result.native_error_code = static_cast<std::uint32_t>(create_result);
        return result;
    }

    result.succeeded = true;
    result.shared_handle = D3d12SharedTextureHandle{reinterpret_cast<std::uintptr_t>(shared_handle)};
    ++impl_->stats.shared_texture_handles_created;
    return result;
}

NativeCommandListHandle DeviceContext::create_command_list(QueueKind queue) {
    if (!valid() || !valid_queue_kind(queue) || impl_->ensure_queue(queue) == nullptr) {
        return NativeCommandListHandle{};
    }

    const auto type = command_list_type(queue);

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    if (FAILED(impl_->device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)))) {
        return NativeCommandListHandle{};
    }

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
    if (FAILED(impl_->device->CreateCommandList(0, type, allocator.Get(), nullptr, IID_PPV_ARGS(&list)))) {
        return NativeCommandListHandle{};
    }

    const auto list_index = static_cast<unsigned>(impl_->command_lists.size() + 1U);
    d3d12_set_object_name_fmt(allocator.Get(), L"GameEngine.RHI.D3D12.CommandList%u.Allocator", list_index);
    const wchar_t* queue_tag = L"Graphics";
    if (queue == QueueKind::compute) {
        queue_tag = L"Compute";
    } else if (queue == QueueKind::copy) {
        queue_tag = L"Copy";
    }
    d3d12_set_object_name_fmt(list.Get(), L"GameEngine.RHI.D3D12.CommandList%u.%ls", list_index, queue_tag);

    impl_->command_lists.push_back(CommandListRecord{
        .queue = queue,
        .allocator = allocator,
        .list = list,
        .submitted_timing_query_heap = nullptr,
        .submitted_timing_readback = nullptr,
        .submitted_fence = FenceValue{},
        .submitted_timing_calibration_before = QueueClockCalibration{},
        .graphics_root_signature = NativeRootSignatureHandle{},
        .compute_root_signature = NativeRootSignatureHandle{},
        .descriptor_heaps = NativeDescriptorHeapBinding{},
        .graphics_pipeline = NativeGraphicsPipelineHandle{},
        .compute_pipeline = NativeComputePipelineHandle{},
        .placed_resource_state_updates = {},
        .render_target_set = false,
        .closed = false,
        .submitted = false,
        .submitted_timing_begin_recorded = false,
        .submitted_timing_resolved = false,
        .submitted_timing_frequency = 0,
        .submitted_timing_status = SubmittedCommandCalibratedTimingStatus::unsupported,
        .submitted_timing_diagnostic = {},
        .gpu_debug_scope_depth = 0,
    });
    auto& record = impl_->command_lists.back();
    if (queue == QueueKind::copy) {
        record.submitted_timing_status = SubmittedCommandCalibratedTimingStatus::unsupported;
        record.submitted_timing_diagnostic = "d3d12 copy queue submitted command timing is not enabled";
    } else {
        const auto support = queue_timestamp_measurement_support(queue);
        record.submitted_timing_frequency = support.frequency;
        if (!support.supported) {
            record.submitted_timing_status = SubmittedCommandCalibratedTimingStatus::unsupported;
            record.submitted_timing_diagnostic = support.diagnostic;
        } else {
            D3D12_QUERY_HEAP_DESC query_heap_desc{};
            query_heap_desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
            query_heap_desc.Count = k_submitted_command_timestamp_count;
            query_heap_desc.NodeMask = 0;

            const auto readback_heap = heap_properties(D3D12_HEAP_TYPE_READBACK);
            const auto readback_desc = buffer_resource_desc(k_submitted_command_timestamp_readback_size);
            if (FAILED(impl_->device->CreateQueryHeap(&query_heap_desc,
                                                      IID_PPV_ARGS(&record.submitted_timing_query_heap))) ||
                FAILED(impl_->device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &readback_desc,
                                                              D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                              IID_PPV_ARGS(&record.submitted_timing_readback)))) {
                record.submitted_timing_status = SubmittedCommandCalibratedTimingStatus::failed;
                record.submitted_timing_diagnostic = "d3d12 submitted command timing resource creation failed";
                record.submitted_timing_query_heap.Reset();
                record.submitted_timing_readback.Reset();
            } else {
                d3d12_set_object_name_fmt(record.submitted_timing_query_heap.Get(),
                                          L"GameEngine.RHI.D3D12.CommandList%u.%lsSubmittedTimingQueryHeap", list_index,
                                          queue_tag);
                d3d12_set_object_name_fmt(record.submitted_timing_readback.Get(),
                                          L"GameEngine.RHI.D3D12.CommandList%u.%lsSubmittedTimingReadback", list_index,
                                          queue_tag);
                if (!record_submitted_command_timing_begin(record)) {
                    record.submitted_timing_status = SubmittedCommandCalibratedTimingStatus::failed;
                    record.submitted_timing_diagnostic = "d3d12 submitted command timing begin query failed";
                }
            }
        }
    }
    ++impl_->stats.command_allocators_created;
    ++impl_->stats.command_lists_created;
    impl_->stats.command_lists_alive = impl_->command_lists.size();

    return NativeCommandListHandle{static_cast<std::uint32_t>(impl_->command_lists.size())};
}

bool DeviceContext::close_command_list(NativeCommandListHandle handle) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* record = impl_->command_list(handle);
    if (record == nullptr || record->closed) {
        return false;
    }

    if (record->submitted_timing_query_heap != nullptr && record->submitted_timing_readback != nullptr &&
        !resolve_submitted_command_timing(*record)) {
        record->submitted_timing_status = SubmittedCommandCalibratedTimingStatus::failed;
        record->submitted_timing_diagnostic = "d3d12 submitted command timing resolve failed";
    }

    if (FAILED(record->list->Close())) {
        if (record->submitted_timing_query_heap != nullptr && record->submitted_timing_readback != nullptr) {
            record->submitted_timing_status = SubmittedCommandCalibratedTimingStatus::failed;
            record->submitted_timing_diagnostic = "d3d12 submitted command timing command list close failed";
        }
        return false;
    }

    record->closed = true;
    ++impl_->stats.command_lists_closed;
    return true;
}

bool DeviceContext::reset_command_list(NativeCommandListHandle handle) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* record = impl_->command_list(handle);
    if (record == nullptr || !record->closed) {
        return false;
    }

    if (record->submitted && impl_->fence->GetCompletedValue() < record->submitted_fence.value) {
        return false;
    }

    if (FAILED(record->allocator->Reset())) {
        return false;
    }
    if (FAILED(record->list->Reset(record->allocator.Get(), nullptr))) {
        return false;
    }

    record->closed = false;
    record->submitted = false;
    record->submitted_fence = FenceValue{};
    record->graphics_root_signature = NativeRootSignatureHandle{};
    record->compute_root_signature = NativeRootSignatureHandle{};
    record->descriptor_heaps = NativeDescriptorHeapBinding{};
    record->graphics_pipeline = NativeGraphicsPipelineHandle{};
    record->compute_pipeline = NativeComputePipelineHandle{};
    record->placed_resource_state_updates.clear();
    record->render_target_set = false;
    record->gpu_debug_scope_depth = 0;
    reset_submitted_command_timing_state(*record);
    if (record->submitted_timing_query_heap != nullptr && record->submitted_timing_readback != nullptr &&
        !record_submitted_command_timing_begin(*record)) {
        record->submitted_timing_status = SubmittedCommandCalibratedTimingStatus::failed;
        record->submitted_timing_diagnostic = "d3d12 submitted command timing reset begin query failed";
    }
    ++impl_->stats.command_lists_reset;
    return true;
}

namespace {

/// `D3D12_EVENT_METADATA_STRING` is not available on all minimum Windows SDK headers included by this translation
/// unit; keep the PIX/D3D12 documented constant value in one place.
constexpr UINT k_d3d12_gpu_debug_string_metadata = 1U;

} // namespace

FenceValue DeviceContext::execute_command_list(NativeCommandListHandle handle) {
    if (!valid()) {
        return FenceValue{};
    }

    CommandListRecord* record = impl_->command_list(handle);
    if (record == nullptr || !record->closed || record->submitted) {
        return FenceValue{};
    }

    ID3D12CommandQueue* queue = impl_->ensure_queue(record->queue);
    if (queue == nullptr) {
        return FenceValue{};
    }
    ID3D12Fence* fence = impl_->ensure_fence(record->queue);
    if (fence == nullptr) {
        return FenceValue{};
    }

    const bool can_read_submitted_timing = record->submitted_timing_resolved &&
                                           record->submitted_timing_query_heap != nullptr &&
                                           record->submitted_timing_readback != nullptr;
    if (can_read_submitted_timing) {
        const auto before = calibrate_queue_clock(record->queue);
        record->submitted_timing_calibration_before = before;
        if (before.status == QueueClockCalibrationStatus::unsupported) {
            record->submitted_timing_status = SubmittedCommandCalibratedTimingStatus::unsupported;
            record->submitted_timing_diagnostic = before.diagnostic;
        } else if (before.status != QueueClockCalibrationStatus::ready) {
            record->submitted_timing_status = SubmittedCommandCalibratedTimingStatus::failed;
            record->submitted_timing_diagnostic = before.diagnostic;
        }
    }

    std::array<ID3D12CommandList*, 1> lists{record->list.Get()};
    queue->ExecuteCommandLists(1, lists.data());

    for (const auto& update : record->placed_resource_state_updates) {
        if (update.before.value != 0) {
            impl_->set_resource_placed_active(update.before, false);
        }
        if (update.after.value != 0) {
            impl_->set_resource_placed_active(update.after, true);
        }
    }
    record->placed_resource_state_updates.clear();

    std::uint64_t& next_fence_value =
        DeviceContext::Impl::queue_fence_value(impl_->next_queue_fence_values, record->queue);
    const auto next_fence = next_fence_value + 1U;
    if (FAILED(queue->Signal(fence, next_fence))) {
        return FenceValue{};
    }

    next_fence_value = next_fence;
    impl_->next_fence_value = std::max(impl_->next_fence_value, next_fence);
    record->submitted = true;
    record->submitted_fence = FenceValue{.value = next_fence, .queue = record->queue};
    impl_->last_submitted_fence = record->submitted_fence;
    if (can_read_submitted_timing &&
        record->submitted_timing_calibration_before.status == QueueClockCalibrationStatus::ready) {
        record->submitted_timing_status = SubmittedCommandCalibratedTimingStatus::not_ready;
        record->submitted_timing_diagnostic = "d3d12 submitted command calibrated timing pending fence completion";
    }
    ++impl_->stats.command_lists_executed;
    ++impl_->stats.fence_signals;
    impl_->stats.last_submitted_fence_value = next_fence;
    record_queue_submit(impl_->stats, record->queue, record->submitted_fence);
    return record->submitted_fence;
}

void DeviceContext::unwind_gpu_debug_events(NativeCommandListHandle handle) noexcept {
    if (!valid()) {
        return;
    }

    CommandListRecord* record = impl_->command_list(handle);
    if (record == nullptr || record->closed || record->list == nullptr) {
        return;
    }

    while (record->gpu_debug_scope_depth > 0) {
        record->list->EndEvent();
        --record->gpu_debug_scope_depth;
    }
}

bool DeviceContext::begin_gpu_debug_event(NativeCommandListHandle handle, std::string_view name) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* record = impl_->command_list(handle);
    if (record == nullptr || record->closed || record->list == nullptr) {
        return false;
    }

    record->list->BeginEvent(k_d3d12_gpu_debug_string_metadata, name.data(), static_cast<UINT>(name.size()));
    ++record->gpu_debug_scope_depth;
    return true;
}

bool DeviceContext::end_gpu_debug_event(NativeCommandListHandle handle) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* record = impl_->command_list(handle);
    if (record == nullptr || record->closed || record->list == nullptr || record->gpu_debug_scope_depth == 0) {
        return false;
    }

    record->list->EndEvent();
    --record->gpu_debug_scope_depth;
    return true;
}

bool DeviceContext::insert_gpu_debug_marker(NativeCommandListHandle handle, std::string_view name) {
    if (!valid()) {
        return false;
    }

    CommandListRecord* record = impl_->command_list(handle);
    if (record == nullptr || record->closed || record->list == nullptr) {
        return false;
    }

    record->list->SetMarker(k_d3d12_gpu_debug_string_metadata, name.data(), static_cast<UINT>(name.size()));
    return true;
}

std::uint32_t DeviceContext::gpu_debug_scope_depth(NativeCommandListHandle handle) noexcept {
    if (!valid()) {
        return 0;
    }

    CommandListRecord* record = impl_->command_list(handle);
    if (record == nullptr) {
        return 0;
    }
    return record->gpu_debug_scope_depth;
}

std::uint64_t DeviceContext::gpu_timestamp_ticks_per_second() const noexcept {
    if (!valid() || impl_->command_queue == nullptr) {
        return 0;
    }

    std::uint64_t frequency = 0;
    if (FAILED(impl_->command_queue->GetTimestampFrequency(&frequency))) {
        return 0;
    }
    return frequency;
}

RhiDeviceMemoryDiagnostics DeviceContext::memory_diagnostics() const {
    RhiDeviceMemoryDiagnostics diagnostics{};
    if (!valid()) {
        return diagnostics;
    }

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
    if (SUCCEEDED(impl_->device.As(&dxgi_device))) {
        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        if (SUCCEEDED(dxgi_device->GetAdapter(&adapter))) {
            Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter3;
            if (SUCCEEDED(adapter.As(&adapter3))) {
                DXGI_QUERY_VIDEO_MEMORY_INFO local_info{};
                if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local_info))) {
                    diagnostics.os_video_memory_budget_available = true;
                    diagnostics.local_video_memory_budget_bytes = local_info.Budget;
                    diagnostics.local_video_memory_usage_bytes = local_info.CurrentUsage;
                }
                DXGI_QUERY_VIDEO_MEMORY_INFO non_local_info{};
                if (SUCCEEDED(
                        adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &non_local_info))) {
                    diagnostics.non_local_video_memory_budget_bytes = non_local_info.Budget;
                    diagnostics.non_local_video_memory_usage_bytes = non_local_info.CurrentUsage;
                }
            }
        }
    }

    std::uint64_t committed = 0;
    for (std::size_t index = 0; index < impl_->resources.size(); ++index) {
        const auto& resource = impl_->resources[index];
        if (resource == nullptr) {
            continue;
        }
        if (index < impl_->resource_heaps.size() && impl_->resource_heaps[index] != nullptr) {
            continue;
        }
        const D3D12_RESOURCE_DESC desc = resource->GetDesc();
        const D3D12_RESOURCE_ALLOCATION_INFO allocation = impl_->device->GetResourceAllocationInfo(0, 1, &desc);
        committed += allocation.SizeInBytes;
    }
    diagnostics.committed_resources_byte_estimate = committed;
    diagnostics.committed_resources_byte_estimate_available = true;
    return diagnostics;
}

FenceValue DeviceContext::completed_fence() const noexcept {
    if (!valid() || impl_->last_submitted_fence.value == 0) {
        return FenceValue{};
    }
    ID3D12Fence* fence = DeviceContext::Impl::queue_fence(impl_->queue_fences, impl_->last_submitted_fence.queue).Get();
    if (fence == nullptr) {
        return FenceValue{};
    }
    return FenceValue{.value = fence->GetCompletedValue(), .queue = impl_->last_submitted_fence.queue};
}

FenceValue DeviceContext::last_submitted_fence() const noexcept {
    if (!valid()) {
        return FenceValue{};
    }
    return impl_->last_submitted_fence;
}

void DeviceContext::destroy_committed_resource(NativeResourceHandle handle) noexcept {
    if (!valid() || handle.value == 0 || handle.value > impl_->resources.size()) {
        return;
    }

    const auto index = handle.value - 1U;
    Microsoft::WRL::ComPtr<ID3D12Resource>& resource = impl_->resources.at(index);
    if (resource.Get() == nullptr) {
        return;
    }

    const bool placed = index < impl_->resource_heaps.size() && impl_->resource_heaps.at(index) != nullptr;
    impl_->resource_rtvs.at(index).heap.Reset();
    impl_->resource_rtvs.at(index).descriptor_size = 0;
    impl_->resource_dsvs.at(index).heap.Reset();
    impl_->resource_dsvs.at(index).descriptor_size = 0;
    resource.Reset();
    if (placed) {
        impl_->resource_heaps.at(index).Reset();
        impl_->set_resource_placed_active(handle, false);
    }
    if (placed && impl_->stats.placed_resources_alive > 0) {
        --impl_->stats.placed_resources_alive;
    } else if (!placed && impl_->stats.committed_resources_alive > 0) {
        --impl_->stats.committed_resources_alive;
    }
}

void DeviceContext::destroy_graphics_pipeline(NativeGraphicsPipelineHandle handle) noexcept {
    if (!valid() || handle.value == 0 || handle.value > impl_->graphics_pipelines.size()) {
        return;
    }
    auto& record = impl_->graphics_pipelines[handle.value - 1U];
    if (record.pipeline_state == nullptr) {
        return;
    }
    record.pipeline_state.Reset();
    record.info.valid = false;
}

void DeviceContext::destroy_compute_pipeline(NativeComputePipelineHandle handle) noexcept {
    if (!valid() || handle.value == 0 || handle.value > impl_->compute_pipelines.size()) {
        return;
    }
    auto& record = impl_->compute_pipelines[handle.value - 1U];
    if (record.pipeline_state == nullptr) {
        return;
    }
    record.pipeline_state.Reset();
    record.info.valid = false;
}

void DeviceContext::destroy_root_signature(NativeRootSignatureHandle handle) noexcept {
    if (!valid() || handle.value == 0 || handle.value > impl_->root_signatures.size()) {
        return;
    }
    auto& record = impl_->root_signatures[handle.value - 1U];
    if (record.root_signature == nullptr) {
        return;
    }
    record.root_signature.Reset();
    record.info.valid = false;
}

void DeviceContext::destroy_shader_module(NativeShaderHandle handle) noexcept {
    if (!valid() || handle.value == 0 || handle.value > impl_->shaders.size()) {
        return;
    }
    auto& record = impl_->shaders[handle.value - 1U];
    if (record.bytecode.empty()) {
        return;
    }
    record.bytecode.clear();
    record.bytecode.shrink_to_fit();
}

bool DeviceContext::wait_for_fence(FenceValue fence, std::uint32_t timeout_ms) {
    if (!valid()) {
        return false;
    }

    ++impl_->stats.fence_waits;

    if (!valid_queue_kind(fence.queue)) {
        ++impl_->stats.fence_wait_timeouts;
        return false;
    }

    ID3D12Fence* source_fence = impl_->ensure_fence(fence.queue);
    if (source_fence == nullptr) {
        ++impl_->stats.fence_wait_timeouts;
        return false;
    }

    if (fence.value <= source_fence->GetCompletedValue()) {
        return true;
    }

    const std::uint64_t& next_fence_for_fence_queue =
        DeviceContext::Impl::queue_fence_value(impl_->next_queue_fence_values, fence.queue);
    if (fence.value > next_fence_for_fence_queue) {
        ++impl_->stats.fence_wait_timeouts;
        return false;
    }

    const Win32Event event;
    if (event.get() == nullptr) {
        return false;
    }

    if (FAILED(source_fence->SetEventOnCompletion(fence.value, event.get()))) {
        return false;
    }

    const DWORD wait_timeout = timeout_ms == 0xFFFFFFFFU ? INFINITE : static_cast<DWORD>(timeout_ms);
    const DWORD wait_result = WaitForSingleObject(event.get(), wait_timeout);
    if (wait_result == WAIT_OBJECT_0) {
        return true;
    }

    if (wait_result == WAIT_TIMEOUT) {
        ++impl_->stats.fence_wait_timeouts;
    }
    return false;
}

bool DeviceContext::queue_wait_for_fence(QueueKind queue, FenceValue fence) {
    if (!valid()) {
        return false;
    }

    ++impl_->stats.queue_waits;

    const std::uint64_t& next_fence_for_fence_queue =
        DeviceContext::Impl::queue_fence_value(impl_->next_queue_fence_values, fence.queue);
    if (!valid_queue_kind(queue) || !valid_queue_kind(fence.queue) || fence.value > next_fence_for_fence_queue) {
        ++impl_->stats.queue_wait_failures;
        return false;
    }

    if (fence.value == 0) {
        return true;
    }

    ID3D12CommandQueue* command_queue = impl_->ensure_queue(queue);
    ID3D12Fence* source_fence = impl_->ensure_fence(fence.queue);
    if (command_queue == nullptr || source_fence == nullptr || FAILED(command_queue->Wait(source_fence, fence.value))) {
        ++impl_->stats.queue_wait_failures;
        return false;
    }

    record_queue_wait(impl_->stats, queue, fence);
    return true;
}

QueueTimestampMeasurementSupport DeviceContext::queue_timestamp_measurement_support(QueueKind queue) {
    QueueTimestampMeasurementSupport result{};
    result.queue = queue;

    if (!valid() || !valid_queue_kind(queue)) {
        result.diagnostic = "d3d12 queue timestamp measurement unavailable";
        return result;
    }

    if (queue == QueueKind::copy) {
        result.diagnostic = "d3d12 copy queue timestamp measurement is not enabled";
        return result;
    }

    ID3D12CommandQueue* command_queue = impl_->ensure_queue(queue);
    if (command_queue == nullptr) {
        result.diagnostic = "d3d12 queue timestamp measurement queue unavailable";
        return result;
    }

    std::uint64_t frequency = 0;
    if (FAILED(command_queue->GetTimestampFrequency(&frequency)) || frequency == 0) {
        result.diagnostic = "d3d12 queue timestamp measurement unsupported";
        return result;
    }

    result.supported = true;
    result.frequency = frequency;
    result.diagnostic = "d3d12 queue timestamp measurement ready";
    return result;
}

QueueTimestampInterval DeviceContext::measure_queue_timestamp_interval(QueueKind queue) {
    QueueTimestampInterval result{};
    result.queue = queue;

    const auto support = queue_timestamp_measurement_support(queue);
    result.frequency = support.frequency;
    if (!support.supported) {
        result.status = QueueTimestampMeasurementStatus::unsupported;
        result.diagnostic = support.diagnostic;
        return result;
    }

    D3D12_QUERY_HEAP_DESC query_heap_desc{};
    query_heap_desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    query_heap_desc.Count = 2;
    query_heap_desc.NodeMask = 0;

    Microsoft::WRL::ComPtr<ID3D12QueryHeap> query_heap;
    if (FAILED(impl_->device->CreateQueryHeap(&query_heap_desc, IID_PPV_ARGS(&query_heap)))) {
        result.status = QueueTimestampMeasurementStatus::failed;
        result.diagnostic = "d3d12 queue timestamp query heap creation failed";
        return result;
    }

    constexpr auto timestamp_count = 2U;
    constexpr auto timestamp_readback_size = sizeof(std::uint64_t) * timestamp_count;
    const auto readback_heap = heap_properties(D3D12_HEAP_TYPE_READBACK);
    const auto readback_desc = buffer_resource_desc(timestamp_readback_size);

    Microsoft::WRL::ComPtr<ID3D12Resource> readback;
    if (FAILED(impl_->device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &readback_desc,
                                                      D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                      IID_PPV_ARGS(&readback)))) {
        result.status = QueueTimestampMeasurementStatus::failed;
        result.diagnostic = "d3d12 queue timestamp readback creation failed";
        return result;
    }

    const wchar_t* queue_tag = queue == QueueKind::compute ? L"Compute" : L"Graphics";
    d3d12_set_object_name_fmt(query_heap.Get(), L"GameEngine.RHI.D3D12.%lsTimestampQueryHeap", queue_tag);
    d3d12_set_object_name_fmt(readback.Get(), L"GameEngine.RHI.D3D12.%lsTimestampReadback", queue_tag);

    const auto commands = create_command_list(queue);
    CommandListRecord* record = impl_->command_list(commands);
    if (record == nullptr || record->list == nullptr || record->closed) {
        result.status = QueueTimestampMeasurementStatus::failed;
        result.diagnostic = "d3d12 queue timestamp command list creation failed";
        return result;
    }

    record->list->EndQuery(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
    record->list->EndQuery(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
    record->list->ResolveQueryData(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, timestamp_count, readback.Get(), 0);

    if (!close_command_list(commands)) {
        result.status = QueueTimestampMeasurementStatus::failed;
        result.diagnostic = "d3d12 queue timestamp command list close failed";
        return result;
    }

    const auto fence = execute_command_list(commands);
    if (fence.value == 0 || !wait_for_fence(fence, 0xFFFFFFFFU)) {
        result.status = QueueTimestampMeasurementStatus::failed;
        result.diagnostic = "d3d12 queue timestamp command list execution failed";
        return result;
    }

    const D3D12_RANGE read_range{0, timestamp_readback_size};
    void* mapped = nullptr;
    if (FAILED(readback->Map(0, &read_range, &mapped)) || mapped == nullptr) {
        result.status = QueueTimestampMeasurementStatus::failed;
        result.diagnostic = "d3d12 queue timestamp readback map failed";
        return result;
    }

    const auto ticks = std::span<const std::uint64_t>{static_cast<const std::uint64_t*>(mapped), 2U};
    std::memcpy(&result.begin_ticks, ticks.data(), sizeof(result.begin_ticks));
    std::memcpy(&result.end_ticks, ticks.subspan(1U).data(), sizeof(result.end_ticks));
    const D3D12_RANGE written_range{0, 0};
    readback->Unmap(0, &written_range);

    if (result.end_ticks < result.begin_ticks) {
        result.status = QueueTimestampMeasurementStatus::failed;
        result.diagnostic = "d3d12 queue timestamp interval was not monotonic";
        return result;
    }

    result.elapsed_seconds =
        static_cast<double>(result.end_ticks - result.begin_ticks) / static_cast<double>(result.frequency);
    result.status = QueueTimestampMeasurementStatus::ready;
    result.diagnostic = "d3d12 queue timestamp interval measured";
    return result;
}

QueueClockCalibration DeviceContext::calibrate_queue_clock(QueueKind queue) {
    QueueClockCalibration result{};
    result.queue = queue;

    const auto support = queue_timestamp_measurement_support(queue);
    result.frequency = support.frequency;
    if (!support.supported) {
        result.status = QueueClockCalibrationStatus::unsupported;
        result.diagnostic = support.diagnostic;
        return result;
    }

    ID3D12CommandQueue* command_queue = impl_->ensure_queue(queue);
    if (command_queue == nullptr) {
        result.status = QueueClockCalibrationStatus::failed;
        result.diagnostic = "d3d12 queue clock calibration queue unavailable";
        return result;
    }

    std::uint64_t gpu_timestamp = 0;
    std::uint64_t cpu_timestamp = 0;
    if (FAILED(command_queue->GetClockCalibration(&gpu_timestamp, &cpu_timestamp)) || cpu_timestamp == 0) {
        result.status = QueueClockCalibrationStatus::failed;
        result.diagnostic = "d3d12 queue clock calibration failed";
        return result;
    }

    result.status = QueueClockCalibrationStatus::ready;
    result.gpu_timestamp = gpu_timestamp;
    result.cpu_timestamp = cpu_timestamp;
    result.diagnostic = "d3d12 queue clock calibration ready";
    return result;
}

namespace {

[[nodiscard]] std::uint64_t query_performance_counter_frequency() noexcept {
    LARGE_INTEGER frequency{};
    if (QueryPerformanceFrequency(&frequency) == 0 || frequency.QuadPart <= 0) {
        return 0;
    }
    return static_cast<std::uint64_t>(frequency.QuadPart);
}

[[nodiscard]] std::uint64_t convert_gpu_timestamp_to_qpc(std::uint64_t gpu_timestamp,
                                                         const QueueClockCalibration& calibration,
                                                         std::uint64_t queue_frequency,
                                                         std::uint64_t qpc_frequency) noexcept {
    if (queue_frequency == 0 || qpc_frequency == 0 || calibration.cpu_timestamp == 0) {
        return 0;
    }

    const long double delta_gpu = gpu_timestamp >= calibration.gpu_timestamp
                                      ? static_cast<long double>(gpu_timestamp - calibration.gpu_timestamp)
                                      : -static_cast<long double>(calibration.gpu_timestamp - gpu_timestamp);
    const long double qpc_timestamp =
        static_cast<long double>(calibration.cpu_timestamp) +
        (delta_gpu * static_cast<long double>(qpc_frequency) / static_cast<long double>(queue_frequency));

    if (qpc_timestamp <= 0.0L) {
        return 0;
    }
    const auto max_value = static_cast<long double>(std::numeric_limits<std::uint64_t>::max());
    if (qpc_timestamp >= max_value) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return static_cast<std::uint64_t>(std::round(qpc_timestamp));
}

} // namespace

QueueCalibratedTiming DeviceContext::measure_calibrated_queue_timing(QueueKind queue) {
    QueueCalibratedTiming result{};
    result.queue = queue;

    const auto before = calibrate_queue_clock(queue);
    result.frequency = before.frequency;
    if (before.status == QueueClockCalibrationStatus::unsupported) {
        result.status = QueueCalibratedTimingStatus::unsupported;
        result.diagnostic = before.diagnostic;
        return result;
    }
    if (before.status != QueueClockCalibrationStatus::ready) {
        result.status = QueueCalibratedTimingStatus::failed;
        result.diagnostic = before.diagnostic;
        return result;
    }

    result.calibration_before_gpu_timestamp = before.gpu_timestamp;
    result.calibration_before_cpu_timestamp = before.cpu_timestamp;
    result.qpc_frequency = query_performance_counter_frequency();
    if (result.qpc_frequency == 0) {
        result.status = QueueCalibratedTimingStatus::failed;
        result.diagnostic = "d3d12 calibrated queue timing qpc frequency unavailable";
        return result;
    }

    const auto interval = measure_queue_timestamp_interval(queue);
    result.frequency = interval.frequency;
    result.begin_ticks = interval.begin_ticks;
    result.end_ticks = interval.end_ticks;
    result.elapsed_seconds = interval.elapsed_seconds;
    if (interval.status == QueueTimestampMeasurementStatus::unsupported) {
        result.status = QueueCalibratedTimingStatus::unsupported;
        result.diagnostic = interval.diagnostic;
        return result;
    }
    if (interval.status != QueueTimestampMeasurementStatus::ready) {
        result.status = QueueCalibratedTimingStatus::failed;
        result.diagnostic = interval.diagnostic;
        return result;
    }

    const auto after = calibrate_queue_clock(queue);
    if (after.status != QueueClockCalibrationStatus::ready) {
        result.status = after.status == QueueClockCalibrationStatus::unsupported
                            ? QueueCalibratedTimingStatus::unsupported
                            : QueueCalibratedTimingStatus::failed;
        result.diagnostic = after.diagnostic;
        return result;
    }

    result.calibration_after_gpu_timestamp = after.gpu_timestamp;
    result.calibration_after_cpu_timestamp = after.cpu_timestamp;
    if (result.calibration_after_cpu_timestamp < result.calibration_before_cpu_timestamp) {
        result.status = QueueCalibratedTimingStatus::failed;
        result.diagnostic = "d3d12 calibrated queue timing qpc samples were not monotonic";
        return result;
    }

    result.calibrated_begin_cpu_timestamp =
        convert_gpu_timestamp_to_qpc(result.begin_ticks, before, result.frequency, result.qpc_frequency);
    result.calibrated_end_cpu_timestamp =
        convert_gpu_timestamp_to_qpc(result.end_ticks, before, result.frequency, result.qpc_frequency);
    if (result.calibrated_end_cpu_timestamp < result.calibrated_begin_cpu_timestamp) {
        result.status = QueueCalibratedTimingStatus::failed;
        result.diagnostic = "d3d12 calibrated queue timing interval was not monotonic";
        return result;
    }

    result.status = QueueCalibratedTimingStatus::ready;
    result.diagnostic = "d3d12 calibrated queue timing measured";
    return result;
}

SubmittedCommandCalibratedTiming DeviceContext::read_submitted_command_calibrated_timing(FenceValue fence) {
    SubmittedCommandCalibratedTiming result{};
    result.queue = fence.queue;
    result.submitted_fence = fence;

    if (!valid() || !valid_queue_kind(fence.queue)) {
        result.status = SubmittedCommandCalibratedTimingStatus::unsupported;
        result.diagnostic = "d3d12 submitted command calibrated timing unavailable";
        return result;
    }
    if (fence.value == 0) {
        result.status = SubmittedCommandCalibratedTimingStatus::not_submitted;
        result.diagnostic = "d3d12 submitted command calibrated timing requires a submitted fence";
        return result;
    }

    CommandListRecord* submitted_record = nullptr;
    for (auto& record : impl_->command_lists) {
        if (record.submitted && record.submitted_fence.value == fence.value &&
            record.submitted_fence.queue == fence.queue) {
            submitted_record = &record;
            break;
        }
    }

    if (submitted_record == nullptr) {
        result.status = SubmittedCommandCalibratedTimingStatus::not_submitted;
        result.diagnostic = "d3d12 submitted command calibrated timing fence was not submitted by this context";
        return result;
    }

    result.queue = submitted_record->queue;
    result.frequency = submitted_record->submitted_timing_frequency;
    result.calibration_before_gpu_timestamp = submitted_record->submitted_timing_calibration_before.gpu_timestamp;
    result.calibration_before_cpu_timestamp = submitted_record->submitted_timing_calibration_before.cpu_timestamp;
    if (submitted_record->submitted_timing_status == SubmittedCommandCalibratedTimingStatus::unsupported ||
        submitted_record->submitted_timing_status == SubmittedCommandCalibratedTimingStatus::failed ||
        submitted_record->submitted_timing_status == SubmittedCommandCalibratedTimingStatus::not_submitted) {
        result.status = submitted_record->submitted_timing_status;
        result.diagnostic = submitted_record->submitted_timing_diagnostic;
        return result;
    }
    if (submitted_record->submitted_timing_query_heap == nullptr ||
        submitted_record->submitted_timing_readback == nullptr || !submitted_record->submitted_timing_resolved ||
        result.frequency == 0 ||
        submitted_record->submitted_timing_calibration_before.status != QueueClockCalibrationStatus::ready) {
        result.status = SubmittedCommandCalibratedTimingStatus::failed;
        result.diagnostic = "d3d12 submitted command calibrated timing evidence is incomplete";
        return result;
    }

    ID3D12Fence* native_fence = impl_->ensure_fence(fence.queue);
    if (native_fence == nullptr) {
        result.status = SubmittedCommandCalibratedTimingStatus::failed;
        result.diagnostic = "d3d12 submitted command calibrated timing fence unavailable";
        return result;
    }
    if (native_fence->GetCompletedValue() < fence.value) {
        result.status = SubmittedCommandCalibratedTimingStatus::not_ready;
        result.diagnostic = "d3d12 submitted command calibrated timing pending fence completion";
        return result;
    }

    result.qpc_frequency = query_performance_counter_frequency();
    if (result.qpc_frequency == 0) {
        result.status = SubmittedCommandCalibratedTimingStatus::failed;
        result.diagnostic = "d3d12 submitted command calibrated timing qpc frequency unavailable";
        return result;
    }

    const D3D12_RANGE read_range{0, k_submitted_command_timestamp_readback_size};
    void* mapped = nullptr;
    if (FAILED(submitted_record->submitted_timing_readback->Map(0, &read_range, &mapped)) || mapped == nullptr) {
        result.status = SubmittedCommandCalibratedTimingStatus::failed;
        result.diagnostic = "d3d12 submitted command calibrated timing readback map failed";
        return result;
    }

    const auto ticks = std::span<const std::uint64_t>{static_cast<const std::uint64_t*>(mapped), 2U};
    std::memcpy(&result.begin_ticks, ticks.data(), sizeof(result.begin_ticks));
    std::memcpy(&result.end_ticks, ticks.subspan(1U).data(), sizeof(result.end_ticks));
    const D3D12_RANGE written_range{0, 0};
    submitted_record->submitted_timing_readback->Unmap(0, &written_range);
    if (result.end_ticks < result.begin_ticks) {
        result.status = SubmittedCommandCalibratedTimingStatus::failed;
        result.diagnostic = "d3d12 submitted command calibrated timing interval was not monotonic";
        return result;
    }
    result.elapsed_seconds =
        static_cast<double>(result.end_ticks - result.begin_ticks) / static_cast<double>(result.frequency);

    const auto after = calibrate_queue_clock(result.queue);
    if (after.status == QueueClockCalibrationStatus::unsupported) {
        result.status = SubmittedCommandCalibratedTimingStatus::unsupported;
        result.diagnostic = after.diagnostic;
        return result;
    }
    if (after.status != QueueClockCalibrationStatus::ready) {
        result.status = SubmittedCommandCalibratedTimingStatus::failed;
        result.diagnostic = after.diagnostic;
        return result;
    }

    result.calibration_after_gpu_timestamp = after.gpu_timestamp;
    result.calibration_after_cpu_timestamp = after.cpu_timestamp;
    if (result.calibration_after_cpu_timestamp < result.calibration_before_cpu_timestamp) {
        result.status = SubmittedCommandCalibratedTimingStatus::failed;
        result.diagnostic = "d3d12 submitted command calibrated timing qpc samples were not monotonic";
        return result;
    }

    result.calibrated_begin_cpu_timestamp =
        convert_gpu_timestamp_to_qpc(result.begin_ticks, submitted_record->submitted_timing_calibration_before,
                                     result.frequency, result.qpc_frequency);
    result.calibrated_end_cpu_timestamp =
        convert_gpu_timestamp_to_qpc(result.end_ticks, submitted_record->submitted_timing_calibration_before,
                                     result.frequency, result.qpc_frequency);
    if (result.calibrated_end_cpu_timestamp < result.calibrated_begin_cpu_timestamp) {
        result.status = SubmittedCommandCalibratedTimingStatus::failed;
        result.diagnostic = "d3d12 submitted command calibrated timing interval was not monotonic";
        return result;
    }

    result.status = SubmittedCommandCalibratedTimingStatus::ready;
    result.diagnostic = "d3d12 submitted command calibrated timing measured";
    submitted_record->submitted_timing_status = result.status;
    submitted_record->submitted_timing_diagnostic = result.diagnostic;
    return result;
}

namespace {

[[nodiscard]] QueueCalibratedTiming
make_queue_calibrated_timing(const SubmittedCommandCalibratedTiming& timing) noexcept {
    QueueCalibratedTiming result{};
    result.queue = timing.queue;
    result.frequency = timing.frequency;
    result.qpc_frequency = timing.qpc_frequency;
    result.begin_ticks = timing.begin_ticks;
    result.end_ticks = timing.end_ticks;
    result.elapsed_seconds = timing.elapsed_seconds;
    result.calibration_before_gpu_timestamp = timing.calibration_before_gpu_timestamp;
    result.calibration_before_cpu_timestamp = timing.calibration_before_cpu_timestamp;
    result.calibration_after_gpu_timestamp = timing.calibration_after_gpu_timestamp;
    result.calibration_after_cpu_timestamp = timing.calibration_after_cpu_timestamp;
    result.calibrated_begin_cpu_timestamp = timing.calibrated_begin_cpu_timestamp;
    result.calibrated_end_cpu_timestamp = timing.calibrated_end_cpu_timestamp;
    result.diagnostic = timing.diagnostic;
    if (timing.status == SubmittedCommandCalibratedTimingStatus::ready) {
        result.status = QueueCalibratedTimingStatus::ready;
    } else if (timing.status == SubmittedCommandCalibratedTimingStatus::unsupported) {
        result.status = QueueCalibratedTimingStatus::unsupported;
    } else {
        result.status = QueueCalibratedTimingStatus::failed;
    }
    return result;
}

} // namespace

QueueCalibratedOverlapDiagnostics
diagnose_calibrated_compute_graphics_overlap(const RhiAsyncOverlapReadinessDiagnostics& schedule,
                                             const QueueCalibratedTiming& compute,
                                             const QueueCalibratedTiming& graphics) noexcept {
    QueueCalibratedOverlapDiagnostics result{};
    result.schedule_status = schedule.status;
    result.schedule_ready = schedule.status == RhiAsyncOverlapReadinessStatus::ready_for_backend_private_timing;
    result.compute_timing_ready = compute.queue == QueueKind::compute &&
                                  compute.status == QueueCalibratedTimingStatus::ready && compute.qpc_frequency > 0 &&
                                  compute.calibrated_begin_cpu_timestamp > 0 &&
                                  compute.calibrated_end_cpu_timestamp >= compute.calibrated_begin_cpu_timestamp;
    result.graphics_timing_ready = graphics.queue == QueueKind::graphics &&
                                   graphics.status == QueueCalibratedTimingStatus::ready &&
                                   graphics.qpc_frequency > 0 && graphics.calibrated_begin_cpu_timestamp > 0 &&
                                   graphics.calibrated_end_cpu_timestamp >= graphics.calibrated_begin_cpu_timestamp;
    result.compute_begin_cpu_timestamp = compute.calibrated_begin_cpu_timestamp;
    result.compute_end_cpu_timestamp = compute.calibrated_end_cpu_timestamp;
    result.graphics_begin_cpu_timestamp = graphics.calibrated_begin_cpu_timestamp;
    result.graphics_end_cpu_timestamp = graphics.calibrated_end_cpu_timestamp;
    result.compute_elapsed_seconds = compute.elapsed_seconds;
    result.graphics_elapsed_seconds = graphics.elapsed_seconds;

    if (schedule.status == RhiAsyncOverlapReadinessStatus::not_requested) {
        result.status = QueueCalibratedOverlapStatus::not_requested;
        result.diagnostic = "d3d12 calibrated overlap diagnostic not requested";
        return result;
    }

    if (!result.schedule_ready) {
        result.status = QueueCalibratedOverlapStatus::missing_schedule_evidence;
        result.diagnostic = "d3d12 calibrated overlap diagnostic missing schedule evidence";
        return result;
    }

    if (!result.compute_timing_ready || !result.graphics_timing_ready ||
        compute.qpc_frequency != graphics.qpc_frequency) {
        result.status = QueueCalibratedOverlapStatus::missing_timing_evidence;
        result.diagnostic = "d3d12 calibrated overlap diagnostic missing calibrated timing evidence";
        return result;
    }

    result.intervals_overlap = result.compute_begin_cpu_timestamp < result.graphics_end_cpu_timestamp &&
                               result.graphics_begin_cpu_timestamp < result.compute_end_cpu_timestamp;
    result.status = result.intervals_overlap ? QueueCalibratedOverlapStatus::measured_overlapping
                                             : QueueCalibratedOverlapStatus::measured_non_overlapping;
    result.diagnostic = result.intervals_overlap
                            ? "d3d12 calibrated overlap diagnostic measured overlapping intervals"
                            : "d3d12 calibrated overlap diagnostic measured non-overlapping intervals";
    return result;
}

QueueCalibratedOverlapDiagnostics
diagnose_calibrated_compute_graphics_overlap(const RhiAsyncOverlapReadinessDiagnostics& schedule,
                                             const SubmittedCommandCalibratedTiming& compute,
                                             const SubmittedCommandCalibratedTiming& graphics) noexcept {
    return diagnose_calibrated_compute_graphics_overlap(schedule, make_queue_calibrated_timing(compute),
                                                        make_queue_calibrated_timing(graphics));
}

struct D3d12DescriptorSetRootTables {
    static constexpr std::uint32_t invalid_root_parameter{std::numeric_limits<std::uint32_t>::max()};
    std::uint32_t cbv_srv_uav{invalid_root_parameter};
    std::uint32_t sampler{invalid_root_parameter};
};

[[nodiscard]] static bool has_root_parameter(std::uint32_t root_parameter) noexcept {
    return root_parameter != D3d12DescriptorSetRootTables::invalid_root_parameter;
}

class D3d12RhiCommandList final : public IRhiCommandList {
  public:
    D3d12RhiCommandList(DeviceContext& context, NativeCommandListHandle native, QueueKind queue,
                        const std::vector<NativeSwapchainHandle>& swapchains,
                        const std::vector<SwapchainDesc>& swapchain_descs,
                        const std::vector<ResourceState>& swapchain_states,
                        const std::vector<bool>& swapchain_presentable, std::vector<bool>& swapchain_frame_reserved,
                        const std::vector<SwapchainHandle>& swapchain_frame_swapchains,
                        std::vector<bool>& swapchain_frame_active, std::vector<bool>& swapchain_frame_presented,
                        const std::vector<NativeResourceHandle>& buffers, const std::vector<BufferDesc>& buffer_descs,
                        const std::vector<bool>& buffer_active, const std::vector<NativeResourceHandle>& textures,
                        const std::vector<TextureDesc>& texture_descs, const std::vector<bool>& texture_active,
                        const std::vector<ResourceState>& texture_states,
                        const std::vector<NativeDescriptorHeapHandle>& descriptor_set_cbv_srv_uav_heaps,
                        const std::vector<std::uint32_t>& descriptor_set_cbv_srv_uav_base_descriptors,
                        const std::vector<NativeDescriptorHeapHandle>& descriptor_set_sampler_heaps,
                        const std::vector<std::uint32_t>& descriptor_set_sampler_base_descriptors,
                        const std::vector<DescriptorSetLayoutHandle>& descriptor_set_layouts,
                        const std::vector<NativeRootSignatureHandle>& pipeline_layouts,
                        const std::vector<std::vector<DescriptorSetLayoutHandle>>& pipeline_layout_descriptor_sets,
                        const std::vector<std::vector<D3d12DescriptorSetRootTables>>& pipeline_layout_descriptor_tables,
                        const std::vector<NativeGraphicsPipelineHandle>& graphics_pipelines,
                        const std::vector<NativeRootSignatureHandle>& graphics_pipeline_root_signatures,
                        const std::vector<PipelineLayoutHandle>& graphics_pipeline_layouts,
                        const std::vector<bool>& graphics_pipeline_active,
                        const std::vector<NativeComputePipelineHandle>& compute_pipelines,
                        const std::vector<NativeRootSignatureHandle>& compute_pipeline_root_signatures,
                        const std::vector<PipelineLayoutHandle>& compute_pipeline_layouts,
                        const std::vector<bool>& compute_pipeline_active,
                        const std::vector<bool>& pipeline_layout_active, const std::vector<bool>& descriptor_set_active,
                        RhiStats& stats) noexcept
        : context_(&context), native_(native), queue_(queue), swapchains_(&swapchains),
          swapchain_descs_(&swapchain_descs), swapchain_states_(swapchain_states),
          swapchain_presentable_(swapchain_presentable), swapchain_frame_reserved_(&swapchain_frame_reserved),
          swapchain_frame_swapchains_(&swapchain_frame_swapchains), swapchain_frame_active_(&swapchain_frame_active),
          swapchain_frame_presented_(&swapchain_frame_presented), buffers_(&buffers), buffer_descs_(&buffer_descs),
          buffer_active_(&buffer_active), textures_(&textures), texture_descs_(&texture_descs),
          texture_active_(&texture_active), texture_states_(texture_states), initial_texture_states_(texture_states),
          descriptor_set_cbv_srv_uav_heaps_(&descriptor_set_cbv_srv_uav_heaps),
          descriptor_set_cbv_srv_uav_base_descriptors_(&descriptor_set_cbv_srv_uav_base_descriptors),
          descriptor_set_sampler_heaps_(&descriptor_set_sampler_heaps),
          descriptor_set_sampler_base_descriptors_(&descriptor_set_sampler_base_descriptors),
          descriptor_set_layouts_(&descriptor_set_layouts), pipeline_layouts_(&pipeline_layouts),
          pipeline_layout_descriptor_sets_(&pipeline_layout_descriptor_sets),
          pipeline_layout_descriptor_tables_(&pipeline_layout_descriptor_tables),
          graphics_pipelines_(&graphics_pipelines),
          graphics_pipeline_root_signatures_(&graphics_pipeline_root_signatures),
          graphics_pipeline_layouts_(&graphics_pipeline_layouts), graphics_pipeline_active_(&graphics_pipeline_active),
          compute_pipelines_(&compute_pipelines), compute_pipeline_root_signatures_(&compute_pipeline_root_signatures),
          compute_pipeline_layouts_(&compute_pipeline_layouts), compute_pipeline_active_(&compute_pipeline_active),
          pipeline_layout_active_(&pipeline_layout_active), descriptor_set_active_(&descriptor_set_active),
          stats_(&stats) {}
    ~D3d12RhiCommandList() override {
        if (context_ != nullptr && !closed_) {
            context_->unwind_gpu_debug_events(native_);
        }
        release_swapchain_reservations();
    }

    [[nodiscard]] QueueKind queue_kind() const noexcept override {
        return queue_;
    }

    [[nodiscard]] bool closed() const noexcept override {
        return closed_;
    }

    void transition_texture(TextureHandle texture, ResourceState before, ResourceState after) override {
        require_open();
        if (render_pass_active_) {
            throw std::logic_error("d3d12 rhi texture transitions must be recorded outside a render pass");
        }
        if (before == ResourceState::undefined && after != ResourceState::undefined &&
            texture_state(texture) == after) {
            observe_texture(texture);
            if (!context_->activate_placed_texture(native_, native_texture(texture))) {
                throw std::logic_error("d3d12 rhi placed texture activation failed");
            }
            return;
        }
        if (!valid_resource_transition(before, after)) {
            throw std::invalid_argument("d3d12 rhi texture transition states must be valid and distinct");
        }
        if (texture_state(texture) != before) {
            throw std::invalid_argument("d3d12 rhi texture transition before state must match tracked state");
        }
        observe_texture(texture);
        if (!context_->activate_placed_texture(native_, native_texture(texture))) {
            throw std::logic_error("d3d12 rhi placed texture activation failed");
        }
        if (!context_->transition_texture(native_, native_texture(texture), before, after)) {
            throw std::logic_error("d3d12 rhi texture transition recording failed");
        }
        set_texture_state(texture, after);
        ++stats_->resource_transitions;
    }

    void texture_aliasing_barrier(TextureHandle before, TextureHandle after) override {
        require_open();
        if (render_pass_active_) {
            throw std::logic_error("d3d12 rhi texture aliasing barriers must be recorded outside a render pass");
        }
        if (before.value == 0 || after.value == 0 || before.value == after.value) {
            throw std::invalid_argument("d3d12 rhi texture aliasing barrier requires distinct texture handles");
        }

        const auto before_native = native_texture(before);
        const auto after_native = native_texture(after);
        observe_texture(before);
        observe_texture(after);
        if (!context_->texture_aliasing_barrier(native_, before_native, after_native)) {
            throw std::logic_error("d3d12 rhi texture aliasing barrier recording failed");
        }

        ++stats_->texture_aliasing_barriers;
    }

    void copy_buffer(BufferHandle source, BufferHandle destination, const BufferCopyRegion& region) override {
        require_open();
        if (render_pass_active_) {
            throw std::logic_error("d3d12 rhi buffer copies must be recorded outside a render pass");
        }
        const auto& source_desc = buffer_desc(source);
        const auto& destination_desc = buffer_desc(destination);
        if (!has_flag(source_desc.usage, BufferUsage::copy_source)) {
            throw std::invalid_argument("d3d12 rhi buffer copy source must use copy_source");
        }
        if (!has_flag(destination_desc.usage, BufferUsage::copy_destination)) {
            throw std::invalid_argument("d3d12 rhi buffer copy destination must use copy_destination");
        }
        validate_buffer_copy_region(source_desc, destination_desc, region);
        if (!context_->copy_buffer(native_, native_buffer(source), native_buffer(destination), region)) {
            throw std::logic_error("d3d12 rhi buffer copy recording failed");
        }
        ++stats_->buffer_copies;
        stats_->bytes_copied += region.size_bytes;
    }

    void copy_buffer_to_texture(BufferHandle source, TextureHandle destination,
                                const BufferTextureCopyRegion& region) override {
        require_open();
        if (render_pass_active_) {
            throw std::logic_error("d3d12 rhi buffer texture copies must be recorded outside a render pass");
        }
        const auto& source_desc = buffer_desc(source);
        const auto& destination_desc = texture_desc(destination);
        if (!has_flag(source_desc.usage, BufferUsage::copy_source)) {
            throw std::invalid_argument("d3d12 rhi buffer texture copy source must use copy_source");
        }
        if (!has_flag(destination_desc.usage, TextureUsage::copy_destination)) {
            throw std::invalid_argument("d3d12 rhi buffer texture copy destination must use copy_destination");
        }
        if (texture_state(destination) != ResourceState::copy_destination) {
            throw std::invalid_argument("d3d12 rhi buffer texture copy destination must be in copy_destination state");
        }
        observe_texture(destination);
        validate_buffer_texture_region(destination_desc, region);
        const auto footprint = d3d12_linear_texture_footprint(destination_desc.format, region);
        if (footprint.required_bytes > source_desc.size_bytes) {
            throw std::invalid_argument("d3d12 rhi buffer texture copy source range is outside the source buffer");
        }
        if (!context_->copy_buffer_to_texture(native_, native_buffer(source), native_texture(destination), region)) {
            throw std::logic_error("d3d12 rhi buffer texture copy recording failed");
        }
        ++stats_->buffer_texture_copies;
    }

    void copy_texture_to_buffer(TextureHandle source, BufferHandle destination,
                                const BufferTextureCopyRegion& region) override {
        require_open();
        if (render_pass_active_) {
            throw std::logic_error("d3d12 rhi texture buffer copies must be recorded outside a render pass");
        }
        const auto& source_desc = texture_desc(source);
        const auto& destination_desc = buffer_desc(destination);
        if (!has_flag(source_desc.usage, TextureUsage::copy_source)) {
            throw std::invalid_argument("d3d12 rhi texture buffer copy source must use copy_source");
        }
        if (!has_flag(destination_desc.usage, BufferUsage::copy_destination)) {
            throw std::invalid_argument("d3d12 rhi texture buffer copy destination must use copy_destination");
        }
        if (texture_state(source) != ResourceState::copy_source) {
            throw std::invalid_argument("d3d12 rhi texture buffer copy source must be in copy_source state");
        }
        observe_texture(source);
        validate_buffer_texture_region(source_desc, region);
        const auto footprint = d3d12_linear_texture_footprint(source_desc.format, region);
        if (footprint.required_bytes > destination_desc.size_bytes) {
            throw std::invalid_argument(
                "d3d12 rhi texture buffer copy destination range is outside the destination buffer");
        }
        if (!context_->copy_texture_to_buffer(native_, native_texture(source), native_buffer(destination), region)) {
            throw std::logic_error("d3d12 rhi texture buffer copy recording failed");
        }
        ++stats_->texture_buffer_copies;
    }

    void present(SwapchainFrameHandle frame) override {
        require_open();
        if (queue_ != QueueKind::graphics) {
            throw std::logic_error("d3d12 rhi swapchain presents require a graphics command list");
        }
        if (render_pass_active_) {
            throw std::logic_error("d3d12 rhi swapchain presents must be recorded outside a render pass");
        }
        if (!owns_swapchain_frame(frame)) {
            throw std::invalid_argument("d3d12 rhi swapchain frame handle is unknown");
        }
        const auto swapchain = swapchain_for_frame(frame);
        const auto native = native_swapchain(swapchain);
        if (swapchain_state(swapchain) != ResourceState::present) {
            throw std::invalid_argument("d3d12 rhi swapchain must be in present state before present command");
        }
        if (!swapchain_presentable(swapchain)) {
            throw std::invalid_argument("d3d12 rhi swapchain must complete a render pass before present command");
        }
        if (!owns_swapchain_frame_reservation(frame)) {
            throw std::invalid_argument("d3d12 rhi swapchain present must match a frame recorded by this command list");
        }
        if (swapchain_frame_presented(frame)) {
            throw std::invalid_argument("d3d12 rhi swapchain frame is already presented");
        }
        set_swapchain_presentable(swapchain, false);
        set_swapchain_frame_presented(frame, true);
        pending_presents_.push_back(native);
        pending_present_frames_.push_back(frame);
    }

    void begin_render_pass(const RenderPassDesc& desc) override {
        require_open();
        if (queue_ != QueueKind::graphics) {
            throw std::logic_error("d3d12 rhi render passes require a graphics command list");
        }
        if (render_pass_active_) {
            throw std::logic_error("d3d12 rhi render pass is already active");
        }
        const bool uses_swapchain = desc.color.swapchain_frame.value != 0;
        const bool uses_texture = desc.color.texture.value != 0;
        if (uses_swapchain == uses_texture) {
            throw std::invalid_argument("d3d12 rhi render pass requires exactly one color attachment");
        }
        active_color_format_ = Format::unknown;
        active_depth_format_ = Format::unknown;
        Format pass_color_format = Format::unknown;
        Extent2D pass_extent{};
        SwapchainHandle pass_swapchain_handle{};
        NativeSwapchainHandle pass_swapchain{};
        NativeResourceHandle pass_color_texture{};

        if (uses_swapchain) {
            if (!owns_swapchain_frame(desc.color.swapchain_frame)) {
                throw std::invalid_argument("d3d12 rhi swapchain frame handle is unknown");
            }
            pass_swapchain_handle = swapchain_for_frame(desc.color.swapchain_frame);
            pass_swapchain = native_swapchain(pass_swapchain_handle);
            const auto& swapchain_description = swapchain_desc(pass_swapchain_handle);
            pass_color_format = swapchain_description.format;
            pass_extent = swapchain_description.extent;
            if (swapchain_state(pass_swapchain_handle) != ResourceState::present) {
                throw std::invalid_argument("d3d12 rhi swapchain render pass attachment must be in present state");
            }
            if (swapchain_presentable(pass_swapchain_handle) || swapchain_frame_presented(desc.color.swapchain_frame) ||
                !swapchain_frame_reserved(pass_swapchain_handle)) {
                throw std::invalid_argument("d3d12 rhi swapchain already has a pending frame");
            }
        } else {
            const auto& target_desc = texture_desc(desc.color.texture);
            pass_color_format = target_desc.format;
            pass_extent = Extent2D{.width = target_desc.extent.width, .height = target_desc.extent.height};
            if (!has_flag(target_desc.usage, TextureUsage::render_target)) {
                throw std::invalid_argument("d3d12 rhi texture render pass attachment must use render_target");
            }
            if (texture_state(desc.color.texture) != ResourceState::render_target) {
                throw std::invalid_argument("d3d12 rhi texture render pass attachment must be in render_target state");
            }
            pass_color_texture = native_texture(desc.color.texture);
        }

        NativeResourceHandle pass_depth_texture{};
        Format pass_depth_format = Format::unknown;
        if (desc.depth.texture.value != 0) {
            const auto& depth_desc = texture_desc(desc.depth.texture);
            if (!has_flag(depth_desc.usage, TextureUsage::depth_stencil)) {
                throw std::invalid_argument("d3d12 rhi render pass depth attachment must use depth_stencil");
            }
            if (!is_d3d12_depth_stencil_format(depth_desc.format)) {
                throw std::invalid_argument("d3d12 rhi render pass depth attachment must use a depth format");
            }
            if (depth_desc.extent.width != pass_extent.width || depth_desc.extent.height != pass_extent.height ||
                depth_desc.extent.depth != 1) {
                throw std::invalid_argument(
                    "d3d12 rhi render pass depth attachment extent must match the color target");
            }
            if (texture_state(desc.depth.texture) != ResourceState::depth_write) {
                throw std::invalid_argument("d3d12 rhi render pass depth attachment must be in depth_write state");
            }
            if (desc.depth.load_action == LoadAction::clear &&
                (!std::isfinite(desc.depth.clear_depth.depth) || desc.depth.clear_depth.depth < 0.0F ||
                 desc.depth.clear_depth.depth > 1.0F)) {
                throw std::invalid_argument("d3d12 rhi render pass depth clear value must be finite and in [0, 1]");
            }
            pass_depth_texture = native_texture(desc.depth.texture);
            pass_depth_format = depth_desc.format;
        }

        if (uses_swapchain) {
            if (!context_->transition_swapchain_back_buffer(native_, pass_swapchain, ResourceState::present,
                                                            ResourceState::render_target) ||
                !context_->set_swapchain_render_target(native_, pass_swapchain, pass_depth_texture)) {
                throw std::logic_error("d3d12 rhi swapchain render pass begin failed");
            }
            if (desc.color.load_action == LoadAction::clear &&
                !context_->clear_swapchain_back_buffer(native_, pass_swapchain, desc.color.clear_color.red,
                                                       desc.color.clear_color.green, desc.color.clear_color.blue,
                                                       desc.color.clear_color.alpha)) {
                throw std::logic_error("d3d12 rhi swapchain render pass clear failed");
            }
            if (desc.depth.texture.value != 0 && desc.depth.load_action == LoadAction::clear &&
                !context_->clear_texture_depth_stencil(native_, pass_depth_texture, desc.depth.clear_depth.depth)) {
                throw std::logic_error("d3d12 rhi swapchain depth attachment clear failed");
            }
            active_swapchain_ = pass_swapchain;
            active_swapchain_handle_ = pass_swapchain_handle;
            active_swapchain_frame_ = desc.color.swapchain_frame;
            track_swapchain_frame(desc.color.swapchain_frame);
            set_swapchain_state(pass_swapchain_handle, ResourceState::render_target);
            set_swapchain_presentable(pass_swapchain_handle, false);
            ++stats_->resource_transitions;
        } else {
            observe_texture(desc.color.texture);
            if (!context_->activate_placed_texture(native_, pass_color_texture)) {
                throw std::logic_error("d3d12 rhi texture render pass placed color activation failed");
            }
            if (desc.depth.texture.value != 0 && !context_->activate_placed_texture(native_, pass_depth_texture)) {
                throw std::logic_error("d3d12 rhi texture render pass placed depth activation failed");
            }
            if (!context_->set_texture_render_target(native_, pass_color_texture, pass_depth_texture)) {
                throw std::logic_error("d3d12 rhi texture render pass begin failed");
            }
            if (desc.color.load_action == LoadAction::clear &&
                !context_->clear_texture_render_target(native_, pass_color_texture, desc.color.clear_color.red,
                                                       desc.color.clear_color.green, desc.color.clear_color.blue,
                                                       desc.color.clear_color.alpha)) {
                throw std::logic_error("d3d12 rhi texture render pass clear failed");
            }
            if (desc.depth.texture.value != 0 && desc.depth.load_action == LoadAction::clear &&
                !context_->clear_texture_depth_stencil(native_, pass_depth_texture, desc.depth.clear_depth.depth)) {
                throw std::logic_error("d3d12 rhi texture depth attachment clear failed");
            }
            active_texture_ = pass_color_texture;
        }

        if (desc.depth.texture.value != 0) {
            observe_texture(desc.depth.texture);
        }

        active_color_format_ = pass_color_format;
        active_depth_format_ = pass_depth_format;
        render_pass_active_ = true;
        ++stats_->render_passes_begun;
    }

    void end_render_pass() override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("d3d12 rhi render pass is not active");
        }
        if (active_swapchain_.value != 0) {
            if (!context_->transition_swapchain_back_buffer(native_, active_swapchain_, ResourceState::render_target,
                                                            ResourceState::present)) {
                throw std::logic_error("d3d12 rhi swapchain render pass end failed");
            }
            set_swapchain_state(active_swapchain_handle_, ResourceState::present);
            set_swapchain_presentable(active_swapchain_handle_, true);
            ++stats_->resource_transitions;
        }

        active_swapchain_ = NativeSwapchainHandle{};
        active_swapchain_handle_ = SwapchainHandle{};
        active_swapchain_frame_ = SwapchainFrameHandle{};
        active_texture_ = NativeResourceHandle{};
        active_color_format_ = Format::unknown;
        active_depth_format_ = Format::unknown;
        render_pass_active_ = false;
        active_root_signature_ = NativeRootSignatureHandle{};
        bound_pipeline_layout_ = PipelineLayoutHandle{};
        graphics_pipeline_bound_ = false;
        vertex_buffer_bound_ = false;
        index_buffer_bound_ = false;
    }

    void bind_graphics_pipeline(GraphicsPipelineHandle pipeline) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("d3d12 rhi graphics pipelines must be bound inside a render pass");
        }
        const auto native_pipeline = native_graphics_pipeline(pipeline);
        const auto root_signature = native_root_signature_for_pipeline(pipeline);
        const auto pipeline_info = context_->graphics_pipeline_info(native_pipeline);
        if (!pipeline_info.valid || pipeline_info.color_format != active_color_format_ ||
            pipeline_info.depth_format != active_depth_format_) {
            throw std::invalid_argument("d3d12 rhi graphics pipeline format must match the active render pass");
        }
        if (!context_->set_graphics_root_signature(native_, root_signature) ||
            !context_->bind_graphics_pipeline(native_, native_pipeline)) {
            throw std::logic_error("d3d12 rhi graphics pipeline bind failed");
        }
        active_root_signature_ = root_signature;
        bound_pipeline_layout_ = public_pipeline_layout_for_pipeline(pipeline);
        graphics_pipeline_bound_ = true;
        vertex_buffer_bound_ = false;
        index_buffer_bound_ = false;
        ++stats_->graphics_pipelines_bound;
    }

    void bind_compute_pipeline(ComputePipelineHandle pipeline) override {
        require_open();
        if (render_pass_active_) {
            throw std::logic_error("d3d12 rhi compute pipelines must be bound outside a render pass");
        }
        if (queue_ != QueueKind::compute) {
            throw std::logic_error("d3d12 rhi compute pipeline binding requires a compute command list");
        }
        const auto native_pipeline = native_compute_pipeline(pipeline);
        const auto root_signature = native_root_signature_for_compute_pipeline(pipeline);
        const auto pipeline_info = context_->compute_pipeline_info(native_pipeline);
        if (!pipeline_info.valid || pipeline_info.root_signature.value != root_signature.value) {
            throw std::invalid_argument("d3d12 rhi compute pipeline is invalid");
        }
        if (!context_->set_compute_root_signature(native_, root_signature) ||
            !context_->bind_compute_pipeline(native_, native_pipeline)) {
            throw std::logic_error("d3d12 rhi compute pipeline bind failed");
        }
        active_compute_root_signature_ = root_signature;
        bound_compute_pipeline_layout_ = public_pipeline_layout_for_compute_pipeline(pipeline);
        compute_pipeline_bound_ = true;
        ++stats_->compute_pipelines_bound;
    }

    void bind_descriptor_set(PipelineLayoutHandle layout, std::uint32_t set_index, DescriptorSetHandle set) override {
        require_open();
        if (!render_pass_active_ && !compute_pipeline_bound_) {
            throw std::logic_error("d3d12 rhi descriptor set binding requires a graphics or compute pipeline");
        }
        if (render_pass_active_ && !graphics_pipeline_bound_) {
            throw std::logic_error("d3d12 rhi descriptor set binding requires a graphics pipeline");
        }

        const auto root_signature = native_pipeline_layout(layout);
        const auto bound_layout = render_pass_active_ ? bound_pipeline_layout_ : bound_compute_pipeline_layout_;
        const auto active_signature = render_pass_active_ ? active_root_signature_ : active_compute_root_signature_;
        if (bound_layout.value != layout.value || active_signature.value != root_signature.value) {
            throw std::invalid_argument("d3d12 rhi descriptor set pipeline layout must match the bound pipeline");
        }

        const auto expected_layout = descriptor_set_layout_for_pipeline_layout(layout, set_index);
        const auto actual_layout = descriptor_set_layout_for_set(set);
        if (expected_layout.value != actual_layout.value) {
            throw std::invalid_argument("d3d12 rhi descriptor set layout is not compatible with the pipeline layout");
        }

        const auto tables = descriptor_tables_for_pipeline_layout(layout, set_index);
        const auto cbv_srv_uav_heap = has_root_parameter(tables.cbv_srv_uav)
                                          ? native_cbv_srv_uav_descriptor_heap_for_set(set)
                                          : NativeDescriptorHeapHandle{};
        const auto sampler_heap = has_root_parameter(tables.sampler) ? native_sampler_descriptor_heap_for_set(set)
                                                                     : NativeDescriptorHeapHandle{};
        if (!context_->set_descriptor_heaps(
                native_, NativeDescriptorHeapBinding{.cbv_srv_uav = cbv_srv_uav_heap, .sampler = sampler_heap})) {
            throw std::logic_error("d3d12 rhi descriptor set bind failed");
        }
        if (has_root_parameter(tables.cbv_srv_uav) &&
            !(render_pass_active_
                  ? context_->set_graphics_descriptor_table(native_, root_signature, tables.cbv_srv_uav,
                                                            cbv_srv_uav_heap, cbv_srv_uav_descriptor_base_for_set(set))
                  : context_->set_compute_descriptor_table(native_, root_signature, tables.cbv_srv_uav,
                                                           cbv_srv_uav_heap,
                                                           cbv_srv_uav_descriptor_base_for_set(set)))) {
            throw std::logic_error("d3d12 rhi cbv srv uav descriptor set bind failed");
        }
        if (has_root_parameter(tables.sampler) &&
            !(render_pass_active_
                  ? context_->set_graphics_descriptor_table(native_, root_signature, tables.sampler, sampler_heap,
                                                            sampler_descriptor_base_for_set(set))
                  : context_->set_compute_descriptor_table(native_, root_signature, tables.sampler, sampler_heap,
                                                           sampler_descriptor_base_for_set(set)))) {
            throw std::logic_error("d3d12 rhi sampler descriptor set bind failed");
        }
        ++stats_->descriptor_sets_bound;
    }

    void bind_vertex_buffer(const VertexBufferBinding& binding) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("d3d12 rhi vertex buffers must be bound inside a render pass");
        }
        if (!graphics_pipeline_bound_) {
            throw std::logic_error("d3d12 rhi vertex buffer binding requires a graphics pipeline");
        }
        if (binding.stride == 0) {
            throw std::invalid_argument("d3d12 rhi vertex buffer stride must be non-zero");
        }
        const auto& desc = buffer_desc(binding.buffer);
        if (!has_flag(desc.usage, BufferUsage::vertex)) {
            throw std::invalid_argument("d3d12 rhi vertex buffer binding requires vertex usage");
        }
        if (binding.offset >= desc.size_bytes) {
            throw std::invalid_argument("d3d12 rhi vertex buffer offset is outside the buffer");
        }
        if (!context_->bind_vertex_buffer(native_, native_buffer(binding.buffer), binding.offset, binding.stride,
                                          binding.binding)) {
            throw std::logic_error("d3d12 rhi vertex buffer bind failed");
        }
        vertex_buffer_bound_ = true;
        ++stats_->vertex_buffer_bindings;
    }

    void bind_index_buffer(const IndexBufferBinding& binding) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("d3d12 rhi index buffers must be bound inside a render pass");
        }
        if (!graphics_pipeline_bound_) {
            throw std::logic_error("d3d12 rhi index buffer binding requires a graphics pipeline");
        }
        if (binding.format == IndexFormat::unknown) {
            throw std::invalid_argument("d3d12 rhi index buffer format must be known");
        }
        const auto& desc = buffer_desc(binding.buffer);
        if (!has_flag(desc.usage, BufferUsage::index)) {
            throw std::invalid_argument("d3d12 rhi index buffer binding requires index usage");
        }
        if (binding.offset >= desc.size_bytes) {
            throw std::invalid_argument("d3d12 rhi index buffer offset is outside the buffer");
        }
        if (!context_->bind_index_buffer(native_, native_buffer(binding.buffer), binding.offset, binding.format)) {
            throw std::logic_error("d3d12 rhi index buffer bind failed");
        }
        index_buffer_bound_ = true;
        ++stats_->index_buffer_bindings;
    }

    void draw(std::uint32_t vertex_count, std::uint32_t instance_count) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("d3d12 rhi draw requires an active render pass");
        }
        if (!context_->draw(native_, vertex_count, instance_count)) {
            throw std::logic_error("d3d12 rhi draw failed");
        }
        ++stats_->draw_calls;
        stats_->vertices_submitted += static_cast<std::uint64_t>(vertex_count) * instance_count;
    }

    void draw_indexed(std::uint32_t index_count, std::uint32_t instance_count) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("d3d12 rhi indexed draw requires an active render pass");
        }
        if (!vertex_buffer_bound_) {
            throw std::logic_error("d3d12 rhi indexed draw requires a vertex buffer");
        }
        if (!index_buffer_bound_) {
            throw std::logic_error("d3d12 rhi indexed draw requires an index buffer");
        }
        if (!context_->draw_indexed(native_, index_count, instance_count)) {
            throw std::logic_error("d3d12 rhi indexed draw failed");
        }
        ++stats_->draw_calls;
        ++stats_->indexed_draw_calls;
        stats_->indices_submitted += static_cast<std::uint64_t>(index_count) * instance_count;
    }

    void dispatch(std::uint32_t group_count_x, std::uint32_t group_count_y, std::uint32_t group_count_z) override {
        require_open();
        if (render_pass_active_) {
            throw std::logic_error("d3d12 rhi dispatch must be recorded outside a render pass");
        }
        if (queue_ != QueueKind::compute) {
            throw std::logic_error("d3d12 rhi dispatch requires a compute command list");
        }
        if (!compute_pipeline_bound_) {
            throw std::logic_error("d3d12 rhi dispatch requires a compute pipeline");
        }
        if (group_count_x == 0 || group_count_y == 0 || group_count_z == 0) {
            throw std::invalid_argument("d3d12 rhi dispatch workgroup counts must be non-zero");
        }
        if (!context_->dispatch(native_, group_count_x, group_count_y, group_count_z)) {
            throw std::logic_error("d3d12 rhi dispatch failed");
        }
        ++stats_->compute_dispatches;
        stats_->compute_workgroups_x += group_count_x;
        stats_->compute_workgroups_y += group_count_y;
        stats_->compute_workgroups_z += group_count_z;
    }

    void set_viewport(const ViewportDesc& viewport) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("d3d12 rhi set_viewport requires an active render pass");
        }
        if (!context_->set_viewport(native_, viewport)) {
            throw std::logic_error("d3d12 rhi set_viewport failed");
        }
    }

    void set_scissor(const ScissorRectDesc& scissor) override {
        require_open();
        if (!render_pass_active_) {
            throw std::logic_error("d3d12 rhi set_scissor requires an active render pass");
        }
        if (!context_->set_scissor(native_, scissor)) {
            throw std::logic_error("d3d12 rhi set_scissor failed");
        }
    }

    void begin_gpu_debug_scope(std::string_view name) override {
        require_open();
        validate_rhi_debug_label(name);
        if (!context_->begin_gpu_debug_event(native_, name)) {
            throw std::logic_error("d3d12 rhi gpu debug scope begin failed");
        }
        ++stats_->gpu_debug_scopes_begun;
    }

    void end_gpu_debug_scope() override {
        require_open();
        if (!context_->end_gpu_debug_event(native_)) {
            throw std::logic_error("d3d12 rhi gpu debug scope end without matching begin");
        }
        ++stats_->gpu_debug_scopes_ended;
    }

    void insert_gpu_debug_marker(std::string_view name) override {
        require_open();
        validate_rhi_debug_label(name);
        if (!context_->insert_gpu_debug_marker(native_, name)) {
            throw std::logic_error("d3d12 rhi gpu debug marker insert failed");
        }
        ++stats_->gpu_debug_markers_inserted;
    }

    void close() override {
        if (closed_) {
            throw std::logic_error("d3d12 rhi command list is already closed");
        }
        if (render_pass_active_) {
            throw std::logic_error("d3d12 rhi command list cannot close with an active render pass");
        }
        if (has_unpresented_swapchain_frame()) {
            throw std::logic_error("d3d12 rhi command list cannot close with an unpresented swapchain frame");
        }
        if (context_->gpu_debug_scope_depth(native_) != 0) {
            throw std::logic_error("d3d12 rhi command list cannot close with unbalanced gpu debug scopes");
        }
        if (context_ == nullptr || !context_->close_command_list(native_)) {
            throw std::logic_error("d3d12 rhi command list close failed");
        }
        closed_ = true;
    }

    [[nodiscard]] NativeCommandListHandle native_handle() const noexcept {
        return native_;
    }

    [[nodiscard]] const std::vector<NativeSwapchainHandle>& pending_presents() const noexcept {
        return pending_presents_;
    }

    void release_submitted_swapchain_frames() {
        release_swapchain_reservations();
    }

    void commit_texture_states(std::vector<ResourceState>& destination) const {
        const auto count = std::min(destination.size(), texture_states_.size());
        for (std::size_t index = 0; index < count; ++index) {
            destination.at(index) = texture_states_.at(index);
        }
    }

    [[nodiscard]] bool observed_texture_states_match(const std::vector<ResourceState>& current) const {
        return std::ranges::all_of(observed_textures_, [&](const auto texture) {
            if (texture.value == 0 || texture.value > current.size() ||
                texture.value > initial_texture_states_.size()) {
                return false;
            }
            const auto index = texture.value - 1U;
            return current.at(index) == initial_texture_states_.at(index);
        });
    }

    void commit_swapchain_states(std::vector<ResourceState>& states, std::vector<bool>& presentable) const {
        const auto state_count = std::min(states.size(), swapchain_states_.size());
        for (std::size_t index = 0; index < state_count; ++index) {
            states.at(index) = swapchain_states_.at(index);
        }

        const auto presentable_count = std::min(presentable.size(), swapchain_presentable_.size());
        for (std::size_t index = 0; index < presentable_count; ++index) {
            presentable.at(index) = swapchain_presentable_.at(index);
        }
    }

  private:
    void require_open() const {
        if (closed_) {
            throw std::logic_error("d3d12 rhi command list is closed");
        }
        if (context_ == nullptr || stats_ == nullptr) {
            throw std::logic_error("d3d12 rhi command list is not bound to a device");
        }
    }

    [[nodiscard]] NativeSwapchainHandle native_swapchain(SwapchainHandle handle) const {
        if (swapchains_ == nullptr || handle.value == 0 || handle.value > swapchains_->size()) {
            throw std::invalid_argument("d3d12 rhi swapchain handle is unknown");
        }
        return swapchains_->at(handle.value - 1U);
    }

    [[nodiscard]] const SwapchainDesc& swapchain_desc(SwapchainHandle handle) const {
        (void)native_swapchain(handle);
        if (swapchain_descs_ == nullptr || handle.value > swapchain_descs_->size()) {
            throw std::invalid_argument("d3d12 rhi swapchain description is unknown");
        }
        return swapchain_descs_->at(handle.value - 1U);
    }

    [[nodiscard]] ResourceState swapchain_state(SwapchainHandle handle) const {
        (void)native_swapchain(handle);
        if (handle.value > swapchain_states_.size()) {
            throw std::invalid_argument("d3d12 rhi swapchain state is unknown");
        }
        return swapchain_states_.at(handle.value - 1U);
    }

    void set_swapchain_state(SwapchainHandle handle, ResourceState state) {
        (void)native_swapchain(handle);
        if (handle.value > swapchain_states_.size()) {
            throw std::invalid_argument("d3d12 rhi swapchain state is unknown");
        }
        swapchain_states_.at(handle.value - 1U) = state;
    }

    [[nodiscard]] bool swapchain_presentable(SwapchainHandle handle) const {
        (void)native_swapchain(handle);
        if (handle.value > swapchain_presentable_.size()) {
            throw std::invalid_argument("d3d12 rhi swapchain present state is unknown");
        }
        return swapchain_presentable_.at(handle.value - 1U);
    }

    void set_swapchain_presentable(SwapchainHandle handle, bool presentable) {
        (void)native_swapchain(handle);
        if (handle.value > swapchain_presentable_.size()) {
            throw std::invalid_argument("d3d12 rhi swapchain present state is unknown");
        }
        swapchain_presentable_.at(handle.value - 1U) = presentable;
    }

    [[nodiscard]] bool swapchain_frame_reserved(SwapchainHandle handle) const {
        (void)native_swapchain(handle);
        if (swapchain_frame_reserved_ == nullptr || handle.value > swapchain_frame_reserved_->size()) {
            throw std::invalid_argument("d3d12 rhi swapchain frame reservation is unknown");
        }
        return swapchain_frame_reserved_->at(handle.value - 1U);
    }

    [[nodiscard]] bool owns_swapchain_frame(SwapchainFrameHandle frame) const noexcept {
        return frame.value > 0 && swapchain_frame_swapchains_ != nullptr && swapchain_frame_active_ != nullptr &&
               frame.value <= swapchain_frame_swapchains_->size() && frame.value <= swapchain_frame_active_->size() &&
               swapchain_frame_active_->at(frame.value - 1U);
    }

    [[nodiscard]] SwapchainHandle swapchain_for_frame(SwapchainFrameHandle frame) const {
        if (!owns_swapchain_frame(frame)) {
            throw std::invalid_argument("d3d12 rhi swapchain frame handle is unknown");
        }
        return swapchain_frame_swapchains_->at(frame.value - 1U);
    }

    [[nodiscard]] bool swapchain_frame_presented(SwapchainFrameHandle frame) const {
        if (!owns_swapchain_frame(frame) || swapchain_frame_presented_ == nullptr ||
            frame.value > swapchain_frame_presented_->size()) {
            throw std::invalid_argument("d3d12 rhi swapchain frame handle is unknown");
        }
        return swapchain_frame_presented_->at(frame.value - 1U);
    }

    void set_swapchain_frame_presented(SwapchainFrameHandle frame, bool presented) {
        if (!owns_swapchain_frame(frame) || swapchain_frame_presented_ == nullptr ||
            frame.value > swapchain_frame_presented_->size()) {
            throw std::invalid_argument("d3d12 rhi swapchain frame handle is unknown");
        }
        swapchain_frame_presented_->at(frame.value - 1U) = presented;
    }

    void track_swapchain_frame(SwapchainFrameHandle frame) {
        (void)swapchain_for_frame(frame);
        if (owns_swapchain_frame_reservation(frame)) {
            throw std::invalid_argument("d3d12 rhi swapchain frame is already tracked by this command list");
        }
        reserved_swapchain_frames_.push_back(frame);
    }

    [[nodiscard]] bool owns_swapchain_frame_reservation(SwapchainFrameHandle frame) const {
        return std::ranges::any_of(reserved_swapchain_frames_,
                                   [frame](SwapchainFrameHandle reserved) { return reserved.value == frame.value; });
    }

    [[nodiscard]] bool has_unpresented_swapchain_frame() const {
        return std::ranges::any_of(reserved_swapchain_frames_, [this](SwapchainFrameHandle reserved) {
            return owns_swapchain_frame(reserved) && !swapchain_frame_presented(reserved);
        });
    }

    void release_swapchain_reservations() noexcept {
        if (swapchain_frame_reserved_ == nullptr || swapchain_frame_swapchains_ == nullptr ||
            swapchain_frame_active_ == nullptr || swapchain_frame_presented_ == nullptr) {
            pending_present_frames_.clear();
            reserved_swapchain_frames_.clear();
            return;
        }
        for (const auto reserved : reserved_swapchain_frames_) {
            if (reserved.value == 0 || reserved.value > swapchain_frame_swapchains_->size() ||
                reserved.value > swapchain_frame_active_->size() ||
                reserved.value > swapchain_frame_presented_->size()) {
                continue;
            }
            const auto frame_index = reserved.value - 1U;
            const bool was_active = swapchain_frame_active_->at(frame_index);
            const auto swapchain = swapchain_frame_swapchains_->at(frame_index);
            if (swapchain.value > 0 && swapchain.value <= swapchain_frame_reserved_->size()) {
                (*swapchain_frame_reserved_)[swapchain.value - 1U] = false;
            }
            (*swapchain_frame_active_)[frame_index] = false;
            (*swapchain_frame_presented_)[frame_index] = false;
            if (was_active && stats_ != nullptr) {
                ++stats_->swapchain_frames_released;
            }
        }
        pending_present_frames_.clear();
        reserved_swapchain_frames_.clear();
    }

    [[nodiscard]] NativeResourceHandle native_buffer(BufferHandle handle) const {
        if (buffers_ == nullptr || buffer_active_ == nullptr || handle.value == 0 || handle.value > buffers_->size() ||
            handle.value > buffer_active_->size() || !buffer_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi buffer handle is unknown");
        }
        return buffers_->at(handle.value - 1U);
    }

    [[nodiscard]] const BufferDesc& buffer_desc(BufferHandle handle) const {
        if (buffer_descs_ == nullptr || buffer_active_ == nullptr || handle.value == 0 ||
            handle.value > buffer_descs_->size() || handle.value > buffer_active_->size() ||
            !buffer_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi buffer handle is unknown");
        }
        return buffer_descs_->at(handle.value - 1U);
    }

    [[nodiscard]] NativeResourceHandle native_texture(TextureHandle handle) const {
        if (textures_ == nullptr || texture_active_ == nullptr || handle.value == 0 ||
            handle.value > textures_->size() || handle.value > texture_active_->size() ||
            !texture_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi texture handle is unknown");
        }
        return textures_->at(handle.value - 1U);
    }

    [[nodiscard]] const TextureDesc& texture_desc(TextureHandle handle) const {
        if (texture_descs_ == nullptr || texture_active_ == nullptr || handle.value == 0 ||
            handle.value > texture_descs_->size() || handle.value > texture_active_->size() ||
            !texture_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi texture handle is unknown");
        }
        return texture_descs_->at(handle.value - 1U);
    }

    [[nodiscard]] ResourceState texture_state(TextureHandle handle) const {
        (void)texture_desc(handle);
        if (handle.value > texture_states_.size()) {
            throw std::invalid_argument("d3d12 rhi texture state is unknown");
        }
        return texture_states_.at(handle.value - 1U);
    }

    void set_texture_state(TextureHandle handle, ResourceState state) {
        (void)texture_desc(handle);
        if (handle.value > texture_states_.size()) {
            throw std::invalid_argument("d3d12 rhi texture state is unknown");
        }
        texture_states_.at(handle.value - 1U) = state;
    }

    void observe_texture(TextureHandle handle) {
        (void)texture_desc(handle);
        const auto found = std::ranges::find_if(
            observed_textures_, [handle](TextureHandle observed) { return observed.value == handle.value; });
        if (found == observed_textures_.end()) {
            observed_textures_.push_back(handle);
        }
    }

    [[nodiscard]] NativeDescriptorHeapHandle
    native_cbv_srv_uav_descriptor_heap_for_set(DescriptorSetHandle handle) const {
        if (descriptor_set_cbv_srv_uav_heaps_ == nullptr || descriptor_set_active_ == nullptr || handle.value == 0 ||
            handle.value > descriptor_set_cbv_srv_uav_heaps_->size() || handle.value > descriptor_set_active_->size() ||
            !descriptor_set_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi descriptor set handle is unknown");
        }
        return descriptor_set_cbv_srv_uav_heaps_->at(handle.value - 1U);
    }

    [[nodiscard]] std::uint32_t cbv_srv_uav_descriptor_base_for_set(DescriptorSetHandle handle) const {
        if (descriptor_set_cbv_srv_uav_base_descriptors_ == nullptr || descriptor_set_active_ == nullptr ||
            handle.value == 0 || handle.value > descriptor_set_cbv_srv_uav_base_descriptors_->size() ||
            handle.value > descriptor_set_active_->size() || !descriptor_set_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi descriptor set handle is unknown");
        }
        return descriptor_set_cbv_srv_uav_base_descriptors_->at(handle.value - 1U);
    }

    [[nodiscard]] NativeDescriptorHeapHandle native_sampler_descriptor_heap_for_set(DescriptorSetHandle handle) const {
        if (descriptor_set_sampler_heaps_ == nullptr || descriptor_set_active_ == nullptr || handle.value == 0 ||
            handle.value > descriptor_set_sampler_heaps_->size() || handle.value > descriptor_set_active_->size() ||
            !descriptor_set_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi descriptor set handle is unknown");
        }
        return descriptor_set_sampler_heaps_->at(handle.value - 1U);
    }

    [[nodiscard]] std::uint32_t sampler_descriptor_base_for_set(DescriptorSetHandle handle) const {
        if (descriptor_set_sampler_base_descriptors_ == nullptr || descriptor_set_active_ == nullptr ||
            handle.value == 0 || handle.value > descriptor_set_sampler_base_descriptors_->size() ||
            handle.value > descriptor_set_active_->size() || !descriptor_set_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi descriptor set handle is unknown");
        }
        return descriptor_set_sampler_base_descriptors_->at(handle.value - 1U);
    }

    [[nodiscard]] DescriptorSetLayoutHandle descriptor_set_layout_for_set(DescriptorSetHandle handle) const {
        if (descriptor_set_layouts_ == nullptr || descriptor_set_active_ == nullptr || handle.value == 0 ||
            handle.value > descriptor_set_layouts_->size() || handle.value > descriptor_set_active_->size() ||
            !descriptor_set_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi descriptor set handle is unknown");
        }
        return descriptor_set_layouts_->at(handle.value - 1U);
    }

    [[nodiscard]] NativeRootSignatureHandle native_pipeline_layout(PipelineLayoutHandle handle) const {
        if (pipeline_layouts_ == nullptr || pipeline_layout_active_ == nullptr || handle.value == 0 ||
            handle.value > pipeline_layouts_->size() || handle.value > pipeline_layout_active_->size() ||
            !pipeline_layout_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi pipeline layout handle is unknown");
        }
        return pipeline_layouts_->at(handle.value - 1U);
    }

    [[nodiscard]] DescriptorSetLayoutHandle descriptor_set_layout_for_pipeline_layout(PipelineLayoutHandle handle,
                                                                                      std::uint32_t set_index) const {
        if (pipeline_layout_descriptor_sets_ == nullptr || pipeline_layout_active_ == nullptr || handle.value == 0 ||
            handle.value > pipeline_layout_descriptor_sets_->size() || handle.value > pipeline_layout_active_->size() ||
            !pipeline_layout_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi pipeline layout handle is unknown");
        }
        const auto& descriptor_sets = pipeline_layout_descriptor_sets_->at(handle.value - 1U);
        if (set_index >= descriptor_sets.size()) {
            throw std::invalid_argument("d3d12 rhi descriptor set index is outside the pipeline layout");
        }
        return descriptor_sets.at(set_index);
    }

    [[nodiscard]] const D3d12DescriptorSetRootTables&
    descriptor_tables_for_pipeline_layout(PipelineLayoutHandle handle, std::uint32_t set_index) const {
        if (pipeline_layout_descriptor_tables_ == nullptr || pipeline_layout_active_ == nullptr || handle.value == 0 ||
            handle.value > pipeline_layout_descriptor_tables_->size() ||
            handle.value > pipeline_layout_active_->size() || !pipeline_layout_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi pipeline layout handle is unknown");
        }
        const auto& descriptor_tables = pipeline_layout_descriptor_tables_->at(handle.value - 1U);
        if (set_index >= descriptor_tables.size()) {
            throw std::invalid_argument("d3d12 rhi descriptor set index is outside the pipeline layout");
        }
        return descriptor_tables.at(set_index);
    }

    [[nodiscard]] NativeGraphicsPipelineHandle native_graphics_pipeline(GraphicsPipelineHandle handle) const {
        if (graphics_pipelines_ == nullptr || graphics_pipeline_active_ == nullptr || handle.value == 0 ||
            handle.value > graphics_pipelines_->size() || handle.value > graphics_pipeline_active_->size() ||
            !graphics_pipeline_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi graphics pipeline handle is unknown");
        }
        return graphics_pipelines_->at(handle.value - 1U);
    }

    [[nodiscard]] NativeRootSignatureHandle native_root_signature_for_pipeline(GraphicsPipelineHandle handle) const {
        if (graphics_pipeline_root_signatures_ == nullptr || graphics_pipeline_active_ == nullptr ||
            handle.value == 0 || handle.value > graphics_pipeline_root_signatures_->size() ||
            handle.value > graphics_pipeline_active_->size() || !graphics_pipeline_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi graphics pipeline root signature is unknown");
        }
        return graphics_pipeline_root_signatures_->at(handle.value - 1U);
    }

    [[nodiscard]] PipelineLayoutHandle public_pipeline_layout_for_pipeline(GraphicsPipelineHandle handle) const {
        if (graphics_pipeline_layouts_ == nullptr || graphics_pipeline_active_ == nullptr || handle.value == 0 ||
            handle.value > graphics_pipeline_layouts_->size() || handle.value > graphics_pipeline_active_->size() ||
            !graphics_pipeline_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi graphics pipeline layout is unknown");
        }
        return graphics_pipeline_layouts_->at(handle.value - 1U);
    }

    [[nodiscard]] NativeComputePipelineHandle native_compute_pipeline(ComputePipelineHandle handle) const {
        if (compute_pipelines_ == nullptr || compute_pipeline_active_ == nullptr || handle.value == 0 ||
            handle.value > compute_pipelines_->size() || handle.value > compute_pipeline_active_->size() ||
            !compute_pipeline_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi compute pipeline handle is unknown");
        }
        return compute_pipelines_->at(handle.value - 1U);
    }

    [[nodiscard]] NativeRootSignatureHandle
    native_root_signature_for_compute_pipeline(ComputePipelineHandle handle) const {
        if (compute_pipeline_root_signatures_ == nullptr || compute_pipeline_active_ == nullptr || handle.value == 0 ||
            handle.value > compute_pipeline_root_signatures_->size() ||
            handle.value > compute_pipeline_active_->size() || !compute_pipeline_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi compute pipeline root signature is unknown");
        }
        return compute_pipeline_root_signatures_->at(handle.value - 1U);
    }

    [[nodiscard]] PipelineLayoutHandle public_pipeline_layout_for_compute_pipeline(ComputePipelineHandle handle) const {
        if (compute_pipeline_layouts_ == nullptr || compute_pipeline_active_ == nullptr || handle.value == 0 ||
            handle.value > compute_pipeline_layouts_->size() || handle.value > compute_pipeline_active_->size() ||
            !compute_pipeline_active_->at(handle.value - 1U)) {
            throw std::invalid_argument("d3d12 rhi compute pipeline layout is unknown");
        }
        return compute_pipeline_layouts_->at(handle.value - 1U);
    }

    DeviceContext* context_{nullptr};
    NativeCommandListHandle native_;
    QueueKind queue_{QueueKind::graphics};
    const std::vector<NativeSwapchainHandle>* swapchains_{nullptr};
    const std::vector<SwapchainDesc>* swapchain_descs_{nullptr};
    std::vector<ResourceState> swapchain_states_;
    std::vector<bool> swapchain_presentable_;
    std::vector<bool>* swapchain_frame_reserved_{nullptr};
    const std::vector<SwapchainHandle>* swapchain_frame_swapchains_{nullptr};
    std::vector<bool>* swapchain_frame_active_{nullptr};
    std::vector<bool>* swapchain_frame_presented_{nullptr};
    const std::vector<NativeResourceHandle>* buffers_{nullptr};
    const std::vector<BufferDesc>* buffer_descs_{nullptr};
    const std::vector<bool>* buffer_active_{nullptr};
    const std::vector<NativeResourceHandle>* textures_{nullptr};
    const std::vector<TextureDesc>* texture_descs_{nullptr};
    const std::vector<bool>* texture_active_{nullptr};
    std::vector<ResourceState> texture_states_;
    std::vector<ResourceState> initial_texture_states_;
    std::vector<TextureHandle> observed_textures_;
    const std::vector<NativeDescriptorHeapHandle>* descriptor_set_cbv_srv_uav_heaps_{nullptr};
    const std::vector<std::uint32_t>* descriptor_set_cbv_srv_uav_base_descriptors_{nullptr};
    const std::vector<NativeDescriptorHeapHandle>* descriptor_set_sampler_heaps_{nullptr};
    const std::vector<std::uint32_t>* descriptor_set_sampler_base_descriptors_{nullptr};
    const std::vector<DescriptorSetLayoutHandle>* descriptor_set_layouts_{nullptr};
    const std::vector<NativeRootSignatureHandle>* pipeline_layouts_{nullptr};
    const std::vector<std::vector<DescriptorSetLayoutHandle>>* pipeline_layout_descriptor_sets_{nullptr};
    const std::vector<std::vector<D3d12DescriptorSetRootTables>>* pipeline_layout_descriptor_tables_{nullptr};
    const std::vector<NativeGraphicsPipelineHandle>* graphics_pipelines_{nullptr};
    const std::vector<NativeRootSignatureHandle>* graphics_pipeline_root_signatures_{nullptr};
    const std::vector<PipelineLayoutHandle>* graphics_pipeline_layouts_{nullptr};
    const std::vector<bool>* graphics_pipeline_active_{nullptr};
    const std::vector<NativeComputePipelineHandle>* compute_pipelines_{nullptr};
    const std::vector<NativeRootSignatureHandle>* compute_pipeline_root_signatures_{nullptr};
    const std::vector<PipelineLayoutHandle>* compute_pipeline_layouts_{nullptr};
    const std::vector<bool>* compute_pipeline_active_{nullptr};
    const std::vector<bool>* pipeline_layout_active_{nullptr};
    const std::vector<bool>* descriptor_set_active_{nullptr};
    RhiStats* stats_{nullptr};
    std::vector<NativeSwapchainHandle> pending_presents_;
    std::vector<SwapchainFrameHandle> pending_present_frames_;
    std::vector<SwapchainFrameHandle> reserved_swapchain_frames_;
    NativeSwapchainHandle active_swapchain_;
    SwapchainHandle active_swapchain_handle_;
    SwapchainFrameHandle active_swapchain_frame_;
    NativeResourceHandle active_texture_;
    Format active_color_format_{Format::unknown};
    Format active_depth_format_{Format::unknown};
    NativeRootSignatureHandle active_root_signature_;
    NativeRootSignatureHandle active_compute_root_signature_;
    PipelineLayoutHandle bound_pipeline_layout_;
    PipelineLayoutHandle bound_compute_pipeline_layout_;
    bool render_pass_active_{false};
    bool graphics_pipeline_bound_{false};
    bool compute_pipeline_bound_{false};
    bool vertex_buffer_bound_{false};
    bool index_buffer_bound_{false};
    bool closed_{false};
};

namespace {

[[nodiscard]] std::string d3d12_rhi_resource_debug_name(std::string_view prefix, std::uint32_t id) {
    std::string out;
    out.reserve(prefix.size() + 16U);
    out.append(prefix);
    out.push_back('-');
    out.append(std::to_string(id));
    return out;
}

/// D3D12 deferred-destroy ordering mirrors Vulkan: PSO and root signatures before shader bytecode rows, descriptor
/// bookkeeping before buffer/texture committed resources. Descriptor set layouts are CPU-only in this RHI slice.
[[nodiscard]] constexpr int deferred_d3d12_destroy_rank(RhiResourceKind kind) noexcept {
    switch (kind) {
    case RhiResourceKind::graphics_pipeline:
    case RhiResourceKind::compute_pipeline:
        return 0;
    case RhiResourceKind::pipeline_layout:
        return 1;
    case RhiResourceKind::descriptor_set:
        return 2;
    case RhiResourceKind::descriptor_set_layout:
        return 3;
    case RhiResourceKind::shader:
        return 4;
    case RhiResourceKind::sampler:
        return 5;
    case RhiResourceKind::buffer:
        return 6;
    case RhiResourceKind::texture:
        return 7;
    default:
        return 99;
    }
}

} // namespace

class D3d12RhiDevice final : public IRhiDevice {
  public:
    explicit D3d12RhiDevice(std::unique_ptr<DeviceContext> context) noexcept : context_(std::move(context)) {}
    ~D3d12RhiDevice() override;

    [[nodiscard]] BackendKind backend_kind() const noexcept override {
        return BackendKind::d3d12;
    }

    [[nodiscard]] std::string_view backend_name() const noexcept override {
        return "d3d12";
    }

    [[nodiscard]] RhiStats stats() const noexcept override {
        return stats_;
    }

    [[nodiscard]] std::uint64_t gpu_timestamp_ticks_per_second() const noexcept override {
        if (!context_) {
            return 0;
        }
        return context_->gpu_timestamp_ticks_per_second();
    }

    [[nodiscard]] QueueCalibratedTiming measure_calibrated_queue_timing(QueueKind queue) {
        QueueCalibratedTiming result{};
        result.queue = queue;
        if (!context_) {
            result.status = QueueCalibratedTimingStatus::unsupported;
            result.diagnostic = "d3d12 rhi device calibrated queue timing context unavailable";
            return result;
        }
        return context_->measure_calibrated_queue_timing(queue);
    }

    [[nodiscard]] SubmittedCommandCalibratedTiming read_submitted_command_calibrated_timing(FenceValue fence) {
        SubmittedCommandCalibratedTiming result{};
        result.queue = fence.queue;
        result.submitted_fence = fence;
        if (!context_) {
            result.status = SubmittedCommandCalibratedTimingStatus::unsupported;
            result.diagnostic = "d3d12 rhi device submitted command timing context unavailable";
            return result;
        }
        return context_->read_submitted_command_calibrated_timing(fence);
    }

    [[nodiscard]] RhiDeviceMemoryDiagnostics memory_diagnostics() const override {
        if (!context_) {
            return RhiDeviceMemoryDiagnostics{};
        }
        return context_->memory_diagnostics();
    }

    [[nodiscard]] const RhiResourceLifetimeRegistry* resource_lifetime_registry() const noexcept override {
        return &resource_lifetime_;
    }

    [[nodiscard]] D3d12SharedTextureExportResult export_shared_texture(TextureHandle texture) noexcept {
        D3d12SharedTextureExportResult result{};
        if (!owns_texture(texture)) {
            result.invalid_texture = true;
            ++stats_.shared_texture_export_failures;
            return result;
        }

        const auto index = texture.value - 1U;
        const auto& desc = texture_descs_[index];
        result.extent = Extent2D{.width = desc.extent.width, .height = desc.extent.height};
        result.format = desc.format;
        if (!has_flag(desc.usage, TextureUsage::shared)) {
            result.texture_not_shareable = true;
            ++stats_.shared_texture_export_failures;
            return result;
        }

        result = context_->export_shared_texture(texture_handles_[index], desc);
        if (result.succeeded) {
            ++stats_.shared_texture_exports;
        } else {
            ++stats_.shared_texture_export_failures;
        }
        return result;
    }

    [[nodiscard]] BufferHandle create_buffer(const BufferDesc& desc) override {
        const auto native = context_->create_committed_buffer(desc);
        if (native.value == 0) {
            throw std::invalid_argument("d3d12 rhi buffer description is invalid or unsupported");
        }

        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::buffer,
            .owner = "d3d12",
            .debug_name =
                d3d12_rhi_resource_debug_name("buffer", static_cast<std::uint32_t>(buffer_handles_.size() + 1U)),
        });
        if (!lifetime_registration.succeeded()) {
            context_->destroy_committed_resource(native);
            throw std::logic_error("d3d12 rhi buffer lifetime registration failed");
        }

        buffer_handles_.push_back(native);
        buffer_descs_.push_back(desc);
        buffer_active_.push_back(true);
        buffer_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.buffers_created;
        return BufferHandle{static_cast<std::uint32_t>(buffer_handles_.size())};
    }

    [[nodiscard]] TextureHandle create_texture(const TextureDesc& desc) override {
        const auto native = context_->create_committed_texture(desc);
        if (native.value == 0) {
            throw std::invalid_argument("d3d12 rhi texture description is invalid or unsupported");
        }

        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::texture,
            .owner = "d3d12",
            .debug_name =
                d3d12_rhi_resource_debug_name("texture", static_cast<std::uint32_t>(texture_handles_.size() + 1U)),
        });
        if (!lifetime_registration.succeeded()) {
            context_->destroy_committed_resource(native);
            throw std::logic_error("d3d12 rhi texture lifetime registration failed");
        }

        texture_handles_.push_back(native);
        texture_descs_.push_back(desc);
        texture_active_.push_back(true);
        texture_states_.push_back(initial_texture_state(desc.usage));
        texture_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.textures_created;
        return TextureHandle{static_cast<std::uint32_t>(texture_handles_.size())};
    }

    [[nodiscard]] SamplerHandle create_sampler(const SamplerDesc& desc) override {
        if (!valid_sampler_desc(desc)) {
            throw std::invalid_argument("d3d12 rhi sampler description is invalid or unsupported");
        }

        sampler_descs_.push_back(desc);
        sampler_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::sampler,
            .owner = "d3d12",
            .debug_name = d3d12_rhi_resource_debug_name("sampler", static_cast<std::uint32_t>(sampler_descs_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            sampler_descs_.pop_back();
            sampler_active_.pop_back();
            throw std::logic_error("d3d12 rhi sampler lifetime registration failed");
        }
        sampler_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.samplers_created;
        return SamplerHandle{static_cast<std::uint32_t>(sampler_descs_.size())};
    }

    [[nodiscard]] SwapchainHandle create_swapchain(const SwapchainDesc& desc) override {
        if (desc.surface.value == 0) {
            throw std::invalid_argument("d3d12 rhi swapchain creation requires a surface handle");
        }

        const auto native = context_->create_swapchain_for_window(NativeSwapchainDesc{
            .window = NativeWindowHandle{reinterpret_cast<void*>(desc.surface.value)},
            .swapchain = desc,
        });
        if (native.value == 0) {
            throw std::invalid_argument("d3d12 rhi swapchain description is invalid or unsupported");
        }

        swapchain_handles_.push_back(native);
        swapchain_descs_.push_back(desc);
        swapchain_states_.push_back(ResourceState::present);
        swapchain_presentable_.push_back(false);
        swapchain_frame_reserved_.push_back(false);
        ++stats_.swapchains_created;
        return SwapchainHandle{static_cast<std::uint32_t>(swapchain_handles_.size())};
    }

    void resize_swapchain(SwapchainHandle swapchain, Extent2D extent) override {
        if (!owns_swapchain(swapchain)) {
            throw std::invalid_argument("d3d12 rhi swapchain handle is unknown");
        }
        if (extent.width == 0 || extent.height == 0) {
            throw std::invalid_argument("d3d12 rhi swapchain extent must be non-zero");
        }

        const auto index = swapchain.value - 1U;
        if (swapchain_states_.at(index) != ResourceState::present || swapchain_presentable_.at(index) ||
            swapchain_frame_reserved_.at(index)) {
            throw std::invalid_argument("d3d12 rhi swapchain cannot be resized while a frame is pending presentation");
        }
        if (!context_->resize_swapchain(swapchain_handles_.at(index), extent)) {
            throw std::logic_error("d3d12 rhi swapchain resize failed");
        }

        swapchain_descs_.at(index).extent = extent;
        swapchain_states_.at(index) = ResourceState::present;
        swapchain_presentable_.at(index) = false;
        swapchain_frame_reserved_.at(index) = false;
        ++stats_.swapchain_resizes;
    }

    [[nodiscard]] SwapchainFrameHandle acquire_swapchain_frame(SwapchainHandle swapchain) override {
        if (!owns_swapchain(swapchain)) {
            throw std::invalid_argument("d3d12 rhi swapchain handle is unknown");
        }
        const auto index = swapchain.value - 1U;
        if (swapchain_states_.at(index) != ResourceState::present || swapchain_presentable_.at(index) ||
            swapchain_frame_reserved_.at(index)) {
            throw std::invalid_argument("d3d12 rhi swapchain already has a pending frame");
        }

        swapchain_frame_reserved_.at(index) = true;
        swapchain_frame_swapchains_.push_back(swapchain);
        swapchain_frame_active_.push_back(true);
        swapchain_frame_presented_.push_back(false);
        ++stats_.swapchain_frames_acquired;
        return SwapchainFrameHandle{static_cast<std::uint32_t>(swapchain_frame_swapchains_.size())};
    }

    void release_swapchain_frame(SwapchainFrameHandle frame) override {
        if (!owns_swapchain_frame(frame)) {
            throw std::invalid_argument("d3d12 rhi swapchain frame handle is unknown");
        }
        const auto frame_index = frame.value - 1U;
        if (swapchain_frame_presented_.at(frame_index)) {
            throw std::invalid_argument(
                "d3d12 rhi swapchain frame cannot be manually released after present recording");
        }
        complete_swapchain_frame(frame);
    }

    [[nodiscard]] TransientBuffer acquire_transient_buffer(const BufferDesc& desc) override {
        const auto buffer = create_buffer(desc);
        const auto lease = TransientResourceHandle{next_transient_resource_++};
        transient_leases_.push_back(TransientLeaseRecord{
            .kind = TransientResourceKind::buffer,
            .buffer = buffer,
            .texture = TextureHandle{},
            .active = true,
        });
        ++stats_.transient_resources_acquired;
        ++stats_.transient_resources_active;
        return TransientBuffer{.lease = lease, .buffer = buffer};
    }

    [[nodiscard]] TransientTexture acquire_transient_texture(const TextureDesc& desc) override {
        const auto native = context_->create_placed_texture(desc);
        if (native.value == 0) {
            throw std::invalid_argument("d3d12 rhi transient texture description is invalid or unsupported");
        }

        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::texture,
            .owner = "d3d12",
            .debug_name =
                d3d12_rhi_resource_debug_name("texture", static_cast<std::uint32_t>(texture_handles_.size() + 1U)),
        });
        if (!lifetime_registration.succeeded()) {
            context_->destroy_committed_resource(native);
            throw std::logic_error("d3d12 rhi transient texture lifetime registration failed");
        }

        texture_handles_.push_back(native);
        texture_descs_.push_back(desc);
        texture_active_.push_back(true);
        texture_states_.push_back(initial_texture_state(desc.usage));
        texture_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.textures_created;
        ++stats_.transient_texture_heap_allocations;
        ++stats_.transient_texture_placed_allocations;
        ++stats_.transient_texture_placed_resources_alive;
        const auto texture = TextureHandle{static_cast<std::uint32_t>(texture_handles_.size())};
        const auto lease = TransientResourceHandle{next_transient_resource_++};
        transient_leases_.push_back(TransientLeaseRecord{
            .kind = TransientResourceKind::texture,
            .buffer = BufferHandle{},
            .texture = texture,
            .active = true,
        });
        ++stats_.transient_resources_acquired;
        ++stats_.transient_resources_active;
        return TransientTexture{.lease = lease, .texture = texture};
    }

    void release_transient(TransientResourceHandle lease) override {
        if (lease.value == 0 || lease.value >= next_transient_resource_) {
            throw std::invalid_argument("d3d12 rhi transient resource lease is unknown");
        }
        auto& record = transient_leases_.at(lease.value - 1U);
        if (!record.active) {
            throw std::invalid_argument("d3d12 rhi transient resource lease is already released");
        }

        record.active = false;
        if (record.kind == TransientResourceKind::buffer) {
            const auto buffer = record.buffer;
            const auto index = buffer.value - 1U;
            if (buffer_active_.at(index)) {
                const auto release_fence = context_->last_submitted_fence().value;
                (void)resource_lifetime_.release_resource_deferred(buffer_lifetime_.at(index), release_fence);
                buffer_active_.at(index) = false;
            }
        } else {
            const auto texture = record.texture;
            const auto index = texture.value - 1U;
            if (texture_active_.at(index)) {
                const auto release_fence = context_->last_submitted_fence().value;
                (void)resource_lifetime_.release_resource_deferred(texture_lifetime_.at(index), release_fence);
                texture_active_.at(index) = false;
                texture_states_.at(index) = ResourceState::undefined;
            }
        }
        retire_deferred_committed_resources(context_->completed_fence().value);
        ++stats_.transient_resources_released;
        --stats_.transient_resources_active;
    }

    [[nodiscard]] ShaderHandle create_shader(const ShaderDesc& desc) override {
        if (!valid_shader_stage(desc.stage) || desc.entry_point.empty() || desc.bytecode == nullptr ||
            desc.bytecode_size == 0) {
            throw std::invalid_argument("d3d12 rhi shader requires entry point and bytecode");
        }

        const auto native = context_->create_shader_module(NativeShaderBytecodeDesc{
            .stage = desc.stage,
            .bytecode = desc.bytecode,
            .bytecode_size = desc.bytecode_size,
        });
        if (native.value == 0) {
            throw std::invalid_argument("d3d12 rhi shader bytecode is invalid or unsupported");
        }

        shader_handles_.push_back(native);
        shader_stages_.push_back(desc.stage);
        shader_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::shader,
            .owner = "d3d12",
            .debug_name = d3d12_rhi_resource_debug_name("shader", static_cast<std::uint32_t>(shader_handles_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            context_->destroy_shader_module(native);
            shader_handles_.pop_back();
            shader_stages_.pop_back();
            shader_active_.pop_back();
            throw std::logic_error("d3d12 rhi shader lifetime registration failed");
        }
        shader_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.shader_modules_created;
        return ShaderHandle{static_cast<std::uint32_t>(shader_handles_.size())};
    }

    [[nodiscard]] DescriptorSetLayoutHandle create_descriptor_set_layout(const DescriptorSetLayoutDesc& desc) override {
        if (!valid_descriptor_set_layout_desc(desc)) {
            throw std::invalid_argument("d3d12 rhi descriptor set layout description is invalid or unsupported");
        }

        descriptor_set_layouts_.push_back(desc);
        descriptor_set_layout_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::descriptor_set_layout,
            .owner = "d3d12",
            .debug_name = d3d12_rhi_resource_debug_name("descriptor-set-layout",
                                                        static_cast<std::uint32_t>(descriptor_set_layouts_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            descriptor_set_layouts_.pop_back();
            descriptor_set_layout_active_.pop_back();
            throw std::logic_error("d3d12 rhi descriptor set layout lifetime registration failed");
        }
        descriptor_set_layout_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.descriptor_set_layouts_created;
        return DescriptorSetLayoutHandle{static_cast<std::uint32_t>(descriptor_set_layouts_.size())};
    }

    [[nodiscard]] DescriptorSetHandle allocate_descriptor_set(DescriptorSetLayoutHandle layout) override {
        if (!owns_descriptor_set_layout(layout)) {
            throw std::invalid_argument("d3d12 rhi descriptor set layout handle is unknown");
        }

        const auto& desc = descriptor_set_layouts_.at(layout.value - 1U);
        const auto cbv_srv_uav_capacity = descriptor_capacity(desc, NativeDescriptorHeapKind::cbv_srv_uav);
        const auto sampler_capacity = descriptor_capacity(desc, NativeDescriptorHeapKind::sampler);
        const auto cbv_srv_uav_heap =
            cbv_srv_uav_capacity > 0
                ? ensure_descriptor_arena(NativeDescriptorHeapKind::cbv_srv_uav, cbv_srv_uav_capacity)
                : NativeDescriptorHeapHandle{};
        const auto sampler_heap = sampler_capacity > 0
                                      ? ensure_descriptor_arena(NativeDescriptorHeapKind::sampler, sampler_capacity)
                                      : NativeDescriptorHeapHandle{};
        if ((cbv_srv_uav_capacity > 0 && cbv_srv_uav_heap.value == 0) ||
            (sampler_capacity > 0 && sampler_heap.value == 0)) {
            throw std::invalid_argument("d3d12 rhi descriptor set heap allocation failed");
        }
        const auto saved_cbv_next = descriptor_arena_next_;
        const auto saved_sampler_next = sampler_descriptor_arena_next_;
        const auto cbv_srv_uav_base_descriptor = descriptor_arena_next_;
        descriptor_arena_next_ += cbv_srv_uav_capacity;
        const auto sampler_base_descriptor = sampler_descriptor_arena_next_;
        sampler_descriptor_arena_next_ += sampler_capacity;

        descriptor_sets_.push_back(DescriptorSetRecord{
            .layout = layout,
            .cbv_srv_uav_heap = cbv_srv_uav_heap,
            .cbv_srv_uav_base_descriptor = cbv_srv_uav_base_descriptor,
            .cbv_srv_uav_capacity = cbv_srv_uav_capacity,
            .sampler_heap = sampler_heap,
            .sampler_base_descriptor = sampler_base_descriptor,
            .sampler_capacity = sampler_capacity,
        });
        descriptor_set_cbv_srv_uav_heaps_.push_back(cbv_srv_uav_heap);
        descriptor_set_cbv_srv_uav_base_descriptors_.push_back(cbv_srv_uav_base_descriptor);
        descriptor_set_sampler_heaps_.push_back(sampler_heap);
        descriptor_set_sampler_base_descriptors_.push_back(sampler_base_descriptor);
        descriptor_set_layout_handles_.push_back(layout);
        descriptor_set_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::descriptor_set,
            .owner = "d3d12",
            .debug_name =
                d3d12_rhi_resource_debug_name("descriptor-set", static_cast<std::uint32_t>(descriptor_sets_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            descriptor_set_active_.pop_back();
            descriptor_set_layout_handles_.pop_back();
            descriptor_set_sampler_base_descriptors_.pop_back();
            descriptor_set_sampler_heaps_.pop_back();
            descriptor_set_cbv_srv_uav_base_descriptors_.pop_back();
            descriptor_set_cbv_srv_uav_heaps_.pop_back();
            descriptor_sets_.pop_back();
            descriptor_arena_next_ = saved_cbv_next;
            sampler_descriptor_arena_next_ = saved_sampler_next;
            throw std::logic_error("d3d12 rhi descriptor set lifetime registration failed");
        }
        descriptor_set_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.descriptor_sets_allocated;
        return DescriptorSetHandle{static_cast<std::uint32_t>(descriptor_sets_.size())};
    }

    void update_descriptor_set(const DescriptorWrite& write) override {
        if (!owns_descriptor_set(write.set)) {
            throw std::invalid_argument("d3d12 rhi descriptor set handle is unknown");
        }
        if (write.resources.empty()) {
            throw std::invalid_argument("d3d12 rhi descriptor write must contain at least one resource");
        }

        const auto layout = descriptor_set_layout_for_set(write.set);
        const auto& layout_desc = descriptor_set_layouts_.at(layout.value - 1U);
        const auto& binding = descriptor_binding(layout_desc, write.binding);
        const auto resource_count = static_cast<std::uint32_t>(write.resources.size());
        if (write.array_element >= binding.count || resource_count > binding.count - write.array_element) {
            throw std::invalid_argument("d3d12 rhi descriptor write exceeds binding range");
        }

        const auto& set_record = descriptor_sets_.at(write.set.value - 1U);
        const auto heap_kind = descriptor_heap_kind_for_type(binding.type);
        const auto binding_offset = descriptor_binding_offset(layout_desc, write.binding, heap_kind);
        for (std::uint32_t index = 0; index < resource_count; ++index) {
            const auto& resource = write.resources.at(index);
            if (resource.type != binding.type) {
                throw std::invalid_argument("d3d12 rhi descriptor resource type does not match binding");
            }
            validate_descriptor_resource(resource, binding.type);
            const auto descriptor_heap =
                heap_kind == NativeDescriptorHeapKind::sampler ? set_record.sampler_heap : set_record.cbv_srv_uav_heap;
            const auto descriptor_base = heap_kind == NativeDescriptorHeapKind::sampler
                                             ? set_record.sampler_base_descriptor
                                             : set_record.cbv_srv_uav_base_descriptor;
            const auto native_resource = binding.type == DescriptorType::sampler
                                             ? NativeResourceHandle{}
                                             : native_resource_handle(resource, binding.type);
            if (!context_->write_descriptor(NativeDescriptorWriteDesc{
                    .heap = descriptor_heap,
                    .descriptor_index = descriptor_base + binding_offset + write.array_element + index,
                    .resource = native_resource,
                    .type = binding.type,
                    .sampler = binding.type == DescriptorType::sampler
                                   ? sampler_descs_.at(resource.sampler_handle.value - 1U)
                                   : SamplerDesc{},
                })) {
                throw std::invalid_argument("d3d12 rhi descriptor resource cannot be written natively");
            }
        }

        ++stats_.descriptor_writes;
    }

    [[nodiscard]] PipelineLayoutHandle create_pipeline_layout(const PipelineLayoutDesc& desc) override {
        if (!valid_rhi_pipeline_layout_desc(desc)) {
            throw std::invalid_argument("d3d12 rhi pipeline layout description is invalid or unsupported");
        }

        std::vector<DescriptorSetLayoutDesc> descriptor_sets;
        descriptor_sets.reserve(desc.descriptor_sets.size());
        for (const auto layout : desc.descriptor_sets) {
            if (!owns_descriptor_set_layout(layout)) {
                throw std::invalid_argument("d3d12 rhi pipeline layout references an unknown descriptor set layout");
            }
            descriptor_sets.push_back(descriptor_set_layouts_.at(layout.value - 1U));
        }

        const auto native = context_->create_root_signature(NativeRootSignatureDesc{
            .descriptor_sets = descriptor_sets,
            .push_constant_bytes = desc.push_constant_bytes,
        });
        if (native.value == 0) {
            throw std::invalid_argument("d3d12 rhi pipeline layout description is invalid or unsupported");
        }

        pipeline_layout_handles_.push_back(native);
        pipeline_layout_descriptor_sets_.push_back(desc.descriptor_sets);
        pipeline_layout_descriptor_tables_.push_back(make_pipeline_layout_descriptor_tables(descriptor_sets));
        pipeline_layout_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::pipeline_layout,
            .owner = "d3d12",
            .debug_name = d3d12_rhi_resource_debug_name("pipeline-layout",
                                                        static_cast<std::uint32_t>(pipeline_layout_handles_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            context_->destroy_root_signature(native);
            pipeline_layout_handles_.pop_back();
            pipeline_layout_descriptor_sets_.pop_back();
            pipeline_layout_descriptor_tables_.pop_back();
            pipeline_layout_active_.pop_back();
            throw std::logic_error("d3d12 rhi pipeline layout lifetime registration failed");
        }
        pipeline_layout_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.pipeline_layouts_created;
        return PipelineLayoutHandle{static_cast<std::uint32_t>(pipeline_layout_handles_.size())};
    }

    [[nodiscard]] GraphicsPipelineHandle create_graphics_pipeline(const GraphicsPipelineDesc& desc) override {
        if (!valid_rhi_graphics_pipeline_desc(desc)) {
            throw std::invalid_argument("d3d12 rhi graphics pipeline description is invalid or unsupported");
        }
        if (!owns_pipeline_layout(desc.layout) || !owns_shader(desc.vertex_shader) ||
            !owns_shader(desc.fragment_shader)) {
            throw std::invalid_argument("d3d12 rhi graphics pipeline references unknown handles");
        }
        if (shader_stages_.at(desc.vertex_shader.value - 1U) != ShaderStage::vertex ||
            shader_stages_.at(desc.fragment_shader.value - 1U) != ShaderStage::fragment) {
            throw std::invalid_argument("d3d12 rhi graphics pipeline shader stages are incompatible");
        }

        const auto native = context_->create_graphics_pipeline(NativeGraphicsPipelineDesc{
            .root_signature = pipeline_layout_handles_.at(desc.layout.value - 1U),
            .vertex_shader = shader_handles_.at(desc.vertex_shader.value - 1U),
            .fragment_shader = shader_handles_.at(desc.fragment_shader.value - 1U),
            .color_format = desc.color_format,
            .depth_format = desc.depth_format,
            .topology = desc.topology,
            .vertex_buffers = desc.vertex_buffers,
            .vertex_attributes = desc.vertex_attributes,
            .depth_state = desc.depth_state,
        });
        if (native.value == 0) {
            throw std::invalid_argument("d3d12 rhi graphics pipeline description is invalid or unsupported");
        }

        graphics_pipeline_handles_.push_back(native);
        graphics_pipeline_root_signatures_.push_back(pipeline_layout_handles_.at(desc.layout.value - 1U));
        graphics_pipeline_layouts_.push_back(desc.layout);
        graphics_pipeline_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::graphics_pipeline,
            .owner = "d3d12",
            .debug_name = d3d12_rhi_resource_debug_name("graphics-pipeline",
                                                        static_cast<std::uint32_t>(graphics_pipeline_handles_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            context_->destroy_graphics_pipeline(native);
            graphics_pipeline_handles_.pop_back();
            graphics_pipeline_root_signatures_.pop_back();
            graphics_pipeline_layouts_.pop_back();
            graphics_pipeline_active_.pop_back();
            throw std::logic_error("d3d12 rhi graphics pipeline lifetime registration failed");
        }
        graphics_pipeline_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.graphics_pipelines_created;
        return GraphicsPipelineHandle{static_cast<std::uint32_t>(graphics_pipeline_handles_.size())};
    }

    [[nodiscard]] ComputePipelineHandle create_compute_pipeline(const ComputePipelineDesc& desc) override {
        if (!valid_rhi_compute_pipeline_desc(desc)) {
            throw std::invalid_argument("d3d12 rhi compute pipeline description is invalid or unsupported");
        }
        if (!owns_pipeline_layout(desc.layout) || !owns_shader(desc.compute_shader)) {
            throw std::invalid_argument("d3d12 rhi compute pipeline references unknown handles");
        }
        if (shader_stages_.at(desc.compute_shader.value - 1U) != ShaderStage::compute) {
            throw std::invalid_argument("d3d12 rhi compute pipeline shader stage is incompatible");
        }

        const auto native = context_->create_compute_pipeline(NativeComputePipelineDesc{
            .root_signature = pipeline_layout_handles_.at(desc.layout.value - 1U),
            .compute_shader = shader_handles_.at(desc.compute_shader.value - 1U),
        });
        if (native.value == 0) {
            throw std::invalid_argument("d3d12 rhi compute pipeline description is invalid or unsupported");
        }

        compute_pipeline_handles_.push_back(native);
        compute_pipeline_root_signatures_.push_back(pipeline_layout_handles_.at(desc.layout.value - 1U));
        compute_pipeline_layouts_.push_back(desc.layout);
        compute_pipeline_active_.push_back(true);
        const auto lifetime_registration = resource_lifetime_.register_resource(RhiResourceRegistrationDesc{
            .kind = RhiResourceKind::compute_pipeline,
            .owner = "d3d12",
            .debug_name = d3d12_rhi_resource_debug_name("compute-pipeline",
                                                        static_cast<std::uint32_t>(compute_pipeline_handles_.size())),
        });
        if (!lifetime_registration.succeeded()) {
            context_->destroy_compute_pipeline(native);
            compute_pipeline_handles_.pop_back();
            compute_pipeline_root_signatures_.pop_back();
            compute_pipeline_layouts_.pop_back();
            compute_pipeline_active_.pop_back();
            throw std::logic_error("d3d12 rhi compute pipeline lifetime registration failed");
        }
        compute_pipeline_lifetime_.push_back(lifetime_registration.handle);
        ++stats_.compute_pipelines_created;
        return ComputePipelineHandle{static_cast<std::uint32_t>(compute_pipeline_handles_.size())};
    }

    [[nodiscard]] std::unique_ptr<IRhiCommandList> begin_command_list(QueueKind queue) override {
        const auto native = context_->create_command_list(queue);
        if (native.value == 0) {
            throw std::invalid_argument("d3d12 rhi command queue kind is invalid or unsupported");
        }

        ++stats_.command_lists_begun;
        return std::make_unique<D3d12RhiCommandList>(
            *context_, native, queue, swapchain_handles_, swapchain_descs_, swapchain_states_, swapchain_presentable_,
            swapchain_frame_reserved_, swapchain_frame_swapchains_, swapchain_frame_active_, swapchain_frame_presented_,
            buffer_handles_, buffer_descs_, buffer_active_, texture_handles_, texture_descs_, texture_active_,
            texture_states_, descriptor_set_cbv_srv_uav_heaps_, descriptor_set_cbv_srv_uav_base_descriptors_,
            descriptor_set_sampler_heaps_, descriptor_set_sampler_base_descriptors_, descriptor_set_layout_handles_,
            pipeline_layout_handles_, pipeline_layout_descriptor_sets_, pipeline_layout_descriptor_tables_,
            graphics_pipeline_handles_, graphics_pipeline_root_signatures_, graphics_pipeline_layouts_,
            graphics_pipeline_active_, compute_pipeline_handles_, compute_pipeline_root_signatures_,
            compute_pipeline_layouts_, compute_pipeline_active_, pipeline_layout_active_, descriptor_set_active_,
            stats_);
    }

    [[nodiscard]] FenceValue submit(IRhiCommandList& commands) override {
        auto* d3d12_commands = dynamic_cast<D3d12RhiCommandList*>(&commands);
        if (d3d12_commands == nullptr) {
            throw std::invalid_argument("d3d12 rhi command list must belong to this backend");
        }
        if (!d3d12_commands->closed()) {
            throw std::logic_error("d3d12 rhi command list must be closed before submit");
        }
        if (!d3d12_commands->observed_texture_states_match(texture_states_)) {
            throw std::logic_error("d3d12 rhi command list was recorded against stale texture state");
        }

        const auto fence = context_->execute_command_list(d3d12_commands->native_handle());
        if (fence.value == 0) {
            throw std::logic_error("d3d12 rhi command list submission failed");
        }

        d3d12_commands->commit_texture_states(texture_states_);
        d3d12_commands->commit_swapchain_states(swapchain_states_, swapchain_presentable_);
        ++stats_.command_lists_submitted;
        ++stats_.fences_signaled;
        stats_.last_submitted_fence_value = fence.value;
        last_submitted_fence_ = fence;
        record_queue_submit(stats_, d3d12_commands->queue_kind(), fence);
        for (const auto swapchain : d3d12_commands->pending_presents()) {
            if (!context_->present_swapchain(swapchain)) {
                throw std::logic_error("d3d12 rhi swapchain present failed");
            }
            ++stats_.present_calls;
        }
        d3d12_commands->release_submitted_swapchain_frames();
        stats_.last_completed_fence_value = context_->completed_fence().value;
        retire_deferred_committed_resources(stats_.last_completed_fence_value);
        return fence;
    }

    void wait(FenceValue fence) override {
        ++stats_.fence_waits;
        if (!context_->wait_for_fence(fence, 0xFFFFFFFFU)) {
            ++stats_.fence_wait_failures;
            throw std::logic_error("d3d12 rhi fence wait failed");
        }
        stats_.last_completed_fence_value = context_->completed_fence().value;
        retire_deferred_committed_resources(stats_.last_completed_fence_value);
    }

    void wait_for_queue(QueueKind queue, FenceValue fence) override {
        ++stats_.queue_waits;
        if (!context_->queue_wait_for_fence(queue, fence)) {
            ++stats_.queue_wait_failures;
            throw std::logic_error("d3d12 rhi queue wait failed");
        }
        record_queue_wait(stats_, queue, fence);
    }

    void write_buffer(BufferHandle buffer, std::uint64_t offset, std::span<const std::uint8_t> bytes) override {
        if (!owns_buffer(buffer)) {
            throw std::invalid_argument("d3d12 rhi write buffer handle is unknown");
        }
        if (bytes.empty()) {
            throw std::invalid_argument("d3d12 rhi write buffer byte span must be non-empty");
        }
        const auto& desc = buffer_descs_.at(buffer.value - 1U);
        if (!has_flag(desc.usage, BufferUsage::copy_source)) {
            throw std::invalid_argument("d3d12 rhi write buffer requires a copy_source upload buffer");
        }
        if (offset > desc.size_bytes || bytes.size() > desc.size_bytes - offset) {
            throw std::invalid_argument("d3d12 rhi write buffer range is outside the buffer");
        }
        if (!context_->write_buffer(buffer_handles_.at(buffer.value - 1U), offset, bytes)) {
            throw std::logic_error("d3d12 rhi write buffer mapping failed");
        }

        ++stats_.buffer_writes;
        stats_.bytes_written += static_cast<std::uint64_t>(bytes.size());
    }

    [[nodiscard]] std::vector<std::uint8_t> read_buffer(BufferHandle buffer, std::uint64_t offset,
                                                        std::uint64_t size_bytes) override {
        if (!owns_buffer(buffer)) {
            throw std::invalid_argument("d3d12 rhi read buffer handle is unknown");
        }
        if (size_bytes == 0) {
            throw std::invalid_argument("d3d12 rhi read buffer size must be non-zero");
        }
        const auto& desc = buffer_descs_.at(buffer.value - 1U);
        if (!has_flag(desc.usage, BufferUsage::copy_destination)) {
            throw std::invalid_argument("d3d12 rhi read buffer requires copy_destination usage");
        }
        if (offset > desc.size_bytes || size_bytes > desc.size_bytes - offset) {
            throw std::invalid_argument("d3d12 rhi read buffer range is outside the buffer");
        }

        auto bytes = context_->read_buffer(buffer_handles_.at(buffer.value - 1U), offset, size_bytes);
        if (bytes.size() != size_bytes) {
            throw std::logic_error("d3d12 rhi read buffer mapping failed");
        }

        ++stats_.buffer_reads;
        stats_.bytes_read += size_bytes;
        return bytes;
    }

  private:
    struct DescriptorSetRecord {
        DescriptorSetLayoutHandle layout;
        NativeDescriptorHeapHandle cbv_srv_uav_heap;
        std::uint32_t cbv_srv_uav_base_descriptor{0};
        std::uint32_t cbv_srv_uav_capacity{0};
        NativeDescriptorHeapHandle sampler_heap;
        std::uint32_t sampler_base_descriptor{0};
        std::uint32_t sampler_capacity{0};
    };

    struct TransientLeaseRecord {
        TransientResourceKind kind{TransientResourceKind::buffer};
        BufferHandle buffer;
        TextureHandle texture;
        bool active{false};
    };

    /// Destroys native committed resources whose lifetime records are eligible to retire at `completed_frame`,
    /// then removes those records from `resource_lifetime_`.
    void retire_deferred_committed_resources(std::uint64_t completed_frame) noexcept;

    [[nodiscard]] bool owns_descriptor_set_layout(DescriptorSetLayoutHandle handle) const noexcept {
        return handle.value > 0 && handle.value <= descriptor_set_layouts_.size() &&
               handle.value <= descriptor_set_layout_active_.size() &&
               descriptor_set_layout_active_.at(handle.value - 1U);
    }

    [[nodiscard]] bool owns_descriptor_set(DescriptorSetHandle handle) const noexcept {
        return handle.value > 0 && handle.value <= descriptor_sets_.size() &&
               handle.value <= descriptor_set_active_.size() && descriptor_set_active_.at(handle.value - 1U);
    }

    [[nodiscard]] bool owns_buffer(BufferHandle handle) const noexcept {
        return handle.value > 0 && handle.value <= buffer_handles_.size() && handle.value <= buffer_active_.size() &&
               buffer_active_.at(handle.value - 1U);
    }

    [[nodiscard]] bool owns_texture(TextureHandle handle) const noexcept {
        return handle.value > 0 && handle.value <= texture_handles_.size() && handle.value <= texture_active_.size() &&
               texture_active_.at(handle.value - 1U);
    }

    [[nodiscard]] bool owns_sampler(SamplerHandle handle) const noexcept {
        return handle.value > 0 && handle.value <= sampler_descs_.size() && handle.value <= sampler_active_.size() &&
               sampler_active_.at(handle.value - 1U);
    }

    [[nodiscard]] bool owns_swapchain(SwapchainHandle handle) const noexcept {
        return handle.value > 0 && handle.value <= swapchain_handles_.size() && handle.value <= swapchain_descs_.size();
    }

    [[nodiscard]] bool owns_swapchain_frame(SwapchainFrameHandle frame) const noexcept {
        return frame.value > 0 && frame.value <= swapchain_frame_swapchains_.size() &&
               frame.value <= swapchain_frame_active_.size() && swapchain_frame_active_.at(frame.value - 1U);
    }

    [[nodiscard]] SwapchainHandle swapchain_for_frame(SwapchainFrameHandle frame) const {
        if (!owns_swapchain_frame(frame)) {
            throw std::invalid_argument("d3d12 rhi swapchain frame handle is unknown");
        }
        return swapchain_frame_swapchains_.at(frame.value - 1U);
    }

    void complete_swapchain_frame(SwapchainFrameHandle frame) {
        const auto swapchain = swapchain_for_frame(frame);
        const auto swapchain_index = swapchain.value - 1U;
        const auto frame_index = frame.value - 1U;
        swapchain_frame_reserved_.at(swapchain_index) = false;
        swapchain_states_.at(swapchain_index) = ResourceState::present;
        swapchain_presentable_.at(swapchain_index) = false;
        swapchain_frame_active_.at(frame_index) = false;
        swapchain_frame_presented_.at(frame_index) = false;
        ++stats_.swapchain_frames_released;
    }

    [[nodiscard]] bool owns_pipeline_layout(PipelineLayoutHandle handle) const noexcept {
        return handle.value > 0 && handle.value <= pipeline_layout_handles_.size() &&
               handle.value <= pipeline_layout_active_.size() && pipeline_layout_active_.at(handle.value - 1U);
    }

    [[nodiscard]] bool owns_shader(ShaderHandle handle) const noexcept {
        return handle.value > 0 && handle.value <= shader_handles_.size() && handle.value <= shader_active_.size() &&
               shader_active_.at(handle.value - 1U);
    }

    [[nodiscard]] bool owns_graphics_pipeline(GraphicsPipelineHandle handle) const noexcept {
        return handle.value > 0 && handle.value <= graphics_pipeline_handles_.size() &&
               handle.value <= graphics_pipeline_active_.size() && graphics_pipeline_active_.at(handle.value - 1U);
    }

    [[nodiscard]] bool owns_compute_pipeline(ComputePipelineHandle handle) const noexcept {
        return handle.value > 0 && handle.value <= compute_pipeline_handles_.size() &&
               handle.value <= compute_pipeline_active_.size() && compute_pipeline_active_.at(handle.value - 1U);
    }

    [[nodiscard]] DescriptorSetLayoutHandle descriptor_set_layout_for_set(DescriptorSetHandle handle) const {
        if (!owns_descriptor_set(handle)) {
            throw std::invalid_argument("d3d12 rhi descriptor set handle is unknown");
        }
        return descriptor_sets_.at(handle.value - 1U).layout;
    }

    [[nodiscard]] static std::uint32_t descriptor_capacity(const DescriptorSetLayoutDesc& desc,
                                                           NativeDescriptorHeapKind kind) {
        std::uint32_t capacity = 0;
        for (const auto& binding : desc.bindings) {
            if (!descriptor_type_matches_heap(binding.type, kind)) {
                continue;
            }
            if (binding.count > std::numeric_limits<std::uint32_t>::max() - capacity) {
                throw std::invalid_argument("d3d12 rhi descriptor set layout is too large");
            }
            capacity += binding.count;
        }
        return capacity;
    }

    [[nodiscard]] NativeDescriptorHeapHandle ensure_descriptor_arena(NativeDescriptorHeapKind kind,
                                                                     std::uint32_t requested_capacity) {
        if (requested_capacity == 0) {
            throw std::invalid_argument("d3d12 rhi descriptor set layout must reserve descriptors");
        }

        auto& heap =
            kind == NativeDescriptorHeapKind::sampler ? sampler_descriptor_arena_heap_ : descriptor_arena_heap_;
        auto& capacity =
            kind == NativeDescriptorHeapKind::sampler ? sampler_descriptor_arena_capacity_ : descriptor_arena_capacity_;
        auto& next =
            kind == NativeDescriptorHeapKind::sampler ? sampler_descriptor_arena_next_ : descriptor_arena_next_;
        if (heap.value == 0) {
            const auto default_capacity = kind == NativeDescriptorHeapKind::sampler
                                              ? default_sampler_descriptor_arena_capacity
                                              : default_descriptor_arena_capacity;
            capacity = std::max(default_capacity, requested_capacity);
            heap = context_->create_descriptor_heap(NativeDescriptorHeapDesc{
                .kind = kind,
                .capacity = capacity,
                .shader_visible = true,
            });
        }
        if (heap.value == 0 || next > capacity || requested_capacity > capacity - next) {
            throw std::invalid_argument("d3d12 rhi descriptor arena is exhausted");
        }

        return heap;
    }

    [[nodiscard]] static const DescriptorBindingDesc& descriptor_binding(const DescriptorSetLayoutDesc& layout,
                                                                         std::uint32_t binding_index) {
        const auto found =
            std::ranges::find_if(layout.bindings, [binding_index](const DescriptorBindingDesc& candidate) {
                return candidate.binding == binding_index;
            });
        if (found == layout.bindings.end()) {
            throw std::invalid_argument("d3d12 rhi descriptor binding is not declared by the set layout");
        }
        return *found;
    }

    [[nodiscard]] static std::uint32_t descriptor_binding_offset(const DescriptorSetLayoutDesc& layout,
                                                                 std::uint32_t binding_index,
                                                                 NativeDescriptorHeapKind kind) {
        std::uint32_t offset = 0;
        for (const auto& binding : layout.bindings) {
            if (binding.binding == binding_index) {
                return offset;
            }
            if (descriptor_type_matches_heap(binding.type, kind)) {
                offset += binding.count;
            }
        }
        throw std::invalid_argument("d3d12 rhi descriptor binding is not declared by the set layout");
    }

    [[nodiscard]] static std::vector<D3d12DescriptorSetRootTables>
    make_pipeline_layout_descriptor_tables(const std::vector<DescriptorSetLayoutDesc>& descriptor_sets) {
        std::vector<D3d12DescriptorSetRootTables> tables;
        tables.reserve(descriptor_sets.size());
        std::uint32_t root_parameter_index = 0;
        for (const auto& descriptor_set : descriptor_sets) {
            D3d12DescriptorSetRootTables set_tables;
            if (descriptor_set_has_heap_kind(descriptor_set, NativeDescriptorHeapKind::cbv_srv_uav)) {
                set_tables.cbv_srv_uav = root_parameter_index++;
            }
            if (descriptor_set_has_heap_kind(descriptor_set, NativeDescriptorHeapKind::sampler)) {
                set_tables.sampler = root_parameter_index++;
            }
            tables.push_back(set_tables);
        }
        return tables;
    }

    [[nodiscard]] static bool is_buffer_descriptor(DescriptorType type) noexcept {
        return type == DescriptorType::uniform_buffer || type == DescriptorType::storage_buffer;
    }

    [[nodiscard]] static bool is_texture_descriptor(DescriptorType type) noexcept {
        return type == DescriptorType::sampled_texture || type == DescriptorType::storage_texture;
    }

    [[nodiscard]] static bool is_sampler_descriptor(DescriptorType type) noexcept {
        return type == DescriptorType::sampler;
    }

    void validate_descriptor_resource(const DescriptorResource& resource, DescriptorType type) const {
        if (is_buffer_descriptor(type)) {
            if (!owns_buffer(resource.buffer_handle)) {
                throw std::invalid_argument("d3d12 rhi descriptor buffer handle is unknown");
            }
            const auto usage = buffer_descs_.at(resource.buffer_handle.value - 1U).usage;
            if (type == DescriptorType::uniform_buffer && !has_flag(usage, BufferUsage::uniform)) {
                throw std::invalid_argument("d3d12 rhi uniform buffer descriptor requires uniform buffer usage");
            }
            if (type == DescriptorType::storage_buffer && !has_flag(usage, BufferUsage::storage)) {
                throw std::invalid_argument("d3d12 rhi storage buffer descriptor requires storage buffer usage");
            }
            return;
        }

        if (is_texture_descriptor(type)) {
            if (!owns_texture(resource.texture_handle)) {
                throw std::invalid_argument("d3d12 rhi descriptor texture handle is unknown");
            }
            const auto usage = texture_descs_.at(resource.texture_handle.value - 1U).usage;
            if (type == DescriptorType::sampled_texture && !has_flag(usage, TextureUsage::shader_resource)) {
                throw std::invalid_argument(
                    "d3d12 rhi sampled texture descriptor requires shader_resource texture usage");
            }
            if (type == DescriptorType::storage_texture && !has_flag(usage, TextureUsage::storage)) {
                throw std::invalid_argument("d3d12 rhi storage texture descriptor requires storage texture usage");
            }
            return;
        }

        if (is_sampler_descriptor(type)) {
            if (!owns_sampler(resource.sampler_handle)) {
                throw std::invalid_argument("d3d12 rhi descriptor sampler handle is unknown");
            }
            return;
        }

        throw std::invalid_argument("d3d12 rhi descriptor resource type is invalid");
    }

    [[nodiscard]] NativeResourceHandle native_resource_handle(const DescriptorResource& resource,
                                                              DescriptorType type) const {
        if (is_buffer_descriptor(type)) {
            return buffer_handles_.at(resource.buffer_handle.value - 1U);
        }
        if (is_texture_descriptor(type)) {
            return texture_handles_.at(resource.texture_handle.value - 1U);
        }
        throw std::invalid_argument("d3d12 rhi descriptor resource type is invalid");
    }

    static constexpr std::uint32_t default_descriptor_arena_capacity{4096};
    static constexpr std::uint32_t default_sampler_descriptor_arena_capacity{2048};

    std::unique_ptr<DeviceContext> context_;
    RhiStats stats_{};
    std::vector<NativeResourceHandle> buffer_handles_;
    std::vector<BufferDesc> buffer_descs_;
    std::vector<bool> buffer_active_;
    std::vector<RhiResourceHandle> buffer_lifetime_;
    std::vector<NativeResourceHandle> texture_handles_;
    std::vector<TextureDesc> texture_descs_;
    std::vector<bool> texture_active_;
    std::vector<RhiResourceHandle> texture_lifetime_;
    std::vector<ResourceState> texture_states_;
    std::vector<bool> sampler_active_;
    std::vector<RhiResourceHandle> sampler_lifetime_;
    std::vector<bool> shader_active_;
    std::vector<RhiResourceHandle> shader_lifetime_;
    std::vector<bool> descriptor_set_layout_active_;
    std::vector<RhiResourceHandle> descriptor_set_layout_lifetime_;
    std::vector<bool> descriptor_set_active_;
    std::vector<RhiResourceHandle> descriptor_set_lifetime_;
    std::vector<bool> pipeline_layout_active_;
    std::vector<RhiResourceHandle> pipeline_layout_lifetime_;
    std::vector<bool> graphics_pipeline_active_;
    std::vector<RhiResourceHandle> graphics_pipeline_lifetime_;
    std::vector<bool> compute_pipeline_active_;
    std::vector<RhiResourceHandle> compute_pipeline_lifetime_;
    std::vector<SamplerDesc> sampler_descs_;
    std::vector<NativeSwapchainHandle> swapchain_handles_;
    std::vector<SwapchainDesc> swapchain_descs_;
    std::vector<ResourceState> swapchain_states_;
    std::vector<bool> swapchain_presentable_;
    std::vector<bool> swapchain_frame_reserved_;
    std::vector<SwapchainHandle> swapchain_frame_swapchains_;
    std::vector<bool> swapchain_frame_active_;
    std::vector<bool> swapchain_frame_presented_;
    std::vector<NativeShaderHandle> shader_handles_;
    std::vector<ShaderStage> shader_stages_;
    std::vector<DescriptorSetLayoutDesc> descriptor_set_layouts_;
    std::vector<DescriptorSetRecord> descriptor_sets_;
    NativeDescriptorHeapHandle descriptor_arena_heap_;
    std::uint32_t descriptor_arena_capacity_{0};
    std::uint32_t descriptor_arena_next_{0};
    NativeDescriptorHeapHandle sampler_descriptor_arena_heap_;
    std::uint32_t sampler_descriptor_arena_capacity_{0};
    std::uint32_t sampler_descriptor_arena_next_{0};
    std::vector<NativeDescriptorHeapHandle> descriptor_set_cbv_srv_uav_heaps_;
    std::vector<std::uint32_t> descriptor_set_cbv_srv_uav_base_descriptors_;
    std::vector<NativeDescriptorHeapHandle> descriptor_set_sampler_heaps_;
    std::vector<std::uint32_t> descriptor_set_sampler_base_descriptors_;
    std::vector<DescriptorSetLayoutHandle> descriptor_set_layout_handles_;
    std::vector<NativeRootSignatureHandle> pipeline_layout_handles_;
    std::vector<std::vector<DescriptorSetLayoutHandle>> pipeline_layout_descriptor_sets_;
    std::vector<std::vector<D3d12DescriptorSetRootTables>> pipeline_layout_descriptor_tables_;
    std::vector<NativeGraphicsPipelineHandle> graphics_pipeline_handles_;
    std::vector<NativeRootSignatureHandle> graphics_pipeline_root_signatures_;
    std::vector<PipelineLayoutHandle> graphics_pipeline_layouts_;
    std::vector<NativeComputePipelineHandle> compute_pipeline_handles_;
    std::vector<NativeRootSignatureHandle> compute_pipeline_root_signatures_;
    std::vector<PipelineLayoutHandle> compute_pipeline_layouts_;
    std::vector<TransientLeaseRecord> transient_leases_;
    std::uint32_t next_transient_resource_{1};
    FenceValue last_submitted_fence_{};
    RhiResourceLifetimeRegistry resource_lifetime_{};
};

void D3d12RhiDevice::retire_deferred_committed_resources(std::uint64_t completed_frame) noexcept {
    if (!context_) {
        return;
    }

    struct PendingDestroy {
        RhiResourceHandle handle{};
        RhiResourceKind kind{RhiResourceKind::unknown};
        int rank{99};
    };
    std::vector<PendingDestroy> pending;
    pending.reserve(32);
    for (const auto& rec : resource_lifetime_.records()) {
        if (rec.state != RhiResourceLifetimeState::deferred_release || rec.release_frame > completed_frame) {
            continue;
        }
        pending.push_back(
            PendingDestroy{.handle = rec.handle, .kind = rec.kind, .rank = deferred_d3d12_destroy_rank(rec.kind)});
    }
    if (pending.empty()) {
        (void)resource_lifetime_.retire_released_resources(completed_frame);
        return;
    }
    std::ranges::stable_sort(pending, [](const PendingDestroy& lhs, const PendingDestroy& rhs) {
        if (lhs.rank != rhs.rank) {
            return lhs.rank < rhs.rank;
        }
        return lhs.handle.id.value < rhs.handle.id.value;
    });

    for (const auto& item : pending) {
        switch (item.kind) {
        case RhiResourceKind::buffer: {
            for (std::size_t i = 0; i < buffer_lifetime_.size(); ++i) {
                if (buffer_lifetime_[i] != item.handle) {
                    continue;
                }
                if (i < buffer_handles_.size()) {
                    context_->destroy_committed_resource(buffer_handles_[i]);
                }
                if (i < buffer_active_.size()) {
                    buffer_active_[i] = false;
                }
                break;
            }
        } break;
        case RhiResourceKind::texture: {
            for (std::size_t i = 0; i < texture_lifetime_.size(); ++i) {
                if (texture_lifetime_[i] != item.handle) {
                    continue;
                }
                if (i < texture_handles_.size()) {
                    const auto placed_alive_before = context_->stats().placed_resources_alive;
                    context_->destroy_committed_resource(texture_handles_[i]);
                    const auto placed_alive_after = context_->stats().placed_resources_alive;
                    if (placed_alive_after < placed_alive_before &&
                        stats_.transient_texture_placed_resources_alive > 0) {
                        --stats_.transient_texture_placed_resources_alive;
                    }
                }
                if (i < texture_active_.size()) {
                    texture_active_[i] = false;
                }
                if (i < texture_states_.size()) {
                    texture_states_[i] = ResourceState::undefined;
                }
                break;
            }
        } break;
        case RhiResourceKind::sampler: {
            for (std::size_t i = 0; i < sampler_lifetime_.size(); ++i) {
                if (sampler_lifetime_[i] != item.handle) {
                    continue;
                }
                if (i < sampler_active_.size()) {
                    sampler_active_[i] = false;
                }
                break;
            }
        } break;
        case RhiResourceKind::shader: {
            for (std::size_t i = 0; i < shader_lifetime_.size(); ++i) {
                if (shader_lifetime_[i] != item.handle) {
                    continue;
                }
                if (i < shader_handles_.size() && i < shader_active_.size() && shader_active_[i]) {
                    context_->destroy_shader_module(shader_handles_[i]);
                    shader_active_[i] = false;
                }
                break;
            }
        } break;
        case RhiResourceKind::descriptor_set_layout: {
            for (std::size_t i = 0; i < descriptor_set_layout_lifetime_.size(); ++i) {
                if (descriptor_set_layout_lifetime_[i] != item.handle) {
                    continue;
                }
                if (i < descriptor_set_layout_active_.size()) {
                    descriptor_set_layout_active_[i] = false;
                }
                break;
            }
        } break;
        case RhiResourceKind::descriptor_set: {
            for (std::size_t i = 0; i < descriptor_set_lifetime_.size(); ++i) {
                if (descriptor_set_lifetime_[i] != item.handle) {
                    continue;
                }
                if (i < descriptor_set_active_.size()) {
                    descriptor_set_active_[i] = false;
                }
                break;
            }
        } break;
        case RhiResourceKind::pipeline_layout: {
            for (std::size_t i = 0; i < pipeline_layout_lifetime_.size(); ++i) {
                if (pipeline_layout_lifetime_[i] != item.handle) {
                    continue;
                }
                if (i < pipeline_layout_handles_.size() && i < pipeline_layout_active_.size() &&
                    pipeline_layout_active_[i]) {
                    context_->destroy_root_signature(pipeline_layout_handles_[i]);
                    pipeline_layout_active_[i] = false;
                }
                break;
            }
        } break;
        case RhiResourceKind::graphics_pipeline: {
            for (std::size_t i = 0; i < graphics_pipeline_lifetime_.size(); ++i) {
                if (graphics_pipeline_lifetime_[i] != item.handle) {
                    continue;
                }
                if (i < graphics_pipeline_handles_.size() && i < graphics_pipeline_active_.size() &&
                    graphics_pipeline_active_[i]) {
                    context_->destroy_graphics_pipeline(graphics_pipeline_handles_[i]);
                    graphics_pipeline_active_[i] = false;
                }
                break;
            }
        } break;
        case RhiResourceKind::compute_pipeline: {
            for (std::size_t i = 0; i < compute_pipeline_lifetime_.size(); ++i) {
                if (compute_pipeline_lifetime_[i] != item.handle) {
                    continue;
                }
                if (i < compute_pipeline_handles_.size() && i < compute_pipeline_active_.size() &&
                    compute_pipeline_active_[i]) {
                    context_->destroy_compute_pipeline(compute_pipeline_handles_[i]);
                    compute_pipeline_active_[i] = false;
                }
                break;
            }
        } break;
        default:
            break;
        }
    }

    (void)resource_lifetime_.retire_released_resources(completed_frame);
}

D3d12RhiDevice::~D3d12RhiDevice() {
    if (!context_ || !context_->valid()) {
        return;
    }

    if (last_submitted_fence_.value > 0) {
        (void)context_->wait_for_fence(last_submitted_fence_, 0xFFFFFFFFU);
    }

    const auto flush_fence = std::numeric_limits<std::uint64_t>::max();
    for (std::size_t i = 0; i < graphics_pipeline_active_.size(); ++i) {
        if (graphics_pipeline_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(graphics_pipeline_lifetime_[i], 0);
            graphics_pipeline_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < compute_pipeline_active_.size(); ++i) {
        if (compute_pipeline_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(compute_pipeline_lifetime_[i], 0);
            compute_pipeline_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < pipeline_layout_active_.size(); ++i) {
        if (pipeline_layout_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(pipeline_layout_lifetime_[i], 0);
            pipeline_layout_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < descriptor_set_active_.size(); ++i) {
        if (descriptor_set_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(descriptor_set_lifetime_[i], 0);
            descriptor_set_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < descriptor_set_layout_active_.size(); ++i) {
        if (descriptor_set_layout_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(descriptor_set_layout_lifetime_[i], 0);
            descriptor_set_layout_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < shader_active_.size(); ++i) {
        if (shader_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(shader_lifetime_[i], 0);
            shader_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < sampler_active_.size(); ++i) {
        if (sampler_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(sampler_lifetime_[i], 0);
            sampler_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < buffer_active_.size(); ++i) {
        if (buffer_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(buffer_lifetime_[i], 0);
            buffer_active_[i] = false;
        }
    }
    for (std::size_t i = 0; i < texture_active_.size(); ++i) {
        if (texture_active_[i]) {
            (void)resource_lifetime_.release_resource_deferred(texture_lifetime_[i], 0);
            texture_active_[i] = false;
        }
    }
    retire_deferred_committed_resources(flush_fence);
}

std::unique_ptr<IRhiDevice> create_rhi_device(const DeviceBootstrapDesc& desc) {
    auto context = DeviceContext::create(desc);
    if (context == nullptr || !context->valid()) {
        return nullptr;
    }

    return std::make_unique<D3d12RhiDevice>(std::move(context));
}

QueueCalibratedTiming measure_rhi_device_calibrated_queue_timing(IRhiDevice& device, QueueKind queue) {
    auto* d3d12_device = dynamic_cast<D3d12RhiDevice*>(&device);
    if (d3d12_device == nullptr) {
        QueueCalibratedTiming result{};
        result.queue = queue;
        result.status = QueueCalibratedTimingStatus::unsupported;
        result.diagnostic = "d3d12 calibrated queue timing requires a d3d12 rhi device";
        return result;
    }

    return d3d12_device->measure_calibrated_queue_timing(queue);
}

SubmittedCommandCalibratedTiming read_rhi_device_submitted_command_calibrated_timing(IRhiDevice& device,
                                                                                     FenceValue fence) {
    auto* d3d12_device = dynamic_cast<D3d12RhiDevice*>(&device);
    if (d3d12_device == nullptr) {
        SubmittedCommandCalibratedTiming result{};
        result.queue = fence.queue;
        result.submitted_fence = fence;
        result.status = SubmittedCommandCalibratedTimingStatus::unsupported;
        result.diagnostic = "d3d12 submitted command calibrated timing requires a d3d12 rhi device";
        return result;
    }

    return d3d12_device->read_submitted_command_calibrated_timing(fence);
}

QueueCalibratedOverlapDiagnostics
diagnose_rhi_device_submitted_command_compute_graphics_overlap(IRhiDevice& device,
                                                               const RhiAsyncOverlapReadinessDiagnostics& schedule,
                                                               FenceValue compute_fence, FenceValue graphics_fence) {
    const auto compute_timing = read_rhi_device_submitted_command_calibrated_timing(device, compute_fence);
    const auto graphics_timing = read_rhi_device_submitted_command_calibrated_timing(device, graphics_fence);
    return diagnose_calibrated_compute_graphics_overlap(schedule, compute_timing, graphics_timing);
}

D3d12SharedTextureExportResult export_shared_texture(IRhiDevice& device, TextureHandle texture) noexcept {
    auto* d3d12_device = dynamic_cast<D3d12RhiDevice*>(&device);
    if (d3d12_device == nullptr) {
        D3d12SharedTextureExportResult result{};
        result.device_unavailable = true;
        return result;
    }

    return d3d12_device->export_shared_texture(texture);
}

void close_shared_texture_handle(D3d12SharedTextureHandle handle) noexcept {
    if (handle.value != 0) {
        CloseHandle(reinterpret_cast<HANDLE>(handle.value));
    }
}

BackendKind backend_kind() noexcept {
    return BackendKind::d3d12;
}

std::string_view backend_name() noexcept {
    return "d3d12";
}

bool compiled_with_windows_sdk() noexcept {
    return D3D12_SDK_VERSION > 0 && DXGI_MAX_SWAP_CHAIN_BUFFERS >= 2;
}

RuntimeProbe probe_runtime() noexcept {
    RuntimeProbe probe;
    probe.windows_sdk_available = compiled_with_windows_sdk();

    Microsoft::WRL::ComPtr<ID3D12Debug> debug;
    probe.debug_layer_available = SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));

    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
        return probe;
    }

    probe.dxgi_factory_created = true;

    for (std::uint32_t adapter_index = 0;; ++adapter_index) {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        const HRESULT result = factory->EnumAdapters1(adapter_index, &adapter);
        if (result == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(result)) {
            continue;
        }

        ++probe.adapter_count;

        DXGI_ADAPTER_DESC1 desc{};
        if (FAILED(adapter->GetDesc1(&desc))) {
            continue;
        }
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0U) {
            continue;
        }

        ++probe.hardware_adapter_count;
        if (adapter_supports_d3d12(adapter.Get())) {
            probe.hardware_device_supported = true;
        }
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter> warp_adapter;
    if (SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter)))) {
        probe.warp_adapter_available = true;
        probe.warp_device_supported = adapter_supports_d3d12(warp_adapter.Get());
    }

    return probe;
}

DeviceBootstrapResult bootstrap_device(const DeviceBootstrapDesc& desc) noexcept {
    DeviceBootstrapResult result;
    result.initial_fence_value = 0;

    if (desc.enable_debug_layer) {
        result.debug_layer_enabled = enable_debug_layer();
    }

    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
        return result;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter1> hardware_adapter;
    Microsoft::WRL::ComPtr<IDXGIAdapter> warp_adapter;
    IUnknown* selected_adapter =
        select_adapter(factory.Get(), desc.prefer_warp, hardware_adapter, warp_adapter, result.used_warp);

    if (selected_adapter == nullptr) {
        return result;
    }

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    if (FAILED(D3D12CreateDevice(selected_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) {
        return result;
    }
    result.device_created = true;

    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.NodeMask = 0;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
    if (FAILED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)))) {
        return result;
    }
    result.command_queue_created = true;

    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    if (FAILED(device->CreateFence(result.initial_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
        return result;
    }
    result.fence_created = true;
    result.succeeded = true;
    return result;
}

ResourceOwnershipResult bootstrap_resource_ownership(const ResourceOwnershipDesc& desc) noexcept {
    ResourceOwnershipResult result;
    if (!valid_resource_ownership_desc(desc)) {
        result.validation_failed = true;
        return result;
    }

    if (desc.device.enable_debug_layer) {
        (void)enable_debug_layer();
    }

    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
        return result;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter1> hardware_adapter;
    Microsoft::WRL::ComPtr<IDXGIAdapter> warp_adapter;
    bool used_warp = false;
    IUnknown* selected_adapter =
        select_adapter(factory.Get(), desc.device.prefer_warp, hardware_adapter, warp_adapter, used_warp);
    if (selected_adapter == nullptr) {
        return result;
    }

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    if (FAILED(D3D12CreateDevice(selected_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) {
        return result;
    }
    result.device_created = true;

    const auto upload_heap = heap_properties(D3D12_HEAP_TYPE_UPLOAD);
    const auto upload_desc = buffer_resource_desc(desc.upload_buffer_size_bytes);
    Microsoft::WRL::ComPtr<ID3D12Resource> upload_buffer;
    if (FAILED(device->CreateCommittedResource(&upload_heap, D3D12_HEAP_FLAG_NONE, &upload_desc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(&upload_buffer)))) {
        return result;
    }
    result.upload_buffer_created = true;
    d3d12_set_object_name(upload_buffer.Get(), L"GameEngine.RHI.D3D12.Bootstrap.UploadBuffer");

    const auto readback_heap = heap_properties(D3D12_HEAP_TYPE_READBACK);
    const auto readback_desc = buffer_resource_desc(desc.readback_buffer_size_bytes);
    Microsoft::WRL::ComPtr<ID3D12Resource> readback_buffer;
    if (FAILED(device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &readback_desc,
                                               D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                               IID_PPV_ARGS(&readback_buffer)))) {
        return result;
    }
    result.readback_buffer_created = true;
    d3d12_set_object_name(readback_buffer.Get(), L"GameEngine.RHI.D3D12.Bootstrap.ReadbackBuffer");

    const auto texture_desc = texture_resource_desc(desc.texture_extent, desc.texture_format);
    const auto allocation = device->GetResourceAllocationInfo(0, 1, &texture_desc);
    result.texture_allocation_size_bytes = allocation.SizeInBytes;

    const auto default_heap = heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    Microsoft::WRL::ComPtr<ID3D12Resource> default_texture;
    if (FAILED(device->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE, &texture_desc,
                                               D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                               IID_PPV_ARGS(&default_texture)))) {
        return result;
    }
    result.default_texture_created = true;
    d3d12_set_object_name(default_texture.Get(), L"GameEngine.RHI.D3D12.Bootstrap.DefaultTexture");
    result.succeeded = true;
    return result;
}

} // namespace mirakana::rhi::d3d12
