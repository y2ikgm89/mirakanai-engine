// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/d3d12/d3d12_mavg_gpu_culling_dispatch.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace mirakana::rhi::d3d12 {
namespace {

constexpr std::uint32_t k_indirect_count_buffer_size_bytes = 4U;

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_mavg_gpu_culling_compute_shader() {
    static constexpr const char* source =
        "struct ClusterRow {"
        "  uint index_count_per_instance;"
        "  uint instance_count;"
        "  uint start_index_location;"
        "  int base_vertex_location;"
        "  uint start_instance_location;"
        "  uint visible;"
        "  uint padding0;"
        "  uint padding1;"
        "};"
        "StructuredBuffer<ClusterRow> cluster_rows : register(t0);"
        "RWByteAddressBuffer argument_buffer : register(u0);"
        "RWByteAddressBuffer count_buffer : register(u1);"
        "cbuffer Constants : register(b0) {"
        "  uint cluster_count;"
        "  uint max_command_count;"
        "  uint record_stride_bytes;"
        "  uint _pad;"
        "};"
        "[numthreads(1, 1, 1)]"
        "void cs_main(uint3 dispatch_id : SV_DispatchThreadID) {"
        "  if (dispatch_id.x != 0u) { return; }"
        "  uint write_slot = 0u;"
        "  for (uint cluster_index = 0u; cluster_index < cluster_count; ++cluster_index) {"
        "    ClusterRow row = cluster_rows[cluster_index];"
        "    if (row.visible == 0u) { continue; }"
        "    if (write_slot >= max_command_count) { break; }"
        "    const uint write_offset = write_slot * record_stride_bytes;"
        "    argument_buffer.Store(write_offset + 0u, row.index_count_per_instance);"
        "    argument_buffer.Store(write_offset + 4u, row.instance_count);"
        "    argument_buffer.Store(write_offset + 8u, row.start_index_location);"
        "    argument_buffer.Store(write_offset + 12u, asuint(row.base_vertex_location));"
        "    argument_buffer.Store(write_offset + 16u, row.start_instance_location);"
        "    write_slot += 1u;"
        "  }"
        "  count_buffer.Store(0u, write_slot);"
        "}";

    Microsoft::WRL::ComPtr<ID3DBlob> bytecode;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    const HRESULT result = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, "cs_main", "cs_5_0",
                                      D3DCOMPILE_ENABLE_STRICTNESS, 0, &bytecode, &errors);
    if (FAILED(result)) {
        if (errors != nullptr) {
            OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
        }
        return nullptr;
    }
    return bytecode;
}

[[nodiscard]] D3D12_HEAP_PROPERTIES default_heap_properties() noexcept {
    D3D12_HEAP_PROPERTIES properties{};
    properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;
    return properties;
}

[[nodiscard]] D3D12_HEAP_PROPERTIES upload_heap_properties() noexcept {
    D3D12_HEAP_PROPERTIES properties{};
    properties.Type = D3D12_HEAP_TYPE_UPLOAD;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;
    return properties;
}

[[nodiscard]] D3D12_HEAP_PROPERTIES readback_heap_properties() noexcept {
    D3D12_HEAP_PROPERTIES properties{};
    properties.Type = D3D12_HEAP_TYPE_READBACK;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;
    return properties;
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

[[nodiscard]] bool wait_for_fence(ID3D12Fence* fence, std::uint64_t fence_value, HANDLE event_handle) noexcept {
    if (fence == nullptr || event_handle == nullptr) {
        return false;
    }
    if (fence->GetCompletedValue() < fence_value) {
        if (FAILED(fence->SetEventOnCompletion(fence_value, event_handle))) {
            return false;
        }
        WaitForSingleObject(event_handle, INFINITE);
    }
    return fence->GetCompletedValue() >= fence_value;
}

[[nodiscard]] bool valid_dispatch_desc(const D3d12MavgGpuCullingDispatchDesc& desc) noexcept {
    return desc.max_command_count > 0U && desc.record_stride_bytes >= 20U && (desc.record_stride_bytes % 4U) == 0U &&
           !desc.cluster_rows.empty();
}

[[nodiscard]] std::uint32_t
visible_cluster_count(std::span<const D3d12MavgGpuCullingDispatchClusterRow> rows) noexcept {
    return static_cast<std::uint32_t>(std::ranges::count_if(
        rows, [](const D3d12MavgGpuCullingDispatchClusterRow& row) { return row.visible != 0U; }));
}

[[nodiscard]] std::uint32_t culled_cluster_count(std::span<const D3d12MavgGpuCullingDispatchClusterRow> rows) noexcept {
    return static_cast<std::uint32_t>(rows.size()) - visible_cluster_count(rows);
}

[[nodiscard]] Microsoft::WRL::ComPtr<IDXGIAdapter> select_adapter(IDXGIFactory6* factory, bool prefer_warp) {
    Microsoft::WRL::ComPtr<IDXGIAdapter1> hardware_adapter;
    for (UINT adapter_index = 0; factory->EnumAdapters1(adapter_index, &hardware_adapter) != DXGI_ERROR_NOT_FOUND;
         ++adapter_index) {
        DXGI_ADAPTER_DESC1 adapter_desc{};
        if (SUCCEEDED(hardware_adapter->GetDesc1(&adapter_desc)) &&
            (adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) {
            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
            if (SUCCEEDED(hardware_adapter.As(&adapter))) {
                return adapter;
            }
        }
        hardware_adapter.Reset();
    }

    if (!prefer_warp) {
        return nullptr;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter> warp_adapter;
    if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter)))) {
        return nullptr;
    }
    return warp_adapter;
}

[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE gpu_descriptor_handle(D3D12_GPU_DESCRIPTOR_HANDLE base,
                                                                std::uint32_t descriptor_index,
                                                                std::uint32_t descriptor_size) noexcept {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = base;
    handle.ptr += static_cast<SIZE_T>(descriptor_index) * static_cast<SIZE_T>(descriptor_size);
    return handle;
}

[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle(D3D12_CPU_DESCRIPTOR_HANDLE base,
                                                                std::uint32_t descriptor_index,
                                                                std::uint32_t descriptor_size) noexcept {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = base;
    handle.ptr += static_cast<SIZE_T>(descriptor_index) * static_cast<SIZE_T>(descriptor_size);
    return handle;
}

[[nodiscard]] bool create_root_signature(ID3D12Device* device,
                                         Microsoft::WRL::ComPtr<ID3D12RootSignature>& root_signature) {
    std::array<D3D12_DESCRIPTOR_RANGE1, 3> descriptor_ranges{};
    descriptor_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_ranges[0].NumDescriptors = 1;
    descriptor_ranges[0].BaseShaderRegister = 0;
    descriptor_ranges[0].RegisterSpace = 0;
    descriptor_ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
    descriptor_ranges[0].OffsetInDescriptorsFromTableStart = 0;

    descriptor_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptor_ranges[1].NumDescriptors = 1;
    descriptor_ranges[1].BaseShaderRegister = 0;
    descriptor_ranges[1].RegisterSpace = 0;
    descriptor_ranges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
    descriptor_ranges[1].OffsetInDescriptorsFromTableStart = 1;

    descriptor_ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptor_ranges[2].NumDescriptors = 1;
    descriptor_ranges[2].BaseShaderRegister = 1;
    descriptor_ranges[2].RegisterSpace = 0;
    descriptor_ranges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
    descriptor_ranges[2].OffsetInDescriptorsFromTableStart = 2;

    std::array<D3D12_ROOT_PARAMETER1, 2> root_parameters{};
    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    root_parameters[0].Descriptor.ShaderRegister = 0;
    root_parameters[0].Descriptor.RegisterSpace = 0;
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[1].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(descriptor_ranges.size());
    root_parameters[1].DescriptorTable.pDescriptorRanges = descriptor_ranges.data();
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc{};
    root_signature_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    root_signature_desc.Desc_1_1.NumParameters = static_cast<UINT>(root_parameters.size());
    root_signature_desc.Desc_1_1.pParameters = root_parameters.data();
    root_signature_desc.Desc_1_1.NumStaticSamplers = 0;
    root_signature_desc.Desc_1_1.pStaticSamplers = nullptr;
    root_signature_desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    Microsoft::WRL::ComPtr<ID3DBlob> signature_blob;
    Microsoft::WRL::ComPtr<ID3DBlob> error_blob;
    if (FAILED(D3D12SerializeVersionedRootSignature(&root_signature_desc, &signature_blob, &error_blob))) {
        return false;
    }

    return SUCCEEDED(device->CreateRootSignature(0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(),
                                                 IID_PPV_ARGS(&root_signature)));
}

[[nodiscard]] bool create_compute_pipeline_state(ID3D12Device* device, ID3D12RootSignature* root_signature,
                                                 ID3DBlob* shader_bytecode,
                                                 Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipeline_state) {
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc{};
    pipeline_desc.pRootSignature = root_signature;
    pipeline_desc.CS = {shader_bytecode->GetBufferPointer(), shader_bytecode->GetBufferSize()};
    return SUCCEEDED(device->CreateComputePipelineState(&pipeline_desc, IID_PPV_ARGS(&pipeline_state)));
}

void transition_resource(ID3D12GraphicsCommandList* command_list, ID3D12Resource* resource,
                         D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, std::uint32_t& barrier_count) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    command_list->ResourceBarrier(1, &barrier);
    ++barrier_count;
}

void uav_barrier(ID3D12GraphicsCommandList* command_list, ID3D12Resource* resource, std::uint32_t& barrier_count) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = resource;
    command_list->ResourceBarrier(1, &barrier);
    ++barrier_count;
}

[[nodiscard]] std::vector<std::uint8_t> readback_buffer_bytes(ID3D12Resource* readback, std::uint64_t size_bytes) {
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size_bytes));
    if (readback == nullptr || size_bytes == 0U) {
        return bytes;
    }

    void* mapped = nullptr;
    D3D12_RANGE read_range{.Begin = 0, .End = static_cast<SIZE_T>(size_bytes)};
    if (FAILED(readback->Map(0, &read_range, &mapped)) || mapped == nullptr) {
        bytes.clear();
        return bytes;
    }

    std::memcpy(bytes.data(), mapped, static_cast<std::size_t>(size_bytes));
    const D3D12_RANGE empty_range{};
    readback->Unmap(0, &empty_range);
    return bytes;
}

} // namespace

D3d12MavgGpuCullingDispatchResult
dispatch_mavg_gpu_culling_indirect(const D3d12MavgGpuCullingDispatchDesc& desc) noexcept {
    D3d12MavgGpuCullingDispatchResult result{};
    result.visible_cluster_count = visible_cluster_count(desc.cluster_rows);
    result.culled_cluster_count = culled_cluster_count(desc.cluster_rows);
    const auto argument_allocation_bytes =
        static_cast<std::uint64_t>(desc.max_command_count) * static_cast<std::uint64_t>(desc.record_stride_bytes);
    result.argument_buffer_size_bytes =
        static_cast<std::uint64_t>(result.visible_cluster_count) * static_cast<std::uint64_t>(desc.record_stride_bytes);

    if (!valid_dispatch_desc(desc) || result.visible_cluster_count == 0U) {
        result.failure_stage = 1U;
        return result;
    }
    if (result.visible_cluster_count > desc.max_command_count) {
        result.failure_stage = 2U;
        return result;
    }

    const auto shader_bytecode = compile_mavg_gpu_culling_compute_shader();
    if (shader_bytecode == nullptr) {
        result.failure_stage = 3U;
        return result;
    }

    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
        return result;
    }

    const auto adapter = select_adapter(factory.Get(), desc.device.prefer_warp);
    if (adapter == nullptr) {
        return result;
    }

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) {
        return result;
    }

    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
    if (FAILED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)))) {
        return result;
    }

    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
        return result;
    }

    HANDLE fence_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (fence_event == nullptr) {
        return result;
    }

    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
    if (!create_root_signature(device.Get(), root_signature) ||
        !create_compute_pipeline_state(device.Get(), root_signature.Get(), shader_bytecode.Get(), pipeline_state)) {
        CloseHandle(fence_event);
        return result;
    }

    const auto cluster_count = static_cast<std::uint32_t>(desc.cluster_rows.size());
    const auto cluster_buffer_size =
        static_cast<std::uint64_t>(cluster_count) * d3d12_mavg_gpu_culling_dispatch_cluster_row_stride_bytes;
    const auto argument_buffer_size = argument_allocation_bytes;
    const auto uav_buffer_flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    Microsoft::WRL::ComPtr<ID3D12Resource> cluster_default_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> cluster_upload_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> argument_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> count_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> constants_upload_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> argument_readback_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> count_readback_buffer;

    const auto default_heap = default_heap_properties();
    const auto upload_heap = upload_heap_properties();
    const auto readback_heap = readback_heap_properties();
    const auto cluster_buffer_desc = buffer_resource_desc(cluster_buffer_size);
    const auto argument_buffer_desc = buffer_resource_desc(argument_buffer_size, uav_buffer_flags);
    const auto count_buffer_desc = buffer_resource_desc(k_indirect_count_buffer_size_bytes, uav_buffer_flags);
    const auto constants_buffer_desc = buffer_resource_desc(256U);
    const auto argument_readback_desc = buffer_resource_desc(argument_buffer_size);
    const auto count_readback_desc = buffer_resource_desc(k_indirect_count_buffer_size_bytes);

    if (FAILED(device->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE, &cluster_buffer_desc,
                                               D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                               IID_PPV_ARGS(&cluster_default_buffer))) ||
        FAILED(device->CreateCommittedResource(&upload_heap, D3D12_HEAP_FLAG_NONE, &cluster_buffer_desc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(&cluster_upload_buffer))) ||
        FAILED(device->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE, &argument_buffer_desc,
                                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
                                               IID_PPV_ARGS(&argument_buffer))) ||
        FAILED(device->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE, &count_buffer_desc,
                                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
                                               IID_PPV_ARGS(&count_buffer))) ||
        FAILED(device->CreateCommittedResource(&upload_heap, D3D12_HEAP_FLAG_NONE, &constants_buffer_desc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(&constants_upload_buffer))) ||
        FAILED(device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &argument_readback_desc,
                                               D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                               IID_PPV_ARGS(&argument_readback_buffer))) ||
        FAILED(device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &count_readback_desc,
                                               D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                               IID_PPV_ARGS(&count_readback_buffer)))) {
        CloseHandle(fence_event);
        return result;
    }

    {
        void* mapped = nullptr;
        if (FAILED(cluster_upload_buffer->Map(0, nullptr, &mapped)) || mapped == nullptr) {
            CloseHandle(fence_event);
            return result;
        }
        std::memcpy(mapped, desc.cluster_rows.data(), static_cast<std::size_t>(cluster_buffer_size));
        cluster_upload_buffer->Unmap(0, nullptr);
    }

    {
        struct Constants {
            std::uint32_t cluster_count;
            std::uint32_t max_command_count;
            std::uint32_t record_stride_bytes;
            std::uint32_t pad;
        };
        Constants constants{
            .cluster_count = cluster_count,
            .max_command_count = desc.max_command_count,
            .record_stride_bytes = desc.record_stride_bytes,
            .pad = 0U,
        };
        void* mapped = nullptr;
        if (FAILED(constants_upload_buffer->Map(0, nullptr, &mapped)) || mapped == nullptr) {
            CloseHandle(fence_event);
            return result;
        }
        std::memcpy(mapped, &constants, sizeof(constants));
        constants_upload_buffer->Unmap(0, nullptr);
    }

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&allocator))) ||
        FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, allocator.Get(), nullptr,
                                         IID_PPV_ARGS(&command_list)))) {
        CloseHandle(fence_event);
        return result;
    }

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = 3;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap;
    if (FAILED(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&descriptor_heap)))) {
        CloseHandle(fence_event);
        return result;
    }

    const auto descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const auto heap_cpu_start = descriptor_heap->GetCPUDescriptorHandleForHeapStart();

    D3D12_SHADER_RESOURCE_VIEW_DESC cluster_srv_desc{};
    cluster_srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    cluster_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    cluster_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    cluster_srv_desc.Buffer.FirstElement = 0;
    cluster_srv_desc.Buffer.NumElements = cluster_count;
    cluster_srv_desc.Buffer.StructureByteStride = d3d12_mavg_gpu_culling_dispatch_cluster_row_stride_bytes;
    cluster_srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    device->CreateShaderResourceView(cluster_default_buffer.Get(), &cluster_srv_desc, heap_cpu_start);

    const auto argument_uav_cpu = cpu_descriptor_handle(heap_cpu_start, 1U, descriptor_size);
    D3D12_UNORDERED_ACCESS_VIEW_DESC argument_uav_desc{};
    argument_uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    argument_uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    argument_uav_desc.Buffer.FirstElement = 0;
    argument_uav_desc.Buffer.NumElements = static_cast<UINT>(argument_buffer_size / 4U);
    argument_uav_desc.Buffer.StructureByteStride = 0;
    argument_uav_desc.Buffer.CounterOffsetInBytes = 0;
    argument_uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    device->CreateUnorderedAccessView(argument_buffer.Get(), nullptr, &argument_uav_desc, argument_uav_cpu);

    const auto count_uav_cpu = cpu_descriptor_handle(heap_cpu_start, 2U, descriptor_size);
    D3D12_UNORDERED_ACCESS_VIEW_DESC count_uav_desc{};
    count_uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    count_uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    count_uav_desc.Buffer.FirstElement = 0;
    count_uav_desc.Buffer.NumElements = 1;
    count_uav_desc.Buffer.StructureByteStride = 0;
    count_uav_desc.Buffer.CounterOffsetInBytes = 0;
    count_uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    device->CreateUnorderedAccessView(count_buffer.Get(), nullptr, &count_uav_desc, count_uav_cpu);

    command_list->CopyBufferRegion(cluster_default_buffer.Get(), 0, cluster_upload_buffer.Get(), 0,
                                   cluster_buffer_size);
    transition_resource(command_list.Get(), cluster_default_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, result.resource_barriers_recorded);

    const auto heap_gpu_start = descriptor_heap->GetGPUDescriptorHandleForHeapStart();
    ID3D12DescriptorHeap* heaps[] = {descriptor_heap.Get()};
    command_list->SetDescriptorHeaps(1, heaps);
    command_list->SetComputeRootSignature(root_signature.Get());
    command_list->SetPipelineState(pipeline_state.Get());
    command_list->SetComputeRootConstantBufferView(0, constants_upload_buffer->GetGPUVirtualAddress());
    command_list->SetComputeRootDescriptorTable(1, heap_gpu_start);

    command_list->Dispatch(1, 1, 1);
    result.compute_dispatches = 1U;

    uav_barrier(command_list.Get(), argument_buffer.Get(), result.resource_barriers_recorded);
    uav_barrier(command_list.Get(), count_buffer.Get(), result.resource_barriers_recorded);
    transition_resource(command_list.Get(), argument_buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, result.resource_barriers_recorded);
    transition_resource(command_list.Get(), count_buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, result.resource_barriers_recorded);

    transition_resource(command_list.Get(), argument_buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
                        D3D12_RESOURCE_STATE_COPY_SOURCE, result.resource_barriers_recorded);
    transition_resource(command_list.Get(), count_buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
                        D3D12_RESOURCE_STATE_COPY_SOURCE, result.resource_barriers_recorded);
    command_list->CopyBufferRegion(argument_readback_buffer.Get(), 0, argument_buffer.Get(), 0, argument_buffer_size);
    command_list->CopyBufferRegion(count_readback_buffer.Get(), 0, count_buffer.Get(), 0,
                                   k_indirect_count_buffer_size_bytes);

    if (FAILED(command_list->Close())) {
        result.failure_stage = 10U;
        CloseHandle(fence_event);
        return result;
    }

    ID3D12CommandList* command_lists[] = {command_list.Get()};
    command_queue->ExecuteCommandLists(1, command_lists);
    const std::uint64_t fence_value = 1U;
    if (FAILED(command_queue->Signal(fence.Get(), fence_value)) ||
        !wait_for_fence(fence.Get(), fence_value, fence_event)) {
        result.failure_stage = 11U;
        CloseHandle(fence_event);
        return result;
    }

    CloseHandle(fence_event);

    result.count_readback_bytes =
        readback_buffer_bytes(count_readback_buffer.Get(), k_indirect_count_buffer_size_bytes);
    if (result.count_readback_bytes.size() != k_indirect_count_buffer_size_bytes) {
        result.failure_stage = 12U;
        return result;
    }

    result.count_buffer_value = static_cast<std::uint32_t>(result.count_readback_bytes[0]) |
                                (static_cast<std::uint32_t>(result.count_readback_bytes[1]) << 8U) |
                                (static_cast<std::uint32_t>(result.count_readback_bytes[2]) << 16U) |
                                (static_cast<std::uint32_t>(result.count_readback_bytes[3]) << 24U);
    result.argument_buffer_size_bytes =
        static_cast<std::uint64_t>(result.count_buffer_value) * static_cast<std::uint64_t>(desc.record_stride_bytes);
    result.argument_readback_bytes =
        readback_buffer_bytes(argument_readback_buffer.Get(), result.argument_buffer_size_bytes);
    if (result.argument_readback_bytes.size() != static_cast<std::size_t>(result.argument_buffer_size_bytes)) {
        result.failure_stage = 13U;
        return result;
    }
    result.executed_gpu_culling = true;
    result.succeeded = true;
    return result;
}

} // namespace mirakana::rhi::d3d12
