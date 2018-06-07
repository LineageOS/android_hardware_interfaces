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
#ifndef ANDROID_HARDWARE_BROADCASTRADIO_V2_0_VIRTUALPROGRAM_H
#define ANDROID_HARDWARE_BROADCASTRADIO_V2_0_VIRTUALPROGRAM_H

#include <android/hardware/broadcastradio/2.0/types.h>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V2_0 {
namespace implementation {

/**
 * A radio program mock.
 *
 * This represents broadcast waves flying over the air,
 * not an entry for a captured station in the radio tuner memory.
 */
struct VirtualProgram {
    ProgramSelector selector;

    std::string programName = "";
    std::string songArtist = "";
    std::string songTitle = "";

    operator ProgramInfo() const;

    /**
     * Defines order in which virtual programs appear on the "air" with
     * ITunerSession::scan().
     *
     * It's for default implementation purposes, may not be complete or correct.
     */
    friend bool operator<(const VirtualProgram& lhs, const VirtualProgram& rhs);
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BROADCASTRADIO_V2_0_VIRTUALPROGRAM_H
