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
#include "mirakana/platform/win32/win32_window.hpp"

#if defined(_WIN32)

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

#endif

int main() {
    return mirakana::test::run_all();
}
