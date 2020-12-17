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

#include <libnl++/generic/FamilyTracker.h>

#include <android-base/logging.h>

namespace android::nl::generic {

bool FamilyTracker::track(const Buffer<nlmsghdr>& buffer) {
    const auto msgMaybe = nl::Message<genlmsghdr>::parse(buffer, {GENL_ID_CTRL});
    if (!msgMaybe.has_value()) return false;

    const auto msg = *msgMaybe;
    if (msg->cmd != CTRL_CMD_NEWFAMILY) return true;

    const auto familyName = msg.attributes.get<std::string>(CTRL_ATTR_FAMILY_NAME);
    const auto familyId = msg.attributes.get<uint16_t>(CTRL_ATTR_FAMILY_ID);

    if (familyId < GENL_START_ALLOC) {
        LOG(WARNING) << "Invalid family ID: " << familyId;
        return true;
    }

    if (familyName == "nl80211") mNl80211FamilyId = familyId;

    return true;
}

std::optional<Message<genlmsghdr>> FamilyTracker::parseNl80211(Buffer<nlmsghdr> msg) {
    if (track(msg)) return std::nullopt;
    if (!mNl80211FamilyId.has_value()) return std::nullopt;

    return nl::Message<genlmsghdr>::parse(msg, {*mNl80211FamilyId});
}

}  // namespace android::nl::generic
