// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "android_audio_output.hpp"

#include "mirakana/platform/mobile.hpp"

#include <android/log.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <cstdint>
#include <exception>
#include <memory>

namespace {

struct AndroidAppState {
    mirakana::VirtualLifecycle lifecycle;
    std::unique_ptr<MK_android::AndroidAudioOutput> audio;
};

AndroidAppState* app_state(android_app* app) noexcept {
    return app == nullptr ? nullptr : static_cast<AndroidAppState*>(app->userData);
}

void push_lifecycle(android_app* app, mirakana::MobileLifecycleEventKind kind) {
    auto* state = app_state(app);
    if (state == nullptr) {
        return;
    }

    mirakana::push_mobile_lifecycle_event(state->lifecycle, kind);
}

void start_audio(android_app* app) noexcept {
    auto* state = app_state(app);
    if (state == nullptr) {
        return;
    }

    try {
        if (!state->audio) {
            state->audio = std::make_unique<MK_android::AndroidAudioOutput>(MK_android::AndroidAudioOutputDesc{
                mirakana::AudioDeviceFormat{48000, 2, mirakana::AudioSampleFormat::float32},
                48000,
                false,
            });
        }
        state->audio->start();
    } catch (const std::exception& error) {
        __android_log_print(ANDROID_LOG_WARN, "Mirakanai", "AAudio output unavailable: %s", error.what());
        state->audio.reset();
    }
}

void stop_audio(android_app* app) noexcept {
    auto* state = app_state(app);
    if (state != nullptr && state->audio) {
        state->audio->stop();
    }
}

void handle_app_command(android_app* app, std::int32_t command) {
    switch (command) {
    case APP_CMD_START:
        push_lifecycle(app, mirakana::MobileLifecycleEventKind::started);
        start_audio(app);
        break;
    case APP_CMD_RESUME:
        push_lifecycle(app, mirakana::MobileLifecycleEventKind::resumed);
        start_audio(app);
        break;
    case APP_CMD_PAUSE:
        stop_audio(app);
        push_lifecycle(app, mirakana::MobileLifecycleEventKind::paused);
        break;
    case APP_CMD_STOP:
        stop_audio(app);
        push_lifecycle(app, mirakana::MobileLifecycleEventKind::stopped);
        break;
    case APP_CMD_LOW_MEMORY:
        push_lifecycle(app, mirakana::MobileLifecycleEventKind::low_memory);
        break;
    case APP_CMD_DESTROY:
        stop_audio(app);
        push_lifecycle(app, mirakana::MobileLifecycleEventKind::destroyed);
        if (auto* state = app_state(app)) {
            state->audio.reset();
        }
        break;
    default:
        break;
    }
}

} // namespace

extern "C" void android_main(android_app* app) {
    AndroidAppState state;
    app->userData = &state;
    app->onAppCmd = handle_app_command;

    __android_log_print(ANDROID_LOG_INFO, "Mirakanai", "Starting %s from %s", MK_ANDROID_GAME_NAME,
                        MK_ANDROID_GAME_MANIFEST);

    while (!state.lifecycle.state().terminating && app->destroyRequested == 0) {
        int events = 0;
        android_poll_source* source = nullptr;
        while (ALooper_pollOnce(0, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
            if (app->destroyRequested != 0) {
                stop_audio(app);
                state.lifecycle.push(mirakana::LifecycleEventKind::terminating);
                break;
            }
        }
    }
    stop_audio(app);
    app->userData = nullptr;
}

