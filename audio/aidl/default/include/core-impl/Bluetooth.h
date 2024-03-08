/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <aidl/android/hardware/audio/core/BnBluetooth.h>
#include <aidl/android/hardware/audio/core/BnBluetoothA2dp.h>
#include <aidl/android/hardware/audio/core/BnBluetoothLe.h>

namespace aidl::android::hardware::audio::core {

class ParamChangeHandler {
  public:
    ParamChangeHandler() = default;
    void registerHandler(std::function<ndk::ScopedAStatus()> handler) { mHandler = handler; }

  protected:
    std::function<ndk::ScopedAStatus()> mHandler = nullptr;
};

class Bluetooth : public BnBluetooth {
  public:
    Bluetooth();

  private:
    ndk::ScopedAStatus setScoConfig(const ScoConfig& in_config, ScoConfig* _aidl_return) override;
    ndk::ScopedAStatus setHfpConfig(const HfpConfig& in_config, HfpConfig* _aidl_return) override;

    ScoConfig mScoConfig;
    HfpConfig mHfpConfig;
};

class BluetoothA2dp : public BnBluetoothA2dp, public ParamChangeHandler {
  public:
    BluetoothA2dp() = default;
    ndk::ScopedAStatus isEnabled(bool* _aidl_return) override;

  private:
    ndk::ScopedAStatus setEnabled(bool in_enabled) override;
    ndk::ScopedAStatus supportsOffloadReconfiguration(bool* _aidl_return) override;
    ndk::ScopedAStatus reconfigureOffload(
            const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
                    in_parameters) override;

    bool mEnabled = false;
};

class BluetoothLe : public BnBluetoothLe, public ParamChangeHandler {
  public:
    BluetoothLe() = default;
    ndk::ScopedAStatus isEnabled(bool* _aidl_return) override;

  private:
    ndk::ScopedAStatus setEnabled(bool in_enabled) override;
    ndk::ScopedAStatus supportsOffloadReconfiguration(bool* _aidl_return) override;
    ndk::ScopedAStatus reconfigureOffload(
            const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
                    in_parameters) override;

    bool mEnabled = false;
};

}  // namespace aidl::android::hardware::audio::core
