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

#include <aidl/android/hardware/broadcastradio/IdentifierType.h>
#include <aidl/android/hardware/broadcastradio/ProgramInfo.h>
#include <aidl/android/hardware/broadcastradio/ProgramSelector.h>

namespace aidl::android::hardware::broadcastradio {

constexpr int kSignalQualityDigital = 100;
constexpr int kSignalQualityNonDigital = 80;
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

}  // namespace aidl::android::hardware::broadcastradio
