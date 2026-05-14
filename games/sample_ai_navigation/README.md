# Sample AI Navigation

Headless sample proving that generated-game code can compose `mirakana_ai` and `mirakana_navigation` through public `mirakana::` APIs.

The sample builds a deterministic grid path, smooths it through dependency-free grid line-of-sight, converts it to point waypoints, builds an explicit route-target `mirakana_ai` perception snapshot, projects target facts into a typed blackboard, evaluates blackboard-driven behavior tree condition nodes plus a game-owned action leaf, and advances a value-type navigation agent until it reaches the destination. It is a source-tree validation proof only: async behavior tree tasks, decorators/services, scene/physics perception integration, navmesh assets, full crowd simulation, scene/physics navigation integration, renderer presentation, and editor graph tooling are not implemented here.

Validate through:

```powershell
cmake --build --preset dev --target sample_ai_navigation
ctest --preset dev --output-on-failure -R "^sample_ai_navigation$"
```
