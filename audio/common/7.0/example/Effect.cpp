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

#define LOG_TAG "EffectsFactory7.0"
#include <log/log.h>

#include <android_audio_policy_configuration_V7_0-enums.h>

#include "Effect.h"

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using namespace ::android::hardware::audio::common::V7_0;
// Make an alias for enumerations generated from the APM config XSD.
namespace xsd {
using namespace ::android::audio::policy::configuration::V7_0;
}

namespace android::hardware::audio::effect::V7_0::implementation {

Return<Result> Effect::init() {
    return Result::OK;
}

Return<Result> Effect::setConfig(
        const EffectConfig& config,
        const ::android::sp<IEffectBufferProviderCallback>& inputBufferProvider,
        const ::android::sp<IEffectBufferProviderCallback>& outputBufferProvider) {
    (void)config;
    (void)inputBufferProvider;
    (void)outputBufferProvider;
    return Result::OK;
}

Return<Result> Effect::reset() {
    return Result::OK;
}

Return<Result> Effect::enable() {
    if (!mEnabled) {
        mEnabled = true;
        return Result::OK;
    } else {
        return Result::NOT_SUPPORTED;
    }
}

Return<Result> Effect::disable() {
    if (mEnabled) {
        mEnabled = false;
        return Result::OK;
    } else {
        return Result::NOT_SUPPORTED;
    }
}

Return<Result> Effect::setDevice(const DeviceAddress& device) {
    (void)device;
    return Result::OK;
}

Return<void> Effect::setAndGetVolume(const hidl_vec<uint32_t>& volumes,
                                     setAndGetVolume_cb _hidl_cb) {
    (void)volumes;
    _hidl_cb(Result::OK, hidl_vec<uint32_t>{});
    return Void();
}

Return<Result> Effect::volumeChangeNotification(const hidl_vec<uint32_t>& volumes) {
    (void)volumes;
    return Result::OK;
}

Return<Result> Effect::setAudioMode(AudioMode mode) {
    (void)mode;
    return Result::OK;
}

Return<Result> Effect::setConfigReverse(
        const EffectConfig& config,
        const ::android::sp<IEffectBufferProviderCallback>& inputBufferProvider,
        const ::android::sp<IEffectBufferProviderCallback>& outputBufferProvider) {
    (void)config;
    (void)inputBufferProvider;
    (void)outputBufferProvider;
    return Result::OK;
}

Return<Result> Effect::setInputDevice(const DeviceAddress& device) {
    (void)device;
    return Result::OK;
}

Return<void> Effect::getConfig(getConfig_cb _hidl_cb) {
    EffectConfig config;
    // inputCfg left unspecified.
    config.outputCfg.base.format.value(toString(xsd::AudioFormat::AUDIO_FORMAT_PCM_16_BIT));
    config.outputCfg.base.sampleRateHz.value(48000);
    config.outputCfg.base.channelMask.value(
            toString(xsd::AudioChannelMask::AUDIO_CHANNEL_OUT_STEREO));
    config.outputCfg.accessMode.value(EffectBufferAccess::ACCESS_ACCUMULATE);
    _hidl_cb(Result::OK, config);
    return Void();
}

Return<void> Effect::getConfigReverse(getConfigReverse_cb _hidl_cb) {
    _hidl_cb(Result::OK, EffectConfig{});
    return Void();
}

Return<void> Effect::getSupportedAuxChannelsConfigs(uint32_t maxConfigs,
                                                    getSupportedAuxChannelsConfigs_cb _hidl_cb) {
    (void)maxConfigs;
    _hidl_cb(Result::OK, hidl_vec<EffectAuxChannelsConfig>{});
    return Void();
}

Return<void> Effect::getAuxChannelsConfig(getAuxChannelsConfig_cb _hidl_cb) {
    _hidl_cb(Result::OK, EffectAuxChannelsConfig{});
    return Void();
}

Return<Result> Effect::setAuxChannelsConfig(const EffectAuxChannelsConfig& config) {
    (void)config;
    return Result::OK;
}

Return<Result> Effect::setAudioSource(const hidl_string& source) {
    (void)source;
    return Result::OK;
}

Return<Result> Effect::offload(const EffectOffloadParameter& param) {
    (void)param;
    return Result::OK;
}

Return<void> Effect::getDescriptor(getDescriptor_cb _hidl_cb) {
    _hidl_cb(Result::OK, mDescriptor);
    return Void();
}

Return<void> Effect::prepareForProcessing(prepareForProcessing_cb _hidl_cb) {
    _hidl_cb(Result::OK, MQDescriptor<Result, kSynchronizedReadWrite>{});
    return Void();
}

Return<Result> Effect::setProcessBuffers(const AudioBuffer& inBuffer,
                                         const AudioBuffer& outBuffer) {
    (void)inBuffer;
    (void)outBuffer;
    return Result::OK;
}

Return<void> Effect::command(uint32_t commandId, const hidl_vec<uint8_t>& data,
                             uint32_t resultMaxSize, command_cb _hidl_cb) {
    (void)commandId;
    (void)data;
    (void)resultMaxSize;
    _hidl_cb(-EINVAL, hidl_vec<uint8_t>{});
    return Void();
}

Return<Result> Effect::setParameter(const hidl_vec<uint8_t>& parameter,
                                    const hidl_vec<uint8_t>& value) {
    (void)parameter;
    (void)value;
    return Result::OK;
}

Return<void> Effect::getParameter(const hidl_vec<uint8_t>& parameter, uint32_t valueMaxSize,
                                  getParameter_cb _hidl_cb) {
    (void)parameter;
    (void)valueMaxSize;
    _hidl_cb(Result::OK, hidl_vec<uint8_t>{});
    return Void();
}

Return<void> Effect::getSupportedConfigsForFeature(uint32_t featureId, uint32_t maxConfigs,
                                                   uint32_t configSize,
                                                   getSupportedConfigsForFeature_cb _hidl_cb) {
    (void)featureId;
    (void)maxConfigs;
    (void)configSize;
    _hidl_cb(Result::OK, 0, hidl_vec<uint8_t>{});
    return Void();
}

Return<void> Effect::getCurrentConfigForFeature(uint32_t featureId, uint32_t configSize,
                                                getCurrentConfigForFeature_cb _hidl_cb) {
    (void)featureId;
    (void)configSize;
    _hidl_cb(Result::OK, hidl_vec<uint8_t>{});
    return Void();
}

Return<Result> Effect::setCurrentConfigForFeature(uint32_t featureId,
                                                  const hidl_vec<uint8_t>& configData) {
    (void)featureId;
    (void)configData;
    return Result::OK;
}

Return<Result> Effect::close() {
    return Result::OK;
}

}  // namespace android::hardware::audio::effect::V7_0::implementation
