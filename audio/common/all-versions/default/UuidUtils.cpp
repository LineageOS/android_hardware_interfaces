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

#include "UuidUtils.h"

#include <common/all-versions/VersionUtils.h>
#include <string.h>

namespace android {
namespace hardware {
namespace audio {
namespace common {
namespace CPP_VERSION {
namespace implementation {

void UuidUtils::uuidFromHal(const audio_uuid_t& halUuid, Uuid* uuid) {
    uuid->timeLow = halUuid.timeLow;
    uuid->timeMid = halUuid.timeMid;
    uuid->versionAndTimeHigh = halUuid.timeHiAndVersion;
    uuid->variantAndClockSeqHigh = halUuid.clockSeq;
    memcpy(uuid->node.data(), halUuid.node, uuid->node.size());
}

void UuidUtils::uuidToHal(const Uuid& uuid, audio_uuid_t* halUuid) {
    halUuid->timeLow = uuid.timeLow;
    halUuid->timeMid = uuid.timeMid;
    halUuid->timeHiAndVersion = uuid.versionAndTimeHigh;
    halUuid->clockSeq = uuid.variantAndClockSeqHigh;
    memcpy(halUuid->node, uuid.node.data(), uuid.node.size());
}

std::string UuidUtils::uuidToString(const audio_uuid_t& halUuid) {
    char str[64];
    snprintf(str, sizeof(str), "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x", halUuid.timeLow,
             halUuid.timeMid, halUuid.timeHiAndVersion, halUuid.clockSeq, halUuid.node[0],
             halUuid.node[1], halUuid.node[2], halUuid.node[3], halUuid.node[4], halUuid.node[5]);
    return str;
}

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace common
}  // namespace audio
}  // namespace hardware
}  // namespace android
