// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/sprite_atlas_residency.hpp"

#include <cmath>
#include <string_view>
#include <unordered_set>

namespace mirakana::runtime {
namespace {

struct PageValidationResult {
    RuntimeSpriteAtlasPageRow row;
    bool valid{true};
};

void add_diagnostic(RuntimeSpriteAtlasResidencyPlan& plan, const RuntimeSpriteAtlasResidencyRequest& request,
                    RuntimeSpriteAtlasResidencyDiagnosticCode code, std::string page_id, AssetId texture_asset,
                    std::string message) {
    plan.diagnostics.push_back(RuntimeSpriteAtlasResidencyDiagnostic{
        .code = code,
        .atlas_id = request.atlas_id,
        .page_id = std::move(page_id),
        .texture_asset = texture_asset,
        .message = std::move(message),
    });
}

[[nodiscard]] bool uv_rect_valid(const RuntimeSpriteAtlasPageRow& page) noexcept {
    return std::isfinite(page.u0) && std::isfinite(page.v0) && std::isfinite(page.u1) && std::isfinite(page.v1) &&
           page.u0 >= 0.0F && page.v0 >= 0.0F && page.u1 <= 1.0F && page.v1 <= 1.0F && page.u0 < page.u1 &&
           page.v0 < page.v1;
}

[[nodiscard]] bool padding_valid(const RuntimeSpriteAtlasPageRow& page) noexcept {
    if (page.padding_pixels == 0U) {
        return true;
    }
    return page.padding_pixels < page.width && page.padding_pixels < page.height &&
           page.padding_pixels < (page.width - page.padding_pixels) &&
           page.padding_pixels < (page.height - page.padding_pixels);
}

[[nodiscard]] std::uint64_t texture_resident_bytes(const RuntimeTexturePayload& payload) noexcept {
    return static_cast<std::uint64_t>(payload.width) * static_cast<std::uint64_t>(payload.height) *
           static_cast<std::uint64_t>(texture_source_bytes_per_pixel(payload.pixel_format));
}

[[nodiscard]] RuntimeSpriteAtlasResidencyUploadHandoffRow make_handoff(std::string_view atlas_id,
                                                                       const RuntimeSpriteAtlasPageRow& page) {
    return RuntimeSpriteAtlasResidencyUploadHandoffRow{
        .handoff_id = std::string{atlas_id} + "/" + page.page_id + "/upload",
        .page_id = page.page_id,
        .texture_asset = page.texture_asset,
        .texture_resource = page.texture_resource,
        .texture_uri = page.texture_uri,
        .width = page.width,
        .height = page.height,
        .padding_pixels = page.padding_pixels,
        .u0 = page.u0,
        .v0 = page.v0,
        .u1 = page.u1,
        .v1 = page.v1,
        .resident_bytes = page.resident_bytes,
    };
}

void validate_request(RuntimeSpriteAtlasResidencyPlan& plan, const RuntimeSpriteAtlasResidencyRequest& request) {
    if (request.atlas_id.empty()) {
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::missing_atlas_id, {}, {},
                       "runtime sprite atlas residency requires an atlas id");
    }
    if (request.package == nullptr) {
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::missing_package, {}, {},
                       "runtime sprite atlas residency requires a loaded runtime package");
    }
    if (request.catalog == nullptr) {
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::missing_catalog, {}, {},
                       "runtime sprite atlas residency requires a live runtime resource catalog");
    }
    if (request.pages.empty()) {
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::missing_page, {}, {},
                       "runtime sprite atlas residency requires at least one page row");
    }
    if (request.request_runtime_source_decode) {
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::runtime_source_decode_requested, {},
                       {}, "runtime sprite atlas residency does not decode source images");
    }
    if (request.request_renderer_residency_ownership) {
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::renderer_residency_ownership_requested,
                       {}, {}, "runtime sprite atlas residency does not own renderer or RHI residency");
    }
    if (request.request_public_native_handle) {
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::public_native_handle_requested, {}, {},
                       "runtime sprite atlas residency does not expose native handles");
    }
}

[[nodiscard]] PageValidationResult validate_page(RuntimeSpriteAtlasResidencyPlan& plan,
                                                 const RuntimeSpriteAtlasResidencyRequest& request,
                                                 const RuntimeSpriteAtlasPageRow& input) {
    PageValidationResult result{.row = input, .valid = true};
    if (input.page_id.empty()) {
        result.valid = false;
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::missing_page_id, input.page_id,
                       input.texture_asset, "runtime sprite atlas page requires a page id");
    }
    if (input.texture_asset.value == 0U) {
        result.valid = false;
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::missing_texture_asset, input.page_id,
                       input.texture_asset, "runtime sprite atlas page requires a cooked texture asset");
    }
    if (input.width == 0U || input.height == 0U) {
        result.valid = false;
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::invalid_page_dimensions, input.page_id,
                       input.texture_asset, "runtime sprite atlas page dimensions must be positive");
    }
    if (!uv_rect_valid(input)) {
        result.valid = false;
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::invalid_uv_rect, input.page_id,
                       input.texture_asset, "runtime sprite atlas page uv rect must be finite and normalized");
    }
    if (!padding_valid(input)) {
        result.valid = false;
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::invalid_padding, input.page_id,
                       input.texture_asset, "runtime sprite atlas page padding must fit inside the page dimensions");
    }

    if (request.catalog != nullptr) {
        const auto* resource = runtime_resource_record_v2(*request.catalog, input.texture_resource);
        if (resource == nullptr || resource->asset != input.texture_asset || resource->kind != AssetKind::texture) {
            result.valid = false;
            add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::stale_resource_handle,
                           input.page_id, input.texture_asset,
                           "runtime sprite atlas page references a stale or mismatched texture resource handle");
        }
    }

    if (request.package == nullptr) {
        return result;
    }
    const RuntimeAssetRecord* record = request.package->find(input.texture_asset);
    if (record == nullptr) {
        result.valid = false;
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::missing_cooked_texture_payload,
                       input.page_id, input.texture_asset,
                       "runtime sprite atlas page texture asset is missing from the loaded package");
        return result;
    }

    const auto texture = runtime_texture_payload(*record);
    if (!texture.succeeded()) {
        result.valid = false;
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::invalid_cooked_texture_payload,
                       input.page_id, input.texture_asset, texture.diagnostic);
        return result;
    }
    if (texture.payload.width != input.width || texture.payload.height != input.height) {
        result.valid = false;
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::invalid_page_dimensions, input.page_id,
                       input.texture_asset,
                       "runtime sprite atlas page dimensions do not match the cooked texture payload");
        return result;
    }

    result.row.resident_bytes = texture_resident_bytes(texture.payload);
    return result;
}

} // namespace

bool RuntimeSpriteAtlasResidencyPlan::succeeded() const noexcept {
    return status == RuntimeSpriteAtlasResidencyStatus::ready && diagnostics.empty();
}

RuntimeSpriteAtlasResidencyPlan plan_runtime_sprite_atlas_residency(const RuntimeSpriteAtlasResidencyRequest& request) {
    RuntimeSpriteAtlasResidencyPlan plan;
    plan.invoked_runtime_source_decode = false;
    plan.requested_renderer_residency_ownership = request.request_renderer_residency_ownership;
    plan.requested_public_native_handle = request.request_public_native_handle;
    plan.invoked_renderer_upload = false;

    validate_request(plan, request);
    if (!plan.diagnostics.empty()) {
        plan.status = RuntimeSpriteAtlasResidencyStatus::invalid_request;
        return plan;
    }

    std::unordered_set<std::string> page_ids;
    plan.page_rows.reserve(request.pages.size());
    for (const auto& page : request.pages) {
        if (!page_ids.insert(page.page_id).second) {
            add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::duplicate_page_id, page.page_id,
                           page.texture_asset, "runtime sprite atlas page id is duplicated");
            continue;
        }

        auto validated = validate_page(plan, request, page);
        if (validated.valid) {
            plan.resident_bytes += validated.row.resident_bytes;
            plan.page_rows.push_back(std::move(validated.row));
        }
    }

    if (request.resident_byte_budget.has_value() && plan.resident_bytes > *request.resident_byte_budget) {
        add_diagnostic(plan, request, RuntimeSpriteAtlasResidencyDiagnosticCode::resident_byte_budget_exceeded, {}, {},
                       "runtime sprite atlas residency exceeds the resident byte budget");
        plan.status = RuntimeSpriteAtlasResidencyStatus::budget_exceeded;
        plan.page_rows.clear();
        return plan;
    }

    if (!plan.diagnostics.empty()) {
        plan.status = RuntimeSpriteAtlasResidencyStatus::diagnostics;
        plan.page_rows.clear();
        return plan;
    }

    plan.upload_handoff_rows.reserve(plan.page_rows.size());
    for (const auto& page : plan.page_rows) {
        plan.upload_handoff_rows.push_back(make_handoff(request.atlas_id, page));
    }
    plan.page_count = plan.page_rows.size();
    plan.upload_handoff_count = plan.upload_handoff_rows.size();
    plan.status = RuntimeSpriteAtlasResidencyStatus::ready;
    return plan;
}

} // namespace mirakana::runtime
