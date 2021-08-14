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

#include <aidl/android/hardware/gnss/BnGnssMeasurementInterface.h>
#include <android/hardware/gnss/1.0/IGnss.h>
#include <android/hardware/gnss/2.0/IGnss.h>
#include <android/hardware/gnss/2.1/IGnss.h>

using ::android::hardware::hidl_vec;

namespace android {
namespace hardware {
namespace gnss {
namespace common {

struct Utils {
    static aidl::android::hardware::gnss::GnssData getMockMeasurement(
            const bool enableCorrVecOutputs);
    static V2_0::IGnssMeasurementCallback::GnssData getMockMeasurementV2_0();
    static V2_1::IGnssMeasurementCallback::GnssData getMockMeasurementV2_1();
    static V2_0::GnssLocation getMockLocationV2_0();
    static V1_0::GnssLocation getMockLocationV1_0();
    static hidl_vec<V2_1::IGnssCallback::GnssSvInfo> getMockSvInfoListV2_1();
    static V2_1::IGnssCallback::GnssSvInfo getMockSvInfoV2_1(
            V2_0::IGnssCallback::GnssSvInfo gnssSvInfoV2_0, float basebandCN0DbHz);
    static V2_0::IGnssCallback::GnssSvInfo getMockSvInfoV2_0(
            V1_0::IGnssCallback::GnssSvInfo gnssSvInfoV1_0, V2_0::GnssConstellationType type);
    static V1_0::IGnssCallback::GnssSvInfo getMockSvInfoV1_0(int16_t svid,
                                                             V1_0::GnssConstellationType type,
                                                             float cN0DbHz, float elevationDegrees,
                                                             float azimuthDegrees,
                                                             float carrierFrequencyHz);
    static hidl_vec<V2_1::IGnssAntennaInfoCallback::GnssAntennaInfo> getMockAntennaInfos();
};

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_common_default_Utils_H_
