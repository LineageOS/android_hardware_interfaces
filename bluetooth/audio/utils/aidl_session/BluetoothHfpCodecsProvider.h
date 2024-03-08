/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <android-base/logging.h>

#include <vector>

#include "aidl/android/hardware/bluetooth/audio/CodecInfo.h"
#include "aidl_android_hardware_bluetooth_audio_hfp_setting.h"
#include "aidl_android_hardware_bluetooth_audio_hfp_setting_enums.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

using hfp::setting::HfpOffloadSetting;

class BluetoothHfpCodecsProvider {
 public:
  static std::optional<HfpOffloadSetting> ParseFromHfpOffloadSettingFile();

  static std::vector<CodecInfo> GetHfpAudioCodecInfo(
      const std::optional<HfpOffloadSetting>& hfp_offload_setting);
};
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
