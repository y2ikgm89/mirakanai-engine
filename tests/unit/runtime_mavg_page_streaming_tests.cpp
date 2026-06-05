// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/runtime/mavg_page_streaming.hpp"

#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgClusterGraphDocument make_page_streaming_graph() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/page-streaming");
    const auto source_mesh = mirakana::AssetId::from_name("meshes/page-streaming-source");
    const auto material = mirakana::AssetId::from_name("materials/page-streaming-material");

    return mirakana::MavgClusterGraphDocument{
        .asset = graph_asset,
        .source_mesh = source_mesh,
        .source_mesh_uri = "source/page-streaming.glb",
        .cluster_payload_uri = "runtime/page-streaming.mavg_payload",
        .target_cluster_triangles = 2,
        .page_size_bytes = 4096,
        .pages =
            {
                mirakana::MavgClusterGraphPage{
                    .page_index = 0, .byte_offset = 0, .byte_size = 256, .first_cluster = 0, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 1, .byte_offset = 256, .byte_size = 256, .first_cluster = 1, .cluster_count = 1},
                mirakana::MavgClusterGraphPage{
                    .page_index = 2, .byte_offset = 512, .byte_size = 256, .first_cluster = 2, .cluster_count = 1},
            },
        .material_partitions =
            {
                mirakana::MavgClusterGraphMaterialPartition{
                    .material = material, .first_cluster = 0, .cluster_count = 3},
            },
        .clusters =
            {
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 0,
                    .page_index = 0,
                    .local_cluster_index = 0,
                    .lod_level = 0,
                    .triangle_count = 2,
                    .vertex_count = 4,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = -1.0F, .y = -1.0F, .z = -1.0F},
                                                     .max = mirakana::MavgVec3f{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = false,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 8.0F,
                    .first_index = 0,
                    .index_count = 6,
                    .vertex_base = 0,
                    .children = {1, 2},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 1,
                    .page_index = 1,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 1,
                    .vertex_count = 3,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = -1.0F, .y = -1.0F, .z = 0.0F},
                                                     .max = mirakana::MavgVec3f{.x = 0.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 6,
                    .index_count = 3,
                    .vertex_base = 4,
                    .children = {},
                },
                mirakana::MavgClusterGraphCluster{
                    .cluster_index = 2,
                    .page_index = 2,
                    .local_cluster_index = 0,
                    .lod_level = 1,
                    .triangle_count = 1,
                    .vertex_count = 3,
                    .bounds = mirakana::MavgBounds3f{.min = mirakana::MavgVec3f{.x = 0.0F, .y = -1.0F, .z = 0.0F},
                                                     .max = mirakana::MavgVec3f{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    .material_partition = 0,
                    .parent_cluster_index = 0,
                    .has_parent = true,
                    .resident_fallback_cluster_index = 0,
                    .geometric_error = 1.0F,
                    .first_index = 9,
                    .index_count = 3,
                    .vertex_base = 7,
                    .children = {},
                },
            },
    };
}

[[nodiscard]] mirakana::MavgLodPageRequest make_request(mirakana::AssetId graph_asset, std::uint32_t page_index,
                                                        float priority, std::string_view reason) {
    return mirakana::MavgLodPageRequest{
        .graph_asset = graph_asset,
        .page_index = page_index,
        .priority = priority,
        .reason = std::string(reason),
    };
}

[[nodiscard]] mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2 make_candidate(std::uint32_t page_index) {
    return mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
        .package_index_path = "runtime/mavg/page" + std::to_string(page_index) + ".geindex",
        .content_root = "runtime/mavg",
        .label = "mavg-page-" + std::to_string(page_index),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeMavgPageStreamingCandidateRow
make_streaming_candidate(mirakana::AssetId graph_asset, std::uint32_t page_index) {
    return mirakana::runtime::RuntimeMavgPageStreamingCandidateRow{
        .graph_asset = graph_asset,
        .page_index = page_index,
        .candidate = make_candidate(page_index),
    };
}

void write_page_package(mirakana::MemoryFileSystem& filesystem, std::uint32_t page_index, mirakana::AssetId asset,
                        mirakana::AssetKind kind, std::string_view content) {
    const auto package_path = "page" + std::to_string(page_index) + ".payload";
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = asset,
                                                                      .kind = kind,
                                                                      .path = package_path,
                                                                      .content = std::string(content),
                                                                      .source_revision = page_index + 1U,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text("runtime/mavg/page" + std::to_string(page_index) + ".geindex",
                          mirakana::serialize_asset_cooked_package_index(index));
    filesystem.write_text("runtime/mavg/" + package_path, content);
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_runtime_package(mirakana::AssetId asset,
                                                                          mirakana::AssetKind kind,
                                                                          std::uint32_t handle_value,
                                                                          std::string_view content) {
    return mirakana::runtime::RuntimeAssetPackage({mirakana::runtime::RuntimeAssetRecord{
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = handle_value},
        .asset = asset,
        .kind = kind,
        .path = "runtime/mavg/resident-" + std::to_string(handle_value) + ".geasset",
        .content_hash = asset.value + handle_value,
        .source_revision = handle_value,
        .dependencies = {},
        .content = std::string(content),
    }});
}

void mount_resident_page(mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set, std::uint32_t mount_id,
                         mirakana::AssetId asset, std::string_view content) {
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = mount_id},
                       .label = "mavg-page-" + std::to_string(mount_id),
                       .package = make_runtime_package(asset, mirakana::AssetKind::mesh, mount_id, content),
                   })
                   .succeeded());
}

[[nodiscard]] bool has_diagnostic(const mirakana::runtime::RuntimeMavgPageStreamingPlanResult& result,
                                  mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_diagnostic(const mirakana::runtime::RuntimeMavgPageStreamingEvictionReviewResult& result,
                                  mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_diagnostic(const mirakana::runtime::RuntimeMavgPageStreamingDispatchPlan& result,
                                  mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_diagnostic(const mirakana::runtime::RuntimeMavgPageStreamingWorkerResult& result,
                                  mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_diagnostic(const mirakana::runtime::RuntimeMavgResidentPageUseGenerationResult& result,
                                  mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode code) {
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool contains_mount_id(const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2>& mount_ids,
                                     mirakana::runtime::RuntimeResidentPackageMountIdV2 mount_id) {
    for (const auto candidate : mount_ids) {
        if (candidate == mount_id) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime mavg page streaming planner coalesces nonresident requests deterministically") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 0, 10.0F, "already-resident"),
        make_request(graph.asset, 2, 2.0F, "prefetch-low"),
        make_request(graph.asset, 1, 4.0F, "visible-missing"),
        make_request(graph.asset, 2, 8.0F, "visible-near"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 1),
        make_streaming_candidate(graph.asset, 2),
    };

    const auto result = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, candidates);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.input_request_count == requests.size());
    MK_REQUIRE(result.resident_skip_count == 1U);
    MK_REQUIRE(result.duplicate_request_count == 1U);
    MK_REQUIRE(!result.budget_degraded);
    MK_REQUIRE(result.queued_page_requests.size() == 2U);
    MK_REQUIRE(result.queued_page_requests[0].page_index == 2U);
    MK_REQUIRE(result.queued_page_requests[0].priority == 8.0F);
    MK_REQUIRE(result.queued_page_requests[0].reason == "visible-near");
    MK_REQUIRE(result.queued_page_requests[0].duplicate_count == 1U);
    MK_REQUIRE(result.queued_page_requests[0].candidate.package_index_path == "runtime/mavg/page2.geindex");
    MK_REQUIRE(result.queued_page_requests[1].page_index == 1U);
    MK_REQUIRE(result.queued_page_requests[1].priority == 4.0F);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_streaming);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg page streaming planner applies deterministic max page budget") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 1, 4.0F, "visible-missing"),
        make_request(graph.asset, 2, 8.0F, "visible-near"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 1),
        make_streaming_candidate(graph.asset, 2),
    };

    const auto result = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
            .max_queued_pages = 1,
        },
        requests, candidates);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.budget_degraded);
    MK_REQUIRE(result.budget_dropped_request_count == 1U);
    MK_REQUIRE(result.queued_page_requests.size() == 1U);
    MK_REQUIRE(result.queued_page_requests[0].page_index == 2U);
}

MK_TEST("runtime mavg page streaming planner rejects invalid request rows without side effects") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const auto other_graph = mirakana::AssetId::from_name("mavg/other");
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(other_graph, 1, 4.0F, "wrong-graph"),
        make_request(graph.asset, 99, 5.0F, "unknown-page"),
        make_request(graph.asset, 2, std::numeric_limits<float>::quiet_NaN(), "invalid-priority"),
        make_request(graph.asset, 2, 3.0F, ""),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 2),
    };

    const auto result = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, candidates);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.queued_page_requests.empty());
    MK_REQUIRE(
        has_diagnostic(result, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::request_graph_mismatch));
    MK_REQUIRE(has_diagnostic(result, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::unknown_page));
    MK_REQUIRE(has_diagnostic(result, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::invalid_priority));
    MK_REQUIRE(has_diagnostic(result, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::invalid_reason));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.executed_streaming);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg page streaming planner rejects missing package candidates") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 1, 4.0F, "visible-missing"),
    };

    const auto result = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, {});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.missing_candidate_count == 1U);
    MK_REQUIRE(has_diagnostic(result, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::missing_candidate));
}

MK_TEST("runtime mavg page streaming dispatch planner builds safe point rows deterministically") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 1, 4.0F, "visible-left"),
        make_request(graph.asset, 2, 8.0F, "visible-right"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 1),
        make_streaming_candidate(graph.asset, 2),
    };
    const auto plan = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, candidates);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.queued_page_requests.size() == 2U);

    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> mount_ids{{.value = 42}, {.value = 43}};
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> eviction_order{{.value = 12}};
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> protected_mounts{{.value = 10},
                                                                                           {.value = 11}};
    const auto dispatch = mirakana::runtime::plan_runtime_mavg_page_streaming_dispatches(
        mirakana::runtime::RuntimeMavgPageStreamingDispatchDesc{
            .queued_page_requests = plan.queued_page_requests,
            .mount_ids = mount_ids,
            .eviction_candidate_unmount_order = eviction_order,
            .protected_mount_ids = protected_mounts,
            .budget = mirakana::runtime::RuntimeResourceResidencyBudgetV2{.max_resident_content_bytes = 4096},
            .mode = mirakana::runtime::RuntimeMavgPageStreamingDispatchMode::caller_owned_safe_point,
        });

    MK_REQUIRE(dispatch.succeeded());
    MK_REQUIRE(dispatch.input_request_count == 2U);
    MK_REQUIRE(dispatch.dispatch_mount_id_count == 2U);
    MK_REQUIRE(!dispatch.budget_degraded);
    MK_REQUIRE(dispatch.dispatch_rows.size() == 2U);
    MK_REQUIRE(dispatch.dispatch_rows[0].dispatch_index == 0U);
    MK_REQUIRE(dispatch.dispatch_rows[0].mode ==
               mirakana::runtime::RuntimeMavgPageStreamingDispatchMode::caller_owned_safe_point);
    MK_REQUIRE(dispatch.dispatch_rows[0].safe_point_required);
    MK_REQUIRE(!dispatch.dispatch_rows[0].background_worker_owned_by_caller);
    MK_REQUIRE(dispatch.dispatch_rows[0].drain_desc.row.page_index == 2U);
    MK_REQUIRE(dispatch.dispatch_rows[0].drain_desc.mount_id ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 42});
    MK_REQUIRE(dispatch.dispatch_rows[0].drain_desc.overlay ==
               mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins);
    MK_REQUIRE(dispatch.dispatch_rows[0].drain_desc.budget.max_resident_content_bytes == 4096U);
    MK_REQUIRE(dispatch.dispatch_rows[0].drain_desc.eviction_candidate_unmount_order.size() == 1U);
    MK_REQUIRE(dispatch.dispatch_rows[0].drain_desc.protected_mount_ids.size() == 2U);
    MK_REQUIRE(dispatch.dispatch_rows[1].dispatch_index == 1U);
    MK_REQUIRE(dispatch.dispatch_rows[1].drain_desc.row.page_index == 1U);
    MK_REQUIRE(dispatch.dispatch_rows[1].drain_desc.mount_id ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 43});
    MK_REQUIRE(!dispatch.invoked_file_io);
    MK_REQUIRE(!dispatch.mutated_mount_set);
    MK_REQUIRE(!dispatch.executed_streaming);
    MK_REQUIRE(!dispatch.executed_background_worker);
    MK_REQUIRE(!dispatch.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg page streaming dispatch planner applies deterministic max dispatch budget") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 1, 4.0F, "visible-left"),
        make_request(graph.asset, 2, 8.0F, "visible-right"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 1),
        make_streaming_candidate(graph.asset, 2),
    };
    const auto plan = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, candidates);
    MK_REQUIRE(plan.succeeded());
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> mount_ids{{.value = 42}, {.value = 43}};

    const auto dispatch = mirakana::runtime::plan_runtime_mavg_page_streaming_dispatches(
        mirakana::runtime::RuntimeMavgPageStreamingDispatchDesc{
            .queued_page_requests = plan.queued_page_requests,
            .mount_ids = mount_ids,
            .max_dispatch_rows = 1,
        });

    MK_REQUIRE(dispatch.succeeded());
    MK_REQUIRE(dispatch.budget_degraded);
    MK_REQUIRE(dispatch.budget_dropped_request_count == 1U);
    MK_REQUIRE(dispatch.dispatch_rows.size() == 1U);
    MK_REQUIRE(dispatch.dispatch_rows[0].drain_desc.row.page_index == 2U);
    MK_REQUIRE(dispatch.dispatch_rows[0].drain_desc.mount_id ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 42});
    MK_REQUIRE(!dispatch.executed_streaming);
    MK_REQUIRE(!dispatch.executed_background_worker);
}

MK_TEST("runtime mavg page streaming dispatch planner records caller owned background queue without executing worker") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 2, 8.0F, "visible-right"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 2),
    };
    const auto plan = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, candidates);
    MK_REQUIRE(plan.succeeded());
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> mount_ids{{.value = 42}};

    const auto dispatch = mirakana::runtime::plan_runtime_mavg_page_streaming_dispatches(
        mirakana::runtime::RuntimeMavgPageStreamingDispatchDesc{
            .queued_page_requests = plan.queued_page_requests,
            .mount_ids = mount_ids,
            .mode = mirakana::runtime::RuntimeMavgPageStreamingDispatchMode::caller_owned_background_queue,
        });

    MK_REQUIRE(dispatch.succeeded());
    MK_REQUIRE(dispatch.caller_owned_background_queue);
    MK_REQUIRE(!dispatch.engine_owned_background_worker);
    MK_REQUIRE(dispatch.requires_safe_point);
    MK_REQUIRE(dispatch.dispatch_rows.size() == 1U);
    MK_REQUIRE(dispatch.dispatch_rows[0].mode ==
               mirakana::runtime::RuntimeMavgPageStreamingDispatchMode::caller_owned_background_queue);
    MK_REQUIRE(dispatch.dispatch_rows[0].background_worker_owned_by_caller);
    MK_REQUIRE(!dispatch.dispatch_rows[0].background_worker_owned_by_engine);
    MK_REQUIRE(dispatch.dispatch_rows[0].safe_point_required);
    MK_REQUIRE(!dispatch.invoked_file_io);
    MK_REQUIRE(!dispatch.mutated_mount_set);
    MK_REQUIRE(!dispatch.executed_streaming);
    MK_REQUIRE(!dispatch.executed_background_worker);
    MK_REQUIRE(!dispatch.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg page streaming dispatch planner records engine owned background worker without executing it") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 2, 8.0F, "visible-right"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 2),
    };
    const auto plan = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, candidates);
    MK_REQUIRE(plan.succeeded());
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> mount_ids{{.value = 42}};

    const auto dispatch = mirakana::runtime::plan_runtime_mavg_page_streaming_dispatches(
        mirakana::runtime::RuntimeMavgPageStreamingDispatchDesc{
            .queued_page_requests = plan.queued_page_requests,
            .mount_ids = mount_ids,
            .mode = mirakana::runtime::RuntimeMavgPageStreamingDispatchMode::engine_owned_background_worker,
        });

    MK_REQUIRE(dispatch.succeeded());
    MK_REQUIRE(!dispatch.caller_owned_background_queue);
    MK_REQUIRE(dispatch.engine_owned_background_worker);
    MK_REQUIRE(dispatch.requires_safe_point);
    MK_REQUIRE(dispatch.dispatch_rows.size() == 1U);
    MK_REQUIRE(dispatch.dispatch_rows[0].mode ==
               mirakana::runtime::RuntimeMavgPageStreamingDispatchMode::engine_owned_background_worker);
    MK_REQUIRE(!dispatch.dispatch_rows[0].background_worker_owned_by_caller);
    MK_REQUIRE(dispatch.dispatch_rows[0].background_worker_owned_by_engine);
    MK_REQUIRE(dispatch.dispatch_rows[0].safe_point_required);
    MK_REQUIRE(!dispatch.invoked_file_io);
    MK_REQUIRE(!dispatch.mutated_mount_set);
    MK_REQUIRE(!dispatch.executed_streaming);
    MK_REQUIRE(!dispatch.executed_background_worker);
    MK_REQUIRE(!dispatch.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg page streaming dispatch planner rejects invalid dispatch rows before side effects") {
    const auto graph = make_page_streaming_graph();
    auto invalid_row = mirakana::runtime::RuntimeMavgPageStreamingPlanRow{
        .graph_asset = graph.asset,
        .page_index = 2,
        .priority = std::numeric_limits<float>::quiet_NaN(),
        .reason = "",
        .candidate = make_candidate(2),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingPlanRow> rows{invalid_row};
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> invalid_mount_ids{{.value = 0}};

    const auto invalid = mirakana::runtime::plan_runtime_mavg_page_streaming_dispatches(
        mirakana::runtime::RuntimeMavgPageStreamingDispatchDesc{
            .queued_page_requests = rows,
            .mount_ids = invalid_mount_ids,
        });

    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(invalid.dispatch_rows.empty());
    MK_REQUIRE(
        has_diagnostic(invalid, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::invalid_dispatch_row));
    MK_REQUIRE(
        has_diagnostic(invalid, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::invalid_dispatch_mount_id));
    MK_REQUIRE(!invalid.invoked_file_io);
    MK_REQUIRE(!invalid.mutated_mount_set);
    MK_REQUIRE(!invalid.executed_streaming);
    MK_REQUIRE(!invalid.executed_background_worker);
    MK_REQUIRE(!invalid.touched_renderer_or_rhi_handles);

    invalid_row.priority = 1.0F;
    invalid_row.reason = "valid-row";
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingPlanRow> valid_rows{invalid_row, invalid_row};
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> duplicate_mount_ids{{.value = 42},
                                                                                              {.value = 42}};
    const auto duplicate = mirakana::runtime::plan_runtime_mavg_page_streaming_dispatches(
        mirakana::runtime::RuntimeMavgPageStreamingDispatchDesc{
            .queued_page_requests = valid_rows,
            .mount_ids = duplicate_mount_ids,
        });

    MK_REQUIRE(!duplicate.succeeded());
    MK_REQUIRE(duplicate.dispatch_rows.empty());
    MK_REQUIRE(duplicate.duplicate_dispatch_mount_id_count == 1U);
    MK_REQUIRE(has_diagnostic(duplicate,
                              mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::duplicate_dispatch_mount_id));
}

MK_TEST("runtime mavg page streaming dispatch planner rejects unsafe no safe point mutation boundary") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 2, 8.0F, "visible-right"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 2),
    };
    const auto plan = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, candidates);
    MK_REQUIRE(plan.succeeded());
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> mount_ids{{.value = 42}};

    const auto dispatch = mirakana::runtime::plan_runtime_mavg_page_streaming_dispatches(
        mirakana::runtime::RuntimeMavgPageStreamingDispatchDesc{
            .queued_page_requests = plan.queued_page_requests,
            .mount_ids = mount_ids,
            .require_safe_point = false,
        });

    MK_REQUIRE(!dispatch.succeeded());
    MK_REQUIRE(dispatch.dispatch_rows.empty());
    MK_REQUIRE(
        has_diagnostic(dispatch, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::unsafe_dispatch_mode));
    MK_REQUIRE(!dispatch.invoked_file_io);
    MK_REQUIRE(!dispatch.mutated_mount_set);
    MK_REQUIRE(!dispatch.executed_streaming);
    MK_REQUIRE(!dispatch.executed_background_worker);
    MK_REQUIRE(!dispatch.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg page streaming executes one queued row through reviewed safe point") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const auto streamed_asset = mirakana::AssetId::from_name("mavg/page-streaming/page-2-payload");
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 2, 8.0F, "visible-near"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 2),
    };
    auto filesystem = mirakana::MemoryFileSystem{};
    write_page_package(filesystem, 2, streamed_asset, mirakana::AssetKind::mesh, "page two payload");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;

    const auto plan = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, candidates);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.queued_page_requests.size() == 1U);

    const auto drain = mirakana::runtime::execute_runtime_mavg_page_streaming_request_safe_point(
        filesystem, mount_set, catalog_cache,
        mirakana::runtime::RuntimeMavgPageStreamingDrainDesc{
            .row = plan.queued_page_requests[0],
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 42},
            .budget = mirakana::runtime::RuntimeResourceResidencyBudgetV2{.max_resident_content_bytes = 4096},
        });

    MK_REQUIRE(drain.succeeded());
    MK_REQUIRE(drain.committed);
    MK_REQUIRE(drain.executed_safe_point);
    MK_REQUIRE(drain.invoked_candidate_load);
    MK_REQUIRE(drain.invoked_catalog_refresh);
    MK_REQUIRE(!drain.executed_background_worker);
    MK_REQUIRE(!drain.touched_renderer_or_rhi_handles);
    MK_REQUIRE(mount_set.mounts().size() == 1U);
    MK_REQUIRE(catalog_cache.has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), streamed_asset).has_value());
}

MK_TEST("runtime mavg page streaming drain rejects invalid mount id before mutation") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 2, 8.0F, "visible-near"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 2),
    };
    auto filesystem = mirakana::MemoryFileSystem{};
    write_page_package(filesystem, 2, mirakana::AssetId::from_name("mavg/page-streaming/page-2-payload"),
                       mirakana::AssetKind::mesh, "page two payload");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const auto plan = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, candidates);
    MK_REQUIRE(plan.succeeded());

    const auto drain = mirakana::runtime::execute_runtime_mavg_page_streaming_request_safe_point(
        filesystem, mount_set, catalog_cache,
        mirakana::runtime::RuntimeMavgPageStreamingDrainDesc{
            .row = plan.queued_page_requests[0],
            .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 0},
            .budget = mirakana::runtime::RuntimeResourceResidencyBudgetV2{.max_resident_content_bytes = 4096},
        });

    MK_REQUIRE(!drain.succeeded());
    MK_REQUIRE(!drain.committed);
    MK_REQUIRE(drain.executed_safe_point);
    MK_REQUIRE(drain.invoked_candidate_load == false);
    MK_REQUIRE(mount_set.mounts().empty());
    MK_REQUIRE(!catalog_cache.has_value());
    MK_REQUIRE(!drain.executed_background_worker);
    MK_REQUIRE(!drain.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg page streaming worker executes engine owned dispatch rows") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const auto page_one_asset = mirakana::AssetId::from_name("mavg/page-streaming/page-1-payload");
    const auto page_two_asset = mirakana::AssetId::from_name("mavg/page-streaming/page-2-payload");
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 1, 4.0F, "visible-left"),
        make_request(graph.asset, 2, 8.0F, "visible-right"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 1),
        make_streaming_candidate(graph.asset, 2),
    };
    auto filesystem = mirakana::MemoryFileSystem{};
    write_page_package(filesystem, 1, page_one_asset, mirakana::AssetKind::mesh, "page one payload");
    write_page_package(filesystem, 2, page_two_asset, mirakana::AssetKind::mesh, "page two payload");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;

    const auto plan = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, candidates);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.queued_page_requests.size() == 2U);
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> mount_ids{{.value = 42}, {.value = 43}};
    const auto dispatch = mirakana::runtime::plan_runtime_mavg_page_streaming_dispatches(
        mirakana::runtime::RuntimeMavgPageStreamingDispatchDesc{
            .queued_page_requests = plan.queued_page_requests,
            .mount_ids = mount_ids,
            .budget = mirakana::runtime::RuntimeResourceResidencyBudgetV2{.max_resident_content_bytes = 4096},
            .mode = mirakana::runtime::RuntimeMavgPageStreamingDispatchMode::engine_owned_background_worker,
        });
    MK_REQUIRE(dispatch.succeeded());

    const auto worker = mirakana::runtime::execute_runtime_mavg_page_streaming_worker(
        mirakana::runtime::RuntimeMavgPageStreamingWorkerDesc{
            .filesystem = &filesystem,
            .mount_set = &mount_set,
            .catalog_cache = &catalog_cache,
        },
        dispatch);

    MK_REQUIRE(worker.succeeded());
    MK_REQUIRE(worker.input_dispatch_row_count == 2U);
    MK_REQUIRE(worker.executed_row_count == 2U);
    MK_REQUIRE(worker.committed_row_count == 2U);
    MK_REQUIRE(worker.failed_row_count == 0U);
    MK_REQUIRE(worker.drain_results.size() == 2U);
    MK_REQUIRE(worker.drain_results[0].row.page_index == 2U);
    MK_REQUIRE(worker.drain_results[1].row.page_index == 1U);
    MK_REQUIRE(worker.drain_results[0].executed_background_worker);
    MK_REQUIRE(worker.drain_results[1].executed_background_worker);
    MK_REQUIRE(worker.invoked_file_io);
    MK_REQUIRE(worker.mutated_mount_set);
    MK_REQUIRE(worker.executed_streaming);
    MK_REQUIRE(worker.executed_background_worker);
    MK_REQUIRE(!worker.touched_renderer_or_rhi_handles);
    MK_REQUIRE(mount_set.mounts().size() == 2U);
    MK_REQUIRE(catalog_cache.has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), page_one_asset).has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), page_two_asset).has_value());
}

MK_TEST("runtime mavg page streaming worker rejects caller owned dispatch plans before mutation") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 2, 8.0F, "visible-right"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 2),
    };
    auto filesystem = mirakana::MemoryFileSystem{};
    write_page_package(filesystem, 2, mirakana::AssetId::from_name("mavg/page-streaming/page-2-payload"),
                       mirakana::AssetKind::mesh, "page two payload");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const auto plan = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, candidates);
    MK_REQUIRE(plan.succeeded());
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> mount_ids{{.value = 42}};
    const auto dispatch = mirakana::runtime::plan_runtime_mavg_page_streaming_dispatches(
        mirakana::runtime::RuntimeMavgPageStreamingDispatchDesc{
            .queued_page_requests = plan.queued_page_requests,
            .mount_ids = mount_ids,
            .mode = mirakana::runtime::RuntimeMavgPageStreamingDispatchMode::caller_owned_background_queue,
        });
    MK_REQUIRE(dispatch.succeeded());

    const auto worker = mirakana::runtime::execute_runtime_mavg_page_streaming_worker(
        mirakana::runtime::RuntimeMavgPageStreamingWorkerDesc{
            .filesystem = &filesystem,
            .mount_set = &mount_set,
            .catalog_cache = &catalog_cache,
        },
        dispatch);

    MK_REQUIRE(!worker.succeeded());
    MK_REQUIRE(has_diagnostic(
        worker, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::unsupported_worker_dispatch_mode));
    MK_REQUIRE(worker.executed_row_count == 0U);
    MK_REQUIRE(!worker.invoked_file_io);
    MK_REQUIRE(!worker.mutated_mount_set);
    MK_REQUIRE(!worker.executed_streaming);
    MK_REQUIRE(!worker.executed_background_worker);
    MK_REQUIRE(!worker.touched_renderer_or_rhi_handles);
    MK_REQUIRE(mount_set.mounts().empty());
    MK_REQUIRE(!catalog_cache.has_value());
}

MK_TEST("runtime mavg page streaming worker applies deterministic max row budget") {
    const auto graph = make_page_streaming_graph();
    const auto resident = mirakana::MavgLodResidentPageSet{.page_indices = {0}};
    const std::vector<mirakana::MavgLodPageRequest> requests{
        make_request(graph.asset, 1, 4.0F, "visible-left"),
        make_request(graph.asset, 2, 8.0F, "visible-right"),
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingCandidateRow> candidates{
        make_streaming_candidate(graph.asset, 1),
        make_streaming_candidate(graph.asset, 2),
    };
    auto filesystem = mirakana::MemoryFileSystem{};
    write_page_package(filesystem, 1, mirakana::AssetId::from_name("mavg/page-streaming/page-1-payload"),
                       mirakana::AssetKind::mesh, "page one payload");
    write_page_package(filesystem, 2, mirakana::AssetId::from_name("mavg/page-streaming/page-2-payload"),
                       mirakana::AssetKind::mesh, "page two payload");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const auto plan = mirakana::runtime::plan_runtime_mavg_page_streaming_requests(
        mirakana::runtime::RuntimeMavgPageStreamingPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_pages = &resident,
        },
        requests, candidates);
    MK_REQUIRE(plan.succeeded());
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> mount_ids{{.value = 42}, {.value = 43}};
    const auto dispatch = mirakana::runtime::plan_runtime_mavg_page_streaming_dispatches(
        mirakana::runtime::RuntimeMavgPageStreamingDispatchDesc{
            .queued_page_requests = plan.queued_page_requests,
            .mount_ids = mount_ids,
            .mode = mirakana::runtime::RuntimeMavgPageStreamingDispatchMode::engine_owned_background_worker,
        });
    MK_REQUIRE(dispatch.succeeded());

    const auto worker = mirakana::runtime::execute_runtime_mavg_page_streaming_worker(
        mirakana::runtime::RuntimeMavgPageStreamingWorkerDesc{
            .filesystem = &filesystem,
            .mount_set = &mount_set,
            .catalog_cache = &catalog_cache,
            .max_worker_rows = 1,
        },
        dispatch);

    MK_REQUIRE(worker.succeeded());
    MK_REQUIRE(worker.budget_degraded);
    MK_REQUIRE(worker.budget_dropped_row_count == 1U);
    MK_REQUIRE(worker.executed_row_count == 1U);
    MK_REQUIRE(worker.committed_row_count == 1U);
    MK_REQUIRE(worker.drain_results.size() == 1U);
    MK_REQUIRE(worker.drain_results[0].row.page_index == 2U);
    MK_REQUIRE(worker.executed_background_worker);
    MK_REQUIRE(mount_set.mounts().size() == 1U);
}

MK_TEST("runtime mavg page streaming eviction review protects selected pages and fallback ancestors") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "root");
    mount_resident_page(mount_set, 11, mirakana::AssetId::from_name("mavg/page-streaming/page-1"), "visible");
    mount_resident_page(mount_set, 12, mirakana::AssetId::from_name("mavg/page-streaming/page-2"), "evict");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
        {.graph_asset = graph.asset, .page_index = 2, .mount_id = {.value = 12}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = graph.asset, .cluster_index = 1},
    };
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> reviewed_candidates{{.value = 12}};

    const auto result = mirakana::runtime::review_runtime_mavg_page_streaming_evictions(
        mount_set, mirakana::runtime::RuntimeMavgPageStreamingEvictionReviewDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = page_mounts,
                       .reviewed_candidate_unmount_order = reviewed_candidates,
                       .target_budget =
                           mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                               .max_resident_content_bytes = 11,
                           },
                   });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.invoked_eviction_plan);
    MK_REQUIRE(!result.inferred_eviction_policy);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(result.protected_visible_page_count == 1U);
    MK_REQUIRE(result.protected_fallback_page_count == 1U);
    MK_REQUIRE(
        contains_mount_id(result.protected_mount_ids, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 10}));
    MK_REQUIRE(
        contains_mount_id(result.protected_mount_ids, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11}));
    MK_REQUIRE(result.eviction_candidate_unmount_order.size() == 1U);
    MK_REQUIRE(result.eviction_candidate_unmount_order[0] ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12});
    MK_REQUIRE(result.eviction_plan.status == mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::planned);
    MK_REQUIRE(result.eviction_plan.steps.size() == 1U);
    MK_REQUIRE(result.eviction_plan.steps[0].mount_id ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12});
    MK_REQUIRE(mount_set.mounts().size() == 3U);
}

MK_TEST("runtime mavg page streaming eviction review rejects protected selected eviction candidate") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "root");
    mount_resident_page(mount_set, 11, mirakana::AssetId::from_name("mavg/page-streaming/page-1"), "visible");
    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = graph.asset, .cluster_index = 1},
    };
    const std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> reviewed_candidates{{.value = 11}};

    const auto result = mirakana::runtime::review_runtime_mavg_page_streaming_evictions(
        mount_set, mirakana::runtime::RuntimeMavgPageStreamingEvictionReviewDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = page_mounts,
                       .reviewed_candidate_unmount_order = reviewed_candidates,
                       .target_budget =
                           mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                               .max_resident_content_bytes = 4,
                           },
                   });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.invoked_eviction_plan);
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::protected_candidate_mount_id);
    MK_REQUIRE(!result.inferred_eviction_policy);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(mount_set.mounts().size() == 2U);
}

MK_TEST("runtime mavg page streaming eviction review rejects invalid selected rows before eviction planning") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "root");
    const auto other_graph = mirakana::AssetId::from_name("mavg/other");
    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = other_graph, .cluster_index = 1},
        {.graph_asset = graph.asset, .cluster_index = 99},
        {.graph_asset = graph.asset, .cluster_index = 2},
    };

    const auto result = mirakana::runtime::review_runtime_mavg_page_streaming_evictions(
        mount_set, mirakana::runtime::RuntimeMavgPageStreamingEvictionReviewDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = page_mounts,
                       .target_budget =
                           mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                               .max_resident_content_bytes = 4,
                           },
                   });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.invoked_eviction_plan);
    MK_REQUIRE(
        has_diagnostic(result, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::selected_graph_mismatch));
    MK_REQUIRE(has_diagnostic(result, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::unknown_cluster));
    MK_REQUIRE(has_diagnostic(result, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::missing_page_mount));
    MK_REQUIRE(!result.inferred_eviction_policy);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg page streaming automatic eviction policy orders unprotected resident pages deterministically") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "root");
    mount_resident_page(mount_set, 11, mirakana::AssetId::from_name("mavg/page-streaming/page-1"), "visible");
    mount_resident_page(mount_set, 12, mirakana::AssetId::from_name("mavg/page-streaming/page-2"), "evict");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 2, .mount_id = {.value = 12}},
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = graph.asset, .cluster_index = 0},
    };

    const auto result = mirakana::runtime::plan_runtime_mavg_page_streaming_automatic_evictions(
        mount_set, mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPlanDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = page_mounts,
                       .target_budget =
                           mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                               .max_resident_content_bytes = 4,
                           },
                   });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.planned_automatic_eviction_policy);
    MK_REQUIRE(!result.inferred_eviction_policy);
    MK_REQUIRE(result.invoked_eviction_plan);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(result.protected_visible_page_count == 1U);
    MK_REQUIRE(result.protected_fallback_page_count == 0U);
    MK_REQUIRE(result.protected_eviction_candidate_skip_count == 1U);
    MK_REQUIRE(result.automatic_eviction_candidate_count == 2U);
    MK_REQUIRE(result.eviction_candidate_unmount_order.size() == 2U);
    MK_REQUIRE(result.eviction_candidate_unmount_order[0] ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12});
    MK_REQUIRE(result.eviction_candidate_unmount_order[1] ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11});
    MK_REQUIRE(result.eviction_plan.status == mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::planned);
    MK_REQUIRE(result.eviction_plan.steps.size() == 2U);
    MK_REQUIRE(result.eviction_plan.steps[0].mount_id ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12});
    MK_REQUIRE(result.eviction_plan.steps[1].mount_id ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11});
    MK_REQUIRE(mount_set.mounts().size() == 3U);
}

MK_TEST("runtime mavg page streaming automatic eviction policy rejects duplicate resident page rows before planning") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "root");
    mount_resident_page(mount_set, 11, mirakana::AssetId::from_name("mavg/page-streaming/page-1"), "visible");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 11}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = graph.asset, .cluster_index = 0},
    };

    const auto result = mirakana::runtime::plan_runtime_mavg_page_streaming_automatic_evictions(
        mount_set, mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPlanDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = page_mounts,
                       .target_budget =
                           mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                               .max_resident_content_bytes = 4,
                           },
                   });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.planned_automatic_eviction_policy);
    MK_REQUIRE(!result.invoked_eviction_plan);
    MK_REQUIRE(has_diagnostic(result, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::duplicate_page_mount));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(mount_set.mounts().size() == 2U);
}

MK_TEST("runtime mavg page streaming caller supplied recency orders older resident pages first") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "new-cold");
    mount_resident_page(mount_set, 11, mirakana::AssetId::from_name("mavg/page-streaming/page-1"), "old-cold");
    mount_resident_page(mount_set, 12, mirakana::AssetId::from_name("mavg/page-streaming/page-2"), "old-hot");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 2, .mount_id = {.value = 12}},
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingRecencyRow> recency_rows{
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resident_page_last_used_generation = 30},
        {.graph_asset = graph.asset,
         .page_index = 1,
         .mount_id = {.value = 11},
         .resident_page_last_used_generation = 10},
        {.graph_asset = graph.asset,
         .page_index = 2,
         .mount_id = {.value = 12},
         .resident_page_last_used_generation = 20},
    };

    const auto result = mirakana::runtime::plan_runtime_mavg_page_streaming_automatic_evictions(
        mount_set,
        mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_page_mounts = page_mounts,
            .policy_kind =
                mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency,
            .recency_rows = recency_rows,
            .target_budget =
                mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                    .max_resident_content_bytes = 4,
                },
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.planned_automatic_eviction_policy);
    MK_REQUIRE(result.applied_caller_supplied_recency_policy);
    MK_REQUIRE(!result.inferred_eviction_policy);
    MK_REQUIRE(result.recency_eviction_candidate_count == 3U);
    MK_REQUIRE(result.eviction_candidate_unmount_order.size() == 3U);
    MK_REQUIRE(result.eviction_candidate_unmount_order[0] ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11});
    MK_REQUIRE(result.eviction_candidate_unmount_order[1] ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12});
    MK_REQUIRE(result.eviction_candidate_unmount_order[2] ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 10});
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg page streaming caller supplied recency rejects duplicate rows before planning") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "root");
    mount_resident_page(mount_set, 11, mirakana::AssetId::from_name("mavg/page-streaming/page-1"), "visible");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingRecencyRow> recency_rows{
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resident_page_last_used_generation = 10},
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resident_page_last_used_generation = 11},
    };

    const auto result = mirakana::runtime::plan_runtime_mavg_page_streaming_automatic_evictions(
        mount_set,
        mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_page_mounts = page_mounts,
            .policy_kind =
                mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency,
            .recency_rows = recency_rows,
            .target_budget =
                mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                    .max_resident_content_bytes = 4,
                },
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.applied_caller_supplied_recency_policy);
    MK_REQUIRE(!result.invoked_eviction_plan);
    MK_REQUIRE(result.duplicate_recency_row_count == 1U);
    MK_REQUIRE(
        has_diagnostic(result, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::duplicate_recency_row));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(mount_set.mounts().size() == 2U);
}

MK_TEST("runtime mavg page streaming caller supplied recency requires one row per eviction candidate") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "root");
    mount_resident_page(mount_set, 11, mirakana::AssetId::from_name("mavg/page-streaming/page-1"), "visible");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingRecencyRow> recency_rows{
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resident_page_last_used_generation = 10},
    };

    const auto result = mirakana::runtime::plan_runtime_mavg_page_streaming_automatic_evictions(
        mount_set,
        mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_page_mounts = page_mounts,
            .policy_kind =
                mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency,
            .recency_rows = recency_rows,
            .target_budget =
                mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                    .max_resident_content_bytes = 4,
                },
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.applied_caller_supplied_recency_policy);
    MK_REQUIRE(!result.invoked_eviction_plan);
    MK_REQUIRE(result.missing_recency_row_count == 1U);
    MK_REQUIRE(has_diagnostic(result, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::missing_recency_row));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(mount_set.mounts().size() == 2U);
}

MK_TEST("runtime mavg page streaming infers selected page generations and carries unselected rows") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "root");
    mount_resident_page(mount_set, 11, mirakana::AssetId::from_name("mavg/page-streaming/page-1"), "visible");
    mount_resident_page(mount_set, 12, mirakana::AssetId::from_name("mavg/page-streaming/page-2"), "cold");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
        {.graph_asset = graph.asset, .page_index = 2, .mount_id = {.value = 12}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = graph.asset, .cluster_index = 1},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingRecencyRow> previous_recency_rows{
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resident_page_last_used_generation = 5},
        {.graph_asset = graph.asset,
         .page_index = 1,
         .mount_id = {.value = 11},
         .resident_page_last_used_generation = 4},
        {.graph_asset = graph.asset,
         .page_index = 2,
         .mount_id = {.value = 12},
         .resident_page_last_used_generation = 3},
    };

    const auto result = mirakana::runtime::infer_runtime_mavg_resident_page_use_generations(
        mount_set, mirakana::runtime::RuntimeMavgResidentPageUseGenerationDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = page_mounts,
                       .previous_recency_rows = previous_recency_rows,
                       .current_use_generation = 7,
                   });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.inferred_resident_page_use_generation);
    MK_REQUIRE(result.input_resident_page_mount_count == 3U);
    MK_REQUIRE(result.input_selected_cluster_count == 1U);
    MK_REQUIRE(result.output_recency_row_count == 3U);
    MK_REQUIRE(result.touched_resident_page_count == 1U);
    MK_REQUIRE(result.carried_recency_row_count == 2U);
    MK_REQUIRE(result.new_resident_page_count == 0U);
    MK_REQUIRE(result.dropped_nonresident_recency_row_count == 0U);
    MK_REQUIRE(result.duplicate_recency_row_count == 0U);
    MK_REQUIRE(result.missing_page_mount_count == 0U);
    MK_REQUIRE(result.recency_rows.size() == 3U);
    MK_REQUIRE(result.recency_rows[0].page_index == 0U);
    MK_REQUIRE(result.recency_rows[0].resident_page_last_used_generation == 5U);
    MK_REQUIRE(result.recency_rows[1].page_index == 1U);
    MK_REQUIRE(result.recency_rows[1].resident_page_last_used_generation == 7U);
    MK_REQUIRE(result.recency_rows[2].page_index == 2U);
    MK_REQUIRE(result.recency_rows[2].resident_page_last_used_generation == 3U);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
    MK_REQUIRE(mount_set.mounts().size() == 3U);
}

MK_TEST("runtime mavg page streaming generated use generations feed recency eviction order") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "root");
    mount_resident_page(mount_set, 11, mirakana::AssetId::from_name("mavg/page-streaming/page-1"), "visible");
    mount_resident_page(mount_set, 12, mirakana::AssetId::from_name("mavg/page-streaming/page-2"), "cold");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
        {.graph_asset = graph.asset, .page_index = 2, .mount_id = {.value = 12}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = graph.asset, .cluster_index = 1},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingRecencyRow> previous_recency_rows{
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resident_page_last_used_generation = 5},
        {.graph_asset = graph.asset,
         .page_index = 1,
         .mount_id = {.value = 11},
         .resident_page_last_used_generation = 4},
        {.graph_asset = graph.asset,
         .page_index = 2,
         .mount_id = {.value = 12},
         .resident_page_last_used_generation = 3},
    };
    const auto use_generations = mirakana::runtime::infer_runtime_mavg_resident_page_use_generations(
        mount_set, mirakana::runtime::RuntimeMavgResidentPageUseGenerationDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = page_mounts,
                       .previous_recency_rows = previous_recency_rows,
                       .current_use_generation = 7,
                   });
    MK_REQUIRE(use_generations.succeeded());

    const auto evictions = mirakana::runtime::plan_runtime_mavg_page_streaming_automatic_evictions(
        mount_set,
        mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPlanDesc{
            .graph_asset = graph.asset,
            .graph = &graph,
            .resident_page_mounts = page_mounts,
            .policy_kind =
                mirakana::runtime::RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency,
            .recency_rows = use_generations.recency_rows,
            .target_budget =
                mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                    .max_resident_content_bytes = 4,
                },
        });

    MK_REQUIRE(evictions.succeeded());
    MK_REQUIRE(evictions.applied_caller_supplied_recency_policy);
    MK_REQUIRE(!evictions.inferred_eviction_policy);
    MK_REQUIRE(evictions.eviction_candidate_unmount_order.size() == 3U);
    MK_REQUIRE(evictions.eviction_candidate_unmount_order[0] ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12});
    MK_REQUIRE(evictions.eviction_candidate_unmount_order[1] ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 10});
    MK_REQUIRE(evictions.eviction_candidate_unmount_order[2] ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11});
}

MK_TEST("runtime mavg page streaming use generation inference drops nonresident rows and initializes cold pages") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "root");
    mount_resident_page(mount_set, 11, mirakana::AssetId::from_name("mavg/page-streaming/page-1"), "new");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
        {.graph_asset = graph.asset, .page_index = 1, .mount_id = {.value = 11}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingRecencyRow> previous_recency_rows{
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resident_page_last_used_generation = 5},
        {.graph_asset = graph.asset,
         .page_index = 2,
         .mount_id = {.value = 12},
         .resident_page_last_used_generation = 9},
    };

    const auto result = mirakana::runtime::infer_runtime_mavg_resident_page_use_generations(
        mount_set, mirakana::runtime::RuntimeMavgResidentPageUseGenerationDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .resident_page_mounts = page_mounts,
                       .previous_recency_rows = previous_recency_rows,
                       .current_use_generation = 6,
                   });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.recency_rows.size() == 2U);
    MK_REQUIRE(result.recency_rows[0].page_index == 0U);
    MK_REQUIRE(result.recency_rows[0].resident_page_last_used_generation == 5U);
    MK_REQUIRE(result.recency_rows[1].page_index == 1U);
    MK_REQUIRE(result.recency_rows[1].resident_page_last_used_generation == 0U);
    MK_REQUIRE(result.carried_recency_row_count == 1U);
    MK_REQUIRE(result.new_resident_page_count == 1U);
    MK_REQUIRE(result.dropped_nonresident_recency_row_count == 1U);
    MK_REQUIRE(result.output_recency_row_count == 2U);
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg page streaming use generation inference rejects duplicate previous rows") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "root");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingRecencyRow> previous_recency_rows{
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resident_page_last_used_generation = 5},
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resident_page_last_used_generation = 6},
    };

    const auto result = mirakana::runtime::infer_runtime_mavg_resident_page_use_generations(
        mount_set, mirakana::runtime::RuntimeMavgResidentPageUseGenerationDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .resident_page_mounts = page_mounts,
                       .previous_recency_rows = previous_recency_rows,
                       .current_use_generation = 7,
                   });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.recency_rows.empty());
    MK_REQUIRE(!result.inferred_resident_page_use_generation);
    MK_REQUIRE(result.duplicate_recency_row_count == 1U);
    MK_REQUIRE(
        has_diagnostic(result, mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::duplicate_recency_row));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

MK_TEST("runtime mavg page streaming use generation inference rejects nonmonotonic generations") {
    const auto graph = make_page_streaming_graph();
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_resident_page(mount_set, 10, mirakana::AssetId::from_name("mavg/page-streaming/page-0"), "root");

    const std::vector<mirakana::runtime::RuntimeMavgResidentPageMountRow> page_mounts{
        {.graph_asset = graph.asset, .page_index = 0, .mount_id = {.value = 10}},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters{
        {.graph_asset = graph.asset, .cluster_index = 0},
    };
    const std::vector<mirakana::runtime::RuntimeMavgPageStreamingRecencyRow> previous_recency_rows{
        {.graph_asset = graph.asset,
         .page_index = 0,
         .mount_id = {.value = 10},
         .resident_page_last_used_generation = 9},
    };

    const auto result = mirakana::runtime::infer_runtime_mavg_resident_page_use_generations(
        mount_set, mirakana::runtime::RuntimeMavgResidentPageUseGenerationDesc{
                       .graph_asset = graph.asset,
                       .graph = &graph,
                       .selected_clusters = selected_clusters,
                       .resident_page_mounts = page_mounts,
                       .previous_recency_rows = previous_recency_rows,
                       .current_use_generation = 7,
                   });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.recency_rows.empty());
    MK_REQUIRE(!result.inferred_resident_page_use_generation);
    MK_REQUIRE(has_diagnostic(result,
                              mirakana::runtime::RuntimeMavgPageStreamingDiagnosticCode::non_monotonic_use_generation));
    MK_REQUIRE(!result.invoked_file_io);
    MK_REQUIRE(!result.mutated_mount_set);
    MK_REQUIRE(!result.touched_renderer_or_rhi_handles);
}

int main() {
    return mirakana::test::run_all();
}
