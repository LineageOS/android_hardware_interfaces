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

#include <libradiocompat/RadioConfigIndication.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "ConfigIndication"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::config;

void RadioConfigIndication::setResponseFunction(
        std::shared_ptr<aidl::IRadioConfigIndication> callback) {
    mCallback = callback;
}

std::shared_ptr<aidl::IRadioConfigIndication> RadioConfigIndication::indicate() {
    return mCallback.get();
}

Return<void> RadioConfigIndication::simSlotsStatusChanged(
        V1_0::RadioIndicationType type, const hidl_vec<config::V1_0::SimSlotStatus>& slotStatus) {
    LOG_CALL << type;
    indicate()->simSlotsStatusChanged(toAidl(type), toAidl(slotStatus));
    return {};
}

Return<void> RadioConfigIndication::simSlotsStatusChanged_1_2(
        V1_0::RadioIndicationType type, const hidl_vec<config::V1_2::SimSlotStatus>& slotStatus) {
    LOG_CALL << type;
    indicate()->simSlotsStatusChanged(toAidl(type), toAidl(slotStatus));
    return {};
}

}  // namespace android::hardware::radio::compat
