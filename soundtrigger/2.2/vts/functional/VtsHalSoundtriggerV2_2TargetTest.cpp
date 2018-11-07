/*
 * Copyright (C) 2018 The Android Open Source Project
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
#include <stdlib.h>
#include <time.h>

#include <condition_variable>
#include <mutex>

#include <android/log.h>
#include <cutils/native_handle.h>
#include <log/log.h>

#include <android/hardware/audio/common/2.0/types.h>
#include <android/hardware/soundtrigger/2.0/ISoundTriggerHw.h>
#include <android/hardware/soundtrigger/2.2/ISoundTriggerHw.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::soundtrigger::V2_0::ISoundTriggerHwCallback;
using ::android::hardware::soundtrigger::V2_0::SoundModelHandle;
using ::android::hardware::soundtrigger::V2_2::ISoundTriggerHw;

// Test environment for SoundTrigger HIDL HAL.
class SoundTriggerHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static SoundTriggerHidlEnvironment* Instance() {
        static SoundTriggerHidlEnvironment* instance = new SoundTriggerHidlEnvironment;
        return instance;
    }

    void registerTestServices() override { registerTestService<ISoundTriggerHw>(); }

   private:
    SoundTriggerHidlEnvironment() {}
};

// The main test class for Sound Trigger HIDL HAL.
class SoundTriggerHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    void SetUp() override {
        mSoundTriggerHal = ::testing::VtsHalHidlTargetTestBase::getService<ISoundTriggerHw>(
            SoundTriggerHidlEnvironment::Instance()->getServiceName<ISoundTriggerHw>());
        ASSERT_NE(nullptr, mSoundTriggerHal.get());
    }

    static void SetUpTestCase() { srand(1234); }

    void TearDown() override {}

   protected:
    sp<ISoundTriggerHw> mSoundTriggerHal;
};

/**
 * Test ISoundTriggerHw::getModelState() method
 *
 * Verifies that:
 *  - the implementation returns -ENOSYS with invalid model handle
 *
 */
TEST_F(SoundTriggerHidlTest, GetModelStateInvalidModel) {
    SoundModelHandle handle = 0;
    Return<int32_t> hidlReturn = mSoundTriggerHal->getModelState(handle);
    EXPECT_TRUE(hidlReturn.isOk());
    EXPECT_EQ(-ENOSYS, hidlReturn);
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(SoundTriggerHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    SoundTriggerHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
