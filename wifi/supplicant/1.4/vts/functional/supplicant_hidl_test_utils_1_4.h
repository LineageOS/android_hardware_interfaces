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

#ifndef SUPPLICANT_HIDL_TEST_UTILS_1_4_H
#define SUPPLICANT_HIDL_TEST_UTILS_1_4_H

#include <android/hardware/wifi/supplicant/1.4/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.4/ISupplicantP2pIface.h>
#include <android/hardware/wifi/supplicant/1.4/ISupplicantStaIface.h>

android::sp<android::hardware::wifi::supplicant::V1_4::ISupplicantStaIface>
getSupplicantStaIface_1_4(
    const android::sp<android::hardware::wifi::supplicant::V1_4::ISupplicant>&
        supplicant);
android::sp<android::hardware::wifi::supplicant::V1_4::ISupplicantP2pIface>
getSupplicantP2pIface_1_4(
    const android::sp<android::hardware::wifi::supplicant::V1_4::ISupplicant>&
        supplicant);
android::sp<android::hardware::wifi::supplicant::V1_4::ISupplicant>
getSupplicant_1_4(const std::string& supplicant_instance_name, bool isP2pOn);

class SupplicantHidlTestBaseV1_4 : public SupplicantHidlTestBase {
   public:
    virtual void SetUp() override {
        SupplicantHidlTestBase::SetUp();
        supplicant_ = getSupplicant_1_4(supplicant_instance_name_, isP2pOn_);
        ASSERT_NE(supplicant_.get(), nullptr);
        EXPECT_TRUE(turnOnExcessiveLogging(supplicant_));
    }

   protected:
    android::sp<android::hardware::wifi::supplicant::V1_4::ISupplicant>
        supplicant_;
};
#endif /* SUPPLICANT_HIDL_TEST_UTILS_1_4_H */
