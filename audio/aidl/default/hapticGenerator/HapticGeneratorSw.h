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

class HapticGeneratorSwContext final : public EffectContext {
  public:
    HapticGeneratorSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
    }

    RetCode setHgHapticScales(const std::vector<HapticGenerator::HapticScale>& hapticScales);
    std::vector<HapticGenerator::HapticScale> getHgHapticScales() const;

    RetCode setHgVibratorInformation(const HapticGenerator::VibratorInformation& vibratorInfo) {
        // All float values are valid for resonantFrequencyHz, qFactor, maxAmplitude
        mVibratorInformation = vibratorInfo;
        return RetCode::SUCCESS;
    }

    HapticGenerator::VibratorInformation getHgVibratorInformation() const {
        return mVibratorInformation;
    }

  private:
    static constexpr float DEFAULT_RESONANT_FREQUENCY = 150.0f;
    static constexpr float DEFAULT_Q_FACTOR = 1.0f;
    static constexpr float DEFAULT_MAX_AMPLITUDE = 0.0f;
    std::map<int /* trackID */, HapticGenerator::HapticScale> mHapticScales;
    HapticGenerator::VibratorInformation mVibratorInformation = {
            DEFAULT_RESONANT_FREQUENCY, DEFAULT_Q_FACTOR, DEFAULT_MAX_AMPLITUDE};
};

class HapticGeneratorSw final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const HapticGenerator::Capability kCapability;
    static const Descriptor kDescriptor;
    HapticGeneratorSw() { LOG(DEBUG) << __func__; }
    ~HapticGeneratorSw() {
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
    std::shared_ptr<HapticGeneratorSwContext> mContext;

    ndk::ScopedAStatus getParameterHapticGenerator(const HapticGenerator::Tag& tag,
                                                   Parameter::Specific* specific);
};
}  // namespace aidl::android::hardware::audio::effect
