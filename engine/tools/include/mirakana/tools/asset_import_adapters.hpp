// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/tools/asset_import_tool.hpp"

namespace mirakana {

class PngTextureExternalAssetImporter final : public IExternalAssetImporter {
  public:
    [[nodiscard]] bool supports(const AssetImportAction& action) const noexcept override;
    [[nodiscard]] std::string import_source_document(IFileSystem& filesystem, const AssetImportAction& action) override;
};

class GltfMeshExternalAssetImporter final : public IExternalAssetImporter {
  public:
    [[nodiscard]] bool supports(const AssetImportAction& action) const noexcept override;
    [[nodiscard]] std::string import_source_document(IFileSystem& filesystem, const AssetImportAction& action) override;
};

class GltfMorphMeshCpuExternalAssetImporter final : public IExternalAssetImporter {
  public:
    [[nodiscard]] bool supports(const AssetImportAction& action) const noexcept override;
    [[nodiscard]] std::string import_source_document(IFileSystem& filesystem, const AssetImportAction& action) override;
};

class AudioExternalAssetImporter final : public IExternalAssetImporter {
  public:
    [[nodiscard]] bool supports(const AssetImportAction& action) const noexcept override;
    [[nodiscard]] std::string import_source_document(IFileSystem& filesystem, const AssetImportAction& action) override;
};

struct ExternalAssetImportAdapters {
    PngTextureExternalAssetImporter png_textures;
    GltfMeshExternalAssetImporter gltf_meshes;
    GltfMorphMeshCpuExternalAssetImporter gltf_morph_meshes_cpu;
    AudioExternalAssetImporter audio_sources;

    [[nodiscard]] AssetImportExecutionOptions options();
};

[[nodiscard]] bool external_asset_importers_available() noexcept;

} // namespace mirakana
