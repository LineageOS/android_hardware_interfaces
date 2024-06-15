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

class VolumeSwContext final : public EffectContext {
  public:
    VolumeSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
    }

    RetCode setVolLevel(int level);

    int getVolLevel() const { return mLevel; }

    RetCode setVolMute(bool mute);

    bool getVolMute() const { return mMute; }

  private:
    int mLevel = 0;
    bool mMute = false;
};

class VolumeSw final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const Capability kCapability;
    static const Descriptor kDescriptor;
    VolumeSw() { LOG(DEBUG) << __func__; }
    ~VolumeSw() {
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
    static const std::vector<Range::VolumeRange> kRanges;
    std::shared_ptr<VolumeSwContext> mContext GUARDED_BY(mImplMutex);

    ndk::ScopedAStatus getParameterVolume(const Volume::Tag& tag, Parameter::Specific* specific)
            REQUIRES(mImplMutex);
};
}  // namespace aidl::android::hardware::audio::effect
