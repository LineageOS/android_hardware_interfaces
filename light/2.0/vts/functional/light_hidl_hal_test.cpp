/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "light_hidl_hal_test"

#include <android-base/logging.h>
#include <android/hardware/light/2.0/ILight.h>
#include <android/hardware/light/2.0/types.h>
#include <gtest/gtest.h>
#include <unistd.h>

using ::android::hardware::light::V2_0::Brightness;
using ::android::hardware::light::V2_0::Flash;
using ::android::hardware::light::V2_0::ILight;
using ::android::hardware::light::V2_0::LightState;
using ::android::hardware::light::V2_0::Status;
using ::android::hardware::light::V2_0::Type;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

#define LIGHT_SERVICE_NAME "light"

#define EXPECT_OK(ret) EXPECT_TRUE(ret.getStatus().isOk())

// The main test class for VIBRATOR HIDL HAL.
class LightHidlTest : public ::testing::Test {
public:
    virtual void SetUp() override {
        light = ILight::getService(LIGHT_SERVICE_NAME);

        ASSERT_NE(light, nullptr);
        ALOGI("Test is remote: %d", light->isRemote());
    }

    virtual void TearDown() override {}

    sp<ILight> light;
};

// A class for test environment setup (kept since this file is a template).
class LightHidlEnvironment : public ::testing::Environment {
public:
    virtual void SetUp() {}
    virtual void TearDown() {}

private:
};

const static LightState kWhite = {
    .color = 0xFFFFFFFF,
    .flashMode = Flash::TIMED,
    .flashOnMs = 100,
    .flashOffMs = 50,
    .brightnessMode = Brightness::USER,
};

const static LightState kOff = {
    .color = 0x00000000,
    .flashMode = Flash::NONE,
    .flashOnMs = 0,
    .flashOffMs = 0,
    .brightnessMode = Brightness::USER,
};

/**
 * Ensure all lights which are reported as supported work.
 */
TEST_F(LightHidlTest, TestSupported) {
    EXPECT_OK(light->getSupportedTypes([this](const hidl_vec<Type> &supportedTypes) {
        for (size_t i = 0; i < supportedTypes.size(); i++) {
            EXPECT_OK(light->setLight(supportedTypes[i], kWhite));
        }

        usleep(500000);

        for (size_t i = 0; i < supportedTypes.size(); i++) {
            EXPECT_OK(light->setLight(supportedTypes[i], kOff));
        }
    }));
}

int main(int argc, char **argv) {
    ::testing::AddGlobalTestEnvironment(new LightHidlEnvironment);
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
