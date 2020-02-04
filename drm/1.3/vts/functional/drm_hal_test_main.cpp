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

/**
 * Instantiate the set of test cases for each vendor module
 */

#define LOG_TAG "drm_hal_test@1.3"

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

#include "android/hardware/drm/1.0/vts/drm_hal_clearkey_test.h" // V1_0 tests
#include "android/hardware/drm/1.0/vts/drm_hal_vendor_test.h"   // V1_0 tests
#include "android/hardware/drm/1.1/vts/drm_hal_clearkey_test.h" // V1_1 tests
#include "android/hardware/drm/1.2/vts/drm_hal_common.h"        // V1_2 tests
#include "android/hardware/drm/1.3/vts/drm_hal_test.h"          // V1_3 tests

using drm_vts::DrmHalTestParam;
using drm_vts::PrintParamInstanceToString;

using android::hardware::drm::V1_0::vts::DrmHalVendorFactoryTest;
using android::hardware::drm::V1_0::vts::DrmHalVendorPluginTest;
using android::hardware::drm::V1_0::vts::DrmHalVendorDecryptTest;
using android::hardware::drm::V1_0::vts::DrmHalClearkeyFactoryTest;
using android::hardware::drm::V1_0::vts::DrmHalClearkeyPluginTest;
using android::hardware::drm::V1_0::vts::DrmHalClearkeyDecryptTest;
using android::hardware::drm::V1_1::vts::DrmHalClearkeyTest;
using android::hardware::drm::V1_2::vts::DrmHalTest;
using android::hardware::drm::V1_2::vts::DrmHalClearkeyTestV1_2;
using android::hardware::drm::V1_3::vts::DrmHalTestV1_3;

static const std::vector<DrmHalTestParam> kAllInstances = [] {
    using ::android::hardware::drm::V1_3::ICryptoFactory;
    using ::android::hardware::drm::V1_3::IDrmFactory;

    std::vector<std::string> drmInstances =
            android::hardware::getAllHalInstanceNames(IDrmFactory::descriptor);
    std::vector<std::string> cryptoInstances =
            android::hardware::getAllHalInstanceNames(ICryptoFactory::descriptor);
    std::set<std::string> allInstances;
    allInstances.insert(drmInstances.begin(), drmInstances.end());
    allInstances.insert(cryptoInstances.begin(), cryptoInstances.end());

    std::vector<DrmHalTestParam> allInstanceUuidCombos;
    for (const auto &instance : allInstances) {
        auto drmFactory = IDrmFactory::getService(instance);
        if (drmFactory == nullptr) {
            continue;
        }
        drmFactory->getSupportedCryptoSchemes(
            [&](const hidl_vec<hidl_array<uint8_t, 16>>& schemes) {
                for (const auto &scheme : schemes) {
                    allInstanceUuidCombos.push_back(DrmHalTestParam(instance, scheme));
                }
            });
    }
    return allInstanceUuidCombos;
}();

INSTANTIATE_TEST_CASE_P(PerInstanceUuidV1_0, DrmHalVendorFactoryTest,
                        testing::ValuesIn(kAllInstances),
                        drm_vts::PrintParamInstanceToString);
INSTANTIATE_TEST_CASE_P(PerInstanceUuidV1_0, DrmHalVendorPluginTest,
                        testing::ValuesIn(kAllInstances),
                        drm_vts::PrintParamInstanceToString);
INSTANTIATE_TEST_CASE_P(PerInstanceUuidV1_0, DrmHalVendorDecryptTest,
                        testing::ValuesIn(kAllInstances),
                        drm_vts::PrintParamInstanceToString);

INSTANTIATE_TEST_SUITE_P(PerInstanceUuidV1_0, DrmHalClearkeyFactoryTest,
                         testing::ValuesIn(kAllInstances),
                         drm_vts::PrintParamInstanceToString);
INSTANTIATE_TEST_SUITE_P(PerInstanceUuidV1_0, DrmHalClearkeyPluginTest,
                         testing::ValuesIn(kAllInstances),
                         drm_vts::PrintParamInstanceToString);
INSTANTIATE_TEST_SUITE_P(PerInstanceUuidV1_0, DrmHalClearkeyDecryptTest,
                         testing::ValuesIn(kAllInstances),
                         drm_vts::PrintParamInstanceToString);

INSTANTIATE_TEST_SUITE_P(PerInstanceUuidV1_1, DrmHalClearkeyTest,
                         testing::ValuesIn(kAllInstances),
                         PrintParamInstanceToString);

INSTANTIATE_TEST_SUITE_P(PerInstanceUuidV1_2, DrmHalTest,
                         testing::ValuesIn(kAllInstances),
                         PrintParamInstanceToString);
INSTANTIATE_TEST_SUITE_P(PerInstanceUuidV1_2, DrmHalClearkeyTestV1_2,
                         testing::ValuesIn(kAllInstances),
                         PrintParamInstanceToString);

INSTANTIATE_TEST_SUITE_P(PerInstanceUuidV1_3, DrmHalTestV1_3,
                         testing::ValuesIn(kAllInstances),
                         PrintParamInstanceToString);

int main(int argc, char** argv) {
#if defined(__LP64__)
    const char* kModulePath = "/data/local/tmp/64/lib";
#else
    const char* kModulePath = "/data/local/tmp/32/lib";
#endif
    DrmHalTest::gVendorModules = new drm_vts::VendorModules(kModulePath);
    if (DrmHalTest::gVendorModules->getPathList().size() == 0) {
        std::cerr << "WARNING: No vendor modules found in " << kModulePath <<
                ", all vendor tests will be skipped" << std::endl;
    }
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
