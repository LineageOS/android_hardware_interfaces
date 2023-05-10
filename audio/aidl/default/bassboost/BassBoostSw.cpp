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

#define LOG_TAG "AHAL_BassBoostSw"
#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>
#include <system/audio_effects/effect_uuid.h>

#include "BassBoostSw.h"

using aidl::android::hardware::audio::effect::BassBoostSw;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectImplUuidBassBoostProxy;
using aidl::android::hardware::audio::effect::getEffectImplUuidBassBoostSw;
using aidl::android::hardware::audio::effect::getEffectTypeUuidBassBoost;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidBassBoostSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<BassBoostSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidBassBoostSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = BassBoostSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string BassBoostSw::kEffectName = "BassBoostSw";

const std::vector<Range::BassBoostRange> BassBoostSw::kRanges = {
        MAKE_RANGE(BassBoost, strengthPm, 0, 1000)};
const Capability BassBoostSw::kCapability = {.range = {BassBoostSw::kRanges}};
const Descriptor BassBoostSw::kDescriptor = {
        .common = {.id = {.type = getEffectTypeUuidBassBoost(),
                          .uuid = getEffectImplUuidBassBoostSw()},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL},
                   .name = BassBoostSw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = BassBoostSw::kCapability};

ndk::ScopedAStatus BassBoostSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BassBoostSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::bassBoost != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& bbParam = specific.get<Parameter::Specific::bassBoost>();
    RETURN_IF(!inRange(bbParam, kRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");
    auto tag = bbParam.getTag();

    switch (tag) {
        case BassBoost::strengthPm: {
            const auto strength = bbParam.get<BassBoost::strengthPm>();
            RETURN_IF(mContext->setBbStrengthPm(strength) != RetCode::SUCCESS, EX_ILLEGAL_ARGUMENT,
                      "strengthPmNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "BassBoostTagNotSupported");
        }
    }
}

ndk::ScopedAStatus BassBoostSw::getParameterSpecific(const Parameter::Id& id,
                                                     Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::bassBoostTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto bbId = id.get<Parameter::Id::bassBoostTag>();
    auto bbIdTag = bbId.getTag();
    switch (bbIdTag) {
        case BassBoost::Id::commonTag:
            return getParameterBassBoost(bbId.get<BassBoost::Id::commonTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "BassBoostTagNotSupported");
    }
}

ndk::ScopedAStatus BassBoostSw::getParameterBassBoost(const BassBoost::Tag& tag,
                                                      Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    BassBoost bbParam;
    switch (tag) {
        case BassBoost::strengthPm: {
            bbParam.set<BassBoost::strengthPm>(mContext->getBbStrengthPm());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "BassBoostTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::bassBoost>(bbParam);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> BassBoostSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<BassBoostSwContext>(1 /* statusFmqDepth */, common);
    }
    return mContext;
}

std::shared_ptr<EffectContext> BassBoostSw::getContext() {
    return mContext;
}

RetCode BassBoostSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status BassBoostSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode BassBoostSwContext::setBbStrengthPm(int strength) {
    mStrength = strength;
    return RetCode::SUCCESS;
}

}  // namespace aidl::android::hardware::audio::effect
