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

#define LOG_TAG "AHAL_StreamBluetooth"

#include <Utils.h>
#include <android-base/logging.h>
#include <audio_utils/clock.h>

#include "BluetoothAudioSessionControl.h"
#include "core-impl/StreamBluetooth.h"

namespace aidl::android::hardware::audio::core {

using ::aidl::android::hardware::audio::common::SinkMetadata;
using ::aidl::android::hardware::audio::common::SourceMetadata;
using ::aidl::android::hardware::audio::core::VendorParameter;
using ::aidl::android::hardware::bluetooth::audio::ChannelMode;
using ::aidl::android::hardware::bluetooth::audio::PcmConfiguration;
using ::aidl::android::hardware::bluetooth::audio::PresentationPosition;
using ::aidl::android::media::audio::common::AudioChannelLayout;
using ::aidl::android::media::audio::common::AudioDevice;
using ::aidl::android::media::audio::common::AudioDeviceAddress;
using ::aidl::android::media::audio::common::AudioFormatDescription;
using ::aidl::android::media::audio::common::AudioFormatType;
using ::aidl::android::media::audio::common::AudioOffloadInfo;
using ::aidl::android::media::audio::common::MicrophoneDynamicInfo;
using ::aidl::android::media::audio::common::MicrophoneInfo;
using ::android::bluetooth::audio::aidl::BluetoothAudioPortAidl;
using ::android::bluetooth::audio::aidl::BluetoothAudioPortAidlIn;
using ::android::bluetooth::audio::aidl::BluetoothAudioPortAidlOut;
using ::android::bluetooth::audio::aidl::BluetoothStreamState;

constexpr int kBluetoothDefaultInputBufferMs = 20;
constexpr int kBluetoothDefaultOutputBufferMs = 10;
// constexpr int kBluetoothSpatializerOutputBufferMs = 10;

size_t getFrameCount(uint64_t durationUs, uint32_t sampleRate) {
    return (durationUs * sampleRate) / 1000000;
}

// pcm configuration params are not really used by the module
StreamBluetooth::StreamBluetooth(StreamContext* context, const Metadata& metadata,
                                 Module::BtProfileHandles&& btHandles)
    : StreamCommonImpl(context, metadata),
      mSampleRate(getContext().getSampleRate()),
      mChannelLayout(getContext().getChannelLayout()),
      mFormat(getContext().getFormat()),
      mFrameSizeBytes(getContext().getFrameSize()),
      mIsInput(isInput(metadata)),
      mBluetoothA2dp(std::move(std::get<Module::BtInterface::BTA2DP>(btHandles))),
      mBluetoothLe(std::move(std::get<Module::BtInterface::BTLE>(btHandles))) {
    mPreferredDataIntervalUs =
            mIsInput ? kBluetoothDefaultInputBufferMs : kBluetoothDefaultOutputBufferMs;
    mPreferredFrameCount = getFrameCount(mPreferredDataIntervalUs, mSampleRate);
    mIsInitialized = false;
    mIsReadyToClose = false;
}

::android::status_t StreamBluetooth::init() {
    return ::android::OK;  // defering this till we get AudioDeviceDescription
}

const StreamCommonInterface::ConnectedDevices& StreamBluetooth::getConnectedDevices() const {
    std::lock_guard guard(mLock);
    return StreamCommonImpl::getConnectedDevices();
}

ndk::ScopedAStatus StreamBluetooth::setConnectedDevices(
        const std::vector<AudioDevice>& connectedDevices) {
    if (mIsInput && connectedDevices.size() > 1) {
        LOG(ERROR) << __func__ << ": wrong device size(" << connectedDevices.size()
                   << ") for input stream";
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    for (const auto& connectedDevice : connectedDevices) {
        if (connectedDevice.address.getTag() != AudioDeviceAddress::mac) {
            LOG(ERROR) << __func__ << ": bad device address" << connectedDevice.address.toString();
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
    }
    std::lock_guard guard(mLock);
    RETURN_STATUS_IF_ERROR(StreamCommonImpl::setConnectedDevices(connectedDevices));
    mIsInitialized = false;  // updated connected device list, need initialization
    return ndk::ScopedAStatus::ok();
}

::android::status_t StreamBluetooth::drain(StreamDescriptor::DrainMode) {
    usleep(1000);
    return ::android::OK;
}

::android::status_t StreamBluetooth::flush() {
    usleep(1000);
    return ::android::OK;
}

::android::status_t StreamBluetooth::pause() {
    return standby();
}

::android::status_t StreamBluetooth::transfer(void* buffer, size_t frameCount,
                                              size_t* actualFrameCount, int32_t* latencyMs) {
    std::lock_guard guard(mLock);
    if (!mIsInitialized || mIsReadyToClose) {
        // 'setConnectedDevices' has been called or stream is ready to close, so no transfers
        *actualFrameCount = 0;
        *latencyMs = StreamDescriptor::LATENCY_UNKNOWN;
        return ::android::OK;
    }
    *actualFrameCount = 0;
    *latencyMs = 0;
    for (auto proxy : mBtDeviceProxies) {
        if (!proxy->start()) {
            LOG(ERROR) << __func__ << ": state = " << proxy->getState() << " failed to start ";
            return -EIO;
        }
        const size_t fc = std::min(frameCount, mPreferredFrameCount);
        const size_t bytesToTransfer = fc * mFrameSizeBytes;
        if (mIsInput) {
            const size_t totalRead = proxy->readData(buffer, bytesToTransfer);
            *actualFrameCount = std::max(*actualFrameCount, totalRead / mFrameSizeBytes);
        } else {
            const size_t totalWrite = proxy->writeData(buffer, bytesToTransfer);
            *actualFrameCount = std::max(*actualFrameCount, totalWrite / mFrameSizeBytes);
        }
        PresentationPosition presentation_position;
        if (!proxy->getPresentationPosition(presentation_position)) {
            LOG(ERROR) << __func__ << ": getPresentationPosition returned error ";
            return ::android::UNKNOWN_ERROR;
        }
        *latencyMs =
                std::max(*latencyMs, (int32_t)(presentation_position.remoteDeviceAudioDelayNanos /
                                               NANOS_PER_MILLISECOND));
    }
    return ::android::OK;
}

::android::status_t StreamBluetooth::initialize() {
    if (!::aidl::android::hardware::bluetooth::audio::BluetoothAudioSession::IsAidlAvailable()) {
        LOG(ERROR) << __func__ << ": IBluetoothAudioProviderFactory service not available";
        return ::android::UNKNOWN_ERROR;
    }
    if (StreamCommonImpl::getConnectedDevices().empty()) {
        LOG(ERROR) << __func__ << ", has no connected devices";
        return ::android::NO_INIT;
    }
    // unregister older proxies (if any)
    for (auto proxy : mBtDeviceProxies) {
        proxy->stop();
        proxy->unregisterPort();
    }
    mBtDeviceProxies.clear();
    for (auto it = StreamCommonImpl::getConnectedDevices().begin();
         it != StreamCommonImpl::getConnectedDevices().end(); ++it) {
        std::shared_ptr<BluetoothAudioPortAidl> proxy =
                mIsInput ? std::shared_ptr<BluetoothAudioPortAidl>(
                                   std::make_shared<BluetoothAudioPortAidlIn>())
                         : std::shared_ptr<BluetoothAudioPortAidl>(
                                   std::make_shared<BluetoothAudioPortAidlOut>());
        if (proxy->registerPort(it->type)) {
            LOG(ERROR) << __func__ << ": cannot init HAL";
            return ::android::UNKNOWN_ERROR;
        }
        PcmConfiguration config;
        if (!proxy->loadAudioConfig(&config)) {
            LOG(ERROR) << __func__ << ": state=" << proxy->getState()
                       << " failed to get audio config";
            return ::android::UNKNOWN_ERROR;
        }
        // TODO: Ensure minimum duration for spatialized output?
        // WAR to support Mono / 16 bits per sample as the Bluetooth stack required
        if (!mIsInput && config.channelMode == ChannelMode::MONO && config.bitsPerSample == 16) {
            proxy->forcePcmStereoToMono(true);
            config.channelMode = ChannelMode::STEREO;
            LOG(INFO) << __func__ << ": force channels = to be AUDIO_CHANNEL_OUT_STEREO";
        }
        if (!checkConfigParams(config)) {
            LOG(ERROR) << __func__ << " checkConfigParams failed";
            return ::android::UNKNOWN_ERROR;
        }
        mBtDeviceProxies.push_back(std::move(proxy));
    }
    mIsInitialized = true;
    return ::android::OK;
}

bool StreamBluetooth::checkConfigParams(
        ::aidl::android::hardware::bluetooth::audio::PcmConfiguration& config) {
    if ((int)mSampleRate != config.sampleRateHz) {
        LOG(ERROR) << __func__ << ": Sample Rate mismatch, stream val = " << mSampleRate
                   << " hal val = " << config.sampleRateHz;
        return false;
    }
    auto channelCount = aidl::android::hardware::audio::common::getChannelCount(mChannelLayout);
    if ((config.channelMode == ChannelMode::MONO && channelCount != 1) ||
        (config.channelMode == ChannelMode::STEREO && channelCount != 2)) {
        LOG(ERROR) << __func__ << ": Channel count mismatch, stream val = " << channelCount
                   << " hal val = " << toString(config.channelMode);
        return false;
    }
    if (mFormat.type != AudioFormatType::PCM) {
        LOG(ERROR) << __func__ << ": unexpected format type "
                   << aidl::android::media::audio::common::toString(mFormat.type);
        return false;
    }
    int8_t bps = aidl::android::hardware::audio::common::getPcmSampleSizeInBytes(mFormat.pcm) * 8;
    if (bps != config.bitsPerSample) {
        LOG(ERROR) << __func__ << ": bits per sample mismatch, stream val = " << bps
                   << " hal val = " << config.bitsPerSample;
        return false;
    }
    if (config.dataIntervalUs > 0) {
        mPreferredDataIntervalUs =
                std::min((int32_t)mPreferredDataIntervalUs, config.dataIntervalUs);
        mPreferredFrameCount = getFrameCount(mPreferredDataIntervalUs, mSampleRate);
    }
    return true;
}

ndk::ScopedAStatus StreamBluetooth::prepareToClose() {
    std::lock_guard guard(mLock);
    mIsReadyToClose = true;
    return ndk::ScopedAStatus::ok();
}

::android::status_t StreamBluetooth::standby() {
    std::lock_guard guard(mLock);
    if (!mIsInitialized) {
        if (auto status = initialize(); status != ::android::OK) return status;
    }
    for (auto proxy : mBtDeviceProxies) {
        if (!proxy->suspend()) {
            LOG(ERROR) << __func__ << ": state = " << proxy->getState() << " failed to stand by ";
            return -EIO;
        }
    }
    return ::android::OK;
}

::android::status_t StreamBluetooth::start() {
    std::lock_guard guard(mLock);
    if (!mIsInitialized) return initialize();
    return ::android::OK;
}

void StreamBluetooth::shutdown() {
    std::lock_guard guard(mLock);
    for (auto proxy : mBtDeviceProxies) {
        proxy->stop();
        proxy->unregisterPort();
    }
    mBtDeviceProxies.clear();
}

ndk::ScopedAStatus StreamBluetooth::updateMetadataCommon(const Metadata& metadata) {
    std::lock_guard guard(mLock);
    if (!mIsInitialized) return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    bool isOk = true;
    if (isInput(metadata)) {
        isOk = mBtDeviceProxies[0]->updateSinkMetadata(std::get<SinkMetadata>(metadata));
    } else {
        for (auto proxy : mBtDeviceProxies) {
            if (!proxy->updateSourceMetadata(std::get<SourceMetadata>(metadata))) isOk = false;
        }
    }
    return isOk ? ndk::ScopedAStatus::ok()
                : ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamBluetooth::bluetoothParametersUpdated() {
    if (mIsInput) {
        LOG(WARNING) << __func__ << ": not handled";
        return ndk::ScopedAStatus::ok();
    }
    auto applyParam = [](const std::shared_ptr<BluetoothAudioPortAidl>& proxy,
                         bool isEnabled) -> bool {
        if (!isEnabled) {
            if (proxy->suspend()) return proxy->setState(BluetoothStreamState::DISABLED);
            return false;
        }
        return proxy->standby();
    };
    bool hasA2dpParam, enableA2dp;
    auto btA2dp = mBluetoothA2dp.lock();
    hasA2dpParam = btA2dp != nullptr && btA2dp->isEnabled(&enableA2dp).isOk();
    bool hasLeParam, enableLe;
    auto btLe = mBluetoothLe.lock();
    hasLeParam = btLe != nullptr && btLe->isEnabled(&enableLe).isOk();
    std::unique_lock lock(mLock);
    ::android::base::ScopedLockAssertion lock_assertion(mLock);
    if (!mIsInitialized) {
        LOG(WARNING) << __func__ << ": init not done";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    for (auto proxy : mBtDeviceProxies) {
        if ((hasA2dpParam && proxy->isA2dp() && !applyParam(proxy, enableA2dp)) ||
            (hasLeParam && proxy->isLeAudio() && !applyParam(proxy, enableLe))) {
            LOG(DEBUG) << __func__ << ": applyParam failed";
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        }
    }
    return ndk::ScopedAStatus::ok();
}

StreamInBluetooth::StreamInBluetooth(StreamContext&& context, const SinkMetadata& sinkMetadata,
                                     const std::vector<MicrophoneInfo>& microphones,
                                     Module::BtProfileHandles&& btProfileHandles)
    : StreamIn(std::move(context), microphones),
      StreamBluetooth(&mContextInstance, sinkMetadata, std::move(btProfileHandles)) {}

ndk::ScopedAStatus StreamInBluetooth::getActiveMicrophones(
        std::vector<MicrophoneDynamicInfo>* _aidl_return __unused) {
    LOG(DEBUG) << __func__ << ": not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

StreamOutBluetooth::StreamOutBluetooth(StreamContext&& context,
                                       const SourceMetadata& sourceMetadata,
                                       const std::optional<AudioOffloadInfo>& offloadInfo,
                                       Module::BtProfileHandles&& btProfileHandles)
    : StreamOut(std::move(context), offloadInfo),
      StreamBluetooth(&mContextInstance, sourceMetadata, std::move(btProfileHandles)) {}

}  // namespace aidl::android::hardware::audio::core
