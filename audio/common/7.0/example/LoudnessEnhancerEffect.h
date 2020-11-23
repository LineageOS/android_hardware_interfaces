/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <android/hardware/audio/effect/7.0/ILoudnessEnhancerEffect.h>

#include "Effect.h"

namespace android::hardware::audio::effect::V7_0::implementation {

class LoudnessEnhancerEffect : public ILoudnessEnhancerEffect {
  public:
    static const EffectDescriptor& getDescriptor();

    LoudnessEnhancerEffect();

    // Methods from IEffect interface.
    ::android::hardware::Return<Result> init() override { return mEffect->init(); }
    ::android::hardware::Return<Result> setConfig(
            const EffectConfig& config,
            const ::android::sp<IEffectBufferProviderCallback>& inputBufferProvider,
            const ::android::sp<IEffectBufferProviderCallback>& outputBufferProvider) override {
        return mEffect->setConfig(config, inputBufferProvider, outputBufferProvider);
    }
    ::android::hardware::Return<Result> reset() override { return mEffect->reset(); }
    ::android::hardware::Return<Result> enable() override { return mEffect->enable(); }
    ::android::hardware::Return<Result> disable() override { return mEffect->disable(); }
    ::android::hardware::Return<Result> setDevice(
            const ::android::hardware::audio::common::V7_0::DeviceAddress& device) override {
        return mEffect->setDevice(device);
    }
    ::android::hardware::Return<void> setAndGetVolume(
            const ::android::hardware::hidl_vec<uint32_t>& volumes,
            setAndGetVolume_cb _hidl_cb) override {
        return mEffect->setAndGetVolume(volumes, _hidl_cb);
    }
    ::android::hardware::Return<Result> volumeChangeNotification(
            const ::android::hardware::hidl_vec<uint32_t>& volumes) override {
        return mEffect->volumeChangeNotification(volumes);
    }
    ::android::hardware::Return<Result> setAudioMode(
            ::android::hardware::audio::common::V7_0::AudioMode mode) override {
        return mEffect->setAudioMode(mode);
    }
    ::android::hardware::Return<Result> setConfigReverse(
            const EffectConfig& config,
            const ::android::sp<IEffectBufferProviderCallback>& inputBufferProvider,
            const ::android::sp<IEffectBufferProviderCallback>& outputBufferProvider) override {
        return mEffect->setConfigReverse(config, inputBufferProvider, outputBufferProvider);
    }
    ::android::hardware::Return<Result> setInputDevice(
            const ::android::hardware::audio::common::V7_0::DeviceAddress& device) override {
        return mEffect->setInputDevice(device);
    }
    ::android::hardware::Return<void> getConfig(getConfig_cb _hidl_cb) override {
        return mEffect->getConfig(_hidl_cb);
    }
    ::android::hardware::Return<void> getConfigReverse(getConfigReverse_cb _hidl_cb) override {
        return mEffect->getConfigReverse(_hidl_cb);
    }
    ::android::hardware::Return<void> getSupportedAuxChannelsConfigs(
            uint32_t maxConfigs, getSupportedAuxChannelsConfigs_cb _hidl_cb) override {
        return mEffect->getSupportedAuxChannelsConfigs(maxConfigs, _hidl_cb);
    }
    ::android::hardware::Return<void> getAuxChannelsConfig(
            getAuxChannelsConfig_cb _hidl_cb) override {
        return mEffect->getAuxChannelsConfig(_hidl_cb);
    }
    ::android::hardware::Return<Result> setAuxChannelsConfig(
            const EffectAuxChannelsConfig& config) override {
        return mEffect->setAuxChannelsConfig(config);
    }
    ::android::hardware::Return<Result> setAudioSource(
            const ::android::hardware::hidl_string& source) override {
        return mEffect->setAudioSource(source);
    }
    ::android::hardware::Return<Result> offload(const EffectOffloadParameter& param) override {
        return mEffect->offload(param);
    }
    ::android::hardware::Return<void> getDescriptor(getDescriptor_cb _hidl_cb) override {
        return mEffect->getDescriptor(_hidl_cb);
    }
    ::android::hardware::Return<void> prepareForProcessing(
            prepareForProcessing_cb _hidl_cb) override {
        return mEffect->prepareForProcessing(_hidl_cb);
    }
    ::android::hardware::Return<Result> setProcessBuffers(const AudioBuffer& inBuffer,
                                                          const AudioBuffer& outBuffer) override {
        return mEffect->setProcessBuffers(inBuffer, outBuffer);
    }
    ::android::hardware::Return<void> command(uint32_t commandId,
                                              const ::android::hardware::hidl_vec<uint8_t>& data,
                                              uint32_t resultMaxSize,
                                              command_cb _hidl_cb) override {
        return mEffect->command(commandId, data, resultMaxSize, _hidl_cb);
    }
    ::android::hardware::Return<Result> setParameter(
            const ::android::hardware::hidl_vec<uint8_t>& parameter,
            const ::android::hardware::hidl_vec<uint8_t>& value) override {
        return mEffect->setParameter(parameter, value);
    }
    ::android::hardware::Return<void> getParameter(
            const ::android::hardware::hidl_vec<uint8_t>& parameter, uint32_t valueMaxSize,
            getParameter_cb _hidl_cb) override {
        return mEffect->getParameter(parameter, valueMaxSize, _hidl_cb);
    }
    ::android::hardware::Return<void> getSupportedConfigsForFeature(
            uint32_t featureId, uint32_t maxConfigs, uint32_t configSize,
            getSupportedConfigsForFeature_cb _hidl_cb) override {
        return mEffect->getSupportedConfigsForFeature(featureId, maxConfigs, configSize, _hidl_cb);
    }
    ::android::hardware::Return<void> getCurrentConfigForFeature(
            uint32_t featureId, uint32_t configSize,
            getCurrentConfigForFeature_cb _hidl_cb) override {
        return mEffect->getCurrentConfigForFeature(featureId, configSize, _hidl_cb);
    }
    ::android::hardware::Return<Result> setCurrentConfigForFeature(
            uint32_t featureId, const ::android::hardware::hidl_vec<uint8_t>& configData) override {
        return mEffect->setCurrentConfigForFeature(featureId, configData);
    }
    ::android::hardware::Return<Result> close() override { return mEffect->close(); }

    // Methods from ILoudnessEnhancerEffect interface.
    ::android::hardware::Return<Result> setTargetGain(int32_t targetGainMb) override;
    ::android::hardware::Return<void> getTargetGain(getTargetGain_cb _hidl_cb) override;

  private:
    sp<Effect> mEffect;
    int32_t mTargetGainMb = 0;
};

}  // namespace android::hardware::audio::effect::V7_0::implementation
