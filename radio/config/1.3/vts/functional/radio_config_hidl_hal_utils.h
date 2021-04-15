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

#pragma once

#include <android-base/logging.h>

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <android/hardware/radio/config/1.1/IRadioConfig.h>
#include <android/hardware/radio/config/1.1/types.h>
#include <android/hardware/radio/config/1.2/IRadioConfigIndication.h>
#include <android/hardware/radio/config/1.2/IRadioConfigResponse.h>
#include <android/hardware/radio/config/1.2/types.h>
#include <android/hardware/radio/config/1.3/IRadioConfig.h>
#include <android/hardware/radio/config/1.3/IRadioConfigResponse.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>

#include "vts_test_util.h"

using namespace ::android::hardware::radio::config::V1_3;

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::radio::config::V1_1::ModemsConfig;
using ::android::hardware::radio::config::V1_1::PhoneCapability;
using ::android::hardware::radio::config::V1_2::IRadioConfigIndication;
using ::android::hardware::radio::config::V1_2::SimSlotStatus;
using ::android::hardware::radio::config::V1_3::IRadioConfig;
using ::android::hardware::radio::config::V1_3::IRadioConfigResponse;
using ::android::hardware::radio::V1_0::RadioResponseInfo;

#define RADIO_SERVICE_NAME "slot1"

class RadioConfigHidlTest;

/* Callback class for radio config response */
class RadioConfigResponse : public IRadioConfigResponse {
  protected:
    RadioResponseWaiter& parent;

  public:
    RadioResponseInfo rspInfo;
    PhoneCapability phoneCap;
    bool modemReducedFeatureSet1;

    RadioConfigResponse(RadioResponseWaiter& parent);
    virtual ~RadioConfigResponse() = default;

    Return<void> getSimSlotsStatusResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<
                    ::android::hardware::radio::config::V1_0::SimSlotStatus>& slotStatus);

    Return<void> getSimSlotsStatusResponse_1_2(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<SimSlotStatus>& slotStatus);

    Return<void> setSimSlotsMappingResponse(const RadioResponseInfo& info);

    Return<void> getPhoneCapabilityResponse(const RadioResponseInfo& info,
                                            const PhoneCapability& phoneCapability);

    Return<void> setPreferredDataModemResponse(const RadioResponseInfo& info);

    Return<void> getModemsConfigResponse(const RadioResponseInfo& info,
                                         const ModemsConfig& mConfig);

    Return<void> setModemsConfigResponse(const RadioResponseInfo& info);

    Return<void> getHalDeviceCapabilitiesResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            bool modemReducedFeatureSet1);
};

/* Callback class for radio config indication */
class RadioConfigIndication : public IRadioConfigIndication {
  protected:
    RadioConfigHidlTest& parent;

  public:
    RadioConfigIndication(RadioConfigHidlTest& parent);
    virtual ~RadioConfigIndication() = default;

    Return<void> simSlotsStatusChanged_1_2(
            ::android::hardware::radio::V1_0::RadioIndicationType type,
            const ::android::hardware::hidl_vec<SimSlotStatus>& slotStatus);
};

// The main test class for Radio config HIDL.
class RadioConfigHidlTest : public ::testing::TestWithParam<std::string>,
                            public RadioResponseWaiter {
  public:
    virtual void SetUp() override;

    void updateSimCardStatus();

    /* radio config service handle */
    sp<IRadioConfig> radioConfig;

    /* radio config response handle */
    sp<RadioConfigResponse> radioConfigRsp;
};
