/*
 * Copyright (C) 2017 The Android Open Source Project
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
#ifndef ANDROID_HARDWARE_BROADCASTRADIO_V1_1_VIRTUALPROGRAM_H
#define ANDROID_HARDWARE_BROADCASTRADIO_V1_1_VIRTUALPROGRAM_H

#include <android/hardware/broadcastradio/1.1/types.h>
#include <cstdint>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V1_1 {
namespace implementation {

struct VirtualProgram {
    ProgramSelector selector;

    std::string programName = "";
    std::string songArtist = "";
    std::string songTitle = "";

    explicit operator ProgramInfo() const;
    friend bool operator<(const VirtualProgram& lhs, const VirtualProgram& rhs);
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BROADCASTRADIO_V1_1_VIRTUALPROGRAM_H
