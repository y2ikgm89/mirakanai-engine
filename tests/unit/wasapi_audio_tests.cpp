// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/audio/wasapi/wasapi_audio_device.hpp"

#include <cstddef>

namespace {

[[nodiscard]] mirakana::AudioDeviceFormat float_stereo_format() noexcept {
    return mirakana::AudioDeviceFormat{
        .sample_rate = 48000,
        .channel_count = 2,
        .sample_format = mirakana::AudioSampleFormat::float32,
    };
}

} // namespace

MK_TEST("wasapi audio adapter validates engine audio device formats") {
    MK_REQUIRE(mirakana::wasapi_audio_device_format_supported(float_stereo_format()));
    MK_REQUIRE(mirakana::wasapi_audio_device_format_supported(mirakana::AudioDeviceFormat{
        .sample_rate = 44100,
        .channel_count = 1,
        .sample_format = mirakana::AudioSampleFormat::pcm16,
    }));
    MK_REQUIRE(!mirakana::wasapi_audio_device_format_supported(mirakana::AudioDeviceFormat{
        .sample_rate = 0,
        .channel_count = 2,
        .sample_format = mirakana::AudioSampleFormat::float32,
    }));
    MK_REQUIRE(!mirakana::wasapi_audio_device_format_supported(mirakana::AudioDeviceFormat{
        .sample_rate = 48000,
        .channel_count = 2,
        .sample_format = mirakana::AudioSampleFormat::unknown,
    }));
}

MK_TEST("wasapi runtime plan keeps com startup and default endpoint ownership private") {
    const auto plan = mirakana::plan_wasapi_audio_runtime(mirakana::WasapiAudioRuntimeDesc{
        .initialize_com = true,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.initialize_com);
    MK_REQUIRE(plan.use_multimedia_device_enumerator);
    MK_REQUIRE(plan.use_default_render_endpoint);
    MK_REQUIRE(plan.use_console_role);
    MK_REQUIRE(!plan.native_handle_exposed);
}

MK_TEST("wasapi shared mode stream plan uses render client without exclusive or callback claims") {
    const auto plan = mirakana::plan_wasapi_shared_mode_stream(mirakana::WasapiAudioDeviceDesc{
        .format = float_stereo_format(),
        .target_latency_frames = 960,
        .start_paused = true,
        .stream_name = "MIRAIKANAI Test Output",
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostic == mirakana::WasapiAudioStreamDiagnostic::none);
    MK_REQUIRE(plan.format.sample_rate == 48000);
    MK_REQUIRE(plan.format.channel_count == 2);
    MK_REQUIRE(plan.format.sample_format == mirakana::AudioSampleFormat::float32);
    MK_REQUIRE(plan.target_latency_frames == 960);
    MK_REQUIRE(plan.use_default_render_endpoint);
    MK_REQUIRE(plan.use_shared_mode);
    MK_REQUIRE(plan.use_render_client);
    MK_REQUIRE(!plan.use_event_callback);
    MK_REQUIRE(!plan.use_exclusive_mode);
    MK_REQUIRE(!plan.use_capture);
    MK_REQUIRE(plan.queue_silence_before_start);
    MK_REQUIRE(!plan.native_handle_exposed);
}

MK_TEST("wasapi shared mode stream plan rejects invalid stream descriptors") {
    const auto invalid_format = mirakana::plan_wasapi_shared_mode_stream(mirakana::WasapiAudioDeviceDesc{
        .format =
            mirakana::AudioDeviceFormat{
                .sample_rate = 0,
                .channel_count = 2,
                .sample_format = mirakana::AudioSampleFormat::float32,
            },
        .target_latency_frames = 960,
        .start_paused = true,
        .stream_name = {},
    });
    MK_REQUIRE(!invalid_format.succeeded());
    MK_REQUIRE(invalid_format.diagnostic == mirakana::WasapiAudioStreamDiagnostic::invalid_format);

    const auto invalid_latency = mirakana::plan_wasapi_shared_mode_stream(mirakana::WasapiAudioDeviceDesc{
        .format = float_stereo_format(),
        .target_latency_frames = 0,
        .start_paused = true,
        .stream_name = {},
    });
    MK_REQUIRE(!invalid_latency.succeeded());
    MK_REQUIRE(invalid_latency.diagnostic == mirakana::WasapiAudioStreamDiagnostic::invalid_latency_frames);
}

MK_TEST("wasapi render queue plan clamps writes to shared endpoint space") {
    const auto plan = mirakana::plan_wasapi_render_queue(mirakana::WasapiAudioQueueRequest{
        .format = float_stereo_format(),
        .buffer_frames = 512,
        .current_padding_frames = 300,
        .requested_frames = 256,
        .interleaved_sample_count = 512,
        .submit_silence = false,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostic == mirakana::WasapiAudioQueueDiagnostic::none);
    MK_REQUIRE(plan.available_frames == 212);
    MK_REQUIRE(plan.frames_to_write == 212);
    MK_REQUIRE(plan.queued_frames_after == 512);
    MK_REQUIRE(!plan.submit_silence);
}

MK_TEST("wasapi render queue plan validates padding and sample alignment") {
    const auto invalid_padding = mirakana::plan_wasapi_render_queue(mirakana::WasapiAudioQueueRequest{
        .format = float_stereo_format(),
        .buffer_frames = 128,
        .current_padding_frames = 129,
        .requested_frames = 16,
        .interleaved_sample_count = 32,
        .submit_silence = false,
    });
    MK_REQUIRE(!invalid_padding.succeeded());
    MK_REQUIRE(invalid_padding.diagnostic == mirakana::WasapiAudioQueueDiagnostic::padding_exceeds_buffer);

    const auto invalid_samples = mirakana::plan_wasapi_render_queue(mirakana::WasapiAudioQueueRequest{
        .format = float_stereo_format(),
        .buffer_frames = 128,
        .current_padding_frames = 0,
        .requested_frames = 16,
        .interleaved_sample_count = 31,
        .submit_silence = false,
    });
    MK_REQUIRE(!invalid_samples.succeeded());
    MK_REQUIRE(invalid_samples.diagnostic == mirakana::WasapiAudioQueueDiagnostic::sample_count_mismatch);

    const auto silence = mirakana::plan_wasapi_render_queue(mirakana::WasapiAudioQueueRequest{
        .format = float_stereo_format(),
        .buffer_frames = 128,
        .current_padding_frames = 0,
        .requested_frames = 16,
        .interleaved_sample_count = 0,
        .submit_silence = true,
    });
    MK_REQUIRE(silence.succeeded());
    MK_REQUIRE(silence.submit_silence);
}

MK_TEST("wasapi audio adapter exposes production stream queue lifecycle evidence") {
    const auto evidence = mirakana::wasapi_audio_device_lifecycle_evidence(true);

    MK_REQUIRE(evidence.backend_id == "wasapi");
    MK_REQUIRE(evidence.uses_logical_device);
    MK_REQUIRE(evidence.uses_audio_stream);
    MK_REQUIRE(evidence.uses_queueing);
    MK_REQUIRE(!evidence.uses_callback);
    MK_REQUIRE(evidence.can_pause_resume);
    MK_REQUIRE(evidence.can_clear);
    MK_REQUIRE(evidence.host_evidence_available);
    MK_REQUIRE(!evidence.native_handle_exposed);
}

int main() {
    return mirakana::test::run_all();
}
