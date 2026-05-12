# Engine agent manifest fragments (SSOT)

The committed [`manifest.json`](../manifest.json) is **generated**. Edit these JSON files only, then refresh the canonical manifest:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

Validation enforces semantic equality: `tools/check-json-contracts.ps1` runs `compose-agent-manifest.ps1 -Verify` before other manifest assertions.

## File naming

Fragments merge in **lexicographic order** by filename. Use `NNN-descriptiveName.json` (`000-`, `001-`, …) so order stays stable.

Each file is a JSON object whose **top-level keys** become top-level keys in `manifest.json`. Keys must not repeat across files.

## Bootstrap / repair

To rebuild fragments from the current `manifest.json` (overwrites this directory’s `*.json`):

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -SplitFromCanonical
```

Then prefer editing fragments going forward and use `-Write` to emit `manifest.json`.

See [ADR 0002](../../docs/adr/0002-agent-manifest-fragments-compose.md).
