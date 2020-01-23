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

#include <android-base/logging.h>

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <android/hardware/radio/config/1.3/IRadioConfig.h>
#include <android/hardware/radio/config/1.3/IRadioConfigIndication.h>
#include <android/hardware/radio/config/1.3/IRadioConfigResponse.h>
#include <android/hardware/radio/config/1.3/types.h>

#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>

#include "vts_test_util.h"

using namespace ::android::hardware::radio::config;

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

using ::android::hardware::radio::V1_0::RadioIndicationType;
using ::android::hardware::radio::V1_0::RadioResponseInfo;
using ::android::hardware::radio::V1_0::RadioResponseType;

#define TIMEOUT_PERIOD 75

class RadioConfigHidlTest;

/* Callback class for radio config response */
class RadioConfigResponse : public V1_3::IRadioConfigResponse {
  protected:
    RadioConfigHidlTest& parent;

  public:
    RadioResponseInfo rspInfo;
    V1_1::PhoneCapability phoneCap_1_1;
    V1_3::PhoneCapability phoneCap_1_3;

    RadioConfigResponse(RadioConfigHidlTest& parent);
    virtual ~RadioConfigResponse() = default;

    /* 1.0 Api */
    Return<void> getSimSlotsStatusResponse(const RadioResponseInfo& info,
                                           const hidl_vec<V1_0::SimSlotStatus>& slotStatus);

    Return<void> setSimSlotsMappingResponse(const RadioResponseInfo& info);

    /* 1.1 Api */
    Return<void> getPhoneCapabilityResponse(const RadioResponseInfo& info,
                                            const V1_1::PhoneCapability& phoneCapability);

    Return<void> setPreferredDataModemResponse(const RadioResponseInfo& info);

    Return<void> getModemsConfigResponse(const RadioResponseInfo& info,
                                         const V1_1::ModemsConfig& mConfig);

    Return<void> setModemsConfigResponse(const RadioResponseInfo& info);

    /* 1.2 Api */
    Return<void> getSimSlotsStatusResponse_1_2(const RadioResponseInfo& info,
                                               const hidl_vec<V1_2::SimSlotStatus>& slotStatus);

    /* 1.3 Api */
    Return<void> getPhoneCapabilityResponse_1_3(const RadioResponseInfo& info,
                                                const V1_3::PhoneCapability& phoneCapability);
};

/* Callback class for radio config indication */
class RadioConfigIndication : public V1_3::IRadioConfigIndication {
  protected:
    RadioConfigHidlTest& parent;

  public:
    RadioConfigIndication(RadioConfigHidlTest& parent);
    virtual ~RadioConfigIndication() = default;

    /* 1.2 Api */
    Return<void> simSlotsStatusChanged_1_2(RadioIndicationType type,
                                           const hidl_vec<V1_2::SimSlotStatus>& slotStatus);
};

// The main test class for Radio config HIDL.
class RadioConfigHidlTest : public ::testing::TestWithParam<std::string> {
  protected:
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_;

  public:
    virtual void SetUp() override;

    /* Used as a mechanism to inform the test about data/event callback */
    void notify(int receivedSerial);

    /* Test code calls this function to wait for response */
    std::cv_status wait();

    void updateSimCardStatus();

    /* Serial number for radio request */
    int serial;

    /* radio config service handle */
    sp<V1_3::IRadioConfig> radioConfig;

    /* radio config response handle */
    sp<RadioConfigResponse> radioConfigRsp;
};
