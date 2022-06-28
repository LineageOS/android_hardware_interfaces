/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "Mac80211hwsim.h"

#include "../../structs.h"
#include "common.h"

#include <libnl++/generic/families/mac80211_hwsim.h>

namespace android::nl::protocols::generic::families {

using DataType = AttributeDefinition::DataType;
using Flags = AttributeDefinition::Flags;

static void hwsim_tx_rateToStream(std::stringstream& ss, const Buffer<nlattr> attr);

static const FlagsMap txControlFlags{
        {HWSIM_TX_CTL_REQ_TX_STATUS, "REQ_TX"},
        {HWSIM_TX_CTL_NO_ACK, "NO_ACK"},
        {HWSIM_TX_STAT_ACK, "ACK"},
};

// clang-format off
Mac80211hwsim::Mac80211hwsim(nlmsgtype_t familyId) : GenericMessageBase(familyId, "hwsim", {
    {HWSIM_CMD_UNSPEC, "UNSPEC"},
    {HWSIM_CMD_REGISTER, "REGISTER"},
    {HWSIM_CMD_FRAME, "FRAME"},
    {HWSIM_CMD_TX_INFO_FRAME, "TX_INFO_FRAME"},
    {HWSIM_CMD_NEW_RADIO, "NEW_RADIO"},
    {HWSIM_CMD_DEL_RADIO, "DEL_RADIO"},
    {HWSIM_CMD_GET_RADIO, "GET_RADIO"},
    {HWSIM_CMD_ADD_MAC_ADDR, "ADD_MAC_ADDR"},
    {HWSIM_CMD_DEL_MAC_ADDR, "DEL_MAC_ADDR"},
}, {
    {HWSIM_ATTR_UNSPEC, {"UNSPEC"}},
    {HWSIM_ATTR_ADDR_RECEIVER, {"ADDR_RECEIVER", DataType::Struct, hwaddrToStream}},
    {HWSIM_ATTR_ADDR_TRANSMITTER, {"ADDR_TRANSMITTER", DataType::Struct, hwaddrToStream}},
    {HWSIM_ATTR_FRAME, {"FRAME", DataType::Raw, AttributeMap{}, Flags::Verbose}},
    {HWSIM_ATTR_FLAGS, {"FLAGS", DataType::Struct, flagsToStream(txControlFlags)}},
    {HWSIM_ATTR_RX_RATE, {"RX_RATE", DataType::Uint}},
    {HWSIM_ATTR_SIGNAL, {"SIGNAL", DataType::Uint}},
    {HWSIM_ATTR_TX_INFO, {"TX_INFO", DataType::Struct, hwsim_tx_rateToStream}},
    {HWSIM_ATTR_COOKIE, {"COOKIE", DataType::Uint}},
    {HWSIM_ATTR_CHANNELS, {"CHANNELS", DataType::Uint}},
    {HWSIM_ATTR_RADIO_ID, {"RADIO_ID", DataType::Uint}},
    {HWSIM_ATTR_REG_HINT_ALPHA2, {"REG_HINT_ALPHA2", DataType::String}},
    {HWSIM_ATTR_REG_CUSTOM_REG, {"REG_CUSTOM_REG", DataType::Uint}},
    {HWSIM_ATTR_REG_STRICT_REG, {"REG_STRICT_REG", DataType::Flag}},
    {HWSIM_ATTR_SUPPORT_P2P_DEVICE, {"SUPPORT_P2P_DEVICE", DataType::Flag}},
    {HWSIM_ATTR_USE_CHANCTX, {"USE_CHANCTX", DataType::Flag}},
    {HWSIM_ATTR_DESTROY_RADIO_ON_CLOSE, {"DESTROY_RADIO_ON_CLOSE", DataType::Flag}},
    {HWSIM_ATTR_RADIO_NAME, {"RADIO_NAME", DataType::String}},
    {HWSIM_ATTR_NO_VIF, {"NO_VIF", DataType::Flag}},
    {HWSIM_ATTR_FREQ, {"FREQ", DataType::Uint}},
    {HWSIM_ATTR_PAD, {"PAD", DataType::Uint}},
    {HWSIM_ATTR_TX_INFO_FLAGS, {"TX_INFO_FLAGS"}},  // hwsim_tx_rate_flag
    {HWSIM_ATTR_PERM_ADDR, {"PERM_ADDR"}},
    {HWSIM_ATTR_IFTYPE_SUPPORT, {"IFTYPE_SUPPORT", DataType::Uint}},  // NL80211_IFTYPE_STATION etc
    {HWSIM_ATTR_CIPHER_SUPPORT, {"CIPHER_SUPPORT", DataType::Struct, arrayToStream<int32_t>}},
}) {}
// clang-format on

static void hwsim_tx_rateToStream(std::stringstream& ss, const Buffer<nlattr> attr) {
    ss << '{';
    bool first = true;
    for (const auto rate : attr.data<hwsim_tx_rate>().getRaw()) {
        if (rate.idx == -1) continue;

        ss << (int)rate.idx << ": " << (unsigned)rate.count;

        if (!first) ss << ", ";
        first = false;
    }
    ss << '}';
}

}  // namespace android::nl::protocols::generic::families
