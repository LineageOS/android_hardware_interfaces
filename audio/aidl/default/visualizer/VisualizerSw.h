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

#include <vector>

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <system/audio_effects/effect_visualizer.h>
#include "effect-impl/EffectImpl.h"

namespace aidl::android::hardware::audio::effect {

class VisualizerSwContext final : public EffectContext {
  public:
    // need align the min/max capture size to VISUALIZER_CAPTURE_SIZE_MIN and
    // VISUALIZER_CAPTURE_SIZE_MAX because of limitation in audio_utils fixedfft.
    static constexpr int32_t kMinCaptureSize = VISUALIZER_CAPTURE_SIZE_MIN;
    static constexpr int32_t kMaxCaptureSize = VISUALIZER_CAPTURE_SIZE_MAX;
    static constexpr int32_t kMaxLatencyMs = 3000;
    VisualizerSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
        mCaptureSampleBuffer.resize(kMaxCaptureSize);
        fill(mCaptureSampleBuffer.begin(), mCaptureSampleBuffer.end(), 0x80);
    }

    RetCode setVsCaptureSize(int captureSize);
    int getVsCaptureSize() const { return mCaptureSize; }

    RetCode setVsScalingMode(Visualizer::ScalingMode scalingMode);
    Visualizer::ScalingMode getVsScalingMode() const { return mScalingMode; }

    RetCode setVsMeasurementMode(Visualizer::MeasurementMode measurementMode);
    Visualizer::MeasurementMode getVsMeasurementMode() const { return mMeasurementMode; }

    RetCode setVsLatency(int latency);
    int getVsLatency() const { return mLatency; }

    Visualizer::Measurement getVsMeasurement() const { return mMeasurement; }
    std::vector<uint8_t> getVsCaptureSampleBuffer() const { return mCaptureSampleBuffer; }

  private:
    int mCaptureSize = kMaxCaptureSize;
    Visualizer::ScalingMode mScalingMode = Visualizer::ScalingMode::NORMALIZED;
    Visualizer::MeasurementMode mMeasurementMode = Visualizer::MeasurementMode::NONE;
    int mLatency = 0;
    const Visualizer::Measurement mMeasurement = {0, 0};
    std::vector<uint8_t> mCaptureSampleBuffer;
};

class VisualizerSw final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const Capability kCapability;
    static const Descriptor kDescriptor;
    VisualizerSw() { LOG(DEBUG) << __func__; }
    ~VisualizerSw() {
        cleanUp();
        LOG(DEBUG) << __func__;
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific)
            REQUIRES(mImplMutex) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id, Parameter::Specific* specific)
            REQUIRES(mImplMutex) override;

    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common)
            REQUIRES(mImplMutex) override;
    RetCode releaseContext() REQUIRES(mImplMutex) override;

    IEffect::Status effectProcessImpl(float* in, float* out, int samples)
            REQUIRES(mImplMutex) override;
    std::string getEffectName() override { return kEffectName; }

  private:
    static const std::vector<Range::VisualizerRange> kRanges;
    std::shared_ptr<VisualizerSwContext> mContext GUARDED_BY(mImplMutex);
    ndk::ScopedAStatus getParameterVisualizer(const Visualizer::Tag& tag,
                                              Parameter::Specific* specific) REQUIRES(mImplMutex);
};
}  // namespace aidl::android::hardware::audio::effect
