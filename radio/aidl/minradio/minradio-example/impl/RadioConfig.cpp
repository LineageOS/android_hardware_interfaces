/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include "RadioConfig.h"

#include <aidl/android/hardware/radio/sim/CardStatus.h>
#include <libminradio/debug.h>
#include <libminradio/response.h>

#define RADIO_MODULE "ConfigImpl"

namespace android::hardware::radio::service {

using ::android::hardware::radio::minimal::noError;
using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::config;
namespace aidlSim = ::aidl::android::hardware::radio::sim;
constexpr auto ok = &ScopedAStatus::ok;

ScopedAStatus RadioConfig::getSimSlotsStatus(int32_t serial) {
    LOG_CALL;
    aidl::SimSlotStatus simslot1Status{
            .cardState = aidlSim::CardStatus::STATE_PRESENT,
            .atr = "",
            .eid = "eUICC-simslot1",
            .portInfo = {{
                    .iccId = "9868308146200231837",
                    .logicalSlotId = 0,
                    .portActive = true,
            }},
    };
    respond()->getSimSlotsStatusResponse(noError(serial), {simslot1Status});
    return ok();
}

}  // namespace android::hardware::radio::service
