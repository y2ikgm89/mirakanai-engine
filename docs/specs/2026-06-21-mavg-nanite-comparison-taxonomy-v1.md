# 2026-06-21 MAVG Nanite Comparison Taxonomy v1

## Purpose

Define a reproducible clean-room comparison taxonomy between MIRAIKANAI MAVG evidence and public Epic Nanite documentation without claiming Nanite format compatibility, replacement equivalence, superiority, or Unreal Engine interoperability.

## Status

Task 11 contract for `mavg-advanced-backend-evidence-closeout-v1`.

Implemented surface:

- `GameEngine.MavgNaniteComparisonReport.v1`
- `mavg-nanite-comparison-report-v1`
- `tools/validate-mavg-nanite-comparison.ps1`
- `mavg_nanite_comparison_report_ready=1`

This ready row is limited to a public-doc clean-room comparison taxonomy. It does not claim Nanite compatibility, Nanite equivalence, superiority over Nanite, Unreal Engine interoperability, runtime backend execution, renderer/RHI readiness, Metal readiness, deformation integration, ray tracing integration, broad optimization, or package-visible MAVG backend readiness.

## Official Sources

| Source | Constraint used by this report |
| --- | --- |
| Epic Nanite Virtualized Geometry Overview, https://dev.epicgames.com/documentation/unreal-engine/nanite-virtualized-geometry-in-unreal-engine | Nanite is an Unreal Engine virtualized geometry system using an internal mesh format, compressed data, fine-grained streaming, automatic LOD, cluster-based rendering, and visibility-driven work. |
| Epic Nanite Technical Details, https://dev.epicgames.com/documentation/unreal-engine/nanite-technical-details | Fallback mesh, precision, residency, trimming, explicit tangent, ray-tracing fallback, and platform/material/deformation limits are comparison axes, not compatibility permissions. |

## Comparison Axes

The report schema `GameEngine.MavgNaniteComparisonReport.v1` requires these public-doc axes:

- `virtualized_geometry_system`
- `internal_compressed_mesh_format`
- `fine_grained_streaming`
- `automatic_lod`
- `cluster_visibility_culling`
- `fallback_mesh_behavior`
- `ray_tracing_fallback_behavior`
- `material_support_limits`
- `deformation_support_limits`
- `platform_support_limits`
- `storage_memory_residency`
- `authoring_import_workflow`

## Clean-Room Boundaries

- Use only first-party MIRAIKANAI assets, generated scenes, and public Epic documentation.
- Do not copy Unreal Engine source code, private Nanite formats, shader code, sample assets, screenshots, benchmark captures, or proprietary behavior.
- Treat public Nanite documentation as taxonomy input only. It cannot establish runtime interoperability, binary/data format compatibility, equivalent quality, equivalent performance, or superiority.
- Keep MAVG evidence scoped to MIRAIKANAI MAVG rows. Environment, renderer, commercial, or all-engine readiness rows are not part of this report.

## Required Output Counters

`tools/validate-mavg-nanite-comparison.ps1` emits:

- `mavg_nanite_comparison_report_status=ready`
- `mavg_nanite_comparison_report_ready=1`
- `mavg_nanite_comparison_axes=12`
- `mavg_nanite_official_source_rows=2`
- `mavg_nanite_public_docs_only=1`
- `mavg_nanite_first_party_assets_only=1`
- `mavg_nanite_unreal_source_used=0`
- `mavg_nanite_private_format_used=0`
- `mavg_nanite_epic_sample_assets_used=0`
- `mavg_nanite_compatible=0`
- `mavg_nanite_equivalent=0`
- `mavg_nanite_superior=0`
- `mavg_nanite_marketing_claim_allowed=0`

## Claim Policy

`mavg_nanite_comparison_report_ready=1` means only that this clean-room comparison taxonomy exists, is schema-backed, and is validated.

These claims remain false by construction:

- `mavg_nanite_compatible=0`
- `mavg_nanite_equivalent=0`
- `mavg_nanite_superior=0`

A future operator who wants any of those words must create a separate legal, benchmark, and public wording plan. This taxonomy cannot be used as that claim.
