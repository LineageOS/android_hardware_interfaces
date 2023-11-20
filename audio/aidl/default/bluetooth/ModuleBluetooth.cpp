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
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::MicrophoneInfo;
using android::bluetooth::audio::aidl::BluetoothAudioPortAidl;
using android::bluetooth::audio::aidl::BluetoothAudioPortAidlOut;

// TODO(b/312265159) bluetooth audio should be in its own process
// Remove this and the shared_libs when that happens
extern "C" binder_status_t createIBluetoothAudioProviderFactory();

namespace aidl::android::hardware::audio::core {

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

ndk::ScopedAStatus ModuleBluetooth::createInputStream(
        StreamContext&& context, const SinkMetadata& sinkMetadata,
        const std::vector<MicrophoneInfo>& microphones, std::shared_ptr<StreamIn>* result) {
    return createStreamInstance<StreamInBluetooth>(result, std::move(context), sinkMetadata,
                                                   microphones, getBtProfileManagerHandles());
}

ndk::ScopedAStatus ModuleBluetooth::createOutputStream(
        StreamContext&& context, const SourceMetadata& sourceMetadata,
        const std::optional<AudioOffloadInfo>& offloadInfo, std::shared_ptr<StreamOut>* result) {
    return createStreamInstance<StreamOutBluetooth>(result, std::move(context), sourceMetadata,
                                                    offloadInfo, getBtProfileManagerHandles());
}

ndk::ScopedAStatus ModuleBluetooth::populateConnectedDevicePort(AudioPort* audioPort) {
    if (audioPort->ext.getTag() != AudioPortExt::device) {
        LOG(ERROR) << __func__ << ": not a device port: " << audioPort->toString();
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    const auto& devicePort = audioPort->ext.get<AudioPortExt::device>();
    const auto& description = devicePort.device.type;
    // Since the configuration of the BT module is static, there is nothing to populate here.
    // However, this method must return an error when the device can not be connected,
    // this is determined by the status of BT profiles.
    if (description.connection == AudioDeviceDescription::CONNECTION_BT_A2DP) {
        bool isA2dpEnabled = false;
        if (!!mBluetoothA2dp) {
            RETURN_STATUS_IF_ERROR((*mBluetoothA2dp).isEnabled(&isA2dpEnabled));
        }
        LOG(DEBUG) << __func__ << ": isA2dpEnabled: " << isA2dpEnabled;
        return isA2dpEnabled ? ndk::ScopedAStatus::ok()
                             : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    } else if (description.connection == AudioDeviceDescription::CONNECTION_BT_LE) {
        bool isLeEnabled = false;
        if (!!mBluetoothLe) {
            RETURN_STATUS_IF_ERROR((*mBluetoothLe).isEnabled(&isLeEnabled));
        }
        LOG(DEBUG) << __func__ << ": isLeEnabled: " << isLeEnabled;
        return isLeEnabled ? ndk::ScopedAStatus::ok()
                           : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    } else if (description.connection == AudioDeviceDescription::CONNECTION_WIRELESS &&
               description.type == AudioDeviceType::OUT_HEARING_AID) {
        // Hearing aids can use a number of profiles, thus the only way to check
        // connectivity is to try to talk to the BT HAL.
        if (!::aidl::android::hardware::bluetooth::audio::BluetoothAudioSession::
                    IsAidlAvailable()) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
        std::shared_ptr<BluetoothAudioPortAidl> proxy = std::shared_ptr<BluetoothAudioPortAidl>(
                std::make_shared<BluetoothAudioPortAidlOut>());
        if (proxy->registerPort(description)) {
            LOG(DEBUG) << __func__ << ": registered hearing aid port";
            proxy->unregisterPort();
            return ndk::ScopedAStatus::ok();
        }
        LOG(DEBUG) << __func__ << ": failed to register hearing aid port";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    LOG(ERROR) << __func__ << ": unsupported device type: " << audioPort->toString();
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
}

ndk::ScopedAStatus ModuleBluetooth::onMasterMuteChanged(bool) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleBluetooth::onMasterVolumeChanged(float) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace aidl::android::hardware::audio::core
