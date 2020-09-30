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

#include "LoudnessEnhancerEffect.h"

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using namespace ::android::hardware::audio::common::V7_0;

namespace android::hardware::audio::effect::V7_0::implementation {

const EffectDescriptor& LoudnessEnhancerEffect::getDescriptor() {
    // Note: for VTS tests only 'type' and 'uuid' fields are required.
    // The actual implementation must provide meaningful values
    // for all fields of the descriptor.
    static const EffectDescriptor descriptor = {
            .type =
                    {// Same UUID as AudioEffect.EFFECT_TYPE_LOUDNESS_ENHANCER in Java.
                     0xfe3199be, 0xaed0, 0x413f, 0x87bb,
                     std::array<uint8_t, 6>{{0x11, 0x26, 0x0e, 0xb6, 0x3c, 0xf1}}},
            .uuid = {0, 0, 0, 2, std::array<uint8_t, 6>{{0, 0, 0, 0, 0, 0}}}};
    return descriptor;
}  // namespace android::hardware::audio::effect::V7_0::implementation

LoudnessEnhancerEffect::LoudnessEnhancerEffect() : mEffect(new Effect(getDescriptor())) {}

Return<Result> LoudnessEnhancerEffect::setTargetGain(int32_t targetGainMb) {
    mTargetGainMb = targetGainMb;
    return Result::OK;
}

Return<void> LoudnessEnhancerEffect::getTargetGain(getTargetGain_cb _hidl_cb) {
    _hidl_cb(Result::OK, mTargetGainMb);
    return Void();
}

}  // namespace android::hardware::audio::effect::V7_0::implementation
