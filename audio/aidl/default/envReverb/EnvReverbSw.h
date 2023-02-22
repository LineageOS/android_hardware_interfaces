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

class EnvReverbSwContext final : public EffectContext {
  public:
    EnvReverbSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
    }

    RetCode setErRoomLevel(int roomLevel);
    int getErRoomLevel() const { return mRoomLevel; }

    RetCode setErRoomHfLevel(int roomHfLevel);
    int getErRoomHfLevel() const { return mRoomHfLevel; }

    RetCode setErDecayTime(int decayTime);
    int getErDecayTime() const { return mDecayTime; }

    RetCode setErDecayHfRatio(int decayHfRatio);
    int getErDecayHfRatio() const { return mDecayHfRatio; }

    RetCode setErLevel(int level);
    int getErLevel() const { return mLevel; }

    RetCode setErDelay(int delay);
    int getErDelay() const { return mDelay; }

    RetCode setErDiffusion(int diffusion);
    int getErDiffusion() const { return mDiffusion; }

    RetCode setErDensity(int density);
    int getErDensity() const { return mDensity; }

    RetCode setErBypass(bool bypass) {
        mBypass = bypass;
        return RetCode::SUCCESS;
    }
    bool getErBypass() const { return mBypass; }

    RetCode setErReflectionsDelay(int delay) {
        mReflectionsDelayMs = delay;
        return RetCode::SUCCESS;
    }
    bool getErReflectionsDelay() const { return mReflectionsDelayMs; }

    RetCode setErReflectionsLevel(int level) {
        mReflectionsLevelMb = level;
        return RetCode::SUCCESS;
    }
    bool getErReflectionsLevel() const { return mReflectionsLevelMb; }

  private:
    int mRoomLevel = -6000;                                        // Default room level
    int mRoomHfLevel = 0;                                          // Default room hf level
    int mDecayTime = 1000;                                         // Default decay time
    int mDecayHfRatio = 500;                                       // Default decay hf ratio
    int mLevel = -6000;                                            // Default level
    int mDelay = 40;                                               // Default delay
    int mReflectionsLevelMb = 0;
    int mReflectionsDelayMs = 0;
    int mDiffusion = 1000;                                         // Default diffusion
    int mDensity = 1000;                                           // Default density
    bool mBypass = false;                                          // Default bypass
};

class EnvReverbSw final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const Capability kCapability;
    static const Descriptor kDescriptor;
    EnvReverbSw() { LOG(DEBUG) << __func__; }
    ~EnvReverbSw() {
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
    static const std::vector<Range::EnvironmentalReverbRange> kRanges;
    std::shared_ptr<EnvReverbSwContext> mContext;
    ndk::ScopedAStatus getParameterEnvironmentalReverb(const EnvironmentalReverb::Tag& tag,
                                                       Parameter::Specific* specific);
};
}  // namespace aidl::android::hardware::audio::effect
