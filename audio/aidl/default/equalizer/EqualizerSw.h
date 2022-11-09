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

#pragma once

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <fmq/AidlMessageQueue.h>
#include <cstdlib>
#include <memory>

#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectUUID.h"

namespace aidl::android::hardware::audio::effect {

class EqualizerSwContext final : public EffectContext {
  public:
    EqualizerSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
    }

    RetCode setEqPreset(const int& presetIdx) {
        if (presetIdx < 0 || presetIdx >= NUM_OF_PRESETS) {
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        mPreset = presetIdx;
        return RetCode::SUCCESS;
    }
    int getEqPreset() { return mPreset; }

    RetCode setEqBandLevels(const std::vector<Equalizer::BandLevel>& bandLevels) {
        if (bandLevels.size() > NUM_OF_BANDS) {
            LOG(ERROR) << __func__ << " return because size exceed " << NUM_OF_BANDS;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        RetCode ret = RetCode::SUCCESS;
        for (auto& it : bandLevels) {
            if (it.index >= NUM_OF_BANDS || it.index < 0) {
                LOG(ERROR) << __func__ << " index illegal, skip: " << it.index << " - "
                           << it.levelMb;
                ret = RetCode::ERROR_ILLEGAL_PARAMETER;
            }
            mBandLevels[it.index] = it.levelMb;
        }
        return ret;
    }

    std::vector<Equalizer::BandLevel> getEqBandLevels() {
        std::vector<Equalizer::BandLevel> bandLevels;
        for (int i = 0; i < NUM_OF_BANDS; i++) {
            bandLevels.push_back({i, mBandLevels[i]});
        }
        return bandLevels;
    }

  private:
    static const int NUM_OF_BANDS = 5;
    static const int NUM_OF_PRESETS = 10;
    static const int PRESET_CUSTOM = -1;
    // preset band level
    int mPreset = PRESET_CUSTOM;
    int32_t mBandLevels[NUM_OF_BANDS] = {3, 0, 0, 0, 3};

    // Add equalizer specific context for processing here
};

class EqualizerSw final : public EffectImpl {
  public:
    EqualizerSw() { LOG(DEBUG) << __func__; }
    ~EqualizerSw() {
        LOG(DEBUG) << __func__;
        releaseContext();
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;
    IEffect::Status effectProcessImpl(float* in, float* out, int process) override;
    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common) override;
    RetCode releaseContext() override;

  private:
    std::shared_ptr<EqualizerSwContext> mContext;
    /* capabilities */
    const std::vector<Equalizer::BandFrequency> mBandFrequency = {{0, 30000, 120000},
                                                                  {1, 120001, 460000},
                                                                  {2, 460001, 1800000},
                                                                  {3, 1800001, 7000000},
                                                                  {4, 7000001, 20000000}};
    // presets supported by the device
    const std::vector<Equalizer::Preset> mPresets = {
            {0, "Normal"},      {1, "Classical"}, {2, "Dance"}, {3, "Flat"}, {4, "Folk"},
            {5, "Heavy Metal"}, {6, "Hip Hop"},   {7, "Jazz"},  {8, "Pop"},  {9, "Rock"}};

    const Equalizer::Capability kEqCap = {.bandFrequencies = mBandFrequency, .presets = mPresets};
    // Effect descriptor.
    const Descriptor kDesc = {.common = {.id = {.type = EqualizerTypeUUID,
                                                .uuid = EqualizerSwImplUUID,
                                                .proxy = std::nullopt},
                                         .flags = {.type = Flags::Type::INSERT,
                                                   .insert = Flags::Insert::FIRST,
                                                   .volume = Flags::Volume::CTRL},
                                         .name = "EqualizerSw"},
                              .capability = Capability::make<Capability::equalizer>(kEqCap)};

    ndk::ScopedAStatus getParameterEqualizer(const Equalizer::Tag& tag,
                                             Parameter::Specific* specific);
};
}  // namespace aidl::android::hardware::audio::effect
