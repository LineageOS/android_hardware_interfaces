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

#include "gnss_hal_test.h"

#include <android/hardware/gnss/IGnss.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

using android::ProcessState;

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GnssHalTest);
INSTANTIATE_TEST_SUITE_P(, GnssHalTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IGnssAidl::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}