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
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/configstore/1.0/types.h>
#include <android/hardware/configstore/1.2/ISurfaceFlingerConfigs.h>
#include <unistd.h>

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::configstore::V1_0::OptionalBool;
using ::android::hardware::configstore::V1_0::OptionalInt64;
using ::android::hardware::configstore::V1_0::OptionalUInt64;
using ::android::hardware::configstore::V1_2::ISurfaceFlingerConfigs;
using ::android::hardware::graphics::common::V1_1::PixelFormat;
using ::android::hardware::graphics::common::V1_2::Dataspace;

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())
#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk())

// Test environment for Configstore HIDL HAL.
class ConfigstoreHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static ConfigstoreHidlEnvironment* Instance() {
        static ConfigstoreHidlEnvironment* instance = new ConfigstoreHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<ISurfaceFlingerConfigs>(); }
};

class ConfigstoreHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    sp<ISurfaceFlingerConfigs> sfConfigs;

    virtual void SetUp() override {
        sfConfigs = ::testing::VtsHalHidlTargetTestBase::getService<ISurfaceFlingerConfigs>(
            ConfigstoreHidlEnvironment::Instance()->getServiceName<ISurfaceFlingerConfigs>());
        ASSERT_NE(sfConfigs, nullptr);
    }

    virtual void TearDown() override {}

    bool isSupportedWideColorGamut(Dataspace dataspace) {
        Dataspace standard = static_cast<Dataspace>(dataspace & Dataspace::STANDARD_MASK);
        return standard == Dataspace::STANDARD_DCI_P3 || standard == Dataspace::STANDARD_BT2020;
    }
};

/**
 * Make sure the constrains of hasWideColorDisplay, hasHDRDisplay
 * and useColorManagement are enforced.
 */
TEST_F(ConfigstoreHidlTest, TestColorConstrainsWithColorManagement) {
    bool hasWideColorDisplay;
    bool hasHDRDisplay;
    bool useColorManagement;

    Return<void> status = sfConfigs->hasWideColorDisplay(
        [&](OptionalBool arg) { hasWideColorDisplay = arg.specified; });
    EXPECT_OK(status);

    status = sfConfigs->hasHDRDisplay([&](OptionalBool arg) { hasHDRDisplay = arg.specified; });
    EXPECT_OK(status);

    status = sfConfigs->useColorManagement(
        [&](OptionalBool arg) { useColorManagement = arg.specified; });
    EXPECT_OK(status);

    // When hasHDRDisplay returns true, hasWideColorDisplay must also return true.
    if (hasHDRDisplay) {
        ASSERT_TRUE(hasWideColorDisplay);
    }

    // When hasWideColorDisplay returns true, useColorManagement
    // must also return true.
    if (hasWideColorDisplay) {
        ASSERT_TRUE(useColorManagement);
    }
}

TEST_F(ConfigstoreHidlTest, TestGetCompositionPreference) {
    bool hasWideColorDisplay;

    Return<void> status = sfConfigs->hasWideColorDisplay(
        [&](OptionalBool arg) { hasWideColorDisplay = arg.specified; });
    EXPECT_OK(status);

    Dataspace defaultDataspace, wcgDataspace;

    status = sfConfigs->getCompositionPreference(
        [&](auto tmpDefaultDataspace, PixelFormat, auto tmpWcgDataspace, PixelFormat) {
            defaultDataspace = tmpDefaultDataspace;
            wcgDataspace = tmpWcgDataspace;
        });
    EXPECT_OK(status);

    // Default data space and wide color gamut data space must not be UNKNOWN.
    ASSERT_TRUE(defaultDataspace != Dataspace::UNKNOWN && wcgDataspace != Dataspace::UNKNOWN);

    // If hasWideColorDisplay returns true, the wide color gamut data space must be a valid wide
    // color gamut.
    if (hasWideColorDisplay) {
        ASSERT_TRUE(isSupportedWideColorGamut(wcgDataspace));
    }
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(ConfigstoreHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    ConfigstoreHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
