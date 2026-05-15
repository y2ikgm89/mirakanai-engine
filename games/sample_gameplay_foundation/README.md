# Sample Gameplay Foundation

Target: `sample_gameplay_foundation`.

This headless sample demonstrates gameplay-facing engine foundations that AI agents can reuse when generating games:

- `mirakana::PhysicsWorld3D` for deterministic 3D motion, AABB contact detection, and contact resolution.
- `mirakana::build_physics_world_3d_from_authored_collision_scene` and `mirakana::move_physics_character_controller_3d` for reviewed in-memory collision rows and conservative capsule grounding.
- `mirakana::NavigationGrid`, `mirakana::plan_navigation_grid_agent_path`, and `mirakana::update_navigation_agent` for deterministic route setup and value-type agent ticks.
- `mirakana::build_ai_perception_snapshot_2d`, `mirakana::write_ai_perception_blackboard`, and `mirakana::evaluate_behavior_tree` for explicit perception target facts and blackboard-driven movement decisions.
- `mirakana::AudioMixer` and `mirakana::render_audio_device_stream_interleaved_float` for a bounded device-independent audio stream pump.
- `mirakana::AnimationStateMachine` for clip/state/trigger/blend animation flow.
- `mirakana::HeadlessRunner` and `mirakana::Registry` for testable game lifecycle and entity state.

The sample exits with status 0 only when the simulated actor lands on the static floor, moves forward deterministically, transitions from `idle` to `walk`, builds two authored collision rows, grounds a controller, reaches the navigation destination through an AI behavior-tree decision, renders two audio stream frames, and cleans up its entity on stop.

This is source-tree headless composition evidence only. It does not claim generated package shipping, scene/physics AI integration, navmesh/crowd, physics joints/exact casts/CCD, audio codec streaming, async AI services, middleware, SDL3, Dear ImGui, or public native handles.
