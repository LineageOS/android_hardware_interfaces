/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_AUDIO_EFFECT_EFFECTSFACTORY_H
#define ANDROID_HARDWARE_AUDIO_EFFECT_EFFECTSFACTORY_H

#include <system/audio_effect.h>

#include PATH(android/hardware/audio/effect/FILE_VERSION/IEffectsFactory.h)

#include <hidl/MQDescriptor.h>

#include <hardware/audio_effect.h>
#include <system/audio_effect.h>

#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace audio {
namespace effect {
namespace CPP_VERSION {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using namespace ::android::hardware::audio::common::CPP_VERSION;
using namespace ::android::hardware::audio::effect::CPP_VERSION;

struct EffectsFactory : public IEffectsFactory {
    // Methods from ::android::hardware::audio::effect::CPP_VERSION::IEffectsFactory follow.
    Return<void> getAllDescriptors(getAllDescriptors_cb _hidl_cb) override;
    Return<void> getDescriptor(const Uuid& uid, getDescriptor_cb _hidl_cb) override;
    Return<void> createEffect(const Uuid& uid, int32_t session, int32_t ioHandle,
                              createEffect_cb _hidl_cb) override;
    Return<void> debugDump(
        const hidl_handle& fd);  //< in CPP_VERSION::IEffectsFactory only, alias of debug
    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>& options) override;

   private:
    static sp<IEffect> dispatchEffectInstanceCreation(const effect_descriptor_t& halDescriptor,
                                                      effect_handle_t handle);
};

extern "C" IEffectsFactory* HIDL_FETCH_IEffectsFactory(const char* name);

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace effect
}  // namespace audio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_AUDIO_EFFECT_EFFECTSFACTORY_H
