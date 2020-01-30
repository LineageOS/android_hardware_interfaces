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

#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>

#include <algorithm>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "android/hardware/drm/1.1/vts/drm_hal_clearkey_test.h"

using android::hardware::drm::V1_1::ICryptoFactory;
using android::hardware::drm::V1_1::IDrmFactory;
using android::hardware::drm::V1_1::vts::DrmHalClearkeyTest;
using android::hardware::drm::V1_1::vts::kClearKeyUUID;

static const std::vector<DrmHalTestParam> kAllInstances = [] {
    std::vector<std::string> drmInstances =
            android::hardware::getAllHalInstanceNames(IDrmFactory::descriptor);
    std::vector<std::string> cryptoInstances =
            android::hardware::getAllHalInstanceNames(ICryptoFactory::descriptor);
    std::set<std::string> allInstances;
    allInstances.insert(drmInstances.begin(), drmInstances.end());
    allInstances.insert(cryptoInstances.begin(), cryptoInstances.end());

    std::vector<DrmHalTestParam> allInstancesWithClearKeyUuid;
    std::transform(allInstances.begin(), allInstances.end(),
            std::back_inserter(allInstancesWithClearKeyUuid),
            [](std::string s) { return DrmHalTestParam(s, kClearKeyUUID); });
    return allInstancesWithClearKeyUuid;
}();

INSTANTIATE_TEST_SUITE_P(PerInstance, DrmHalClearkeyTest, testing::ValuesIn(kAllInstances),
                         PrintParamInstanceToString);
