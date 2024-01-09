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

#define LOG_TAG "HealthHalUtils"

#include <android-base/logging.h>
#include <healthhalutils/HealthHalUtils.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_0 {

// Deprecated. Kept to minimize migration cost.
// The function can be removed once Health 2.1 is removed
// (i.e. compatibility_matrix.7.xml is the lowest supported level).
sp<IHealth> get_health_service() {
    // Health 2.1 requires OEMs to install the
    // implementation to the recovery partition when it is necessary (i.e. on
    // non-A/B devices, where IsBatteryOk() is needed in recovery).
    return IHealth::getService();
}

}  // namespace V2_0
}  // namespace health
}  // namespace hardware
}  // namespace android
