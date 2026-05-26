// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/network_production_security.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
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

[[nodiscard]] bool is_valid_evidence_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U;
    });
}

void add_diagnostic(RuntimeNetworkProductionSecurityPlan& plan, RuntimeNetworkProductionSecurityDiagnosticCode code,
                    std::string evidence_id, std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(RuntimeNetworkProductionSecurityDiagnostic{
        .code = code,
        .evidence_id = std::move(evidence_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_diagnostics(RuntimeNetworkProductionSecurityPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.evidence_id != rhs.evidence_id) {
            return lhs.evidence_id < rhs.evidence_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void add_evidence(RuntimeNetworkProductionSecurityPlan& plan, RuntimeNetworkProductionEvidenceKind kind,
                  std::string evidence_id, bool ready, bool host_gated, std::uint32_t source_index) {
    plan.evidence_rows.push_back(RuntimeNetworkProductionEvidenceRow{
        .kind = kind,
        .evidence_id = std::move(evidence_id),
        .ready = ready,
        .host_gated = host_gated,
        .source_index = source_index,
    });
}

[[nodiscard]] bool threat_model_complete(const RuntimeNetworkProductionThreatModelEvidenceRow& row) {
    return is_valid_evidence_id(row.evidence_id) && row.attacker_capabilities_reviewed &&
           row.trust_boundaries_reviewed && row.packet_tampering_reviewed && row.packet_replay_reviewed &&
           row.authentication_gap_reviewed && row.denial_of_service_reviewed &&
           row.nat_matchmaking_exclusions_reviewed && row.save_rollback_abuse_reviewed && row.official_sources_reviewed;
}

[[nodiscard]] bool has_validation_kind(const RuntimeNetworkProductionSecurityRequest& request,
                                       RuntimeNetworkProductionValidationKind kind) {
    return std::ranges::any_of(request.validation_evidence_rows, [kind](const auto& row) {
        return row.kind == kind && row.reviewed && is_valid_evidence_id(row.evidence_id);
    });
}

[[nodiscard]] std::size_t count_delivery_rows(const RuntimeNetworkLoopbackExchangeResult& exchange,
                                              RuntimeNetworkTransportDelivery delivery) {
    const auto count_rows = [delivery](const auto& rows) {
        return static_cast<std::size_t>(std::ranges::count_if(
            rows, [delivery](const auto& row) { return row.delivery == delivery && !row.payload.empty(); }));
    };
    return count_rows(exchange.server_received_packets) + count_rows(exchange.client_received_packets);
}

[[nodiscard]] std::size_t count_channel_policy_rows(const RuntimeNetworkLoopbackExchangeResult& exchange) {
    std::vector<std::uint8_t> channel_ids;
    const auto add_channel = [&channel_ids](const auto& row) {
        if (std::ranges::find(channel_ids, row.channel_id) == channel_ids.end()) {
            channel_ids.push_back(row.channel_id);
        }
    };
    std::ranges::for_each(exchange.server_received_packets, add_channel);
    std::ranges::for_each(exchange.client_received_packets, add_channel);
    return channel_ids.size();
}

[[nodiscard]] std::size_t request_row_count(const RuntimeNetworkProductionSecurityRequest& request) {
    return 1U + request.foundation_plan.sessions.size() + request.replication_plan.session_rows.size() +
           request.replication_plan.object_rows.size() + request.replication_plan.input_rows.size() +
           request.replication_plan.snapshot_rows.size() + request.replication_plan.rollback_rows.size() +
           request.replication_plan.transport_evidence_rows.size() + request.validation_evidence_rows.size() +
           request.unsupported_online_claim_rows.size();
}

void validate_threat_model(RuntimeNetworkProductionSecurityPlan& plan,
                           const RuntimeNetworkProductionThreatModelEvidenceRow& threat_model) {
    if (!is_valid_evidence_id(threat_model.evidence_id)) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::missing_threat_model, {},
                       "network production security requires a threat model evidence id", threat_model.source_index);
        return;
    }
    if (!threat_model_complete(threat_model)) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::incomplete_threat_model,
                       threat_model.evidence_id,
                       "threat model must cover attacker capabilities trust boundaries packet tampering replay "
                       "authentication gaps denial of service NAT matchmaking exclusions save rollback abuse and "
                       "official sources",
                       threat_model.source_index);
        return;
    }
    plan.threat_model_reviewed = true;
}

void validate_foundation(RuntimeNetworkProductionSecurityPlan& plan,
                         const RuntimeNetworkFoundationPlan& foundation_plan) {
    if (!foundation_plan.succeeded() || foundation_plan.sessions.empty()) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::invalid_foundation_plan, {},
                       "network production security requires a successful foundation plan with session rows", 0U);
        return;
    }

    plan.session_lifecycle_rows = foundation_plan.sessions.size();
    plan.connection_state_rows = 1U;
    plan.channel_policy_rows = foundation_plan.channels.size();
}

void validate_replication(RuntimeNetworkProductionSecurityPlan& plan,
                          const RuntimeNetworkReplicationPlan& replication_plan) {
    if (replication_plan.status == RuntimeNetworkReplicationStatus::ready &&
        replication_plan.has_transport_host_evidence && !replication_plan.invoked_network_io &&
        !replication_plan.invoked_rollback_execution && !replication_plan.invoked_world_mutation) {
        plan.replication_evidence_ready = true;
        return;
    }

    if (replication_plan.status == RuntimeNetworkReplicationStatus::host_evidence_required) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::missing_replication_host_evidence, {},
                       "network production security requires replication transport host evidence", 0U);
        return;
    }

    add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::invalid_replication_plan, {},
                   "network production security requires a ready side-effect-free replication plan", 0U);
}

void validate_loopback(RuntimeNetworkProductionSecurityPlan& plan,
                       const RuntimeNetworkLoopbackExchangeResult& loopback_exchange) {
    plan.loopback_host_evidence = loopback_exchange.succeeded() && loopback_exchange.packets_sent > 0U &&
                                  loopback_exchange.packets_received > 0U && loopback_exchange.bytes_sent > 0U &&
                                  loopback_exchange.bytes_received > 0U;
    if (!plan.loopback_host_evidence) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::missing_loopback_host_evidence,
                       loopback_exchange.adapter_id,
                       "network production security requires successful loopback host evidence", 0U);
        return;
    }

    plan.reliable_delivery_rows =
        count_delivery_rows(loopback_exchange, RuntimeNetworkTransportDelivery::reliable_ordered);
    plan.unreliable_delivery_rows =
        count_delivery_rows(loopback_exchange, RuntimeNetworkTransportDelivery::unreliable_unordered);
    plan.channel_policy_rows = count_channel_policy_rows(loopback_exchange);
    plan.connection_state_rows = 3U;
    plan.session_lifecycle_rows = 4U;

    if (plan.reliable_delivery_rows == 0U) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::missing_reliable_delivery,
                       loopback_exchange.adapter_id,
                       "network production security requires reliable loopback delivery evidence", 0U);
    }
    if (plan.unreliable_delivery_rows == 0U) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::missing_unreliable_delivery,
                       loopback_exchange.adapter_id,
                       "network production security requires unreliable loopback delivery evidence", 0U);
    }
}

void validate_validation_evidence(RuntimeNetworkProductionSecurityPlan& plan,
                                  const RuntimeNetworkProductionSecurityRequest& request) {
    struct RequiredValidation {
        RuntimeNetworkProductionValidationKind kind;
        RuntimeNetworkProductionSecurityDiagnosticCode code;
    };
    constexpr auto required_validations = std::array{
        RequiredValidation{RuntimeNetworkProductionValidationKind::sequence_replay_rejection,
                           RuntimeNetworkProductionSecurityDiagnosticCode::missing_sequence_replay_rejection},
        RequiredValidation{RuntimeNetworkProductionValidationKind::input_command_validation,
                           RuntimeNetworkProductionSecurityDiagnosticCode::missing_input_command_validation},
        RequiredValidation{RuntimeNetworkProductionValidationKind::snapshot_validation,
                           RuntimeNetworkProductionSecurityDiagnosticCode::missing_snapshot_validation},
        RequiredValidation{RuntimeNetworkProductionValidationKind::rollback_window_diagnostic,
                           RuntimeNetworkProductionSecurityDiagnosticCode::missing_rollback_window_diagnostic},
    };

    for (const auto validation : required_validations) {
        if (!has_validation_kind(request, validation.kind)) {
            add_diagnostic(plan, validation.code, {}, "network production security validation evidence is missing", 0U);
        }
    }

    plan.sequence_replay_rejection_rows =
        has_validation_kind(request, RuntimeNetworkProductionValidationKind::sequence_replay_rejection) ? 1U : 0U;
    plan.input_command_validation_rows =
        has_validation_kind(request, RuntimeNetworkProductionValidationKind::input_command_validation) ? 1U : 0U;
    plan.snapshot_validation_rows =
        has_validation_kind(request, RuntimeNetworkProductionValidationKind::snapshot_validation) ? 1U : 0U;
    plan.rollback_window_diagnostic_rows =
        has_validation_kind(request, RuntimeNetworkProductionValidationKind::rollback_window_diagnostic) ? 1U : 0U;
}

void validate_unsupported_claims(RuntimeNetworkProductionSecurityPlan& plan,
                                 const RuntimeNetworkProductionSecurityRequest& request) {
    for (const auto& row : request.unsupported_online_claim_rows) {
        if (!row.requested) {
            continue;
        }
        ++plan.unsupported_online_claim_rows;
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::unsupported_online_service_claim,
                       row.claim_id,
                       "network production security does not implement encryption authentication matchmaking NAT cloud "
                       "internet execution or broad multiplayer readiness",
                       row.source_index);
    }
}

void validate_side_effects(RuntimeNetworkProductionSecurityPlan& plan,
                           const RuntimeNetworkProductionSecurityRequest& request) {
    if (request.request_native_handles || request.loopback_exchange.native_handles_exposed) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::native_handle_exposure, {},
                       "network production security does not expose native transport handles", 0U);
    }
    if (request.invoked_external_network_io) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::side_effect_claim, "external-network-io",
                       "network production security gate must not execute external network IO", 0U);
    }
    if (request.invoked_threads) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::side_effect_claim, "threads",
                       "network production security gate must not start threads", 0U);
    }
    if (request.invoked_save_io) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::side_effect_claim, "save-io",
                       "network production security gate must not read or write saves", 0U);
    }
    if (request.invoked_world_mutation) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::side_effect_claim, "world-mutation",
                       "network production security gate must not mutate worlds", 0U);
    }
}

[[nodiscard]] bool is_host_gate_diagnostic(RuntimeNetworkProductionSecurityDiagnosticCode code) noexcept {
    return code == RuntimeNetworkProductionSecurityDiagnosticCode::missing_loopback_host_evidence ||
           code == RuntimeNetworkProductionSecurityDiagnosticCode::missing_replication_host_evidence;
}

[[nodiscard]] bool has_only_host_gate_diagnostics(const RuntimeNetworkProductionSecurityPlan& plan) {
    return !plan.diagnostics.empty() && std::ranges::all_of(plan.diagnostics, [](const auto& diagnostic) {
        return is_host_gate_diagnostic(diagnostic.code);
    });
}

void clear_ready_rows_for_invalid_plan(RuntimeNetworkProductionSecurityPlan& plan) {
    plan.evidence_rows.clear();
    plan.session_lifecycle_rows = 0U;
    plan.connection_state_rows = 0U;
    plan.channel_policy_rows = 0U;
    plan.reliable_delivery_rows = 0U;
    plan.unreliable_delivery_rows = 0U;
    plan.sequence_replay_rejection_rows = 0U;
    plan.input_command_validation_rows = 0U;
    plan.snapshot_validation_rows = 0U;
    plan.rollback_window_diagnostic_rows = 0U;
    plan.replay_hash = 0U;
}

void append_ready_evidence_rows(RuntimeNetworkProductionSecurityPlan& plan,
                                const RuntimeNetworkProductionSecurityRequest& request) {
    add_evidence(plan, RuntimeNetworkProductionEvidenceKind::threat_model, request.threat_model.evidence_id, true,
                 false, request.threat_model.source_index);
    add_evidence(plan, RuntimeNetworkProductionEvidenceKind::session_lifecycle, "session-lifecycle-local-loopback",
                 plan.session_lifecycle_rows > 0U, false, 0U);
    add_evidence(plan, RuntimeNetworkProductionEvidenceKind::connection_state, "connection-state-local-loopback",
                 plan.connection_state_rows > 0U, false, 0U);
    add_evidence(plan, RuntimeNetworkProductionEvidenceKind::channel_policy, "channel-policy-local-loopback",
                 plan.channel_policy_rows > 0U, false, 0U);
    add_evidence(plan, RuntimeNetworkProductionEvidenceKind::reliable_delivery, request.loopback_exchange.adapter_id,
                 plan.reliable_delivery_rows > 0U, false, 0U);
    add_evidence(plan, RuntimeNetworkProductionEvidenceKind::unreliable_delivery, request.loopback_exchange.adapter_id,
                 plan.unreliable_delivery_rows > 0U, false, 0U);
    add_evidence(plan, RuntimeNetworkProductionEvidenceKind::replication_security,
                 request.replication_plan.session_rows.empty()
                     ? "replication-security"
                     : request.replication_plan.session_rows.front().session_id,
                 plan.replication_evidence_ready, false, 0U);
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

[[nodiscard]] std::uint64_t compute_replay_hash(const RuntimeNetworkProductionSecurityPlan& plan,
                                                const RuntimeNetworkProductionSecurityRequest& request) {
    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, request.seed);
    hash_string(hash, request.threat_model.evidence_id);
    hash_mix(hash, request.foundation_plan.sessions.size());
    hash_mix(hash, request.replication_plan.replay_hash);
    hash_mix(hash, request.loopback_exchange.packets_sent);
    hash_mix(hash, request.loopback_exchange.packets_received);
    hash_mix(hash, plan.session_lifecycle_rows);
    hash_mix(hash, plan.connection_state_rows);
    hash_mix(hash, plan.channel_policy_rows);
    hash_mix(hash, plan.reliable_delivery_rows);
    hash_mix(hash, plan.unreliable_delivery_rows);
    hash_mix(hash, plan.sequence_replay_rejection_rows);
    hash_mix(hash, plan.input_command_validation_rows);
    hash_mix(hash, plan.snapshot_validation_rows);
    hash_mix(hash, plan.rollback_window_diagnostic_rows);
    return hash == 0U ? 1U : hash;
}

} // namespace

bool RuntimeNetworkProductionSecurityPlan::succeeded() const noexcept {
    return status == RuntimeNetworkProductionSecurityStatus::ready && diagnostics.empty();
}

RuntimeNetworkProductionSecurityPlan
plan_runtime_network_production_security_gate(const RuntimeNetworkProductionSecurityRequest& request) {
    RuntimeNetworkProductionSecurityPlan plan;
    plan.invoked_external_network_io = request.invoked_external_network_io;
    plan.invoked_threads = request.invoked_threads;
    plan.invoked_save_io = request.invoked_save_io;
    plan.invoked_world_mutation = request.invoked_world_mutation;

    validate_threat_model(plan, request.threat_model);
    validate_foundation(plan, request.foundation_plan);
    validate_replication(plan, request.replication_plan);
    validate_loopback(plan, request.loopback_exchange);
    validate_validation_evidence(plan, request);
    validate_unsupported_claims(plan, request);
    validate_side_effects(plan, request);

    if (request_row_count(request) > request.row_budget) {
        add_diagnostic(plan, RuntimeNetworkProductionSecurityDiagnosticCode::row_budget_exceeded, {},
                       "network production security request exceeds its row budget", 0U);
    }

    plan.general_online_ready = false;
    if (plan.diagnostics.empty()) {
        append_ready_evidence_rows(plan, request);
        plan.replay_hash = compute_replay_hash(plan, request);
        plan.status = RuntimeNetworkProductionSecurityStatus::ready;
        return plan;
    }

    sort_diagnostics(plan);
    if (has_only_host_gate_diagnostics(plan) && plan.threat_model_reviewed) {
        plan.replay_hash = compute_replay_hash(plan, request);
        plan.status = RuntimeNetworkProductionSecurityStatus::host_evidence_required;
        return plan;
    }

    clear_ready_rows_for_invalid_plan(plan);
    plan.status = RuntimeNetworkProductionSecurityStatus::invalid_request;
    return plan;
}

} // namespace mirakana::runtime
