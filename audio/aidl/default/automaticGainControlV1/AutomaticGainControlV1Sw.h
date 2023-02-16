/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectUUID.h"

namespace aidl::android::hardware::audio::effect {

class AutomaticGainControlV1SwContext final : public EffectContext {
  public:
    AutomaticGainControlV1SwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
    }

    RetCode setTargetPeakLevel(int targetPeakLevel);
    int getTargetPeakLevel();
    RetCode setMaxCompressionGain(int maxCompressionGainDb);
    int getMaxCompressionGain();
    RetCode setEnableLimiter(bool enableLimiter);
    bool getEnableLimiter();

  private:
    int mTargetPeakLevel = 0;
    int mMaxCompressionGain = 0;
    bool mEnableLimiter = false;
};

class AutomaticGainControlV1Sw final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const bool kStrengthSupported;
    static const Capability kCapability;
    static const Descriptor kDescriptor;
    AutomaticGainControlV1Sw() { LOG(DEBUG) << __func__; }
    ~AutomaticGainControlV1Sw() {
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

    std::string getEffectName() override { return kEffectName; };
    IEffect::Status effectProcessImpl(float* in, float* out, int samples) override;

  private:
    static const std::vector<Range::AutomaticGainControlV1Range> kRanges;
    std::shared_ptr<AutomaticGainControlV1SwContext> mContext;
    ndk::ScopedAStatus getParameterAutomaticGainControlV1(const AutomaticGainControlV1::Tag& tag,
                                                          Parameter::Specific* specific);
};
}  // namespace aidl::android::hardware::audio::effect
