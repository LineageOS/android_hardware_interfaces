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

#define LOG_TAG "AHAL_ModuleBluetooth"

#include <android-base/logging.h>

#include "BluetoothAudioSession.h"
#include "core-impl/ModuleBluetooth.h"
#include "core-impl/StreamBluetooth.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::hardware::bluetooth::audio::ChannelMode;
using aidl::android::hardware::bluetooth::audio::PcmConfiguration;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioConfigBase;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioIoFlags;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioProfile;
using aidl::android::media::audio::common::Int;
using aidl::android::media::audio::common::MicrophoneInfo;
using aidl::android::media::audio::common::PcmType;
using android::bluetooth::audio::aidl::BluetoothAudioPortAidl;
using android::bluetooth::audio::aidl::BluetoothAudioPortAidlIn;
using android::bluetooth::audio::aidl::BluetoothAudioPortAidlOut;

// TODO(b/312265159) bluetooth audio should be in its own process
// Remove this and the shared_libs when that happens
extern "C" binder_status_t createIBluetoothAudioProviderFactory();

namespace aidl::android::hardware::audio::core {

namespace {

PcmType pcmTypeFromBitsPerSample(int8_t bitsPerSample) {
    if (bitsPerSample == 8)
        return PcmType::UINT_8_BIT;
    else if (bitsPerSample == 16)
        return PcmType::INT_16_BIT;
    else if (bitsPerSample == 24)
        return PcmType::INT_24_BIT;
    else if (bitsPerSample == 32)
        return PcmType::INT_32_BIT;
    ALOGE("Unsupported bitsPerSample: %d", bitsPerSample);
    return PcmType::DEFAULT;
}

AudioChannelLayout channelLayoutFromChannelMode(ChannelMode mode) {
    if (mode == ChannelMode::MONO) {
        return AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                AudioChannelLayout::LAYOUT_MONO);
    } else if (mode == ChannelMode::STEREO || mode == ChannelMode::DUALMONO) {
        return AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                AudioChannelLayout::LAYOUT_STEREO);
    }
    ALOGE("Unsupported channel mode: %s", toString(mode).c_str());
    return AudioChannelLayout{};
}

}  // namespace

ModuleBluetooth::ModuleBluetooth(std::unique_ptr<Module::Configuration>&& config)
    : Module(Type::BLUETOOTH, std::move(config)) {
    // TODO(b/312265159) bluetooth audio should be in its own process
    // Remove this and the shared_libs when that happens
    binder_status_t status = createIBluetoothAudioProviderFactory();
    if (status != STATUS_OK) {
        LOG(ERROR) << "Failed to create bluetooth audio provider factory. Status: "
                   << ::android::statusToString(status);
    }
}

ndk::ScopedAStatus ModuleBluetooth::getBluetoothA2dp(
        std::shared_ptr<IBluetoothA2dp>* _aidl_return) {
    *_aidl_return = getBtA2dp().getInstance();
    LOG(DEBUG) << __func__ << ": returning instance of IBluetoothA2dp: " << _aidl_return->get();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleBluetooth::getBluetoothLe(std::shared_ptr<IBluetoothLe>* _aidl_return) {
    *_aidl_return = getBtLe().getInstance();
    LOG(DEBUG) << __func__ << ": returning instance of IBluetoothLe: " << _aidl_return->get();
    return ndk::ScopedAStatus::ok();
}

ChildInterface<BluetoothA2dp>& ModuleBluetooth::getBtA2dp() {
    if (!mBluetoothA2dp) {
        auto handle = ndk::SharedRefBase::make<BluetoothA2dp>();
        handle->registerHandler(std::bind(&ModuleBluetooth::bluetoothParametersUpdated, this));
        mBluetoothA2dp = handle;
    }
    return mBluetoothA2dp;
}

ChildInterface<BluetoothLe>& ModuleBluetooth::getBtLe() {
    if (!mBluetoothLe) {
        auto handle = ndk::SharedRefBase::make<BluetoothLe>();
        handle->registerHandler(std::bind(&ModuleBluetooth::bluetoothParametersUpdated, this));
        mBluetoothLe = handle;
    }
    return mBluetoothLe;
}

ModuleBluetooth::BtProfileHandles ModuleBluetooth::getBtProfileManagerHandles() {
    return std::make_tuple(std::weak_ptr<IBluetooth>(), getBtA2dp().getPtr(), getBtLe().getPtr());
}

ndk::ScopedAStatus ModuleBluetooth::getMicMute(bool* _aidl_return __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleBluetooth::setMicMute(bool in_mute __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleBluetooth::setAudioPortConfig(const AudioPortConfig& in_requested,
                                                       AudioPortConfig* out_suggested,
                                                       bool* _aidl_return) {
    auto fillConfig = [this](const AudioPort& port, AudioPortConfig* config) {
        if (port.ext.getTag() == AudioPortExt::device) {
            CachedProxy proxy;
            auto status = findOrCreateProxy(port, proxy);
            if (status.isOk()) {
                const auto& pcmConfig = proxy.pcmConfig;
                LOG(DEBUG) << "setAudioPortConfig: suggesting port config from "
                           << pcmConfig.toString();
                const auto pcmType = pcmTypeFromBitsPerSample(pcmConfig.bitsPerSample);
                const auto channelMask = channelLayoutFromChannelMode(pcmConfig.channelMode);
                if (pcmType != PcmType::DEFAULT && channelMask != AudioChannelLayout{}) {
                    config->format =
                            AudioFormatDescription{.type = AudioFormatType::PCM, .pcm = pcmType};
                    config->channelMask = channelMask;
                    config->sampleRate = Int{.value = pcmConfig.sampleRateHz};
                    config->flags = port.flags;
                    config->ext = port.ext;
                    return true;
                }
            }
        }
        return generateDefaultPortConfig(port, config);
    };
    return Module::setAudioPortConfigImpl(in_requested, fillConfig, out_suggested, _aidl_return);
}

ndk::ScopedAStatus ModuleBluetooth::checkAudioPatchEndpointsMatch(
        const std::vector<AudioPortConfig*>& sources, const std::vector<AudioPortConfig*>& sinks) {
    // Both sources and sinks must be non-empty, this is guaranteed by 'setAudioPatch'.
    const bool isInput = sources[0]->ext.getTag() == AudioPortExt::device;
    const int32_t devicePortId = isInput ? sources[0]->portId : sinks[0]->portId;
    const auto proxyIt = mProxies.find(devicePortId);
    if (proxyIt == mProxies.end()) return ndk::ScopedAStatus::ok();
    const auto& pcmConfig = proxyIt->second.pcmConfig;
    const AudioPortConfig* mixPortConfig = isInput ? sinks[0] : sources[0];
    if (!StreamBluetooth::checkConfigParams(
                pcmConfig, AudioConfigBase{.sampleRate = mixPortConfig->sampleRate->value,
                                           .channelMask = *(mixPortConfig->channelMask),
                                           .format = *(mixPortConfig->format)})) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    if (int32_t handle = mixPortConfig->ext.get<AudioPortExt::mix>().handle; handle > 0) {
        mConnections.insert(std::pair(handle, devicePortId));
    }
    return ndk::ScopedAStatus::ok();
}

void ModuleBluetooth::onExternalDeviceConnectionChanged(const AudioPort& audioPort,
                                                        bool connected) {
    if (!connected) mProxies.erase(audioPort.id);
}

ndk::ScopedAStatus ModuleBluetooth::createInputStream(
        StreamContext&& context, const SinkMetadata& sinkMetadata,
        const std::vector<MicrophoneInfo>& microphones, std::shared_ptr<StreamIn>* result) {
    CachedProxy proxy;
    RETURN_STATUS_IF_ERROR(fetchAndCheckProxy(context, proxy));
    return createStreamInstance<StreamInBluetooth>(result, std::move(context), sinkMetadata,
                                                   microphones, getBtProfileManagerHandles(),
                                                   proxy.ptr, proxy.pcmConfig);
}

ndk::ScopedAStatus ModuleBluetooth::createOutputStream(
        StreamContext&& context, const SourceMetadata& sourceMetadata,
        const std::optional<AudioOffloadInfo>& offloadInfo, std::shared_ptr<StreamOut>* result) {
    CachedProxy proxy;
    RETURN_STATUS_IF_ERROR(fetchAndCheckProxy(context, proxy));
    return createStreamInstance<StreamOutBluetooth>(result, std::move(context), sourceMetadata,
                                                    offloadInfo, getBtProfileManagerHandles(),
                                                    proxy.ptr, proxy.pcmConfig);
}

ndk::ScopedAStatus ModuleBluetooth::populateConnectedDevicePort(AudioPort* audioPort,
                                                                int32_t nextPortId) {
    if (audioPort->ext.getTag() != AudioPortExt::device) {
        LOG(ERROR) << __func__ << ": not a device port: " << audioPort->toString();
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (!::aidl::android::hardware::bluetooth::audio::BluetoothAudioSession::IsAidlAvailable()) {
        LOG(ERROR) << __func__ << ": IBluetoothAudioProviderFactory AIDL service not available";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    const auto& devicePort = audioPort->ext.get<AudioPortExt::device>();
    const auto& description = devicePort.device.type;
    // This method must return an error when the device can not be connected.
    if (description.connection == AudioDeviceDescription::CONNECTION_BT_A2DP) {
        bool isA2dpEnabled = false;
        if (!!mBluetoothA2dp) {
            RETURN_STATUS_IF_ERROR((*mBluetoothA2dp).isEnabled(&isA2dpEnabled));
        }
        LOG(DEBUG) << __func__ << ": isA2dpEnabled: " << isA2dpEnabled;
        if (!isA2dpEnabled) return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    } else if (description.connection == AudioDeviceDescription::CONNECTION_BT_LE) {
        bool isLeEnabled = false;
        if (!!mBluetoothLe) {
            RETURN_STATUS_IF_ERROR((*mBluetoothLe).isEnabled(&isLeEnabled));
        }
        LOG(DEBUG) << __func__ << ": isLeEnabled: " << isLeEnabled;
        if (!isLeEnabled) return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    } else if (description.connection == AudioDeviceDescription::CONNECTION_WIRELESS &&
               description.type == AudioDeviceType::OUT_HEARING_AID) {
        // Hearing aids can use a number of profiles, no single switch exists.
    } else {
        LOG(ERROR) << __func__ << ": unsupported device type: " << audioPort->toString();
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    CachedProxy proxy;
    RETURN_STATUS_IF_ERROR(createProxy(*audioPort, nextPortId, proxy));
    // Since the device is already connected and configured by the BT stack, provide
    // the current configuration instead of all possible profiles.
    const auto& pcmConfig = proxy.pcmConfig;
    audioPort->profiles.clear();
    audioPort->profiles.push_back(
            AudioProfile{.format = AudioFormatDescription{.type = AudioFormatType::PCM,
                                                          .pcm = pcmTypeFromBitsPerSample(
                                                                  pcmConfig.bitsPerSample)},
                         .channelMasks = std::vector<AudioChannelLayout>(
                                 {channelLayoutFromChannelMode(pcmConfig.channelMode)}),
                         .sampleRates = std::vector<int>({pcmConfig.sampleRateHz})});
    LOG(DEBUG) << __func__ << ": " << audioPort->toString();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleBluetooth::onMasterMuteChanged(bool) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleBluetooth::onMasterVolumeChanged(float) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

int32_t ModuleBluetooth::getNominalLatencyMs(const AudioPortConfig& portConfig) {
    const auto connectionsIt = mConnections.find(portConfig.ext.get<AudioPortExt::mix>().handle);
    if (connectionsIt != mConnections.end()) {
        const auto proxyIt = mProxies.find(connectionsIt->second);
        if (proxyIt != mProxies.end()) {
            auto proxy = proxyIt->second.ptr;
            size_t dataIntervalUs = 0;
            if (!proxy->getPreferredDataIntervalUs(dataIntervalUs)) {
                LOG(WARNING) << __func__ << ": could not fetch preferred data interval";
            }
            const bool isInput = portConfig.flags->getTag() == AudioIoFlags::input;
            return isInput ? StreamInBluetooth::getNominalLatencyMs(dataIntervalUs)
                           : StreamOutBluetooth::getNominalLatencyMs(dataIntervalUs);
        }
    }
    LOG(ERROR) << __func__ << ": no connection or proxy found for " << portConfig.toString();
    return Module::getNominalLatencyMs(portConfig);
}

ndk::ScopedAStatus ModuleBluetooth::createProxy(const AudioPort& audioPort, int32_t instancePortId,
                                                CachedProxy& proxy) {
    const bool isInput = audioPort.flags.getTag() == AudioIoFlags::input;
    proxy.ptr = isInput ? std::shared_ptr<BluetoothAudioPortAidl>(
                                  std::make_shared<BluetoothAudioPortAidlIn>())
                        : std::shared_ptr<BluetoothAudioPortAidl>(
                                  std::make_shared<BluetoothAudioPortAidlOut>());
    const auto& devicePort = audioPort.ext.get<AudioPortExt::device>();
    const auto device = devicePort.device.type;
    bool registrationSuccess = false;
    for (int i = 0; i < kCreateProxyRetries && !registrationSuccess; ++i) {
        registrationSuccess = proxy.ptr->registerPort(device);
        usleep(kCreateProxyRetrySleepMs * 1000);
    }
    if (!registrationSuccess) {
        LOG(ERROR) << __func__ << ": failed to register BT port for " << device.toString();
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (!proxy.ptr->loadAudioConfig(proxy.pcmConfig)) {
        LOG(ERROR) << __func__ << ": state=" << proxy.ptr->getState()
                   << ", failed to load audio config";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    mProxies.insert(std::pair(instancePortId, proxy));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleBluetooth::fetchAndCheckProxy(const StreamContext& context,
                                                       CachedProxy& proxy) {
    const auto connectionsIt = mConnections.find(context.getMixPortHandle());
    if (connectionsIt != mConnections.end()) {
        const auto proxyIt = mProxies.find(connectionsIt->second);
        if (proxyIt != mProxies.end()) {
            proxy = proxyIt->second;
            mProxies.erase(proxyIt);
        }
        mConnections.erase(connectionsIt);
    }
    if (proxy.ptr != nullptr) {
        if (!StreamBluetooth::checkConfigParams(
                    proxy.pcmConfig, AudioConfigBase{.sampleRate = context.getSampleRate(),
                                                     .channelMask = context.getChannelLayout(),
                                                     .format = context.getFormat()})) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
    }
    // Not having a proxy is OK, it may happen in VTS tests when streams are opened on unconnected
    // mix ports.
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleBluetooth::findOrCreateProxy(const AudioPort& audioPort,
                                                      CachedProxy& proxy) {
    if (auto proxyIt = mProxies.find(audioPort.id); proxyIt != mProxies.end()) {
        proxy = proxyIt->second;
        return ndk::ScopedAStatus::ok();
    }
    return createProxy(audioPort, audioPort.id, proxy);
}

}  // namespace aidl::android::hardware::audio::core
