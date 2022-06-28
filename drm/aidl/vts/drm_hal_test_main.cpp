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

/**
 * Instantiate the set of test cases for each vendor module
 */

#define LOG_TAG "drm_hal_test_main"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android/binder_process.h>
#include <log/log.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "drm_hal_common.h"

using ::aidl::android::hardware::drm::vts::DrmHalClearkeyTest;
using ::aidl::android::hardware::drm::vts::DrmHalTest;
using ::aidl::android::hardware::drm::vts::HalBaseName;
using drm_vts::DrmHalTestParam;
using drm_vts::PrintParamInstanceToString;

static const std::vector<DrmHalTestParam> getAllInstances() {
    using ::aidl::android::hardware::drm::IDrmFactory;

    std::vector<std::string> drmInstances =
            android::getAidlHalInstanceNames(IDrmFactory::descriptor);

    std::set<std::string> allInstances;
    for (auto svc : drmInstances) {
        allInstances.insert(HalBaseName(svc));
    }

    std::vector<DrmHalTestParam> allInstanceUuidCombos;
    auto noUUID = [](std::string s) { return DrmHalTestParam(s); };
    std::transform(allInstances.begin(), allInstances.end(),
                   std::back_inserter(allInstanceUuidCombos), noUUID);
    return allInstanceUuidCombos;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DrmHalTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, DrmHalTest, testing::ValuesIn(getAllInstances()),
                         PrintParamInstanceToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DrmHalClearkeyTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, DrmHalClearkeyTest, testing::ValuesIn(getAllInstances()),
                         PrintParamInstanceToString);

int main(int argc, char** argv) {
#if defined(__LP64__)
    const char* kModulePath = "/data/local/tmp/64/lib";
#else
    const char* kModulePath = "/data/local/tmp/32/lib";
#endif
    DrmHalTest::gVendorModules = new drm_vts::VendorModules(kModulePath);
    if (DrmHalTest::gVendorModules->getPathList().size() == 0) {
        std::cerr << "WARNING: No vendor modules found in " << kModulePath
                  << ", all vendor tests will be skipped" << std::endl;
    }
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
