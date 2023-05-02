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

#include <algorithm>
#include <cstddef>

#define LOG_TAG "AHAL_EqualizerSw"
#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>
#include <system/audio_effects/effect_uuid.h>

#include "EqualizerSw.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::EqualizerSw;
using aidl::android::hardware::audio::effect::getEffectImplUuidEqualizerSw;
using aidl::android::hardware::audio::effect::getEffectTypeUuidEqualizer;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidEqualizerSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<EqualizerSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidEqualizerSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = EqualizerSw::kDesc;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string EqualizerSw::kEffectName = "EqualizerSw";

const std::vector<Equalizer::BandFrequency> EqualizerSw::kBandFrequency = {{0, 30000, 120000},
                                                                           {1, 120001, 460000},
                                                                           {2, 460001, 1800000},
                                                                           {3, 1800001, 7000000},
                                                                           {4, 7000001, 20000000}};
const std::vector<Equalizer::Preset> EqualizerSw::kPresets = {
        {0, "Normal"},      {1, "Classical"}, {2, "Dance"}, {3, "Flat"}, {4, "Folk"},
        {5, "Heavy Metal"}, {6, "Hip Hop"},   {7, "Jazz"},  {8, "Pop"},  {9, "Rock"}};

/**
 * Use the same min and max to build a capability represented by Range.
 */
const std::vector<Range::EqualizerRange> EqualizerSw::kRanges = {
        MAKE_RANGE(Equalizer, preset, 0, EqualizerSw::kPresets.size() - 1),
        MAKE_RANGE(Equalizer, bandLevels,
                   std::vector<Equalizer::BandLevel>{
                           Equalizer::BandLevel({.index = 0, .levelMb = -15})},
                   std::vector<Equalizer::BandLevel>{Equalizer::BandLevel(
                           {.index = EqualizerSwContext::kMaxBandNumber - 1, .levelMb = 15})}),
        /* capability definition */
        MAKE_RANGE(Equalizer, bandFrequencies, EqualizerSw::kBandFrequency,
                   EqualizerSw::kBandFrequency),
        MAKE_RANGE(Equalizer, presets, EqualizerSw::kPresets, EqualizerSw::kPresets),
        /* centerFreqMh is get only, set invalid range min > max */
        MAKE_RANGE(Equalizer, centerFreqMh, std::vector<int>({1}), std::vector<int>({0}))};

const Capability EqualizerSw::kEqCap = {.range = EqualizerSw::kRanges};
const Descriptor EqualizerSw::kDesc = {.common = {.id = {.type = getEffectTypeUuidEqualizer(),
                                                         .uuid = getEffectImplUuidEqualizerSw()},
                                                  .flags = {.type = Flags::Type::INSERT,
                                                            .insert = Flags::Insert::FIRST,
                                                            .volume = Flags::Volume::CTRL},
                                                  .name = EqualizerSw::kEffectName,
                                                  .implementor = "The Android Open Source Project"},
                                       .capability = EqualizerSw::kEqCap};

ndk::ScopedAStatus EqualizerSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDesc.toString();
    *_aidl_return = kDesc;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EqualizerSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::equalizer != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& eqParam = specific.get<Parameter::Specific::equalizer>();
    RETURN_IF(!inRange(eqParam, kRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");
    auto tag = eqParam.getTag();
    switch (tag) {
        case Equalizer::preset: {
            RETURN_IF(mContext->setEqPreset(eqParam.get<Equalizer::preset>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setBandLevelsFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Equalizer::bandLevels: {
            RETURN_IF(mContext->setEqBandLevels(eqParam.get<Equalizer::bandLevels>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setBandLevelsFailed");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "EqTagNotSupported");
        }
    }

    LOG(ERROR) << __func__ << " unsupported eq param tag: " << toString(tag);
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                            "ParamNotSupported");
}

ndk::ScopedAStatus EqualizerSw::getParameterSpecific(const Parameter::Id& id,
                                                     Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::equalizerTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto eqId = id.get<Parameter::Id::equalizerTag>();
    auto eqIdTag = eqId.getTag();
    switch (eqIdTag) {
        case Equalizer::Id::commonTag:
            return getParameterEqualizer(eqId.get<Equalizer::Id::commonTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " tag " << toString(eqIdTag) << " not supported";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "EqualizerTagNotSupported");
    }
}

ndk::ScopedAStatus EqualizerSw::getParameterEqualizer(const Equalizer::Tag& tag,
                                                      Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    Equalizer eqParam;
    switch (tag) {
        case Equalizer::bandLevels: {
            eqParam.set<Equalizer::bandLevels>(mContext->getEqBandLevels());
            break;
        }
        case Equalizer::preset: {
            eqParam.set<Equalizer::preset>(mContext->getEqPreset());
            break;
        }
        case Equalizer::centerFreqMh: {
            eqParam.set<Equalizer::centerFreqMh>(mContext->getCenterFreqs());
            break;
        }
        case Equalizer::bandFrequencies: {
            eqParam.set<Equalizer::bandFrequencies>(kBandFrequency);
            break;
        }
        case Equalizer::presets: {
            eqParam.set<Equalizer::presets>(kPresets);
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " not handled tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "unsupportedTag");
        }
    }

    specific->set<Parameter::Specific::equalizer>(eqParam);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> EqualizerSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<EqualizerSwContext>(1 /* statusFmqDepth */, common);
    }

    return mContext;
}

std::shared_ptr<EffectContext> EqualizerSw::getContext() {
    return mContext;
}

RetCode EqualizerSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status EqualizerSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

}  // namespace aidl::android::hardware::audio::effect
