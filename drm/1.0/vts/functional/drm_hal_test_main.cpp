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

#define LOG_TAG "drm_hal_vendor_test@1.0"

#include "vendor_modules.h"
#include "android/hardware/drm/1.0/vts/drm_hal_vendor_test.h"
#include "android/hardware/drm/1.0/vts/drm_hal_clearkey_test.h"

using ::android::hardware::drm::V1_0::ICryptoFactory;
using ::android::hardware::drm::V1_0::IDrmFactory;

using ::android::hardware::drm::V1_0::vts::DrmHalClearkeyFactoryTest;
using ::android::hardware::drm::V1_0::vts::DrmHalClearkeyPluginTest;
using ::android::hardware::drm::V1_0::vts::DrmHalClearkeyDecryptTest;

using ::android::hardware::drm::V1_0::vts::DrmHalVendorFactoryTest;
using ::android::hardware::drm::V1_0::vts::DrmHalVendorPluginTest;
using ::android::hardware::drm::V1_0::vts::DrmHalVendorDecryptTest;

/**
 * Instantiate the set of test cases for each vendor module
 */

static const std::vector<DrmHalTestParam> kAllInstances = [] {
    std::vector<std::string> drmInstances =
            android::hardware::getAllHalInstanceNames(IDrmFactory::descriptor);
    std::vector<std::string> cryptoInstances =
            android::hardware::getAllHalInstanceNames(ICryptoFactory::descriptor);
    std::set<std::string> allInstances;
    allInstances.insert(drmInstances.begin(), drmInstances.end());
    allInstances.insert(cryptoInstances.begin(), cryptoInstances.end());

    std::vector<DrmHalTestParam> allInstanceUuidCombos;
    auto noUUID = [](std::string s) { return DrmHalTestParam(s); };
    std::transform(allInstances.begin(), allInstances.end(),
            std::back_inserter(allInstanceUuidCombos), noUUID);
    return allInstanceUuidCombos;
}();

INSTANTIATE_TEST_CASE_P(DrmHalVendorFactoryTestCases, DrmHalVendorFactoryTest,
                        testing::ValuesIn(kAllInstances),
                        drm_vts::PrintParamInstanceToString);

INSTANTIATE_TEST_CASE_P(DrmHalVendorPluginTestCases, DrmHalVendorPluginTest,
                        testing::ValuesIn(kAllInstances),
                        drm_vts::PrintParamInstanceToString);

INSTANTIATE_TEST_CASE_P(DrmHalVendorDecryptTestCases, DrmHalVendorDecryptTest,
                        testing::ValuesIn(kAllInstances),
                        drm_vts::PrintParamInstanceToString);

INSTANTIATE_TEST_SUITE_P(PerInstance, DrmHalClearkeyFactoryTest, testing::ValuesIn(kAllInstances),
                         drm_vts::PrintParamInstanceToString);
INSTANTIATE_TEST_SUITE_P(PerInstance, DrmHalClearkeyPluginTest, testing::ValuesIn(kAllInstances),
                         drm_vts::PrintParamInstanceToString);
INSTANTIATE_TEST_SUITE_P(PerInstance, DrmHalClearkeyDecryptTest, testing::ValuesIn(kAllInstances),
                         drm_vts::PrintParamInstanceToString);
