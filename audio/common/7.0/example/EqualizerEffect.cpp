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

#include <limits>

#define LOG_TAG "EffectsFactory7.0"
#include <log/log.h>

#include "EqualizerEffect.h"

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using namespace ::android::hardware::audio::common::V7_0;

namespace android::hardware::audio::effect::V7_0::implementation {

const EffectDescriptor& EqualizerEffect::getDescriptor() {
    // Note: for VTS tests only 'type' and 'uuid' fields are required.
    // The actual implementation must provide meaningful values
    // for all fields of the descriptor.
    static const EffectDescriptor descriptor = {
            .type =
                    {// Same UUID as AudioEffect.EFFECT_TYPE_EQUALIZER in Java.
                     0x0bed4300, 0xddd6, 0x11db, 0x8f34,
                     std::array<uint8_t, 6>{{0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}},
            .uuid = {0, 0, 0, 1, std::array<uint8_t, 6>{{0, 0, 0, 0, 0, 0}}}};
    return descriptor;
}

EqualizerEffect::EqualizerEffect() : mEffect(new Effect(getDescriptor())) {
    mProperties.bandLevels.resize(kNumBands);
}

Return<void> EqualizerEffect::getNumBands(getNumBands_cb _hidl_cb) {
    _hidl_cb(Result::OK, kNumBands);
    return Void();
}

Return<void> EqualizerEffect::getLevelRange(getLevelRange_cb _hidl_cb) {
    _hidl_cb(Result::OK, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max());
    return Void();
}

Return<Result> EqualizerEffect::setBandLevel(uint16_t band, int16_t level) {
    if (band < kNumBands) {
        mProperties.bandLevels[band] = level;
        return Result::OK;
    } else {
        return Result::INVALID_ARGUMENTS;
    }
}

Return<void> EqualizerEffect::getBandLevel(uint16_t band, getBandLevel_cb _hidl_cb) {
    if (band < kNumBands) {
        _hidl_cb(Result::OK, mProperties.bandLevels[band]);
    } else {
        _hidl_cb(Result::INVALID_ARGUMENTS, 0);
    }
    return Void();
}

Return<void> EqualizerEffect::getBandCenterFrequency(uint16_t band,
                                                     getBandCenterFrequency_cb _hidl_cb) {
    (void)band;
    _hidl_cb(Result::OK, 0);
    return Void();
}

Return<void> EqualizerEffect::getBandFrequencyRange(uint16_t band,
                                                    getBandFrequencyRange_cb _hidl_cb) {
    (void)band;
    _hidl_cb(Result::OK, 0, 1);
    return Void();
}

Return<void> EqualizerEffect::getBandForFrequency(uint32_t freq, getBandForFrequency_cb _hidl_cb) {
    (void)freq;
    _hidl_cb(Result::OK, 0);
    return Void();
}

Return<void> EqualizerEffect::getPresetNames(getPresetNames_cb _hidl_cb) {
    hidl_vec<hidl_string> presetNames;
    presetNames.resize(kNumPresets);
    presetNames[0] = "default";
    _hidl_cb(Result::OK, presetNames);
    return Void();
}

Return<Result> EqualizerEffect::setCurrentPreset(uint16_t preset) {
    if (preset < kNumPresets) {
        mProperties.curPreset = preset;
        return Result::OK;
    } else {
        return Result::INVALID_ARGUMENTS;
    }
}

Return<void> EqualizerEffect::getCurrentPreset(getCurrentPreset_cb _hidl_cb) {
    _hidl_cb(Result::OK, mProperties.curPreset);
    return Void();
}

Return<Result> EqualizerEffect::setAllProperties(
        const IEqualizerEffect::AllProperties& properties) {
    mProperties = properties;
    return Result::OK;
}

Return<void> EqualizerEffect::getAllProperties(getAllProperties_cb _hidl_cb) {
    _hidl_cb(Result::OK, mProperties);
    return Void();
}

}  // namespace android::hardware::audio::effect::V7_0::implementation
