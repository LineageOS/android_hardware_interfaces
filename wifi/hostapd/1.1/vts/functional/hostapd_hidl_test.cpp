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
#include <cutils/properties.h>

#include <VtsHalHidlTargetTestBase.h>

#include <android/hardware/wifi/hostapd/1.1/IHostapd.h>

#include "hostapd_hidl_call_util.h"
#include "hostapd_hidl_test_utils.h"
#include "hostapd_hidl_test_utils_1_1.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::hostapd::V1_0::HostapdStatus;
using ::android::hardware::wifi::hostapd::V1_0::HostapdStatusCode;
using ::android::hardware::wifi::hostapd::V1_1::IHostapd;
using ::android::hardware::wifi::hostapd::V1_1::IHostapdCallback;

class HostapdHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        startHostapdAndWaitForHidlService();
        hostapd_ = getHostapd_1_1();
        ASSERT_NE(hostapd_.get(), nullptr);
    }

    virtual void TearDown() override { stopHostapd(); }

   protected:
    // IHostapd object used for all tests in this fixture.
    sp<IHostapd> hostapd_;
};

class IfaceCallback : public IHostapdCallback {
    Return<void> onFailure(
        const hidl_string& /* Name of the interface */) override {
        return Void();
    }
};

/*
 * RegisterCallback
 */
TEST_F(HostapdHidlTest, registerCallback) {
    hostapd_->registerCallback(
        new IfaceCallback(), [](const HostapdStatus& status) {
            EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
        });
}
