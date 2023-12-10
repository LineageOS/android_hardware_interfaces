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

#include <limits>

#define LOG_TAG "AHAL_StreamPrimary"
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <error/expected_utils.h>

#include "PrimaryMixer.h"
#include "core-impl/StreamPrimary.h"
#include "core-impl/StreamStub.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneInfo;
using android::base::GetBoolProperty;

namespace aidl::android::hardware::audio::core {

StreamPrimary::StreamPrimary(StreamContext* context, const Metadata& metadata)
    : StreamAlsa(context, metadata, 3 /*readWriteRetries*/), mIsInput(isInput(metadata)) {}

std::vector<alsa::DeviceProfile> StreamPrimary::getDeviceProfiles() {
    static const std::vector<alsa::DeviceProfile> kBuiltInSource{
            alsa::DeviceProfile{.card = primary::PrimaryMixer::kAlsaCard,
                                .device = primary::PrimaryMixer::kAlsaDevice,
                                .direction = PCM_IN,
                                .isExternal = false}};
    static const std::vector<alsa::DeviceProfile> kBuiltInSink{
            alsa::DeviceProfile{.card = primary::PrimaryMixer::kAlsaCard,
                                .device = primary::PrimaryMixer::kAlsaDevice,
                                .direction = PCM_OUT,
                                .isExternal = false}};
    return mIsInput ? kBuiltInSource : kBuiltInSink;
}

StreamInPrimary::StreamInPrimary(StreamContext&& context, const SinkMetadata& sinkMetadata,
                                 const std::vector<MicrophoneInfo>& microphones)
    : StreamIn(std::move(context), microphones),
      StreamSwitcher(&mContextInstance, sinkMetadata),
      StreamInHwGainHelper(&mContextInstance) {}

bool StreamInPrimary::useStubStream(const AudioDevice& device) {
    static const bool kSimulateInput =
            GetBoolProperty("ro.boot.audio.tinyalsa.simulate_input", false);
    return kSimulateInput || device.type.type == AudioDeviceType::IN_TELEPHONY_RX ||
           device.type.type == AudioDeviceType::IN_FM_TUNER ||
           device.type.connection == AudioDeviceDescription::CONNECTION_BUS;
}

StreamSwitcher::DeviceSwitchBehavior StreamInPrimary::switchCurrentStream(
        const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) {
    LOG(DEBUG) << __func__;
    if (devices.size() > 1) {
        LOG(ERROR) << __func__ << ": primary stream can only be connected to one device, got: "
                   << devices.size();
        return DeviceSwitchBehavior::UNSUPPORTED_DEVICES;
    }
    if (devices.empty() || useStubStream(devices[0]) == isStubStream()) {
        return DeviceSwitchBehavior::USE_CURRENT_STREAM;
    }
    return DeviceSwitchBehavior::CREATE_NEW_STREAM;
}

std::unique_ptr<StreamCommonInterfaceEx> StreamInPrimary::createNewStream(
        const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
        StreamContext* context, const Metadata& metadata) {
    if (devices.empty()) {
        LOG(FATAL) << __func__ << ": called with empty devices";  // see 'switchCurrentStream'
    }
    if (useStubStream(devices[0])) {
        return std::unique_ptr<StreamCommonInterfaceEx>(
                new InnerStreamWrapper<StreamStub>(context, metadata));
    }
    return std::unique_ptr<StreamCommonInterfaceEx>(
            new InnerStreamWrapper<StreamPrimary>(context, metadata));
}

ndk::ScopedAStatus StreamInPrimary::getHwGain(std::vector<float>* _aidl_return) {
    if (isStubStream()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    return getHwGainImpl(_aidl_return);
}

ndk::ScopedAStatus StreamInPrimary::setHwGain(const std::vector<float>& in_channelGains) {
    if (isStubStream()) {
        LOG(DEBUG) << __func__ << ": gains " << ::android::internal::ToString(in_channelGains);
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    auto currentGains = mHwGains;
    RETURN_STATUS_IF_ERROR(setHwGainImpl(in_channelGains));
    if (in_channelGains.size() < 1) {
        LOG(FATAL) << __func__ << ": unexpected gain vector size: " << in_channelGains.size();
    }
    if (auto status = primary::PrimaryMixer::getInstance().setMicGain(in_channelGains[0]);
        !status.isOk()) {
        mHwGains = currentGains;
        return status;
    }
    return ndk::ScopedAStatus::ok();
}

StreamOutPrimary::StreamOutPrimary(StreamContext&& context, const SourceMetadata& sourceMetadata,
                                   const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamOut(std::move(context), offloadInfo),
      StreamSwitcher(&mContextInstance, sourceMetadata),
      StreamOutHwVolumeHelper(&mContextInstance) {}

bool StreamOutPrimary::useStubStream(const AudioDevice& device) {
    static const bool kSimulateOutput =
            GetBoolProperty("ro.boot.audio.tinyalsa.ignore_output", false);
    return kSimulateOutput || device.type.type == AudioDeviceType::OUT_TELEPHONY_TX ||
           device.type.connection == AudioDeviceDescription::CONNECTION_BUS;
}

StreamSwitcher::DeviceSwitchBehavior StreamOutPrimary::switchCurrentStream(
        const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) {
    LOG(DEBUG) << __func__;
    if (devices.size() > 1) {
        LOG(ERROR) << __func__ << ": primary stream can only be connected to one device, got: "
                   << devices.size();
        return DeviceSwitchBehavior::UNSUPPORTED_DEVICES;
    }
    if (devices.empty() || useStubStream(devices[0]) == isStubStream()) {
        return DeviceSwitchBehavior::USE_CURRENT_STREAM;
    }
    return DeviceSwitchBehavior::CREATE_NEW_STREAM;
}

std::unique_ptr<StreamCommonInterfaceEx> StreamOutPrimary::createNewStream(
        const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
        StreamContext* context, const Metadata& metadata) {
    if (devices.empty()) {
        LOG(FATAL) << __func__ << ": called with empty devices";  // see 'switchCurrentStream'
    }
    if (useStubStream(devices[0])) {
        return std::unique_ptr<StreamCommonInterfaceEx>(
                new InnerStreamWrapper<StreamStub>(context, metadata));
    }
    return std::unique_ptr<StreamCommonInterfaceEx>(
            new InnerStreamWrapper<StreamPrimary>(context, metadata));
}

ndk::ScopedAStatus StreamOutPrimary::getHwVolume(std::vector<float>* _aidl_return) {
    if (isStubStream()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    return getHwVolumeImpl(_aidl_return);
}

ndk::ScopedAStatus StreamOutPrimary::setHwVolume(const std::vector<float>& in_channelVolumes) {
    if (isStubStream()) {
        LOG(DEBUG) << __func__ << ": volumes " << ::android::internal::ToString(in_channelVolumes);
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    auto currentVolumes = mHwVolumes;
    RETURN_STATUS_IF_ERROR(setHwVolumeImpl(in_channelVolumes));
    if (auto status = primary::PrimaryMixer::getInstance().setVolumes(in_channelVolumes);
        !status.isOk()) {
        mHwVolumes = currentVolumes;
        return status;
    }
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::audio::core
