# 2026-05-24 Networking And Multiplayer v1 Threat Model

## Status

Active for `networking-and-multiplayer-v1` until the optional ENet loopback adapter PR closes. After merge, this spec remains the retained threat-model record for the first-party runtime transport facade and optional `network-enet` adapter boundary.

## Scope

This spec covers the first `networking-and-multiplayer-v1` implementation slice: a first-party runtime network transport facade in `MK_runtime` and an optional ENet loopback adapter in `MK_runtime_network_enet`.

The supported surface is local loopback packet exchange for validation. It is not broad multiplayer, not remote session hosting, not public socket ownership, and not generated-game default network I/O.

## Assets

- First-party public runtime API stability for `mirakana::runtime` network transport values.
- Process-local validation integrity for reliable and unreliable packet round trips.
- Dependency/legal integrity for the optional ENet lane.
- Public API boundary integrity: no ENet headers, ENet symbols, sockets, or native handles in public gameplay APIs.
- Generated-game claim integrity: existing value-only networking foundation remains the default AI/game-facing surface.

## Trust Boundaries

- Game/generated code can construct first-party request values and call the facade with a caller-owned adapter pointer.
- `MK_runtime` validates requests and adapter capabilities before dispatch.
- `MK_runtime_network_enet` owns all ENet lifecycle, host, peer, packet, and socket behavior privately.
- vcpkg installs ENet only through `tools/bootstrap-deps.ps1`; CMake configure consumes an already-installed tree.
- Hosted CI or a dependency-ready local host validates the optional adapter lane.

## Threats And Controls

| Threat | Control |
| --- | --- |
| Public API leaks ENet headers, symbols, or native handles. | Public ENet adapter header exposes only a factory returning `IRuntimeNetworkTransportAdapter`; `tools/check-public-api-boundaries.ps1` scans `engine/runtime/network/enet/include` and bans `enet/`, `ENet*`, and `enet_*`. |
| A caller requests native handles or external socket ownership through the facade. | `execute_runtime_network_loopback_exchange` rejects `request_native_handles` and adapters that advertise native handles. |
| The facade accepts broad multiplayer shapes before routing exists. | v1 requires exactly one loopback client peer and rejects `peer_count != 1`. Packet `source_index` remains diagnostic row identity, not peer routing. |
| Oversized payloads bypass the first-party facade ceiling. | Payload validation uses the stricter of adapter capability and `runtime_network_transport_max_payload_bytes`. |
| Unsupported delivery, channel, peer, or service budgets cause partial dispatch. | The facade validates capability, channel count, channel ids, peer count, payload sizes, and service iterations before adapter dispatch. Failed validation returns no send/receive rows. |
| ENet packet ownership leaks or double frees. | Sent packets are owned by ENet after successful `enet_peer_send`; failed sends destroy the packet; received packets are copied into first-party rows and destroyed immediately. |
| Optional dependency install runs during CMake configure. | `network-enet` preset sets `VCPKG_MANIFEST_INSTALL=OFF` and `VCPKG_INSTALLED_DIR=${sourceDir}/vcpkg_installed`; `tools/bootstrap-deps.ps1` is the only vcpkg install entrypoint. |
| Optional loopback proof is mistaken for secure internet multiplayer. | Docs, manifest, game-code guidance, and backlog text state no encryption/authentication, NAT traversal, matchmaking, replication, rollback, WAN readiness, or broad multiplayer readiness. |

## Explicit Non-Goals

- Remote host/server productization.
- Public socket/native transport handles.
- Encryption, authentication, authorization, anti-cheat, or secure peer identity.
- Matchmaking, lobby, NAT traversal, relay, discovery, or platform network services.
- Replication, prediction, rollback, lockstep, or deterministic multiplayer scheduling.
- Multi-peer routing beyond the single loopback client proof.
- Generated-game default network I/O.
- ENet exposure through public headers or public gameplay values.

## Validation

- `MK_runtime_network_transport_adapter_tests` proves facade fail-closed behavior and successful caller-owned adapter exchange rows.
- `MK_runtime_network_enet_tests` proves optional ENet loopback exchange when `MK_ENABLE_NETWORK_ENET=ON` and `network-enet` is bootstrapped.
- `tools/validate-network-enet.ps1` configures, builds, tests, installs, and validates the installed SDK consumer against the optional ENet lane.
- `tools/check-public-api-boundaries.ps1`, `tools/check-dependency-policy.ps1`, `tools/check-json-contracts.ps1`, and `tools/check-ai-integration.ps1` guard public API, dependency, manifest, and agent-surface drift.
