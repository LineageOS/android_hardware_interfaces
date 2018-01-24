/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.1
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HARDWARE_RADIO_CONFIG_V1_0_RADIOCONFIGINDICATION_H
#define ANDROID_HARDWARE_RADIO_CONFIG_V1_0_RADIOCONFIGINDICATION_H

#include <android/hardware/radio/config/1.0/IRadioConfigIndication.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace radio {
namespace config {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct RadioConfigIndication : public IRadioConfigIndication {
    // Methods from ::android::hardware::radio::config::V1_0::IRadioConfigIndication follow.
    Return<void> simSlotsStatusChanged(
        ::android::hardware::radio::V1_0::RadioIndicationType type,
        const hidl_vec<::android::hardware::radio::config::V1_0::SimSlotStatus>& slotStatus)
        override;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace config
}  // namespace radio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_RADIO_CONFIG_V1_0_RADIOCONFIGINDICATION_H
