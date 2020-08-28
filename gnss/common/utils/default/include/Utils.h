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
#include <android/hardware/gnss/2.0/IGnss.h>
#include <android/hardware/gnss/2.1/IGnss.h>

using ::android::hardware::hidl_vec;

namespace android {
namespace hardware {
namespace gnss {
namespace common {

using GnssDataV2_0 = V2_0::IGnssMeasurementCallback::GnssData;
using GnssDataV2_1 = V2_1::IGnssMeasurementCallback::GnssData;
using GnssSvInfoV1_0 = V1_0::IGnssCallback::GnssSvInfo;
using GnssSvInfoV2_0 = V2_0::IGnssCallback::GnssSvInfo;
using GnssSvInfoV2_1 = V2_1::IGnssCallback::GnssSvInfo;
using GnssAntennaInfo = ::android::hardware::gnss::V2_1::IGnssAntennaInfoCallback::GnssAntennaInfo;
using Row = ::android::hardware::gnss::V2_1::IGnssAntennaInfoCallback::Row;
using Coord = ::android::hardware::gnss::V2_1::IGnssAntennaInfoCallback::Coord;

struct Utils {
    static GnssDataV2_0 getMockMeasurementV2_0();
    static GnssDataV2_1 getMockMeasurementV2_1();
    static V2_0::GnssLocation getMockLocationV2_0();
    static V1_0::GnssLocation getMockLocationV1_0();
    static hidl_vec<GnssSvInfoV2_1> getMockSvInfoListV2_1();
    static GnssSvInfoV2_1 getMockSvInfoV2_1(GnssSvInfoV2_0 gnssSvInfoV2_0, float basebandCN0DbHz);
    static GnssSvInfoV2_0 getMockSvInfoV2_0(GnssSvInfoV1_0 gnssSvInfoV1_0,
                                            V2_0::GnssConstellationType type);
    static GnssSvInfoV1_0 getMockSvInfoV1_0(int16_t svid, V1_0::GnssConstellationType type,
                                            float cN0DbHz, float elevationDegrees,
                                            float azimuthDegrees);
    static hidl_vec<GnssAntennaInfo> getMockAntennaInfos();
};

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_common_default_Utils_H_
