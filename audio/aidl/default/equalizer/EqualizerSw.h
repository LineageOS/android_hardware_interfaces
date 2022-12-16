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
            } else {
                mBandLevels[it.index] = it.levelMb;
            }
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
    static const std::string kEffectName;
    static const std::vector<Equalizer::BandFrequency> kBandFrequency;
    static const std::vector<Equalizer::Preset> kPresets;
    static const Equalizer::Capability kEqCap;
    static const Descriptor kDesc;

    EqualizerSw() { LOG(DEBUG) << __func__; }
    ~EqualizerSw() {
        cleanUp();
        LOG(DEBUG) << __func__;
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;

    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common) override;
    std::shared_ptr<EffectContext> getContext() override;
    RetCode releaseContext() override;

    IEffect::Status effectProcessImpl(float* in, float* out, int samples) override;
    std::string getEffectName() override { return kEffectName; }

  private:
    ndk::ScopedAStatus getParameterEqualizer(const Equalizer::Tag& tag,
                                             Parameter::Specific* specific);
    std::shared_ptr<EqualizerSwContext> mContext;
};

}  // namespace aidl::android::hardware::audio::effect
