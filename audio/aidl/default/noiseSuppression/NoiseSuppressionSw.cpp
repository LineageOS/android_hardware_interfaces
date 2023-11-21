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
#include <memory>
#define LOG_TAG "AHAL_NoiseSuppressionSw"

#define LOG_TAG "AHAL_NoiseSuppressionSw"
#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>
#include <system/audio_effects/effect_uuid.h>

#include "NoiseSuppressionSw.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectImplUuidNoiseSuppressionSw;
using aidl::android::hardware::audio::effect::getEffectTypeUuidNoiseSuppression;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::NoiseSuppressionSw;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidNoiseSuppressionSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<NoiseSuppressionSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidNoiseSuppressionSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = NoiseSuppressionSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string NoiseSuppressionSw::kEffectName = "NoiseSuppressionSw";
const Descriptor NoiseSuppressionSw::kDescriptor = {
        .common = {.id = {.type = getEffectTypeUuidNoiseSuppression(),
                          .uuid = getEffectImplUuidNoiseSuppressionSw(),
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::PRE_PROC,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::NONE},
                   .name = NoiseSuppressionSw::kEffectName,
                   .implementor = "The Android Open Source Project"}};

ndk::ScopedAStatus NoiseSuppressionSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus NoiseSuppressionSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::noiseSuppression != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& param = specific.get<Parameter::Specific::noiseSuppression>();
    auto tag = param.getTag();

    switch (tag) {
        case NoiseSuppression::level: {
            RETURN_IF(mContext->setLevel(param.get<NoiseSuppression::level>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "levelNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case NoiseSuppression::type: {
            RETURN_IF(mContext->setType(param.get<NoiseSuppression::type>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "typeNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case NoiseSuppression::vendor: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "NoiseSuppressionTagNotSupported");
        }
    }
}

ndk::ScopedAStatus NoiseSuppressionSw::getParameterSpecific(const Parameter::Id& id,
                                                            Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::noiseSuppressionTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto specificId = id.get<Parameter::Id::noiseSuppressionTag>();
    auto specificIdTag = specificId.getTag();
    switch (specificIdTag) {
        case NoiseSuppression::Id::commonTag:
            return getParameterNoiseSuppression(specificId.get<NoiseSuppression::Id::commonTag>(),
                                                specific);
        case NoiseSuppression::Id::vendorExtensionTag: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "NoiseSuppressionTagNotSupported");
        }
    }
}

ndk::ScopedAStatus NoiseSuppressionSw::getParameterNoiseSuppression(
        const NoiseSuppression::Tag& tag, Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    NoiseSuppression param;
    switch (tag) {
        case NoiseSuppression::level: {
            param.set<NoiseSuppression::level>(mContext->getLevel());
            break;
        }
        case NoiseSuppression::type: {
            param.set<NoiseSuppression::type>(mContext->getType());
            break;
        }
        case NoiseSuppression::vendor: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "NoiseSuppressionTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::noiseSuppression>(param);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> NoiseSuppressionSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<NoiseSuppressionSwContext>(1 /* statusFmqDepth */, common);
    }
    return mContext;
}

std::shared_ptr<EffectContext> NoiseSuppressionSw::getContext() {
    return mContext;
}

RetCode NoiseSuppressionSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status NoiseSuppressionSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode NoiseSuppressionSwContext::setLevel(NoiseSuppression::Level level) {
    mLevel = level;
    return RetCode::SUCCESS;
}

NoiseSuppression::Level NoiseSuppressionSwContext::getLevel() {
    return mLevel;
}

RetCode NoiseSuppressionSwContext::setType(NoiseSuppression::Type type) {
    mType = type;
    return RetCode::SUCCESS;
}

}  // namespace aidl::android::hardware::audio::effect
