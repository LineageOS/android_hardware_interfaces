/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef android_hardware_gnss_common_default_Utils_H_
#define android_hardware_gnss_common_default_Utils_H_

#include <android/hardware/gnss/1.0/IGnss.h>

using GnssConstellationType = ::android::hardware::gnss::V1_0::GnssConstellationType;
using GnssLocation = ::android::hardware::gnss::V1_0::GnssLocation;
using GnssSvInfo = ::android::hardware::gnss::V1_0::IGnssCallback::GnssSvInfo;

namespace android {
namespace hardware {
namespace gnss {
namespace common {

struct Utils {
    static GnssLocation getMockLocation();
    static GnssSvInfo getSvInfo(int16_t svid, GnssConstellationType type, float cN0DbHz,
                                float elevationDegrees, float azimuthDegrees);
};

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_common_default_Utils_H_
