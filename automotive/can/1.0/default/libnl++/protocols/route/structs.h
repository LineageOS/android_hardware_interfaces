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

#include <linux/rtnetlink.h>

#include <sstream>

namespace android::nl::protocols::route {

// rtnl_link_ifmap
void mapToStream(std::stringstream& ss, const Buffer<nlattr> attr);

// ifla_cacheinfo
void ifla_cacheinfoToStream(std::stringstream& ss, const Buffer<nlattr> attr);

// rtnl_link_stats or rtnl_link_stats64
template <typename T>
void statsToStream(std::stringstream& ss, const Buffer<nlattr> attr) {
    const auto& [ok, data] = attr.data<T>().getFirst();
    if (!ok) {
        ss << "invalid structure";
        return;
    }
    ss << '{'                              //
       << data.rx_packets << ','           //
       << data.tx_packets << ','           //
       << data.rx_bytes << ','             //
       << data.tx_bytes << ','             //
       << data.rx_errors << ','            //
       << data.tx_errors << ','            //
       << data.rx_dropped << ','           //
       << data.tx_dropped << ','           //
       << data.multicast << ','            //
       << data.collisions << ','           //
       << data.rx_length_errors << ','     //
       << data.rx_over_errors << ','       //
       << data.rx_crc_errors << ','        //
       << data.rx_frame_errors << ','      //
       << data.rx_fifo_errors << ','       //
       << data.rx_missed_errors << ','     //
       << data.tx_aborted_errors << ','    //
       << data.tx_carrier_errors << ','    //
       << data.tx_fifo_errors << ','       //
       << data.tx_heartbeat_errors << ','  //
       << data.tx_window_errors << ','     //
       << data.rx_compressed << ','        //
       << data.tx_compressed << ','        //
       << data.rx_nohandler << '}';
}

}  // namespace android::nl::protocols::route
