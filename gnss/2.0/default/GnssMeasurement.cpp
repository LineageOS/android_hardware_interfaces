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
#define LOG_TAG "GnssMeasurement"

#include "GnssMeasurement.h"
#include <log/log.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

using GnssConstellationType = V1_0::GnssConstellationType;
using GnssMeasurementFlags = V1_0::IGnssMeasurementCallback::GnssMeasurementFlags;
using GnssMeasurementState = V1_0::IGnssMeasurementCallback::GnssMeasurementState;

sp<V2_0::IGnssMeasurementCallback> GnssMeasurement::sCallback = nullptr;

GnssMeasurement::GnssMeasurement() : mMinIntervalMillis(1000) {}

GnssMeasurement::~GnssMeasurement() {
    stop();
}

// Methods from V1_0::IGnssMeasurement follow.
Return<V1_0::IGnssMeasurement::GnssMeasurementStatus> GnssMeasurement::setCallback(
    const sp<V1_0::IGnssMeasurementCallback>&) {
    // TODO implement
    return V1_0::IGnssMeasurement::GnssMeasurementStatus{};
}

Return<void> GnssMeasurement::close() {
    std::unique_lock<std::mutex> lock(mMutex);
    stop();
    sCallback = nullptr;
    return Void();
}

// Methods from V1_1::IGnssMeasurement follow.
Return<V1_0::IGnssMeasurement::GnssMeasurementStatus> GnssMeasurement::setCallback_1_1(
    const sp<V1_1::IGnssMeasurementCallback>&, bool) {
    // TODO implement
    return V1_0::IGnssMeasurement::GnssMeasurementStatus{};
}

// Methods from V2_0::IGnssMeasurement follow.
Return<V1_0::IGnssMeasurement::GnssMeasurementStatus> GnssMeasurement::setCallback_2_0(
    const sp<V2_0::IGnssMeasurementCallback>& callback, bool) {
    std::unique_lock<std::mutex> lock(mMutex);
    sCallback = callback;

    if (mIsActive) {
        ALOGW("GnssMeasurement callback already set. Resetting the callback...");
        stop();
    }
    start();

    return V1_0::IGnssMeasurement::GnssMeasurementStatus::SUCCESS;
}

void GnssMeasurement::start() {
    mIsActive = true;
    mThread = std::thread([this]() {
        while (mIsActive == true) {
            auto measurement = this->getMockMeasurement();
            this->reportMeasurement(measurement);

            std::this_thread::sleep_for(std::chrono::milliseconds(mMinIntervalMillis));
        }
    });
}

void GnssMeasurement::stop() {
    mIsActive = false;
    if (mThread.joinable()) {
        mThread.join();
    }
}

GnssData GnssMeasurement::getMockMeasurement() {
    V1_0::IGnssMeasurementCallback::GnssMeasurement measurement_1_0 = {
        .flags = (uint32_t)GnssMeasurementFlags::HAS_CARRIER_FREQUENCY,
        .svid = (int16_t)6,
        .constellation = GnssConstellationType::GLONASS,
        .timeOffsetNs = 0.0,
        .state = GnssMeasurementState::STATE_CODE_LOCK | GnssMeasurementState::STATE_BIT_SYNC |
                 GnssMeasurementState::STATE_SUBFRAME_SYNC |
                 GnssMeasurementState::STATE_TOW_DECODED |
                 GnssMeasurementState::STATE_GLO_STRING_SYNC |
                 GnssMeasurementState::STATE_GLO_TOD_DECODED,
        .receivedSvTimeInNs = 8195997131077,
        .receivedSvTimeUncertaintyInNs = 15,
        .cN0DbHz = 30.0,
        .pseudorangeRateMps = -484.13739013671875,
        .pseudorangeRateUncertaintyMps = 1.0379999876022339,
        .accumulatedDeltaRangeState = (uint32_t)
            V1_0::IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState::ADR_STATE_UNKNOWN,
        .accumulatedDeltaRangeM = 0.0,
        .accumulatedDeltaRangeUncertaintyM = 0.0,
        .carrierFrequencyHz = 1.59975e+09,
        .multipathIndicator =
            V1_0::IGnssMeasurementCallback::GnssMultipathIndicator::INDICATOR_UNKNOWN};
    V1_1::IGnssMeasurementCallback::GnssMeasurement measurement_1_1 = {.v1_0 = measurement_1_0};
    V2_0::IGnssMeasurementCallback::GnssMeasurement measurement_2_0 = {
        .v1_1 = measurement_1_1,
        .codeType = IGnssMeasurementCallback::GnssMeasurementCodeType::CODE_TYPE_C};

    hidl_vec<IGnssMeasurementCallback::GnssMeasurement> measurements(1);
    measurements[0] = measurement_2_0;
    V1_0::IGnssMeasurementCallback::GnssClock clock = {.timeNs = 2713545000000,
                                                       .fullBiasNs = -1226701900521857520,
                                                       .biasNs = 0.59689998626708984,
                                                       .biasUncertaintyNs = 47514.989972114563,
                                                       .driftNsps = -51.757811607455452,
                                                       .driftUncertaintyNsps = 310.64968328491528,
                                                       .hwClockDiscontinuityCount = 1};
    GnssData gnssData = {.measurements = measurements, .clock = clock};
    return gnssData;
}

void GnssMeasurement::reportMeasurement(const GnssData& data) {
    ALOGD("reportMeasurement()");
    std::unique_lock<std::mutex> lock(mMutex);
    if (sCallback == nullptr) {
        ALOGE("%s: GnssMeasurement::sCallback is null.", __func__);
        return;
    }
    sCallback->gnssMeasurementCb_2_0(data);
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
