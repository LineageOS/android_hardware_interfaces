/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_AUDIO_EFFECT_VERSION_UTILS_H
#define ANDROID_HARDWARE_AUDIO_EFFECT_VERSION_UTILS_H

#include <android/hardware/audio/common/2.0/types.h>

namespace android {
namespace hardware {
namespace audio {
namespace common {
namespace V2_0 {
namespace implementation {

typedef common::V2_0::AudioDevice AudioDeviceBitfield;
typedef common::V2_0::AudioChannelMask AudioChannelBitfield;
typedef common::V2_0::AudioOutputFlag AudioOutputFlagBitfield;
typedef common::V2_0::AudioInputFlag AudioInputFlagBitfield;

}  // namespace implementation
}  // namespace V2_0
}  // namespace common
}  // namespace audio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_AUDIO_EFFECT_VERSION_UTILS_H
