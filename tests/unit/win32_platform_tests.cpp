// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/platform/win32/win32_cursor.hpp"
#include "mirakana/platform/win32/win32_event_pump.hpp"
#include "mirakana/platform/win32/win32_runtime.hpp"
#include "mirakana/platform/win32/win32_window.hpp"

#if defined(_WIN32)

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

#endif

int main() {
    return mirakana::test::run_all();
}
