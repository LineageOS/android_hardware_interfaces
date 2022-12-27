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

#pragma once

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <vector>

#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectUUID.h"

namespace aidl::android::hardware::audio::effect {

class VisualizerSwContext final : public EffectContext {
  public:
    static const int kMinCaptureSize = 0x80;
    static const int kMaxCaptureSize = 0x400;
    static const int kMaxLatencyMs = 3000;
    static const int kMaxCaptureBufSize = 0xffff;
    static const Visualizer::CaptureSamplesRange kCaptureSamplesRange;
    VisualizerSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
        mCaptureSampleBuffer.resize(kMaxCaptureBufSize);
        fill(mCaptureSampleBuffer.begin(), mCaptureSampleBuffer.end(), 0x80);
    }

    RetCode setVsCaptureSize(int captureSize);
    int getVsCaptureSize() const { return mCaptureSize; }

    RetCode setVsScalingMode(Visualizer::ScalingMode scalingMode);
    Visualizer::ScalingMode getVsScalingMode() const { return mScalingMode; }

    RetCode setVsMeasurementMode(Visualizer::MeasurementMode measurementMode);
    Visualizer::MeasurementMode getVsMeasurementMode() const { return mMeasurementMode; }

    RetCode setVsLatency(int latency);

    Visualizer::GetOnlyParameters::Measurement getVsMeasurement() const { return mMeasurement; }
    std::vector<uint8_t> getVsCaptureSampleBuffer() const { return mCaptureSampleBuffer; }

  private:
    int mCaptureSize = kMaxCaptureSize;
    Visualizer::ScalingMode mScalingMode = Visualizer::ScalingMode::NORMALIZED;
    Visualizer::MeasurementMode mMeasurementMode = Visualizer::MeasurementMode::NONE;
    int mLatency = 0;
    const Visualizer::GetOnlyParameters::Measurement mMeasurement = {0, 0};
    std::vector<uint8_t> mCaptureSampleBuffer;
};

class VisualizerSw final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const Visualizer::Capability kCapability;
    static const Descriptor kDescriptor;
    VisualizerSw() { LOG(DEBUG) << __func__; }
    ~VisualizerSw() {
        cleanUp();
        LOG(DEBUG) << __func__;
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;

    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common) override;
    std::shared_ptr<EffectContext> getContext() override;
    RetCode releaseContext() override;

    IEffect::Status effectProcessImpl(float* in, float* out, int samples) override;
    std::string getEffectName() override { return kEffectName; }

  private:
    std::shared_ptr<VisualizerSwContext> mContext;

    ndk::ScopedAStatus setSetOnlyParameterVisualizer(Visualizer::SetOnlyParameters setOnlyParam);
    ndk::ScopedAStatus getParameterVisualizer(const Visualizer::Tag& tag,
                                              Parameter::Specific* specific);
    ndk::ScopedAStatus getGetOnlyParameterVisualizer(const Visualizer::GetOnlyParameters::Tag& tag,
                                                     Parameter::Specific* specific);
};
}  // namespace aidl::android::hardware::audio::effect
