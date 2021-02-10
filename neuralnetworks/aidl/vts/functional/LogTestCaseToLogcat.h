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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_AIDL_LOG_TEST_CASE_TO_LOGCAT_H
#define ANDROID_HARDWARE_NEURALNETWORKS_AIDL_LOG_TEST_CASE_TO_LOGCAT_H

#include <android-base/logging.h>
#include <gtest/gtest.h>

namespace aidl::android::hardware::neuralnetworks {

class LogTestCaseToLogcat : public ::testing::EmptyTestEventListener {
  public:
    void OnTestStart(const ::testing::TestInfo& test_info) override {
        LOG(INFO) << "[Test Case] " << test_info.test_suite_name() << "." << test_info.name()
                  << " BEGIN";
    }

    void OnTestEnd(const ::testing::TestInfo& test_info) override {
        LOG(INFO) << "[Test Case] " << test_info.test_suite_name() << "." << test_info.name()
                  << " END";
    }
};

}  // namespace aidl::android::hardware::neuralnetworks

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_AIDL_LOG_TEST_CASE_TO_LOGCAT_H
