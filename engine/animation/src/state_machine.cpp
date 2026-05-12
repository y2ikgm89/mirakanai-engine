// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/animation/state_machine.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

inline constexpr std::size_t invalid_state_index = static_cast<std::size_t>(-1);

[[nodiscard]] bool finite(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool is_safe_name(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('=') == std::string_view::npos;
}

[[nodiscard]] bool state_exists(const AnimationStateMachineDesc& desc, std::string_view state_name) noexcept {
    return std::ranges::any_of(desc.states,
                               [state_name](const AnimationStateDesc& state) { return state.name == state_name; });
}

[[nodiscard]] bool has_duplicate_state_names(const AnimationStateMachineDesc& desc) noexcept {
    for (std::size_t first = 0; first < desc.states.size(); ++first) {
        for (std::size_t second = first + 1U; second < desc.states.size(); ++second) {
            if (desc.states[first].name == desc.states[second].name) {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] float clamp_blend_alpha(float elapsed, float duration) noexcept {
    if (duration <= 0.0F) {
        return 1.0F;
    }
    return std::clamp(elapsed / duration, 0.0F, 1.0F);
}

[[nodiscard]] bool is_valid_clip_desc(const AnimationClipDesc& clip) noexcept {
    return is_safe_name(clip.name) && finite(clip.duration_seconds) && clip.duration_seconds > 0.0F;
}

[[nodiscard]] float sample_clip_time(const AnimationClipDesc& clip, float time_seconds) {
    if (!is_valid_clip_desc(clip) || !finite(time_seconds)) {
        throw std::invalid_argument("animation clip sample is invalid");
    }

    if (time_seconds <= 0.0F) {
        return 0.0F;
    }
    if (clip.looping) {
        return std::fmod(time_seconds, clip.duration_seconds);
    }
    return std::min(time_seconds, clip.duration_seconds);
}

[[nodiscard]] float total_blend_tree_weight(const AnimationBlendTreeDesc& desc) noexcept {
    float total = 0.0F;
    for (const auto& child : desc.children) {
        total += child.weight;
    }
    return total;
}

[[nodiscard]] float total_layer_weight(const std::vector<AnimationLayerDesc>& layers) noexcept {
    float total = 0.0F;
    for (const auto& layer : layers) {
        total += layer.weight;
    }
    return total;
}

[[nodiscard]] bool valid_retarget_mode(AnimationRetargetMode mode) noexcept {
    switch (mode) {
    case AnimationRetargetMode::preserve_source:
    case AnimationRetargetMode::scale_to_target:
        return true;
    }
    return false;
}

} // namespace

bool is_valid_animation_state_machine_desc(const AnimationStateMachineDesc& desc) noexcept {
    if (desc.states.empty() || !is_safe_name(desc.initial_state) || has_duplicate_state_names(desc) ||
        !state_exists(desc, desc.initial_state)) {
        return false;
    }

    for (const auto& state : desc.states) {
        if (!is_safe_name(state.name) || !is_valid_clip_desc(state.clip)) {
            return false;
        }
    }

    return std::ranges::all_of(desc.transitions, [&desc](const AnimationTransitionDesc& transition) noexcept {
        return is_safe_name(transition.from_state) && is_safe_name(transition.to_state) &&
               is_safe_name(transition.trigger) && finite(transition.blend_seconds) &&
               transition.blend_seconds >= 0.0F && state_exists(desc, transition.from_state) &&
               state_exists(desc, transition.to_state);
    });
}

bool is_valid_animation_blend_tree_desc(const AnimationBlendTreeDesc& desc) noexcept {
    if (desc.children.empty()) {
        return false;
    }

    float total_weight = 0.0F;
    for (const auto& child : desc.children) {
        if (!is_safe_name(child.name) || !is_valid_clip_desc(child.clip) || !finite(child.weight) ||
            child.weight < 0.0F) {
            return false;
        }
        total_weight += child.weight;
    }
    return total_weight > 0.0F;
}

bool is_valid_animation_blend_1d_desc(const AnimationBlend1DDesc& desc) noexcept {
    if (!is_safe_name(desc.parameter_name) || desc.children.empty()) {
        return false;
    }

    float previous_threshold = -std::numeric_limits<float>::infinity();
    for (const auto& child : desc.children) {
        if (!is_safe_name(child.name) || !is_valid_clip_desc(child.clip) || !finite(child.threshold) ||
            child.threshold <= previous_threshold) {
            return false;
        }
        previous_threshold = child.threshold;
    }
    return true;
}

bool is_valid_animation_layers(const std::vector<AnimationLayerDesc>& layers) noexcept {
    if (layers.empty()) {
        return false;
    }

    float total_weight = 0.0F;
    for (const auto& layer : layers) {
        if (!is_safe_name(layer.name) || !is_valid_animation_blend_tree_desc(layer.blend_tree) ||
            !finite(layer.weight) || layer.weight < 0.0F) {
            return false;
        }
        total_weight += layer.weight;
    }
    return total_weight > 0.0F;
}

bool is_valid_animation_retarget_policy_desc(const AnimationRetargetPolicyDesc& desc) noexcept {
    if (desc.bindings.empty()) {
        return false;
    }

    for (std::size_t index = 0; index < desc.bindings.size(); ++index) {
        const auto& binding = desc.bindings[index];
        if (!is_safe_name(binding.source) || !is_safe_name(binding.target) || !valid_retarget_mode(binding.mode) ||
            !finite(binding.source_reference_length) || !finite(binding.target_reference_length) ||
            binding.source_reference_length <= 0.0F || binding.target_reference_length <= 0.0F) {
            return false;
        }
        for (std::size_t other = index + 1U; other < desc.bindings.size(); ++other) {
            if (binding.source == desc.bindings[other].source || binding.target == desc.bindings[other].target) {
                return false;
            }
        }
    }

    return true;
}

std::vector<AnimationBlendTreeSample> sample_animation_blend_tree(const AnimationBlendTreeDesc& desc,
                                                                  float time_seconds) {
    if (!is_valid_animation_blend_tree_desc(desc) || !finite(time_seconds)) {
        throw std::invalid_argument("animation blend tree description is invalid");
    }

    const auto total_weight = total_blend_tree_weight(desc);
    std::vector<AnimationBlendTreeSample> samples;
    samples.reserve(desc.children.size());
    for (const auto& child : desc.children) {
        if (child.weight == 0.0F) {
            continue;
        }
        samples.push_back(AnimationBlendTreeSample{.name = child.name,
                                                   .clip_name = child.clip.name,
                                                   .time_seconds = sample_clip_time(child.clip, time_seconds),
                                                   .normalized_weight = child.weight / total_weight});
    }
    return samples;
}

std::vector<AnimationBlendTreeSample> sample_animation_blend_1d(const AnimationBlend1DDesc& desc, float parameter_value,
                                                                float time_seconds) {
    if (!is_valid_animation_blend_1d_desc(desc) || !finite(parameter_value) || !finite(time_seconds)) {
        throw std::invalid_argument("animation 1d blend description is invalid");
    }

    const auto make_sample = [time_seconds](const AnimationBlend1DChildDesc& child, float weight) {
        return AnimationBlendTreeSample{.name = child.name,
                                        .clip_name = child.clip.name,
                                        .time_seconds = sample_clip_time(child.clip, time_seconds),
                                        .normalized_weight = weight};
    };

    if (parameter_value <= desc.children.front().threshold) {
        return {make_sample(desc.children.front(), 1.0F)};
    }
    if (parameter_value >= desc.children.back().threshold) {
        return {make_sample(desc.children.back(), 1.0F)};
    }

    for (std::size_t index = 0; index + 1U < desc.children.size(); ++index) {
        const auto& lower = desc.children[index];
        const auto& upper = desc.children[index + 1U];
        if (parameter_value < lower.threshold || parameter_value > upper.threshold) {
            continue;
        }

        const auto range = upper.threshold - lower.threshold;
        const auto alpha = std::clamp(((parameter_value - lower.threshold) / range), 0.0F, 1.0F);
        if (alpha <= 0.000001F) {
            return {make_sample(lower, 1.0F)};
        }
        if (alpha >= 0.999999F) {
            return {make_sample(upper, 1.0F)};
        }
        return {make_sample(lower, 1.0F - alpha), make_sample(upper, alpha)};
    }

    return {make_sample(desc.children.back(), 1.0F)};
}

std::vector<AnimationLayerSample> sample_animation_layers(const std::vector<AnimationLayerDesc>& layers,
                                                          float time_seconds) {
    if (!is_valid_animation_layers(layers) || !finite(time_seconds)) {
        throw std::invalid_argument("animation layer descriptions are invalid");
    }

    const auto total_weight = total_layer_weight(layers);
    std::vector<AnimationLayerSample> samples;
    samples.reserve(layers.size());
    for (const auto& layer : layers) {
        if (layer.weight == 0.0F) {
            continue;
        }
        samples.push_back(AnimationLayerSample{.name = layer.name,
                                               .normalized_weight = layer.weight / total_weight,
                                               .additive = layer.additive,
                                               .clips = sample_animation_blend_tree(layer.blend_tree, time_seconds)});
    }
    return samples;
}

std::vector<AnimationRetargetSample> evaluate_animation_retarget_policy(const AnimationRetargetPolicyDesc& desc) {
    if (!is_valid_animation_retarget_policy_desc(desc)) {
        throw std::invalid_argument("animation retarget policy description is invalid");
    }

    std::vector<AnimationRetargetSample> samples;
    samples.reserve(desc.bindings.size());
    for (const auto& binding : desc.bindings) {
        const auto scale = binding.mode == AnimationRetargetMode::scale_to_target
                               ? binding.target_reference_length / binding.source_reference_length
                               : 1.0F;
        samples.push_back(AnimationRetargetSample{
            .source = binding.source, .target = binding.target, .mode = binding.mode, .scale = scale});
    }
    return samples;
}

AnimationStateMachine::AnimationStateMachine(AnimationStateMachineDesc desc) : desc_(std::move(desc)) {
    if (!is_valid_animation_state_machine_desc(desc_)) {
        throw std::invalid_argument("animation state machine description is invalid");
    }
    active_state_index_ = state_index(desc_.initial_state);
}

AnimationStateMachineSample AnimationStateMachine::sample() const {
    if (!blending_) {
        return AnimationStateMachineSample{.from_state = std::string{},
                                           .to_state = desc_.states[active_state_index_].name,
                                           .from_time_seconds = 0.0F,
                                           .to_time_seconds = active_time_seconds_,
                                           .blend_alpha = 1.0F,
                                           .blending = false};
    }

    return AnimationStateMachineSample{.from_state = desc_.states[from_state_index_].name,
                                       .to_state = desc_.states[active_state_index_].name,
                                       .from_time_seconds = from_time_seconds_,
                                       .to_time_seconds = active_time_seconds_,
                                       .blend_alpha =
                                           clamp_blend_alpha(blend_elapsed_seconds_, blend_duration_seconds_),
                                       .blending = true};
}

std::string_view AnimationStateMachine::active_state() const noexcept {
    return desc_.states[active_state_index_].name;
}

float AnimationStateMachine::active_time_seconds() const noexcept {
    return active_time_seconds_;
}

bool AnimationStateMachine::is_blending() const noexcept {
    return blending_;
}

bool AnimationStateMachine::trigger(std::string_view trigger_name) {
    if (!is_safe_name(trigger_name)) {
        return false;
    }

    const auto& active_name = desc_.states[active_state_index_].name;
    for (const auto& transition : desc_.transitions) {
        if (transition.from_state != active_name || transition.trigger != trigger_name) {
            continue;
        }

        const auto target_index = state_index(transition.to_state);
        if (target_index == invalid_state_index) {
            return false;
        }

        from_state_index_ = active_state_index_;
        from_time_seconds_ = active_time_seconds_;
        active_state_index_ = target_index;
        active_time_seconds_ = 0.0F;
        blend_elapsed_seconds_ = 0.0F;
        blend_duration_seconds_ = transition.blend_seconds;
        blending_ = blend_duration_seconds_ > 0.0F;
        return true;
    }

    return false;
}

void AnimationStateMachine::update(float delta_seconds) {
    if (!finite(delta_seconds) || delta_seconds < 0.0F) {
        throw std::invalid_argument("animation delta seconds is invalid");
    }

    active_time_seconds_ = advance_clip_time(active_state_index_, active_time_seconds_, delta_seconds);
    if (!blending_) {
        return;
    }

    from_time_seconds_ = advance_clip_time(from_state_index_, from_time_seconds_, delta_seconds);
    blend_elapsed_seconds_ += delta_seconds;
    if (blend_elapsed_seconds_ >= blend_duration_seconds_) {
        blending_ = false;
        blend_elapsed_seconds_ = blend_duration_seconds_;
    }
}

std::size_t AnimationStateMachine::state_index(std::string_view state_name) const noexcept {
    for (std::size_t index = 0; index < desc_.states.size(); ++index) {
        if (desc_.states[index].name == state_name) {
            return index;
        }
    }
    return invalid_state_index;
}

float AnimationStateMachine::advance_clip_time(std::size_t state_index, float time_seconds, float delta_seconds) const {
    const auto& clip = desc_.states[state_index].clip;
    const auto advanced = time_seconds + delta_seconds;
    if (clip.looping) {
        return std::fmod(advanced, clip.duration_seconds);
    }
    return std::min(advanced, clip.duration_seconds);
}

} // namespace mirakana
