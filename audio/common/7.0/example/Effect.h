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

#include <android/hardware/audio/effect/7.0/IEffect.h>

namespace android::hardware::audio::effect::V7_0::implementation {

class Effect : public IEffect {
  public:
    explicit Effect(const EffectDescriptor& descriptor) : mDescriptor(descriptor) {}

    ::android::hardware::Return<Result> init() override;
    ::android::hardware::Return<Result> setConfig(
            const EffectConfig& config,
            const ::android::sp<IEffectBufferProviderCallback>& inputBufferProvider,
            const ::android::sp<IEffectBufferProviderCallback>& outputBufferProvider) override;
    ::android::hardware::Return<Result> reset() override;
    ::android::hardware::Return<Result> enable() override;
    ::android::hardware::Return<Result> disable() override;
    ::android::hardware::Return<Result> setDevice(
            const ::android::hardware::audio::common::V7_0::DeviceAddress& device) override;
    ::android::hardware::Return<void> setAndGetVolume(
            const ::android::hardware::hidl_vec<uint32_t>& volumes,
            setAndGetVolume_cb _hidl_cb) override;
    ::android::hardware::Return<Result> volumeChangeNotification(
            const ::android::hardware::hidl_vec<uint32_t>& volumes) override;
    ::android::hardware::Return<Result> setAudioMode(
            ::android::hardware::audio::common::V7_0::AudioMode mode) override;
    ::android::hardware::Return<Result> setConfigReverse(
            const EffectConfig& config,
            const ::android::sp<IEffectBufferProviderCallback>& inputBufferProvider,
            const ::android::sp<IEffectBufferProviderCallback>& outputBufferProvider) override;
    ::android::hardware::Return<Result> setInputDevice(
            const ::android::hardware::audio::common::V7_0::DeviceAddress& device) override;
    ::android::hardware::Return<void> getConfig(getConfig_cb _hidl_cb) override;
    ::android::hardware::Return<void> getConfigReverse(getConfigReverse_cb _hidl_cb) override;
    ::android::hardware::Return<void> getSupportedAuxChannelsConfigs(
            uint32_t maxConfigs, getSupportedAuxChannelsConfigs_cb _hidl_cb) override;
    ::android::hardware::Return<void> getAuxChannelsConfig(
            getAuxChannelsConfig_cb _hidl_cb) override;
    ::android::hardware::Return<Result> setAuxChannelsConfig(
            const EffectAuxChannelsConfig& config) override;
    ::android::hardware::Return<Result> setAudioSource(
            const ::android::hardware::hidl_string& source) override;
    ::android::hardware::Return<Result> offload(const EffectOffloadParameter& param) override;
    ::android::hardware::Return<void> getDescriptor(getDescriptor_cb _hidl_cb) override;
    ::android::hardware::Return<void> prepareForProcessing(
            prepareForProcessing_cb _hidl_cb) override;
    ::android::hardware::Return<Result> setProcessBuffers(const AudioBuffer& inBuffer,
                                                          const AudioBuffer& outBuffer) override;
    ::android::hardware::Return<void> command(uint32_t commandId,
                                              const ::android::hardware::hidl_vec<uint8_t>& data,
                                              uint32_t resultMaxSize, command_cb _hidl_cb) override;
    ::android::hardware::Return<Result> setParameter(
            const ::android::hardware::hidl_vec<uint8_t>& parameter,
            const ::android::hardware::hidl_vec<uint8_t>& value) override;
    ::android::hardware::Return<void> getParameter(
            const ::android::hardware::hidl_vec<uint8_t>& parameter, uint32_t valueMaxSize,
            getParameter_cb _hidl_cb) override;
    ::android::hardware::Return<void> getSupportedConfigsForFeature(
            uint32_t featureId, uint32_t maxConfigs, uint32_t configSize,
            getSupportedConfigsForFeature_cb _hidl_cb) override;
    ::android::hardware::Return<void> getCurrentConfigForFeature(
            uint32_t featureId, uint32_t configSize,
            getCurrentConfigForFeature_cb _hidl_cb) override;
    ::android::hardware::Return<Result> setCurrentConfigForFeature(
            uint32_t featureId, const ::android::hardware::hidl_vec<uint8_t>& configData) override;
    ::android::hardware::Return<Result> close() override;

  private:
    const EffectDescriptor mDescriptor;
    bool mEnabled = false;
};

}  // namespace android::hardware::audio::effect::V7_0::implementation
