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

#define LOG_TAG "AHAL_SpatializerSw"

#include "SpatializerSw.h"

#include <android-base/logging.h>
#include <system/audio_effects/effect_uuid.h>

#include <optional>

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectImplUuidSpatializerSw;
using aidl::android::hardware::audio::effect::getEffectTypeUuidSpatializer;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::SpatializerSw;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioUuid;
using aidl::android::media::audio::common::HeadTracking;
using aidl::android::media::audio::common::Spatialization;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidSpatializerSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (!instanceSpp) {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }

    *instanceSpp = ndk::SharedRefBase::make<SpatializerSw>();
    LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
    return EX_NONE;
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidSpatializerSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = SpatializerSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string SpatializerSw::kEffectName = "SpatializerSw";

const std::vector<Range::SpatializerRange> SpatializerSw::kRanges = {
        MAKE_RANGE(Spatializer, spatializationLevel, Spatialization::Level::NONE,
                   Spatialization::Level::BED_PLUS_OBJECTS),
        MAKE_RANGE(Spatializer, spatializationMode, Spatialization::Mode::BINAURAL,
                   Spatialization::Mode::TRANSAURAL),
        MAKE_RANGE(Spatializer, headTrackingSensorId, std::numeric_limits<int>::min(),
                   std::numeric_limits<int>::max()),
        MAKE_RANGE(Spatializer, headTrackingMode, HeadTracking::Mode::OTHER,
                   HeadTracking::Mode::RELATIVE_SCREEN),
        MAKE_RANGE(Spatializer, headTrackingConnectionMode,
                   HeadTracking::ConnectionMode::FRAMEWORK_PROCESSED,
                   HeadTracking::ConnectionMode::DIRECT_TO_SENSOR_TUNNEL)};
const Capability SpatializerSw::kCapability = {.range = {SpatializerSw::kRanges}};
const Descriptor SpatializerSw::kDescriptor = {
        .common = {.id = {.type = getEffectTypeUuidSpatializer(),
                          .uuid = getEffectImplUuidSpatializerSw()},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .hwAcceleratorMode = Flags::HardwareAccelerator::NONE},
                   .name = SpatializerSw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = SpatializerSw::kCapability};

ndk::ScopedAStatus SpatializerSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus SpatializerSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::spatializer != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto& param = specific.get<Parameter::Specific::spatializer>();
    RETURN_IF(!inRange(param, kRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");

    return mContext->setParam(param.getTag(), param);
}

ndk::ScopedAStatus SpatializerSw::getParameterSpecific(const Parameter::Id& id,
                                                       Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::spatializerTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto spatializerId = id.get<Parameter::Id::spatializerTag>();
    auto spatializerTag = spatializerId.getTag();
    switch (spatializerTag) {
        case Spatializer::Id::commonTag: {
            auto specificTag = spatializerId.get<Spatializer::Id::commonTag>();
            std::optional<Spatializer> param = mContext->getParam(specificTag);
            if (!param.has_value()) {
                return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                        EX_ILLEGAL_ARGUMENT, "SpatializerTagNotSupported");
            }
            specific->set<Parameter::Specific::spatializer>(param.value());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "SpatializerTagNotSupported");
        }
    }
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> SpatializerSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<SpatializerSwContext>(1 /* statusFmqDepth */, common);
    }
    return mContext;
}

RetCode SpatializerSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

SpatializerSw::~SpatializerSw() {
    cleanUp();
    LOG(DEBUG) << __func__;
}

// Processing method running in EffectWorker thread.
IEffect::Status SpatializerSw::effectProcessImpl(float* in, float* out, int samples) {
    RETURN_VALUE_IF(!mContext, (IEffect::Status{EX_NULL_POINTER, 0, 0}), "nullContext");
    return mContext->process(in, out, samples);
}

SpatializerSwContext::SpatializerSwContext(int statusDepth, const Parameter::Common& common)
    : EffectContext(statusDepth, common) {
    LOG(DEBUG) << __func__;
}

SpatializerSwContext::~SpatializerSwContext() {
    LOG(DEBUG) << __func__;
}

template <typename TAG>
std::optional<Spatializer> SpatializerSwContext::getParam(TAG tag) {
    if (mParamsMap.find(tag) != mParamsMap.end()) {
        return mParamsMap.at(tag);
    }
    if (tag == Spatializer::supportedChannelLayout) {
        return Spatializer::make<Spatializer::supportedChannelLayout>(
                {AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                        AudioChannelLayout::LAYOUT_5POINT1)});
    }
    return std::nullopt;
}

template <typename TAG>
ndk::ScopedAStatus SpatializerSwContext::setParam(TAG tag, Spatializer spatializer) {
    RETURN_IF(tag == Spatializer::supportedChannelLayout, EX_ILLEGAL_ARGUMENT,
              "supportedChannelLayoutGetOnly");

    mParamsMap[tag] = spatializer;
    return ndk::ScopedAStatus::ok();
}

IEffect::Status SpatializerSwContext::process(float* in, float* out, int samples) {
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    IEffect::Status status = {EX_ILLEGAL_ARGUMENT, 0, 0};

    const auto inputChannelCount = getChannelCount(mCommon.input.base.channelMask);
    const auto outputChannelCount = getChannelCount(mCommon.output.base.channelMask);
    if (outputChannelCount < 2 || inputChannelCount < outputChannelCount) {
        LOG(ERROR) << __func__ << " invalid channel count, in: " << inputChannelCount
                   << " out: " << outputChannelCount;
        return status;
    }

    int iFrames = samples / inputChannelCount;
    for (int i = 0; i < iFrames; i++) {
        std::memcpy(out, in, outputChannelCount);
        in += inputChannelCount;
        out += outputChannelCount;
    }
    return {STATUS_OK, static_cast<int32_t>(iFrames * inputChannelCount),
            static_cast<int32_t>(iFrames * outputChannelCount)};
}

}  // namespace aidl::android::hardware::audio::effect
