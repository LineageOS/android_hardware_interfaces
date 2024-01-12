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

#include "effect-impl/EffectContext.h"
#include "effect-impl/EffectImpl.h"

#include <fmq/AidlMessageQueue.h>

#include <unordered_map>
#include <vector>

namespace aidl::android::hardware::audio::effect {

class SpatializerSwContext final : public EffectContext {
  public:
    SpatializerSwContext(int statusDepth, const Parameter::Common& common);
    ~SpatializerSwContext();

    template <typename TAG>
    std::optional<Spatializer> getParam(TAG tag);
    template <typename TAG>
    ndk::ScopedAStatus setParam(TAG tag, Spatializer spatializer);

    IEffect::Status process(float* in, float* out, int samples);

  private:
    std::unordered_map<Spatializer::Tag, Spatializer> mParamsMap;
};

class SpatializerSw final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const Capability kCapability;
    static const Descriptor kDescriptor;
    ~SpatializerSw();

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific)
            REQUIRES(mImplMutex) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id, Parameter::Specific* specific)
            REQUIRES(mImplMutex) override;

    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common)
            REQUIRES(mImplMutex) override;
    RetCode releaseContext() REQUIRES(mImplMutex) override;

    std::string getEffectName() override { return kEffectName; };
    IEffect::Status effectProcessImpl(float* in, float* out, int samples)
            REQUIRES(mImplMutex) override;

  private:
    static const std::vector<Range::SpatializerRange> kRanges;
    std::shared_ptr<SpatializerSwContext> mContext GUARDED_BY(mImplMutex) = nullptr;
};
}  // namespace aidl::android::hardware::audio::effect
