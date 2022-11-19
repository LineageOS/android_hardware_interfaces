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

class DynamicsProcessingSwContext final : public EffectContext {
  public:
    DynamicsProcessingSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
    }
    // TODO: add specific context here
};

class DynamicsProcessingSw final : public EffectImpl {
  public:
    DynamicsProcessingSw() { LOG(DEBUG) << __func__; }
    ~DynamicsProcessingSw() {
        cleanUp();
        LOG(DEBUG) << __func__;
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;
    IEffect::Status effectProcessImpl(float* in, float* out, int process) override;
    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common) override;
    RetCode releaseContext() override;

  private:
    std::shared_ptr<DynamicsProcessingSwContext> mContext;
    /* capabilities */
    const DynamicsProcessing::Capability kCapability;
    /* Effect descriptor */
    const Descriptor kDescriptor = {
            .common = {.id = {.type = kDynamicsProcessingTypeUUID,
                              .uuid = kDynamicsProcessingSwImplUUID,
                              .proxy = std::nullopt},
                       .flags = {.type = Flags::Type::INSERT,
                                 .insert = Flags::Insert::FIRST,
                                 .volume = Flags::Volume::CTRL},
                       .name = "DynamicsProcessingSw",
                       .implementor = "The Android Open Source Project"},
            .capability = Capability::make<Capability::dynamicsProcessing>(kCapability)};

    /* parameters */
    DynamicsProcessing mSpecificParam;
};
}  // namespace aidl::android::hardware::audio::effect
