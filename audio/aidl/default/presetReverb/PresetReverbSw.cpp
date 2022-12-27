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

#include <cstddef>
#define LOG_TAG "AHAL_PresetReverbSw"
#include <Utils.h>
#include <algorithm>
#include <unordered_set>

#include <android-base/logging.h>
#include <android/binder_enums.h>
#include <fmq/AidlMessageQueue.h>

#include "PresetReverbSw.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::kPresetReverbSwImplUUID;
using aidl::android::hardware::audio::effect::PresetReverbSw;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != kPresetReverbSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<PresetReverbSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != kPresetReverbSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = PresetReverbSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string PresetReverbSw::kEffectName = "PresetReverbSw";

const std::vector<PresetReverb::Presets> kSupportedPresets{
        ndk::enum_range<PresetReverb::Presets>().begin(),
        ndk::enum_range<PresetReverb::Presets>().end()};

const PresetReverb::Capability PresetReverbSw::kCapability = {.supportedPresets =
                                                                      kSupportedPresets};
const Descriptor PresetReverbSw::kDescriptor = {
        .common = {.id = {.type = kPresetReverbTypeUUID,
                          .uuid = kPresetReverbSwImplUUID,
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL},
                   .name = PresetReverbSw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = Capability::make<Capability::presetReverb>(PresetReverbSw::kCapability)};

ndk::ScopedAStatus PresetReverbSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PresetReverbSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::presetReverb != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");

    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& prParam = specific.get<Parameter::Specific::presetReverb>();
    auto tag = prParam.getTag();

    switch (tag) {
        case PresetReverb::preset: {
            RETURN_IF(
                    mContext->setPRPreset(prParam.get<PresetReverb::preset>()) != RetCode::SUCCESS,
                    EX_ILLEGAL_ARGUMENT, "setPresetFailed");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "PresetReverbTagNotSupported");
        }
    }
}

ndk::ScopedAStatus PresetReverbSw::getParameterSpecific(const Parameter::Id& id,
                                                        Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::presetReverbTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto prId = id.get<Parameter::Id::presetReverbTag>();
    auto prIdTag = prId.getTag();
    switch (prIdTag) {
        case PresetReverb::Id::commonTag:
            return getParameterPresetReverb(prId.get<PresetReverb::Id::commonTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "PresetReverbTagNotSupported");
    }
}

ndk::ScopedAStatus PresetReverbSw::getParameterPresetReverb(const PresetReverb::Tag& tag,
                                                            Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    PresetReverb prParam;
    switch (tag) {
        case PresetReverb::preset: {
            prParam.set<PresetReverb::preset>(mContext->getPRPreset());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "PresetReverbTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::presetReverb>(prParam);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> PresetReverbSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<PresetReverbSwContext>(1 /* statusFmqDepth */, common);
    }

    return mContext;
}

std::shared_ptr<EffectContext> PresetReverbSw::getContext() {
    return mContext;
}

RetCode PresetReverbSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status PresetReverbSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

}  // namespace aidl::android::hardware::audio::effect
