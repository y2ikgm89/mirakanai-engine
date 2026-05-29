// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/postprocess_policy.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool is_supported_effect(PostprocessEffectKind kind) noexcept {
    switch (kind) {
    case PostprocessEffectKind::tone_mapping:
    case PostprocessEffectKind::exposure:
    case PostprocessEffectKind::bloom:
    case PostprocessEffectKind::color_grading:
    case PostprocessEffectKind::fog:
    case PostprocessEffectKind::anti_aliasing:
        return true;
    case PostprocessEffectKind::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] std::string_view effect_name(PostprocessEffectKind kind) noexcept {
    switch (kind) {
    case PostprocessEffectKind::tone_mapping:
        return "tone_mapping";
    case PostprocessEffectKind::exposure:
        return "exposure";
    case PostprocessEffectKind::bloom:
        return "bloom";
    case PostprocessEffectKind::color_grading:
        return "color_grading";
    case PostprocessEffectKind::fog:
        return "fog";
    case PostprocessEffectKind::anti_aliasing:
        return "anti_aliasing";
    case PostprocessEffectKind::unknown:
        return "unknown";
    }
    return "unknown";
}

[[nodiscard]] std::string_view backend_name(rhi::BackendKind backend) noexcept {
    switch (backend) {
    case rhi::BackendKind::null:
        return "null";
    case rhi::BackendKind::d3d12:
        return "d3d12";
    case rhi::BackendKind::vulkan:
        return "vulkan";
    case rhi::BackendKind::metal:
        return "metal";
    }
    return "unknown";
}

void add_diagnostic(PostprocessChainPolicyPlan& plan, PostprocessChainDiagnosticCode code, std::size_t effect_index,
                    std::uint32_t source_index, std::string message) {
    plan.diagnostics.push_back(PostprocessChainDiagnostic{
        .code = code,
        .effect_index = effect_index,
        .source_index = source_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool uses_depth(const PostprocessEffectDesc& effect) noexcept {
    return effect.requires_scene_depth || effect.kind == PostprocessEffectKind::fog;
}

[[nodiscard]] bool has_duplicate_kind(const std::vector<PostprocessEffectRow>& rows,
                                      PostprocessEffectKind kind) noexcept {
    return std::ranges::any_of(rows, [kind](const PostprocessEffectRow& row) { return row.kind == kind; });
}

[[nodiscard]] bool is_supported_anti_aliasing(PostprocessAntiAliasingMode mode) noexcept {
    return mode == PostprocessAntiAliasingMode::none || mode == PostprocessAntiAliasingMode::fxaa;
}

[[nodiscard]] bool is_supported_tone_mapping_backend(rhi::BackendKind backend) noexcept {
    return backend == rhi::BackendKind::d3d12 || backend == rhi::BackendKind::vulkan ||
           backend == rhi::BackendKind::metal;
}

[[nodiscard]] bool contains_backend(const std::vector<rhi::BackendKind>& backends, rhi::BackendKind backend) {
    return std::ranges::find(backends, backend) != backends.end();
}

[[nodiscard]] std::uint8_t backend_sort_key(rhi::BackendKind backend) noexcept {
    return static_cast<std::uint8_t>(backend);
}

[[nodiscard]] bool is_token_char(char ch) noexcept {
    const auto value = static_cast<unsigned char>(ch);
    return (value >= static_cast<unsigned char>('a') && value <= static_cast<unsigned char>('z')) ||
           (value >= static_cast<unsigned char>('A') && value <= static_cast<unsigned char>('Z')) ||
           (value >= static_cast<unsigned char>('0') && value <= static_cast<unsigned char>('9'));
}

[[nodiscard]] char lower_ascii(char ch) noexcept {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

[[nodiscard]] bool is_forbidden_native_token(std::string_view token) {
    return token == "native" || token == "handle" || token == "hwnd" || token == "id3d12" || token == "idxgi" ||
           token == "dxgi" || token == "d3d12" || token == "direct3d" || token == "direct3d12" || token == "vulkan" ||
           token == "vk" || token == "vkimage" || token == "vkdevice" || token == "metal" || token == "mtl" ||
           token == "sdl" || token == "sdl3" || token == "imgui";
}

[[nodiscard]] bool has_native_token(std::string_view value) {
    std::string token;
    for (const auto ch : value) {
        if (is_token_char(ch)) {
            token.push_back(lower_ascii(ch));
            continue;
        }
        if (is_forbidden_native_token(token)) {
            return true;
        }
        token.clear();
    }
    return is_forbidden_native_token(token);
}

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool row_backend_allowed(const PostprocessToneMappingEvidenceRequest& request, rhi::BackendKind backend) {
    return is_supported_tone_mapping_backend(backend) && contains_backend(request.required_backends, backend);
}

[[nodiscard]] bool is_supported_operator(PostprocessToneMappingOperator tone_mapping_operator) noexcept {
    return tone_mapping_operator == PostprocessToneMappingOperator::aces_fitted ||
           tone_mapping_operator == PostprocessToneMappingOperator::reinhard ||
           tone_mapping_operator == PostprocessToneMappingOperator::neutral ||
           tone_mapping_operator == PostprocessToneMappingOperator::filmic;
}

[[nodiscard]] bool is_supported_input_transfer(PostprocessColorTransferFunction transfer) noexcept {
    return transfer == PostprocessColorTransferFunction::linear_scene ||
           transfer == PostprocessColorTransferFunction::sc_rgb ||
           transfer == PostprocessColorTransferFunction::pq_hdr10;
}

[[nodiscard]] bool is_supported_output_transfer(PostprocessColorTransferFunction transfer) noexcept {
    return transfer == PostprocessColorTransferFunction::srgb || transfer == PostprocessColorTransferFunction::sc_rgb ||
           transfer == PostprocessColorTransferFunction::pq_hdr10;
}

[[nodiscard]] bool has_valid_luminance_range(const PostprocessToneMappingEvidenceRow& row) noexcept {
    return row.paper_white_nits != 0U && row.max_content_nits >= row.paper_white_nits &&
           row.display_max_nits >= row.paper_white_nits && row.max_content_nits <= 100000U &&
           row.display_max_nits <= 100000U;
}

[[nodiscard]] bool is_host_gated_metal_row(const PostprocessToneMappingEvidenceRow& row) noexcept {
    return row.backend == rhi::BackendKind::metal && row.host_gate_required && !row.host_validated;
}

[[nodiscard]] bool is_ready_tone_mapping_row(const PostprocessToneMappingEvidenceRow& row) noexcept {
    return row.hdr_input_available && row.color_space_evidence_ready && row.resource_synchronization_evidence_ready &&
           row.shader_validation_evidence_ready && row.backend_validation_evidence_ready && row.host_validated &&
           !row.host_gate_required;
}

void add_diagnostic(PostprocessToneMappingEvidencePlan& plan, PostprocessToneMappingEvidenceDiagnosticCode code,
                    rhi::BackendKind backend, std::string chain_id, std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(PostprocessToneMappingEvidenceDiagnostic{
        .code = code,
        .backend = backend,
        .chain_id = std::move(chain_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

[[nodiscard]] std::size_t request_row_count(const PostprocessToneMappingEvidenceRequest& request) noexcept {
    return request.rows.size();
}

void sort_required_backends(std::vector<rhi::BackendKind>& backends) {
    std::ranges::sort(backends,
                      [](const auto lhs, const auto rhs) { return backend_sort_key(lhs) < backend_sort_key(rhs); });
}

void sort_tone_mapping_rows(std::vector<PostprocessToneMappingEvidenceRow>& rows) {
    std::ranges::sort(rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.backend != rhs.backend) {
            return backend_sort_key(lhs.backend) < backend_sort_key(rhs.backend);
        }
        if (lhs.chain_id != rhs.chain_id) {
            return lhs.chain_id < rhs.chain_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void sort_diagnostics(PostprocessToneMappingEvidencePlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.backend != rhs.backend) {
            return backend_sort_key(lhs.backend) < backend_sort_key(rhs.backend);
        }
        if (lhs.chain_id != rhs.chain_id) {
            return lhs.chain_id < rhs.chain_id;
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        return lhs.message < rhs.message;
    });
}

[[nodiscard]] std::string row_key(const PostprocessToneMappingEvidenceDiagnostic& diagnostic) {
    std::string key;
    key.reserve(diagnostic.chain_id.size() + 32U);
    key.append(std::to_string(static_cast<std::uint8_t>(diagnostic.backend)));
    key.push_back('\n');
    key.append(diagnostic.chain_id);
    key.push_back('\n');
    key.append(std::to_string(diagnostic.source_index));
    return key;
}

[[nodiscard]] std::size_t count_rejected_rows(const PostprocessToneMappingEvidencePlan& plan) {
    std::vector<std::string> keys;
    keys.reserve(plan.diagnostics.size());
    for (const auto& diagnostic : plan.diagnostics) {
        auto key = row_key(diagnostic);
        if (std::ranges::find(keys, key) == keys.end()) {
            keys.push_back(std::move(key));
        }
    }
    return keys.size();
}

void validate_required_backends(PostprocessToneMappingEvidencePlan& plan,
                                const PostprocessToneMappingEvidenceRequest& request) {
    std::vector<rhi::BackendKind> seen;
    seen.reserve(request.required_backends.size());
    for (const auto backend : request.required_backends) {
        if (!is_supported_tone_mapping_backend(backend)) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::invalid_required_backend, backend, {},
                           "tone mapping evidence supports d3d12, vulkan, and metal backends only", 0U);
            continue;
        }
        if (contains_backend(seen, backend)) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::duplicate_required_backend, backend, {},
                           "required tone mapping evidence backends must be unique", 0U);
            continue;
        }
        seen.push_back(backend);
    }
}

void validate_duplicate_backend_chains(PostprocessToneMappingEvidencePlan& plan,
                                       const PostprocessToneMappingEvidenceRequest& request) {
    std::vector<std::string> keys;
    keys.reserve(request.rows.size());
    for (const auto& row : request.rows) {
        std::string key;
        key.reserve(row.chain_id.size() + 8U);
        key.append(std::to_string(static_cast<std::uint8_t>(row.backend)));
        key.push_back('\n');
        key.append(row.chain_id);
        if (std::ranges::find(keys, key) != keys.end()) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::duplicate_backend_chain, row.backend,
                           row.chain_id, "tone mapping evidence rows must be unique per backend and chain id",
                           row.source_index);
            continue;
        }
        keys.push_back(std::move(key));
    }
}

void validate_tone_mapping_rows(PostprocessToneMappingEvidencePlan& plan,
                                const PostprocessToneMappingEvidenceRequest& request) {
    for (const auto& row : request.rows) {
        if (!row_backend_allowed(request, row.backend)) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::unsupported_backend, row.backend,
                           row.chain_id, "tone mapping evidence row backend must be one of the required backends",
                           row.source_index);
        }
        if (!is_valid_id(row.chain_id) || has_native_token(row.chain_id)) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::invalid_chain_id, row.backend,
                           row.chain_id, "tone mapping evidence requires backend-neutral chain ids", row.source_index);
        }
        if (!is_supported_operator(row.tone_mapping_operator)) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::unsupported_operator, row.backend,
                           row.chain_id, "tone mapping evidence requires a supported tone mapping operator",
                           row.source_index);
        }
        if (!is_supported_input_transfer(row.input_transfer) || !is_supported_output_transfer(row.output_transfer)) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::invalid_transfer_function, row.backend,
                           row.chain_id,
                           "tone mapping evidence requires explicit HDR input and output transfer functions",
                           row.source_index);
        }
        if (!has_valid_luminance_range(row)) {
            add_diagnostic(
                plan, PostprocessToneMappingEvidenceDiagnosticCode::invalid_luminance_range, row.backend, row.chain_id,
                "tone mapping evidence requires paper white, content, and display luminance ranges", row.source_index);
        }
        if (!std::isfinite(row.exposure_bias_ev) || row.exposure_bias_ev < -24.0F || row.exposure_bias_ev > 24.0F) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::invalid_exposure_bias, row.backend,
                           row.chain_id, "tone mapping evidence requires a finite exposure bias in [-24, 24] EV",
                           row.source_index);
        }
        if (!row.hdr_input_available) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::missing_hdr_input, row.backend,
                           row.chain_id, "tone mapping evidence requires HDR scene/input evidence", row.source_index);
        }
        if (row.request_native_handle_access) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::unsupported_native_handle_claim,
                           row.backend, row.chain_id,
                           "tone mapping evidence must not expose native renderer or RHI handles", row.source_index);
        }
        if (row.request_subjective_visual_quality_claim) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::unsupported_subjective_quality_claim,
                           row.backend, row.chain_id,
                           "subjective visual quality approval remains outside the deterministic evidence gate",
                           row.source_index);
        }
        if (is_host_gated_metal_row(row)) {
            continue;
        }
        if (!row.color_space_evidence_ready) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::missing_color_space_evidence,
                           row.backend, row.chain_id,
                           "tone mapping evidence requires explicit swapchain/output color-space proof",
                           row.source_index);
        }
        if (!row.resource_synchronization_evidence_ready) {
            add_diagnostic(
                plan, PostprocessToneMappingEvidenceDiagnosticCode::missing_resource_synchronization_evidence,
                row.backend, row.chain_id,
                "tone mapping evidence requires resource synchronization or layout-transition proof", row.source_index);
        }
        if (!row.shader_validation_evidence_ready) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::missing_shader_validation_evidence,
                           row.backend, row.chain_id, "tone mapping evidence requires shader artifact validation proof",
                           row.source_index);
        }
        if (!row.backend_validation_evidence_ready) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::missing_backend_validation_evidence,
                           row.backend, row.chain_id, "tone mapping evidence requires backend validation proof",
                           row.source_index);
        }
        if (!row.host_validated || row.host_gate_required) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::missing_host_validation_evidence,
                           row.backend, row.chain_id,
                           "tone mapping evidence requires host validation unless it is an explicit Metal host gate",
                           row.source_index);
        }
    }
}

void validate_backend_rows(PostprocessToneMappingEvidencePlan& plan,
                           const PostprocessToneMappingEvidenceRequest& request) {
    for (const auto backend : request.required_backends) {
        if (!is_supported_tone_mapping_backend(backend)) {
            continue;
        }
        const auto has_backend_row = std::ranges::any_of(request.rows, [backend](const auto& row) {
            return row.backend == backend && is_valid_id(row.chain_id) && !has_native_token(row.chain_id);
        });
        if (!has_backend_row) {
            add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::missing_backend_row, backend, {},
                           "each required backend needs its own tone mapping evidence row", 0U);
        }
    }
}

void validate_tone_mapping_budgets(PostprocessToneMappingEvidencePlan& plan,
                                   const PostprocessToneMappingEvidenceRequest& request) {
    if (request_row_count(request) > request.row_budget) {
        add_diagnostic(plan, PostprocessToneMappingEvidenceDiagnosticCode::row_budget_exceeded, rhi::BackendKind::null,
                       {}, "tone mapping evidence request exceeds its row budget", 0U);
    }
}

void append_tone_mapping_output_rows(PostprocessToneMappingEvidencePlan& plan,
                                     const PostprocessToneMappingEvidenceRequest& request) {
    plan.required_backends = request.required_backends;
    sort_required_backends(plan.required_backends);
    plan.rows = request.rows;
    sort_tone_mapping_rows(plan.rows);
    plan.row_count = plan.rows.size();
    plan.host_gated_row_count = static_cast<std::size_t>(
        std::ranges::count_if(plan.rows, [](const auto& row) { return is_host_gated_metal_row(row); }));
    plan.ready_row_count = static_cast<std::size_t>(
        std::ranges::count_if(plan.rows, [](const auto& row) { return is_ready_tone_mapping_row(row); }));
}

void hash_mix(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value;
    hash *= 1099511628211ULL;
}

void hash_signed(std::uint64_t& hash, std::int64_t value) noexcept {
    hash_mix(hash, static_cast<std::uint64_t>(value));
}

void hash_string(std::uint64_t& hash, std::string_view value) noexcept {
    for (const auto ch : value) {
        hash_mix(hash, static_cast<unsigned char>(ch));
    }
    hash_mix(hash, 0xffU);
}

[[nodiscard]] std::uint64_t compute_replay_hash(const PostprocessToneMappingEvidencePlan& plan,
                                                const PostprocessToneMappingEvidenceRequest& request) {
    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, request.seed);
    for (const auto backend : plan.required_backends) {
        hash_mix(hash, static_cast<std::uint8_t>(backend));
    }
    for (const auto& row : plan.rows) {
        hash_string(hash, row.chain_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, static_cast<std::uint8_t>(row.tone_mapping_operator));
        hash_mix(hash, static_cast<std::uint8_t>(row.input_transfer));
        hash_mix(hash, static_cast<std::uint8_t>(row.output_transfer));
        const auto exposure_bias_centi_ev =
            static_cast<std::int32_t>(std::lround(static_cast<double>(row.exposure_bias_ev) * 100.0));
        hash_signed(hash, exposure_bias_centi_ev);
        hash_mix(hash, row.paper_white_nits);
        hash_mix(hash, row.max_content_nits);
        hash_mix(hash, row.display_max_nits);
        hash_mix(hash, row.hdr_input_available ? 1U : 0U);
        hash_mix(hash, row.color_space_evidence_ready ? 1U : 0U);
        hash_mix(hash, row.resource_synchronization_evidence_ready ? 1U : 0U);
        hash_mix(hash, row.shader_validation_evidence_ready ? 1U : 0U);
        hash_mix(hash, row.backend_validation_evidence_ready ? 1U : 0U);
        hash_mix(hash, row.host_validated ? 1U : 0U);
        hash_mix(hash, row.host_gate_required ? 1U : 0U);
        hash_mix(hash, row.request_native_handle_access ? 1U : 0U);
        hash_mix(hash, row.request_subjective_visual_quality_claim ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    return hash == 0U ? 1U : hash;
}

void compute_tone_mapping_backend_readiness(PostprocessToneMappingEvidencePlan& plan) {
    for (const auto backend : plan.required_backends) {
        bool has_backend_row{false};
        bool all_rows_ready{true};
        bool any_host_gated{false};
        for (const auto& row : plan.rows) {
            if (row.backend != backend) {
                continue;
            }
            has_backend_row = true;
            const auto row_ready = is_ready_tone_mapping_row(row);
            const auto row_host_gated = is_host_gated_metal_row(row);
            all_rows_ready = all_rows_ready && row_ready;
            any_host_gated = any_host_gated || row_host_gated;
        }
        const auto backend_ready = has_backend_row && all_rows_ready;
        if (backend_ready) {
            ++plan.host_validated_backend_count;
        }
        if (backend == rhi::BackendKind::d3d12) {
            plan.d3d12_tone_mapping_ready = backend_ready;
        } else if (backend == rhi::BackendKind::vulkan) {
            plan.vulkan_strict_tone_mapping_ready = backend_ready;
        } else if (backend == rhi::BackendKind::metal) {
            plan.requires_metal_host_evidence = true;
            plan.metal_tone_mapping_ready = backend_ready;
            plan.has_metal_host_evidence = backend_ready && !any_host_gated;
        }
    }
}

} // namespace

PostprocessChainPolicyPlan plan_postprocess_chain_policy(const PostprocessChainPolicyDesc& desc) {
    PostprocessChainPolicyPlan plan;
    plan.frame_extent = desc.frame_extent;
    plan.backend = desc.backend;
    plan.backend_shader_evidence_ready = desc.backend_shader_evidence_ready;
    const auto enabled_effect_count = static_cast<std::uint32_t>(
        std::ranges::count_if(desc.effects, [](const PostprocessEffectDesc& effect) { return effect.enabled; }));
    plan.scene_color_required = enabled_effect_count != 0;

    if (desc.frame_extent.width == 0 || desc.frame_extent.height == 0) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::invalid_frame_extent, 0, 0,
                       "postprocess chain policy requires a non-zero frame extent");
    }
    if (enabled_effect_count == 0) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::no_effects, 0, 0,
                       "postprocess chain policy requires at least one enabled effect");
    }
    if (desc.max_effect_count == 0) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::too_many_effects, 0, 0,
                       "postprocess chain policy max_effect_count must be greater than zero");
    }
    if (desc.max_postprocess_pass_count == 0) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::too_many_postprocess_passes, 0, 0,
                       "postprocess chain policy max_postprocess_pass_count must be greater than zero");
    }
    if (enabled_effect_count > desc.max_effect_count) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::too_many_effects, enabled_effect_count, 0,
                       "postprocess chain policy exceeds max_effect_count");
    }
    if (plan.scene_color_required && !desc.scene_color_available) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::missing_scene_color, 0, 0,
                       "postprocess chain policy requires scene_color before postprocess execution");
    }
    if (desc.require_backend_shader_evidence && !desc.backend_shader_evidence_ready) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::missing_backend_shader_evidence, 0, 0,
                       std::string{"postprocess chain policy requires shader evidence for "} +
                           std::string{backend_name(desc.backend)});
    }

    bool bloom_requested = false;
    for (std::size_t index = 0; index < desc.effects.size(); ++index) {
        const auto& effect = desc.effects[index];
        if (!effect.enabled) {
            continue;
        }

        if (!is_supported_effect(effect.kind)) {
            add_diagnostic(plan, PostprocessChainDiagnosticCode::unsupported_effect, index, effect.source_index,
                           std::string{"unsupported postprocess effect "} + std::string{effect_name(effect.kind)});
            continue;
        }
        if (has_duplicate_kind(plan.effect_rows, effect.kind)) {
            add_diagnostic(plan, PostprocessChainDiagnosticCode::duplicate_effect, index, effect.source_index,
                           std::string{"duplicate postprocess effect "} + std::string{effect_name(effect.kind)});
            continue;
        }
        if (!std::isfinite(effect.intensity) || effect.intensity < 0.0F) {
            add_diagnostic(plan, PostprocessChainDiagnosticCode::invalid_effect_intensity, index, effect.source_index,
                           std::string{"postprocess effect "} + std::string{effect_name(effect.kind)} +
                               " requires finite non-negative intensity");
            continue;
        }
        if (effect.kind == PostprocessEffectKind::bloom) {
            if (effect.bloom_iterations == 0 || effect.bloom_iterations > 8) {
                add_diagnostic(plan, PostprocessChainDiagnosticCode::invalid_bloom_iterations, index,
                               effect.source_index, "postprocess bloom requires bloom_iterations in the range 1..8");
                continue;
            }
            bloom_requested = true;
        }
        if (effect.kind == PostprocessEffectKind::anti_aliasing && !is_supported_anti_aliasing(effect.anti_aliasing)) {
            add_diagnostic(plan, PostprocessChainDiagnosticCode::unsupported_anti_aliasing_mode, index,
                           effect.source_index, "postprocess anti-aliasing currently supports none or fxaa only");
            continue;
        }

        const bool depth_required = uses_depth(effect);
        if (depth_required) {
            plan.scene_depth_required = true;
            if (!desc.scene_depth_available) {
                add_diagnostic(plan, PostprocessChainDiagnosticCode::missing_scene_depth, index, effect.source_index,
                               std::string{"postprocess effect "} + std::string{effect_name(effect.kind)} +
                                   " requires scene depth input");
            }
        }

        plan.effect_rows.push_back(PostprocessEffectRow{
            .kind = effect.kind,
            .intensity = effect.intensity,
            .bloom_iterations = effect.kind == PostprocessEffectKind::bloom ? effect.bloom_iterations : 0,
            .anti_aliasing = effect.kind == PostprocessEffectKind::anti_aliasing ? effect.anti_aliasing
                                                                                 : PostprocessAntiAliasingMode::none,
            .uses_scene_depth = depth_required,
            .pass_index = 0,
            .source_index = effect.source_index,
        });
    }

    plan.effect_count = static_cast<std::uint32_t>(plan.effect_rows.size());

    plan.bloom_work_texture_required = bloom_requested;
    plan.postprocess_pass_count = plan.effect_count == 0 ? 0U : bloom_requested ? 2U : 1U;
    if (plan.postprocess_pass_count > desc.max_postprocess_pass_count) {
        add_diagnostic(plan, PostprocessChainDiagnosticCode::too_many_postprocess_passes, 0, 0,
                       "postprocess chain policy exceeds max_postprocess_pass_count");
    }
    if (plan.postprocess_pass_count != 0) {
        plan.frame_graph_pass_count = 1U + plan.postprocess_pass_count;
        plan.frame_graph_barrier_step_budget = plan.postprocess_pass_count * 2U;
    }
    if (plan.postprocess_pass_count > 1U) {
        for (auto& row : plan.effect_rows) {
            if (row.kind == PostprocessEffectKind::anti_aliasing || row.kind == PostprocessEffectKind::color_grading) {
                row.pass_index = plan.postprocess_pass_count - 1U;
            }
        }
    }
    return plan;
}

bool has_postprocess_chain_policy_effect(const PostprocessChainPolicyPlan& plan, PostprocessEffectKind kind) noexcept {
    return std::ranges::any_of(plan.effect_rows, [kind](const PostprocessEffectRow& row) { return row.kind == kind; });
}

bool has_postprocess_chain_policy_diagnostic(const PostprocessChainPolicyPlan& plan,
                                             PostprocessChainDiagnosticCode code) noexcept {
    return std::ranges::any_of(
        plan.diagnostics, [code](const PostprocessChainDiagnostic& diagnostic) { return diagnostic.code == code; });
}

bool PostprocessToneMappingEvidencePlan::succeeded() const noexcept {
    return status == PostprocessToneMappingEvidenceStatus::ready ||
           status == PostprocessToneMappingEvidenceStatus::no_rows;
}

PostprocessToneMappingEvidencePlan
plan_postprocess_tone_mapping_evidence(const PostprocessToneMappingEvidenceRequest& request) {
    PostprocessToneMappingEvidencePlan plan;
    if (request_row_count(request) == 0U) {
        plan.status = PostprocessToneMappingEvidenceStatus::no_rows;
        return plan;
    }

    validate_required_backends(plan, request);
    validate_duplicate_backend_chains(plan, request);
    validate_tone_mapping_rows(plan, request);
    validate_backend_rows(plan, request);
    validate_tone_mapping_budgets(plan, request);

    if (!plan.diagnostics.empty()) {
        plan.rejected_unsafe_row_count = count_rejected_rows(plan);
        sort_diagnostics(plan);
        plan.status = PostprocessToneMappingEvidenceStatus::invalid_request;
        return plan;
    }

    append_tone_mapping_output_rows(plan, request);
    compute_tone_mapping_backend_readiness(plan);
    plan.replay_hash = compute_replay_hash(plan, request);
    plan.status = plan.host_gated_row_count == 0U ? PostprocessToneMappingEvidenceStatus::ready
                                                  : PostprocessToneMappingEvidenceStatus::host_evidence_required;
    return plan;
}

bool has_postprocess_tone_mapping_evidence_diagnostic(const PostprocessToneMappingEvidencePlan& plan,
                                                      PostprocessToneMappingEvidenceDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
