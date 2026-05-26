// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/network_transport.hpp"
#include "mirakana/runtime/networking_foundation.hpp"
#include "mirakana/runtime/production_network_replication.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeNetworkProductionSecurityStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    invalid_request,
};

enum class RuntimeNetworkProductionValidationKind : std::uint8_t {
    sequence_replay_rejection = 0,
    input_command_validation,
    snapshot_validation,
    rollback_window_diagnostic,
};

enum class RuntimeNetworkUnsupportedOnlineClaimKind : std::uint8_t {
    encryption_execution = 0,
    peer_authentication,
    matchmaking,
    nat_traversal,
    cloud_service,
    internet_remote_execution,
    broad_multiplayer_readiness,
};

enum class RuntimeNetworkProductionEvidenceKind : std::uint8_t {
    threat_model = 0,
    session_lifecycle,
    connection_state,
    channel_policy,
    reliable_delivery,
    unreliable_delivery,
    replication_security,
};

enum class RuntimeNetworkProductionSecurityDiagnosticCode : std::uint8_t {
    missing_threat_model = 0,
    incomplete_threat_model,
    invalid_foundation_plan,
    invalid_replication_plan,
    missing_loopback_host_evidence,
    missing_replication_host_evidence,
    missing_reliable_delivery,
    missing_unreliable_delivery,
    missing_sequence_replay_rejection,
    missing_input_command_validation,
    missing_snapshot_validation,
    missing_rollback_window_diagnostic,
    unsupported_online_service_claim,
    native_handle_exposure,
    side_effect_claim,
    row_budget_exceeded,
};

struct RuntimeNetworkProductionThreatModelEvidenceRow {
    std::string evidence_id;
    bool attacker_capabilities_reviewed{false};
    bool trust_boundaries_reviewed{false};
    bool packet_tampering_reviewed{false};
    bool packet_replay_reviewed{false};
    bool authentication_gap_reviewed{false};
    bool denial_of_service_reviewed{false};
    bool nat_matchmaking_exclusions_reviewed{false};
    bool save_rollback_abuse_reviewed{false};
    bool official_sources_reviewed{false};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkProductionValidationEvidenceRow {
    RuntimeNetworkProductionValidationKind kind{RuntimeNetworkProductionValidationKind::sequence_replay_rejection};
    std::string evidence_id;
    bool reviewed{false};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkUnsupportedOnlineClaimRow {
    RuntimeNetworkUnsupportedOnlineClaimKind kind{RuntimeNetworkUnsupportedOnlineClaimKind::encryption_execution};
    std::string claim_id;
    bool requested{false};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkProductionSecurityRequest {
    RuntimeNetworkProductionThreatModelEvidenceRow threat_model;
    RuntimeNetworkFoundationPlan foundation_plan;
    RuntimeNetworkReplicationPlan replication_plan;
    RuntimeNetworkLoopbackExchangeResult loopback_exchange;
    std::vector<RuntimeNetworkProductionValidationEvidenceRow> validation_evidence_rows;
    std::vector<RuntimeNetworkUnsupportedOnlineClaimRow> unsupported_online_claim_rows;
    std::size_t row_budget{64U};
    bool request_native_handles{false};
    bool invoked_external_network_io{false};
    bool invoked_threads{false};
    bool invoked_save_io{false};
    bool invoked_world_mutation{false};
    std::uint64_t seed{0U};
};

struct RuntimeNetworkProductionEvidenceRow {
    RuntimeNetworkProductionEvidenceKind kind{RuntimeNetworkProductionEvidenceKind::threat_model};
    std::string evidence_id;
    bool ready{false};
    bool host_gated{false};
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkProductionSecurityDiagnostic {
    RuntimeNetworkProductionSecurityDiagnosticCode code{
        RuntimeNetworkProductionSecurityDiagnosticCode::missing_threat_model};
    std::string evidence_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct RuntimeNetworkProductionSecurityPlan {
    RuntimeNetworkProductionSecurityStatus status{RuntimeNetworkProductionSecurityStatus::invalid_request};
    std::vector<RuntimeNetworkProductionSecurityDiagnostic> diagnostics;
    std::vector<RuntimeNetworkProductionEvidenceRow> evidence_rows;
    std::size_t session_lifecycle_rows{0U};
    std::size_t connection_state_rows{0U};
    std::size_t channel_policy_rows{0U};
    std::size_t reliable_delivery_rows{0U};
    std::size_t unreliable_delivery_rows{0U};
    std::size_t sequence_replay_rejection_rows{0U};
    std::size_t input_command_validation_rows{0U};
    std::size_t snapshot_validation_rows{0U};
    std::size_t rollback_window_diagnostic_rows{0U};
    std::size_t unsupported_online_claim_rows{0U};
    std::uint64_t replay_hash{0U};
    bool threat_model_reviewed{false};
    bool loopback_host_evidence{false};
    bool replication_evidence_ready{false};
    bool general_online_ready{false};
    bool invoked_external_network_io{false};
    bool invoked_threads{false};
    bool invoked_save_io{false};
    bool invoked_world_mutation{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews local loopback execution, foundation session policy, replication validation, and security evidence.
/// This value-only gate consumes already-reviewed evidence rows. It does not open sockets, start threads, expose native
/// handles, perform encryption/authentication, provide matchmaking/NAT/cloud services, execute rollback, mutate worlds,
/// read/write saves, or claim broad online multiplayer readiness.
[[nodiscard]] RuntimeNetworkProductionSecurityPlan
plan_runtime_network_production_security_gate(const RuntimeNetworkProductionSecurityRequest& request);

} // namespace mirakana::runtime
