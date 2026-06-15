// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/d3d12/d3d12_environment_weather_solver.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace mirakana::rhi::d3d12 {
namespace {

constexpr std::uint64_t fnv_offset_basis = 14695981039346656037ULL;
constexpr std::uint64_t fnv_prime = 1099511628211ULL;

static_assert(sizeof(D3d12EnvironmentWeatherSolverCellRow) == d3d12_environment_weather_solver_cell_row_stride_bytes);
static_assert(sizeof(D3d12EnvironmentWeatherSolverOutputRow) ==
              d3d12_environment_weather_solver_output_row_stride_bytes);

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_environment_weather_solver_compute_shader() {
    static constexpr const char* source =
        "struct CellInput {"
        "  float temperature_celsius;"
        "  float vapor_water_kg_per_m2;"
        "  float cloud_water_kg_per_m2;"
        "  float surface_water_kg_per_m2;"
        "  float surface_evaporation_kg_per_m2_s;"
        "  float temperature_delta_celsius_per_s;"
        "  float cloud_precipitation_rate_per_s;"
        "  float padding0;"
        "};"
        "struct CellOutput {"
        "  float temperature_celsius;"
        "  float vapor_water_kg_per_m2;"
        "  float cloud_water_kg_per_m2;"
        "  float surface_water_kg_per_m2;"
        "  float saturation_vapor_kg_per_m2;"
        "  float evaporated_kg_per_m2;"
        "  float condensed_kg_per_m2;"
        "  float precipitated_kg_per_m2;"
        "};"
        "StructuredBuffer<CellInput> input_cells : register(t0);"
        "RWStructuredBuffer<CellOutput> output_cells : register(u0);"
        "cbuffer Constants : register(b0) {"
        "  uint cell_count;"
        "  float effective_timestep_s;"
        "  float air_pressure_hpa;"
        "  float mixing_height_m;"
        "};"
        "float saturation_vapor_kg_per_m2(float temperature_celsius, float pressure_hpa, float mixing_height) {"
        "  float temperature_kelvin = temperature_celsius + 273.15f;"
        "  float pressure_pa = pressure_hpa * 100.0f;"
        "  float saturation_hpa = 6.11f * pow(10.0f, (7.5f * temperature_celsius) / (237.3f + temperature_celsius));"
        "  float saturation_pa = saturation_hpa * 100.0f;"
        "  float air_density = pressure_pa / (287.05f * temperature_kelvin);"
        "  float air_mass = air_density * mixing_height;"
        "  float specific_humidity = (0.622f * saturation_pa) / (pressure_pa - (0.378f * saturation_pa));"
        "  return specific_humidity * air_mass;"
        "}"
        "[numthreads(64, 1, 1)]"
        "void cs_main(uint3 dispatch_id : SV_DispatchThreadID) {"
        "  uint index = dispatch_id.x;"
        "  if (index >= cell_count) { return; }"
        "  CellInput input = input_cells[index];"
        "  CellOutput output;"
        "  output.temperature_celsius = input.temperature_celsius + "
        "      (input.temperature_delta_celsius_per_s * effective_timestep_s);"
        "  float evaporated = min(input.surface_water_kg_per_m2, "
        "      input.surface_evaporation_kg_per_m2_s * effective_timestep_s);"
        "  float vapor = input.vapor_water_kg_per_m2 + evaporated;"
        "  float surface = input.surface_water_kg_per_m2 - evaporated;"
        "  float cloud = input.cloud_water_kg_per_m2;"
        "  float saturation = saturation_vapor_kg_per_m2(output.temperature_celsius, air_pressure_hpa, "
        "mixing_height_m);"
        "  float condensed = max(0.0f, vapor - saturation);"
        "  vapor -= condensed;"
        "  cloud += condensed;"
        "  float precipitated = min(cloud, cloud * input.cloud_precipitation_rate_per_s * effective_timestep_s);"
        "  cloud -= precipitated;"
        "  surface += precipitated;"
        "  output.vapor_water_kg_per_m2 = vapor;"
        "  output.cloud_water_kg_per_m2 = cloud;"
        "  output.surface_water_kg_per_m2 = surface;"
        "  output.saturation_vapor_kg_per_m2 = saturation;"
        "  output.evaporated_kg_per_m2 = evaporated;"
        "  output.condensed_kg_per_m2 = condensed;"
        "  output.precipitated_kg_per_m2 = precipitated;"
        "  output_cells[index] = output;"
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

void transition_resource(ID3D12GraphicsCommandList* command_list, ID3D12Resource* resource,
                         D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after,
                         std::uint32_t& barriers_recorded) noexcept {
    if (before == after) {
        return;
    }
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    command_list->ResourceBarrier(1, &barrier);
    ++barriers_recorded;
}

void uav_barrier(ID3D12GraphicsCommandList* command_list, ID3D12Resource* resource,
                 std::uint32_t& barriers_recorded) noexcept {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = resource;
    command_list->ResourceBarrier(1, &barrier);
    ++barriers_recorded;
}

[[nodiscard]] bool create_root_signature(ID3D12Device* device,
                                         Microsoft::WRL::ComPtr<ID3D12RootSignature>& root_signature) {
    std::array<D3D12_DESCRIPTOR_RANGE1, 2> descriptor_ranges{};
    descriptor_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptor_ranges[0].NumDescriptors = 1;
    descriptor_ranges[0].BaseShaderRegister = 0;
    descriptor_ranges[0].OffsetInDescriptorsFromTableStart = 0;

    descriptor_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptor_ranges[1].NumDescriptors = 1;
    descriptor_ranges[1].BaseShaderRegister = 0;
    descriptor_ranges[1].OffsetInDescriptorsFromTableStart = 1;

    std::array<D3D12_ROOT_PARAMETER1, 2> root_parameters{};
    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    root_parameters[0].Descriptor.ShaderRegister = 0;
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    root_parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[1].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(descriptor_ranges.size());
    root_parameters[1].DescriptorTable.pDescriptorRanges = descriptor_ranges.data();
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc{};
    root_signature_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    root_signature_desc.Desc_1_1.NumParameters = static_cast<UINT>(root_parameters.size());
    root_signature_desc.Desc_1_1.pParameters = root_parameters.data();
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

[[nodiscard]] bool valid_desc(const D3d12EnvironmentWeatherSolverDesc& desc) noexcept {
    return !desc.cell_rows.empty() && desc.effective_timestep_s > 0.0F && desc.air_pressure_hpa > 0.0F &&
           desc.mixing_height_m > 0.0F &&
           desc.cell_rows.size() <= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
}

[[nodiscard]] std::vector<std::uint8_t> readback_buffer_bytes(ID3D12Resource* buffer, std::uint64_t size_bytes) {
    std::vector<std::uint8_t> bytes;
    if (buffer == nullptr || size_bytes == 0U ||
        size_bytes > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return bytes;
    }
    void* mapped = nullptr;
    D3D12_RANGE read_range{0, static_cast<SIZE_T>(size_bytes)};
    if (FAILED(buffer->Map(0, &read_range, &mapped)) || mapped == nullptr) {
        return bytes;
    }
    bytes.resize(static_cast<std::size_t>(size_bytes));
    std::memcpy(bytes.data(), mapped, bytes.size());
    D3D12_RANGE written_range{0, 0};
    buffer->Unmap(0, &written_range);
    return bytes;
}

[[nodiscard]] std::uint64_t hash_bytes(std::span<const std::uint8_t> bytes) noexcept {
    std::uint64_t hash = fnv_offset_basis;
    for (const auto byte : bytes) {
        hash ^= byte;
        hash *= fnv_prime;
    }
    return hash == 0U ? fnv_offset_basis : hash;
}

[[nodiscard]] bool any_nonzero(std::span<const std::uint8_t> bytes) noexcept {
    return std::ranges::any_of(bytes, [](const std::uint8_t byte) { return byte != 0U; });
}

[[nodiscard]] std::vector<D3d12EnvironmentWeatherSolverOutputRow>
decode_output_rows(std::span<const std::uint8_t> bytes) {
    std::vector<D3d12EnvironmentWeatherSolverOutputRow> rows;
    if (bytes.empty() || (bytes.size() % sizeof(D3d12EnvironmentWeatherSolverOutputRow)) != 0U) {
        return rows;
    }
    rows.resize(bytes.size() / sizeof(D3d12EnvironmentWeatherSolverOutputRow));
    std::memcpy(rows.data(), bytes.data(), bytes.size());
    return rows;
}

[[nodiscard]] D3d12EnvironmentWeatherSolverResult
dispatch_environment_weather_solver_on_device(const D3d12EnvironmentWeatherSolverDesc& desc, ID3D12Device* device,
                                              ID3D12CommandQueue* command_queue, ID3D12Fence* fence,
                                              HANDLE fence_event) noexcept {
    D3d12EnvironmentWeatherSolverResult result{};
    result.cell_count = static_cast<std::uint32_t>(desc.cell_rows.size());
    result.output_buffer_size_bytes =
        static_cast<std::uint64_t>(result.cell_count) * d3d12_environment_weather_solver_output_row_stride_bytes;

    if (!valid_desc(desc) || device == nullptr || command_queue == nullptr || fence == nullptr ||
        fence_event == nullptr) {
        result.failure_stage = 1U;
        return result;
    }

    const auto shader_bytecode = compile_environment_weather_solver_compute_shader();
    if (shader_bytecode == nullptr) {
        result.failure_stage = 2U;
        return result;
    }

    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
    if (!create_root_signature(device, root_signature) ||
        !create_compute_pipeline_state(device, root_signature.Get(), shader_bytecode.Get(), pipeline_state)) {
        result.failure_stage = 3U;
        return result;
    }

    const auto input_buffer_size =
        static_cast<std::uint64_t>(result.cell_count) * d3d12_environment_weather_solver_cell_row_stride_bytes;
    const auto output_buffer_size = result.output_buffer_size_bytes;
    const auto default_heap = default_heap_properties();
    const auto upload_heap = upload_heap_properties();
    const auto readback_heap = readback_heap_properties();
    const auto input_desc = buffer_resource_desc(input_buffer_size);
    const auto output_desc = buffer_resource_desc(output_buffer_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    const auto output_readback_desc = buffer_resource_desc(output_buffer_size);
    const auto constants_desc = buffer_resource_desc(256U);

    Microsoft::WRL::ComPtr<ID3D12Resource> input_default_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> input_upload_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> output_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> output_readback_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> constants_upload_buffer;

    if (FAILED(device->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE, &input_desc,
                                               D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                               IID_PPV_ARGS(&input_default_buffer))) ||
        FAILED(device->CreateCommittedResource(&upload_heap, D3D12_HEAP_FLAG_NONE, &input_desc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(&input_upload_buffer))) ||
        FAILED(device->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE, &output_desc,
                                               D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
                                               IID_PPV_ARGS(&output_buffer))) ||
        FAILED(device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &output_readback_desc,
                                               D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                               IID_PPV_ARGS(&output_readback_buffer))) ||
        FAILED(device->CreateCommittedResource(&upload_heap, D3D12_HEAP_FLAG_NONE, &constants_desc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(&constants_upload_buffer)))) {
        result.failure_stage = 4U;
        return result;
    }

    {
        void* mapped = nullptr;
        if (FAILED(input_upload_buffer->Map(0, nullptr, &mapped)) || mapped == nullptr) {
            result.failure_stage = 5U;
            return result;
        }
        std::memcpy(mapped, desc.cell_rows.data(), static_cast<std::size_t>(input_buffer_size));
        input_upload_buffer->Unmap(0, nullptr);
    }

    {
        struct Constants {
            std::uint32_t cell_count;
            float effective_timestep_s;
            float air_pressure_hpa;
            float mixing_height_m;
        };
        const Constants constants{
            .cell_count = result.cell_count,
            .effective_timestep_s = desc.effective_timestep_s,
            .air_pressure_hpa = desc.air_pressure_hpa,
            .mixing_height_m = desc.mixing_height_m,
        };
        void* mapped = nullptr;
        if (FAILED(constants_upload_buffer->Map(0, nullptr, &mapped)) || mapped == nullptr) {
            result.failure_stage = 6U;
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
        result.failure_stage = 7U;
        return result;
    }

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = 2;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap;
    if (FAILED(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&descriptor_heap)))) {
        result.failure_stage = 8U;
        return result;
    }

    const auto descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const auto heap_cpu_start = descriptor_heap->GetCPUDescriptorHandleForHeapStart();

    D3D12_SHADER_RESOURCE_VIEW_DESC input_srv_desc{};
    input_srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    input_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    input_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    input_srv_desc.Buffer.NumElements = result.cell_count;
    input_srv_desc.Buffer.StructureByteStride = d3d12_environment_weather_solver_cell_row_stride_bytes;
    device->CreateShaderResourceView(input_default_buffer.Get(), &input_srv_desc, heap_cpu_start);

    D3D12_UNORDERED_ACCESS_VIEW_DESC output_uav_desc{};
    output_uav_desc.Format = DXGI_FORMAT_UNKNOWN;
    output_uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    output_uav_desc.Buffer.NumElements = result.cell_count;
    output_uav_desc.Buffer.StructureByteStride = d3d12_environment_weather_solver_output_row_stride_bytes;
    device->CreateUnorderedAccessView(output_buffer.Get(), nullptr, &output_uav_desc,
                                      cpu_descriptor_handle(heap_cpu_start, 1U, descriptor_size));

    command_list->CopyBufferRegion(input_default_buffer.Get(), 0, input_upload_buffer.Get(), 0, input_buffer_size);
    transition_resource(command_list.Get(), input_default_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, result.resource_barriers_recorded);

    ID3D12DescriptorHeap* heaps[] = {descriptor_heap.Get()};
    command_list->SetDescriptorHeaps(1, heaps);
    command_list->SetComputeRootSignature(root_signature.Get());
    command_list->SetPipelineState(pipeline_state.Get());
    command_list->SetComputeRootConstantBufferView(0, constants_upload_buffer->GetGPUVirtualAddress());
    command_list->SetComputeRootDescriptorTable(
        1, gpu_descriptor_handle(descriptor_heap->GetGPUDescriptorHandleForHeapStart(), 0U, descriptor_size));

    const auto workgroups = static_cast<UINT>((result.cell_count + 63U) / 64U);
    command_list->Dispatch(workgroups, 1, 1);
    result.compute_dispatches = 1U;

    uav_barrier(command_list.Get(), output_buffer.Get(), result.resource_barriers_recorded);
    transition_resource(command_list.Get(), output_buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                        D3D12_RESOURCE_STATE_COPY_SOURCE, result.resource_barriers_recorded);
    command_list->CopyBufferRegion(output_readback_buffer.Get(), 0, output_buffer.Get(), 0, output_buffer_size);

    if (FAILED(command_list->Close())) {
        result.failure_stage = 9U;
        return result;
    }

    ID3D12CommandList* command_lists[] = {command_list.Get()};
    command_queue->ExecuteCommandLists(1, command_lists);
    const std::uint64_t fence_value = 1U;
    if (FAILED(command_queue->Signal(fence, fence_value)) || !wait_for_fence(fence, fence_value, fence_event)) {
        result.failure_stage = 10U;
        return result;
    }

    const auto bytes = readback_buffer_bytes(output_readback_buffer.Get(), output_buffer_size);
    if (bytes.size() != static_cast<std::size_t>(output_buffer_size)) {
        result.failure_stage = 11U;
        return result;
    }
    result.output_readback_nonzero = any_nonzero(bytes);
    result.output_checksum = hash_bytes(bytes);
    result.output_rows = decode_output_rows(bytes);
    result.executed_gpu_solver = true;
    result.succeeded = result.output_readback_nonzero && result.output_rows.size() == desc.cell_rows.size();
    return result;
}

} // namespace

D3d12EnvironmentWeatherSolverResult
dispatch_environment_weather_solver(const D3d12EnvironmentWeatherSolverDesc& desc) noexcept {
    D3d12EnvironmentWeatherSolverResult result{};

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

    result = dispatch_environment_weather_solver_on_device(desc, device.Get(), command_queue.Get(), fence.Get(),
                                                           fence_event);
    CloseHandle(fence_event);
    return result;
}

} // namespace mirakana::rhi::d3d12
