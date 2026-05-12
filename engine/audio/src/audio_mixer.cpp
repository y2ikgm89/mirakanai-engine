// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/audio/audio_mixer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_name(std::string_view value) noexcept {
    return !value.empty() && value.find_first_of("\r\n=") == std::string_view::npos;
}

[[nodiscard]] bool valid_gain(float value) noexcept {
    return std::isfinite(value) && value >= 0.0F && value <= 16.0F;
}

[[nodiscard]] bool finite_point(AudioPoint3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] AudioPoint3 subtract(AudioPoint3 lhs, AudioPoint3 rhs) noexcept {
    return AudioPoint3{.x = lhs.x - rhs.x, .y = lhs.y - rhs.y, .z = lhs.z - rhs.z};
}

[[nodiscard]] float dot(AudioPoint3 lhs, AudioPoint3 rhs) noexcept {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

[[nodiscard]] float length(AudioPoint3 value) noexcept {
    return std::sqrt(dot(value, value));
}

[[nodiscard]] AudioPoint3 normalize(AudioPoint3 value) noexcept {
    const auto value_length = length(value);
    if (value_length <= 0.000001F) {
        return AudioPoint3{};
    }
    const auto inv_length = 1.0F / value_length;
    return AudioPoint3{.x = value.x * inv_length, .y = value.y * inv_length, .z = value.z * inv_length};
}

[[nodiscard]] bool valid_sample_rate(std::uint32_t value) noexcept {
    return value > 0 && value <= 384000U;
}

[[nodiscard]] bool valid_channel_count(std::uint32_t value) noexcept {
    return value > 0 && value <= 64U;
}

[[nodiscard]] bool valid_sample_format(AudioSampleFormat value) noexcept {
    return value == AudioSampleFormat::float32 || value == AudioSampleFormat::pcm16;
}

[[nodiscard]] bool valid_resampling_quality(AudioResamplingQuality value) noexcept {
    return value == AudioResamplingQuality::nearest || value == AudioResamplingQuality::linear;
}

[[nodiscard]] std::uint64_t scale_frames_floor(std::uint64_t value, std::uint32_t numerator,
                                               std::uint32_t denominator) {
    if (denominator == 0U) {
        throw std::invalid_argument("audio frame scale denominator is invalid");
    }
    const auto quotient = value / denominator;
    const auto remainder = value % denominator;
    if (numerator != 0U && quotient > std::numeric_limits<std::uint64_t>::max() / numerator) {
        throw std::overflow_error("audio frame scale overflows");
    }
    return (quotient * numerator) + ((remainder * numerator) / denominator);
}

[[nodiscard]] std::uint64_t scale_frames_ceil(std::uint64_t value, std::uint32_t numerator, std::uint32_t denominator) {
    if (denominator == 0U) {
        throw std::invalid_argument("audio frame scale denominator is invalid");
    }
    const auto floor = scale_frames_floor(value, numerator, denominator);
    const auto remainder = ((value % denominator) * numerator) % denominator;
    if (remainder == 0U) {
        return floor;
    }
    if (floor == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("audio frame scale overflows");
    }
    return floor + 1U;
}

[[nodiscard]] std::uint64_t saturating_add_frames(std::uint64_t lhs, std::uint64_t rhs) noexcept {
    if (lhs > std::numeric_limits<std::uint64_t>::max() - rhs) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return lhs + rhs;
}

[[nodiscard]] AudioFormatConversion audio_format_conversion(const AudioClipDesc& clip,
                                                            const AudioDeviceFormat& device) noexcept {
    auto conversion = AudioFormatConversion::none;
    if (clip.sample_rate != device.sample_rate) {
        conversion |= AudioFormatConversion::resample;
    }
    if (clip.channel_count != device.channel_count) {
        conversion |= AudioFormatConversion::channel_convert;
    }
    if (clip.sample_format != device.sample_format) {
        conversion |= AudioFormatConversion::sample_format_convert;
    }
    return conversion;
}

[[nodiscard]] std::size_t checked_audio_sample_count(std::uint64_t frame_count, std::uint32_t channel_count) {
    const auto channels = static_cast<std::uint64_t>(channel_count);
    if (frame_count != 0U && channels > std::numeric_limits<std::uint64_t>::max() / frame_count) {
        throw std::overflow_error("audio sample count overflows");
    }
    const auto sample_count = frame_count * channels;
    if (sample_count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::overflow_error("audio sample count overflows");
    }
    return static_cast<std::size_t>(sample_count);
}

[[nodiscard]] const AudioClipSampleData* find_clip_samples(std::span<const AudioClipSampleData> samples,
                                                           AssetId clip) noexcept {
    const auto it =
        std::ranges::find_if(samples, [clip](const AudioClipSampleData& candidate) { return candidate.clip == clip; });
    return it == samples.end() ? nullptr : &*it;
}

[[nodiscard]] const AudioSpatialVoiceDesc* find_spatial_voice(std::span<const AudioSpatialVoiceDesc> voices,
                                                              AudioVoiceId voice) noexcept {
    const auto it = std::ranges::find_if(
        voices, [voice](const AudioSpatialVoiceDesc& candidate) { return candidate.voice == voice; });
    return it == voices.end() ? nullptr : &*it;
}

[[nodiscard]] float clamp_audio_sample(float sample) noexcept {
    return std::clamp(sample, -1.0F, 1.0F);
}

[[nodiscard]] bool same_format(const AudioDeviceFormat& lhs, const AudioDeviceFormat& rhs) noexcept {
    return lhs.sample_rate == rhs.sample_rate && lhs.channel_count == rhs.channel_count &&
           lhs.sample_format == rhs.sample_format;
}

[[nodiscard]] AudioDeviceFormat format_for_clip(const AudioClipDesc& clip) noexcept {
    return AudioDeviceFormat{
        .sample_rate = clip.sample_rate, .channel_count = clip.channel_count, .sample_format = clip.sample_format};
}

[[nodiscard]] float sample_source_channel(const AudioClipSampleData& samples, std::uint64_t frame,
                                          std::uint32_t output_channel, std::uint32_t output_channels) {
    const auto source_channels = samples.format.channel_count;
    const auto frame_start = checked_audio_sample_count(frame, source_channels);
    if (source_channels == output_channels) {
        return samples.interleaved_float_samples[frame_start + output_channel];
    }
    if (source_channels == 1U) {
        return samples.interleaved_float_samples[frame_start];
    }
    if (output_channels == 1U) {
        float sum = 0.0F;
        for (std::uint32_t channel = 0; channel < source_channels; ++channel) {
            sum += samples.interleaved_float_samples[frame_start + channel];
        }
        return sum / static_cast<float>(source_channels);
    }
    const auto source_channel = std::min(output_channel, source_channels - 1U);
    return samples.interleaved_float_samples[frame_start + source_channel];
}

[[nodiscard]] float sample_source_channel_resampled(const AudioClipSampleData& samples,
                                                    std::uint64_t source_start_frame, std::uint64_t output_frame,
                                                    std::uint32_t output_sample_rate, std::uint32_t output_channel,
                                                    std::uint32_t output_channels, bool looping,
                                                    AudioResamplingQuality quality) {
    const auto source_rate = samples.format.sample_rate;
    const auto position = static_cast<long double>(source_start_frame) +
                          ((static_cast<long double>(output_frame) * static_cast<long double>(source_rate)) /
                           static_cast<long double>(output_sample_rate));
    if (!std::isfinite(static_cast<double>(position)) ||
        position > static_cast<long double>(std::numeric_limits<std::uint64_t>::max())) {
        throw std::overflow_error("audio source frame overflows");
    }

    auto frame0 = static_cast<std::uint64_t>(std::floor(position));
    const auto fraction = static_cast<float>(position - static_cast<long double>(frame0));
    if (looping) {
        frame0 %= samples.frame_count;
    } else if (frame0 >= samples.frame_count) {
        return 0.0F;
    }

    if (quality == AudioResamplingQuality::nearest || source_rate == output_sample_rate) {
        return sample_source_channel(samples, frame0, output_channel, output_channels);
    }

    auto frame1 = frame0 + 1U;
    if (looping) {
        frame1 %= samples.frame_count;
    } else if (frame1 >= samples.frame_count) {
        frame1 = frame0;
    }

    const auto sample0 = sample_source_channel(samples, frame0, output_channel, output_channels);
    const auto sample1 = sample_source_channel(samples, frame1, output_channel, output_channels);
    return sample0 + ((sample1 - sample0) * fraction);
}

} // namespace

bool is_valid_audio_bus_desc(const AudioBusDesc& bus) noexcept {
    return valid_name(bus.name) && valid_gain(bus.gain);
}

bool is_valid_audio_voice_desc(const AudioVoiceDesc& voice) noexcept {
    return voice.clip.value != 0 && valid_name(voice.bus) && valid_gain(voice.gain);
}

bool is_valid_audio_spatial_listener_desc(const AudioSpatialListenerDesc& listener) noexcept {
    const auto right_length = length(listener.right);
    return finite_point(listener.position) && finite_point(listener.right) && std::isfinite(right_length) &&
           right_length > 0.000001F;
}

bool is_valid_audio_spatial_voice_desc(const AudioSpatialVoiceDesc& voice) noexcept {
    if (voice.voice == null_audio_voice) {
        return false;
    }
    if (!voice.spatialized) {
        return true;
    }
    return finite_point(voice.position) && std::isfinite(voice.min_distance) && std::isfinite(voice.max_distance) &&
           voice.min_distance >= 0.0F && voice.max_distance > voice.min_distance;
}

bool is_valid_audio_clip_desc(const AudioClipDesc& clip) noexcept {
    return clip.clip.value != 0 && valid_sample_rate(clip.sample_rate) && valid_channel_count(clip.channel_count) &&
           clip.frame_count > 0 && valid_sample_format(clip.sample_format) &&
           clip.buffered_frame_count <= clip.frame_count;
}

bool is_valid_audio_device_format(const AudioDeviceFormat& format) noexcept {
    return valid_sample_rate(format.sample_rate) && valid_channel_count(format.channel_count) &&
           valid_sample_format(format.sample_format);
}

bool is_valid_audio_render_request(const AudioRenderRequest& request) noexcept {
    return is_valid_audio_device_format(request.format) && request.frame_count > 0 &&
           request.device_frame <= std::numeric_limits<std::uint64_t>::max() - request.frame_count &&
           valid_resampling_quality(request.resampling_quality);
}

bool is_valid_audio_clip_sample_data(const AudioClipSampleData& samples) noexcept {
    if (samples.clip.value == 0 || !is_valid_audio_device_format(samples.format) ||
        samples.format.sample_format != AudioSampleFormat::float32 || samples.frame_count == 0U) {
        return false;
    }

    std::size_t expected_sample_count = 0;
    try {
        expected_sample_count = checked_audio_sample_count(samples.frame_count, samples.format.channel_count);
    } catch (const std::overflow_error&) {
        return false;
    }
    if (samples.interleaved_float_samples.size() != expected_sample_count) {
        return false;
    }

    return std::ranges::all_of(samples.interleaved_float_samples, [](float sample) { return std::isfinite(sample); });
}

bool is_valid_audio_device_stream_request(const AudioDeviceStreamRequest& request) noexcept {
    return is_valid_audio_device_format(request.format) && request.target_queued_frames > 0U &&
           request.max_render_frames > 0U && valid_resampling_quality(request.resampling_quality);
}

AudioDeviceStreamPlan plan_audio_device_stream(AudioDeviceStreamRequest request) noexcept {
    AudioDeviceStreamPlan plan;
    plan.render_request.format = request.format;
    plan.render_request.device_frame = request.device_frame;
    plan.render_request.underrun_warning_threshold_frames = request.underrun_warning_threshold_frames;
    plan.render_request.resampling_quality = request.resampling_quality;
    plan.queued_frames_before = request.queued_frames;
    plan.target_queued_frames = request.target_queued_frames;
    plan.queued_frames_after = request.queued_frames;

    if (!is_valid_audio_device_format(request.format)) {
        plan.status = AudioDeviceStreamStatus::invalid_request;
        plan.diagnostic = AudioDeviceStreamDiagnostic::invalid_format;
        return plan;
    }
    if (request.target_queued_frames == 0U) {
        plan.status = AudioDeviceStreamStatus::invalid_request;
        plan.diagnostic = AudioDeviceStreamDiagnostic::invalid_queue_target;
        return plan;
    }
    if (request.max_render_frames == 0U) {
        plan.status = AudioDeviceStreamStatus::invalid_request;
        plan.diagnostic = AudioDeviceStreamDiagnostic::invalid_render_budget;
        return plan;
    }
    if (!valid_resampling_quality(request.resampling_quality)) {
        plan.status = AudioDeviceStreamStatus::invalid_request;
        plan.diagnostic = AudioDeviceStreamDiagnostic::invalid_resampling_quality;
        return plan;
    }
    if (request.queued_frames >= request.target_queued_frames) {
        plan.status = AudioDeviceStreamStatus::no_work;
        plan.diagnostic = AudioDeviceStreamDiagnostic::none;
        return plan;
    }

    const auto frames_needed = request.target_queued_frames - request.queued_frames;
    const auto frames_to_render = std::min(frames_needed, request.max_render_frames);
    const auto queued_frames = static_cast<std::uint64_t>(request.queued_frames);
    const auto render_frames = static_cast<std::uint64_t>(frames_to_render);
    if (request.device_frame > std::numeric_limits<std::uint64_t>::max() - queued_frames) {
        plan.status = AudioDeviceStreamStatus::invalid_request;
        plan.diagnostic = AudioDeviceStreamDiagnostic::device_frame_overflow;
        return plan;
    }
    const auto render_start_frame = request.device_frame + queued_frames;
    if (render_start_frame > std::numeric_limits<std::uint64_t>::max() - render_frames) {
        plan.status = AudioDeviceStreamStatus::invalid_request;
        plan.diagnostic = AudioDeviceStreamDiagnostic::device_frame_overflow;
        return plan;
    }

    plan.status = AudioDeviceStreamStatus::ready;
    plan.diagnostic = AudioDeviceStreamDiagnostic::none;
    plan.frames_to_render = frames_to_render;
    plan.queued_frames_after = request.queued_frames + frames_to_render;
    plan.render_request.frame_count = frames_to_render;
    plan.render_request.device_frame = render_start_frame;
    return plan;
}

AudioDeviceStreamRenderResult
render_audio_device_stream_interleaved_float(const AudioMixer& mixer, AudioDeviceStreamRequest request,
                                             std::span<const AudioClipSampleData> samples) {
    AudioDeviceStreamRenderResult result;
    result.plan = plan_audio_device_stream(request);
    if (result.plan.status == AudioDeviceStreamStatus::ready && result.plan.frames_to_render > 0U) {
        result.buffer = mixer.render_interleaved_float(result.plan.render_request, samples);
    }
    return result;
}

AudioMixer::AudioMixer() {
    add_bus(AudioBusDesc{.name = "master", .gain = 1.0F, .muted = false});
}

bool AudioMixer::try_add_bus(AudioBusDesc bus) {
    if (!is_valid_audio_bus_desc(bus) || bus_indices_.find(bus.name) != bus_indices_.end()) {
        return false;
    }
    const auto index = buses_.size();
    bus_indices_.emplace(bus.name, index);
    buses_.push_back(AudioBusState{std::move(bus)});
    return true;
}

void AudioMixer::add_bus(AudioBusDesc bus) {
    if (!try_add_bus(std::move(bus))) {
        throw std::invalid_argument("audio bus description is invalid or duplicated");
    }
}

void AudioMixer::set_bus_gain(std::string_view bus, float gain) {
    if (!valid_gain(gain)) {
        throw std::invalid_argument("audio bus gain is invalid");
    }
    auto* state = find_bus(bus);
    if (state == nullptr) {
        throw std::invalid_argument("audio bus does not exist");
    }
    state->desc.gain = gain;
}

void AudioMixer::set_bus_muted(std::string_view bus, bool muted) {
    auto* state = find_bus(bus);
    if (state == nullptr) {
        throw std::invalid_argument("audio bus does not exist");
    }
    state->desc.muted = muted;
}

AudioVoiceId AudioMixer::play(AudioVoiceDesc voice) {
    if (!is_valid_audio_voice_desc(voice) || find_bus(voice.bus) == nullptr) {
        throw std::invalid_argument("audio voice description is invalid");
    }
    const auto id = AudioVoiceId{static_cast<std::uint32_t>(voices_.size() + 1U)};
    voices_.push_back(AudioVoiceState{.id = id, .desc = std::move(voice)});
    return id;
}

std::vector<AudioMixCommand> AudioMixer::mix_plan() const {
    std::vector<AudioMixCommand> plan;
    plan.reserve(voices_.size());
    for (const auto& voice : voices_) {
        const auto* bus = find_bus(voice.desc.bus);
        if (bus == nullptr) {
            continue;
        }
        const auto gain = bus->desc.muted ? 0.0F : voice.desc.gain * bus->desc.gain;
        plan.push_back(AudioMixCommand{
            .voice = voice.id,
            .clip = voice.desc.clip,
            .bus = voice.desc.bus,
            .gain = gain,
            .looping = voice.desc.looping,
        });
    }
    return plan;
}

std::vector<AudioSpatialMixCommand>
AudioMixer::spatial_mix_plan(AudioSpatialListenerDesc listener,
                             std::span<const AudioSpatialVoiceDesc> spatial_voices) const {
    if (!is_valid_audio_spatial_listener_desc(listener)) {
        throw std::invalid_argument("audio spatial listener description is invalid");
    }
    for (const auto& spatial_voice : spatial_voices) {
        if (!is_valid_audio_spatial_voice_desc(spatial_voice)) {
            throw std::invalid_argument("audio spatial voice description is invalid");
        }
    }
    for (auto lhs = spatial_voices.begin(); lhs != spatial_voices.end(); ++lhs) {
        for (auto rhs = lhs + 1; rhs != spatial_voices.end(); ++rhs) {
            if (lhs->voice == rhs->voice) {
                throw std::invalid_argument("audio spatial voice descriptions contain duplicates");
            }
        }
    }

    const auto base_plan = mix_plan();
    const auto right = normalize(listener.right);
    std::vector<AudioSpatialMixCommand> result;
    result.reserve(base_plan.size());

    for (const auto& command : base_plan) {
        AudioSpatialMixCommand row{
            .voice = command.voice,
            .clip = command.clip,
            .bus = command.bus,
            .spatialized = false,
            .distance = 0.0F,
            .attenuation = 1.0F,
            .pan = 0.0F,
            .center_gain = command.gain,
            .left_gain = command.gain,
            .right_gain = command.gain,
            .looping = command.looping,
        };

        const auto* spatial_voice = find_spatial_voice(spatial_voices, command.voice);
        if (spatial_voice != nullptr && spatial_voice->spatialized) {
            const auto delta = subtract(spatial_voice->position, listener.position);
            const auto distance = length(delta);
            if (!std::isfinite(distance)) {
                throw std::invalid_argument("audio spatial voice distance is invalid");
            }
            float attenuation = 1.0F;
            if (distance >= spatial_voice->max_distance) {
                attenuation = 0.0F;
            } else if (distance > spatial_voice->min_distance) {
                attenuation = (spatial_voice->max_distance - distance) /
                              (spatial_voice->max_distance - spatial_voice->min_distance);
            }

            const auto direction = distance > 0.000001F ? normalize(delta) : AudioPoint3{};
            const auto pan = std::clamp(dot(direction, right), -1.0F, 1.0F);
            const auto center_gain = command.gain * attenuation;
            row.spatialized = true;
            row.distance = distance;
            row.attenuation = attenuation;
            row.pan = pan;
            row.center_gain = center_gain;
            row.left_gain = center_gain * (pan > 0.0F ? 1.0F - pan : 1.0F);
            row.right_gain = center_gain * (pan < 0.0F ? 1.0F + pan : 1.0F);
        }

        result.push_back(std::move(row));
    }

    return result;
}

bool AudioMixer::register_clip(AudioClipDesc clip) {
    if (!is_valid_audio_clip_desc(clip) || clips_.find(clip.clip) != clips_.end()) {
        return false;
    }
    clips_.emplace(clip.clip, clip);
    return true;
}

void AudioMixer::add_clip(AudioClipDesc clip) {
    if (!register_clip(clip)) {
        throw std::invalid_argument("audio clip description is invalid or duplicated");
    }
}

void AudioMixer::set_clip_buffered_frames(AssetId clip, std::uint64_t buffered_frame_count) {
    auto it = clips_.find(clip);
    if (it == clips_.end()) {
        throw std::invalid_argument("audio clip does not exist");
    }
    if (buffered_frame_count > it->second.frame_count) {
        throw std::invalid_argument("audio clip buffered frame count is invalid");
    }
    it->second.buffered_frame_count = buffered_frame_count;
}

const AudioClipDesc* AudioMixer::find_clip(AssetId clip) const noexcept {
    const auto it = clips_.find(clip);
    return it == clips_.end() ? nullptr : &it->second;
}

AudioRenderPlan AudioMixer::render_plan(AudioRenderRequest request) const {
    if (!is_valid_audio_render_request(request)) {
        throw std::invalid_argument("audio render request is invalid");
    }

    AudioRenderPlan plan;
    plan.format = request.format;
    plan.requested_output_frames = request.frame_count;
    plan.device_frame = request.device_frame;
    plan.commands.reserve(voices_.size());

    const auto output_end_frame = request.device_frame + request.frame_count;
    for (const auto& voice : voices_) {
        const auto* bus = find_bus(voice.desc.bus);
        const auto* clip = find_clip(voice.desc.clip);
        if (bus == nullptr || clip == nullptr) {
            continue;
        }

        const auto active_output_start = std::max(request.device_frame, voice.desc.start_frame);
        if (active_output_start >= output_end_frame) {
            continue;
        }

        auto output_frame_count = output_end_frame - active_output_start;
        const auto elapsed_output_frames = active_output_start - voice.desc.start_frame;
        auto source_start_frame =
            scale_frames_floor(elapsed_output_frames, clip->sample_rate, request.format.sample_rate);
        if (voice.desc.looping) {
            source_start_frame %= clip->frame_count;
        } else if (source_start_frame >= clip->frame_count) {
            continue;
        }

        auto requested_source_frames =
            scale_frames_ceil(output_frame_count, clip->sample_rate, request.format.sample_rate);
        if (request.resampling_quality == AudioResamplingQuality::linear &&
            clip->sample_rate != request.format.sample_rate &&
            requested_source_frames < std::numeric_limits<std::uint64_t>::max() &&
            (voice.desc.looping ||
             (requested_source_frames <= std::numeric_limits<std::uint64_t>::max() - source_start_frame &&
              source_start_frame + requested_source_frames < clip->frame_count))) {
            ++requested_source_frames;
        }
        if (!voice.desc.looping) {
            const auto remaining_source_frames = clip->frame_count - source_start_frame;
            if (requested_source_frames > remaining_source_frames) {
                requested_source_frames = remaining_source_frames;
                output_frame_count =
                    scale_frames_floor(requested_source_frames, request.format.sample_rate, clip->sample_rate);
            }
        }
        if (output_frame_count == 0U || requested_source_frames == 0U) {
            continue;
        }

        const auto buffered_source_frames = clip->streaming ? clip->buffered_frame_count : clip->frame_count;
        const auto warning_threshold =
            saturating_add_frames(saturating_add_frames(source_start_frame, requested_source_frames),
                                  request.underrun_warning_threshold_frames);
        if (clip->streaming && buffered_source_frames < warning_threshold) {
            const auto available_source_frames =
                buffered_source_frames > source_start_frame ? buffered_source_frames - source_start_frame : 0U;
            plan.underruns.push_back(AudioUnderrunDiagnostic{
                .voice = voice.id,
                .clip = clip->clip,
                .requested_source_frames = requested_source_frames,
                .buffered_source_frames = available_source_frames,
                .missing_source_frames = requested_source_frames > available_source_frames
                                             ? requested_source_frames - available_source_frames
                                             : 0U,
            });
        }

        const auto gain = bus->desc.muted ? 0.0F : voice.desc.gain * bus->desc.gain;
        plan.commands.push_back(AudioRenderCommand{
            .voice = voice.id,
            .clip = voice.desc.clip,
            .bus = voice.desc.bus,
            .gain = gain,
            .output_offset_frames = active_output_start - request.device_frame,
            .output_frame_count = output_frame_count,
            .source_start_frame = source_start_frame,
            .source_frame_count = requested_source_frames,
            .conversion = audio_format_conversion(*clip, request.format),
            .resampling_quality = request.resampling_quality,
            .looping = voice.desc.looping,
        });
    }

    return plan;
}

AudioRenderBuffer AudioMixer::render_interleaved_float(AudioRenderRequest request,
                                                       std::span<const AudioClipSampleData> samples) const {
    if (!is_valid_audio_render_request(request) || request.format.sample_format != AudioSampleFormat::float32) {
        throw std::invalid_argument("audio float render request is invalid");
    }
    for (const auto& sample_data : samples) {
        if (!is_valid_audio_clip_sample_data(sample_data)) {
            throw std::invalid_argument("audio clip sample data is invalid");
        }
    }

    AudioRenderBuffer buffer;
    buffer.format = request.format;
    buffer.frame_count = request.frame_count;
    buffer.device_frame = request.device_frame;
    buffer.interleaved_float_samples.assign(
        checked_audio_sample_count(request.frame_count, request.format.channel_count), 0.0F);
    buffer.plan = render_plan(request);

    for (const auto& command : buffer.plan.commands) {
        const auto* clip = find_clip(command.clip);
        const auto* sample_data = find_clip_samples(samples, command.clip);
        if (clip == nullptr || sample_data == nullptr) {
            throw std::invalid_argument("audio render is missing clip sample data");
        }
        if (clip->sample_rate != sample_data->format.sample_rate ||
            clip->channel_count != sample_data->format.channel_count ||
            clip->sample_format != sample_data->format.sample_format || clip->frame_count != sample_data->frame_count) {
            throw std::invalid_argument("audio clip sample data does not match registered clip metadata");
        }

        for (std::uint64_t output_frame = 0; output_frame < command.output_frame_count; ++output_frame) {
            const auto output_index =
                checked_audio_sample_count(command.output_offset_frames + output_frame, request.format.channel_count);
            for (std::uint32_t channel = 0; channel < request.format.channel_count; ++channel) {
                buffer.interleaved_float_samples[output_index + channel] +=
                    sample_source_channel_resampled(*sample_data, command.source_start_frame, output_frame,
                                                    request.format.sample_rate, channel, request.format.channel_count,
                                                    command.looping, command.resampling_quality) *
                    command.gain;
            }
        }
    }

    for (auto& sample : buffer.interleaved_float_samples) {
        sample = clamp_audio_sample(sample);
    }
    return buffer;
}

AudioMixer::AudioBusState* AudioMixer::find_bus(std::string_view bus) noexcept {
    const auto it = bus_indices_.find(std::string(bus));
    if (it == bus_indices_.end()) {
        return nullptr;
    }
    return &buses_[it->second];
}

const AudioMixer::AudioBusState* AudioMixer::find_bus(std::string_view bus) const noexcept {
    const auto it = bus_indices_.find(std::string(bus));
    if (it == bus_indices_.end()) {
        return nullptr;
    }
    return &buses_[it->second];
}

AudioStreamingQueue::AudioStreamingQueue(AudioClipDesc clip) : clip_(clip) {
    if (!is_valid_audio_clip_desc(clip_) || !clip_.streaming || clip_.buffered_frame_count != 0U) {
        throw std::invalid_argument("audio streaming queue clip description is invalid");
    }
}

const AudioClipDesc& AudioStreamingQueue::clip() const noexcept {
    return clip_;
}

std::uint64_t AudioStreamingQueue::read_cursor_frame() const noexcept {
    return read_cursor_frame_;
}

std::uint64_t AudioStreamingQueue::buffered_end_frame() const noexcept {
    return buffered_end_frame_;
}

std::uint64_t AudioStreamingQueue::buffered_frame_count() const noexcept {
    return buffered_end_frame_ - read_cursor_frame_;
}

bool AudioStreamingQueue::append(AudioStreamingChunkDesc chunk) noexcept {
    if (chunk.clip != clip_.clip || !same_format(chunk.format, format_for_clip(clip_)) || chunk.frame_count == 0U ||
        chunk.start_frame != buffered_end_frame_ ||
        chunk.frame_count > std::numeric_limits<std::uint64_t>::max() - chunk.start_frame ||
        chunk.start_frame + chunk.frame_count > clip_.frame_count) {
        return false;
    }

    buffered_end_frame_ += chunk.frame_count;
    return true;
}

AudioStreamingConsumeResult AudioStreamingQueue::consume(std::uint64_t requested_frames) noexcept {
    const auto available_frames = buffered_frame_count();
    const auto consumed_frames = std::min(requested_frames, available_frames);
    read_cursor_frame_ += consumed_frames;
    const auto missing_frames = requested_frames - consumed_frames;
    return AudioStreamingConsumeResult{
        .requested_frames = requested_frames,
        .consumed_frames = consumed_frames,
        .missing_frames = missing_frames,
        .next_read_cursor_frame = read_cursor_frame_,
        .starved = missing_frames > 0U,
    };
}

} // namespace mirakana
