/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "EffectsFactory7.0"
#include <log/log.h>

#include "EffectsFactory.h"
#include "EqualizerEffect.h"
#include "LoudnessEnhancerEffect.h"

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using namespace ::android::hardware::audio::common::V7_0;

namespace android::hardware::audio::effect::V7_0::implementation {

Return<void> EffectsFactory::getAllDescriptors(getAllDescriptors_cb _hidl_cb) {
    hidl_vec<EffectDescriptor> descriptors;
    descriptors.resize(2);
    descriptors[0] = EqualizerEffect::getDescriptor();
    descriptors[1] = LoudnessEnhancerEffect::getDescriptor();
    _hidl_cb(Result::OK, descriptors);
    return Void();
}

Return<void> EffectsFactory::getDescriptor(const Uuid& uuid, getDescriptor_cb _hidl_cb) {
    if (auto desc = EqualizerEffect::getDescriptor(); uuid == desc.type || uuid == desc.uuid) {
        _hidl_cb(Result::OK, desc);
    } else if (auto desc = LoudnessEnhancerEffect::getDescriptor();
               uuid == desc.type || uuid == desc.uuid) {
        _hidl_cb(Result::OK, desc);
    } else {
        _hidl_cb(Result::INVALID_ARGUMENTS, EffectDescriptor{});
    }
    return Void();
}

Return<void> EffectsFactory::createEffect(const Uuid& uuid, int32_t session, int32_t ioHandle,
                                          int32_t device, createEffect_cb _hidl_cb) {
    (void)session;
    (void)ioHandle;
    (void)device;
    if (auto desc = EqualizerEffect::getDescriptor(); uuid == desc.type || uuid == desc.uuid) {
        _hidl_cb(Result::OK, new EqualizerEffect(), 0);
    } else if (auto desc = LoudnessEnhancerEffect::getDescriptor();
               uuid == desc.type || uuid == desc.uuid) {
        _hidl_cb(Result::OK, new LoudnessEnhancerEffect(), 0);
    } else {
        _hidl_cb(Result::INVALID_ARGUMENTS, nullptr, 0);
    }
    return Void();
}

Return<void> EffectsFactory::debug(const hidl_handle& fd, const hidl_vec<hidl_string>& options) {
    (void)fd;
    (void)options;
    return Void();
}

}  // namespace android::hardware::audio::effect::V7_0::implementation
