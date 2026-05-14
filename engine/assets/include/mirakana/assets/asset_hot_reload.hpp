// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_dependency_graph.hpp"
#include "mirakana/assets/asset_registry.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mirakana {

struct AssetFileSnapshot {
    AssetId asset;
    std::string path;
    std::uint64_t revision{0};
    std::uint64_t size_bytes{0};
};

enum class AssetHotReloadEventKind : std::uint8_t { unknown, added, modified, removed };

struct AssetHotReloadEvent {
    AssetHotReloadEventKind kind{AssetHotReloadEventKind::unknown};
    AssetId asset;
    std::string path;
    std::uint64_t previous_revision{0};
    std::uint64_t current_revision{0};
    std::uint64_t previous_size_bytes{0};
    std::uint64_t current_size_bytes{0};
};

enum class AssetHotReloadRecookReason : std::uint8_t {
    unknown,
    source_added,
    source_modified,
    source_removed,
    dependency_invalidated,
};

struct AssetHotReloadRecookRequest {
    AssetId asset;
    AssetId source_asset;
    std::string trigger_path;
    AssetHotReloadEventKind trigger_event_kind{AssetHotReloadEventKind::unknown};
    AssetHotReloadRecookReason reason{AssetHotReloadRecookReason::unknown};
    std::uint64_t previous_revision{0};
    std::uint64_t current_revision{0};
    std::uint64_t ready_tick{0};
};

struct AssetHotReloadRecookSchedulerDesc {
    std::uint64_t debounce_ticks{2};
};

class AssetHotReloadRecookScheduler final {
  public:
    explicit AssetHotReloadRecookScheduler(AssetHotReloadRecookSchedulerDesc desc = {});

    void enqueue(std::vector<AssetHotReloadEvent> events, const AssetDependencyGraph& dependencies,
                 std::uint64_t now_tick);
    [[nodiscard]] std::vector<AssetHotReloadRecookRequest> ready(std::uint64_t now_tick);
    void clear() noexcept;

    [[nodiscard]] const AssetHotReloadRecookRequest* find_pending(AssetId asset) const noexcept;
    [[nodiscard]] std::size_t pending_count() const noexcept;

  private:
    AssetHotReloadRecookSchedulerDesc desc_;
    std::unordered_map<AssetId, AssetHotReloadRecookRequest, AssetIdHash> pending_by_asset_;
};

enum class AssetHotReloadApplyResultKind : std::uint8_t {
    unknown,
    staged,
    applied,
    failed_rolled_back,
};

struct AssetHotReloadAppliedAsset {
    AssetId asset;
    std::string path;
    std::uint64_t revision{0};
};

struct AssetHotReloadApplyResult {
    AssetHotReloadApplyResultKind kind{AssetHotReloadApplyResultKind::unknown};
    AssetId asset;
    std::string path;
    std::uint64_t requested_revision{0};
    std::uint64_t active_revision{0};
    std::string diagnostic;
};

class AssetHotReloadApplyState final {
  public:
    void seed(std::vector<AssetFileSnapshot> snapshots);
    [[nodiscard]] AssetHotReloadApplyResult mark_applied(const AssetHotReloadRecookRequest& request,
                                                         std::uint64_t cooked_revision);
    [[nodiscard]] AssetHotReloadApplyResult mark_failed(const AssetHotReloadRecookRequest& request,
                                                        std::string diagnostic) const;

    [[nodiscard]] const AssetHotReloadAppliedAsset* find(AssetId asset) const noexcept;
    [[nodiscard]] std::size_t active_count() const noexcept;

  private:
    std::unordered_map<AssetId, AssetHotReloadAppliedAsset, AssetIdHash> active_by_asset_;
};

struct AssetRuntimeReplacement {
    AssetId asset;
    std::string path;
    std::uint64_t requested_revision{0};
    std::uint64_t cooked_revision{0};
};

class AssetRuntimeReplacementState final {
  public:
    void seed(std::vector<AssetFileSnapshot> snapshots);
    [[nodiscard]] AssetHotReloadApplyResult stage(const AssetHotReloadRecookRequest& request, std::string cooked_path,
                                                  std::uint64_t cooked_revision);
    [[nodiscard]] AssetHotReloadApplyResult mark_failed(const AssetHotReloadRecookRequest& request,
                                                        std::string diagnostic);
    [[nodiscard]] std::vector<AssetHotReloadApplyResult> commit_safe_point();
    void clear() noexcept;

    [[nodiscard]] const AssetHotReloadAppliedAsset* find_active(AssetId asset) const noexcept;
    [[nodiscard]] const AssetRuntimeReplacement* find_pending(AssetId asset) const noexcept;
    [[nodiscard]] std::size_t active_count() const noexcept;
    [[nodiscard]] std::size_t pending_count() const noexcept;

  private:
    std::unordered_map<AssetId, AssetHotReloadAppliedAsset, AssetIdHash> active_by_asset_;
    std::unordered_map<AssetId, AssetRuntimeReplacement, AssetIdHash> pending_by_asset_;
};

[[nodiscard]] bool is_valid_asset_file_snapshot(const AssetFileSnapshot& snapshot) noexcept;

class AssetHotReloadTracker {
  public:
    [[nodiscard]] std::vector<AssetHotReloadEvent> update(std::vector<AssetFileSnapshot> snapshots);
    [[nodiscard]] const AssetFileSnapshot* find(std::string_view path) const noexcept;
    [[nodiscard]] std::size_t watched_count() const noexcept;

  private:
    std::unordered_map<std::string, AssetFileSnapshot> snapshots_by_path_;
};

} // namespace mirakana
