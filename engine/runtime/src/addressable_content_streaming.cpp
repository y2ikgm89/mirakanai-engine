// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/addressable_content_streaming.hpp"

#include <algorithm>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

struct AddressLookupRow {
    RuntimeAddressableAssetId address_id;
    AssetId asset;
    std::uint64_t resident_bytes{0U};
};

struct ResidentRefCountRow {
    RuntimeAddressableAssetId address_id;
    AssetId asset;
    std::uint64_t resident_bytes{0U};
    std::uint32_t ref_count{0U};
};

[[nodiscard]] bool is_valid_stream_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool is_valid_address_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == ':';
    });
}

void add_diagnostic(RuntimeAddressableContentStreamingPlan& plan, RuntimeAddressableContentDiagnosticCode code,
                    std::string stream_id, RuntimeAddressableAssetId address_id,
                    RuntimeAddressableAssetId dependency_address_id, AssetId asset, AssetId dependency,
                    std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(RuntimeAddressableContentDiagnostic{
        .code = code,
        .stream_id = std::move(stream_id),
        .address_id = std::move(address_id),
        .dependency_address_id = std::move(dependency_address_id),
        .asset = asset,
        .dependency = dependency,
        .message = std::move(message),
        .source_index = source_index,
    });
}

[[nodiscard]] const RuntimeAddressableAssetRow* find_address_row(const std::vector<RuntimeAddressableAssetRow>& rows,
                                                                 std::string_view address_id) {
    const auto found =
        std::ranges::find_if(rows, [address_id](const auto& row) { return row.address_id.value == address_id; });
    return found == rows.end() ? nullptr : &*found;
}

[[nodiscard]] const RuntimeAddressableAssetRow*
find_address_row_by_asset(const std::vector<RuntimeAddressableAssetRow>& rows, AssetId asset) {
    const auto found = std::ranges::find_if(rows, [asset](const auto& row) { return row.asset == asset; });
    return found == rows.end() ? nullptr : &*found;
}

[[nodiscard]] const AddressLookupRow* find_lookup(const std::vector<AddressLookupRow>& rows,
                                                  std::string_view address_id) {
    const auto found =
        std::ranges::find_if(rows, [address_id](const auto& row) { return row.address_id.value == address_id; });
    return found == rows.end() ? nullptr : &*found;
}

[[nodiscard]] ResidentRefCountRow* find_ref_count(std::vector<ResidentRefCountRow>& rows, std::string_view address_id) {
    const auto found =
        std::ranges::find_if(rows, [address_id](const auto& row) { return row.address_id.value == address_id; });
    return found == rows.end() ? nullptr : &*found;
}

[[nodiscard]] bool contains_address(const std::vector<RuntimeAddressableAssetId>& rows, std::string_view address_id) {
    return std::ranges::find_if(rows, [address_id](const auto& row) { return row.value == address_id; }) != rows.end();
}

[[nodiscard]] bool contains_asset(const std::vector<AssetId>& rows, AssetId asset) {
    return std::ranges::find(rows, asset) != rows.end();
}

void sort_diagnostics(RuntimeAddressableContentStreamingPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.address_id.value != rhs.address_id.value) {
            return lhs.address_id.value < rhs.address_id.value;
        }
        if (lhs.dependency_address_id.value != rhs.dependency_address_id.value) {
            return lhs.dependency_address_id.value < rhs.dependency_address_id.value;
        }
        if (lhs.asset.value != rhs.asset.value) {
            return lhs.asset.value < rhs.asset.value;
        }
        if (lhs.dependency.value != rhs.dependency.value) {
            return lhs.dependency.value < rhs.dependency.value;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void sort_plan_rows(RuntimeAddressableContentStreamingPlan& plan) {
    std::ranges::sort(plan.address_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.address_id.value != rhs.address_id.value) {
            return lhs.address_id.value < rhs.address_id.value;
        }
        return lhs.source_index < rhs.source_index;
    });
    std::ranges::sort(plan.dependency_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.address_id.value != rhs.address_id.value) {
            return lhs.address_id.value < rhs.address_id.value;
        }
        if (lhs.dependency_address_id.value != rhs.dependency_address_id.value) {
            return lhs.dependency_address_id.value < rhs.dependency_address_id.value;
        }
        if (lhs.kind != rhs.kind) {
            return static_cast<std::uint8_t>(lhs.kind) < static_cast<std::uint8_t>(rhs.kind);
        }
        return lhs.path < rhs.path;
    });
    std::ranges::sort(plan.load_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.address_id.value != rhs.address_id.value) {
            return lhs.address_id.value < rhs.address_id.value;
        }
        if (lhs.action != rhs.action) {
            return static_cast<std::uint8_t>(lhs.action) < static_cast<std::uint8_t>(rhs.action);
        }
        return lhs.source_index < rhs.source_index;
    });
    std::ranges::sort(plan.release_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.address_id.value != rhs.address_id.value) {
            return lhs.address_id.value < rhs.address_id.value;
        }
        if (lhs.action != rhs.action) {
            return static_cast<std::uint8_t>(lhs.action) < static_cast<std::uint8_t>(rhs.action);
        }
        return lhs.source_index < rhs.source_index;
    });
}

[[nodiscard]] std::uint64_t record_resident_bytes(const RuntimeAssetRecord& record) noexcept {
    return static_cast<std::uint64_t>(record.content.size());
}

[[nodiscard]] std::vector<AddressLookupRow>
validate_address_rows(RuntimeAddressableContentStreamingPlan& plan,
                      const RuntimeAddressableContentStreamingRequest& request) {
    std::vector<AddressLookupRow> lookup_rows;
    lookup_rows.reserve(request.addressable_assets.size());

    if (!is_valid_stream_id(request.stream_id)) {
        add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::missing_stream_id, request.stream_id, {}, {}, {},
                       {}, "runtime addressable content stream id must be non-empty and path-safe", 0U);
    }

    std::vector<RuntimeAddressableAssetId> seen_addresses;
    seen_addresses.reserve(request.addressable_assets.size());
    std::vector<AssetId> seen_assets;
    seen_assets.reserve(request.addressable_assets.size());
    for (const auto& row : request.addressable_assets) {
        if (!is_valid_address_id(row.address_id.value)) {
            add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::missing_address_id, request.stream_id,
                           row.address_id, {}, row.asset, {},
                           "runtime addressable asset id must be non-empty and address-safe", row.source_index);
            continue;
        }
        if (contains_address(seen_addresses, row.address_id.value)) {
            add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::duplicate_address_id, request.stream_id,
                           row.address_id, {}, row.asset, {}, "runtime addressable asset ids must be unique",
                           row.source_index);
            continue;
        }
        seen_addresses.push_back(row.address_id);

        const auto* record = request.package.find(row.asset);
        if (record == nullptr) {
            add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::missing_package_asset, request.stream_id,
                           row.address_id, {}, row.asset, {},
                           "runtime addressable asset must reference a record in the reviewed package",
                           row.source_index);
            continue;
        }
        if (contains_asset(seen_assets, row.asset)) {
            add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::duplicate_address_asset, request.stream_id,
                           row.address_id, {}, row.asset, {},
                           "runtime addressable asset rows must map one address to one package asset",
                           row.source_index);
            continue;
        }
        seen_assets.push_back(row.asset);

        lookup_rows.push_back(AddressLookupRow{
            .address_id = row.address_id,
            .asset = row.asset,
            .resident_bytes = record_resident_bytes(*record),
        });
    }

    return lookup_rows;
}

void validate_dependency_rows(RuntimeAddressableContentStreamingPlan& plan,
                              const RuntimeAddressableContentStreamingRequest& request) {
    for (const auto& edge : request.package.dependency_edges()) {
        const auto* asset_address = find_address_row_by_asset(request.addressable_assets, edge.asset);
        const auto* dependency_address = find_address_row_by_asset(request.addressable_assets, edge.dependency);
        if (request.package.find(edge.asset) == nullptr || request.package.find(edge.dependency) == nullptr) {
            add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::missing_dependency_asset, request.stream_id,
                           asset_address == nullptr ? RuntimeAddressableAssetId{} : asset_address->address_id,
                           dependency_address == nullptr ? RuntimeAddressableAssetId{} : dependency_address->address_id,
                           edge.asset, edge.dependency,
                           "runtime addressable dependency edge must reference package records", 0U);
            continue;
        }
        if (asset_address == nullptr || dependency_address == nullptr) {
            add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::missing_dependency_address, request.stream_id,
                           asset_address == nullptr ? RuntimeAddressableAssetId{} : asset_address->address_id,
                           dependency_address == nullptr ? RuntimeAddressableAssetId{} : dependency_address->address_id,
                           edge.asset, edge.dependency,
                           "runtime addressable dependency edge must have address rows for both assets", 0U);
            continue;
        }

        plan.dependency_rows.push_back(RuntimeAddressableDependencyRow{
            .address_id = asset_address->address_id,
            .dependency_address_id = dependency_address->address_id,
            .asset = edge.asset,
            .dependency = edge.dependency,
            .kind = edge.kind,
            .path = edge.path,
        });
    }
}

void validate_request_rows(RuntimeAddressableContentStreamingPlan& plan,
                           const RuntimeAddressableContentStreamingRequest& request) {
    std::vector<RuntimeAddressableAssetId> resident_addresses;
    resident_addresses.reserve(request.resident_assets.size());
    for (const auto& row : request.resident_assets) {
        if (find_address_row(request.addressable_assets, row.address_id.value) == nullptr) {
            add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::unknown_load_address, request.stream_id,
                           row.address_id, {}, {}, {},
                           "runtime addressable resident row must reference a known address", row.source_index);
        }
        if (contains_address(resident_addresses, row.address_id.value)) {
            add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::duplicate_resident_address, request.stream_id,
                           row.address_id, {}, {}, {}, "runtime addressable resident rows must be unique",
                           row.source_index);
        } else {
            resident_addresses.push_back(row.address_id);
        }
    }

    for (const auto& row : request.load_requests) {
        if (find_address_row(request.addressable_assets, row.address_id.value) == nullptr) {
            add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::unknown_load_address, request.stream_id,
                           row.address_id, {}, {}, {},
                           "runtime addressable load request must reference a known address", row.source_index);
        }
    }

    for (const auto& row : request.release_requests) {
        if (find_address_row(request.addressable_assets, row.address_id.value) == nullptr) {
            add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::unknown_release_address, request.stream_id,
                           row.address_id, {}, {}, {},
                           "runtime addressable release request must reference a known address", row.source_index);
        }
    }
}

[[nodiscard]] std::vector<RuntimeAddressableAssetId>
dependency_closure_for(const RuntimeAddressableContentStreamingPlan& plan, std::string_view address_id) {
    std::vector<RuntimeAddressableAssetId> result;
    std::vector<RuntimeAddressableAssetId> visited{RuntimeAddressableAssetId{.value = std::string(address_id)}};
    std::vector<RuntimeAddressableAssetId> pending{RuntimeAddressableAssetId{.value = std::string(address_id)}};

    while (!pending.empty()) {
        const auto current = std::move(pending.back());
        pending.pop_back();
        for (const auto& dependency : plan.dependency_rows) {
            if (dependency.address_id.value == current.value &&
                !contains_address(visited, dependency.dependency_address_id.value)) {
                visited.push_back(dependency.dependency_address_id);
                result.push_back(dependency.dependency_address_id);
                pending.push_back(dependency.dependency_address_id);
            }
        }
    }

    std::ranges::sort(result, [](const auto& lhs, const auto& rhs) { return lhs.value < rhs.value; });
    return result;
}

[[nodiscard]] std::vector<ResidentRefCountRow>
seed_resident_rows(const RuntimeAddressableContentStreamingRequest& request,
                   const std::vector<AddressLookupRow>& lookup) {
    std::vector<ResidentRefCountRow> rows;
    rows.reserve(request.resident_assets.size() + request.load_requests.size());

    for (const auto& resident : request.resident_assets) {
        const auto* address = find_lookup(lookup, resident.address_id.value);
        if (address != nullptr) {
            rows.push_back(ResidentRefCountRow{
                .address_id = resident.address_id,
                .asset = address->asset,
                .resident_bytes = address->resident_bytes,
                .ref_count = resident.ref_count,
            });
        }
    }

    return rows;
}

[[nodiscard]] ResidentRefCountRow& ensure_ref_count_row(std::vector<ResidentRefCountRow>& rows,
                                                        const AddressLookupRow& lookup) {
    auto* row = find_ref_count(rows, lookup.address_id.value);
    if (row != nullptr) {
        return *row;
    }

    rows.push_back(ResidentRefCountRow{
        .address_id = lookup.address_id,
        .asset = lookup.asset,
        .resident_bytes = lookup.resident_bytes,
        .ref_count = 0U,
    });
    return rows.back();
}

void append_load_row(RuntimeAddressableContentStreamingPlan& plan, std::vector<ResidentRefCountRow>& resident_rows,
                     const AddressLookupRow& lookup, RuntimeAddressableLoadAction action, std::uint32_t source_index) {
    auto& ref_count = ensure_ref_count_row(resident_rows, lookup);
    const auto before = ref_count.ref_count;
    if (ref_count.ref_count < std::numeric_limits<std::uint32_t>::max()) {
        ++ref_count.ref_count;
    }
    plan.load_rows.push_back(RuntimeAddressableLoadRow{
        .action = action,
        .address_id = lookup.address_id,
        .asset = lookup.asset,
        .resident_bytes = lookup.resident_bytes,
        .ref_count_before = before,
        .ref_count_after = ref_count.ref_count,
        .source_index = source_index,
    });
}

[[nodiscard]] bool append_release_row(RuntimeAddressableContentStreamingPlan& plan,
                                      std::vector<ResidentRefCountRow>& resident_rows, std::string_view stream_id,
                                      const AddressLookupRow& lookup, RuntimeAddressableReleaseAction action,
                                      std::uint32_t release_count, std::uint32_t source_index) {
    auto& ref_count = ensure_ref_count_row(resident_rows, lookup);
    const auto before = ref_count.ref_count;
    if (before < release_count) {
        add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::release_underflow, std::string{stream_id},
                       lookup.address_id, {}, lookup.asset, {},
                       "runtime addressable release count exceeds current resident refcount", source_index);
        return false;
    }

    ref_count.ref_count -= release_count;
    plan.release_rows.push_back(RuntimeAddressableReleaseRow{
        .action = action,
        .address_id = lookup.address_id,
        .asset = lookup.asset,
        .resident_bytes_released = ref_count.ref_count == 0U ? lookup.resident_bytes : 0U,
        .ref_count_before = before,
        .ref_count_after = ref_count.ref_count,
        .source_index = source_index,
    });
    return true;
}

void plan_loads(RuntimeAddressableContentStreamingPlan& plan, std::vector<ResidentRefCountRow>& resident_rows,
                const RuntimeAddressableContentStreamingRequest& request, const std::vector<AddressLookupRow>& lookup) {
    for (const auto& load : request.load_requests) {
        const auto* address = find_lookup(lookup, load.address_id.value);
        if (address == nullptr) {
            continue;
        }

        append_load_row(plan, resident_rows, *address, RuntimeAddressableLoadAction::load_asset, load.source_index);
        if (!load.include_dependencies) {
            continue;
        }

        for (const auto& dependency_address_id : dependency_closure_for(plan, load.address_id.value)) {
            const auto* dependency = find_lookup(lookup, dependency_address_id.value);
            if (dependency != nullptr) {
                append_load_row(plan, resident_rows, *dependency, RuntimeAddressableLoadAction::load_dependency,
                                load.source_index);
            }
        }
    }
}

void plan_releases(RuntimeAddressableContentStreamingPlan& plan, std::vector<ResidentRefCountRow>& resident_rows,
                   const RuntimeAddressableContentStreamingRequest& request,
                   const std::vector<AddressLookupRow>& lookup) {
    for (const auto& release : request.release_requests) {
        const auto* address = find_lookup(lookup, release.address_id.value);
        if (address == nullptr) {
            continue;
        }

        const bool released_root = append_release_row(plan, resident_rows, request.stream_id, *address,
                                                      RuntimeAddressableReleaseAction::release_asset,
                                                      release.release_count, release.source_index);
        if (!released_root || !release.include_dependencies) {
            continue;
        }

        for (const auto& dependency_address_id : dependency_closure_for(plan, release.address_id.value)) {
            const auto* dependency = find_lookup(lookup, dependency_address_id.value);
            if (dependency != nullptr) {
                (void)append_release_row(plan, resident_rows, request.stream_id, *dependency,
                                         RuntimeAddressableReleaseAction::release_dependency, release.release_count,
                                         release.source_index);
            }
        }
    }
}

[[nodiscard]] std::uint64_t final_resident_bytes(const std::vector<ResidentRefCountRow>& resident_rows) {
    std::uint64_t bytes{0U};
    for (const auto& row : resident_rows) {
        if (row.ref_count > 0U) {
            bytes += row.resident_bytes;
        }
    }
    return bytes;
}

} // namespace

bool RuntimeAddressableContentStreamingPlan::succeeded() const noexcept {
    return status == RuntimeAddressableContentStreamingStatus::ready ||
           status == RuntimeAddressableContentStreamingStatus::no_requests;
}

RuntimeAddressableContentStreamingPlan
plan_runtime_addressable_content_streaming(const RuntimeAddressableContentStreamingRequest& request) {
    RuntimeAddressableContentStreamingPlan plan;
    plan.address_rows = request.addressable_assets;
    plan.resident_budget_bytes = request.budget.max_resident_bytes;

    const auto lookup = validate_address_rows(plan, request);
    validate_dependency_rows(plan, request);
    validate_request_rows(plan, request);
    sort_plan_rows(plan);
    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan);
        plan.load_rows.clear();
        plan.release_rows.clear();
        plan.status = RuntimeAddressableContentStreamingStatus::invalid_request;
        return plan;
    }

    auto resident_rows = seed_resident_rows(request, lookup);
    plan_loads(plan, resident_rows, request, lookup);
    plan_releases(plan, resident_rows, request, lookup);
    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan);
        plan.load_rows.clear();
        plan.release_rows.clear();
        plan.status = RuntimeAddressableContentStreamingStatus::invalid_request;
        return plan;
    }

    plan.final_resident_bytes = final_resident_bytes(resident_rows);
    plan.address_count = plan.address_rows.size();
    plan.dependency_count = plan.dependency_rows.size();
    plan.planned_load_count = plan.load_rows.size();
    plan.planned_release_count = plan.release_rows.size();

    if (plan.resident_budget_bytes > 0U && plan.final_resident_bytes > plan.resident_budget_bytes) {
        plan.budget_rejected_load_count = request.load_requests.size();
        add_diagnostic(plan, RuntimeAddressableContentDiagnosticCode::resident_budget_exceeded, request.stream_id, {},
                       {}, {}, {}, "runtime addressable final resident byte estimate exceeds the requested budget", 0U);
        sort_diagnostics(plan);
        sort_plan_rows(plan);
        plan.status = RuntimeAddressableContentStreamingStatus::budget_limited;
        return plan;
    }

    sort_plan_rows(plan);
    plan.status = plan.load_rows.empty() && plan.release_rows.empty()
                      ? RuntimeAddressableContentStreamingStatus::no_requests
                      : RuntimeAddressableContentStreamingStatus::ready;
    return plan;
}

} // namespace mirakana::runtime
