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

#include "gnss_hal_test.h"
#include <hidl/ServiceManagement.h>

using GnssConstellationTypeAidl = android::hardware::gnss::GnssConstellationType;

void GnssHalTest::SetUp() {
    // Get AIDL handle
    aidl_gnss_hal_ = android::waitForDeclaredService<IGnssAidl>(String16(GetParam().c_str()));
    ASSERT_NE(aidl_gnss_hal_, nullptr);

    const auto& hidlInstanceNames = android::hardware::getAllHalInstanceNames(
            android::hardware::gnss::V2_1::IGnss::descriptor);
    gnss_hal_ = IGnss_V2_1::getService(hidlInstanceNames[0]);
    ASSERT_NE(gnss_hal_, nullptr);

    SetUpGnssCallback();
}

void GnssHalTest::SetUpGnssCallback() {
    aidl_gnss_cb_ = new GnssCallbackAidl();
    ASSERT_NE(aidl_gnss_cb_, nullptr);

    auto status = aidl_gnss_hal_->setCallback(aidl_gnss_cb_);
    if (!status.isOk()) {
        ALOGE("Failed to setCallback");
    }

    ASSERT_TRUE(status.isOk());

    /*
     * Capabilities callback should trigger.
     */
    EXPECT_TRUE(aidl_gnss_cb_->capabilities_cbq_.retrieve(aidl_gnss_cb_->last_capabilities_,
                                                          TIMEOUT_SEC));

    EXPECT_EQ(aidl_gnss_cb_->capabilities_cbq_.calledCount(), 1);

    // Invoke the super method.
    GnssHalTestTemplate<IGnss_V2_1>::SetUpGnssCallback();
}
