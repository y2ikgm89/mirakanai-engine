# 2026-05-26 Networking Production Security Threat Model

## Goal

Define the production networking security boundary for MIRAIKANAI Engine Phase 5. The engine may prove selected local loopback transport, session policy, and replication review evidence, but it must not claim internet multiplayer, matchmaking, NAT traversal, encryption, authentication, cloud service, or broad online readiness until those systems have their own official-doc-backed implementation and host evidence.

## Status

Active evidence spec for Runtime Network Production Security Gate v1. First-party value and package proofs are expected to run in the default validation lane; optional ENet host-loopback proof remains dependency-gated until `network-enet` packages are bootstrapped through `tools/bootstrap-deps.ps1` in an approval-capable session.

## Official Sources Checked

- ENet official repository and documentation surface: <https://github.com/lsalzman/enet>
- Context7 `/lsalzman/enet` documentation check on 2026-05-26 for `enet_initialize`, `enet_deinitialize`, `enet_host_create`, `enet_host_connect`, `enet_host_service`, `enet_packet_create`, `ENET_PACKET_FLAG_RELIABLE`, `enet_peer_send`, `enet_host_flush`, and receive-packet destruction.

ENet provides reliable UDP transport primitives and packet/channel semantics. It does not provide encryption, peer authentication, matchmaking, NAT traversal, cloud account identity, anti-cheat, persistence security, or authorization policy by itself.

## Assets

- Runtime session topology, trust-boundary, transport-capability, channel, and replay prerequisite rows.
- Local loopback transport execution evidence and delivery counters.
- Replication session, object, input-command, snapshot, rollback-policy, and replay-hash rows.
- Package-visible networking counters used by generated-game and sample-package validation.
- Host-gated optional adapter validation evidence, including `tools/validate-network-enet.ps1`.

## Trust Boundaries

- `MK_runtime` public value contracts are trusted only as validation and planning surfaces; they must remain side-effect-free unless an API explicitly owns a host-gated execution adapter.
- `MK_runtime_network_enet` is an optional developer-owned host adapter. ENet headers, ENet pointers, sockets, and native transport handles stay private to that adapter.
- Remote peers are untrusted. Packets may be malformed, replayed, reordered, delayed, duplicated, dropped, or intentionally oversized.
- Package samples are proof surfaces. They can report exact counters but cannot broaden a local proof into an online-service claim.

## Attacker Capabilities

- Tamper with packet payload bytes, delivery channels, sequence numbers, target ticks, and snapshot object references.
- Replay old input commands or snapshots within or beyond rollback windows.
- Flood sessions with excessive payload sizes, packet counts, channels, peers, or service iterations.
- Claim unsupported transport features such as encryption, authentication, NAT traversal, matchmaking, cloud routing, or native socket ownership.
- Abuse save or rollback metadata by presenting inconsistent replay prerequisites, non-deterministic state, invalid history windows, or stale snapshots.
- Attempt to bypass first-party adapters by requesting native handles, platform sockets, backend APIs, renderer/RHI handles, threads, file IO, or world mutation.

## Evidence Topics

- Packet Tampering And Replay
- Authentication Gap
- Denial Of Service
- NAT And Matchmaking Exclusions
- Save And Rollback Abuse

## Required Mitigations

- Threat model evidence must explicitly cover attacker capabilities, trust boundaries, packet tampering/replay, authentication gaps, denial-of-service budgets, NAT/matchmaking exclusions, and save/rollback abuse.
- The transport facade must fail closed before adapter dispatch for missing adapters, unsupported reliable/unreliable delivery, invalid peer/channel/service budgets, oversized or empty payloads, native-handle requests, and adapter exceptions.
- The production security gate must require local loopback host evidence before selected networking execution is considered ready.
- The replication review must validate an already successful networking foundation plan, authoritative topology, channel authority, object identity, input sequence uniqueness, input tick monotonicity, snapshot tick monotonicity, snapshot object references, snapshot byte budgets, rollback prerequisites, rollback windows, and deterministic replay hashes.
- Unsupported claims for encryption, authentication, matchmaking, NAT traversal, cloud service, internet remote execution, and broad multiplayer readiness must produce diagnostics and keep the general online-readiness flag false.
- Default validation can use value-only and local loopback proofs. Real internet, external service, NAT, account identity, encrypted/authenticated session execution, and hostile network testing require explicit future host-gated recipes.

## Non-Goals

- No new encryption, authentication, matchmaking, NAT traversal, cloud transport, account identity, anti-cheat, external service, or internet-facing execution implementation.
- No native socket or ENet handle exposure in public gameplay APIs.
- No generated-game default network IO.
- No rollback or prediction execution.
- No world, package, save, file, thread, renderer, RHI, platform, or editor side effects in the production security gate.

## Promotion Rule

The selected Phase 5 claim may be:

- A threat-modeled local networking execution/security lane is implemented for reviewed loopback transport plus replication planning evidence.
- Optional ENet loopback host validation is available through the host-gated `network-enet` recipe.

The selected Phase 5 claim must not be:

- Broad online multiplayer readiness.
- Secure transport readiness.
- Authentication readiness.
- NAT/matchmaking/cloud readiness.
- Internet-facing production service readiness.
