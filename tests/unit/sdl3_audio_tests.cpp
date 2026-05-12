// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/audio/sdl3/sdl_audio_device.hpp"

#include <cstddef>
#include <vector>

MK_TEST("sdl3 audio adapter validates engine audio device formats") {
    MK_REQUIRE(mirakana::sdl3_audio_device_format_supported(mirakana::AudioDeviceFormat{
        48000,
        2,
        mirakana::AudioSampleFormat::float32,
    }));
    MK_REQUIRE(mirakana::sdl3_audio_device_format_supported(mirakana::AudioDeviceFormat{
        44100,
        1,
        mirakana::AudioSampleFormat::pcm16,
    }));
    MK_REQUIRE(!mirakana::sdl3_audio_device_format_supported(mirakana::AudioDeviceFormat{
        0,
        2,
        mirakana::AudioSampleFormat::float32,
    }));
    MK_REQUIRE(!mirakana::sdl3_audio_device_format_supported(mirakana::AudioDeviceFormat{
        48000,
        2,
        mirakana::AudioSampleFormat::unknown,
    }));
}

MK_TEST("sdl3 audio device opens dummy playback stream and queues pcm data") {
    mirakana::SdlAudioRuntime runtime(mirakana::SdlAudioRuntimeDesc{
        "dummy",
    });
    mirakana::SdlAudioDevice device(mirakana::SdlAudioDeviceDesc{
        mirakana::AudioDeviceFormat{
            48000,
            2,
            mirakana::AudioSampleFormat::float32,
        },
        true,
        "GameEngine Test Output",
    });

    MK_REQUIRE(device.active());
    MK_REQUIRE(device.backend_name() == "sdl3");
    MK_REQUIRE(device.format().sample_rate == 48000);
    MK_REQUIRE(device.format().channel_count == 2);
    MK_REQUIRE(device.format().sample_format == mirakana::AudioSampleFormat::float32);
    MK_REQUIRE(device.paused());
    MK_REQUIRE(device.queued_frames() == 0);

    device.queue_silence_frames(128);
    MK_REQUIRE(device.queued_frames() >= 128);

    mirakana::AudioMixer mixer;
    const auto clip = mirakana::AssetId::from_name("audio/sdl3-rendered-tone");
    mixer.add_clip(mirakana::AudioClipDesc{clip, 48000, 1, 64, mirakana::AudioSampleFormat::float32, false, 64});
    (void)mixer.play(mirakana::AudioVoiceDesc{clip, "master", 0.25F, false});
    const std::vector<mirakana::AudioClipSampleData> source_samples{
        mirakana::AudioClipSampleData{
            clip,
            mirakana::AudioDeviceFormat{48000, 1, mirakana::AudioSampleFormat::float32},
            64,
            std::vector<float>(64, 1.0F),
        },
    };
    const auto queued_before_render = device.queued_frames();
    const auto rendered = mirakana::render_audio_device_stream_interleaved_float(mixer,
                                                                                 mirakana::AudioDeviceStreamRequest{
                                                                                     device.format(),
                                                                                     0,
                                                                                     queued_before_render,
                                                                                     queued_before_render + 64,
                                                                                     64,
                                                                                     0,
                                                                                 },
                                                                                 source_samples);

    MK_REQUIRE(rendered.plan.status == mirakana::AudioDeviceStreamStatus::ready);
    MK_REQUIRE(rendered.buffer.interleaved_float_samples.size() ==
               static_cast<std::size_t>(64 * device.format().channel_count));
    device.queue_interleaved_float_frames(rendered.buffer.interleaved_float_samples);
    MK_REQUIRE(device.queued_frames() >= queued_before_render + 64);

    device.resume();
    MK_REQUIRE(!device.paused());
    device.pause();
    MK_REQUIRE(device.paused());
    device.clear();
    MK_REQUIRE(device.queued_frames() == 0);
}

int main() {
    return mirakana::test::run_all();
}
