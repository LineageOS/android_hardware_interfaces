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

#pragma once

#include <android/hardware/gnss/1.0/IGnssDebug.h>
#include <hidl/Status.h>

namespace android::hardware::gnss::V1_1::implementation {

/* Interface for GNSS Debug support. */
struct GnssDebug : public V1_0::IGnssDebug {
    /*
     * Methods from ::android::hardware::gnss::V1_0::IGnssDebug follow.
     * These declarations were generated from IGnssDebug.hal.
     */
    Return<void> getDebugData(V1_0::IGnssDebug::getDebugData_cb _hidl_cb) override;
};

}  // namespace android::hardware::gnss::V1_1::implementation
