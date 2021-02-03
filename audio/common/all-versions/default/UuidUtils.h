/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef android_hardware_audio_Uuid_Utils_H_
#define android_hardware_audio_Uuid_Utils_H_

#include <string>

// clang-format off
#include PATH(android/hardware/audio/common/FILE_VERSION/types.h)
// clang-format on

#include <system/audio.h>

namespace android {
namespace hardware {
namespace audio {
namespace common {
namespace CPP_VERSION {
namespace implementation {

using namespace ::android::hardware::audio::common::CPP_VERSION;

class UuidUtils {
  public:
    static void uuidFromHal(const audio_uuid_t& halUuid, Uuid* uuid);
    static void uuidToHal(const Uuid& uuid, audio_uuid_t* halUuid);
    static std::string uuidToString(const audio_uuid_t& halUuid);
};

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace common
}  // namespace audio
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_audio_Uuid_Utils_H_
