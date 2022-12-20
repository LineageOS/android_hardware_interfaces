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
#define LOG_TAG "AHAL_AutomaticGainControlSw"
#include <Utils.h>
#include <unordered_set>

#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

#include "AutomaticGainControlSw.h"

using aidl::android::hardware::audio::effect::AutomaticGainControlSw;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::kAutomaticGainControlSwImplUUID;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != kAutomaticGainControlSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<AutomaticGainControlSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != kAutomaticGainControlSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = AutomaticGainControlSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string AutomaticGainControlSw::kEffectName = "AutomaticGainControlSw";
const AutomaticGainControl::Capability AutomaticGainControlSw::kCapability = {
        .maxFixedDigitalGainMb = 50000, .maxSaturationMarginMb = 10000};
const Descriptor AutomaticGainControlSw::kDescriptor = {
        .common = {.id = {.type = kAutomaticGainControlTypeUUID,
                          .uuid = kAutomaticGainControlSwImplUUID,
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL},
                   .name = AutomaticGainControlSw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = Capability::make<Capability::automaticGainControl>(
                AutomaticGainControlSw::kCapability)};

ndk::ScopedAStatus AutomaticGainControlSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AutomaticGainControlSw::setParameterSpecific(
        const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::automaticGainControl != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& param = specific.get<Parameter::Specific::automaticGainControl>();
    auto tag = param.getTag();

    switch (tag) {
        case AutomaticGainControl::fixedDigitalGainMb: {
            RETURN_IF(mContext->setDigitalGain(
                              param.get<AutomaticGainControl::fixedDigitalGainMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "digitalGainNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case AutomaticGainControl::levelEstimator: {
            RETURN_IF(
                    mContext->setLevelEstimator(
                            param.get<AutomaticGainControl::levelEstimator>()) != RetCode::SUCCESS,
                    EX_ILLEGAL_ARGUMENT, "levelEstimatorNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case AutomaticGainControl::saturationMarginMb: {
            RETURN_IF(mContext->setSaturationMargin(
                              param.get<AutomaticGainControl::saturationMarginMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "saturationMarginNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AutomaticGainControlTagNotSupported");
        }
    }
}

ndk::ScopedAStatus AutomaticGainControlSw::getParameterSpecific(const Parameter::Id& id,
                                                                Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::automaticGainControlTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto specificId = id.get<Parameter::Id::automaticGainControlTag>();
    auto specificIdTag = specificId.getTag();
    switch (specificIdTag) {
        case AutomaticGainControl::Id::commonTag:
            return getParameterAutomaticGainControl(
                    specificId.get<AutomaticGainControl::Id::commonTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AutomaticGainControlTagNotSupported");
    }
}

ndk::ScopedAStatus AutomaticGainControlSw::getParameterAutomaticGainControl(
        const AutomaticGainControl::Tag& tag, Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    AutomaticGainControl param;
    switch (tag) {
        case AutomaticGainControl::fixedDigitalGainMb: {
            param.set<AutomaticGainControl::fixedDigitalGainMb>(mContext->getDigitalGain());
            break;
        }
        case AutomaticGainControl::levelEstimator: {
            param.set<AutomaticGainControl::levelEstimator>(mContext->getLevelEstimator());
            break;
        }
        case AutomaticGainControl::saturationMarginMb: {
            param.set<AutomaticGainControl::saturationMarginMb>(mContext->getSaturationMargin());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AutomaticGainControlTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::automaticGainControl>(param);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> AutomaticGainControlSw::createContext(
        const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<AutomaticGainControlSwContext>(1 /* statusFmqDepth */, common);
    }
    return mContext;
}

std::shared_ptr<EffectContext> AutomaticGainControlSw::getContext() {
    return mContext;
}

RetCode AutomaticGainControlSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status AutomaticGainControlSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode AutomaticGainControlSwContext::setDigitalGain(int gain) {
    if (gain < 0 || gain > AutomaticGainControlSw::kCapability.maxFixedDigitalGainMb) {
        LOG(DEBUG) << __func__ << " illegal digital gain " << gain;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    mDigitalGain = gain;
    return RetCode::SUCCESS;
}

int AutomaticGainControlSwContext::getDigitalGain() {
    return mDigitalGain;
}

RetCode AutomaticGainControlSwContext::setLevelEstimator(
        AutomaticGainControl::LevelEstimator levelEstimator) {
    mLevelEstimator = levelEstimator;
    return RetCode::SUCCESS;
}

AutomaticGainControl::LevelEstimator AutomaticGainControlSwContext::getLevelEstimator() {
    return mLevelEstimator;
}

RetCode AutomaticGainControlSwContext::setSaturationMargin(int margin) {
    if (margin < 0 || margin > AutomaticGainControlSw::kCapability.maxSaturationMarginMb) {
        LOG(DEBUG) << __func__ << " illegal saturationMargin " << margin;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    mSaturationMargin = margin;
    return RetCode::SUCCESS;
}

int AutomaticGainControlSwContext::getSaturationMargin() {
    return mSaturationMargin;
}

}  // namespace aidl::android::hardware::audio::effect
