---
name: gameengine-license-audit
description: Audits third-party code, dependencies, and distributable assets for license compliance. Use when changing vcpkg.json, legal notices, or shipping external material.
paths:
  - "vcpkg.json"
  - "docs/legal-and-licensing.md"
  - "docs/dependencies.md"
  - "THIRD_PARTY_NOTICES.md"
  - "tools/check-dependency-policy.ps1"
---

# GameEngine License Audit

Before external material ships, update `THIRD_PARTY_NOTICES.md` with source URL, retrieved date, version, author, SPDX license, modification status, and distribution target.

For C++ package-manager dependencies, also update `vcpkg.json`, keep optional dependencies behind manifest features, pin the official registry through `builtin-baseline`, update `docs/dependencies.md`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` after vcpkg dependency or manifest-feature changes, and run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`.

Keep vcpkg package installation out of CMake configure; dependency bootstrap belongs to `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1`.

Treat license-less material as unusable. Do not copy snippets from books, blogs, Stack Overflow, or GitHub without explicit license review.
