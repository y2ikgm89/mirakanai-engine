// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/platform/win32/win32_clipboard.hpp"
#include "mirakana/platform/win32/win32_cpu_sets.hpp"
#include "mirakana/platform/win32/win32_cursor.hpp"
#include "mirakana/platform/win32/win32_event_pump.hpp"
#include "mirakana/platform/win32/win32_file_dialog.hpp"
#include "mirakana/platform/win32/win32_input.hpp"
#include "mirakana/platform/win32/win32_runtime.hpp"
#include "mirakana/platform/win32/win32_text_input.hpp"
#include "mirakana/platform/win32/win32_ui_accessibility.hpp"
#include "mirakana/platform/win32/win32_window.hpp"

#if defined(_WIN32)

namespace {

[[nodiscard]] bool has_tsf_diagnostic(const std::vector<mirakana::win32::Win32TsfTextSessionDiagnostic>& diagnostics,
                                      mirakana::win32::Win32TsfTextSessionDiagnosticCode code) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool
has_uia_diagnostic(const std::vector<mirakana::win32::Win32UiaProviderPublicationDiagnostic>& diagnostics,
                   mirakana::win32::Win32UiaProviderPublicationDiagnosticCode code) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] mirakana::win32::Win32TsfTextSessionDesc make_valid_tsf_text_session_desc() {
    const mirakana::ui::ElementId target{.value = "win32-tsf-target"};
    return mirakana::win32::Win32TsfTextSessionDesc{
        .active_request =
            mirakana::ui::PlatformTextInputRequest{
                .target = target,
                .text_bounds = mirakana::ui::Rect{.x = 8.0F, .y = 12.0F, .width = 240.0F, .height = 32.0F},
                .surrounding_text = "kana",
                .cursor_byte_offset = 4U,
            },
        .composition_begin = true,
        .composition_text = "ka",
        .composition_cursor_byte_offset = 2U,
        .composition_end = true,
        .committed_text_rows = {mirakana::ui::CommittedTextInput{.target = target, .text = "\xE3\x81\x8B"}},
        .candidate_intent_rows = {mirakana::win32::Win32TsfCandidateIntentRow{
            .target = target,
            .candidate_count = 3U,
            .selected_index = 1U,
            .native_candidate_ui_requested = true,
        }},
        .row_budget = 16U,
    };
}

[[nodiscard]] mirakana::win32::Win32UiaProviderPublicationDesc make_valid_uia_publication_desc() {
    const mirakana::ui::ElementId root{.value = "runtime-ui.root"};
    const mirakana::ui::ElementId button{.value = "runtime-ui.pause"};
    return mirakana::win32::Win32UiaProviderPublicationDesc{
        .provider_root_id = root,
        .nodes =
            {
                mirakana::win32::Win32UiaRuntimeNodeRow{
                    .runtime_id = root,
                    .child_runtime_ids = {button},
                    .role = mirakana::ui::SemanticRole::panel,
                    .name = "HUD root",
                    .description = "Runtime HUD root",
                    .screen_bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 640.0F, .height = 360.0F},
                    .reading_order = 0U,
                    .live_region_status = "off",
                },
                mirakana::win32::Win32UiaRuntimeNodeRow{
                    .runtime_id = button,
                    .parent_runtime_id = root,
                    .role = mirakana::ui::SemanticRole::button,
                    .name = "Pause",
                    .description = "Open pause menu",
                    .screen_bounds = mirakana::ui::Rect{.x = 16.0F, .y = 16.0F, .width = 96.0F, .height = 32.0F},
                    .focusable = true,
                    .focused = true,
                    .action_ids = {"invoke"},
                    .invoke_pattern_supported = true,
                    .reading_order = 1U,
                    .keyboard_shortcut = "Esc",
                    .event_publication_requested = true,
                },
            },
        .publish_events = true,
        .row_budget = 16U,
    };
}

} // namespace

MK_TEST("win32 CPU set worker placement filters unavailable rows and honors core preference") {
    const std::vector<mirakana::win32::Win32CpuSetRow> rows = {
        mirakana::win32::Win32CpuSetRow{.id = 10,
                                        .group = 0,
                                        .logical_processor_index = 0,
                                        .core_index = 0,
                                        .numa_node_index = 0,
                                        .efficiency_class = 9,
                                        .parked = true},
        mirakana::win32::Win32CpuSetRow{.id = 11,
                                        .group = 0,
                                        .logical_processor_index = 1,
                                        .core_index = 1,
                                        .numa_node_index = 0,
                                        .efficiency_class = 7,
                                        .allocated = true,
                                        .allocated_to_target_process = false},
        mirakana::win32::Win32CpuSetRow{.id = 30,
                                        .group = 0,
                                        .logical_processor_index = 2,
                                        .core_index = 2,
                                        .numa_node_index = 0,
                                        .efficiency_class = 8},
        mirakana::win32::Win32CpuSetRow{.id = 31,
                                        .group = 0,
                                        .logical_processor_index = 3,
                                        .core_index = 3,
                                        .numa_node_index = 0,
                                        .efficiency_class = 2},
    };

    const auto performance =
        mirakana::win32::select_win32_cpu_set_worker_placement(mirakana::win32::Win32CpuSetWorkerPlacementDesc{
            .cpu_sets = rows,
            .worker_count = 3,
            .mode = mirakana::JobExecutionPlacementPolicyMode::prefer_performance_cores,
        });

    MK_REQUIRE(performance.status == mirakana::win32::Win32CpuSetWorkerPlacementStatus::ready);
    MK_REQUIRE(performance.worker_rows.size() == 3U);
    MK_REQUIRE(performance.worker_rows[0].cpu_set_id == 30U);
    MK_REQUIRE(performance.worker_rows[1].cpu_set_id == 31U);
    MK_REQUIRE(performance.worker_rows[2].cpu_set_id == 30U);
    MK_REQUIRE(performance.selected_cpu_set_count == 2U);
    MK_REQUIRE(performance.diagnostics.empty());

    const auto efficiency =
        mirakana::win32::select_win32_cpu_set_worker_placement(mirakana::win32::Win32CpuSetWorkerPlacementDesc{
            .cpu_sets = rows,
            .worker_count = 2,
            .mode = mirakana::JobExecutionPlacementPolicyMode::prefer_efficiency_cores,
        });

    MK_REQUIRE(efficiency.status == mirakana::win32::Win32CpuSetWorkerPlacementStatus::ready);
    MK_REQUIRE(efficiency.worker_rows.size() == 2U);
    MK_REQUIRE(efficiency.worker_rows[0].cpu_set_id == 31U);
    MK_REQUIRE(efficiency.worker_rows[1].cpu_set_id == 30U);
}

MK_TEST("win32 CPU set worker placement avoids SMT siblings before reusing a core") {
    const std::vector<mirakana::win32::Win32CpuSetRow> rows = {
        mirakana::win32::Win32CpuSetRow{.id = 100,
                                        .group = 0,
                                        .logical_processor_index = 0,
                                        .core_index = 2,
                                        .numa_node_index = 0,
                                        .efficiency_class = 8},
        mirakana::win32::Win32CpuSetRow{.id = 101,
                                        .group = 0,
                                        .logical_processor_index = 1,
                                        .core_index = 2,
                                        .numa_node_index = 0,
                                        .efficiency_class = 8},
        mirakana::win32::Win32CpuSetRow{.id = 200,
                                        .group = 0,
                                        .logical_processor_index = 2,
                                        .core_index = 3,
                                        .numa_node_index = 0,
                                        .efficiency_class = 8},
        mirakana::win32::Win32CpuSetRow{.id = 300,
                                        .group = 0,
                                        .logical_processor_index = 3,
                                        .core_index = 4,
                                        .numa_node_index = 0,
                                        .efficiency_class = 2},
        mirakana::win32::Win32CpuSetRow{.id = 400,
                                        .group = 0,
                                        .logical_processor_index = 4,
                                        .core_index = 5,
                                        .numa_node_index = 0,
                                        .efficiency_class = 9,
                                        .allocated = true,
                                        .allocated_to_target_process = false},
    };

    const auto placement =
        mirakana::win32::select_win32_cpu_set_worker_placement(mirakana::win32::Win32CpuSetWorkerPlacementDesc{
            .cpu_sets = rows,
            .worker_count = 4,
            .mode = mirakana::JobExecutionPlacementPolicyMode::avoid_smt_siblings,
        });

    MK_REQUIRE(placement.status == mirakana::win32::Win32CpuSetWorkerPlacementStatus::ready);
    MK_REQUIRE(placement.selected_cpu_set_count == 4U);
    MK_REQUIRE(placement.distinct_core_count == 3U);
    MK_REQUIRE(placement.smt_sibling_cpu_set_count == 1U);
    MK_REQUIRE(placement.smt_sibling_topology_known);
    MK_REQUIRE(placement.smt_sibling_policy_applied);
    MK_REQUIRE(placement.worker_rows.size() == 4U);
    MK_REQUIRE(placement.worker_rows[0].cpu_set_id == 100U);
    MK_REQUIRE(placement.worker_rows[1].cpu_set_id == 200U);
    MK_REQUIRE(placement.worker_rows[2].cpu_set_id == 300U);
    MK_REQUIRE(placement.worker_rows[3].cpu_set_id == 101U);
    MK_REQUIRE(!placement.worker_rows[0].smt_sibling_deferred);
    MK_REQUIRE(!placement.worker_rows[1].smt_sibling_deferred);
    MK_REQUIRE(!placement.worker_rows[2].smt_sibling_deferred);
    MK_REQUIRE(placement.worker_rows[3].smt_sibling_deferred);
}

MK_TEST("win32 runtime plans startup and shutdown rows") {
    const auto startup = mirakana::win32::plan_win32_runtime_startup(mirakana::win32::Win32RuntimeDesc{
        .window_class_name = "MIRAIKANAI Test Window",
        .dpi_aware = true,
    });

    MK_REQUIRE(startup.succeeded());
    MK_REQUIRE(startup.register_window_class);
    MK_REQUIRE(startup.initialize_dpi_awareness);
    MK_REQUIRE(startup.window_class_name == "MIRAIKANAI Test Window");

    const auto shutdown = mirakana::win32::plan_win32_runtime_shutdown(startup);
    MK_REQUIRE(shutdown.succeeded());
    MK_REQUIRE(shutdown.unregister_window_class);
}

MK_TEST("win32 runtime rejects invalid window class names") {
    const auto startup = mirakana::win32::plan_win32_runtime_startup(mirakana::win32::Win32RuntimeDesc{
        .window_class_name = "",
        .dpi_aware = true,
    });

    MK_REQUIRE(!startup.succeeded());
    MK_REQUIRE(!startup.diagnostic.empty());
}

MK_TEST("win32 window creation plan validates title and client extent") {
    const auto valid = mirakana::win32::plan_win32_window_creation(mirakana::WindowDesc{
        .title = "Win32 Test Window",
        .extent = mirakana::WindowExtent{.width = 320, .height = 240},
    });
    MK_REQUIRE(valid.succeeded());
    MK_REQUIRE(valid.client_extent.width == 320);
    MK_REQUIRE(valid.client_extent.height == 240);
    MK_REQUIRE(valid.resizable);

    const auto invalid_title = mirakana::win32::plan_win32_window_creation(mirakana::WindowDesc{
        .title = "",
        .extent = mirakana::WindowExtent{.width = 320, .height = 240},
    });
    MK_REQUIRE(!invalid_title.succeeded());

    const auto invalid_extent = mirakana::win32::plan_win32_window_creation(mirakana::WindowDesc{
        .title = "bad",
        .extent = mirakana::WindowExtent{.width = 0, .height = 240},
    });
    MK_REQUIRE(!invalid_extent.succeeded());
}

MK_TEST("win32 copied message rows translate resize focus minimize close and display events") {
    const mirakana::win32::Win32CopiedMessage resize{
        .message = mirakana::win32::Win32MessageId::size,
        .wparam = mirakana::win32::win32_size_restored,
        .low_word = 640,
        .high_word = 480,
        .window_token = 7,
    };
    const auto resized = mirakana::win32::translate_win32_window_message(resize);
    MK_REQUIRE(resized.kind == mirakana::win32::Win32WindowEventKind::resized);
    MK_REQUIRE(resized.extent.width == 640);
    MK_REQUIRE(resized.extent.height == 480);
    MK_REQUIRE(resized.window_token == 7);

    const mirakana::win32::Win32CopiedMessage minimized{
        .message = mirakana::win32::Win32MessageId::size,
        .wparam = mirakana::win32::win32_size_minimized,
        .low_word = 640,
        .high_word = 480,
    };
    MK_REQUIRE(mirakana::win32::translate_win32_window_message(minimized).kind ==
               mirakana::win32::Win32WindowEventKind::minimized);

    const mirakana::win32::Win32CopiedMessage focus{
        .message = mirakana::win32::Win32MessageId::set_focus,
        .window_token = 7,
    };
    MK_REQUIRE(mirakana::win32::translate_win32_window_message(focus).kind ==
               mirakana::win32::Win32WindowEventKind::focus_gained);

    const mirakana::win32::Win32CopiedMessage kill_focus{
        .message = mirakana::win32::Win32MessageId::kill_focus,
    };
    MK_REQUIRE(mirakana::win32::translate_win32_window_message(kill_focus).kind ==
               mirakana::win32::Win32WindowEventKind::focus_lost);

    const mirakana::win32::Win32CopiedMessage close{
        .message = mirakana::win32::Win32MessageId::close,
    };
    MK_REQUIRE(mirakana::win32::translate_win32_window_message(close).kind ==
               mirakana::win32::Win32WindowEventKind::close_requested);

    const mirakana::win32::Win32CopiedMessage display{
        .message = mirakana::win32::Win32MessageId::display_change,
        .low_word = 1920,
        .high_word = 1080,
    };
    const auto display_changed = mirakana::win32::translate_win32_window_message(display);
    MK_REQUIRE(display_changed.kind == mirakana::win32::Win32WindowEventKind::display_changed);
    MK_REQUIRE(display_changed.display_pixel_extent.width == 1920);
    MK_REQUIRE(display_changed.display_pixel_extent.height == 1080);
}

MK_TEST("win32 window applies copied rows to first party window and lifecycle state") {
    mirakana::win32::Win32WindowModel window(mirakana::WindowDesc{
        .title = "Win32 Model",
        .extent = mirakana::WindowExtent{.width = 320, .height = 240},
    });
    mirakana::VirtualLifecycle lifecycle;

    window.handle_event(mirakana::win32::Win32WindowEvent{
        .kind = mirakana::win32::Win32WindowEventKind::resized,
        .extent = mirakana::WindowExtent{.width = 800, .height = 600},
    });
    MK_REQUIRE(window.extent().width == 800);
    MK_REQUIRE(window.extent().height == 600);

    window.handle_event(mirakana::win32::Win32WindowEvent{
        .kind = mirakana::win32::Win32WindowEventKind::focus_lost,
    });
    MK_REQUIRE(!window.focused());

    window.handle_event(mirakana::win32::Win32WindowEvent{
        .kind = mirakana::win32::Win32WindowEventKind::focus_gained,
    });
    MK_REQUIRE(window.focused());

    window.handle_event(mirakana::win32::Win32WindowEvent{
        .kind = mirakana::win32::Win32WindowEventKind::minimized,
    });
    MK_REQUIRE(window.minimized());

    window.handle_event(
        mirakana::win32::Win32WindowEvent{
            .kind = mirakana::win32::Win32WindowEventKind::close_requested,
        },
        &lifecycle);
    MK_REQUIRE(!window.is_open());
    MK_REQUIRE(lifecycle.state().quit_requested);
}

MK_TEST("win32 runtime creates a hidden native window that can resize move and close") {
    mirakana::win32::Win32Runtime runtime(mirakana::win32::Win32RuntimeDesc{
        .window_class_name = "MIRAIKANAI Native Window Test",
        .dpi_aware = true,
    });
    mirakana::win32::Win32Window window(runtime, mirakana::WindowDesc{
                                                     .title = "Native Win32 Window",
                                                     .extent = mirakana::WindowExtent{.width = 320, .height = 240},
                                                     .position = mirakana::WindowPosition{.x = 40, .y = 60},
                                                 });

    MK_REQUIRE(window.native_window_token() != 0);
    MK_REQUIRE(window.title() == "Native Win32 Window");
    MK_REQUIRE(window.extent().width == 320);
    MK_REQUIRE(window.extent().height == 240);
    MK_REQUIRE(window.is_open());

    window.resize(mirakana::WindowExtent{.width = 640, .height = 480});
    window.move(mirakana::WindowPosition{.x = 80, .y = 120});
    MK_REQUIRE(window.extent().width == 640);
    MK_REQUIRE(window.extent().height == 480);
    MK_REQUIRE(window.position().x == 80);
    MK_REQUIRE(window.position().y == 120);
    MK_REQUIRE(mirakana::is_valid_window_display_state(window.display_state()));

    window.request_close();
    MK_REQUIRE(!window.is_open());
}

MK_TEST("win32 event pump translates posted close rows for native windows") {
    mirakana::win32::Win32Runtime runtime(mirakana::win32::Win32RuntimeDesc{
        .window_class_name = "MIRAIKANAI Native Pump Test",
        .dpi_aware = true,
    });
    mirakana::win32::Win32Window window(runtime, mirakana::WindowDesc{
                                                     .title = "Native Win32 Pump",
                                                     .extent = mirakana::WindowExtent{.width = 320, .height = 240},
                                                 });
    mirakana::win32::Win32EventPump pump;

    window.post_close_request();
    const auto events = pump.poll();

    bool saw_close = false;
    for (const auto& event : events) {
        if (event.kind == mirakana::win32::Win32WindowEventKind::close_requested &&
            event.window_token == window.native_window_token()) {
            saw_close = true;
            window.handle_event(event);
        }
    }

    MK_REQUIRE(saw_close);
    MK_REQUIRE(!window.is_open());
}

MK_TEST("win32 cursor mode plan keeps native calls behind first party rows") {
    const auto normal = mirakana::win32::plan_win32_cursor_mode(mirakana::CursorMode::normal);
    MK_REQUIRE(normal.state.mode == mirakana::CursorMode::normal);
    MK_REQUIRE(normal.show_cursor);
    MK_REQUIRE(!normal.clip_to_window);
    MK_REQUIRE(!normal.capture_mouse);

    const auto confined = mirakana::win32::plan_win32_cursor_mode(mirakana::CursorMode::confined);
    MK_REQUIRE(confined.state.mode == mirakana::CursorMode::confined);
    MK_REQUIRE(confined.show_cursor);
    MK_REQUIRE(confined.clip_to_window);
    MK_REQUIRE(confined.capture_mouse);

    const auto relative = mirakana::win32::plan_win32_cursor_mode(mirakana::CursorMode::relative);
    MK_REQUIRE(relative.state.mode == mirakana::CursorMode::relative);
    MK_REQUIRE(!relative.show_cursor);
    MK_REQUIRE(relative.clip_to_window);
    MK_REQUIRE(relative.capture_mouse);
    MK_REQUIRE(relative.request_raw_relative_motion);
}

MK_TEST("win32 input rows translate keyboard mouse and modifiers into first party state") {
    mirakana::VirtualInput input;
    mirakana::VirtualPointerInput pointer_input;

    const auto key_down = mirakana::win32::translate_win32_input_message(mirakana::win32::Win32CopiedInputMessage{
        .message = mirakana::win32::Win32InputMessageId::key_down,
        .virtual_key = mirakana::win32::win32_vk_left,
        .repeated = false,
        .modifiers = mirakana::win32::Win32ModifierState{.control = true},
        .window_token = 11,
    });
    MK_REQUIRE(key_down.kind == mirakana::win32::Win32InputEventKind::key_pressed);
    MK_REQUIRE(key_down.key == mirakana::Key::left);
    MK_REQUIRE(key_down.modifiers.control);
    MK_REQUIRE(key_down.window_token == 11);

    mirakana::win32::apply_win32_input_event(key_down, &input, nullptr);
    MK_REQUIRE(input.key_pressed(mirakana::Key::left));

    const auto repeated = mirakana::win32::translate_win32_input_message(mirakana::win32::Win32CopiedInputMessage{
        .message = mirakana::win32::Win32InputMessageId::key_down,
        .virtual_key = mirakana::win32::win32_vk_space,
        .repeated = true,
    });
    mirakana::win32::apply_win32_input_event(repeated, &input, nullptr);
    MK_REQUIRE(!input.key_pressed(mirakana::Key::space));

    const auto pointer_down = mirakana::win32::translate_win32_input_message(mirakana::win32::Win32CopiedInputMessage{
        .message = mirakana::win32::Win32InputMessageId::left_button_down,
        .low_word = 32,
        .high_word = 48,
    });
    MK_REQUIRE(pointer_down.kind == mirakana::win32::Win32InputEventKind::pointer_pressed);
    MK_REQUIRE(pointer_down.pointer.id == mirakana::primary_pointer_id);
    MK_REQUIRE(pointer_down.pointer.kind == mirakana::PointerKind::mouse);

    mirakana::win32::apply_win32_input_event(pointer_down, nullptr, &pointer_input);
    MK_REQUIRE(pointer_input.pointer_pressed(mirakana::primary_pointer_id));
    MK_REQUIRE(pointer_input.pointer_position(mirakana::primary_pointer_id).x == 32.0F);
    MK_REQUIRE(pointer_input.pointer_position(mirakana::primary_pointer_id).y == 48.0F);
}

MK_TEST("win32 raw input registration plan remains keyboard mouse scoped") {
    const auto plan = mirakana::win32::plan_win32_raw_input_registration(mirakana::win32::Win32RawInputRequest{
        .keyboard = true,
        .mouse = true,
        .relative_mouse = true,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.register_keyboard);
    MK_REQUIRE(plan.register_mouse);
    MK_REQUIRE(plan.capture_relative_mouse);
    MK_REQUIRE(!plan.register_hid_controllers);
}

MK_TEST("win32 clipboard text plans unicode clipboard rows") {
    const auto write = mirakana::win32::plan_win32_clipboard_write("hello clipboard");
    MK_REQUIRE(write.succeeded());
    MK_REQUIRE(write.open_clipboard);
    MK_REQUIRE(write.empty_clipboard);
    MK_REQUIRE(write.set_unicode_text);
    MK_REQUIRE(write.utf16_text.size() == 15);

    const auto clear = mirakana::win32::plan_win32_clipboard_write("");
    MK_REQUIRE(clear.succeeded());
    MK_REQUIRE(clear.empty_clipboard);
    MK_REQUIRE(!clear.set_unicode_text);

    const auto read = mirakana::win32::plan_win32_clipboard_read();
    MK_REQUIRE(read.open_clipboard);
    MK_REQUIRE(read.request_unicode_text);
}

MK_TEST("win32 file dialog request plan converts filters and options") {
    const auto plan = mirakana::win32::plan_win32_file_dialog_request(mirakana::FileDialogRequest{
        .kind = mirakana::FileDialogKind::open_file,
        .title = "Open Asset",
        .filters = {mirakana::FileDialogFilter{.name = "Images", .pattern = "png;jpg"}},
        .default_location = "assets",
        .allow_many = true,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.use_open_dialog);
    MK_REQUIRE(!plan.use_save_dialog);
    MK_REQUIRE(plan.force_filesystem);
    MK_REQUIRE(plan.allow_many);
    MK_REQUIRE(plan.filters.size() == 1);
    MK_REQUIRE(plan.filters[0].display_name == "Images");
    MK_REQUIRE(plan.filters[0].spec == "*.png;*.jpg");
}

MK_TEST("win32 text input rows produce committed UTF-8 and session plans") {
    const mirakana::ui::ElementId target{.value = "win32-text-target"};
    const auto committed = mirakana::win32::win32_committed_text_from_message(
        target, mirakana::win32::Win32CopiedTextInputMessage{
                    .message = mirakana::win32::Win32TextInputMessageId::char_input,
                    .utf16_text = std::u16string{static_cast<char16_t>(0xD83D), static_cast<char16_t>(0xDE00)},
                });

    MK_REQUIRE(committed.has_value());
    MK_REQUIRE(committed->target.value == target.value);
    MK_REQUIRE(committed->text == "\xF0\x9F\x98\x80");

    const auto begin = mirakana::win32::plan_win32_text_input_begin(mirakana::ui::PlatformTextInputRequest{
        .target = target,
        .text_bounds = mirakana::ui::Rect{.x = 1.0F, .y = 2.0F, .width = 120.0F, .height = 24.0F},
    });
    MK_REQUIRE(begin.succeeded());
    MK_REQUIRE(begin.start_session);
    MK_REQUIRE(begin.update_composition_window);

    const auto end = mirakana::win32::plan_win32_text_input_end(target);
    MK_REQUIRE(end.succeeded());
    MK_REQUIRE(end.end_session);

    const auto edit_command = mirakana::win32::win32_text_edit_command_from_input_event(
        target, mirakana::win32::Win32InputEvent{
                    .kind = mirakana::win32::Win32InputEventKind::key_pressed,
                    .key = mirakana::Key::left,
                });
    MK_REQUIRE(edit_command.has_value());
    MK_REQUIRE(edit_command->kind == mirakana::ui::TextEditCommandKind::move_cursor_backward);

    const auto clipboard_command = mirakana::win32::win32_text_edit_clipboard_command_from_input_event(
        target, mirakana::win32::Win32InputEvent{
                    .kind = mirakana::win32::Win32InputEventKind::key_pressed,
                    .virtual_key = mirakana::win32::win32_vk_v,
                    .modifiers = mirakana::win32::Win32ModifierState{.control = true},
                });
    MK_REQUIRE(clipboard_command.has_value());
    MK_REQUIRE(clipboard_command->kind == mirakana::ui::TextEditClipboardCommandKind::paste_text);
}

MK_TEST("win32 TSF text session proof emits private official SDK evidence rows") {
    const auto result = mirakana::win32::plan_win32_tsf_text_session(make_valid_tsf_text_session_desc());

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.tsf_thread_manager_available);
    MK_REQUIRE(result.tsf_document_manager_available);
    MK_REQUIRE(result.tsf_context_available);
    MK_REQUIRE(result.focus_sink_rows == 1U);
    MK_REQUIRE(result.text_store_lock_rows == 1U);
    MK_REQUIRE(result.text_area_rows.size() == 1U);
    MK_REQUIRE(result.text_area_rows.front().target.value == "win32-tsf-target");
    MK_REQUIRE(result.text_area_rows.front().caret_rect.width == 240.0F);
    MK_REQUIRE(result.composition_rows.size() == 3U);
    MK_REQUIRE(result.composition_rows[0].begin);
    MK_REQUIRE(result.composition_rows[1].update);
    MK_REQUIRE(result.composition_rows[1].composition_text == "ka");
    MK_REQUIRE(result.composition_rows[2].end);
    MK_REQUIRE(result.committed_text_rows.size() == 1U);
    MK_REQUIRE(result.committed_text_rows.front().target.value == "win32-tsf-target");
    MK_REQUIRE(result.committed_text_rows.front().text == "\xE3\x81\x8B");
    MK_REQUIRE(result.candidate_intent_rows.size() == 1U);
    MK_REQUIRE(result.candidate_intent_rows.front().candidate_count == 3U);
    MK_REQUIRE(!result.candidate_intent_rows.front().native_candidate_ui_ready);
    MK_REQUIRE(!result.native_candidate_ui_ready);
    MK_REQUIRE(!result.cross_platform_ime_ready);
    MK_REQUIRE(!result.public_native_handles_exposed);

    const auto evidence = mirakana::win32::make_win32_tsf_native_ime_production_evidence(result);
    MK_REQUIRE(evidence.id == "runtime-ui-platform.native-ime.win32.tsf");
    MK_REQUIRE(evidence.feature == mirakana::ui::RuntimeUiPlatformProductionFeature::native_ime_session);
    MK_REQUIRE(evidence.proof == mirakana::ui::RuntimeUiPlatformProductionProofKind::official_sdk_adapter);
    MK_REQUIRE(evidence.selected);
    MK_REQUIRE(evidence.ready);
    MK_REQUIRE(evidence.dependency_recorded);
    MK_REQUIRE(evidence.host_evidence_available);
    MK_REQUIRE(!evidence.public_native_handles);
    MK_REQUIRE(!evidence.external_engine_parity_claim);
}

MK_TEST("win32 TSF text session proof fails closed for invalid rows and parity claims") {
    auto missing_target = make_valid_tsf_text_session_desc();
    missing_target.active_request.target.value.clear();
    const auto missing_target_result = mirakana::win32::plan_win32_tsf_text_session(missing_target);
    MK_REQUIRE(!missing_target_result.succeeded());
    MK_REQUIRE(has_tsf_diagnostic(missing_target_result.diagnostics,
                                  mirakana::win32::Win32TsfTextSessionDiagnosticCode::missing_active_target));

    auto invalid_rect = make_valid_tsf_text_session_desc();
    invalid_rect.active_request.text_bounds.width = 0.0F;
    const auto invalid_rect_result = mirakana::win32::plan_win32_tsf_text_session(invalid_rect);
    MK_REQUIRE(!invalid_rect_result.succeeded());
    MK_REQUIRE(has_tsf_diagnostic(invalid_rect_result.diagnostics,
                                  mirakana::win32::Win32TsfTextSessionDiagnosticCode::invalid_caret_rect));

    auto invalid_utf8 = make_valid_tsf_text_session_desc();
    invalid_utf8.active_request.surrounding_text = std::string{"\xC3\x28", 2U};
    const auto invalid_utf8_result = mirakana::win32::plan_win32_tsf_text_session(invalid_utf8);
    MK_REQUIRE(!invalid_utf8_result.succeeded());
    MK_REQUIRE(has_tsf_diagnostic(invalid_utf8_result.diagnostics,
                                  mirakana::win32::Win32TsfTextSessionDiagnosticCode::invalid_surrounding_text));

    auto composition_without_begin = make_valid_tsf_text_session_desc();
    composition_without_begin.composition_begin = false;
    const auto composition_without_begin_result =
        mirakana::win32::plan_win32_tsf_text_session(composition_without_begin);
    MK_REQUIRE(!composition_without_begin_result.succeeded());
    MK_REQUIRE(
        has_tsf_diagnostic(composition_without_begin_result.diagnostics,
                           mirakana::win32::Win32TsfTextSessionDiagnosticCode::composition_update_without_begin));

    auto committed_mismatch = make_valid_tsf_text_session_desc();
    committed_mismatch.committed_text_rows.front().target.value = "other-target";
    const auto committed_mismatch_result = mirakana::win32::plan_win32_tsf_text_session(committed_mismatch);
    MK_REQUIRE(!committed_mismatch_result.succeeded());
    MK_REQUIRE(has_tsf_diagnostic(committed_mismatch_result.diagnostics,
                                  mirakana::win32::Win32TsfTextSessionDiagnosticCode::committed_text_target_mismatch));

    auto candidates_without_session = make_valid_tsf_text_session_desc();
    candidates_without_session.request_tsf_session = false;
    const auto candidates_without_session_result =
        mirakana::win32::plan_win32_tsf_text_session(candidates_without_session);
    MK_REQUIRE(!candidates_without_session_result.succeeded());
    MK_REQUIRE(has_tsf_diagnostic(candidates_without_session_result.diagnostics,
                                  mirakana::win32::Win32TsfTextSessionDiagnosticCode::candidate_rows_without_session));

    auto native_handles = make_valid_tsf_text_session_desc();
    native_handles.public_native_handles_exposed = true;
    const auto native_handles_result = mirakana::win32::plan_win32_tsf_text_session(native_handles);
    MK_REQUIRE(!native_handles_result.succeeded());
    MK_REQUIRE(has_tsf_diagnostic(native_handles_result.diagnostics,
                                  mirakana::win32::Win32TsfTextSessionDiagnosticCode::public_native_handles_exposed));

    auto broad_parity = make_valid_tsf_text_session_desc();
    broad_parity.claims_cross_platform_ime_ready = true;
    const auto broad_parity_result = mirakana::win32::plan_win32_tsf_text_session(broad_parity);
    MK_REQUIRE(!broad_parity_result.succeeded());
    MK_REQUIRE(has_tsf_diagnostic(broad_parity_result.diagnostics,
                                  mirakana::win32::Win32TsfTextSessionDiagnosticCode::broad_ime_parity_claim));
}

MK_TEST("win32 UIA provider publication emits private official SDK accessibility rows") {
    const auto result = mirakana::win32::publish_runtime_ui_to_win32_uia(make_valid_uia_publication_desc());

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.uia_provider_root_available);
    MK_REQUIRE(result.provider_root_id.value == "runtime-ui.root");
    MK_REQUIRE(result.node_rows.size() == 2U);
    MK_REQUIRE(result.role_rows == 2U);
    MK_REQUIRE(result.name_rows == 2U);
    MK_REQUIRE(result.description_rows == 2U);
    MK_REQUIRE(result.state_rows == 2U);
    MK_REQUIRE(result.focus_rows == 1U);
    MK_REQUIRE(result.action_rows == 1U);
    MK_REQUIRE(result.relationship_rows == 1U);
    MK_REQUIRE(result.reading_order_rows == 2U);
    MK_REQUIRE(result.live_region_rows == 1U);
    MK_REQUIRE(result.keyboard_pattern_rows == 1U);
    MK_REQUIRE(result.bounds_rows == 2U);
    MK_REQUIRE(result.event_publication_rows == 1U);
    MK_REQUIRE(!result.cross_platform_accessibility_ready);
    MK_REQUIRE(!result.public_native_handles_exposed);

    const auto evidence = mirakana::win32::make_win32_uia_accessibility_publication_production_evidence(result);
    MK_REQUIRE(evidence.id == "runtime-ui-platform.accessibility.win32.uia");
    MK_REQUIRE(evidence.feature == mirakana::ui::RuntimeUiPlatformProductionFeature::os_accessibility_publication);
    MK_REQUIRE(evidence.proof == mirakana::ui::RuntimeUiPlatformProductionProofKind::official_sdk_adapter);
    MK_REQUIRE(evidence.selected);
    MK_REQUIRE(evidence.ready);
    MK_REQUIRE(evidence.dependency_recorded);
    MK_REQUIRE(evidence.host_evidence_available);
    MK_REQUIRE(!evidence.public_native_handles);
    MK_REQUIRE(!evidence.external_engine_parity_claim);
}

MK_TEST("win32 UIA provider publication fails closed for invalid rows and public handle claims") {
    auto missing_name = make_valid_uia_publication_desc();
    missing_name.nodes[1].name.clear();
    const auto missing_name_result = mirakana::win32::publish_runtime_ui_to_win32_uia(missing_name);
    MK_REQUIRE(!missing_name_result.succeeded());
    MK_REQUIRE(has_uia_diagnostic(missing_name_result.diagnostics,
                                  mirakana::win32::Win32UiaProviderPublicationDiagnosticCode::missing_accessible_name));

    auto duplicate_id = make_valid_uia_publication_desc();
    duplicate_id.nodes[1].runtime_id = duplicate_id.provider_root_id;
    const auto duplicate_id_result = mirakana::win32::publish_runtime_ui_to_win32_uia(duplicate_id);
    MK_REQUIRE(!duplicate_id_result.succeeded());
    MK_REQUIRE(has_uia_diagnostic(duplicate_id_result.diagnostics,
                                  mirakana::win32::Win32UiaProviderPublicationDiagnosticCode::duplicate_runtime_id));

    auto invalid_bounds = make_valid_uia_publication_desc();
    invalid_bounds.nodes[1].screen_bounds.width = 0.0F;
    const auto invalid_bounds_result = mirakana::win32::publish_runtime_ui_to_win32_uia(invalid_bounds);
    MK_REQUIRE(!invalid_bounds_result.succeeded());
    MK_REQUIRE(has_uia_diagnostic(invalid_bounds_result.diagnostics,
                                  mirakana::win32::Win32UiaProviderPublicationDiagnosticCode::invalid_bounds));

    auto focus_without_action = make_valid_uia_publication_desc();
    focus_without_action.nodes[1].invoke_pattern_supported = false;
    focus_without_action.nodes[1].action_ids.clear();
    const auto focus_without_action_result = mirakana::win32::publish_runtime_ui_to_win32_uia(focus_without_action);
    MK_REQUIRE(!focus_without_action_result.succeeded());
    MK_REQUIRE(has_uia_diagnostic(
        focus_without_action_result.diagnostics,
        mirakana::win32::Win32UiaProviderPublicationDiagnosticCode::focusable_without_action_pattern));

    auto child_without_parent = make_valid_uia_publication_desc();
    child_without_parent.nodes[1].parent_runtime_id.value = "missing-parent";
    const auto child_without_parent_result = mirakana::win32::publish_runtime_ui_to_win32_uia(child_without_parent);
    MK_REQUIRE(!child_without_parent_result.succeeded());
    MK_REQUIRE(has_uia_diagnostic(child_without_parent_result.diagnostics,
                                  mirakana::win32::Win32UiaProviderPublicationDiagnosticCode::child_without_parent));

    auto unsupported_pattern = make_valid_uia_publication_desc();
    unsupported_pattern.nodes[1].unsupported_pattern_claim = true;
    const auto unsupported_pattern_result = mirakana::win32::publish_runtime_ui_to_win32_uia(unsupported_pattern);
    MK_REQUIRE(!unsupported_pattern_result.succeeded());
    MK_REQUIRE(
        has_uia_diagnostic(unsupported_pattern_result.diagnostics,
                           mirakana::win32::Win32UiaProviderPublicationDiagnosticCode::unsupported_pattern_claim));

    auto event_without_root = make_valid_uia_publication_desc();
    event_without_root.provider_root_id.value.clear();
    const auto event_without_root_result = mirakana::win32::publish_runtime_ui_to_win32_uia(event_without_root);
    MK_REQUIRE(!event_without_root_result.succeeded());
    MK_REQUIRE(has_uia_diagnostic(
        event_without_root_result.diagnostics,
        mirakana::win32::Win32UiaProviderPublicationDiagnosticCode::event_claim_without_provider_root));

    auto native_handles = make_valid_uia_publication_desc();
    native_handles.public_native_handles_exposed = true;
    const auto native_handles_result = mirakana::win32::publish_runtime_ui_to_win32_uia(native_handles);
    MK_REQUIRE(!native_handles_result.succeeded());
    MK_REQUIRE(
        has_uia_diagnostic(native_handles_result.diagnostics,
                           mirakana::win32::Win32UiaProviderPublicationDiagnosticCode::public_native_handles_exposed));

    auto broad_accessibility = make_valid_uia_publication_desc();
    broad_accessibility.claims_cross_platform_accessibility_ready = true;
    const auto broad_accessibility_result = mirakana::win32::publish_runtime_ui_to_win32_uia(broad_accessibility);
    MK_REQUIRE(!broad_accessibility_result.succeeded());
    MK_REQUIRE(has_uia_diagnostic(
        broad_accessibility_result.diagnostics,
        mirakana::win32::Win32UiaProviderPublicationDiagnosticCode::broad_accessibility_parity_claim));
}

#endif

int main() {
    return mirakana::test::run_all();
}
