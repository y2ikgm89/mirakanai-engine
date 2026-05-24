// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/physics/native_adapter.hpp"

#include <memory>

namespace mirakana {

[[nodiscard]] PhysicsNative3DAdapterCapabilities jolt_physics_3d_adapter_capabilities();

[[nodiscard]] std::unique_ptr<IPhysicsNative3DAdapter> make_jolt_physics_3d_adapter();

} // namespace mirakana
