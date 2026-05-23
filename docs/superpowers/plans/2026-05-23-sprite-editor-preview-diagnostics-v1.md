# Sprite Editor Preview Diagnostics v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add retained editor and game-facing diagnostics for selected sprites, atlas pages, animation frames, sorting metadata, hitboxes, and package dependencies so operators can review 2D sprite readiness without renderer-native handle exposure.

**Plan ID:** `sprite-editor-preview-diagnostics-v1`

**Status:** Planned.

**Gap:** `sprite-editor-preview-diagnostics-v1`

**Architecture:** Planned next child of Candidate Backlog Burn-down v1. Keep diagnostics value-oriented and reviewable, prefer existing editor-core retained models and runtime diagnostics contracts, and keep visible preview execution host-gated.

**Tech Stack:** C++23, `MK_editor_core`, `MK_editor`, `MK_runtime`, `MK_scene`, `MK_scene_renderer` only through public rows as needed, repository `tools/*.ps1`, composed engine agent manifest fragments.

## Official Docs Review

- Use project editor retained-model conventions and official target-scoped CMake patterns for new tests.
- Use first-party diagnostics rows and package manifests as the source of truth; do not scrape renderer-native objects or mutate game packages from preview code.

## Non-Goals

- Production sprite editor UX polish, source image decoding/import breadth, GPU/RHI preview handles, live package mutation, arbitrary shell execution, Vulkan/Metal preview parity, subjective visual-quality claims, or broad renderer readiness.

## Done When

- Unit tests prove retained diagnostics for selected sprite/package inputs and fail-closed malformed references.
- Visible/editor or package-facing smoke evidence is host-gated and reports selected diagnostic rows without native handle exposure.
- Backlog row promoted; manifest/docs/static checks synced; focused validation and `validate.ps1` green; PR merged.
