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

#include "structs.h"

namespace android::nl::protocols::route {

void mapToStream(std::stringstream& ss, const Buffer<nlattr> attr) {
    const auto& [ok, data] = attr.data<rtnl_link_ifmap>().getFirst();
    if (!ok) {
        ss << "invalid structure";
        return;
    }
    ss << '{'                        //
       << data.mem_start << ','      //
       << data.mem_end << ','        //
       << data.base_addr << ','      //
       << data.irq << ','            //
       << unsigned(data.dma) << ','  //
       << unsigned(data.port) << '}';
}

void ifla_cacheinfoToStream(std::stringstream& ss, const Buffer<nlattr> attr) {
    const auto& [ok, data] = attr.data<ifla_cacheinfo>().getFirst();
    if (!ok) {
        ss << "invalid structure";
        return;
    }
    ss << '{'                         //
       << data.max_reasm_len << ','   //
       << data.tstamp << ','          //
       << data.reachable_time << ','  //
       << data.retrans_time << '}';
}

}  // namespace android::nl::protocols::route
