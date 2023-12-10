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

#include "core-impl/ModuleBluetooth.h"
#include "core-impl/StreamBluetooth.h"

namespace aidl::android::hardware::audio::core {

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

ndk::ScopedAStatus ModuleBluetooth::getBluetoothA2dp(
        std::shared_ptr<IBluetoothA2dp>* _aidl_return) {
    if (!mBluetoothA2dp) {
        auto handle = ndk::SharedRefBase::make<BluetoothA2dp>();
        handle->registerHandler(std::bind(&ModuleBluetooth::bluetoothParametersUpdated, this));
        mBluetoothA2dp = handle;
    }
    *_aidl_return = mBluetoothA2dp.getInstance();
    LOG(DEBUG) << __func__ << ": returning instance of IBluetoothA2dp: " << _aidl_return->get();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleBluetooth::getBluetoothLe(std::shared_ptr<IBluetoothLe>* _aidl_return) {
    if (!mBluetoothLe) {
        auto handle = ndk::SharedRefBase::make<BluetoothLe>();
        handle->registerHandler(std::bind(&ModuleBluetooth::bluetoothParametersUpdated, this));
        mBluetoothLe = handle;
    }
    *_aidl_return = mBluetoothLe.getInstance();
    LOG(DEBUG) << __func__ << ": returning instance of IBluetoothLe: " << _aidl_return->get();
    return ndk::ScopedAStatus::ok();
}

Module::BtProfileHandles ModuleBluetooth::getBtProfileManagerHandles() {
    return std::make_tuple(std::weak_ptr<IBluetooth>(), mBluetoothA2dp.getInstance(),
                           mBluetoothLe.getInstance());
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

ndk::ScopedAStatus ModuleBluetooth::onMasterMuteChanged(bool) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleBluetooth::onMasterVolumeChanged(float) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace aidl::android::hardware::audio::core
