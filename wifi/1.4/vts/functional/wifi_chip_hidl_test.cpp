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

#include <VtsHalHidlTargetCallbackBase.h>
#include <android-base/logging.h>

#undef NAN  // NAN is defined in bionic/libc/include/math.h:38

#include <android/hardware/wifi/1.4/IWifi.h>
#include <android/hardware/wifi/1.4/IWifiChip.h>
#include <android/hardware/wifi/1.4/IWifiChipEventCallback.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::V1_0::IfaceType;
using ::android::hardware::wifi::V1_0::WifiDebugRingBufferStatus;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_4::IWifiChip;
using ::android::hardware::wifi::V1_4::IWifiChipEventCallback;

/**
 * Fixture to use for all Wifi chip HIDL interface tests.
 */
class WifiChipHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        // Make sure to start with a clean state
        stopWifi(GetInstanceName());

        wifi_chip_ = IWifiChip::castFrom(getWifiChip(GetInstanceName()));
        ASSERT_NE(nullptr, wifi_chip_.get());
    }

    virtual void TearDown() override { stopWifi(GetInstanceName()); }

    // A simple test implementation of WifiChipEventCallback.
    class WifiChipEventCallback
        : public ::testing::VtsHalHidlTargetCallbackBase<WifiChipHidlTest>,
          public IWifiChipEventCallback {
       public:
        WifiChipEventCallback(){};

        virtual ~WifiChipEventCallback() = default;

        Return<void> onChipReconfigured(uint32_t modeId __unused) {
            return Void();
        };

        Return<void> onChipReconfigureFailure(
            const WifiStatus& status __unused) {
            return Void();
        };

        Return<void> onIfaceAdded(IfaceType type __unused,
                                  const hidl_string& name __unused) {
            return Void();
        };

        Return<void> onIfaceRemoved(IfaceType type __unused,
                                    const hidl_string& name __unused) {
            return Void();
        };

        Return<void> onDebugRingBufferDataAvailable(
            const WifiDebugRingBufferStatus& status __unused,
            const hidl_vec<uint8_t>& data __unused) {
            return Void();
        };

        Return<void> onDebugErrorAlert(int32_t errorCode __unused,
                                       const hidl_vec<uint8_t>& debugData
                                           __unused) {
            return Void();
        };

        Return<void> onRadioModeChange(
            const hidl_vec<::android::hardware::wifi::V1_2::
                               IWifiChipEventCallback::RadioModeInfo>&
                radioModeInfos __unused) {
            return Void();
        };

        Return<void> onRadioModeChange_1_4(
            const hidl_vec<RadioModeInfo>& radioModeInfos __unused) {
            return Void();
        };
    };

   protected:
    sp<IWifiChip> wifi_chip_;

   private:
    std::string GetInstanceName() { return GetParam(); }
};

/*
 * registerEventCallback_1_4
 * This test case tests the registerEventCallback_1_4() API which registers
 * a call back function with the hal implementation
 *
 * Note: it is not feasible to test the invocation of the call back function
 * since event is triggered internally in the HAL implementation, and can not be
 * triggered from the test case
 */
TEST_P(WifiChipHidlTest, registerEventCallback_1_4) {
    sp<WifiChipEventCallback> wifiChipEventCallback =
        new WifiChipEventCallback();
    const auto& status = HIDL_INVOKE(wifi_chip_, registerEventCallback_1_4,
                                     wifiChipEventCallback);

    if (status.code != WifiStatusCode::SUCCESS) {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
        return;
    }
}
