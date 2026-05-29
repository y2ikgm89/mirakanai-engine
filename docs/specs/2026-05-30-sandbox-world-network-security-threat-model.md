# 2026-05-30 Sandbox World Network And Modding Gate Threat Model

## Status

Active implementation evidence for `docs/superpowers/plans/2026-05-30-sandbox-world-network-modding-gate-v1.md`.

## Goal

Define the security boundary for generic 2D sandbox world mutation replication and modding policy rows. This spec extends the existing runtime networking and scripting gates with sandbox-world-specific value rows only, including `plan_runtime_network_replication` sandbox rows and `plan_runtime_script_modding_policy`; it does not promote broad online multiplayer, remote service, or general modding readiness.

## Official Sources Checked

- Context7 `/lsalzman/enet` on 2026-05-30 for ENet initialization, host creation/destruction, peer connections, packet reliability flags, `enet_host_service` polling, and packet lifetime rules.
- Context7 `/websites/cmake_cmake_help` on 2026-05-30 for target-based CTest registration through `add_test(NAME ... COMMAND ...)`.
- Context7 `/microsoft/vcpkg` on 2026-05-30 for optional dependencies in manifest features and explicit feature installation.
- Existing retained specs: `docs/specs/2026-05-24-networking-and-multiplayer-v1-threat-model.md` and `docs/specs/2026-05-26-networking-production-security-threat-model.md`.

## Assets

- Sandbox world mutation command rows and their serialized payload hashes.
- Snapshot delta rows that describe changed sandbox cells without exposing save files or package payloads.
- Network foundation, replication, and optional transport host evidence rows.
- Script and modding adapter policy rows, including deterministic replay seeds and denied capability counters.
- Package-visible validation counters that must not be broadened into internet, secure transport, or broad modding claims.

## Trust Boundaries

- Remote clients are untrusted. They may submit malformed, replayed, reordered, duplicated, oversized, or tampered sandbox mutation commands.
- The authoritative session is server or host owned. Client mutation commands are input intent only; they do not directly mutate worlds.
- Snapshot deltas are reviewed state evidence only. They do not perform persistence IO, package IO, or save migration.
- `MK_runtime_network_enet` remains an optional host adapter. ENet handles, sockets, peers, packets, and native transport state stay private to that adapter.
- Script/mod metadata is untrusted until reviewed through first-party policy rows. Filesystem, network, process, native plugin, and package mutation capabilities are denied by default.

## Attacker Capabilities

- Change tile ids, chunk coordinates, layer/cell coordinates, command ids, sequence numbers, target ticks, or payload hashes.
- Replay an old mutation command inside or outside the rollback window.
- Use a state-replication channel for client mutation input, or an input channel for snapshot deltas.
- Reference unknown mutation commands from a snapshot delta.
- Inflate command or delta byte counts to exhaust bandwidth, memory, or rollback history budgets.
- Claim ENet loopback proof as encryption, authentication, NAT traversal, matchmaking, internet readiness, or broad online readiness.
- Request script filesystem access, network access, process spawning, native plugins, package mutation, or unreviewed host APIs.

## Required Mitigations

- Mutation command rows must validate backend-neutral ids, channel authority, monotonic target ticks per player/channel, unique sequences per player/channel, positive byte counts, and non-zero payload hashes.
- Snapshot delta rows must validate backend-neutral ids, server/host authoritative channels, increasing base-to-target ticks, monotonic deltas per channel, non-empty changed-cell counts, non-zero state hashes, a shared snapshot/delta byte budget, and references to unique reviewed mutation command ids.
- Rollback policies must remain bounded by reviewed replay prerequisites and snapshot history.
- Transport host evidence is optional and host-gated. Missing evidence may produce `host_evidence_required`; invalid evidence must fail closed.
- Modding adapter policy rows must be reviewed, deterministic, and capability-denied by default for filesystem, network, process, native plugin, and package mutation requests.
- Security evidence may report local loopback or value-row readiness only. Broad online and broad modding readiness remain false.

## Non-Goals

- No encryption, authentication, account identity, anti-cheat, matchmaking, NAT traversal, relay, cloud service, or internet-facing server implementation.
- No rollback, prediction, or authoritative world mutation execution.
- No filesystem, package, save, renderer, RHI, platform, editor, thread, or native handle side effects.
- No Lua, WASM, JIT, plugin loading, runtime source parsing, or broad modding runtime.
- No SDL3 or legacy desktop middleware adapter.

## Promotion Rule

The selected Phase 9 claim may state that sandbox world mutation replication and modding policy gates have deterministic value-row review, fail-closed diagnostics, host-gated optional transport evidence, and denied-by-default script/mod capabilities.

The selected Phase 9 claim must not state that the engine is internet multiplayer ready, secure transport ready, server authoritative at runtime, rollback execution ready, or generally modding ready.
