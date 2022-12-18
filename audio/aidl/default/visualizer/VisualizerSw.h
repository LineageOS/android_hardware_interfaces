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

#define MIN_CAPTURE_SIZE 128
#define MAX_CAPTURE_SIZE 1024
#define MAX_LATENCY 3000
#define CAPTURE_BUF_SIZE 65536

namespace aidl::android::hardware::audio::effect {

class VisualizerSwContext final : public EffectContext {
  public:
    VisualizerSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
        mCaptureBytes.resize(CAPTURE_BUF_SIZE);
        fill(mCaptureBytes.begin(), mCaptureBytes.end(), 0x80);
    }

    RetCode setVsCaptureSize(int captureSize) {
        if (captureSize < MIN_CAPTURE_SIZE || captureSize > MAX_CAPTURE_SIZE) {
            LOG(ERROR) << __func__ << " invalid captureSize " << captureSize;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new captureSize
        mCaptureSize = captureSize;
        return RetCode::SUCCESS;
    }
    int getVsCaptureSize() const { return mCaptureSize; }

    RetCode setVsScalingMode(Visualizer::ScalingMode scalingMode) {
        // TODO : Add implementation to apply new scalingMode
        mScalingMode = scalingMode;
        return RetCode::SUCCESS;
    }
    Visualizer::ScalingMode getVsScalingMode() const { return mScalingMode; }

    RetCode setVsMeasurementMode(Visualizer::MeasurementMode measurementMode) {
        // TODO : Add implementation to apply new measurementMode
        mMeasurementMode = measurementMode;
        return RetCode::SUCCESS;
    }
    Visualizer::MeasurementMode getVsMeasurementMode() const { return mMeasurementMode; }

    RetCode setVsLatency(int latency) {
        if (latency < 0 || latency > MAX_LATENCY) {
            LOG(ERROR) << __func__ << " invalid latency " << latency;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to modify latency
        mLatency = latency;
        return RetCode::SUCCESS;
    }

    Visualizer::GetOnlyParameters::Measurement getVsMeasurement() const { return mMeasurement; }
    std::vector<uint8_t> getVsCaptureBytes() const { return mCaptureBytes; }

  private:
    int mCaptureSize = MAX_CAPTURE_SIZE;
    Visualizer::ScalingMode mScalingMode = Visualizer::ScalingMode::NORMALIZED;
    Visualizer::MeasurementMode mMeasurementMode = Visualizer::MeasurementMode::NONE;
    int mLatency;
    const Visualizer::GetOnlyParameters::Measurement mMeasurement = {0, 0};
    std::vector<uint8_t> mCaptureBytes;
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
