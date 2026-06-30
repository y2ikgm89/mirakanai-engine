# Asset Import Regression Fixtures

These fixtures are first-party, text-only regression inputs for
`GameEngine.AssetImportRegressionCorpus.v1`.

## Source Tree Fixture Corpus

- `first_party_corpus.gecorpus` is the committed tiny corpus manifest.
- `sources/gltf/*.gltf` are generated first-party glTF JSON fixtures.
- `sources/materials/*.gematerial` are generated first-party material-document
  fixtures.
- The committed corpus does not include large binary assets, marketplace
  material, external-engine samples, Unity assets, Unreal Engine assets, Godot
  assets, logos, trademarks, or compatibility claims.

The fixture corpus exists to prove deterministic manifest/provenance/hash
validation and to give future importer-runner tests stable source paths and
diagnostic intent labels. PNG, OpenEXR, KTX2/Basis, audio, and large real-asset
coverage must be generated or supplied by a reviewed host corpus before importer
execution claims are made.

## Host-Owned Large Corpus Layout

Host-owned corpora are not tracked by the repository by default. Use this layout:

```text
out/host-artifacts/asset-import-regression-corpus/
  corpus.gecorpus
  report.gereport
  sources/
    gltf/
    textures/
    materials/
    audio/
  notices/
    THIRD_PARTY_ASSET_NOTICES.md
  expected/
    hashes.gehashes
  retained/
    official-source-ledger.md
    corpus-selection-summary.md
```

`tools/generate-asset-import-regression-corpus-manifest.ps1` creates
`corpus.gecorpus` and `expected/hashes.gehashes` from already-local files under
`sources/` plus operator-supplied `key=value` rows in
`notices/THIRD_PARTY_ASSET_NOTICES.md`. The generator does not download assets,
infer licenses, or approve notices. Use `-FailOnMissingNotice` so each
`provenance.notice_id` must also appear as an explicit non-metadata notice line.
The generator requires the canonical host layout paths exactly:
`-SourcesRoot <corpus>/sources`, `-NoticesPath
<corpus>/notices/THIRD_PARTY_ASSET_NOTICES.md`, and `-OutputManifest
<corpus>/corpus.gecorpus`.

`tools/validate-asset-import-regression-corpus.ps1 -CorpusRoot
out/host-artifacts/asset-import-regression-corpus` validates the manifest,
source paths, SHA-256 hashes, provenance rows, and legal policy before any
importer execution. `-RequireReady` additionally requires `report.gereport`,
canonical `corpus.gecorpus`, `expected/hashes.gehashes`,
`notices/THIRD_PARTY_ASSET_NOTICES.md`, source files under the canonical
`sources/<category>/` directory for their asset kind,
`retained/official-source-ledger.md`, and
`retained/corpus-selection-summary.md`, plus minimum commercial composition
evidence: at least 40 glTF rows, 30 texture rows, 20 material rows, 20 animation
rows, and 20 audio rows. `-RequireReady` is reserved for hosts with an approved
large corpus plus a ready report.

`retained/corpus-selection-summary.md` is `key=value` metadata. Required count
keys are `gltf_rows`, `texture_rows`, `material_rows`, `animation_rows`, and
`audio_rows`. Required category flags must be set to `true`: `gltf.mesh_only`,
`gltf.scene`, `gltf.skin`, `gltf.morph`, `gltf.material`,
`gltf.external_buffer`, `gltf.external_image`, `gltf.animation_trs`,
`gltf.quaternion_animation`, `gltf.invalid_external_path`,
`gltf.invalid_extension`, `gltf.invalid_accessor`,
`gltf.unsupported_interpolation`, `texture.png`, `texture.openexr`,
`texture.ktx2_basis`, `texture.color_management`, `texture.alpha`,
`texture.hdr_metadata`, `texture.mip_array_cubemap`,
`texture.needs_transcoding`, `texture.backend_target_policy`,
`material.factor_only`, `material.texture_backed`,
`material.missing_dependency`, `material.duplicated_texture_reference`,
`material.unsupported_graph_export`, `material.package_output`,
`animation.node_trs`, `animation.skin`, `animation.morph_weight`,
`animation.valid_quaternion`, `animation.invalid_quaternion`,
`animation.unit_up_axis_conversion`,
`animation.unsupported_scalar_z_up_rotation`, `audio.wav`, `audio.mp3`,
`audio.flac`, `audio.static_pcm`, `audio.streaming_source_review`,
`audio.invalid_truncated_decode`, `audio.channel_sample_rate_variance`, and
`audio.loop_normalization_preset`. Matching manifest `required_feature` tokens
use underscore names, such as `gltf_mesh_only`, `texture_ktx2_basis`,
`animation_invalid_quaternion`, and `audio_loop_normalization_preset`.

## Legal Gate

Accepted tracked fixture material in this directory is first-party repository
content under `LicenseRef-Proprietary`.

Third-party host corpus assets require per-asset provenance and notice rows.
Allowed licenses are `CC0-1.0`, `MIT`, `BSD-2-Clause`, `BSD-3-Clause`,
`Apache-2.0`, `Zlib`, or `CC-BY-4.0` with attribution. Reject missing,
ambiguous, marketplace-only, `CC-BY-NC-*`, `CC-BY-ND-*`, trademark-heavy, and
external-engine sample material unless a separate legal approval record exists.
