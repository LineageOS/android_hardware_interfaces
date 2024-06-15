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

#define LOG_TAG "AHAL_AutomaticGainControlV1Sw"

#include <android-base/logging.h>
#include <system/audio_effects/effect_uuid.h>

#include "AutomaticGainControlV1Sw.h"

using aidl::android::hardware::audio::effect::AutomaticGainControlV1Sw;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectImplUuidAutomaticGainControlV1Sw;
using aidl::android::hardware::audio::effect::getEffectTypeUuidAutomaticGainControlV1;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidAutomaticGainControlV1Sw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<AutomaticGainControlV1Sw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidAutomaticGainControlV1Sw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = AutomaticGainControlV1Sw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string AutomaticGainControlV1Sw::kEffectName = "AutomaticGainControlV1Sw";

const std::vector<Range::AutomaticGainControlV1Range> AutomaticGainControlV1Sw::kRanges = {
        MAKE_RANGE(AutomaticGainControlV1, targetPeakLevelDbFs, -3100, 0),
        MAKE_RANGE(AutomaticGainControlV1, maxCompressionGainDb, 0, 9000)};

const Capability AutomaticGainControlV1Sw::kCapability = {
        .range = AutomaticGainControlV1Sw::kRanges};

const Descriptor AutomaticGainControlV1Sw::kDescriptor = {
        .common = {.id = {.type = getEffectTypeUuidAutomaticGainControlV1(),
                          .uuid = getEffectImplUuidAutomaticGainControlV1Sw(),
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL},
                   .name = AutomaticGainControlV1Sw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = AutomaticGainControlV1Sw::kCapability};

ndk::ScopedAStatus AutomaticGainControlV1Sw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AutomaticGainControlV1Sw::setParameterSpecific(
        const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::automaticGainControlV1 != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& param = specific.get<Parameter::Specific::automaticGainControlV1>();
    RETURN_IF(!inRange(param, kRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");
    auto tag = param.getTag();
    switch (tag) {
        case AutomaticGainControlV1::targetPeakLevelDbFs: {
            RETURN_IF(mContext->setTargetPeakLevel(
                              param.get<AutomaticGainControlV1::targetPeakLevelDbFs>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "targetPeakLevelNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case AutomaticGainControlV1::maxCompressionGainDb: {
            RETURN_IF(mContext->setMaxCompressionGain(
                              param.get<AutomaticGainControlV1::maxCompressionGainDb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "maxCompressionGainNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case AutomaticGainControlV1::enableLimiter: {
            RETURN_IF(
                    mContext->setEnableLimiter(
                            param.get<AutomaticGainControlV1::enableLimiter>()) != RetCode::SUCCESS,
                    EX_ILLEGAL_ARGUMENT, "enableLimiterNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AutomaticGainControlV1TagNotSupported");
        }
    }
}

ndk::ScopedAStatus AutomaticGainControlV1Sw::getParameterSpecific(const Parameter::Id& id,
                                                                  Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::automaticGainControlV1Tag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto specificId = id.get<Parameter::Id::automaticGainControlV1Tag>();
    auto specificIdTag = specificId.getTag();
    switch (specificIdTag) {
        case AutomaticGainControlV1::Id::commonTag:
            return getParameterAutomaticGainControlV1(
                    specificId.get<AutomaticGainControlV1::Id::commonTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AutomaticGainControlV1TagNotSupported");
    }
}

ndk::ScopedAStatus AutomaticGainControlV1Sw::getParameterAutomaticGainControlV1(
        const AutomaticGainControlV1::Tag& tag, Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    AutomaticGainControlV1 param;
    switch (tag) {
        case AutomaticGainControlV1::targetPeakLevelDbFs: {
            param.set<AutomaticGainControlV1::targetPeakLevelDbFs>(mContext->getTargetPeakLevel());
            break;
        }
        case AutomaticGainControlV1::maxCompressionGainDb: {
            param.set<AutomaticGainControlV1::maxCompressionGainDb>(
                    mContext->getMaxCompressionGain());
            break;
        }
        case AutomaticGainControlV1::enableLimiter: {
            param.set<AutomaticGainControlV1::enableLimiter>(mContext->getEnableLimiter());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AutomaticGainControlV1TagNotSupported");
        }
    }

    specific->set<Parameter::Specific::automaticGainControlV1>(param);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> AutomaticGainControlV1Sw::createContext(
        const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext =
                std::make_shared<AutomaticGainControlV1SwContext>(1 /* statusFmqDepth */, common);
    }
    return mContext;
}

RetCode AutomaticGainControlV1Sw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status AutomaticGainControlV1Sw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode AutomaticGainControlV1SwContext::setTargetPeakLevel(int targetPeakLevel) {
    mTargetPeakLevel = targetPeakLevel;
    return RetCode::SUCCESS;
}

int AutomaticGainControlV1SwContext::getTargetPeakLevel() {
    return mTargetPeakLevel;
}

RetCode AutomaticGainControlV1SwContext::setMaxCompressionGain(int maxCompressionGain) {
    mMaxCompressionGain = maxCompressionGain;
    return RetCode::SUCCESS;
}

int AutomaticGainControlV1SwContext::getMaxCompressionGain() {
    return mMaxCompressionGain;
}

RetCode AutomaticGainControlV1SwContext::setEnableLimiter(bool enableLimiter) {
    mEnableLimiter = enableLimiter;
    return RetCode::SUCCESS;
}

bool AutomaticGainControlV1SwContext::getEnableLimiter() {
    return mEnableLimiter;
}

}  // namespace aidl::android::hardware::audio::effect
