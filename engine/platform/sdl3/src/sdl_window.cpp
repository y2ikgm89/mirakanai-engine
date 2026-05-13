// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/sdl3/sdl_window.hpp"

#include "mirakana/platform/sdl3/sdl_input.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_touch.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] std::runtime_error sdl_error(const char* operation) {
    return std::runtime_error(std::string(operation) + " failed: " + SDL_GetError());
}

void validate_window_desc(const WindowDesc& desc) {
    if (desc.title.empty()) {
        throw std::invalid_argument("window title must not be empty");
    }
    if (desc.extent.width == 0 || desc.extent.height == 0) {
        throw std::invalid_argument("window extent must be non-zero");
    }
}

[[nodiscard]] DisplayRect display_rect_from_sdl(const SDL_Rect& rect) noexcept {
    return DisplayRect{
        .x = rect.x,
        .y = rect.y,
        .width = rect.w > 0 ? static_cast<std::uint32_t>(rect.w) : 0U,
        .height = rect.h > 0 ? static_cast<std::uint32_t>(rect.h) : 0U,
    };
}

[[nodiscard]] DisplayRect window_extent_rect(WindowExtent extent) noexcept {
    return DisplayRect{.x = 0, .y = 0, .width = extent.width, .height = extent.height};
}

[[nodiscard]] float positive_or_default(float value, float fallback) noexcept {
    return value > 0.0F ? value : fallback;
}

[[nodiscard]] std::string copy_sdl_text(const char* text) {
    return text != nullptr ? std::string{text} : std::string{};
}

[[nodiscard]] std::vector<std::string> copy_sdl_text_candidates(const char* const* candidates,
                                                                std::int32_t candidate_count) {
    std::vector<std::string> copied;
    if (candidates == nullptr || candidate_count <= 0) {
        return copied;
    }

    const std::span<const char* const> candidate_rows{candidates, static_cast<std::size_t>(candidate_count)};
    copied.reserve(candidate_rows.size());
    for (const char* candidate : candidate_rows) {
        copied.push_back(copy_sdl_text(candidate));
    }

    return copied;
}

[[nodiscard]] bool is_touch_generated_mouse(std::uint32_t which) noexcept {
    return which == SDL_TOUCH_MOUSEID;
}

} // namespace

struct SdlWindow::Impl {
    explicit Impl(SDL_Window* native_window) noexcept : window(native_window) {}

    ~Impl() {
        if (window != nullptr) {
            SDL_DestroyWindow(window);
        }
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    SDL_Window* window{nullptr};
};

SdlRuntime::SdlRuntime(SdlRuntimeDesc desc) {
    if (!desc.video_driver_hint.empty()) {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, desc.video_driver_hint.c_str());
    }

    flags_ = SDL_INIT_VIDEO;
    if (desc.enable_gamepad) {
        flags_ |= SDL_INIT_GAMEPAD;
    }

    if (!SDL_InitSubSystem(flags_)) {
        throw sdl_error("SDL_InitSubSystem");
    }
}

SdlRuntime::~SdlRuntime() {
    SDL_QuitSubSystem(flags_);
}

std::vector<DisplayInfo> sdl3_displays() {
    int count = 0;
    SDL_DisplayID* ids = SDL_GetDisplays(&count);
    if (ids == nullptr || count <= 0) {
        throw sdl_error("SDL_GetDisplays");
    }

    const auto primary = SDL_GetPrimaryDisplay();
    std::vector<DisplayInfo> displays;
    displays.reserve(static_cast<std::size_t>(count));

    for (int index = 0; index < count; ++index) {
        SDL_Rect bounds{};
        if (!SDL_GetDisplayBounds(ids[index], &bounds)) {
            SDL_free(ids);
            throw sdl_error("SDL_GetDisplayBounds");
        }

        SDL_Rect usable_bounds{};
        if (!SDL_GetDisplayUsableBounds(ids[index], &usable_bounds)) {
            usable_bounds = bounds;
        }

        const char* name = SDL_GetDisplayName(ids[index]);
        displays.push_back(DisplayInfo{
            .id = ids[index],
            .name = name != nullptr ? std::string{name} : std::string{},
            .bounds = display_rect_from_sdl(bounds),
            .usable_bounds = display_rect_from_sdl(usable_bounds),
            .content_scale = positive_or_default(SDL_GetDisplayContentScale(ids[index]), 1.0F),
            .primary = ids[index] == primary,
        });
    }

    SDL_free(ids);
    std::ranges::sort(displays, [](const DisplayInfo& lhs, const DisplayInfo& rhs) { return lhs.id < rhs.id; });
    return displays;
}

std::optional<DisplayInfo> sdl3_select_display(DisplaySelectionRequest request) {
    return select_display(sdl3_displays(), request);
}

SdlWindowEvent sdl3_translate_window_event(SdlRawEventHandle event, WindowExtent window_extent) {
    if (event.value == nullptr) {
        return {};
    }

    const auto& source = *static_cast<const SDL_Event*>(event.value);
    SdlWindowEvent translated{};
    switch (source.type) {
    case SDL_EVENT_QUIT:
        translated.kind = SdlWindowEventKind::quit_requested;
        return translated;
    case SDL_EVENT_TERMINATING:
        translated.kind = SdlWindowEventKind::terminating;
        return translated;
    case SDL_EVENT_LOW_MEMORY:
        translated.kind = SdlWindowEventKind::low_memory;
        return translated;
    case SDL_EVENT_WILL_ENTER_BACKGROUND:
        translated.kind = SdlWindowEventKind::will_enter_background;
        return translated;
    case SDL_EVENT_DID_ENTER_BACKGROUND:
        translated.kind = SdlWindowEventKind::did_enter_background;
        return translated;
    case SDL_EVENT_WILL_ENTER_FOREGROUND:
        translated.kind = SdlWindowEventKind::will_enter_foreground;
        return translated;
    case SDL_EVENT_DID_ENTER_FOREGROUND:
        translated.kind = SdlWindowEventKind::did_enter_foreground;
        return translated;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        translated.kind = SdlWindowEventKind::close_requested;
        translated.window_id = source.window.windowID;
        return translated;
    case SDL_EVENT_WINDOW_RESIZED:
        translated.kind = SdlWindowEventKind::resized;
        translated.window_id = source.window.windowID;
        if (source.window.data1 > 0 && source.window.data2 > 0) {
            translated.extent = WindowExtent{.width = static_cast<std::uint32_t>(source.window.data1),
                                             .height = static_cast<std::uint32_t>(source.window.data2)};
        }
        return translated;
    case SDL_EVENT_WINDOW_MOVED:
        translated.kind = SdlWindowEventKind::moved;
        translated.window_id = source.window.windowID;
        translated.position = WindowPosition{.x = source.window.data1, .y = source.window.data2};
        return translated;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (is_touch_generated_mouse(source.button.which)) {
            return translated;
        }
        if (source.button.button != SDL_BUTTON_LEFT) {
            return translated;
        }
        translated.kind = SdlWindowEventKind::pointer_pressed;
        translated.window_id = source.button.windowID;
        translated.pointer = sdl3_mouse_pointer_sample(source.button.x, source.button.y);
        translated.pointer_id = translated.pointer.id;
        return translated;
    case SDL_EVENT_MOUSE_MOTION:
        if (is_touch_generated_mouse(source.motion.which)) {
            return translated;
        }
        translated.kind = SdlWindowEventKind::pointer_moved;
        translated.window_id = source.motion.windowID;
        translated.pointer = sdl3_mouse_pointer_sample(source.motion.x, source.motion.y);
        translated.pointer_id = translated.pointer.id;
        return translated;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (is_touch_generated_mouse(source.button.which)) {
            return translated;
        }
        if (source.button.button != SDL_BUTTON_LEFT) {
            return translated;
        }
        translated.kind = SdlWindowEventKind::pointer_released;
        translated.window_id = source.button.windowID;
        translated.pointer_id = primary_pointer_id;
        return translated;
    case SDL_EVENT_FINGER_DOWN:
        translated.kind = SdlWindowEventKind::pointer_pressed;
        translated.window_id = source.tfinger.windowID;
        translated.pointer =
            sdl3_touch_pointer_sample(source.tfinger.fingerID, source.tfinger.x, source.tfinger.y, window_extent);
        translated.pointer_id = translated.pointer.id;
        return translated;
    case SDL_EVENT_FINGER_MOTION:
        translated.kind = SdlWindowEventKind::pointer_moved;
        translated.window_id = source.tfinger.windowID;
        translated.pointer =
            sdl3_touch_pointer_sample(source.tfinger.fingerID, source.tfinger.x, source.tfinger.y, window_extent);
        translated.pointer_id = translated.pointer.id;
        return translated;
    case SDL_EVENT_FINGER_UP:
        translated.kind = SdlWindowEventKind::pointer_released;
        translated.window_id = source.tfinger.windowID;
        translated.pointer_id = sdl3_touch_pointer_id(source.tfinger.fingerID);
        return translated;
    case SDL_EVENT_FINGER_CANCELED:
        translated.kind = SdlWindowEventKind::pointer_canceled;
        translated.window_id = source.tfinger.windowID;
        translated.pointer_id = sdl3_touch_pointer_id(source.tfinger.fingerID);
        return translated;
    case SDL_EVENT_GAMEPAD_ADDED:
        translated.kind = SdlWindowEventKind::gamepad_connected;
        translated.gamepad_id = static_cast<GamepadId>(source.gdevice.which);
        return translated;
    case SDL_EVENT_GAMEPAD_REMOVED:
        translated.kind = SdlWindowEventKind::gamepad_disconnected;
        translated.gamepad_id = static_cast<GamepadId>(source.gdevice.which);
        return translated;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        translated.kind = source.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? SdlWindowEventKind::gamepad_button_pressed
                                                                       : SdlWindowEventKind::gamepad_button_released;
        translated.gamepad_id = static_cast<GamepadId>(source.gbutton.which);
        translated.gamepad_button = sdl3_gamepad_button_to_button(source.gbutton.button);
        return translated;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        translated.kind = SdlWindowEventKind::gamepad_axis_moved;
        translated.gamepad_id = static_cast<GamepadId>(source.gaxis.which);
        translated.gamepad_axis = sdl3_gamepad_axis_to_axis(source.gaxis.axis);
        translated.gamepad_axis_value = sdl3_gamepad_axis_value(translated.gamepad_axis, source.gaxis.value);
        return translated;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        translated.kind =
            source.type == SDL_EVENT_KEY_DOWN ? SdlWindowEventKind::key_pressed : SdlWindowEventKind::key_released;
        translated.window_id = source.key.windowID;
        translated.key = sdl3_key_to_key(source.key.key);
        translated.sdl_keycode = source.key.key;
        translated.key_modifiers = static_cast<std::uint16_t>(source.key.mod);
        translated.repeated = source.key.repeat;
        return translated;
    case SDL_EVENT_TEXT_INPUT:
        translated.kind = SdlWindowEventKind::text_input;
        translated.window_id = source.text.windowID;
        translated.text = copy_sdl_text(source.text.text);
        return translated;
    case SDL_EVENT_TEXT_EDITING:
        translated.kind = SdlWindowEventKind::text_editing;
        translated.window_id = source.edit.windowID;
        translated.text = copy_sdl_text(source.edit.text);
        translated.text_edit_start = source.edit.start;
        translated.text_edit_length = source.edit.length;
        return translated;
    case SDL_EVENT_TEXT_EDITING_CANDIDATES:
        translated.kind = SdlWindowEventKind::text_editing_candidates;
        translated.window_id = source.edit_candidates.windowID;
        translated.text_editing_candidates =
            copy_sdl_text_candidates(source.edit_candidates.candidates, source.edit_candidates.num_candidates);
        translated.selected_text_editing_candidate = source.edit_candidates.selected_candidate;
        translated.text_editing_candidates_horizontal = source.edit_candidates.horizontal;
        return translated;
    default:
        return translated;
    }
}

SdlWindow::SdlWindow(WindowDesc desc) : title_(std::move(desc.title)), extent_(desc.extent), position_(desc.position) {
    validate_window_desc(WindowDesc{.title = title_, .extent = extent_});

    SDL_Window* window = SDL_CreateWindow(title_.c_str(), static_cast<int>(extent_.width),
                                          static_cast<int>(extent_.height), SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        throw sdl_error("SDL_CreateWindow");
    }

    window_id_ = SDL_GetWindowID(window);
    if (window_id_ == 0) {
        SDL_DestroyWindow(window);
        throw sdl_error("SDL_GetWindowID");
    }

    impl_ = std::make_unique<Impl>(window);
    move(position_);
}

SdlWindow::~SdlWindow() = default;

std::string_view SdlWindow::title() const noexcept {
    return title_;
}

WindowExtent SdlWindow::extent() const noexcept {
    return extent_;
}

WindowPosition SdlWindow::position() const noexcept {
    return position_;
}

bool SdlWindow::is_open() const noexcept {
    return open_;
}

SdlNativeWindowHandle SdlWindow::native_window() const noexcept {
    return SdlNativeWindowHandle{reinterpret_cast<std::uintptr_t>(impl_ != nullptr ? impl_->window : nullptr)};
}

WindowDisplayState SdlWindow::display_state() const {
    const auto display_id = SDL_GetDisplayForWindow(impl_->window);
    if (display_id == 0) {
        throw sdl_error("SDL_GetDisplayForWindow");
    }

    SDL_Rect safe_area{};
    const auto safe_area_rect = SDL_GetWindowSafeArea(impl_->window, &safe_area) ? display_rect_from_sdl(safe_area)
                                                                                 : window_extent_rect(extent_);

    return WindowDisplayState{
        .display_id = display_id,
        .content_scale = positive_or_default(SDL_GetWindowDisplayScale(impl_->window), 1.0F),
        .pixel_density = positive_or_default(SDL_GetWindowPixelDensity(impl_->window), 1.0F),
        .safe_area = is_valid_display_rect(safe_area_rect) ? safe_area_rect : window_extent_rect(extent_),
    };
}

void SdlWindow::resize(WindowExtent extent) {
    if (extent.width == 0 || extent.height == 0) {
        throw std::invalid_argument("window extent must be non-zero");
    }

    if (!SDL_SetWindowSize(impl_->window, static_cast<int>(extent.width), static_cast<int>(extent.height))) {
        throw sdl_error("SDL_SetWindowSize");
    }
    extent_ = extent;
}

void SdlWindow::move(WindowPosition position) {
    if (!SDL_SetWindowPosition(impl_->window, static_cast<int>(position.x), static_cast<int>(position.y))) {
        throw sdl_error("SDL_SetWindowPosition");
    }
    position_ = position;
}

void SdlWindow::apply_placement(WindowPlacement placement) {
    resize(placement.extent);
    move(placement.position);
}

void SdlWindow::request_close() noexcept {
    open_ = false;
}

namespace {

[[nodiscard]] LifecycleEventKind lifecycle_event_kind_from_sdl(SdlWindowEventKind event_type) noexcept {
    switch (event_type) {
    case SdlWindowEventKind::quit_requested:
        return LifecycleEventKind::quit_requested;
    case SdlWindowEventKind::terminating:
        return LifecycleEventKind::terminating;
    case SdlWindowEventKind::low_memory:
        return LifecycleEventKind::low_memory;
    case SdlWindowEventKind::will_enter_background:
        return LifecycleEventKind::will_enter_background;
    case SdlWindowEventKind::did_enter_background:
        return LifecycleEventKind::did_enter_background;
    case SdlWindowEventKind::will_enter_foreground:
        return LifecycleEventKind::will_enter_foreground;
    case SdlWindowEventKind::did_enter_foreground:
        return LifecycleEventKind::did_enter_foreground;
    default:
        return LifecycleEventKind::quit_requested;
    }
}

[[nodiscard]] bool is_sdl_lifecycle_event(SdlWindowEventKind event_type) noexcept {
    return event_type == SdlWindowEventKind::quit_requested || event_type == SdlWindowEventKind::terminating ||
           event_type == SdlWindowEventKind::low_memory || event_type == SdlWindowEventKind::will_enter_background ||
           event_type == SdlWindowEventKind::did_enter_background ||
           event_type == SdlWindowEventKind::will_enter_foreground ||
           event_type == SdlWindowEventKind::did_enter_foreground;
}

[[nodiscard]] bool applies_to_window(std::uint32_t event_window_id, std::uint32_t window_id) noexcept {
    return event_window_id == 0 || event_window_id == window_id;
}

} // namespace

void SdlWindow::handle_event(const SdlWindowEvent& event, VirtualInput* input, VirtualPointerInput* pointer_input,
                             VirtualGamepadInput* gamepad_input, VirtualLifecycle* lifecycle) noexcept {
    if (lifecycle != nullptr && is_sdl_lifecycle_event(event.kind)) {
        lifecycle->push(lifecycle_event_kind_from_sdl(event.kind));
    }

    if (event.kind == SdlWindowEventKind::quit_requested) {
        open_ = false;
        return;
    }

    if (event.kind == SdlWindowEventKind::terminating || event.kind == SdlWindowEventKind::low_memory ||
        event.kind == SdlWindowEventKind::will_enter_background ||
        event.kind == SdlWindowEventKind::did_enter_background ||
        event.kind == SdlWindowEventKind::will_enter_foreground ||
        event.kind == SdlWindowEventKind::did_enter_foreground) {
        return;
    }

    if (!applies_to_window(event.window_id, window_id_)) {
        return;
    }

    if (event.kind == SdlWindowEventKind::close_requested) {
        open_ = false;
        return;
    }

    if (event.kind == SdlWindowEventKind::resized) {
        if (event.extent.width > 0 && event.extent.height > 0) {
            extent_ = event.extent;
        }
        return;
    }

    if (event.kind == SdlWindowEventKind::moved) {
        position_ = event.position;
        return;
    }

    if (pointer_input != nullptr && event.kind == SdlWindowEventKind::pointer_pressed) {
        pointer_input->press(event.pointer);
        return;
    }

    if (pointer_input != nullptr && event.kind == SdlWindowEventKind::pointer_moved) {
        pointer_input->move(event.pointer);
        return;
    }

    if (pointer_input != nullptr && event.kind == SdlWindowEventKind::pointer_released) {
        pointer_input->release(event.pointer_id);
        return;
    }

    if (pointer_input != nullptr && event.kind == SdlWindowEventKind::pointer_canceled) {
        pointer_input->cancel(event.pointer_id);
        return;
    }

    if (gamepad_input != nullptr && event.kind == SdlWindowEventKind::gamepad_connected) {
        gamepad_input->connect(event.gamepad_id);
        return;
    }

    if (gamepad_input != nullptr && event.kind == SdlWindowEventKind::gamepad_disconnected) {
        gamepad_input->disconnect(event.gamepad_id);
        return;
    }

    if (gamepad_input != nullptr && (event.kind == SdlWindowEventKind::gamepad_button_pressed ||
                                     event.kind == SdlWindowEventKind::gamepad_button_released)) {
        if (event.gamepad_button == GamepadButton::unknown) {
            return;
        }

        if (event.kind == SdlWindowEventKind::gamepad_button_pressed) {
            gamepad_input->press(event.gamepad_id, event.gamepad_button);
        } else {
            gamepad_input->release(event.gamepad_id, event.gamepad_button);
        }
        return;
    }

    if (gamepad_input != nullptr && event.kind == SdlWindowEventKind::gamepad_axis_moved) {
        if (event.gamepad_axis == GamepadAxis::unknown) {
            return;
        }

        gamepad_input->set_axis(event.gamepad_id, event.gamepad_axis, event.gamepad_axis_value);
        return;
    }

    if (event.kind != SdlWindowEventKind::key_pressed && event.kind != SdlWindowEventKind::key_released) {
        return;
    }

    if (input == nullptr || event.repeated) {
        return;
    }

    if (event.key == Key::unknown) {
        return;
    }

    if (event.kind == SdlWindowEventKind::key_pressed) {
        input->press(event.key);
    } else if (event.kind == SdlWindowEventKind::key_released) {
        input->release(event.key);
    }
}

} // namespace mirakana
