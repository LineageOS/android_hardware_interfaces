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
#include "VirtualProgram.h"

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V1_1 {
namespace implementation {

using V1_0::MetaData;
using V1_0::MetadataKey;
using V1_0::MetadataType;

VirtualProgram::operator ProgramInfo() const {
    ProgramInfo info11 = {};
    auto& info10 = info11.base;

    info10.channel = channel;
    info10.tuned = true;
    info10.stereo = true;
    info10.signalStrength = 100;

    info10.metadata = hidl_vec<MetaData>({
        {MetadataType::TEXT, MetadataKey::RDS_PS, {}, {}, programName, {}},
        {MetadataType::TEXT, MetadataKey::TITLE, {}, {}, songTitle, {}},
        {MetadataType::TEXT, MetadataKey::ARTIST, {}, {}, songArtist, {}},
    });

    return info11;
}

bool operator<(const VirtualProgram& lhs, const VirtualProgram& rhs) {
    return lhs.channel < rhs.channel;
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android
