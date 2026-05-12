# Sample UI Audio Assets

This headless sample demonstrates runtime-facing contracts that generated games can combine without depending on editor, SDL3, Dear ImGui, or `mirakana_tools` during normal gameplay:

- retained-mode `mirakana_ui` HUD/menu model construction
- logical asset registration and first-party source document serialization
- material definition plus backend-neutral binding metadata
- cooked runtime package loading into a deserialized scene, `SceneMaterialPalette`, and renderer-neutral mesh submission
- device-independent audio clip registration, voice scheduling, render-plan generation, and interleaved float PCM generation

The sample exits with status 0 only when the UI hierarchy is deterministic, first-party asset source documents round-trip, material binding metadata is valid, a cooked scene/material package can be loaded and submitted through `NullRenderer`, and audio render planning produces scheduled commands plus interleaved float PCM without underrun diagnostics.

