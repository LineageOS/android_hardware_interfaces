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

#define LOG_TAG "SoundTriggerHidlHalTest"

#include <android-base/logging.h>
#include <android/hardware/audio/common/2.0/types.h>
#include <android/hardware/soundtrigger/2.4/ISoundTriggerHwGlobalCallback.h>
#include <android/hardware/soundtrigger/2.4/ISoundTriggerHw.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Status;
using ::android::hardware::soundtrigger::V2_4::ISoundTriggerHw;
using ::android::hardware::soundtrigger::V2_4::ISoundTriggerHwGlobalCallback;

/**
 * Test class holding the instance of the SoundTriggerHW service to test.
 * The passed parameter is the registered name of the implementing service
 * supplied by INSTANTIATE_TEST_SUITE_P() call.
 */
class SoundTriggerHidlTest : public testing::TestWithParam<std::string> {
public:
    void SetUp() override {
        mSoundtrigger = ISoundTriggerHw::getService(GetParam());

        ASSERT_NE(mSoundtrigger, nullptr);
        LOG(INFO) << "Test is remote " << mSoundtrigger->isRemote();
    }

protected:
    sp<ISoundTriggerHw> mSoundtrigger;
};

/**
 * Empty test is in place to ensure service is initialized.
 * Due to the nature of SoundTrigger HAL providing an interface for
 * proprietary or vendor specific implementations, limited testing on
 * individual APIs is possible.
 */
TEST_P(SoundTriggerHidlTest, ServiceIsInstantiated) {}

class GlobalCallback : public ISoundTriggerHwGlobalCallback {
    Return<void> onResourcesAvailable() override {
        return Status::ok();
    }
};

/**
 * Test ISoundTriggerHw::registerGlobalCallback method
 *
 * Verifies that:
 * - the implementation implements the method
 * - the method returns no error
 */
TEST_P(SoundTriggerHidlTest, RegisterGlobalCallback) {
    Return<void> hidlReturn;
    sp<ISoundTriggerHwGlobalCallback> callback = new GlobalCallback();
    hidlReturn = mSoundtrigger->registerGlobalCallback(callback);
    EXPECT_TRUE(hidlReturn.isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SoundTriggerHidlTest);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, SoundTriggerHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(ISoundTriggerHw::descriptor)),
        android::hardware::PrintInstanceNameToString);
