---
name: license-audit
description: Audits third-party code, dependencies, and distributable assets for license compliance. Use when changing vcpkg.json, legal notices, or shipping external material.
paths:
  - "vcpkg.json"
  - "docs/legal-and-licensing.md"
  - "docs/dependencies.md"
  - "THIRD_PARTY_NOTICES.md"
  - "tools/check-dependency-policy.ps1"
---

# License Audit

## Required Record

Before external material ships, update `THIRD_PARTY_NOTICES.md` with:

- name
- source URL
- retrieved date
- version or commit
- author or copyright holder
- SPDX license identifier or custom license reference
- modification status
- distribution target

For C++ package-manager dependencies, also update `vcpkg.json`, keep dependencies behind manifest features unless they are required by the default build, pin the official registry through `builtin-baseline`, and update `docs/dependencies.md`.

## Policy

- Prefer permissive licenses.
- Treat license-less material as unusable.
- Keep third-party code isolated.
- Preserve upstream license and notice files.
- Record substantial AI-generated distributable assets.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` after vcpkg dependency or manifest-feature changes so the root `vcpkg_installed/` package tree is explicit before build lanes use it.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1` after dependency metadata changes.
- Keep vcpkg package installation out of CMake configure; dependency bootstrap belongs to `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1`.

## Do Not

- Copy snippets from books, blogs, Stack Overflow, or GitHub without explicit license review.
- Use CC-NC material for commercial or monetized outputs.
- Use CC-ND material when modifications are required.
