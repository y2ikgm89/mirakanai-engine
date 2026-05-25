// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/asset_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

struct RuntimeAddressableAssetId {
    std::string value;
};

enum class RuntimeAddressableLoadAction : std::uint8_t {
    load_asset = 0,
    load_dependency,
};

enum class RuntimeAddressableReleaseAction : std::uint8_t {
    release_asset = 0,
    release_dependency,
};

enum class RuntimeAddressableContentStreamingStatus : std::uint8_t {
    ready = 0,
    no_requests,
    budget_limited,
    invalid_request,
};

enum class RuntimeAddressableContentDiagnosticCode : std::uint8_t {
    missing_stream_id = 0,
    missing_address_id,
    duplicate_address_id,
    duplicate_address_asset,
    missing_package_asset,
    missing_dependency_asset,
    missing_dependency_address,
    unknown_load_address,
    unknown_release_address,
    duplicate_resident_address,
    release_underflow,
    resident_budget_exceeded,
};

struct RuntimeAddressableAssetRow {
    RuntimeAddressableAssetId address_id;
    AssetId asset;
    std::uint32_t source_index{0U};
};

struct RuntimeAddressableResidentAssetRow {
    RuntimeAddressableAssetId address_id;
    std::uint32_t ref_count{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeAddressableLoadRequest {
    RuntimeAddressableAssetId address_id;
    bool include_dependencies{true};
    std::uint32_t source_index{0U};
};

struct RuntimeAddressableReleaseRequest {
    RuntimeAddressableAssetId address_id;
    std::uint32_t release_count{1U};
    bool include_dependencies{false};
    std::uint32_t source_index{0U};
};

struct RuntimeAddressableResidentBudget {
    std::uint64_t max_resident_bytes{0U};
};

struct RuntimeAddressableContentStreamingRequest {
    std::string stream_id;
    RuntimeAssetPackage package;
    std::vector<RuntimeAddressableAssetRow> addressable_assets;
    std::vector<RuntimeAddressableResidentAssetRow> resident_assets;
    std::vector<RuntimeAddressableLoadRequest> load_requests;
    std::vector<RuntimeAddressableReleaseRequest> release_requests;
    RuntimeAddressableResidentBudget budget;
};

struct RuntimeAddressableContentDiagnostic {
    RuntimeAddressableContentDiagnosticCode code{RuntimeAddressableContentDiagnosticCode::missing_stream_id};
    std::string stream_id;
    RuntimeAddressableAssetId address_id;
    RuntimeAddressableAssetId dependency_address_id;
    AssetId asset;
    AssetId dependency;
    std::string message;
    std::uint32_t source_index{0U};
};

struct RuntimeAddressableDependencyRow {
    RuntimeAddressableAssetId address_id;
    RuntimeAddressableAssetId dependency_address_id;
    AssetId asset;
    AssetId dependency;
    AssetDependencyKind kind{AssetDependencyKind::unknown};
    std::string path;
};

struct RuntimeAddressableLoadRow {
    RuntimeAddressableLoadAction action{RuntimeAddressableLoadAction::load_asset};
    RuntimeAddressableAssetId address_id;
    AssetId asset;
    std::uint64_t resident_bytes{0U};
    std::uint32_t ref_count_before{0U};
    std::uint32_t ref_count_after{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeAddressableReleaseRow {
    RuntimeAddressableReleaseAction action{RuntimeAddressableReleaseAction::release_asset};
    RuntimeAddressableAssetId address_id;
    AssetId asset;
    std::uint64_t resident_bytes_released{0U};
    std::uint32_t ref_count_before{0U};
    std::uint32_t ref_count_after{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeAddressableContentStreamingPlan {
    RuntimeAddressableContentStreamingStatus status{RuntimeAddressableContentStreamingStatus::invalid_request};
    std::vector<RuntimeAddressableContentDiagnostic> diagnostics;
    std::vector<RuntimeAddressableAssetRow> address_rows;
    std::vector<RuntimeAddressableDependencyRow> dependency_rows;
    std::vector<RuntimeAddressableLoadRow> load_rows;
    std::vector<RuntimeAddressableReleaseRow> release_rows;
    std::size_t address_count{0U};
    std::size_t dependency_count{0U};
    std::size_t planned_load_count{0U};
    std::size_t planned_release_count{0U};
    std::uint64_t final_resident_bytes{0U};
    std::uint64_t resident_budget_bytes{0U};
    std::size_t budget_rejected_load_count{0U};
    bool committed{false};
    bool invoked_package_io{false};
    bool invoked_async_execution{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans addressable package/content handles, dependency closure, explicit load/release refcount rows, and coarse
/// resident-byte budget diagnostics. This value-only helper does not read packages, mutate resident state, execute
/// asynchronous work, create threads, upload renderer/RHI resources, or expose native handles.
[[nodiscard]] RuntimeAddressableContentStreamingPlan
plan_runtime_addressable_content_streaming(const RuntimeAddressableContentStreamingRequest& request);

} // namespace mirakana::runtime
