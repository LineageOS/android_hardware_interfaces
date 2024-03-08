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
#include <unordered_set>

#define LOG_TAG "AHAL_AcousticEchoCancelerSw"
#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>
#include <system/audio_effects/effect_uuid.h>

#include "AcousticEchoCancelerSw.h"

using aidl::android::hardware::audio::effect::AcousticEchoCancelerSw;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectImplUuidAcousticEchoCancelerSw;
using aidl::android::hardware::audio::effect::getEffectTypeUuidAcousticEchoCanceler;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::Range;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidAcousticEchoCancelerSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<AcousticEchoCancelerSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidAcousticEchoCancelerSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = AcousticEchoCancelerSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string AcousticEchoCancelerSw::kEffectName = "AcousticEchoCancelerSw";

const std::vector<Range::AcousticEchoCancelerRange> AcousticEchoCancelerSw::kRanges = {
        MAKE_RANGE(AcousticEchoCanceler, echoDelayUs, 0, 500),
        /* mobile mode not supported, and not settable */
        MAKE_RANGE(AcousticEchoCanceler, mobileMode, false, false)};

const Capability AcousticEchoCancelerSw::kCapability = {.range = AcousticEchoCancelerSw::kRanges};

const Descriptor AcousticEchoCancelerSw::kDescriptor = {
        .common = {.id = {.type = getEffectTypeUuidAcousticEchoCanceler(),
                          .uuid = getEffectImplUuidAcousticEchoCancelerSw(),
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::PRE_PROC,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::NONE},
                   .name = AcousticEchoCancelerSw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = AcousticEchoCancelerSw::kCapability};

ndk::ScopedAStatus AcousticEchoCancelerSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AcousticEchoCancelerSw::setParameterSpecific(
        const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::acousticEchoCanceler != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& param = specific.get<Parameter::Specific::acousticEchoCanceler>();
    RETURN_IF(!inRange(param, kRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");

    auto tag = param.getTag();
    switch (tag) {
        case AcousticEchoCanceler::echoDelayUs: {
            RETURN_IF(mContext->setEchoDelay(param.get<AcousticEchoCanceler::echoDelayUs>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "echoDelayNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case AcousticEchoCanceler::mobileMode: {
            RETURN_IF(true == param.get<AcousticEchoCanceler::mobileMode>(), EX_ILLEGAL_ARGUMENT,
                      "SettingmobileModeSupported");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AcousticEchoCancelerTagNotSupported");
        }
    }
}

ndk::ScopedAStatus AcousticEchoCancelerSw::getParameterSpecific(const Parameter::Id& id,
                                                                Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::acousticEchoCancelerTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto specificId = id.get<Parameter::Id::acousticEchoCancelerTag>();
    auto specificIdTag = specificId.getTag();
    switch (specificIdTag) {
        case AcousticEchoCanceler::Id::commonTag:
            return getParameterAcousticEchoCanceler(
                    specificId.get<AcousticEchoCanceler::Id::commonTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AcousticEchoCancelerTagNotSupported");
    }
}

ndk::ScopedAStatus AcousticEchoCancelerSw::getParameterAcousticEchoCanceler(
        const AcousticEchoCanceler::Tag& tag, Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    AcousticEchoCanceler param;
    switch (tag) {
        case AcousticEchoCanceler::echoDelayUs: {
            param.set<AcousticEchoCanceler::echoDelayUs>(mContext->getEchoDelay());
            break;
        }
        case AcousticEchoCanceler::mobileMode: {
            param.set<AcousticEchoCanceler::mobileMode>(false);
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AcousticEchoCancelerTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::acousticEchoCanceler>(param);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> AcousticEchoCancelerSw::createContext(
        const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<AcousticEchoCancelerSwContext>(1 /* statusFmqDepth */, common);
    }
    return mContext;
}

std::shared_ptr<EffectContext> AcousticEchoCancelerSw::getContext() {
    return mContext;
}

RetCode AcousticEchoCancelerSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status AcousticEchoCancelerSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode AcousticEchoCancelerSwContext::setEchoDelay(int echoDelayUs) {
    mEchoDelayUs = echoDelayUs;
    return RetCode::SUCCESS;
}

}  // namespace aidl::android::hardware::audio::effect
