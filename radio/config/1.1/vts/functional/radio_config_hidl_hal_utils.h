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

#include <android-base/logging.h>

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <android/hardware/radio/config/1.1/IRadioConfig.h>
#include <android/hardware/radio/config/1.1/IRadioConfigResponse.h>
#include <android/hardware/radio/config/1.1/types.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>

#include "vts_test_util.h"

using namespace ::android::hardware::radio::config::V1_1;

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::radio::config::V1_0::SimSlotStatus;
using ::android::hardware::radio::V1_0::RadioResponseInfo;
using ::android::hardware::radio::V1_0::RadioResponseType;

#define TIMEOUT_PERIOD 75
#define RADIO_SERVICE_NAME "slot1"

class RadioConfigHidlTest;

/* Callback class for radio config response */
class RadioConfigResponse : public IRadioConfigResponse {
   protected:
    RadioConfigHidlTest& parent;

   public:
    RadioResponseInfo rspInfo;
    PhoneCapability phoneCap;

    RadioConfigResponse(RadioConfigHidlTest& parent);
    virtual ~RadioConfigResponse() = default;

    Return<void> getSimSlotsStatusResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<SimSlotStatus>& slotStatus);

    Return<void> setSimSlotsMappingResponse(const RadioResponseInfo& info);

    Return<void> getPhoneCapabilityResponse(const RadioResponseInfo& info,
                                            const PhoneCapability& phoneCapability);

    Return<void> setPreferredDataModemResponse(const RadioResponseInfo& info);

    Return<void> getModemsConfigResponse(const RadioResponseInfo& info,
                                         const ModemsConfig& mConfig);

    Return<void> setModemsConfigResponse(const RadioResponseInfo& info);
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
    sp<IRadioConfig> radioConfig;

    /* radio config response handle */
    sp<RadioConfigResponse> radioConfigRsp;
};
