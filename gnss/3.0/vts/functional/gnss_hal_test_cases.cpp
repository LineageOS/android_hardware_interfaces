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

#define LOG_TAG "GnssHalTestCases"

#include <gnss_hal_test.h>
#include <cmath>
#include "Utils.h"

#include <gtest/gtest.h>

using android::hardware::hidl_string;
using android::hardware::hidl_vec;

using android::hardware::gnss::common::Utils;

using android::hardware::gnss::V3_0::IGnssPsds;

/*
 * SetupTeardownCreateCleanup:
 * Requests the gnss HAL then calls cleanup
 *
 * Empty test fixture to verify basic Setup & Teardown
 */
TEST_P(GnssHalTest, SetupTeardownCreateCleanup) {}

/*
 * TestPsdsExtension:
 * Gets the PsdsExtension and verifies that it returns a non-null extension.
 */
TEST_P(GnssHalTest, TestPsdsExtension) {
    auto psds = gnss_hal_->getExtensionPsds();
    ASSERT_TRUE(psds.isOk());
    sp<IGnssPsds> iPsds = psds;
    ASSERT_TRUE(iPsds != nullptr);
}
