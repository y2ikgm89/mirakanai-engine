// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/d3d12/d3d12_mavg_mesh_shader_lod.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::rhi::d3d12 {
namespace {

constexpr std::uint32_t render_target_width = 64U;
constexpr std::uint32_t render_target_height = 64U;
constexpr std::uint32_t render_target_row_pitch = 256U;
constexpr std::uint64_t fnv_offset = 14695981039346656037ULL;
constexpr std::uint64_t fnv_prime = 1099511628211ULL;

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
template <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, typename Value> struct alignas(void*) PipelineStreamSubobject {
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type{Type};
    Value value{};
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

struct MeshPipelineStream {
    PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*> root_signature;
    PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS, D3D12_SHADER_BYTECODE> amplification_shader;
    PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE> mesh_shader;
    PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE> pixel_shader;
    PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC> blend;
    PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC> rasterizer;
    PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, D3D12_DEPTH_STENCIL_DESC> depth_stencil;
    PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE>
        primitive_topology;
    PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY>
        render_target_formats;
    PipelineStreamSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC> sample_desc;
};

[[nodiscard]] std::string narrow_adapter_name(const WCHAR* name) {
    if (name == nullptr || name[0] == L'\0') {
        return {};
    }
    const int required = WideCharToMultiByte(CP_UTF8, 0, name, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) {
        return {};
    }
    std::string result(static_cast<std::size_t>(required - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, name, -1, result.data(), required, nullptr, nullptr);
    return result;
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

[[nodiscard]] D3D12_HEAP_PROPERTIES readback_heap_properties() noexcept {
    D3D12_HEAP_PROPERTIES properties{};
    properties.Type = D3D12_HEAP_TYPE_READBACK;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;
    return properties;
}

[[nodiscard]] D3D12_RESOURCE_DESC render_target_desc(std::uint32_t width, std::uint32_t height) noexcept {
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    return desc;
}

[[nodiscard]] D3D12_RESOURCE_DESC buffer_desc(std::uint64_t size_bytes) noexcept {
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
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
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

[[nodiscard]] Microsoft::WRL::ComPtr<IDXGIAdapter1> select_adapter(IDXGIFactory6* factory, bool prefer_warp) {
    if (factory == nullptr) {
        return nullptr;
    }

    if (!prefer_warp) {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> hardware_adapter;
        for (UINT adapter_index = 0; factory->EnumAdapters1(adapter_index, &hardware_adapter) != DXGI_ERROR_NOT_FOUND;
             ++adapter_index) {
            DXGI_ADAPTER_DESC1 desc{};
            if (SUCCEEDED(hardware_adapter->GetDesc1(&desc)) && (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) {
                return hardware_adapter;
            }
            hardware_adapter.Reset();
        }
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter> warp_adapter;
    if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter)))) {
        return nullptr;
    }
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1;
    if (FAILED(warp_adapter.As(&adapter1))) {
        return nullptr;
    }
    return adapter1;
}

struct DeviceProbe {
    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    D3d12MavgMeshShaderLodCapabilityResult capability;
};

[[nodiscard]] DeviceProbe create_probe_device(const DeviceBootstrapDesc& desc) noexcept {
    DeviceProbe probe;
    probe.capability.windows_sdk_available = compiled_with_windows_sdk();
    if (!probe.capability.windows_sdk_available) {
        probe.capability.diagnostic_text = "windows_sdk_unavailable";
        ++probe.capability.diagnostic_count;
        return probe;
    }

    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&probe.factory)))) {
        probe.capability.diagnostic_text = "dxgi_factory_creation_failed";
        ++probe.capability.diagnostic_count;
        return probe;
    }
    probe.capability.dxgi_factory_created = true;
    probe.adapter = select_adapter(probe.factory.Get(), desc.prefer_warp);
    if (probe.adapter == nullptr) {
        probe.capability.diagnostic_text = "dxgi_adapter_selection_failed";
        ++probe.capability.diagnostic_count;
        return probe;
    }
    probe.capability.adapter_selected = true;
    DXGI_ADAPTER_DESC1 adapter_desc{};
    if (SUCCEEDED(probe.adapter->GetDesc1(&adapter_desc))) {
        probe.capability.adapter_name = narrow_adapter_name(adapter_desc.Description);
    }

    if (FAILED(D3D12CreateDevice(probe.adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&probe.device)))) {
        probe.capability.diagnostic_text = "d3d12_device_creation_failed";
        ++probe.capability.diagnostic_count;
        return probe;
    }
    probe.capability.device_created = true;

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7{};
    if (FAILED(probe.device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7)))) {
        probe.capability.diagnostic_text = "d3d12_options7_feature_query_failed";
        ++probe.capability.diagnostic_count;
        return probe;
    }

    probe.capability.feature_query_executed = true;
    probe.capability.mesh_shader_tier = static_cast<std::uint32_t>(options7.MeshShaderTier);
    probe.capability.mesh_shader_supported = options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;
    if (!probe.capability.mesh_shader_supported) {
        probe.capability.diagnostic_text = "d3d12_mesh_shader_tier_not_supported";
        ++probe.capability.diagnostic_count;
    }
    return probe;
}

[[nodiscard]] bool valid_task_rows(std::span<const D3d12MavgMeshShaderLodTaskRow> rows) noexcept {
    return !rows.empty() && std::ranges::all_of(rows, [](const D3d12MavgMeshShaderLodTaskRow& row) {
        return row.output_vertex_count >= 3U && row.output_primitive_count > 0U && row.group_thread_count > 0U &&
               row.group_thread_count <= 128U && row.fallback_index_count > 0U;
    });
}

[[nodiscard]] std::string_view default_mavg_mesh_shader_lod_hlsl() noexcept {
    return R"(
struct Payload { uint color_index; };
struct VertexOut { float4 position : SV_Position; float4 color : COLOR0; };

[numthreads(1, 1, 1)]
void as_main(uint3 dispatch_id : SV_DispatchThreadID) {
    Payload payload_out;
    payload_out.color_index = dispatch_id.x;
    DispatchMesh(1, 1, 1, payload_out);
}

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void ms_main(in payload Payload payload_in, out vertices VertexOut vertices_out[3], out indices uint3 primitives_out[1]) {
    SetMeshOutputCounts(3, 1);
    vertices_out[0].position = float4(0.0, 0.5, 0.0, 1.0);
    vertices_out[1].position = float4(0.5, -0.5, 0.0, 1.0);
    vertices_out[2].position = float4(-0.5, -0.5, 0.0, 1.0);
    vertices_out[0].color = float4(0.0, 1.0, 0.0, 1.0);
    vertices_out[1].color = float4(0.0, 1.0, 0.0, 1.0);
    vertices_out[2].color = float4(0.0, 1.0, 0.0, 1.0);
    primitives_out[0] = uint3(0, 1, 2);
}

float4 ps_main(VertexOut input) : SV_Target {
    return input.color;
}
)";
}

struct DxcCompilerSession {
    HMODULE module{nullptr};
    DxcCreateInstanceProc create_instance{nullptr};
    Microsoft::WRL::ComPtr<IDxcUtils> utils;
    Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;

    DxcCompilerSession() = default;
    DxcCompilerSession(const DxcCompilerSession&) = delete;
    DxcCompilerSession& operator=(const DxcCompilerSession&) = delete;

    DxcCompilerSession(DxcCompilerSession&& other) noexcept
        : module(std::exchange(other.module, nullptr)), create_instance(std::exchange(other.create_instance, nullptr)),
          utils(std::move(other.utils)), compiler(std::move(other.compiler)) {}

    DxcCompilerSession& operator=(DxcCompilerSession&& other) noexcept {
        if (this != &other) {
            compiler.Reset();
            utils.Reset();
            if (module != nullptr) {
                FreeLibrary(module);
            }
            module = std::exchange(other.module, nullptr);
            create_instance = std::exchange(other.create_instance, nullptr);
            utils = std::move(other.utils);
            compiler = std::move(other.compiler);
        }
        return *this;
    }

    ~DxcCompilerSession() {
        compiler.Reset();
        utils.Reset();
        if (module != nullptr) {
            FreeLibrary(module);
        }
    }
};

[[nodiscard]] DxcCompilerSession create_dxc_compiler_session() noexcept {
    DxcCompilerSession session;
    session.module = LoadLibraryW(L"dxcompiler.dll");
    if (session.module == nullptr) {
        return session;
    }
    session.create_instance =
        reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(session.module, "DxcCreateInstance"));
    if (session.create_instance == nullptr) {
        return session;
    }
    if (FAILED(session.create_instance(CLSID_DxcUtils, IID_PPV_ARGS(&session.utils))) ||
        FAILED(session.create_instance(CLSID_DxcCompiler, IID_PPV_ARGS(&session.compiler)))) {
        session.utils.Reset();
        session.compiler.Reset();
        return session;
    }
    return session;
}

[[nodiscard]] bool session_ready(const DxcCompilerSession& session) noexcept {
    return session.create_instance != nullptr && session.utils != nullptr && session.compiler != nullptr;
}

[[nodiscard]] Microsoft::WRL::ComPtr<IDxcBlob> compile_dxil(DxcCompilerSession& session, std::string_view source,
                                                            const wchar_t* entry, const wchar_t* target) {
    if (!session_ready(session)) {
        return nullptr;
    }

    DxcBuffer source_buffer{};
    source_buffer.Ptr = source.data();
    source_buffer.Size = source.size();
    source_buffer.Encoding = DXC_CP_UTF8;

    const wchar_t* args[] = {
        L"-E", entry, L"-T", target, L"-HV", L"2021", L"-Qstrip_debug", L"-Qstrip_reflect",
    };
    Microsoft::WRL::ComPtr<IDxcResult> compile_result;
    if (FAILED(session.compiler->Compile(&source_buffer, args, static_cast<std::uint32_t>(std::size(args)), nullptr,
                                         IID_PPV_ARGS(&compile_result))) ||
        compile_result == nullptr) {
        return nullptr;
    }

    HRESULT status = E_FAIL;
    if (FAILED(compile_result->GetStatus(&status)) || FAILED(status)) {
        return nullptr;
    }

    Microsoft::WRL::ComPtr<IDxcBlob> bytecode;
    if (FAILED(compile_result->GetResult(&bytecode))) {
        return nullptr;
    }
    return bytecode;
}

[[nodiscard]] bool create_root_signature(ID3D12Device* device,
                                         Microsoft::WRL::ComPtr<ID3D12RootSignature>& root_signature) {
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc{};
    root_signature_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    root_signature_desc.Desc_1_1.NumParameters = 0;
    root_signature_desc.Desc_1_1.pParameters = nullptr;
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

[[nodiscard]] D3D12_DEPTH_STENCIL_DESC disabled_depth_stencil_desc() noexcept {
    D3D12_DEPTH_STENCIL_DESC desc{};
    desc.DepthEnable = FALSE;
    desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    desc.StencilEnable = FALSE;
    desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    desc.BackFace = desc.FrontFace;
    return desc;
}

[[nodiscard]] bool create_mesh_pipeline_state(ID3D12Device* device, ID3D12RootSignature* root_signature,
                                              IDxcBlob* amplification_shader, IDxcBlob* mesh_shader,
                                              IDxcBlob* pixel_shader,
                                              Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipeline_state) {
    Microsoft::WRL::ComPtr<ID3D12Device2> device2;
    if (device == nullptr || FAILED(device->QueryInterface(IID_PPV_ARGS(&device2)))) {
        return false;
    }

    D3D12_RT_FORMAT_ARRAY rt_formats{};
    rt_formats.NumRenderTargets = 1;
    rt_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    MeshPipelineStream stream{};
    stream.root_signature.value = root_signature;
    stream.amplification_shader.value = {amplification_shader->GetBufferPointer(),
                                         amplification_shader->GetBufferSize()};
    stream.mesh_shader.value = {mesh_shader->GetBufferPointer(), mesh_shader->GetBufferSize()};
    stream.pixel_shader.value = {pixel_shader->GetBufferPointer(), pixel_shader->GetBufferSize()};
    stream.blend.value = default_blend_desc();
    stream.rasterizer.value = default_rasterizer_desc();
    stream.depth_stencil.value = disabled_depth_stencil_desc();
    stream.primitive_topology.value = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    stream.render_target_formats.value = rt_formats;
    stream.sample_desc.value = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0};

    D3D12_PIPELINE_STATE_STREAM_DESC stream_desc{};
    stream_desc.SizeInBytes = sizeof(stream);
    stream_desc.pPipelineStateSubobjectStream = &stream;
    return SUCCEEDED(device2->CreatePipelineState(&stream_desc, IID_PPV_ARGS(&pipeline_state)));
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

[[nodiscard]] std::vector<std::uint8_t> readback_buffer_bytes(ID3D12Resource* readback, std::uint64_t size_bytes) {
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size_bytes));
    void* mapped = nullptr;
    D3D12_RANGE read_range{.Begin = 0, .End = static_cast<SIZE_T>(size_bytes)};
    if (readback == nullptr || FAILED(readback->Map(0, &read_range, &mapped)) || mapped == nullptr) {
        bytes.clear();
        return bytes;
    }
    std::memcpy(bytes.data(), mapped, static_cast<std::size_t>(size_bytes));
    const D3D12_RANGE empty_range{};
    readback->Unmap(0, &empty_range);
    return bytes;
}

[[nodiscard]] std::uint64_t hash_bytes(std::span<const std::uint8_t> bytes) noexcept {
    std::uint64_t hash = fnv_offset;
    for (const auto byte : bytes) {
        hash ^= byte;
        hash *= fnv_prime;
    }
    return hash;
}

[[nodiscard]] bool center_pixel_is_green(std::span<const std::uint8_t> bytes) noexcept {
    const auto center_offset = (render_target_height / 2U) * render_target_row_pitch + (render_target_width / 2U) * 4U;
    if (bytes.size() <= center_offset + 3U) {
        return false;
    }
    return bytes[center_offset] <= 8U && bytes[center_offset + 1U] >= 220U && bytes[center_offset + 2U] <= 8U &&
           bytes[center_offset + 3U] == 255U;
}

} // namespace

D3d12MavgMeshShaderLodCapabilityResult
probe_d3d12_mavg_mesh_shader_lod_capability(const DeviceBootstrapDesc& desc) noexcept {
    return create_probe_device(desc).capability;
}

D3d12MavgMeshShaderLodDispatchResult
execute_d3d12_mavg_mesh_shader_lod(const D3d12MavgMeshShaderLodDispatchDesc& desc) noexcept {
    D3d12MavgMeshShaderLodDispatchResult result;
    if (!valid_task_rows(desc.task_rows) || desc.render_width != render_target_width ||
        desc.render_height != render_target_height) {
        result.diagnostic_text = "invalid_mavg_mesh_shader_lod_dispatch_desc";
        ++result.diagnostic_count;
        result.failure_stage = 1U;
        return result;
    }

    auto probe = create_probe_device(desc.device);
    result.feature_query_executed = probe.capability.feature_query_executed;
    result.d3d12_mesh_shader_supported = probe.capability.mesh_shader_supported;
    result.mesh_shader_tier = probe.capability.mesh_shader_tier;
    result.adapter_name = probe.capability.adapter_name;
    result.diagnostic_text = probe.capability.diagnostic_text;
    if (!probe.capability.mesh_shader_supported || probe.device == nullptr) {
        result.host_gated = true;
        result.diagnostic_count = std::max<std::uint32_t>(1U, probe.capability.diagnostic_count);
        result.failure_stage = 2U;
        return result;
    }

    DxcCompilerSession dxc = create_dxc_compiler_session();
    result.dxcompiler_available = session_ready(dxc);
    if (!result.dxcompiler_available) {
        result.host_gated = true;
        result.diagnostic_text = "dxcompiler_unavailable";
        ++result.diagnostic_count;
        result.failure_stage = 3U;
        return result;
    }

    const auto source = default_mavg_mesh_shader_lod_hlsl();
    const auto amplification_shader = compile_dxil(dxc, source, L"as_main", L"as_6_5");
    const auto mesh_shader = compile_dxil(dxc, source, L"ms_main", L"ms_6_5");
    const auto pixel_shader = compile_dxil(dxc, source, L"ps_main", L"ps_6_0");
    if (amplification_shader == nullptr || mesh_shader == nullptr || pixel_shader == nullptr) {
        result.diagnostic_text = "dxc_mesh_shader_compilation_failed";
        ++result.diagnostic_count;
        result.failure_stage = 4U;
        return result;
    }

    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
    if (!create_root_signature(probe.device.Get(), root_signature) ||
        !create_mesh_pipeline_state(probe.device.Get(), root_signature.Get(), amplification_shader.Get(),
                                    mesh_shader.Get(), pixel_shader.Get(), pipeline_state)) {
        result.diagnostic_text = "d3d12_mesh_pipeline_state_creation_failed";
        ++result.diagnostic_count;
        result.failure_stage = 5U;
        return result;
    }
    result.created_mesh_pipeline_state = true;
    result.used_mesh_shader_bind_point = true;
    result.used_amplification_shader_bind_point = true;
    result.used_input_layout = false;
    result.used_index_buffer = false;

    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> mesh_command_list;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    if (FAILED(probe.device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue))) ||
        FAILED(probe.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))) ||
        FAILED(probe.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
                                               IID_PPV_ARGS(&command_list))) ||
        FAILED(command_list.As(&mesh_command_list)) ||
        FAILED(probe.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
        result.diagnostic_text = "d3d12_mesh_command_infrastructure_creation_failed";
        ++result.diagnostic_count;
        result.failure_stage = 6U;
        return result;
    }

    const auto default_heap = default_heap_properties();
    const auto readback_heap = readback_heap_properties();
    const auto target_desc = render_target_desc(render_target_width, render_target_height);
    const auto readback_size = static_cast<std::uint64_t>(render_target_row_pitch) * render_target_height;
    const auto readback_desc = buffer_desc(readback_size);
    D3D12_CLEAR_VALUE clear_value{};
    clear_value.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    clear_value.Color[0] = 0.0F;
    clear_value.Color[1] = 0.0F;
    clear_value.Color[2] = 0.0F;
    clear_value.Color[3] = 1.0F;

    Microsoft::WRL::ComPtr<ID3D12Resource> render_target;
    Microsoft::WRL::ComPtr<ID3D12Resource> readback;
    if (FAILED(probe.device->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE, &target_desc,
                                                     D3D12_RESOURCE_STATE_RENDER_TARGET, &clear_value,
                                                     IID_PPV_ARGS(&render_target))) ||
        FAILED(probe.device->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE, &readback_desc,
                                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                     IID_PPV_ARGS(&readback)))) {
        result.diagnostic_text = "d3d12_mesh_readback_resource_creation_failed";
        ++result.diagnostic_count;
        result.failure_stage = 7U;
        return result;
    }

    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc{};
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_heap_desc.NumDescriptors = 1;
    rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap;
    if (FAILED(probe.device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap)))) {
        result.diagnostic_text = "d3d12_mesh_rtv_heap_creation_failed";
        ++result.diagnostic_count;
        result.failure_stage = 8U;
        return result;
    }
    const auto rtv = rtv_heap->GetCPUDescriptorHandleForHeapStart();
    probe.device->CreateRenderTargetView(render_target.Get(), nullptr, rtv);

    const D3D12_VIEWPORT viewport{.TopLeftX = 0.0F,
                                  .TopLeftY = 0.0F,
                                  .Width = static_cast<float>(render_target_width),
                                  .Height = static_cast<float>(render_target_height),
                                  .MinDepth = 0.0F,
                                  .MaxDepth = 1.0F};
    const D3D12_RECT scissor{.left = 0,
                             .top = 0,
                             .right = static_cast<LONG>(render_target_width),
                             .bottom = static_cast<LONG>(render_target_height)};
    const std::array<float, 4> clear_color{0.0F, 0.0F, 0.0F, 1.0F};
    mesh_command_list->SetGraphicsRootSignature(root_signature.Get());
    mesh_command_list->SetPipelineState(pipeline_state.Get());
    mesh_command_list->RSSetViewports(1, &viewport);
    mesh_command_list->RSSetScissorRects(1, &scissor);
    mesh_command_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    mesh_command_list->ClearRenderTargetView(rtv, clear_color.data(), 0, nullptr);
    mesh_command_list->DispatchMesh(1, 1, 1);
    result.dispatch_mesh_direct_calls = 1U;

    transition_resource(mesh_command_list.Get(), render_target.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                        D3D12_RESOURCE_STATE_COPY_SOURCE, result.resource_barriers_recorded);
    D3D12_TEXTURE_COPY_LOCATION source_location{};
    source_location.pResource = render_target.Get();
    source_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    source_location.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION destination_location{};
    destination_location.pResource = readback.Get();
    destination_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    destination_location.PlacedFootprint.Offset = 0;
    destination_location.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    destination_location.PlacedFootprint.Footprint.Width = render_target_width;
    destination_location.PlacedFootprint.Footprint.Height = render_target_height;
    destination_location.PlacedFootprint.Footprint.Depth = 1;
    destination_location.PlacedFootprint.Footprint.RowPitch = render_target_row_pitch;
    mesh_command_list->CopyTextureRegion(&destination_location, 0, 0, 0, &source_location, nullptr);

    if (FAILED(mesh_command_list->Close())) {
        result.diagnostic_text = "d3d12_mesh_command_list_close_failed";
        ++result.diagnostic_count;
        result.failure_stage = 9U;
        return result;
    }

    ID3D12CommandList* command_lists[] = {mesh_command_list.Get()};
    command_queue->ExecuteCommandLists(1, command_lists);
    HANDLE fence_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (fence_event == nullptr || FAILED(command_queue->Signal(fence.Get(), 1U)) ||
        !wait_for_fence(fence.Get(), 1U, fence_event)) {
        if (fence_event != nullptr) {
            CloseHandle(fence_event);
        }
        result.diagnostic_text = "d3d12_mesh_fence_wait_failed";
        ++result.diagnostic_count;
        result.failure_stage = 10U;
        return result;
    }
    CloseHandle(fence_event);

    const auto bytes = readback_buffer_bytes(readback.Get(), readback_size);
    if (bytes.size() != static_cast<std::size_t>(readback_size)) {
        result.diagnostic_text = "d3d12_mesh_readback_map_failed";
        ++result.diagnostic_count;
        result.failure_stage = 11U;
        return result;
    }

    result.readback_hash = hash_bytes(bytes);
    result.readback_nonzero = center_pixel_is_green(bytes);
    result.executed_mesh_shader = result.readback_nonzero;
    result.succeeded = result.executed_mesh_shader && result.dispatch_mesh_direct_calls == 1U &&
                       result.execute_indirect_mesh_dispatch_calls == 0U && result.created_mesh_pipeline_state &&
                       result.used_mesh_shader_bind_point && result.used_amplification_shader_bind_point &&
                       !result.used_input_layout && !result.used_index_buffer;
    result.mavg_mesh_shader_lod_d3d12_ready = result.succeeded;
    if (!result.succeeded) {
        result.diagnostic_text = "d3d12_mesh_readback_content_mismatch";
        ++result.diagnostic_count;
        result.failure_stage = 12U;
    }
    return result;
}

} // namespace mirakana::rhi::d3d12
