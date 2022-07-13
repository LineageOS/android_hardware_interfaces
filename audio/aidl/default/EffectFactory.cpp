/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "AHAL_EffectFactory"
#include <android-base/logging.h>

#include "effectFactory-impl/EffectFactory.h"
#include "equalizer-impl/Equalizer.h"
#include "visualizer-impl/Visualizer.h"

using aidl::android::media::audio::common::AudioUuid;

namespace aidl::android::hardware::audio::effect {

Factory::Factory() {
    // TODO: implement this with xml parser on audio_effect.xml, and filter with optional
    // parameters.
    Descriptor::Identity id;
    id.type = {static_cast<int32_t>(0x0bed4300),
               0xddd6,
               0x11db,
               0x8f34,
               {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};
    id.uuid = EqualizerUUID;
    mIdentityList.push_back(id);
    id.type = {static_cast<int32_t>(0xd3467faa),
               0xacc7,
               0x4d34,
               0xacaf,
               {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};
    id.uuid = VisualizerUUID;
    mIdentityList.push_back(id);
}

ndk::ScopedAStatus Factory::queryEffects(const std::optional<AudioUuid>& in_type,
                                         const std::optional<AudioUuid>& in_instance,
                                         std::vector<Descriptor::Identity>* _aidl_return) {
    std::copy_if(mIdentityList.begin(), mIdentityList.end(), std::back_inserter(*_aidl_return),
                 [&](auto& desc) {
                     return (!in_type.has_value() || in_type.value() == desc.type) &&
                            (!in_instance.has_value() || in_instance.value() == desc.uuid);
                 });
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::audio::effect
