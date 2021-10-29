/*
 * Copyright 2021 The Android Open Source Project
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

#define LOG_TAG "BTAudioProviderSessionCodecsDB_2_2"

#include "BluetoothAudioSupportedCodecsDB_2_2.h"

#include <android-base/logging.h>

namespace android {
namespace bluetooth {
namespace audio {

using SessionType_2_1 =
    ::android::hardware::bluetooth::audio::V2_1::SessionType;

bool IsOffloadLeAudioConfigurationValid(
    const ::android::hardware::bluetooth::audio::V2_1::SessionType&
        session_type,
    const ::android::hardware::bluetooth::audio::V2_2::LeAudioConfiguration&) {
  if (session_type !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH &&
      session_type !=
          SessionType_2_1::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
    return false;
  }

  // TODO: perform checks on le_audio_codec_config once we know supported
  // parameters

  return true;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace android
