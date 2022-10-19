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

#include "effect-impl/EffectContext.h"
#include "effect-impl/EffectTypes.h"
#include "effect-impl/EffectUUID.h"
#include "effect-impl/EffectWorker.h"

namespace aidl::android::hardware::audio::effect {

class EqualizerSwContext : public EffectContext {
  public:
    EqualizerSwContext(int statusDepth, int inBufferSize, int outBufferSize)
        : EffectContext(statusDepth, inBufferSize, outBufferSize) {
        LOG(DEBUG) << __func__;
    }

  private:
    // Add equalizer specific context for processing here
};

class EqualizerSw : public BnEffect, EffectWorker {
  public:
    EqualizerSw() {
        Equalizer::Capability eqCap = {.bandFrequencies = mBandFrequency, .presets = mPresets};
        mDesc.capability.set<Capability::equalizer>(eqCap);
        LOG(DEBUG) << __func__;
    };
    ~EqualizerSw() {
        cleanUp();
        LOG(DEBUG) << __func__;
    };
    ndk::ScopedAStatus open(const Parameter::Common& common, const Parameter::Specific& specific,
                            OpenEffectReturn* _aidl_return) override;
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;

    ndk::ScopedAStatus getState(State* _aidl_return) override;
    ndk::ScopedAStatus command(CommandId in_commandId) override;
    ndk::ScopedAStatus setParameter(const Parameter& in_param) override;
    ndk::ScopedAStatus getParameter(const Parameter::Id& in_paramId,
                                    Parameter* _aidl_return) override;

    IEffect::Status effectProcessImpl() override;

  private:
    // Effect descriptor.
    Descriptor mDesc = {.common = {.id = {.type = EqualizerTypeUUID, .uuid = EqualizerSwImplUUID}}};

    // Parameters.
    Parameter::Common mCommonParam;
    Equalizer mEqualizerParam;  // TODO: the equalizer parameter needs to update

    // Instance state INIT by default.
    State mState = State::INIT;

    int mPreset = PRESET_CUSTOM;  // the current preset
    const std::vector<Equalizer::BandFrequency> mBandFrequency = {{0, 30000, 120000},
                                                                  {1, 120001, 460000},
                                                                  {2, 460001, 1800000},
                                                                  {3, 1800001, 7000000},
                                                                  {4, 7000001, 20000000}};
    // preset band level
    std::vector<Equalizer::BandLevel> mBandLevels = {{0, 3}, {1, 0}, {2, 0}, {3, 0}, {4, 3}};
    // presets supported by the device
    const std::vector<Equalizer::Preset> mPresets = {
            {0, "Normal"},      {1, "Classical"}, {2, "Dance"}, {3, "Flat"}, {4, "Folk"},
            {5, "Heavy Metal"}, {6, "Hip Hop"},   {7, "Jazz"},  {8, "Pop"},  {9, "Rock"}};
    static const int NUM_OF_BANDS = 5;
    static const int NUM_OF_PRESETS = 10;
    static const int PRESET_CUSTOM = -1;

    // Equalizer worker context
    std::shared_ptr<EqualizerSwContext> mContext;

    ndk::ScopedAStatus setCommonParameter(const Parameter::Common& common_param);
    ndk::ScopedAStatus setSpecificParameter(const Parameter::Specific& specific);
    ndk::ScopedAStatus getSpecificParameter(Parameter::Specific::Id id,
                                            Parameter::Specific* specific);

    void cleanUp();

    IEffect::Status status(binder_status_t status, size_t consumed, size_t produced);
};
}  // namespace aidl::android::hardware::audio::effect
