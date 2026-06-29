// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_identity.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class AssetImportProvenanceOrigin : std::uint8_t { first_party, third_party, generated_ai };

struct AssetImportProvenanceRowV1 {
    AssetKeyV2 asset_key;
    AssetImportProvenanceOrigin origin{AssetImportProvenanceOrigin::third_party};
    std::string source_url;
    std::string retrieved_date;
    std::string version_or_commit;
    std::string copyright_holder;
    std::string license_id;
    std::string modification_status;
    std::string distribution_target;
    std::string notice_id;
    bool notice_complete{false};
    bool external_engine_material{false};
};

struct AssetImportProvenanceDocumentV1 {
    std::vector<AssetImportProvenanceRowV1> rows;
};

[[nodiscard]] std::string_view asset_import_provenance_origin_label(AssetImportProvenanceOrigin origin) noexcept;
[[nodiscard]] std::vector<std::string>
validate_asset_import_provenance_document(const AssetImportProvenanceDocumentV1& document);
[[nodiscard]] std::string serialize_asset_import_provenance_document(const AssetImportProvenanceDocumentV1& document);
[[nodiscard]] AssetImportProvenanceDocumentV1 deserialize_asset_import_provenance_document(std::string_view text);

} // namespace mirakana
