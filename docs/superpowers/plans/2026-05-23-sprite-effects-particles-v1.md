# Sprite Effects Particles v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add budgeted first-party sprite effects and particle planning for transient 2D visuals such as damage numbers, trails, hit flashes, and short-lived effects, then prove package-visible counters in the selected 2D runtime package.

**Plan ID:** `sprite-effects-particles-v1`

**Status:** Planned.

**Gap:** `sprite-effects-particles-v1`

**Architecture:** Planned next child of Candidate Backlog Burn-down v1. Keep the runtime contract value-only and backend-neutral, integrate with existing scene/sprite renderer telemetry only through reviewed rows, and avoid native/RHI handle exposure.

**Tech Stack:** C++23, `MK_runtime`, `MK_scene_renderer`, `MK_renderer` statistics as needed, repository `tools/*.ps1`, composed engine agent manifest fragments.

## Official Docs Review

- Use official CMake target-scoped patterns for new tests/targets.
- Use first-party engine contracts for authored effect rows and renderer statistics; do not add middleware or native backend types to public gameplay APIs.

## Non-Goals

- GPU particle simulation, editor timeline tooling, external VFX middleware, native/RHI handles, source image decoding, Metal parity, subjective visual-quality claims, or broad renderer readiness.

## Done When

- Unit tests prove deterministic effect/particle rows, budget diagnostics, and stable ordering.
- `sample_2d_desktop_runtime_package --require-sprite-effects-particles` exits 0 with package-visible effect counters.
- Backlog row promoted; manifest/docs/static checks synced; focused validation and `validate.ps1` green; PR merged.
