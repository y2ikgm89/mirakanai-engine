# Sample Input Renderer Game

This sample demonstrates AI-friendly gameplay using the current headless engine contracts:

- `mirakana::VirtualInput` for deterministic input
- `mirakana::Transform2D` for position and scale
- `mirakana::NullRenderer` for renderer frame lifecycle validation
- `mirakana::SpriteCommand` for backend-independent draw submission
- `mirakana::GameApp` and `mirakana::HeadlessRunner` for runtime flow

The sample simulates holding the right key for four frames and verifies that the player moved to `x = 4`.

