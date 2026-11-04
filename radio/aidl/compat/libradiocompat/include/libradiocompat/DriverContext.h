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

#include <aidl/android/hardware/radio/RadioResponseInfo.h>
#include <aidl/android/hardware/radio/data/DataProfileInfo.h>

#include <map>
#include <mutex>

namespace android::hardware::radio::compat {

class DriverContext {
    std::map<std::string, ::aidl::android::hardware::radio::data::DataProfileInfo> mDataProfiles;

    struct PendingResponses {
        size_t remaining;
        ::aidl::android::hardware::radio::RadioError error;
    };
    std::mutex mSignalCriteriaLock;
    std::map<int32_t, PendingResponses> mSignalCriteriaResponses;

  public:
    void addDataProfile(const ::aidl::android::hardware::radio::data::DataProfileInfo& profile);
    ::aidl::android::hardware::radio::data::DataProfileInfo getDataProfile(const std::string& apn);

    /**
     * HIDL setSignalStrengthReportingCriteria_1_5() only takes a single SignalThresholdInfo, so
     * one AIDL request may be split into several HIDL requests sharing the same serial.
     * Register how many HIDL responses are expected for the given serial.
     */
    void expectSignalCriteriaResponses(int32_t serial, size_t count);

    /**
     * Collect a HIDL setSignalStrengthReportingCriteria response. Returns true if the (merged)
     * response should be forwarded to the AIDL client, i.e. it is the last expected response
     * or the serial is not tracked. The first error seen for the serial is reported.
     */
    bool collectSignalCriteriaResponse(::aidl::android::hardware::radio::RadioResponseInfo& info);
};

}  // namespace android::hardware::radio::compat
