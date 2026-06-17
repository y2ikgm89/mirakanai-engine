// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/environment_commercial_readiness.hpp"

#include <array>
#include <span>
#include <utility>

namespace mirakana {
namespace {

constexpr std::array kRequiredKinds{
    EnvironmentCommercialReadinessRequirementKind::strict_vulkan_aggregate,
    EnvironmentCommercialReadinessRequirementKind::metal_host_aggregate,
    EnvironmentCommercialReadinessRequirementKind::backend_parity,
    EnvironmentCommercialReadinessRequirementKind::platform_windows_d3d12,
    EnvironmentCommercialReadinessRequirementKind::platform_windows_vulkan,
    EnvironmentCommercialReadinessRequirementKind::platform_linux_vulkan,
    EnvironmentCommercialReadinessRequirementKind::platform_macos_metal,
    EnvironmentCommercialReadinessRequirementKind::platform_ios_metal,
    EnvironmentCommercialReadinessRequirementKind::platform_android_vulkan,
    EnvironmentCommercialReadinessRequirementKind::broad_optimization,
    EnvironmentCommercialReadinessRequirementKind::asset_pipeline_openexr_ktx_basis,
    EnvironmentCommercialReadinessRequirementKind::aaa_preset_library,
    EnvironmentCommercialReadinessRequirementKind::physical_weather_simulation,
    EnvironmentCommercialReadinessRequirementKind::artist_workflow,
};

[[nodiscard]] const EnvironmentCommercialReadinessRequirementInputRow*
find_requirement(std::span<const EnvironmentCommercialReadinessRequirementInputRow> rows,
                 EnvironmentCommercialReadinessRequirementKind kind) noexcept {
    for (const auto& row : rows) {
        if (row.kind == kind) {
            return &row;
        }
    }
    return nullptr;
}

void hash_mix(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value;
    hash *= 1099511628211ULL;
}

void hash_text(std::uint64_t& hash, std::string_view text) noexcept {
    for (const auto c : text) {
        hash_mix(hash, static_cast<unsigned char>(c));
    }
}

[[nodiscard]] bool row_blocks_commercial(const EnvironmentCommercialReadinessRequirementRow& row) noexcept {
    return row.status == EnvironmentCommercialReadinessRequirementStatus::blocked ||
           row.status == EnvironmentCommercialReadinessRequirementStatus::unsupported ||
           row.status == EnvironmentCommercialReadinessRequirementStatus::missing || !row.package_visible ||
           !row.legal_notice_current || !row.validation_guarded || row.native_handle_access ||
           row.broad_environment_ready_claimed || row.evidence_id.empty() || row.package_counter.empty();
}

} // namespace

std::string_view environment_commercial_readiness_status_label(EnvironmentCommercialReadinessStatus status) noexcept {
    switch (status) {
    case EnvironmentCommercialReadinessStatus::ready:
        return "ready";
    case EnvironmentCommercialReadinessStatus::host_evidence_required:
        return "host_evidence_required";
    case EnvironmentCommercialReadinessStatus::blocked:
        return "blocked";
    }
    return "unknown";
}

std::string_view
environment_commercial_readiness_requirement_id(EnvironmentCommercialReadinessRequirementKind kind) noexcept {
    switch (kind) {
    case EnvironmentCommercialReadinessRequirementKind::strict_vulkan_aggregate:
        return "environment.commercial.strict_vulkan_aggregate";
    case EnvironmentCommercialReadinessRequirementKind::metal_host_aggregate:
        return "environment.commercial.metal_host_aggregate";
    case EnvironmentCommercialReadinessRequirementKind::backend_parity:
        return "environment.commercial.backend_parity";
    case EnvironmentCommercialReadinessRequirementKind::platform_windows_d3d12:
        return "environment.commercial.platform_windows_d3d12";
    case EnvironmentCommercialReadinessRequirementKind::platform_windows_vulkan:
        return "environment.commercial.platform_windows_vulkan";
    case EnvironmentCommercialReadinessRequirementKind::platform_linux_vulkan:
        return "environment.commercial.platform_linux_vulkan";
    case EnvironmentCommercialReadinessRequirementKind::platform_macos_metal:
        return "environment.commercial.platform_macos_metal";
    case EnvironmentCommercialReadinessRequirementKind::platform_ios_metal:
        return "environment.commercial.platform_ios_metal";
    case EnvironmentCommercialReadinessRequirementKind::platform_android_vulkan:
        return "environment.commercial.platform_android_vulkan";
    case EnvironmentCommercialReadinessRequirementKind::broad_optimization:
        return "environment.commercial.broad_optimization";
    case EnvironmentCommercialReadinessRequirementKind::asset_pipeline_openexr_ktx_basis:
        return "environment.commercial.asset_pipeline_openexr_ktx_basis";
    case EnvironmentCommercialReadinessRequirementKind::aaa_preset_library:
        return "environment.commercial.aaa_preset_library";
    case EnvironmentCommercialReadinessRequirementKind::physical_weather_simulation:
        return "environment.commercial.physical_weather_simulation";
    case EnvironmentCommercialReadinessRequirementKind::artist_workflow:
        return "environment.commercial.artist_workflow";
    }
    return "environment.commercial.unknown";
}

std::string_view environment_commercial_readiness_requirement_status_label(
    EnvironmentCommercialReadinessRequirementStatus status) noexcept {
    switch (status) {
    case EnvironmentCommercialReadinessRequirementStatus::ready:
        return "ready";
    case EnvironmentCommercialReadinessRequirementStatus::host_gated:
        return "host_gated";
    case EnvironmentCommercialReadinessRequirementStatus::blocked:
        return "blocked";
    case EnvironmentCommercialReadinessRequirementStatus::unsupported:
        return "unsupported";
    case EnvironmentCommercialReadinessRequirementStatus::missing:
        return "missing";
    }
    return "unknown";
}

EnvironmentCommercialReadinessPlan
plan_environment_commercial_readiness(const EnvironmentCommercialReadinessDesc& desc) {
    EnvironmentCommercialReadinessPlan plan{
        .required_row_count = static_cast<std::uint32_t>(kRequiredKinds.size()),
        .optional_dependency_legal_records_current = desc.optional_dependency_legal_records_current,
        .adjacent_broad_non_claims_declared = desc.adjacent_broad_non_claims_declared,
    };
    plan.rows.reserve(kRequiredKinds.size());

    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, 20260617U);
    hash_mix(hash, desc.optional_dependency_legal_records_current ? 1U : 0U);
    hash_mix(hash, desc.adjacent_broad_non_claims_declared ? 1U : 0U);
    hash_mix(hash, desc.request_commercial_ready ? 1U : 0U);

    for (const auto kind : kRequiredKinds) {
        const auto* input = find_requirement(desc.requirements, kind);
        EnvironmentCommercialReadinessRequirementRow row{
            .row_id = std::string{environment_commercial_readiness_requirement_id(kind)},
            .kind = kind,
        };
        if (input != nullptr) {
            row.status = input->status;
            row.evidence_id = input->evidence_id;
            row.package_counter = input->package_counter;
            row.package_visible = input->package_visible;
            row.legal_notice_current = input->legal_notice_current;
            row.validation_guarded = input->validation_guarded;
            row.native_handle_access = input->native_handle_access;
            row.broad_environment_ready_claimed = input->broad_environment_ready_claimed;
        }

        if (row.status == EnvironmentCommercialReadinessRequirementStatus::ready && row.package_visible &&
            row.legal_notice_current && row.validation_guarded && !row.native_handle_access &&
            !row.broad_environment_ready_claimed && !row.evidence_id.empty() && !row.package_counter.empty()) {
            ++plan.ready_row_count;
        }
        if (row.status == EnvironmentCommercialReadinessRequirementStatus::host_gated) {
            ++plan.host_gated_row_count;
        }
        if (row.status == EnvironmentCommercialReadinessRequirementStatus::missing) {
            ++plan.missing_row_count;
        }
        if (row.package_visible) {
            ++plan.package_visible_row_count;
        }
        if (row.validation_guarded) {
            ++plan.validation_guarded_row_count;
        }
        if (row.legal_notice_current) {
            ++plan.legal_notice_current_row_count;
        }
        if (row.native_handle_access) {
            ++plan.native_handle_access_count;
        }
        plan.broad_environment_ready_claimed =
            plan.broad_environment_ready_claimed || row.broad_environment_ready_claimed;
        if (row_blocks_commercial(row)) {
            ++plan.blocked_row_count;
        }

        hash_text(hash, row.row_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.status));
        hash_text(hash, row.evidence_id);
        hash_text(hash, row.package_counter);
        hash_mix(hash, row.package_visible ? 1U : 0U);
        hash_mix(hash, row.legal_notice_current ? 1U : 0U);
        hash_mix(hash, row.validation_guarded ? 1U : 0U);
        hash_mix(hash, row.native_handle_access ? 1U : 0U);
        hash_mix(hash, row.broad_environment_ready_claimed ? 1U : 0U);
        plan.rows.push_back(std::move(row));
    }

    if (!desc.optional_dependency_legal_records_current || !desc.adjacent_broad_non_claims_declared) {
        ++plan.blocked_row_count;
    }

    const bool all_rows_ready = plan.ready_row_count == plan.required_row_count && plan.host_gated_row_count == 0U &&
                                plan.blocked_row_count == 0U && plan.missing_row_count == 0U &&
                                plan.package_visible_row_count == plan.required_row_count &&
                                plan.validation_guarded_row_count == plan.required_row_count &&
                                plan.legal_notice_current_row_count == plan.required_row_count &&
                                desc.optional_dependency_legal_records_current &&
                                desc.adjacent_broad_non_claims_declared && plan.native_handle_access_count == 0U &&
                                !plan.broad_environment_ready_claimed;
    plan.environment_commercial_ready = desc.request_commercial_ready && all_rows_ready;
    if (plan.environment_commercial_ready) {
        plan.status = EnvironmentCommercialReadinessStatus::ready;
    } else if (plan.blocked_row_count == 0U && plan.host_gated_row_count > 0U) {
        plan.status = EnvironmentCommercialReadinessStatus::host_evidence_required;
    } else {
        plan.status = EnvironmentCommercialReadinessStatus::blocked;
    }
    plan.replay_hash = hash == 0U ? 1U : hash;
    return plan;
}

} // namespace mirakana
