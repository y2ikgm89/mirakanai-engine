// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct AnimationClipDesc {
    std::string name;
    float duration_seconds{0.0F};
    bool looping{false};
};

struct AnimationStateDesc {
    std::string name;
    AnimationClipDesc clip;
};

struct AnimationTransitionDesc {
    std::string from_state;
    std::string to_state;
    std::string trigger;
    float blend_seconds{0.0F};
};

struct AnimationStateMachineDesc {
    std::vector<AnimationStateDesc> states;
    std::string initial_state;
    std::vector<AnimationTransitionDesc> transitions;
};

struct AnimationStateMachineSample {
    std::string from_state;
    std::string to_state;
    float from_time_seconds{0.0F};
    float to_time_seconds{0.0F};
    float blend_alpha{1.0F};
    bool blending{false};
};

struct AnimationBlendTreeChildDesc {
    std::string name;
    AnimationClipDesc clip;
    float weight{1.0F};
};

struct AnimationBlendTreeDesc {
    std::vector<AnimationBlendTreeChildDesc> children;
};

struct AnimationBlendTreeSample {
    std::string name;
    std::string clip_name;
    float time_seconds{0.0F};
    float normalized_weight{0.0F};
};

struct AnimationBlend1DChildDesc {
    std::string name;
    AnimationClipDesc clip;
    float threshold{0.0F};
};

struct AnimationBlend1DDesc {
    std::string parameter_name;
    std::vector<AnimationBlend1DChildDesc> children;
};

struct AnimationLayerDesc {
    std::string name;
    AnimationBlendTreeDesc blend_tree;
    float weight{1.0F};
    bool additive{false};
};

struct AnimationLayerSample {
    std::string name;
    float normalized_weight{0.0F};
    bool additive{false};
    std::vector<AnimationBlendTreeSample> clips;
};

enum class AnimationRetargetMode : std::uint8_t { preserve_source, scale_to_target };

struct AnimationRetargetBindingDesc {
    std::string source;
    std::string target;
    AnimationRetargetMode mode{AnimationRetargetMode::preserve_source};
    float source_reference_length{1.0F};
    float target_reference_length{1.0F};
};

struct AnimationRetargetPolicyDesc {
    std::vector<AnimationRetargetBindingDesc> bindings;
};

struct AnimationRetargetSample {
    std::string source;
    std::string target;
    AnimationRetargetMode mode{AnimationRetargetMode::preserve_source};
    float scale{1.0F};
};

[[nodiscard]] bool is_valid_animation_state_machine_desc(const AnimationStateMachineDesc& desc) noexcept;
[[nodiscard]] bool is_valid_animation_blend_tree_desc(const AnimationBlendTreeDesc& desc) noexcept;
[[nodiscard]] bool is_valid_animation_blend_1d_desc(const AnimationBlend1DDesc& desc) noexcept;
[[nodiscard]] bool is_valid_animation_layers(const std::vector<AnimationLayerDesc>& layers) noexcept;
[[nodiscard]] bool is_valid_animation_retarget_policy_desc(const AnimationRetargetPolicyDesc& desc) noexcept;

[[nodiscard]] std::vector<AnimationBlendTreeSample> sample_animation_blend_tree(const AnimationBlendTreeDesc& desc,
                                                                                float time_seconds);
[[nodiscard]] std::vector<AnimationBlendTreeSample>
sample_animation_blend_1d(const AnimationBlend1DDesc& desc, float parameter_value, float time_seconds);
[[nodiscard]] std::vector<AnimationLayerSample> sample_animation_layers(const std::vector<AnimationLayerDesc>& layers,
                                                                        float time_seconds);
[[nodiscard]] std::vector<AnimationRetargetSample>
evaluate_animation_retarget_policy(const AnimationRetargetPolicyDesc& desc);

class AnimationStateMachine {
  public:
    explicit AnimationStateMachine(AnimationStateMachineDesc desc);

    [[nodiscard]] AnimationStateMachineSample sample() const;
    [[nodiscard]] std::string_view active_state() const noexcept;
    [[nodiscard]] float active_time_seconds() const noexcept;
    [[nodiscard]] bool is_blending() const noexcept;

    [[nodiscard]] bool trigger(std::string_view trigger_name);
    void update(float delta_seconds);

  private:
    [[nodiscard]] std::size_t state_index(std::string_view state_name) const noexcept;
    [[nodiscard]] float advance_clip_time(std::size_t state_index, float time_seconds, float delta_seconds) const;

    AnimationStateMachineDesc desc_;
    std::size_t active_state_index_{0};
    float active_time_seconds_{0.0F};
    bool blending_{false};
    std::size_t from_state_index_{0};
    float from_time_seconds_{0.0F};
    float blend_elapsed_seconds_{0.0F};
    float blend_duration_seconds_{0.0F};
};

} // namespace mirakana
