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

#pragma once

#include <libnl++/Buffer.h>
#include <libnl++/Message.h>

#include <linux/genetlink.h>

#include <optional>

namespace android::nl::generic {

/**
 * Tracker of Netlink family ID registrations.
 */
class FamilyTracker {
  public:
    /**
     * Try parsing NL80211 message.
     *
     * Proper parsing of NL80211 nessages requires prior parsing of control message for Generic
     * Netlink protocol.
     *
     * \param msg Message to parse
     * \returns Parsed NL80211 message or std::nullopt if it wasn't one
     */
    std::optional<Message<genlmsghdr>> parseNl80211(Buffer<nlmsghdr> msg);

  private:
    /* For efficiency, we use a single hardcoded family ID. However, if we supported multiple family
     * types, this should probably be a map.
     */
    std::optional<uint16_t> mNl80211FamilyId;

    /**
     * Track Generic protocol messages.
     *
     * This method is looking for family registration messages.
     *
     * \param msg Message to track
     * \returns True, if the message was a control message (regardless of carrying
     *          family info or not)
     */
    bool track(const Buffer<nlmsghdr>& msg);
};

}  // namespace android::nl::generic
