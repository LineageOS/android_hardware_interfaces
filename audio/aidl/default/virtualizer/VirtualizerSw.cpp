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
#include <optional>

#define LOG_TAG "AHAL_VirtualizerSw"
#include <Utils.h>
#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>
#include <system/audio_effects/effect_uuid.h>

#include "VirtualizerSw.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectImplUuidVirtualizerSw;
using aidl::android::hardware::audio::effect::getEffectTypeUuidVirtualizer;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::State;
using aidl::android::hardware::audio::effect::VirtualizerSw;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidVirtualizerSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<VirtualizerSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidVirtualizerSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = VirtualizerSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string VirtualizerSw::kEffectName = "VirtualizerSw";

const std::vector<Range::VirtualizerRange> VirtualizerSw::kRanges = {
        MAKE_RANGE(Virtualizer, strengthPm, 0, 1000),
        /* speakerAngle is get-only, set min > max */
        MAKE_RANGE(Virtualizer, speakerAngles, {Virtualizer::ChannelAngle({.channel = 1})},
                   {Virtualizer::ChannelAngle({.channel = 0})})};

const Capability VirtualizerSw::kCapability = {
        .range = Range::make<Range::virtualizer>(VirtualizerSw::kRanges)};

const Descriptor VirtualizerSw::kDescriptor = {
        .common = {.id = {.type = getEffectTypeUuidVirtualizer(),
                          .uuid = getEffectImplUuidVirtualizerSw()},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL},
                   .name = VirtualizerSw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = VirtualizerSw::kCapability};

ndk::ScopedAStatus VirtualizerSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VirtualizerSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::virtualizer != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");

    auto& vrParam = specific.get<Parameter::Specific::virtualizer>();
    RETURN_IF(!inRange(vrParam, kRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");
    auto tag = vrParam.getTag();

    switch (tag) {
        case Virtualizer::strengthPm: {
            RETURN_IF(mContext->setVrStrength(vrParam.get<Virtualizer::strengthPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setStrengthPmFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Virtualizer::device: {
            RETURN_IF(mContext->setForcedDevice(vrParam.get<Virtualizer::device>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDeviceFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Virtualizer::speakerAngles:
            FALLTHROUGH_INTENDED;
        case Virtualizer::vendor: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VirtualizerTagNotSupported");
        }
    }
}

ndk::ScopedAStatus VirtualizerSw::getParameterSpecific(const Parameter::Id& id,
                                                       Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::virtualizerTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto vrId = id.get<Parameter::Id::virtualizerTag>();
    auto vrIdTag = vrId.getTag();
    switch (vrIdTag) {
        case Virtualizer::Id::commonTag:
            return getParameterVirtualizer(vrId.get<Virtualizer::Id::commonTag>(), specific);
        case Virtualizer::Id::speakerAnglesPayload:
            return getSpeakerAngles(vrId.get<Virtualizer::Id::speakerAnglesPayload>(), specific);
        case Virtualizer::Id::vendorExtensionTag: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VirtualizerTagNotSupported");
        }
    }
}

ndk::ScopedAStatus VirtualizerSw::getParameterVirtualizer(const Virtualizer::Tag& tag,
                                                          Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    Virtualizer vrParam;
    switch (tag) {
        case Virtualizer::strengthPm: {
            vrParam.set<Virtualizer::strengthPm>(mContext->getVrStrength());
            break;
        }
        case Virtualizer::device: {
            vrParam.set<Virtualizer::device>(mContext->getForcedDevice());
            break;
        }
        case Virtualizer::speakerAngles:
            FALLTHROUGH_INTENDED;
        case Virtualizer::vendor: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VirtualizerTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::virtualizer>(vrParam);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VirtualizerSw::getSpeakerAngles(const Virtualizer::SpeakerAnglesPayload payload,
                                                   Parameter::Specific* specific) {
    std::vector<Virtualizer::ChannelAngle> angles;
    const auto chNum = ::aidl::android::hardware::audio::common::getChannelCount(payload.layout);
    if (chNum == 1) {
        angles = {{.channel = (int32_t)AudioChannelLayout::CHANNEL_FRONT_LEFT,
                   .azimuthDegree = 0,
                   .elevationDegree = 0}};
    } else if (chNum == 2) {
        angles = {{.channel = (int32_t)AudioChannelLayout::CHANNEL_FRONT_LEFT,
                   .azimuthDegree = -90,
                   .elevationDegree = 0},
                  {.channel = (int32_t)AudioChannelLayout::CHANNEL_FRONT_RIGHT,
                   .azimuthDegree = 90,
                   .elevationDegree = 0}};
    } else {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "supportUpTo2Ch");
    }

    Virtualizer param = Virtualizer::make<Virtualizer::speakerAngles>(angles);
    specific->set<Parameter::Specific::virtualizer>(param);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> VirtualizerSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<VirtualizerSwContext>(1 /* statusFmqDepth */, common);
    }

    return mContext;
}

std::shared_ptr<EffectContext> VirtualizerSw::getContext() {
    return mContext;
}

RetCode VirtualizerSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status VirtualizerSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode VirtualizerSwContext::setVrStrength(int strength) {
    mStrength = strength;
    return RetCode::SUCCESS;
}

}  // namespace aidl::android::hardware::audio::effect
