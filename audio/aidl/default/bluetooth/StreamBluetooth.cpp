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

#include <inttypes.h>
#include <algorithm>

#define LOG_TAG "AHAL_StreamBluetooth"
#include <Utils.h>
#include <android-base/logging.h>
#include <audio_utils/clock.h>

#include "core-impl/StreamBluetooth.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::hardware::audio::core::VendorParameter;
using aidl::android::hardware::bluetooth::audio::ChannelMode;
using aidl::android::hardware::bluetooth::audio::LatencyMode;
using aidl::android::hardware::bluetooth::audio::PcmConfiguration;
using aidl::android::hardware::bluetooth::audio::PresentationPosition;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioConfigBase;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioLatencyMode;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneDynamicInfo;
using aidl::android::media::audio::common::MicrophoneInfo;
using android::bluetooth::audio::aidl::BluetoothAudioPortAidl;
using android::bluetooth::audio::aidl::BluetoothAudioPortAidlIn;
using android::bluetooth::audio::aidl::BluetoothAudioPortAidlOut;
using android::bluetooth::audio::aidl::BluetoothStreamState;

namespace aidl::android::hardware::audio::core {

constexpr int kBluetoothDefaultInputBufferMs = 20;
constexpr int kBluetoothDefaultOutputBufferMs = 10;
// constexpr int kBluetoothSpatializerOutputBufferMs = 10;
constexpr int kBluetoothDefaultRemoteDelayMs = 200;

namespace {

LatencyMode audio2bt_LatencyMode(AudioLatencyMode mode) {
    switch (mode) {
        case AudioLatencyMode::FREE:
            return LatencyMode::FREE;
        case AudioLatencyMode::LOW:
            return LatencyMode::LOW_LATENCY;
        case AudioLatencyMode::DYNAMIC_SPATIAL_AUDIO_SOFTWARE:
            return LatencyMode::DYNAMIC_SPATIAL_AUDIO_SOFTWARE;
        case AudioLatencyMode::DYNAMIC_SPATIAL_AUDIO_HARDWARE:
            return LatencyMode::DYNAMIC_SPATIAL_AUDIO_HARDWARE;
    }
    return LatencyMode::UNKNOWN;
}

std::optional<AudioLatencyMode> bt2audio_LatencyMode(LatencyMode mode) {
    switch (mode) {
        case LatencyMode::UNKNOWN:
            return std::nullopt;
        case LatencyMode::FREE:
            return AudioLatencyMode::FREE;
        case LatencyMode::LOW_LATENCY:
            return AudioLatencyMode::LOW;
        case LatencyMode::DYNAMIC_SPATIAL_AUDIO_SOFTWARE:
            return AudioLatencyMode::DYNAMIC_SPATIAL_AUDIO_SOFTWARE;
        case LatencyMode::DYNAMIC_SPATIAL_AUDIO_HARDWARE:
            return AudioLatencyMode::DYNAMIC_SPATIAL_AUDIO_HARDWARE;
    }
    return std::nullopt;
}

std::vector<AudioLatencyMode> bt2audio_LatencyModes(const std::vector<LatencyMode>& modes) {
    std::vector<AudioLatencyMode> result;
    for (const auto m : modes) {
        if (const auto alm = bt2audio_LatencyMode(m); alm.has_value()) {
            result.push_back(*alm);
        }
    }
    return result;
}

}  // namespace

void PortCallbacksHandler::onRecommendedLatencyModeChanged(const std::vector<LatencyMode>& modes) {
    mStreamCallback->onRecommendedLatencyModeChanged(bt2audio_LatencyModes(modes));
}

StreamBluetooth::StreamBluetooth(StreamContext* context, const Metadata& metadata,
                                 ModuleBluetooth::BtProfileHandles&& btHandles,
                                 const std::shared_ptr<BluetoothAudioPortAidl>& btDeviceProxy,
                                 const PcmConfiguration& pcmConfig)
    : StreamCommonImpl(context, metadata),
      mFrameSizeBytes(getContext().getFrameSize()),
      mIsInput(isInput(metadata)),
      mBluetoothA2dp(std::move(std::get<ModuleBluetooth::BtInterface::BTA2DP>(btHandles))),
      mBluetoothLe(std::move(std::get<ModuleBluetooth::BtInterface::BTLE>(btHandles))),
      mPreferredDataIntervalUs(pcmConfig.dataIntervalUs != 0
                                       ? pcmConfig.dataIntervalUs
                                       : (mIsInput ? kBluetoothDefaultInputBufferMs
                                                   : kBluetoothDefaultOutputBufferMs) *
                                                 1000),
      mCallbacksHandler(new PortCallbacksHandler(getContext().getOutEventCallback())),
      mBtDeviceProxy(btDeviceProxy) {
    if (mBtDeviceProxy != nullptr) {
        mSessionTypeName = mBtDeviceProxy->getSessionNameForDebug();
        if (mCallbacksHandler->hasCallback()) {
            mBtDeviceProxy->setCallbacks(mCallbacksHandler);
        }
    }
}

StreamBluetooth::~StreamBluetooth() {
    cleanupWorker();
}

::android::status_t StreamBluetooth::init(DriverCallbackInterface*) {
    std::lock_guard guard(mLock);
    if (mBtDeviceProxy == nullptr) {
        // This is a normal situation in VTS tests.
        LOG(INFO) << __func__ << ": no BT HAL proxy, stream is non-functional";
    }
    LOG(INFO) << __func__ << ": preferred data interval (us): " << mPreferredDataIntervalUs;
    return ::android::OK;
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
    if (!mEnabled) {
        *actualFrameCount = frameCount;
        *latencyMs = kBluetoothDefaultRemoteDelayMs;
        usleep((float)(frameCount * 1000000) / (float)getContext().getSampleRate());
        return ::android::OK;
    }
    *actualFrameCount = 0;
    *latencyMs = StreamDescriptor::LATENCY_UNKNOWN;
    if (mBtDeviceProxy == nullptr || !mBtDeviceProxy->start()) {
        // The BT session is turned down.
        return ::android::OK;
    }
    *latencyMs = 0;
    const size_t bytesToTransfer = frameCount * mFrameSizeBytes;
    const size_t bytesTransferred = mIsInput ? mBtDeviceProxy->readData(buffer, bytesToTransfer)
                                             : mBtDeviceProxy->writeData(buffer, bytesToTransfer);
    *actualFrameCount = bytesTransferred / mFrameSizeBytes;
    PresentationPosition presentation_position;
    if (!mBtDeviceProxy->getPresentationPosition(presentation_position)) {
        presentation_position.remoteDeviceAudioDelayNanos =
                kBluetoothDefaultRemoteDelayMs * NANOS_PER_MILLISECOND;
        LOG(WARNING) << __func__ << ": getPresentationPosition failed, latency info is unavailable";
    }
    // TODO(b/317117580): incorporate logic from
    //                    packages/modules/Bluetooth/system/audio_bluetooth_hw/stream_apis.cc
    //                    out_calculate_feeding_delay_ms / in_calculate_starving_delay_ms
    *latencyMs = std::max(*latencyMs, (int32_t)(presentation_position.remoteDeviceAudioDelayNanos /
                                                NANOS_PER_MILLISECOND));
    return ::android::OK;
}

// static
bool StreamBluetooth::checkConfigParams(const PcmConfiguration& pcmConfig,
                                        const AudioConfigBase& config) {
    if ((int)config.sampleRate != pcmConfig.sampleRateHz) {
        LOG(ERROR) << __func__ << ": sample rate mismatch, stream value=" << config.sampleRate
                   << ", BT HAL value=" << pcmConfig.sampleRateHz;
        return false;
    }
    const auto channelCount =
            aidl::android::hardware::audio::common::getChannelCount(config.channelMask);
    if ((pcmConfig.channelMode == ChannelMode::MONO && channelCount != 1) ||
        (pcmConfig.channelMode == ChannelMode::STEREO && channelCount != 2)) {
        LOG(ERROR) << __func__ << ": Channel count mismatch, stream value=" << channelCount
                   << ", BT HAL value=" << toString(pcmConfig.channelMode);
        return false;
    }
    if (config.format.type != AudioFormatType::PCM) {
        LOG(ERROR) << __func__
                   << ": unexpected stream format type: " << toString(config.format.type);
        return false;
    }
    const int8_t bitsPerSample =
            aidl::android::hardware::audio::common::getPcmSampleSizeInBytes(config.format.pcm) * 8;
    if (bitsPerSample != pcmConfig.bitsPerSample) {
        LOG(ERROR) << __func__ << ": bits per sample mismatch, stream value=" << bitsPerSample
                   << ", BT HAL value=" << pcmConfig.bitsPerSample;
        return false;
    }
    return true;
}

ndk::ScopedAStatus StreamBluetooth::prepareToClose() {
    std::lock_guard guard(mLock);
    if (mBtDeviceProxy != nullptr) {
        mBtDeviceProxy->stop();
    }
    return ndk::ScopedAStatus::ok();
}

::android::status_t StreamBluetooth::standby() {
    std::lock_guard guard(mLock);
    if (mBtDeviceProxy != nullptr) mBtDeviceProxy->suspend();
    return ::android::OK;
}

::android::status_t StreamBluetooth::start() {
    std::lock_guard guard(mLock);
    if (!mEnabled) {
        return ::android::OK;
    }
    if (mBtDeviceProxy != nullptr) mBtDeviceProxy->start();
    return ::android::OK;
}

void StreamBluetooth::shutdown() {
    std::lock_guard guard(mLock);
    if (mBtDeviceProxy != nullptr) {
        mBtDeviceProxy->stop();
        mBtDeviceProxy = nullptr;
    }
}

ndk::ScopedAStatus StreamBluetooth::updateMetadataCommon(const Metadata& metadata) {
    std::lock_guard guard(mLock);
    if (mBtDeviceProxy == nullptr) {
        return ndk::ScopedAStatus::ok();
    }
    bool isOk = true;
    if (isInput(metadata)) {
        isOk = mBtDeviceProxy->updateSinkMetadata(std::get<SinkMetadata>(metadata));
    } else {
        isOk = mBtDeviceProxy->updateSourceMetadata(std::get<SourceMetadata>(metadata));
    }
    return isOk ? ndk::ScopedAStatus::ok()
                : ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamBluetooth::bluetoothParametersUpdated() {
    if (mIsInput) {
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
    std::lock_guard guard(mLock);
    if (mBtDeviceProxy != nullptr) {
        if ((hasA2dpParam && mBtDeviceProxy->isA2dp() && !applyParam(mBtDeviceProxy, enableA2dp)) ||
            (hasLeParam && mBtDeviceProxy->isLeAudio() && !applyParam(mBtDeviceProxy, enableLe))) {
            LOG(DEBUG) << __func__ << ": applyParam failed";
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        }
        mEnabled = (mBtDeviceProxy->isA2dp() && enableA2dp) ||
                   (mBtDeviceProxy->isLeAudio() && enableLe);
        LOG(INFO) << __func__ << ": mEnabled: " << mEnabled;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamBluetooth::getRecommendedLatencyModes(
        std::vector<AudioLatencyMode>* _aidl_return) {
    LOG(DEBUG) << __func__;
    std::vector<LatencyMode> modes;
    std::lock_guard guard(mLock);
    if (!mBtDeviceProxy || !mBtDeviceProxy->getRecommendedLatencyModes(&modes)) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    *_aidl_return = bt2audio_LatencyModes(modes);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamBluetooth::setLatencyMode(AudioLatencyMode in_mode) {
    LOG(DEBUG) << __func__ << ": " << toString(in_mode);
    std::lock_guard guard(mLock);
    if (mBtDeviceProxy != nullptr) {
        return mBtDeviceProxy->setLatencyMode(audio2bt_LatencyMode(in_mode))
                       ? ndk::ScopedAStatus::ok()
                       : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
}

void StreamBluetooth::dump(int fd, const char** args, uint32_t numArgs) {
    const int indent = 4;
    if (::aidl::android::hardware::audio::common::hasArgument(
                args, numArgs,
                ::aidl::android::hardware::audio::common::kDumpFromAudioServerArgument)) {
        // Just provide the frames count.
        dprintf(fd, "%*sFrames transferred: %" PRId64 "\n", indent, "",
                getContext().getFrameCount());
        return;
    }
    dprintf(fd, "%*sI/O handle %d:\n", indent, "", getContext().getMixPortHandle());
    dprintf(fd, "%*sFrames transferred: %" PRId64 "\n", indent + 2, "",
            getContext().getFrameCount());
    dprintf(fd, "%*sSession type: %s\n", indent + 2, "", mSessionTypeName.c_str());
    dprintf(fd, "%*sSample rate: %d\n", indent + 2, "", getContext().getSampleRate());
}

// static
int32_t StreamInBluetooth::getNominalLatencyMs(size_t dataIntervalUs) {
    if (dataIntervalUs == 0) dataIntervalUs = kBluetoothDefaultInputBufferMs * 1000LL;
    return dataIntervalUs / 1000LL;
}

StreamInBluetooth::StreamInBluetooth(StreamContext&& context, const SinkMetadata& sinkMetadata,
                                     const std::vector<MicrophoneInfo>& microphones,
                                     ModuleBluetooth::BtProfileHandles&& btProfileHandles,
                                     const std::shared_ptr<BluetoothAudioPortAidl>& btDeviceProxy,
                                     const PcmConfiguration& pcmConfig)
    : StreamIn(std::move(context), microphones),
      StreamBluetooth(&mContextInstance, sinkMetadata, std::move(btProfileHandles), btDeviceProxy,
                      pcmConfig) {}

ndk::ScopedAStatus StreamInBluetooth::getActiveMicrophones(
        std::vector<MicrophoneDynamicInfo>* _aidl_return __unused) {
    LOG(DEBUG) << __func__ << ": not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

binder_status_t StreamInBluetooth::dump(int fd, const char** args, uint32_t numArgs) {
    StreamBluetooth::dump(fd, args, numArgs);
    return ::android::OK;
}

// static
int32_t StreamOutBluetooth::getNominalLatencyMs(size_t dataIntervalUs) {
    if (dataIntervalUs == 0) dataIntervalUs = kBluetoothDefaultOutputBufferMs * 1000LL;
    return dataIntervalUs / 1000LL;
}

StreamOutBluetooth::StreamOutBluetooth(StreamContext&& context,
                                       const SourceMetadata& sourceMetadata,
                                       const std::optional<AudioOffloadInfo>& offloadInfo,
                                       ModuleBluetooth::BtProfileHandles&& btProfileHandles,
                                       const std::shared_ptr<BluetoothAudioPortAidl>& btDeviceProxy,
                                       const PcmConfiguration& pcmConfig)
    : StreamOut(std::move(context), offloadInfo),
      StreamBluetooth(&mContextInstance, sourceMetadata, std::move(btProfileHandles), btDeviceProxy,
                      pcmConfig) {}

ndk::ScopedAStatus StreamOutBluetooth::getRecommendedLatencyModes(
        std::vector<AudioLatencyMode>* _aidl_return) {
    return StreamBluetooth::getRecommendedLatencyModes(_aidl_return);
}

ndk::ScopedAStatus StreamOutBluetooth::setLatencyMode(AudioLatencyMode in_mode) {
    return StreamBluetooth::setLatencyMode(in_mode);
}

binder_status_t StreamOutBluetooth::dump(int fd, const char** args, uint32_t numArgs) {
    StreamBluetooth::dump(fd, args, numArgs);
    return ::android::OK;
}

}  // namespace aidl::android::hardware::audio::core
