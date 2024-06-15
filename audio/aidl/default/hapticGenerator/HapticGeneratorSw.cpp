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

#define LOG_TAG "AHAL_HapticGeneratorSw"
#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>
#include <system/audio_effects/effect_uuid.h>

#include "HapticGeneratorSw.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectImplUuidHapticGeneratorSw;
using aidl::android::hardware::audio::effect::getEffectTypeUuidHapticGenerator;
using aidl::android::hardware::audio::effect::HapticGeneratorSw;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidHapticGeneratorSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<HapticGeneratorSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidHapticGeneratorSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = HapticGeneratorSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string HapticGeneratorSw::kEffectName = "HapticGeneratorSw";
/* Effect descriptor */
const Descriptor HapticGeneratorSw::kDescriptor = {
        .common = {.id = {.type = getEffectTypeUuidHapticGenerator(),
                          .uuid = getEffectImplUuidHapticGeneratorSw(),
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL},
                   .name = HapticGeneratorSw::kEffectName,
                   .implementor = "The Android Open Source Project"}};

ndk::ScopedAStatus HapticGeneratorSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus HapticGeneratorSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::hapticGenerator != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& hgParam = specific.get<Parameter::Specific::hapticGenerator>();
    auto tag = hgParam.getTag();

    switch (tag) {
        case HapticGenerator::hapticScales: {
            RETURN_IF(mContext->setHgHapticScales(hgParam.get<HapticGenerator::hapticScales>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "HapticScaleNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case HapticGenerator::vibratorInfo: {
            RETURN_IF(mContext->setHgVibratorInformation(
                              hgParam.get<HapticGenerator::vibratorInfo>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "VibratorInfoNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "HapticGeneratorTagNotSupported");
        }
    }
}

ndk::ScopedAStatus HapticGeneratorSw::getParameterSpecific(const Parameter::Id& id,
                                                           Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::hapticGeneratorTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto hgId = id.get<Parameter::Id::hapticGeneratorTag>();
    auto hgIdTag = hgId.getTag();
    switch (hgIdTag) {
        case HapticGenerator::Id::commonTag:
            return getParameterHapticGenerator(hgId.get<HapticGenerator::Id::commonTag>(),
                                               specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "HapticGeneratorTagNotSupported");
    }
}

ndk::ScopedAStatus HapticGeneratorSw::getParameterHapticGenerator(const HapticGenerator::Tag& tag,
                                                                  Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    HapticGenerator hgParam;
    switch (tag) {
        case HapticGenerator::hapticScales: {
            hgParam.set<HapticGenerator::hapticScales>(mContext->getHgHapticScales());
            break;
        }
        case HapticGenerator::vibratorInfo: {
            hgParam.set<HapticGenerator::vibratorInfo>(mContext->getHgVibratorInformation());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "HapticGeneratorTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::hapticGenerator>(hgParam);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> HapticGeneratorSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<HapticGeneratorSwContext>(1 /* statusFmqDepth */, common);
    }

    return mContext;
}

RetCode HapticGeneratorSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status HapticGeneratorSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode HapticGeneratorSwContext::setHgHapticScales(
        const std::vector<HapticGenerator::HapticScale>& hapticScales) {
    // Assume any audio track ID is valid
    for (auto& it : hapticScales) {
        mHapticScales[it.id] = it;
    }
    return RetCode::SUCCESS;
}

std::vector<HapticGenerator::HapticScale> HapticGeneratorSwContext::getHgHapticScales() const {
    std::vector<HapticGenerator::HapticScale> result;
    std::transform(mHapticScales.begin(), mHapticScales.end(), std::back_inserter(result),
                   [](auto& scaleIt) { return scaleIt.second; });
    return result;
}

}  // namespace aidl::android::hardware::audio::effect
