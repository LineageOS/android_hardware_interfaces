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

#ifndef DRM_HAL_TEST_V1_3_H
#define DRM_HAL_TEST_V1_3_H

#include <android/hardware/drm/1.3/ICryptoFactory.h>
#include <android/hardware/drm/1.3/IDrmFactory.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "drm_hal_vendor_module_api.h"
#include "drm_vts_helper.h"
#include "vendor_modules.h"
#include "VtsHalHidlTargetCallbackBase.h"

namespace android {
namespace hardware {
namespace drm {
namespace V1_3 {
namespace vts {

using android::hardware::hidl_array;
using android::hardware::hidl_string;

using drm_vts::DrmHalTestParam;

using IDrmFactoryV1_3 = android::hardware::drm::V1_3::IDrmFactory;
using IDrmPluginV1_0 = android::hardware::drm::V1_0::IDrmPlugin;
using StatusV1_0 = android::hardware::drm::V1_0::Status;

class DrmHalTestV1_3 : public ::testing::TestWithParam<DrmHalTestParam> {
public:
    DrmHalTestV1_3()
        : drmFactory_(IDrmFactoryV1_3::getService(GetParam().instance_)) {}

    virtual void SetUp() override {
        ASSERT_NE(drmFactory_, nullptr);

        // create plugin
        hidl_string packageName("android.hardware.drm.V1_3.vts");
        auto res = drmFactory_->createPlugin(
            GetParam().scheme_, packageName,
            [&](StatusV1_0 status, const sp<IDrmPluginV1_0>& pluginV1_0) {
                EXPECT_EQ(StatusV1_0::OK, status);
                drmPlugin_ = pluginV1_0;
            });
        EXPECT_TRUE(res.isOk());
        ASSERT_NE(drmPlugin_, nullptr);
    }

    virtual void TearDown() override {}

protected:
    sp<IDrmFactoryV1_3> drmFactory_;
    sp<IDrmPluginV1_0> drmPlugin_;
};

}  // namespace vts
}  // namespace V1_3
}  // namespace drm
}  // namespace hardware
}  // namespace android

#endif  // DRM_HAL_TEST_V1_3_H
