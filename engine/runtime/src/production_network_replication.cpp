// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/production_network_replication.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool is_token_char(char ch) noexcept {
    const auto value = static_cast<unsigned char>(ch);
    return (value >= static_cast<unsigned char>('a') && value <= static_cast<unsigned char>('z')) ||
           (value >= static_cast<unsigned char>('A') && value <= static_cast<unsigned char>('Z')) ||
           (value >= static_cast<unsigned char>('0') && value <= static_cast<unsigned char>('9')) || ch == '_';
}

[[nodiscard]] char lower_ascii(char ch) noexcept {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

[[nodiscard]] bool is_forbidden_runtime_token(std::string_view token) {
    constexpr auto forbidden = std::array<std::string_view, 12U>{
        "native", "socket", "thread", "backend", "renderer", "rhi", "d3d12", "vulkan", "metal", "sdl", "sdl3", "imgui",
    };
    return std::ranges::find(forbidden, token) != forbidden.end();
}

[[nodiscard]] bool has_runtime_backend_reference(std::string_view value) {
    std::string token;
    for (const auto ch : value) {
        if (is_token_char(ch)) {
            token.push_back(lower_ascii(ch));
            continue;
        }
        if (is_forbidden_runtime_token(token)) {
            return true;
        }
        token.clear();
    }
    return is_forbidden_runtime_token(token);
}

[[nodiscard]] bool is_authoritative_topology(RuntimeNetworkSessionTopology topology) noexcept {
    switch (topology) {
    case RuntimeNetworkSessionTopology::listen_server:
    case RuntimeNetworkSessionTopology::dedicated_server:
        return true;
    case RuntimeNetworkSessionTopology::local_only:
    case RuntimeNetworkSessionTopology::peer_to_peer:
        return false;
    }
    return false;
}

[[nodiscard]] bool is_snapshot_authority(RuntimeNetworkReplicationAuthority authority) noexcept {
    return authority == RuntimeNetworkReplicationAuthority::server ||
           authority == RuntimeNetworkReplicationAuthority::host;
}

[[nodiscard]] bool is_input_authority(RuntimeNetworkReplicationAuthority authority) noexcept {
    return authority == RuntimeNetworkReplicationAuthority::client;
}

[[nodiscard]] std::size_t request_row_count(const RuntimeNetworkReplicationRequest& request) {
    return 1U + request.object_rows.size() + request.input_rows.size() + request.snapshot_rows.size() +
           (request.rollback_policy.mode == RuntimeRollbackMode::disabled ? 0U : 1U) +
           request.transport_evidence_rows.size();
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) {
    return std::ranges::any_of(ids, [id](const auto& candidate) { return candidate == id; });
}

[[nodiscard]] bool has_object_id(const std::vector<std::string>& ids, std::string_view id) {
    return contains_id(ids, id);
}

[[nodiscard]] std::string make_pair_key(std::string_view first, std::string_view second) {
    std::string key;
    key.reserve(first.size() + second.size() + 1U);
    key.append(first);
    key.push_back('\n');
    key.append(second);
    return key;
}

void add_diagnostic(RuntimeNetworkReplicationPlan& plan, RuntimeNetworkReplicationDiagnosticCode code,
                    std::string session_id, std::string row_id, std::string secondary_id, std::string message,
                    std::uint32_t source_index) {
    plan.diagnostics.push_back(RuntimeNetworkReplicationDiagnostic{
        .code = code,
        .session_id = std::move(session_id),
        .row_id = std::move(row_id),
        .secondary_id = std::move(secondary_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_diagnostics(RuntimeNetworkReplicationPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.session_id != rhs.session_id) {
            return lhs.session_id < rhs.session_id;
        }
        if (lhs.row_id != rhs.row_id) {
            return lhs.row_id < rhs.row_id;
        }
        if (lhs.secondary_id != rhs.secondary_id) {
            return lhs.secondary_id < rhs.secondary_id;
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        return lhs.message < rhs.message;
    });
}

[[nodiscard]] std::size_t count_rejected_unsafe_rows(const RuntimeNetworkReplicationPlan& plan) {
    std::vector<std::string> row_keys;
    row_keys.reserve(plan.diagnostics.size());
    for (const auto& diagnostic : plan.diagnostics) {
        std::string row_key;
        row_key.reserve(diagnostic.session_id.size() + diagnostic.row_id.size() + 24U);
        row_key.append(diagnostic.session_id);
        row_key.push_back('\n');
        row_key.append(diagnostic.row_id);
        row_key.push_back('\n');
        row_key.append(std::to_string(diagnostic.source_index));
        if (!contains_id(row_keys, row_key)) {
            row_keys.push_back(std::move(row_key));
        }
    }
    return row_keys.size();
}

[[nodiscard]] const RuntimeNetworkSessionPlanRow*
find_foundation_session(const RuntimeNetworkFoundationPlan& foundation_plan, std::string_view session_id) {
    const auto iter = std::ranges::find_if(foundation_plan.sessions,
                                           [session_id](const auto& row) { return row.session_id == session_id; });
    if (iter == foundation_plan.sessions.end()) {
        return nullptr;
    }
    return &(*iter);
}

[[nodiscard]] const RuntimeNetworkReplicationChannelRow*
find_foundation_channel(const RuntimeNetworkFoundationPlan& foundation_plan, std::string_view session_id,
                        std::string_view channel_id) {
    const auto iter = std::ranges::find_if(foundation_plan.channels, [session_id, channel_id](const auto& row) {
        return row.session_id == session_id && row.channel_id == channel_id;
    });
    if (iter == foundation_plan.channels.end()) {
        return nullptr;
    }
    return &(*iter);
}

[[nodiscard]] const RuntimeNetworkReplayPrerequisiteRow*
find_foundation_replay(const RuntimeNetworkFoundationPlan& foundation_plan, std::string_view session_id) {
    const auto iter = std::ranges::find_if(foundation_plan.replay_prerequisites,
                                           [session_id](const auto& row) { return row.session_id == session_id; });
    if (iter == foundation_plan.replay_prerequisites.end()) {
        return nullptr;
    }
    return &(*iter);
}

void validate_foundation(RuntimeNetworkReplicationPlan& plan, const RuntimeNetworkReplicationRequest& request) {
    if (!request.foundation_plan.succeeded()) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::invalid_foundation_plan,
                       request.session.session_id, {}, {},
                       "network replication requires a successful runtime networking foundation plan",
                       request.session.source_index);
    }

    const auto* session = find_foundation_session(request.foundation_plan, request.session.session_id);
    if (session == nullptr) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::missing_authoritative_session,
                       request.session.session_id, request.session.session_id, {},
                       "network replication requires a matching authoritative foundation session",
                       request.session.source_index);
        return;
    }

    if (!is_authoritative_topology(session->topology)) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::unsupported_topology, request.session.session_id,
                       request.session.session_id, {},
                       "network replication v1 supports listen_server and dedicated_server topologies only",
                       session->source_index);
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::missing_authoritative_session,
                       request.session.session_id, request.session.session_id, {},
                       "peer-to-peer and local-only sessions are not authoritative replication sessions",
                       session->source_index);
    }

    const auto* replay = find_foundation_replay(request.foundation_plan, request.session.session_id);
    if (replay != nullptr && request.session.fixed_tick_rate_hz != 0U &&
        replay->fixed_tick_rate_hz != request.session.fixed_tick_rate_hz) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::invalid_session_row, request.session.session_id,
                       request.session.session_id, {},
                       "network replication session tick rate must match foundation replay prerequisites",
                       request.session.source_index);
    }
}

void validate_session(RuntimeNetworkReplicationPlan& plan, const RuntimeNetworkReplicationRequest& request) {
    if (request.session.mode != RuntimeNetworkReplicationMode::authoritative_snapshot) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::unsupported_replication_mode,
                       request.session.session_id, request.session.session_id, {},
                       "network replication v1 supports authoritative snapshot mode only",
                       request.session.source_index);
    }

    if (!is_valid_id(request.session.session_id) || !is_valid_id(request.session.world_id) ||
        has_runtime_backend_reference(request.session.session_id) ||
        has_runtime_backend_reference(request.session.world_id) || request.session.fixed_tick_rate_hz == 0U ||
        request.session.max_players == 0U || request.session.max_objects == 0U) {
        add_diagnostic(
            plan, RuntimeNetworkReplicationDiagnosticCode::invalid_session_row, request.session.session_id,
            request.session.world_id, {},
            "network replication session requires backend-neutral ids and positive tick/player/object limits",
            request.session.source_index);
    }
    if (request.object_rows.size() > request.session.max_objects) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::row_budget_exceeded, request.session.session_id,
                       request.session.session_id, {},
                       "network replication object rows exceed the session max_objects limit",
                       request.session.source_index);
    }
}

[[nodiscard]] bool valid_object_row(const RuntimeReplicatedObjectRow& row) {
    return is_valid_id(row.object_id) && is_valid_id(row.entity_id.value) && is_valid_id(row.region_id.value) &&
           is_valid_id(row.schema_id.value) && is_valid_id(row.channel_id) && row.priority > 0U &&
           !has_runtime_backend_reference(row.object_id) && !has_runtime_backend_reference(row.entity_id.value) &&
           !has_runtime_backend_reference(row.region_id.value) && !has_runtime_backend_reference(row.schema_id.value) &&
           !has_runtime_backend_reference(row.channel_id);
}

[[nodiscard]] std::vector<std::string> validate_objects(RuntimeNetworkReplicationPlan& plan,
                                                        const RuntimeNetworkReplicationRequest& request) {
    std::vector<std::string> object_ids;
    object_ids.reserve(request.object_rows.size());
    for (const auto& row : request.object_rows) {
        auto row_valid = true;
        if (!valid_object_row(row)) {
            add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::invalid_object_id, request.session.session_id,
                           row.object_id, row.channel_id,
                           "replicated objects require backend-neutral object/entity/region/schema/channel ids",
                           row.source_index);
            row_valid = false;
        }
        if (contains_id(object_ids, row.object_id)) {
            add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::duplicate_object_id,
                           request.session.session_id, row.object_id, row.channel_id,
                           "replicated object ids must be unique per session", row.source_index);
        } else if (row_valid) {
            object_ids.push_back(row.object_id);
        }

        const auto* channel =
            find_foundation_channel(request.foundation_plan, request.session.session_id, row.channel_id);
        if (channel == nullptr) {
            add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::unknown_channel_id,
                           request.session.session_id, row.object_id, row.channel_id,
                           "replicated object channel must exist in the foundation plan", row.source_index);
        } else if (!is_snapshot_authority(channel->authority)) {
            add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::channel_authority_mismatch,
                           request.session.session_id, row.object_id, row.channel_id,
                           "replicated object state channels must be server or host authoritative", row.source_index);
        }
    }
    return object_ids;
}

void validate_inputs(RuntimeNetworkReplicationPlan& plan, const RuntimeNetworkReplicationRequest& request) {
    std::vector<std::string> sequence_keys;
    std::vector<std::string> timeline_keys;
    std::vector<std::uint64_t> last_ticks;

    for (const auto& row : request.input_rows) {
        if (!is_valid_id(row.player_id) || !is_valid_id(row.command_id) || !is_valid_id(row.channel_id) ||
            has_runtime_backend_reference(row.player_id) || has_runtime_backend_reference(row.command_id) ||
            has_runtime_backend_reference(row.channel_id) || row.payload_hash == 0U) {
            add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::invalid_input_row, request.session.session_id,
                           row.command_id, row.channel_id,
                           "replication input rows require backend-neutral ids and non-zero payload hashes",
                           row.source_index);
        }

        const auto* channel =
            find_foundation_channel(request.foundation_plan, request.session.session_id, row.channel_id);
        if (channel == nullptr) {
            add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::unknown_channel_id,
                           request.session.session_id, row.command_id, row.channel_id,
                           "replication input channel must exist in the foundation plan", row.source_index);
        } else if (!is_input_authority(channel->authority)) {
            add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::channel_authority_mismatch,
                           request.session.session_id, row.command_id, row.channel_id,
                           "replication input rows must use client authoritative channels", row.source_index);
        }

        auto sequence_key = make_pair_key(row.player_id, row.channel_id);
        sequence_key.push_back('\n');
        sequence_key.append(std::to_string(row.sequence));
        if (contains_id(sequence_keys, sequence_key)) {
            add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::duplicate_input_sequence,
                           request.session.session_id, row.command_id, row.channel_id,
                           "replication input sequences must be unique per player and channel", row.source_index);
        } else {
            sequence_keys.push_back(sequence_key);
        }

        const auto timeline_key = make_pair_key(row.player_id, row.channel_id);
        const auto timeline_iter = std::ranges::find(timeline_keys, timeline_key);
        if (timeline_iter == timeline_keys.end()) {
            timeline_keys.push_back(timeline_key);
            last_ticks.push_back(row.target_tick);
        } else {
            const auto index = static_cast<std::size_t>(timeline_iter - timeline_keys.begin());
            if (row.target_tick <= last_ticks[index]) {
                add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::non_monotonic_input_tick,
                               request.session.session_id, row.command_id, row.channel_id,
                               "replication input target ticks must increase per player and channel", row.source_index);
            }
            last_ticks[index] = row.target_tick;
        }
    }
}

void validate_snapshots(RuntimeNetworkReplicationPlan& plan, const RuntimeNetworkReplicationRequest& request,
                        const std::vector<std::string>& object_ids) {
    std::vector<std::string> channel_ids;
    std::vector<std::uint64_t> last_ticks;
    std::uint64_t byte_count{0U};

    for (const auto& row : request.snapshot_rows) {
        if (!is_valid_id(row.snapshot_id) || !is_valid_id(row.channel_id) ||
            has_runtime_backend_reference(row.snapshot_id) || has_runtime_backend_reference(row.channel_id) ||
            row.state_hash == 0U || row.byte_count == 0U || row.object_ids.empty()) {
            add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::invalid_snapshot_row,
                           request.session.session_id, row.snapshot_id, row.channel_id,
                           "replication snapshots require backend-neutral ids, objects, byte count, and state hash",
                           row.source_index);
        }

        const auto* channel =
            find_foundation_channel(request.foundation_plan, request.session.session_id, row.channel_id);
        if (channel == nullptr) {
            add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::unknown_channel_id,
                           request.session.session_id, row.snapshot_id, row.channel_id,
                           "replication snapshot channel must exist in the foundation plan", row.source_index);
        } else if (!is_snapshot_authority(channel->authority)) {
            add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::channel_authority_mismatch,
                           request.session.session_id, row.snapshot_id, row.channel_id,
                           "replication snapshots must use server or host authoritative channels", row.source_index);
        }

        for (const auto& object_id : row.object_ids) {
            if (!has_object_id(object_ids, object_id)) {
                add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::unknown_snapshot_object,
                               request.session.session_id, row.snapshot_id, object_id,
                               "replication snapshot object ids must reference reviewed replicated objects",
                               row.source_index);
            }
        }

        const auto channel_iter = std::ranges::find(channel_ids, row.channel_id);
        if (channel_iter == channel_ids.end()) {
            channel_ids.push_back(row.channel_id);
            last_ticks.push_back(row.tick);
        } else {
            const auto index = static_cast<std::size_t>(channel_iter - channel_ids.begin());
            if (row.tick <= last_ticks[index]) {
                add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::non_monotonic_snapshot_tick,
                               request.session.session_id, row.snapshot_id, row.channel_id,
                               "replication snapshot ticks must increase per channel", row.source_index);
            }
            last_ticks[index] = row.tick;
        }

        if (row.byte_count > std::numeric_limits<std::uint64_t>::max() - byte_count) {
            byte_count = std::numeric_limits<std::uint64_t>::max();
        } else {
            byte_count += row.byte_count;
        }
    }

    if (byte_count > request.snapshot_byte_budget) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::snapshot_byte_budget_exceeded,
                       request.session.session_id, request.session.session_id, {},
                       "replication snapshot byte count exceeds the request snapshot byte budget", 0U);
    }
}

void validate_rollback(RuntimeNetworkReplicationPlan& plan, const RuntimeNetworkReplicationRequest& request) {
    if (request.rollback_policy.mode == RuntimeRollbackMode::disabled) {
        return;
    }

    if (request.rollback_policy.mode != RuntimeRollbackMode::input_resimulation) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::unsupported_replication_mode,
                       request.session.session_id, request.session.session_id, {},
                       "network replication v1 supports input resimulation rollback policy only",
                       request.rollback_policy.source_index);
    }

    if (request.rollback_policy.max_rollback_ticks == 0U || request.rollback_policy.snapshot_history_limit == 0U ||
        request.rollback_policy.max_rollback_ticks > request.rollback_policy.snapshot_history_limit) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::rollback_window_exceeded,
                       request.session.session_id, request.session.session_id, {},
                       "rollback policy requires a non-zero rollback window within snapshot history",
                       request.rollback_policy.source_index);
    }

    const auto* replay = find_foundation_replay(request.foundation_plan, request.session.session_id);
    if (replay == nullptr) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::rollback_prerequisite_missing,
                       request.session.session_id, request.session.session_id, {},
                       "rollback policy requires foundation replay prerequisites",
                       request.rollback_policy.source_index);
        return;
    }

    if (request.rollback_policy.requires_deterministic_simulation && !replay->deterministic_simulation) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::rollback_prerequisite_missing,
                       request.session.session_id, request.session.session_id, "deterministic_simulation",
                       "rollback policy requires deterministic simulation acknowledgement",
                       request.rollback_policy.source_index);
    }
    if (request.rollback_policy.requires_ordered_inputs && !replay->ordered_inputs) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::rollback_prerequisite_missing,
                       request.session.session_id, request.session.session_id, "ordered_inputs",
                       "rollback policy requires ordered input acknowledgement", request.rollback_policy.source_index);
    }
}

void validate_transport_evidence(RuntimeNetworkReplicationPlan& plan, const RuntimeNetworkReplicationRequest& request) {
    for (const auto& row : request.transport_evidence_rows) {
        if (!is_valid_id(row.adapter_id) || has_runtime_backend_reference(row.adapter_id) || !row.loopback_validated ||
            !row.reliable_validated || !row.unreliable_validated) {
            add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::unsupported_host_claim,
                           request.session.session_id, row.adapter_id, {},
                           "transport host evidence must be reviewed loopback reliable/unreliable proof only",
                           row.source_index);
        }
    }
}

void validate_budgets(RuntimeNetworkReplicationPlan& plan, const RuntimeNetworkReplicationRequest& request) {
    if (request_row_count(request) > request.row_budget) {
        add_diagnostic(plan, RuntimeNetworkReplicationDiagnosticCode::row_budget_exceeded, request.session.session_id,
                       request.session.session_id, {}, "network replication request exceeds its row budget", 0U);
    }
}

void clear_output_rows(RuntimeNetworkReplicationPlan& plan) {
    plan.session_rows.clear();
    plan.object_rows.clear();
    plan.input_rows.clear();
    plan.snapshot_rows.clear();
    plan.rollback_rows.clear();
    plan.transport_evidence_rows.clear();
    plan.replicated_object_count = 0U;
    plan.input_row_count = 0U;
    plan.snapshot_row_count = 0U;
    plan.rollback_row_count = 0U;
    plan.replay_hash = 0U;
    plan.requires_transport_host_evidence = false;
    plan.has_transport_host_evidence = false;
}

void sort_output_rows(RuntimeNetworkReplicationPlan& plan) {
    std::ranges::sort(plan.object_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.object_id != rhs.object_id) {
            return lhs.object_id < rhs.object_id;
        }
        return lhs.source_index < rhs.source_index;
    });
    std::ranges::sort(plan.input_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.player_id != rhs.player_id) {
            return lhs.player_id < rhs.player_id;
        }
        if (lhs.channel_id != rhs.channel_id) {
            return lhs.channel_id < rhs.channel_id;
        }
        if (lhs.sequence != rhs.sequence) {
            return lhs.sequence < rhs.sequence;
        }
        return lhs.source_index < rhs.source_index;
    });
    std::ranges::sort(plan.snapshot_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.channel_id != rhs.channel_id) {
            return lhs.channel_id < rhs.channel_id;
        }
        if (lhs.tick != rhs.tick) {
            return lhs.tick < rhs.tick;
        }
        return lhs.source_index < rhs.source_index;
    });
    std::ranges::sort(plan.transport_evidence_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.adapter_id != rhs.adapter_id) {
            return lhs.adapter_id < rhs.adapter_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void hash_mix(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value;
    hash *= 1099511628211ULL;
}

void hash_string(std::uint64_t& hash, std::string_view value) noexcept {
    for (const auto ch : value) {
        hash_mix(hash, static_cast<unsigned char>(ch));
    }
    hash_mix(hash, 0xffU);
}

[[nodiscard]] std::uint64_t compute_replay_hash(const RuntimeNetworkReplicationPlan& plan,
                                                const RuntimeNetworkReplicationRequest& request) {
    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, request.seed);
    for (const auto& row : plan.session_rows) {
        hash_string(hash, row.session_id);
        hash_string(hash, row.world_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.mode));
        hash_mix(hash, row.fixed_tick_rate_hz);
        hash_mix(hash, row.max_players);
        hash_mix(hash, row.max_objects);
    }
    for (const auto& row : plan.object_rows) {
        hash_string(hash, row.object_id);
        hash_string(hash, row.entity_id.value);
        hash_string(hash, row.region_id.value);
        hash_string(hash, row.schema_id.value);
        hash_string(hash, row.channel_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.ownership));
        hash_mix(hash, row.priority);
    }
    for (const auto& row : plan.input_rows) {
        hash_string(hash, row.player_id);
        hash_string(hash, row.command_id);
        hash_string(hash, row.channel_id);
        hash_mix(hash, row.target_tick);
        hash_mix(hash, row.sequence);
        hash_mix(hash, row.payload_hash);
    }
    for (const auto& row : plan.snapshot_rows) {
        hash_string(hash, row.snapshot_id);
        hash_string(hash, row.channel_id);
        hash_mix(hash, row.tick);
        hash_mix(hash, static_cast<std::uint8_t>(row.kind));
        for (const auto& object_id : row.object_ids) {
            hash_string(hash, object_id);
        }
        hash_mix(hash, row.state_hash);
        hash_mix(hash, row.byte_count);
    }
    for (const auto& row : plan.rollback_rows) {
        hash_mix(hash, static_cast<std::uint8_t>(row.mode));
        hash_mix(hash, row.max_rollback_ticks);
        hash_mix(hash, row.input_delay_ticks);
        hash_mix(hash, row.snapshot_history_limit);
        hash_mix(hash, row.requires_deterministic_simulation ? 1U : 0U);
        hash_mix(hash, row.requires_ordered_inputs ? 1U : 0U);
    }
    return hash == 0U ? 1U : hash;
}

void append_output_rows(RuntimeNetworkReplicationPlan& plan, const RuntimeNetworkReplicationRequest& request) {
    plan.session_rows.push_back(request.session);
    plan.object_rows = request.object_rows;
    plan.input_rows = request.input_rows;
    plan.snapshot_rows = request.snapshot_rows;
    if (request.rollback_policy.mode != RuntimeRollbackMode::disabled) {
        plan.rollback_rows.push_back(request.rollback_policy);
    }
    plan.transport_evidence_rows = request.transport_evidence_rows;
    plan.replicated_object_count = plan.object_rows.size();
    plan.input_row_count = plan.input_rows.size();
    plan.snapshot_row_count = plan.snapshot_rows.size();
    plan.rollback_row_count = plan.rollback_rows.size();
    plan.requires_transport_host_evidence = request.rollback_policy.requires_transport_host_evidence;
    plan.has_transport_host_evidence = std::ranges::any_of(plan.transport_evidence_rows, [](const auto& row) {
        return row.loopback_validated && row.reliable_validated && row.unreliable_validated;
    });
    sort_output_rows(plan);
    plan.replay_hash = compute_replay_hash(plan, request);
}

} // namespace

bool RuntimeNetworkReplicationPlan::succeeded() const noexcept {
    return status == RuntimeNetworkReplicationStatus::ready || status == RuntimeNetworkReplicationStatus::no_rows;
}

RuntimeNetworkReplicationPlan plan_runtime_network_replication(const RuntimeNetworkReplicationRequest& request) {
    RuntimeNetworkReplicationPlan plan;

    validate_foundation(plan, request);
    validate_session(plan, request);
    const auto object_ids = validate_objects(plan, request);
    validate_inputs(plan, request);
    validate_snapshots(plan, request, object_ids);
    validate_rollback(plan, request);
    validate_transport_evidence(plan, request);
    validate_budgets(plan, request);

    if (!plan.diagnostics.empty()) {
        plan.rejected_unsafe_row_count = count_rejected_unsafe_rows(plan);
        sort_diagnostics(plan);
        clear_output_rows(plan);
        plan.status = RuntimeNetworkReplicationStatus::invalid_request;
        return plan;
    }

    if (request.object_rows.empty() && request.input_rows.empty() && request.snapshot_rows.empty() &&
        request.rollback_policy.mode == RuntimeRollbackMode::disabled) {
        plan.status = RuntimeNetworkReplicationStatus::no_rows;
        return plan;
    }

    append_output_rows(plan, request);
    plan.status = plan.requires_transport_host_evidence && !plan.has_transport_host_evidence
                      ? RuntimeNetworkReplicationStatus::host_evidence_required
                      : RuntimeNetworkReplicationStatus::ready;
    return plan;
}

} // namespace mirakana::runtime
