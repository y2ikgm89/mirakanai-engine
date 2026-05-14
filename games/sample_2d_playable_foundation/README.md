# Sample 2D Playable Foundation

Source-tree proof for the `2d-playable-source-tree` recipe.

This sample runs through `mirakana::HeadlessRunner`, `mirakana::VirtualInput`, `mirakana::runtime::RuntimeInputActionMap`,
`mirakana::Scene` orthographic camera plus sprite nodes, `mirakana::validate_playable_2d_scene`, `mirakana::submit_scene_render_packet`,
`mirakana::UiDocument`/`mirakana::submit_ui_renderer_submission`, `mirakana::AudioMixer`, and `mirakana::NullRenderer`.

It does not claim a native window, texture atlas cook, tilemap editor UX, runtime image decoding, or GPU-visible package
proof. Those remain separate host-gated or planned slices.

Validate through the default source-tree lane:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```
