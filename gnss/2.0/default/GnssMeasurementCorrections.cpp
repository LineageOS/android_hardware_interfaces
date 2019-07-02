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

#define LOG_TAG "GnssMeasurementCorrections"

#include "GnssMeasurementCorrections.h"
#include <log/log.h>

namespace android {
namespace hardware {
namespace gnss {
namespace measurement_corrections {
namespace V1_0 {
namespace implementation {

// Methods from V1_0::IMeasurementCorrections follow.
Return<bool> GnssMeasurementCorrections::setCorrections(const MeasurementCorrections& corrections) {
    ALOGD("setCorrections");
    ALOGD("corrections = lat: %f, lng: %f, alt: %f, hUnc: %f, vUnc: %f, toa: %llu, "
          "satCorrections.size: %d",
          corrections.latitudeDegrees, corrections.longitudeDegrees, corrections.altitudeMeters,
          corrections.horizontalPositionUncertaintyMeters,
          corrections.verticalPositionUncertaintyMeters,
          static_cast<unsigned long long>(corrections.toaGpsNanosecondsOfWeek),
          static_cast<int>(corrections.satCorrections.size()));
    for (auto singleSatCorrection : corrections.satCorrections) {
        ALOGD("singleSatCorrection = flags: %d, constellation: %d, svid: %d, cfHz: %f, probLos: %f,"
              " epl: %f, eplUnc: %f",
              static_cast<int>(singleSatCorrection.singleSatCorrectionFlags),
              static_cast<int>(singleSatCorrection.constellation),
              static_cast<int>(singleSatCorrection.svid), singleSatCorrection.carrierFrequencyHz,
              singleSatCorrection.probSatIsLos, singleSatCorrection.excessPathLengthMeters,
              singleSatCorrection.excessPathLengthUncertaintyMeters);
        ALOGD("reflecting plane = lat: %f, lng: %f, alt: %f, azm: %f",
              singleSatCorrection.reflectingPlane.latitudeDegrees,
              singleSatCorrection.reflectingPlane.longitudeDegrees,
              singleSatCorrection.reflectingPlane.altitudeMeters,
              singleSatCorrection.reflectingPlane.azimuthDegrees);
    }

    return true;
}

Return<bool> GnssMeasurementCorrections::setCallback(
        const sp<V1_0::IMeasurementCorrectionsCallback>& callback) {
    using Capabilities = V1_0::IMeasurementCorrectionsCallback::Capabilities;
    auto ret =
            callback->setCapabilitiesCb(Capabilities::LOS_SATS | Capabilities::EXCESS_PATH_LENGTH |
                                        Capabilities::REFLECTING_PLANE);
    if (!ret.isOk()) {
        ALOGE("%s: Unable to invoke callback", __func__);
        return false;
    }
    return true;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace measurement_corrections
}  // namespace gnss
}  // namespace hardware
}  // namespace android
