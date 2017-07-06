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

#define LOG_TAG "ConfigstoreHidlHalTest"

#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>
#include <android/hardware/configstore/1.0/types.h>
#include <android/hardware/configstore/1.1/ISurfaceFlingerConfigs.h>
#include <unistd.h>

using ::android::hardware::configstore::V1_1::ISurfaceFlingerConfigs;
using ::android::sp;

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())
#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk())

class ConfigstoreHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    sp<ISurfaceFlingerConfigs> sfConfigs;

    virtual void SetUp() override {
        sfConfigs = ::testing::VtsHalHidlTargetTestBase::getService<ISurfaceFlingerConfigs>();
        ASSERT_NE(sfConfigs, nullptr);
    }

    virtual void TearDown() override {}
};

/**
 * Placeholder testcase.
 */
TEST_F(ConfigstoreHidlTest, Test) {
    ASSERT_TRUE(true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
