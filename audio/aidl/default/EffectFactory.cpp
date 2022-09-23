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
    id.type = EqualizerTypeUUID;
    id.uuid = EqualizerSwImplUUID;
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

ndk::ScopedAStatus Factory::createEffect(
        const AudioUuid& in_impl_uuid,
        std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>* _aidl_return) {
    LOG(DEBUG) << __func__ << ": UUID " << in_impl_uuid.toString();
    if (in_impl_uuid == EqualizerSwImplUUID) {
        *_aidl_return = ndk::SharedRefBase::make<Equalizer>();
    } else {
        LOG(ERROR) << __func__ << ": UUID "
                   << " not supported";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Factory::destroyEffect(
        const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_handle) {
    if (in_handle) {
        // TODO: b/245393900 need check the instance state with IEffect.getState before destroy.
        return ndk::ScopedAStatus::ok();
    } else {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
}

}  // namespace aidl::android::hardware::audio::effect
