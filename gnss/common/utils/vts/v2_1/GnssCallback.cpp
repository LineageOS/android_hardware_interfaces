/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "GnssCallback"

#include "v2_1/GnssCallback.h"
#include <chrono>
#include "Utils.h"

#include <gtest/gtest.h>

using ::android::hardware::gnss::common::Utils;

namespace android::hardware::gnss::common {

GnssCallback::GnssCallback()
    : info_cbq_("system_info"),
      name_cbq_("name"),
      capabilities_cbq_("capabilities"),
      location_cbq_("location"),
      sv_info_list_cbq_("sv_info") {}

Return<void> GnssCallback::gnssSetSystemInfoCb(const IGnssCallback_1_0::GnssSystemInfo& info) {
    ALOGI("Info received, year %d", info.yearOfHw);
    info_cbq_.store(info);
    return Void();
}

Return<void> GnssCallback::gnssSetCapabilitesCb(uint32_t capabilities) {
    ALOGI("Capabilities received %d", capabilities);
    capabilities_cbq_.store(capabilities);
    return Void();
}

Return<void> GnssCallback::gnssSetCapabilitiesCb_2_0(uint32_t capabilities) {
    ALOGI("Capabilities (v2.0) received %d", capabilities);
    capabilities_cbq_.store(capabilities);
    return Void();
}

Return<void> GnssCallback::gnssSetCapabilitiesCb_2_1(uint32_t capabilities) {
    ALOGI("Capabilities (v2.1) received %d", capabilities);
    capabilities_cbq_.store(capabilities);
    return Void();
}

Return<void> GnssCallback::gnssNameCb(const android::hardware::hidl_string& name) {
    ALOGI("Name received: %s", name.c_str());
    name_cbq_.store(name);
    return Void();
}

Return<void> GnssCallback::gnssLocationCb(const GnssLocation_1_0& location) {
    ALOGI("Location received");
    GnssLocation_2_0 location_v2_0;
    location_v2_0.v1_0 = location;
    return gnssLocationCbImpl(location_v2_0);
}

Return<void> GnssCallback::gnssLocationCb_2_0(const GnssLocation_2_0& location) {
    ALOGI("Location (v2.0) received");
    return gnssLocationCbImpl(location);
}

Return<void> GnssCallback::gnssLocationCbImpl(const GnssLocation_2_0& location) {
    location_cbq_.store(location);
    return Void();
}

Return<void> GnssCallback::gnssSvStatusCb(const IGnssCallback_1_0::GnssSvStatus&) {
    ALOGI("gnssSvStatusCb");
    return Void();
}

Return<void> GnssCallback::gnssSvStatusCb_2_1(
        const hidl_vec<IGnssCallback::GnssSvInfo>& svInfoList) {
    ALOGI("gnssSvStatusCb_2_1. Size = %d", (int)svInfoList.size());
    sv_info_list_cbq_.store(svInfoList);
    return Void();
}

}  // namespace android::hardware::gnss::common
