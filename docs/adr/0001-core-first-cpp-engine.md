# ADR 0001: Core-First C++ Engine

## Status

Accepted. Historical initial milestone completed; the current `core-first-mvp` closure record is `docs/superpowers/plans/2026-05-01-core-first-mvp-closure.md`.

## Decision

Build the engine as a C++23 core-first project. At the time of this ADR, the first milestone excluded desktop windows, mobile lifecycle integration, real graphics backends, asset importers, editor UI, and external runtime dependencies.

## Rationale

The engine must support both 2D and 3D long term, and it must be easy for AI agents to modify safely. A small headless core gives tests, ownership rules, and API shape before platform and renderer complexity enters the system.

## Consequences

- The first playable graphics output is deferred.
- Core contracts can be validated without GPU access.
- Renderer and platform layers can be added behind explicit interfaces.
- Backward compatibility is not maintained during early architecture discovery.
