# Sample Headless Game

This game is intentionally small. It demonstrates the current AI-friendly gameplay contract:

- implement `mirakana::GameApp`
- use `mirakana::Registry` for runtime entities/components
- run through `mirakana::HeadlessRunner`
- keep game code outside engine modules

Build and test through:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```
