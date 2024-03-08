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

#define LOG_TAG "AHAL_VisualizerSw"

#include <android-base/logging.h>
#include <system/audio_effects/effect_uuid.h>

#include "VisualizerSw.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectImplUuidVisualizerSw;
using aidl::android::hardware::audio::effect::getEffectTypeUuidVisualizer;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::State;
using aidl::android::hardware::audio::effect::VisualizerSw;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidVisualizerSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<VisualizerSw>();
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* in_impl_uuid, Descriptor* _aidl_return) {
    if (!in_impl_uuid || *in_impl_uuid != getEffectImplUuidVisualizerSw()) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = VisualizerSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string VisualizerSw::kEffectName = "VisualizerSw";

/* capabilities */
const std::vector<Range::VisualizerRange> VisualizerSw::kRanges = {
        MAKE_RANGE(Visualizer, latencyMs, 0, VisualizerSwContext::kMaxLatencyMs),
        MAKE_RANGE(Visualizer, captureSamples, VisualizerSwContext::kMinCaptureSize,
                   VisualizerSwContext::kMaxCaptureSize)};

const Capability VisualizerSw::kCapability = {
        .range = Range::make<Range::visualizer>(VisualizerSw::kRanges)};

const Descriptor VisualizerSw::kDescriptor = {
        .common = {.id = {.type = getEffectTypeUuidVisualizer(),
                          .uuid = getEffectImplUuidVisualizerSw(),
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::NONE},
                   .name = VisualizerSw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = VisualizerSw::kCapability};

ndk::ScopedAStatus VisualizerSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VisualizerSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::visualizer != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");

    auto& vsParam = specific.get<Parameter::Specific::visualizer>();
    RETURN_IF(!inRange(vsParam, kRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");
    auto tag = vsParam.getTag();

    switch (tag) {
        case Visualizer::captureSamples: {
            RETURN_IF(mContext->setVsCaptureSize(vsParam.get<Visualizer::captureSamples>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setCaptureSizeFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Visualizer::scalingMode: {
            RETURN_IF(mContext->setVsScalingMode(vsParam.get<Visualizer::scalingMode>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setScalingModeFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Visualizer::measurementMode: {
            RETURN_IF(mContext->setVsMeasurementMode(vsParam.get<Visualizer::measurementMode>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setMeasurementModeFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Visualizer::latencyMs: {
            RETURN_IF(mContext->setVsLatency(vsParam.get<Visualizer::latencyMs>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setLatencyFailed");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VisualizerTagNotSupported");
        }
    }
}

ndk::ScopedAStatus VisualizerSw::getParameterSpecific(const Parameter::Id& id,
                                                      Parameter::Specific* specific) {
    auto tag = id.getTag();
    RETURN_IF(Parameter::Id::visualizerTag != tag, EX_ILLEGAL_ARGUMENT, "wrongIdTag");
    auto vsId = id.get<Parameter::Id::visualizerTag>();
    auto vsIdTag = vsId.getTag();
    switch (vsIdTag) {
        case Visualizer::Id::commonTag:
            return getParameterVisualizer(vsId.get<Visualizer::Id::commonTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VisualizerTagNotSupported");
    }
}
ndk::ScopedAStatus VisualizerSw::getParameterVisualizer(const Visualizer::Tag& tag,
                                                        Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    Visualizer vsParam;
    switch (tag) {
        case Visualizer::captureSamples: {
            vsParam.set<Visualizer::captureSamples>(mContext->getVsCaptureSize());
            break;
        }
        case Visualizer::scalingMode: {
            vsParam.set<Visualizer::scalingMode>(mContext->getVsScalingMode());
            break;
        }
        case Visualizer::measurementMode: {
            vsParam.set<Visualizer::measurementMode>(mContext->getVsMeasurementMode());
            break;
        }
        case Visualizer::measurement: {
            vsParam.set<Visualizer::measurement>(mContext->getVsMeasurement());
            break;
        }
        case Visualizer::captureSampleBuffer: {
            vsParam.set<Visualizer::captureSampleBuffer>(mContext->getVsCaptureSampleBuffer());
            break;
        }
        case Visualizer::latencyMs: {
            vsParam.set<Visualizer::latencyMs>(mContext->getVsLatency());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VisualizerTagNotSupported");
        }
    }
    specific->set<Parameter::Specific::visualizer>(vsParam);
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> VisualizerSw::createContext(const Parameter::Common& common) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        mContext = std::make_shared<VisualizerSwContext>(1 /* statusFmqDepth */, common);
    }

    return mContext;
}

std::shared_ptr<EffectContext> VisualizerSw::getContext() {
    return mContext;
}

RetCode VisualizerSw::releaseContext() {
    if (mContext) {
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

// Processing method running in EffectWorker thread.
IEffect::Status VisualizerSw::effectProcessImpl(float* in, float* out, int samples) {
    // TODO: get data buffer and process.
    LOG(DEBUG) << __func__ << " in " << in << " out " << out << " samples " << samples;
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, samples, samples};
}

RetCode VisualizerSwContext::setVsCaptureSize(int captureSize) {
    mCaptureSize = captureSize;
    return RetCode::SUCCESS;
}

RetCode VisualizerSwContext::setVsScalingMode(Visualizer::ScalingMode scalingMode) {
    mScalingMode = scalingMode;
    return RetCode::SUCCESS;
}

RetCode VisualizerSwContext::setVsMeasurementMode(Visualizer::MeasurementMode measurementMode) {
    mMeasurementMode = measurementMode;
    return RetCode::SUCCESS;
}

RetCode VisualizerSwContext::setVsLatency(int latency) {
    mLatency = latency;
    return RetCode::SUCCESS;
}

}  // namespace aidl::android::hardware::audio::effect
