/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

// clang-format off
#include PATH(android/hardware/audio/effect/FILE_VERSION/types.h)
// clang-format on

#include <system/audio_effect.h>

namespace android {
namespace hardware {
namespace audio {
namespace effect {
namespace CPP_VERSION {
namespace implementation {

using namespace ::android::hardware::audio::effect::CPP_VERSION;

struct EffectUtils {
    static status_t effectBufferConfigFromHal(const buffer_config_t& halConfig, bool isInput,
                                              EffectBufferConfig* config);
    static status_t effectBufferConfigToHal(const EffectBufferConfig& config,
                                            buffer_config_t* halConfig);
    static status_t effectConfigFromHal(const effect_config_t& halConfig, bool isInput,
                                        EffectConfig* config);
    static status_t effectConfigToHal(const EffectConfig& config, effect_config_t* halConfig);
    static status_t effectDescriptorFromHal(const effect_descriptor_t& halDescriptor,
                                            EffectDescriptor* descriptor);
    static status_t effectDescriptorToHal(const EffectDescriptor& descriptor,
                                          effect_descriptor_t* halDescriptor);
};

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace effect
}  // namespace audio
}  // namespace hardware
}  // namespace android
