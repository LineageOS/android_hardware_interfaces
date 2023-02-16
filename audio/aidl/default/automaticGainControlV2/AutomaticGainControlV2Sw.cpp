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
#define LOG_TAG "AHAL_AutomaticGainControlV2Sw"
#include <Utils.h>
#include <unordered_set>

#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

#include "AutomaticGainControlV2Sw.h"

using aidl::android::hardware::audio::effect::AutomaticGainControlV2Sw;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::kAutomaticGainControlV2SwImplUUID;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != kAutomaticGainControlV2SwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<AutomaticGainControlV2Sw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != kAutomaticGainControlV2SwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = AutomaticGainControlV2Sw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string AutomaticGainControlV2Sw::kEffectName = "AutomaticGainControlV2Sw";

const std::vector<Range::AutomaticGainControlV2Range> AutomaticGainControlV2Sw::kRanges = {
        MAKE_RANGE(AutomaticGainControlV2, fixedDigitalGainMb, 0, 50000),
        MAKE_RANGE(AutomaticGainControlV2, saturationMarginMb, 0, 10000)};

const Capability AutomaticGainControlV2Sw::kCapability = {
        .range = AutomaticGainControlV2Sw::kRanges};

const Descriptor AutomaticGainControlV2Sw::kDescriptor = {
        .common = {.id = {.type = kAutomaticGainControlV2TypeUUID,
                          .uuid = kAutomaticGainControlV2SwImplUUID,
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL},
                   .name = AutomaticGainControlV2Sw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = AutomaticGainControlV2Sw::kCapability};

ndk::ScopedAStatus AutomaticGainControlV2Sw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AutomaticGainControlV2Sw::setParameterSpecific(
        const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::automaticGainControlV2 != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& param = specific.get<Parameter::Specific::automaticGainControlV2>();
    RETURN_IF(!inRange(param, kRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");
    auto tag = param.getTag();
    switch (tag) {
        case AutomaticGainControlV2::fixedDigitalGainMb: {
            RETURN_IF(mContext->setDigitalGain(
                              param.get<AutomaticGainControlV2::fixedDigitalGainMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "digitalGainNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case AutomaticGainControlV2::levelEstimator: {
            RETURN_IF(mContext->setLevelEstimator(
                              param.get<AutomaticGainControlV2::levelEstimator>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "levelEstimatorNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case AutomaticGainControlV2::saturationMarginMb: {
            RETURN_IF(mContext->setSaturationMargin(
                              param.get<AutomaticGainControlV2::saturationMarginMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "saturationMarginNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AutomaticGainControlV2TagNotSupported");
        }
    }
}

ndk::ScopedAStatus AutomaticGainControlV2Sw::getParameterSpecific(const Parameter::Id& id,
                                                                  Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::automaticGainControlV2Tag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto specificId = id.get<Parameter::Id::automaticGainControlV2Tag>();
    auto specificIdTag = specificId.getTag();
    switch (specificIdTag) {
        case AutomaticGainControlV2::Id::commonTag:
            return getParameterAutomaticGainControlV2(
                    specificId.get<AutomaticGainControlV2::Id::commonTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AutomaticGainControlV2TagNotSupported");
    }
}

ndk::ScopedAStatus AutomaticGainControlV2Sw::getParameterAutomaticGainControlV2(
        const AutomaticGainControlV2::Tag& tag, Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    AutomaticGainControlV2 param;
    switch (tag) {
        case AutomaticGainControlV2::fixedDigitalGainMb: {
            param.set<AutomaticGainControlV2::fixedDigitalGainMb>(mContext->getDigitalGain());
            break;
        }
        case AutomaticGainControlV2::levelEstimator: {
            param.set<AutomaticGainControlV2::levelEstimator>(mContext->getLevelEstimator());
            break;
        }
        case AutomaticGainControlV2::saturationMarginMb: {
            param.set<AutomaticGainControlV2::saturationMarginMb>(mContext->getSaturationMargin());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AutomaticGainControlV2TagNotSupported");
        }
    }

    specific->set<Parameter::Specific::automaticGainControlV2>(param);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> AutomaticGainControlV2Sw::createContext(
        const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext =
                std::make_shared<AutomaticGainControlV2SwContext>(1 /* statusFmqDepth */, common);
    }
    return mContext;
}

std::shared_ptr<EffectContext> AutomaticGainControlV2Sw::getContext() {
    return mContext;
}

RetCode AutomaticGainControlV2Sw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status AutomaticGainControlV2Sw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode AutomaticGainControlV2SwContext::setDigitalGain(int gain) {
    mDigitalGain = gain;
    return RetCode::SUCCESS;
}

int AutomaticGainControlV2SwContext::getDigitalGain() {
    return mDigitalGain;
}

RetCode AutomaticGainControlV2SwContext::setLevelEstimator(
        AutomaticGainControlV2::LevelEstimator levelEstimator) {
    mLevelEstimator = levelEstimator;
    return RetCode::SUCCESS;
}

AutomaticGainControlV2::LevelEstimator AutomaticGainControlV2SwContext::getLevelEstimator() {
    return mLevelEstimator;
}

RetCode AutomaticGainControlV2SwContext::setSaturationMargin(int margin) {
    mSaturationMargin = margin;
    return RetCode::SUCCESS;
}

int AutomaticGainControlV2SwContext::getSaturationMargin() {
    return mSaturationMargin;
}

}  // namespace aidl::android::hardware::audio::effect
