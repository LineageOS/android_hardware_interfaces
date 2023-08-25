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

#include "Addr.h"

#include "../structs.h"
#include "attributes.h"
#include "structs.h"

namespace android::nl::protocols::route {

using DataType = AttributeDefinition::DataType;

// clang-format off
Addr::Addr() : MessageDefinition<ifaddrmsg>("addr", {
    {RTM_NEWADDR, {"NEWADDR", MessageGenre::New}},
    {RTM_DELADDR, {"DELADDR", MessageGenre::Delete}},
    {RTM_GETADDR, {"GETADDR", MessageGenre::Get}},
}, gAttributes) {}

static const FlagsMap ifaFlagsMap {
    {IFA_F_SECONDARY, "SECONDARY"},
    {IFA_F_NODAD, "NODAD"},
    {IFA_F_OPTIMISTIC, "OPTIMISTIC"},
    {IFA_F_DADFAILED, "DADFAILED"},
    {IFA_F_HOMEADDRESS, "HOMEADDRESS"},
    {IFA_F_DEPRECATED, "DEPRECATED"},
    {IFA_F_TENTATIVE, "TENTATIVE"},
    {IFA_F_PERMANENT, "PERMANENT"},
    {IFA_F_MANAGETEMPADDR, "MANAGETEMPADDR"},
    {IFA_F_NOPREFIXROUTE, "NOPREFIXROUTE"},
    {IFA_F_MCAUTOJOIN, "MCAUTOJOIN"},
    {IFA_F_STABLE_PRIVACY, "STABLE_PRIVACY"},
};
// clang-format on

void Addr::toStream(std::stringstream& ss, const ifaddrmsg& data) const {
    ss << "ifaddrmsg{"
       << "family=" << familyToString(data.ifa_family)
       << ", prefixlen=" << unsigned(data.ifa_prefixlen) << ", flags=";
    flagsToStream(ss, ifaFlagsMap, data.ifa_flags);
    ss << ", scope=" << unsigned(data.ifa_scope) << ", index=" << data.ifa_index << "}";
}

}  // namespace android::nl::protocols::route
