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

namespace aidl::android::hardware::audio::effect {

class EqualizerSwContext final : public EffectContext {
  public:
    EqualizerSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
    }

    RetCode setEqPreset(const int& presetIdx) {
        if (presetIdx < 0 || presetIdx >= kMaxPresetNumber) {
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        mPreset = presetIdx;
        return RetCode::SUCCESS;
    }
    int getEqPreset() { return mPreset; }

    RetCode setEqBandLevels(const std::vector<Equalizer::BandLevel>& bandLevels) {
        if (bandLevels.size() > kMaxBandNumber) {
            LOG(ERROR) << __func__ << " return because size exceed " << kMaxBandNumber;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        RetCode ret = RetCode::SUCCESS;
        for (auto& it : bandLevels) {
            if (it.index >= kMaxBandNumber || it.index < 0) {
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
        for (int i = 0; i < kMaxBandNumber; i++) {
            bandLevels.push_back({i, mBandLevels[i]});
        }
        return bandLevels;
    }

    std::vector<int> getCenterFreqs() {
        return {std::begin(kPresetsFrequencies), std::end(kPresetsFrequencies)};
    }
    static const int kMaxBandNumber = 5;
    static const int kMaxPresetNumber = 10;
    static const int kCustomPreset = -1;

  private:
    static constexpr std::array<uint16_t, kMaxBandNumber> kPresetsFrequencies = {60, 230, 910, 3600,
                                                                                 14000};
    // preset band level
    int mPreset = kCustomPreset;
    int32_t mBandLevels[kMaxBandNumber] = {3, 0, 0, 0, 3};

    // Add equalizer specific context for processing here
};

class EqualizerSw final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const Capability kEqCap;
    static const Descriptor kDesc;

    EqualizerSw() { LOG(DEBUG) << __func__; }
    ~EqualizerSw() {
        cleanUp();
        LOG(DEBUG) << __func__;
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific)
            REQUIRES(mImplMutex) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id, Parameter::Specific* specific)
            REQUIRES(mImplMutex) override;

    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common)
            REQUIRES(mImplMutex) override;
    RetCode releaseContext() REQUIRES(mImplMutex) override;

    IEffect::Status effectProcessImpl(float* in, float* out, int samples)
            REQUIRES(mImplMutex) override;
    std::string getEffectName() override { return kEffectName; }

  private:
    static const std::vector<Equalizer::BandFrequency> kBandFrequency;
    static const std::vector<Equalizer::Preset> kPresets;
    static const std::vector<Range::EqualizerRange> kRanges;
    ndk::ScopedAStatus getParameterEqualizer(const Equalizer::Tag& tag,
                                             Parameter::Specific* specific) REQUIRES(mImplMutex);
    std::shared_ptr<EqualizerSwContext> mContext;
};

}  // namespace aidl::android::hardware::audio::effect
