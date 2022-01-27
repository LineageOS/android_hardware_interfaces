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

// A workaround for b/216149583 (vendor code having its own copy of VersionMacro.h)
#ifndef COMMON_TYPES_MINOR_VERSION
#define COMMON_TYPES_MINOR_VERSION MINOR_VERSION
#endif
#ifndef CORE_TYPES_MINOR_VERSION
#define CORE_TYPES_MINOR_VERSION MINOR_VERSION
#endif
#ifndef COMMON_TYPES_FILE_VERSION
#define COMMON_TYPES_FILE_VERSION EXPAND_CONCAT_3(MAJOR_VERSION, ., COMMON_TYPES_MINOR_VERSION)
#endif
#ifndef CORE_TYPES_FILE_VERSION
#define CORE_TYPES_FILE_VERSION EXPAND_CONCAT_3(MAJOR_VERSION, ., CORE_TYPES_MINOR_VERSION)
#endif
#ifndef COMMON_TYPES_CPP_VERSION
#define COMMON_TYPES_CPP_VERSION EXPAND_CONCAT_4(V, MAJOR_VERSION, _, COMMON_TYPES_MINOR_VERSION)
#endif
#ifndef CORE_TYPES_CPP_VERSION
#define CORE_TYPES_CPP_VERSION EXPAND_CONCAT_4(V, MAJOR_VERSION, _, CORE_TYPES_MINOR_VERSION)
#endif
// End of workaround

// clang-format off
#include PATH(android/hardware/audio/common/COMMON_TYPES_FILE_VERSION/types.h)
// clang-format on

namespace android {
namespace hardware {
namespace audio {
namespace common {
namespace CPP_VERSION {
namespace implementation {

#if MAJOR_VERSION == 2
typedef common::CPP_VERSION::AudioDevice AudioDeviceBitfield;
typedef common::CPP_VERSION::AudioChannelMask AudioChannelBitfield;
typedef common::CPP_VERSION::AudioOutputFlag AudioOutputFlagBitfield;
typedef common::CPP_VERSION::AudioInputFlag AudioInputFlagBitfield;
#elif MAJOR_VERSION >= 4 && MAJOR_VERSION <= 6
typedef hidl_bitfield<common::CPP_VERSION::AudioDevice> AudioDeviceBitfield;
typedef hidl_bitfield<common::CPP_VERSION::AudioChannelMask> AudioChannelBitfield;
typedef hidl_bitfield<common::CPP_VERSION::AudioOutputFlag> AudioOutputFlagBitfield;
typedef hidl_bitfield<common::CPP_VERSION::AudioInputFlag> AudioInputFlagBitfield;
#endif

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace common
}  // namespace audio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_AUDIO_EFFECT_VERSION_UTILS_H
