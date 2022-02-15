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
#pragma once

#include "GuaranteedCallback.h"

#include <aidl/android/hardware/radio/config/IRadioConfigIndication.h>
#include <android/hardware/radio/config/1.2/IRadioConfigIndication.h>

namespace android::hardware::radio::compat {

class RadioConfigIndication : public config::V1_2::IRadioConfigIndication {
    GuaranteedCallback<aidl::android::hardware::radio::config::IRadioConfigIndication,
                       aidl::android::hardware::radio::config::IRadioConfigIndicationDefault, true>
            mCallback;

    Return<void> simSlotsStatusChanged(
            V1_0::RadioIndicationType type,
            const hidl_vec<config::V1_0::SimSlotStatus>& slotStatus) override;
    Return<void> simSlotsStatusChanged_1_2(
            V1_0::RadioIndicationType type,
            const hidl_vec<config::V1_2::SimSlotStatus>& slotStatus) override;

  public:
    void setResponseFunction(
            std::shared_ptr<aidl::android::hardware::radio::config::IRadioConfigIndication> cb);

    std::shared_ptr<aidl::android::hardware::radio::config::IRadioConfigIndication> indicate();
};

}  // namespace android::hardware::radio::compat
