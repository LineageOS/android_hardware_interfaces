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

#define LOG_TAG "VtsHalUsbV1_3TargetTest"
#include <android-base/logging.h>

#include <android/hardware/usb/1.3/IUsb.h>

#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <log/log.h>
#include <stdlib.h>
#include <condition_variable>

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::usb::V1_0::Status;
using ::android::hardware::usb::V1_3::IUsb;
using ::android::hidl::base::V1_0::IBase;

// The main test class for the USB hidl HAL
class UsbHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        ALOGI(__FUNCTION__);
        usb = IUsb::getService(GetParam());
        ASSERT_NE(usb, nullptr);
    }

    virtual void TearDown() override { ALOGI("Teardown"); }

    // USB hidl hal Proxy
    sp<IUsb> usb;
};

/*
 * Check to see if enable usb data signal succeeds.
 * HAL service should call enableUsbDataSignal.
 */
TEST_P(UsbHidlTest, enableUsbDataSignal) {
    Return<bool> ret = usb->enableUsbDataSignal(true);
    ASSERT_TRUE(ret.isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(UsbHidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, UsbHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IUsb::descriptor)),
        android::hardware::PrintInstanceNameToString);
