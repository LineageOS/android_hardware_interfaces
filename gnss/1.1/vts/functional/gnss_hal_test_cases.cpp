/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <gnss_hal_test.h>

#include <VtsHalHidlTargetTestBase.h>

#define TIMEOUT_SEC 2  // for basic commands/responses

/*
 * SetupTeardownCreateCleanup:
 * Requests the gnss HAL then calls cleanup
 *
 * Empty test fixture to verify basic Setup & Teardown
 */
TEST_F(GnssHalTest, SetupTeardownCreateCleanup) {}

/*
 * SetCallbackResponses:
 * Sets up the callback, awaits the info & name
 */
TEST_F(GnssHalTest, SetCallbackResponses) {
    gnss_cb_ = new GnssCallback(*this);
    ASSERT_NE(gnss_cb_, nullptr);

    auto result = gnss_hal_->setCallback_1_1(gnss_cb_);
    if (!result.isOk()) {
        ALOGE("result of failed setCallback %s", result.description().c_str());
    }

    ASSERT_TRUE(result.isOk());
    ASSERT_TRUE(result);

    /*
     * Both name and systemInfo callbacks should trigger
     */
    EXPECT_EQ(std::cv_status::no_timeout, wait(TIMEOUT_SEC));
    EXPECT_EQ(std::cv_status::no_timeout, wait(TIMEOUT_SEC));

    EXPECT_EQ(info_called_count_, 1);
    EXPECT_EQ(name_called_count_, 1);
}