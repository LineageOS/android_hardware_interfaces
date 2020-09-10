/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef android_hardware_gnss_V1_1_GnssDebug_H_
#define android_hardware_gnss_V1_1_GnssDebug_H_

#include <android/hardware/gnss/1.0/IGnssDebug.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V1_1 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using V1_0::IGnssDebug;

/* Interface for GNSS Debug support. */
struct GnssDebug : public IGnssDebug {
    /*
     * Methods from ::android::hardware::gnss::V1_0::IGnssDebug follow.
     * These declarations were generated from IGnssDebug.hal.
     */
    Return<void> getDebugData(V1_0::IGnssDebug::getDebugData_cb _hidl_cb) override;
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_V1_1_GnssDebug_H_
