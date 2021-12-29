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

#pragma once

#include <android/hardware/bluetooth/audio/2.2/types.h>

#include "BluetoothAudioSupportedCodecsDB.h"
#include "BluetoothAudioSupportedCodecsDB_2_1.h"

namespace android {
namespace bluetooth {
namespace audio {

bool IsOffloadLeAudioConfigurationValid(
    const ::android::hardware::bluetooth::audio::V2_1::SessionType&
        session_type,
    const ::android::hardware::bluetooth::audio::V2_2::LeAudioConfiguration&
        le_audio_codec_config);

std::vector<hardware::bluetooth::audio::V2_2::LeAudioCodecCapabilitiesPair>
GetLeAudioOffloadCodecCapabilities(
    const ::android::hardware::bluetooth::audio::V2_1::SessionType&
        session_type);
}  // namespace audio
}  // namespace bluetooth
}  // namespace android
