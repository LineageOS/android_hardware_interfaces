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

    RetCode setErRoomLevel(int roomLevel) {
        if (roomLevel < EnvironmentalReverb::MIN_ROOM_LEVEL_MB ||
            roomLevel > EnvironmentalReverb::MAX_ROOM_LEVEL_MB) {
            LOG(ERROR) << __func__ << " invalid roomLevel: " << roomLevel;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new room level
        mRoomLevel = roomLevel;
        return RetCode::SUCCESS;
    }
    int getErRoomLevel() const { return mRoomLevel; }

    RetCode setErRoomHfLevel(int roomHfLevel) {
        if (roomHfLevel < EnvironmentalReverb::MIN_ROOM_HF_LEVEL_MB ||
            roomHfLevel > EnvironmentalReverb::MAX_ROOM_HF_LEVEL_MB) {
            LOG(ERROR) << __func__ << " invalid roomHfLevel: " << roomHfLevel;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new room HF level
        mRoomHfLevel = roomHfLevel;
        return RetCode::SUCCESS;
    }
    int getErRoomHfLevel() const { return mRoomHfLevel; }

    RetCode setErDecayTime(int decayTime) {
        if (decayTime < EnvironmentalReverb::MIN_DECAY_TIME_MS ||
            decayTime > EnvironmentalReverb::MAX_DECAY_TIME_MS) {
            LOG(ERROR) << __func__ << " invalid decayTime: " << decayTime;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new decay time
        mDecayTime = decayTime;
        return RetCode::SUCCESS;
    }
    int getErDecayTime() const { return mDecayTime; }

    RetCode setErDecayHfRatio(int decayHfRatio) {
        if (decayHfRatio < EnvironmentalReverb::MIN_DECAY_HF_RATIO_PM ||
            decayHfRatio > EnvironmentalReverb::MAX_DECAY_HF_RATIO_PM) {
            LOG(ERROR) << __func__ << " invalid decayHfRatio: " << decayHfRatio;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new decay HF ratio
        mDecayHfRatio = decayHfRatio;
        return RetCode::SUCCESS;
    }
    int getErDecayHfRatio() const { return mDecayHfRatio; }

    RetCode setErLevel(int level) {
        if (level < EnvironmentalReverb::MIN_LEVEL_MB ||
            level > EnvironmentalReverb::MAX_LEVEL_MB) {
            LOG(ERROR) << __func__ << " invalid level: " << level;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new level
        mLevel = level;
        return RetCode::SUCCESS;
    }
    int getErLevel() const { return mLevel; }

    RetCode setErDelay(int delay) {
        if (delay < EnvironmentalReverb::MIN_DELAY_MS ||
            delay > EnvironmentalReverb::MAX_DELAY_MS) {
            LOG(ERROR) << __func__ << " invalid delay: " << delay;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new delay
        mDelay = delay;
        return RetCode::SUCCESS;
    }
    int getErDelay() const { return mDelay; }

    RetCode setErDiffusion(int diffusion) {
        if (diffusion < EnvironmentalReverb::MIN_DIFFUSION_PM ||
            diffusion > EnvironmentalReverb::MAX_DIFFUSION_PM) {
            LOG(ERROR) << __func__ << " invalid diffusion: " << diffusion;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new diffusion
        mDiffusion = diffusion;
        return RetCode::SUCCESS;
    }
    int getErDiffusion() const { return mDiffusion; }

    RetCode setErDensity(int density) {
        if (density < EnvironmentalReverb::MIN_DENSITY_PM ||
            density > EnvironmentalReverb::MAX_DENSITY_PM) {
            LOG(ERROR) << __func__ << " invalid density: " << density;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new density
        mDensity = density;
        return RetCode::SUCCESS;
    }
    int getErDensity() const { return mDensity; }

    RetCode setErBypass(bool bypass) {
        // TODO : Add implementation to apply new bypass
        mBypass = bypass;
        return RetCode::SUCCESS;
    }
    bool getErBypass() const { return mBypass; }

  private:
    int mRoomLevel = EnvironmentalReverb::MIN_ROOM_LEVEL_MB;       // Default room level
    int mRoomHfLevel = EnvironmentalReverb::MAX_ROOM_HF_LEVEL_MB;  // Default room hf level
    int mDecayTime = 1000;                                         // Default decay time
    int mDecayHfRatio = 500;                                       // Default decay hf ratio
    int mLevel = EnvironmentalReverb::MIN_LEVEL_MB;                // Default level
    int mDelay = 40;                                               // Default delay
    int mDiffusion = EnvironmentalReverb::MAX_DIFFUSION_PM;        // Default diffusion
    int mDensity = EnvironmentalReverb::MAX_DENSITY_PM;            // Default density
    bool mBypass = false;                                          // Default bypass
};

class EnvReverbSw final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const EnvironmentalReverb::Capability kCapability;
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
    std::shared_ptr<EnvReverbSwContext> mContext;
    ndk::ScopedAStatus getParameterEnvironmentalReverb(const EnvironmentalReverb::Tag& tag,
                                                       Parameter::Specific* specific);
};
}  // namespace aidl::android::hardware::audio::effect
