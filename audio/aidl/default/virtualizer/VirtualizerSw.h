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

class VirtualizerSwContext final : public EffectContext {
  public:
    VirtualizerSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
    }
    RetCode setVrStrength(int strength);
    int getVrStrength() const { return mStrength; }
    RetCode setForcedDevice(
            const ::aidl::android::media::audio::common::AudioDeviceDescription& device) {
        mForceDevice = device;
        return RetCode::SUCCESS;
    }
    aidl::android::media::audio::common::AudioDeviceDescription getForcedDevice() const {
        return mForceDevice;
    }

  private:
    int mStrength = 0;
    ::aidl::android::media::audio::common::AudioDeviceDescription mForceDevice;
};

class VirtualizerSw final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const Capability kCapability;
    static const Descriptor kDescriptor;
    VirtualizerSw() { LOG(DEBUG) << __func__; }
    ~VirtualizerSw() {
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

    IEffect::Status effectProcessImpl(float* in, float* out, int samples) override;
    std::string getEffectName() override { return kEffectName; }

  private:
    static const std::vector<Range::VirtualizerRange> kRanges;
    std::shared_ptr<VirtualizerSwContext> mContext GUARDED_BY(mImplMutex);

    ndk::ScopedAStatus getParameterVirtualizer(const Virtualizer::Tag& tag,
                                               Parameter::Specific* specific) REQUIRES(mImplMutex);
    ndk::ScopedAStatus getSpeakerAngles(const Virtualizer::SpeakerAnglesPayload payload,
                                        Parameter::Specific* specific) REQUIRES(mImplMutex);
};
}  // namespace aidl::android::hardware::audio::effect
