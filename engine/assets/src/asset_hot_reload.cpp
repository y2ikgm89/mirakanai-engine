// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_hot_reload.hpp"

#include "mirakana/assets/asset_dependency_graph.hpp"
#include "mirakana/assets/asset_registry.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_path(std::string_view path) noexcept {
    return !path.empty() && path.find('\n') == std::string_view::npos && path.find('\r') == std::string_view::npos &&
           path.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool event_less(const AssetHotReloadEvent& lhs, const AssetHotReloadEvent& rhs) noexcept {
    if (lhs.path != rhs.path) {
        return lhs.path < rhs.path;
    }
    return lhs.asset.value < rhs.asset.value;
}

[[nodiscard]] bool valid_event_kind(AssetHotReloadEventKind kind) noexcept {
    switch (kind) {
    case AssetHotReloadEventKind::added:
    case AssetHotReloadEventKind::modified:
    case AssetHotReloadEventKind::removed:
        return true;
    case AssetHotReloadEventKind::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_hot_reload_event(const AssetHotReloadEvent& event) noexcept {
    if (event.asset.value == 0 || !valid_path(event.path) || !valid_event_kind(event.kind)) {
        return false;
    }

    switch (event.kind) {
    case AssetHotReloadEventKind::added:
        return event.previous_revision == 0 && event.current_revision != 0;
    case AssetHotReloadEventKind::modified:
        return event.previous_revision != 0 && event.current_revision != 0;
    case AssetHotReloadEventKind::removed:
        return event.previous_revision != 0 && event.current_revision == 0;
    case AssetHotReloadEventKind::unknown:
        break;
    }
    return false;
}

[[nodiscard]] AssetHotReloadRecookReason recook_reason_for(AssetHotReloadEventKind kind) noexcept {
    switch (kind) {
    case AssetHotReloadEventKind::added:
        return AssetHotReloadRecookReason::source_added;
    case AssetHotReloadEventKind::modified:
        return AssetHotReloadRecookReason::source_modified;
    case AssetHotReloadEventKind::removed:
        return AssetHotReloadRecookReason::source_removed;
    case AssetHotReloadEventKind::unknown:
        break;
    }
    return AssetHotReloadRecookReason::unknown;
}

[[nodiscard]] bool valid_recook_reason(AssetHotReloadRecookReason reason) noexcept {
    switch (reason) {
    case AssetHotReloadRecookReason::source_added:
    case AssetHotReloadRecookReason::source_modified:
    case AssetHotReloadRecookReason::source_removed:
    case AssetHotReloadRecookReason::dependency_invalidated:
        return true;
    case AssetHotReloadRecookReason::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool is_valid_recook_request(const AssetHotReloadRecookRequest& request) noexcept {
    return request.asset.value != 0 && request.source_asset.value != 0 && valid_path(request.trigger_path) &&
           valid_event_kind(request.trigger_event_kind) && valid_recook_reason(request.reason);
}

[[nodiscard]] bool request_targets_dependency(const AssetHotReloadRecookRequest& request) noexcept {
    return request.asset != request.source_asset;
}

[[nodiscard]] bool request_less(const AssetHotReloadRecookRequest& lhs,
                                const AssetHotReloadRecookRequest& rhs) noexcept {
    if (lhs.ready_tick != rhs.ready_tick) {
        return lhs.ready_tick < rhs.ready_tick;
    }
    if (request_targets_dependency(lhs) != request_targets_dependency(rhs)) {
        return request_targets_dependency(lhs);
    }
    if (lhs.trigger_path != rhs.trigger_path) {
        return lhs.trigger_path < rhs.trigger_path;
    }
    if (lhs.asset.value != rhs.asset.value) {
        return lhs.asset.value < rhs.asset.value;
    }
    return lhs.source_asset.value < rhs.source_asset.value;
}

[[nodiscard]] bool replacement_less(const AssetRuntimeReplacement& lhs, const AssetRuntimeReplacement& rhs) noexcept {
    if (lhs.path != rhs.path) {
        return lhs.path < rhs.path;
    }
    return lhs.asset.value < rhs.asset.value;
}

[[nodiscard]] AssetHotReloadRecookRequest make_recook_request(AssetId asset, const AssetHotReloadEvent& event,
                                                              AssetHotReloadRecookReason reason,
                                                              std::uint64_t ready_tick) {
    AssetHotReloadRecookRequest request{
        .asset = asset,
        .source_asset = event.asset,
        .trigger_path = event.path,
        .trigger_event_kind = event.kind,
        .reason = reason,
        .previous_revision = event.previous_revision,
        .current_revision = event.current_revision,
        .ready_tick = ready_tick,
    };
    if (!is_valid_recook_request(request)) {
        throw std::invalid_argument("asset hot reload recook request is invalid");
    }
    return request;
}

} // namespace

bool is_valid_asset_file_snapshot(const AssetFileSnapshot& snapshot) noexcept {
    return snapshot.asset.value != 0 && valid_path(snapshot.path) && snapshot.revision != 0;
}

AssetHotReloadRecookScheduler::AssetHotReloadRecookScheduler(AssetHotReloadRecookSchedulerDesc desc) : desc_(desc) {}

void AssetHotReloadRecookScheduler::enqueue(std::vector<AssetHotReloadEvent> events,
                                            const AssetDependencyGraph& dependencies, std::uint64_t now_tick) {
    std::ranges::sort(events, event_less);
    const auto ready_tick = now_tick + desc_.debounce_ticks;

    for (const auto& event : events) {
        if (!valid_hot_reload_event(event)) {
            throw std::invalid_argument("asset hot reload event is invalid for recook scheduling");
        }

        auto source_request = make_recook_request(event.asset, event, recook_reason_for(event.kind), ready_tick);
        auto [source_it, source_inserted] = pending_by_asset_.emplace(source_request.asset, source_request);
        if (!source_inserted && source_request.ready_tick >= source_it->second.ready_tick) {
            source_it->second = std::move(source_request);
        }

        std::deque<AssetId> queue;
        std::unordered_set<std::uint64_t> visited;
        queue.push_back(event.asset);
        visited.insert(event.asset.value);

        while (!queue.empty()) {
            const auto current = queue.front();
            queue.pop_front();

            for (const auto& edge : dependencies.dependents_of(current)) {
                if (!visited.insert(edge.asset.value).second) {
                    continue;
                }

                auto dependent_request = make_recook_request(
                    edge.asset, event, AssetHotReloadRecookReason::dependency_invalidated, ready_tick);
                auto [dependent_it, dependent_inserted] =
                    pending_by_asset_.emplace(dependent_request.asset, dependent_request);
                if (!dependent_inserted && dependent_request.ready_tick >= dependent_it->second.ready_tick) {
                    dependent_it->second = std::move(dependent_request);
                }
                queue.push_back(edge.asset);
            }
        }
    }
}

std::vector<AssetHotReloadRecookRequest> AssetHotReloadRecookScheduler::ready(std::uint64_t now_tick) {
    std::vector<AssetHotReloadRecookRequest> result;
    for (auto it = pending_by_asset_.begin(); it != pending_by_asset_.end();) {
        if (it->second.ready_tick <= now_tick) {
            result.push_back(std::move(it->second));
            it = pending_by_asset_.erase(it);
            continue;
        }
        ++it;
    }
    std::ranges::sort(result, request_less);
    return result;
}

void AssetHotReloadRecookScheduler::clear() noexcept {
    pending_by_asset_.clear();
}

const AssetHotReloadRecookRequest* AssetHotReloadRecookScheduler::find_pending(AssetId asset) const noexcept {
    const auto it = pending_by_asset_.find(asset);
    if (it == pending_by_asset_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::size_t AssetHotReloadRecookScheduler::pending_count() const noexcept {
    return pending_by_asset_.size();
}

void AssetHotReloadApplyState::seed(std::vector<AssetFileSnapshot> snapshots) {
    std::unordered_set<std::uint64_t> assets;
    std::unordered_map<AssetId, AssetHotReloadAppliedAsset, AssetIdHash> next;
    next.reserve(snapshots.size());

    for (auto& snapshot : snapshots) {
        if (!is_valid_asset_file_snapshot(snapshot)) {
            throw std::invalid_argument("asset file snapshot is invalid for hot reload apply state");
        }
        if (!assets.insert(snapshot.asset.value).second) {
            throw std::invalid_argument("asset file snapshot asset is duplicated for hot reload apply state");
        }
        next.emplace(snapshot.asset, AssetHotReloadAppliedAsset{
                                         .asset = snapshot.asset,
                                         .path = std::move(snapshot.path),
                                         .revision = snapshot.revision,
                                     });
    }

    active_by_asset_ = std::move(next);
}

AssetHotReloadApplyResult AssetHotReloadApplyState::mark_applied(const AssetHotReloadRecookRequest& request,
                                                                 std::uint64_t cooked_revision) {
    if (!is_valid_recook_request(request) || cooked_revision == 0) {
        throw std::invalid_argument("asset hot reload applied result is invalid");
    }

    active_by_asset_[request.asset] = AssetHotReloadAppliedAsset{
        .asset = request.asset,
        .path = request.trigger_path,
        .revision = cooked_revision,
    };
    return AssetHotReloadApplyResult{
        .kind = AssetHotReloadApplyResultKind::applied,
        .asset = request.asset,
        .path = request.trigger_path,
        .requested_revision = request.current_revision,
        .active_revision = cooked_revision,
        .diagnostic = {},
    };
}

AssetHotReloadApplyResult AssetHotReloadApplyState::mark_failed(const AssetHotReloadRecookRequest& request,
                                                                std::string diagnostic) const {
    if (!is_valid_recook_request(request) || diagnostic.empty() || !valid_path(diagnostic)) {
        throw std::invalid_argument("asset hot reload failed result is invalid");
    }

    const auto* active = find(request.asset);
    return AssetHotReloadApplyResult{
        .kind = AssetHotReloadApplyResultKind::failed_rolled_back,
        .asset = request.asset,
        .path = active != nullptr ? active->path : request.trigger_path,
        .requested_revision = request.current_revision,
        .active_revision = active != nullptr ? active->revision : 0,
        .diagnostic = std::move(diagnostic),
    };
}

const AssetHotReloadAppliedAsset* AssetHotReloadApplyState::find(AssetId asset) const noexcept {
    const auto it = active_by_asset_.find(asset);
    if (it == active_by_asset_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::size_t AssetHotReloadApplyState::active_count() const noexcept {
    return active_by_asset_.size();
}

void AssetRuntimeReplacementState::seed(std::vector<AssetFileSnapshot> snapshots) {
    std::unordered_set<std::uint64_t> assets;
    std::unordered_map<AssetId, AssetHotReloadAppliedAsset, AssetIdHash> next;
    next.reserve(snapshots.size());

    for (auto& snapshot : snapshots) {
        if (!is_valid_asset_file_snapshot(snapshot)) {
            throw std::invalid_argument("asset file snapshot is invalid for runtime replacement state");
        }
        if (!assets.insert(snapshot.asset.value).second) {
            throw std::invalid_argument("asset file snapshot asset is duplicated for runtime replacement state");
        }
        next.emplace(snapshot.asset, AssetHotReloadAppliedAsset{
                                         .asset = snapshot.asset,
                                         .path = std::move(snapshot.path),
                                         .revision = snapshot.revision,
                                     });
    }

    active_by_asset_ = std::move(next);
    pending_by_asset_.clear();
}

AssetHotReloadApplyResult AssetRuntimeReplacementState::stage(const AssetHotReloadRecookRequest& request,
                                                              std::string cooked_path, std::uint64_t cooked_revision) {
    if (!is_valid_recook_request(request) || !valid_path(cooked_path) || cooked_revision == 0) {
        throw std::invalid_argument("asset runtime replacement stage request is invalid");
    }

    const auto* active = find_active(request.asset);
    pending_by_asset_[request.asset] = AssetRuntimeReplacement{
        .asset = request.asset,
        .path = std::move(cooked_path),
        .requested_revision = request.current_revision,
        .cooked_revision = cooked_revision,
    };

    return AssetHotReloadApplyResult{
        .kind = AssetHotReloadApplyResultKind::staged,
        .asset = request.asset,
        .path = pending_by_asset_.at(request.asset).path,
        .requested_revision = request.current_revision,
        .active_revision = active != nullptr ? active->revision : 0,
        .diagnostic = {},
    };
}

AssetHotReloadApplyResult AssetRuntimeReplacementState::mark_failed(const AssetHotReloadRecookRequest& request,
                                                                    std::string diagnostic) {
    if (!is_valid_recook_request(request) || diagnostic.empty() || !valid_path(diagnostic)) {
        throw std::invalid_argument("asset runtime replacement failure is invalid");
    }

    pending_by_asset_.erase(request.asset);
    const auto* active = find_active(request.asset);
    return AssetHotReloadApplyResult{
        .kind = AssetHotReloadApplyResultKind::failed_rolled_back,
        .asset = request.asset,
        .path = active != nullptr ? active->path : request.trigger_path,
        .requested_revision = request.current_revision,
        .active_revision = active != nullptr ? active->revision : 0,
        .diagnostic = std::move(diagnostic),
    };
}

namespace {

[[nodiscard]] std::vector<AssetRuntimeReplacement>
select_pending_replacements(const std::unordered_map<AssetId, AssetRuntimeReplacement, AssetIdHash>& pending_by_asset,
                            std::span<const AssetId> assets) {
    std::vector<AssetRuntimeReplacement> pending;
    pending.reserve(assets.size());
    std::unordered_set<std::uint64_t> selected_assets;
    selected_assets.reserve(assets.size());
    for (const auto asset : assets) {
        if (asset.value == 0) {
            throw std::invalid_argument("asset runtime replacement commit asset is invalid");
        }
        if (!selected_assets.insert(asset.value).second) {
            continue;
        }
        const auto it = pending_by_asset.find(asset);
        if (it == pending_by_asset.end()) {
            continue;
        }
        pending.push_back(it->second);
    }
    std::ranges::sort(pending, replacement_less);
    return pending;
}

} // namespace

std::vector<AssetHotReloadApplyResult> AssetRuntimeReplacementState::commit_safe_point() {
    std::vector<AssetRuntimeReplacement> pending;
    pending.reserve(pending_by_asset_.size());
    for (const auto& [_, replacement] : pending_by_asset_) {
        pending.push_back(replacement);
    }
    std::ranges::sort(pending, replacement_less);

    std::vector<AssetHotReloadApplyResult> results;
    results.reserve(pending.size());
    for (auto& replacement : pending) {
        active_by_asset_[replacement.asset] = AssetHotReloadAppliedAsset{
            .asset = replacement.asset,
            .path = replacement.path,
            .revision = replacement.cooked_revision,
        };
        results.push_back(AssetHotReloadApplyResult{
            .kind = AssetHotReloadApplyResultKind::applied,
            .asset = replacement.asset,
            .path = std::move(replacement.path),
            .requested_revision = replacement.requested_revision,
            .active_revision = replacement.cooked_revision,
            .diagnostic = {},
        });
    }

    pending_by_asset_.clear();
    return results;
}

std::vector<AssetHotReloadApplyResult>
AssetRuntimeReplacementState::commit_safe_point(std::span<const AssetId> assets) {
    auto pending = select_pending_replacements(pending_by_asset_, assets);

    std::vector<AssetHotReloadApplyResult> results;
    results.reserve(pending.size());
    for (auto& replacement : pending) {
        active_by_asset_[replacement.asset] = AssetHotReloadAppliedAsset{
            .asset = replacement.asset,
            .path = replacement.path,
            .revision = replacement.cooked_revision,
        };
        results.push_back(AssetHotReloadApplyResult{
            .kind = AssetHotReloadApplyResultKind::applied,
            .asset = replacement.asset,
            .path = std::move(replacement.path),
            .requested_revision = replacement.requested_revision,
            .active_revision = replacement.cooked_revision,
            .diagnostic = {},
        });
        pending_by_asset_.erase(replacement.asset);
    }

    return results;
}

void AssetRuntimeReplacementState::discard_pending(std::span<const AssetId> assets) noexcept {
    for (const auto asset : assets) {
        if (asset.value == 0) {
            continue;
        }
        pending_by_asset_.erase(asset);
    }
}

void AssetRuntimeReplacementState::clear() noexcept {
    active_by_asset_.clear();
    pending_by_asset_.clear();
}

const AssetHotReloadAppliedAsset* AssetRuntimeReplacementState::find_active(AssetId asset) const noexcept {
    const auto it = active_by_asset_.find(asset);
    if (it == active_by_asset_.end()) {
        return nullptr;
    }
    return &it->second;
}

const AssetRuntimeReplacement* AssetRuntimeReplacementState::find_pending(AssetId asset) const noexcept {
    const auto it = pending_by_asset_.find(asset);
    if (it == pending_by_asset_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::size_t AssetRuntimeReplacementState::active_count() const noexcept {
    return active_by_asset_.size();
}

std::size_t AssetRuntimeReplacementState::pending_count() const noexcept {
    return pending_by_asset_.size();
}

std::vector<AssetHotReloadEvent> AssetHotReloadTracker::update(std::vector<AssetFileSnapshot> snapshots) {
    std::unordered_set<std::string> paths;
    std::unordered_set<std::uint64_t> assets;
    for (const auto& snapshot : snapshots) {
        if (!is_valid_asset_file_snapshot(snapshot)) {
            throw std::invalid_argument("asset file snapshot is invalid");
        }
        if (!paths.insert(snapshot.path).second) {
            throw std::invalid_argument("asset file snapshot path is duplicated");
        }
        if (!assets.insert(snapshot.asset.value).second) {
            throw std::invalid_argument("asset file snapshot asset is duplicated");
        }
    }

    std::unordered_map<std::string, AssetFileSnapshot> next_by_path;
    next_by_path.reserve(snapshots.size());
    for (auto& snapshot : snapshots) {
        next_by_path.emplace(snapshot.path, std::move(snapshot));
    }

    std::vector<AssetHotReloadEvent> events;
    for (const auto& [path, previous] : snapshots_by_path_) {
        const auto next = next_by_path.find(path);
        if (next == next_by_path.end()) {
            events.push_back(AssetHotReloadEvent{
                .kind = AssetHotReloadEventKind::removed,
                .asset = previous.asset,
                .path = previous.path,
                .previous_revision = previous.revision,
                .current_revision = 0,
                .previous_size_bytes = previous.size_bytes,
                .current_size_bytes = 0,
            });
            continue;
        }

        const auto& current = next->second;
        if (previous.asset != current.asset || previous.revision != current.revision ||
            previous.size_bytes != current.size_bytes) {
            events.push_back(AssetHotReloadEvent{
                .kind = AssetHotReloadEventKind::modified,
                .asset = current.asset,
                .path = current.path,
                .previous_revision = previous.revision,
                .current_revision = current.revision,
                .previous_size_bytes = previous.size_bytes,
                .current_size_bytes = current.size_bytes,
            });
        }
    }

    for (const auto& [path, current] : next_by_path) {
        if (snapshots_by_path_.find(path) == snapshots_by_path_.end()) {
            events.push_back(AssetHotReloadEvent{
                .kind = AssetHotReloadEventKind::added,
                .asset = current.asset,
                .path = current.path,
                .previous_revision = 0,
                .current_revision = current.revision,
                .previous_size_bytes = 0,
                .current_size_bytes = current.size_bytes,
            });
        }
    }

    std::ranges::sort(events, event_less);
    snapshots_by_path_ = std::move(next_by_path);
    return events;
}

const AssetFileSnapshot* AssetHotReloadTracker::find(std::string_view path) const noexcept {
    const auto it = snapshots_by_path_.find(std::string(path));
    if (it == snapshots_by_path_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::size_t AssetHotReloadTracker::watched_count() const noexcept {
    return snapshots_by_path_.size();
}

} // namespace mirakana
