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

#pragma once

#include "core-impl/Bluetooth.h"
#include "core-impl/Module.h"

namespace aidl::android::hardware::audio::core {

class ModuleBluetooth final : public Module {
  public:
    enum BtInterface : int { BTSCO, BTA2DP, BTLE };
    typedef std::tuple<std::weak_ptr<IBluetooth>, std::weak_ptr<IBluetoothA2dp>,
                       std::weak_ptr<IBluetoothLe>>
            BtProfileHandles;

    ModuleBluetooth(std::unique_ptr<Configuration>&& config);

  private:
    ChildInterface<BluetoothA2dp>& getBtA2dp();
    ChildInterface<BluetoothLe>& getBtLe();
    BtProfileHandles getBtProfileManagerHandles();

    ndk::ScopedAStatus getBluetoothA2dp(std::shared_ptr<IBluetoothA2dp>* _aidl_return) override;
    ndk::ScopedAStatus getBluetoothLe(std::shared_ptr<IBluetoothLe>* _aidl_return) override;
    ndk::ScopedAStatus getMicMute(bool* _aidl_return) override;
    ndk::ScopedAStatus setMicMute(bool in_mute) override;

    ndk::ScopedAStatus createInputStream(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones,
            std::shared_ptr<StreamIn>* result) override;
    ndk::ScopedAStatus createOutputStream(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo,
            std::shared_ptr<StreamOut>* result) override;
    ndk::ScopedAStatus populateConnectedDevicePort(
            ::aidl::android::media::audio::common::AudioPort* audioPort) override;
    ndk::ScopedAStatus onMasterMuteChanged(bool mute) override;
    ndk::ScopedAStatus onMasterVolumeChanged(float volume) override;

    ChildInterface<BluetoothA2dp> mBluetoothA2dp;
    ChildInterface<BluetoothLe> mBluetoothLe;
};

}  // namespace aidl::android::hardware::audio::core
