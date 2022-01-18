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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_TEST_UTILS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_TEST_UTILS_H

#include <gtest/gtest.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <string>

namespace aidl::android::hardware::neuralnetworks::utils {

class VersionedAidlUtilsTestBase : public ::testing::TestWithParam<nn::Version> {
  protected:
    const nn::Version kVersion = GetParam();
};

std::string printTestVersion(const testing::TestParamInfo<nn::Version>& info);

inline const auto kAllAidlVersions =
        ::testing::Values(nn::kVersionFeatureLevel5, nn::kVersionFeatureLevel6,
                          nn::kVersionFeatureLevel7, nn::kVersionFeatureLevel8);

#define INSTANTIATE_VERSIONED_AIDL_UTILS_TEST(TestSuite, versions) \
    INSTANTIATE_TEST_SUITE_P(Versioned, TestSuite, versions, printTestVersion)

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_TEST_TEST_UTILS_H
