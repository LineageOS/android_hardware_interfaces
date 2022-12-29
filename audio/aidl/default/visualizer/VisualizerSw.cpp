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

#include "VisualizerSw.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::kVisualizerSwImplUUID;
using aidl::android::hardware::audio::effect::State;
using aidl::android::hardware::audio::effect::VisualizerSw;
using aidl::android::media::audio::common::AudioUuid;

extern "C" binder_exception_t createEffect(const AudioUuid* in_impl_uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!in_impl_uuid || *in_impl_uuid != kVisualizerSwImplUUID) {
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
    if (!in_impl_uuid || *in_impl_uuid != kVisualizerSwImplUUID) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    *_aidl_return = VisualizerSw::kDescriptor;
    return EX_NONE;
}

namespace aidl::android::hardware::audio::effect {

const std::string VisualizerSw::kEffectName = "VisualizerSw";
/* capabilities */
const Visualizer::CaptureSamplesRange VisualizerSwContext::kCaptureSamplesRange = {
        VisualizerSwContext::kMinCaptureSize, VisualizerSwContext::kMaxCaptureSize};
const Visualizer::Capability VisualizerSw::kCapability = {
        .maxLatencyMs = VisualizerSwContext::kMaxLatencyMs,
        .captureSampleRange = VisualizerSwContext::kCaptureSamplesRange};

const Descriptor VisualizerSw::kDescriptor = {
        .common = {.id = {.type = kVisualizerTypeUUID,
                          .uuid = kVisualizerSwImplUUID,
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL},
                   .name = VisualizerSw::kEffectName,
                   .implementor = "The Android Open Source Project"},
        .capability = Capability::make<Capability::visualizer>(VisualizerSw::kCapability)};

ndk::ScopedAStatus VisualizerSw::getDescriptor(Descriptor* _aidl_return) {
    LOG(DEBUG) << __func__ << kDescriptor.toString();
    *_aidl_return = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VisualizerSw::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(Parameter::Specific::visualizer != specific.getTag(), EX_ILLEGAL_ARGUMENT,
              "EffectNotSupported");

    auto& vsParam = specific.get<Parameter::Specific::visualizer>();
    auto tag = vsParam.getTag();

    switch (tag) {
        case Visualizer::captureSamples: {
            RETURN_IF(mContext->setVsCaptureSize(vsParam.get<Visualizer::captureSamples>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "captureSizeNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case Visualizer::scalingMode: {
            RETURN_IF(mContext->setVsScalingMode(vsParam.get<Visualizer::scalingMode>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "scalingModeNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case Visualizer::measurementMode: {
            RETURN_IF(mContext->setVsMeasurementMode(vsParam.get<Visualizer::measurementMode>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "measurementModeNotSupported");
            return ndk::ScopedAStatus::ok();
        }
        case Visualizer::setOnlyParameters: {
            return setSetOnlyParameterVisualizer(vsParam.get<Visualizer::setOnlyParameters>());
        }
        case Visualizer::getOnlyParameters: {
            LOG(ERROR) << __func__ << " unsupported settable getOnlyParam";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "SetofGetOnlyParamsNotSupported");
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VisualizerTagNotSupported");
        }
    }
}

ndk::ScopedAStatus VisualizerSw::setSetOnlyParameterVisualizer(
        Visualizer::SetOnlyParameters setOnlyParam) {
    auto tag = setOnlyParam.getTag();
    RETURN_IF(Visualizer::SetOnlyParameters::latencyMs != tag, EX_ILLEGAL_ARGUMENT,
              "SetOnlyParametersTagNotSupported");
    RETURN_IF(
            mContext->setVsLatency(setOnlyParam.get<Visualizer::SetOnlyParameters::latencyMs>()) !=
                    RetCode::SUCCESS,
            EX_ILLEGAL_ARGUMENT, "latencyNotSupported");
    return ndk::ScopedAStatus::ok();
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
        case Visualizer::Id::getOnlyParamTag:
            return getGetOnlyParameterVisualizer(vsId.get<Visualizer::Id::getOnlyParamTag>(),
                                                 specific);
        case Visualizer::Id::setOnlyParamTag: {
            LOG(ERROR) << __func__ << " unsupported gettable setOnlyParam";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "GetofSetOnlyParamsNotSupported");
        }
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
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VisualizerTagNotSupported");
        }
    }
    specific->set<Parameter::Specific::visualizer>(vsParam);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VisualizerSw::getGetOnlyParameterVisualizer(
        const Visualizer::GetOnlyParameters::Tag& tag, Parameter::Specific* specific) {
    Visualizer::GetOnlyParameters getOnlyParam;
    switch (tag) {
        case Visualizer::GetOnlyParameters::measurement: {
            getOnlyParam.set<Visualizer::GetOnlyParameters::measurement>(
                    mContext->getVsMeasurement());
            break;
        }
        case Visualizer::GetOnlyParameters::captureSampleBuffer: {
            getOnlyParam.set<Visualizer::GetOnlyParameters::captureSampleBuffer>(
                    mContext->getVsCaptureSampleBuffer());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "GetOnlyParameterTagNotSupported");
        }
    }
    Visualizer vsParam;
    vsParam.set<Visualizer::getOnlyParameters>(getOnlyParam);
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
    if (captureSize < VisualizerSw::kCapability.captureSampleRange.min ||
        captureSize > VisualizerSw::kCapability.captureSampleRange.max) {
        LOG(ERROR) << __func__ << " invalid captureSize " << captureSize;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    // TODO : Add implementation to apply new captureSize
    mCaptureSize = captureSize;
    return RetCode::SUCCESS;
}

RetCode VisualizerSwContext::setVsScalingMode(Visualizer::ScalingMode scalingMode) {
    // TODO : Add implementation to apply new scalingMode
    mScalingMode = scalingMode;
    return RetCode::SUCCESS;
}

RetCode VisualizerSwContext::setVsMeasurementMode(Visualizer::MeasurementMode measurementMode) {
    // TODO : Add implementation to apply new measurementMode
    mMeasurementMode = measurementMode;
    return RetCode::SUCCESS;
}

RetCode VisualizerSwContext::setVsLatency(int latency) {
    if (latency < 0 || latency > VisualizerSw::kCapability.maxLatencyMs) {
        LOG(ERROR) << __func__ << " invalid latency " << latency;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    // TODO : Add implementation to modify latency
    mLatency = latency;
    return RetCode::SUCCESS;
}

}  // namespace aidl::android::hardware::audio::effect
