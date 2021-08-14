/*
 * Copyright (C) 2016 The Android Open Source Project
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
#pragma clang diagnostic ignored "-Wweak-vtables"

#include <VtsCoreUtil.h>
#include <android/hardware/wifi/supplicant/1.1/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.1/ISupplicantStaIface.h>
#include <android/hardware/wifi/supplicant/1.1/ISupplicantStaNetwork.h>
#include <gtest/gtest.h>

android::sp<android::hardware::wifi::supplicant::V1_1::ISupplicant>
getSupplicant_1_1(const std::string& supplicant_instance_name, bool isP2pOn);

android::sp<android::hardware::wifi::supplicant::V1_1::ISupplicantStaIface>
getSupplicantStaIface_1_1(
    const android::sp<android::hardware::wifi::supplicant::V1_1::ISupplicant>&
        supplicant);

android::sp<android::hardware::wifi::supplicant::V1_1::ISupplicantStaNetwork>
createSupplicantStaNetwork_1_1(
    const android::sp<android::hardware::wifi::supplicant::V1_1::ISupplicant>&
        supplicant);

class SupplicantHidlTestBaseV1_1 : public SupplicantHidlTestBase {
   public:
    virtual void SetUp() override {
        SupplicantHidlTestBase::SetUp();
        supplicant_ = getSupplicant_1_1(supplicant_instance_name_, isP2pOn_);
        ASSERT_NE(supplicant_.get(), nullptr);
    }

   protected:
    android::sp<android::hardware::wifi::supplicant::V1_1::ISupplicant>
        supplicant_;
};
