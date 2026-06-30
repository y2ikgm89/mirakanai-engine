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
  sources/
    gltf/
    textures/
    materials/
    audio/
  notices/
    THIRD_PARTY_ASSET_NOTICES.md
  expected/
    hashes.gehashes
```

`tools/validate-asset-import-regression-corpus.ps1 -CorpusRoot
out/host-artifacts/asset-import-regression-corpus` validates the manifest,
source paths, SHA-256 hashes, provenance rows, and legal policy before any
importer execution. `-RequireReady` is reserved for hosts with an approved large
corpus plus a ready report.

## Legal Gate

Accepted tracked fixture material in this directory is first-party repository
content under `LicenseRef-Proprietary`.

Third-party host corpus assets require per-asset provenance and notice rows.
Allowed licenses are `CC0-1.0`, `MIT`, `BSD-2-Clause`, `BSD-3-Clause`,
`Apache-2.0`, `Zlib`, or `CC-BY-4.0` with attribution. Reject missing,
ambiguous, marketplace-only, `CC-BY-NC-*`, `CC-BY-ND-*`, trademark-heavy, and
external-engine sample material unless a separate legal approval record exists.
