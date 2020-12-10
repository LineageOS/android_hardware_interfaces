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

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <android/hardware/gnss/IGnss.h>
#include <binder/IServiceManager.h>

#include <android/hardware/gnss/2.1/IGnss.h>
#include "GnssCallbackAidl.h"
#include "v2_1/gnss_hal_test_template.h"

using IGnss_V2_1 = android::hardware::gnss::V2_1::IGnss;

using android::ProcessState;
using android::sp;
using android::String16;
using IGnssAidl = android::hardware::gnss::IGnss;
using android::hardware::gnss::BlocklistedSource;
using android::hardware::gnss::IGnssConfiguration;

// The main test class for GNSS HAL.
class GnssHalTest : public android::hardware::gnss::common::GnssHalTestTemplate<IGnss_V2_1> {
  public:
    GnssHalTest(){};
    ~GnssHalTest(){};
    virtual void SetUp() override;
    virtual void SetUpGnssCallback() override;

    sp<IGnssAidl> aidl_gnss_hal_;
    sp<GnssCallbackAidl> aidl_gnss_cb_;  // Primary callback interface
};
