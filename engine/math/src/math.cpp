// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/math/mat4.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/math/vec.hpp"

namespace mirakana {
static_assert(Mat4::identity().at(0, 0) == 1.0F);
}
