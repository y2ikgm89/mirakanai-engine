// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/animation/morph.hpp"
#include "mirakana/assets/asset_source_format.hpp"

namespace mirakana {

[[nodiscard]] MorphMeshCpuSourceDocument
morph_mesh_cpu_source_document_from_animation_desc(const AnimationMorphMeshCpuDesc& desc);

[[nodiscard]] AnimationMorphMeshCpuDesc
animation_morph_mesh_cpu_desc_from_source_document(const MorphMeshCpuSourceDocument& document);

} // namespace mirakana
