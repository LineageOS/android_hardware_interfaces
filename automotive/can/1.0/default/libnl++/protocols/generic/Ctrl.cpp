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

#include "Ctrl.h"

#include "families/Nl80211.h"

#include <libnl++/Message.h>

namespace android::nl::protocols::generic {

using DataType = AttributeDefinition::DataType;
using Flags = AttributeDefinition::Flags;

// clang-format off
Ctrl::Ctrl(Generic::FamilyRegister& familyRegister)
    : GenericMessageBase(GENL_ID_CTRL, "ID_CTRL",
{
    {CTRL_CMD_NEWFAMILY, "NEWFAMILY"},
    {CTRL_CMD_DELFAMILY, "DELFAMILY"},
    {CTRL_CMD_GETFAMILY, "GETFAMILY"},
    {CTRL_CMD_NEWOPS, "NEWOPS"},
    {CTRL_CMD_DELOPS, "DELOPS"},
    {CTRL_CMD_GETOPS, "GETOPS"},
    {CTRL_CMD_NEWMCAST_GRP, "NEWMCAST_GRP"},
    {CTRL_CMD_DELMCAST_GRP, "DELMCAST_GRP"},
    {CTRL_CMD_GETMCAST_GRP, "GETMCAST_GRP"},
}, {
    {CTRL_ATTR_FAMILY_ID, {"FAMILY_ID", DataType::Uint}},
    {CTRL_ATTR_FAMILY_NAME, {"FAMILY_NAME", DataType::StringNul}},
    {CTRL_ATTR_VERSION, {"VERSION", DataType::Uint}},
    {CTRL_ATTR_HDRSIZE, {"HDRSIZE", DataType::Uint}},
    {CTRL_ATTR_MAXATTR, {"MAXATTR", DataType::Uint}},
    {CTRL_ATTR_OPS, {"OPS", DataType::Nested, AttributeMap{
        {std::nullopt, {"OP", DataType::Nested, AttributeMap{
            {CTRL_ATTR_OP_ID, {"ID", DataType::Uint}},
            {CTRL_ATTR_OP_FLAGS, {"FLAGS", DataType::Uint}},
        }}},
    }, Flags::Verbose}},
    {CTRL_ATTR_MCAST_GROUPS, {"MCAST_GROUPS", DataType::Nested, AttributeMap{
        {std::nullopt, {"GRP", DataType::Nested, AttributeMap{
            {CTRL_ATTR_MCAST_GRP_NAME, {"NAME", DataType::StringNul}},
            {CTRL_ATTR_MCAST_GRP_ID, {"ID", DataType::Uint}},
        }}},
    }}},
}), mFamilyRegister(familyRegister) {}
// clang-format on

void Ctrl::track(const Buffer<nlmsghdr> hdr) {
    const auto msgMaybe = Message<genlmsghdr>::parse(hdr, {GENL_ID_CTRL});
    if (!msgMaybe.has_value()) return;
    const auto msg = *msgMaybe;

    if (msg->cmd != CTRL_CMD_NEWFAMILY) return;
    const auto familyId = msg.attributes.get<uint16_t>(CTRL_ATTR_FAMILY_ID);
    const auto familyName = msg.attributes.get<std::string>(CTRL_ATTR_FAMILY_NAME);

    /* For now, we support just a single family. But if you add more, please define proper
     * abstraction and not hardcode every name and class here.
     */
    if (familyName == "nl80211") {
        mFamilyRegister[familyId] = std::make_shared<families::Nl80211>(familyId);
    }
}

}  // namespace android::nl::protocols::generic
