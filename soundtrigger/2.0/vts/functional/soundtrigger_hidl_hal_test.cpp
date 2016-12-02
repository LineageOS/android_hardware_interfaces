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

#define LOG_TAG "SoundTriggerHidlHalTest"
#include <android/log.h>
#include <cutils/native_handle.h>

#include <android/hardware/audio/common/2.0/types.h>
#include <android/hardware/soundtrigger/2.0/ISoundTriggerHw.h>
#include <android/hardware/soundtrigger/2.0/types.h>

#include <gtest/gtest.h>

using ::android::hardware::audio::common::V2_0::AudioDevice;
using ::android::hardware::soundtrigger::V2_0::SoundModelHandle;
using ::android::hardware::soundtrigger::V2_0::SoundModelType;
using ::android::hardware::soundtrigger::V2_0::RecognitionMode;
using ::android::hardware::soundtrigger::V2_0::PhraseRecognitionExtra;
using ::android::hardware::soundtrigger::V2_0::ISoundTriggerHw;
using ::android::hardware::soundtrigger::V2_0::ISoundTriggerHwCallback;
using ::android::hardware::Return;
using ::android::hardware::Status;
using ::android::hardware::Void;
using ::android::sp;

// The main test class for Sound Trigger HIDL HAL.
class SoundTriggerHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    mSoundTriggerHal = ISoundTriggerHw::getService("sound_trigger.primary", false);
    ASSERT_NE(nullptr, mSoundTriggerHal.get());
    ASSERT_TRUE(mSoundTriggerHal->isRemote());
    mCallback = new MyCallback();
    ASSERT_NE(nullptr, mCallback.get());
  }

  class MyCallback : public ISoundTriggerHwCallback {
      virtual Return<void> recognitionCallback(
                  const ISoundTriggerHwCallback::RecognitionEvent& event __unused,
                  int32_t cookie __unused) {
          ALOGI("%s", __FUNCTION__);
          return Void();
      }

      virtual Return<void> phraseRecognitionCallback(
              const ISoundTriggerHwCallback::PhraseRecognitionEvent& event __unused,
              int32_t cookie __unused) {
          ALOGI("%s", __FUNCTION__);
          return Void();
      }

      virtual Return<void> soundModelCallback(
              const ISoundTriggerHwCallback::ModelEvent& event __unused,
              int32_t cookie __unused) {
          ALOGI("%s", __FUNCTION__);
          return Void();
      }
  };

  virtual void TearDown() override {}

  sp<ISoundTriggerHw> mSoundTriggerHal;
  sp<MyCallback> mCallback;
};

// A class for test environment setup (kept since this file is a template).
class SoundTriggerHidlEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}

 private:
};

/**
 * Test ISoundTriggerHw::getProperties() method
 *
 * Verifies that:
 *  - the implementation implements the method
 *  - the method returns 0 (no error)
 *  - the implementation supports at least one sound model and one key phrase
 *  - the implementation supports at least VOICE_TRIGGER recognition mode
 */
TEST_F(SoundTriggerHidlTest, GetProperties) {
  ISoundTriggerHw::Properties halProperties;
  Return<void> hidlReturn;
  int ret = -ENODEV;

  hidlReturn = mSoundTriggerHal->getProperties([&](int rc, auto res) {
      ret = rc;
      halProperties = res;
  });

  EXPECT_EQ(Status::EX_NONE, hidlReturn.getStatus().exceptionCode());
  EXPECT_EQ(0, ret);
  EXPECT_GT(halProperties.maxSoundModels, 0u);
  EXPECT_GT(halProperties.maxKeyPhrases, 0u);
  EXPECT_NE(0u, (halProperties.recognitionModes & (uint32_t)RecognitionMode::VOICE_TRIGGER));
}

/**
 * Test ISoundTriggerHw::loadPhraseSoundModel() method
 *
 * Verifies that:
 *  - the implementation implements the method
 *  - the implementation returns an error when passed a malformed sound model
 *
 * There is no way to verify that implementation actually can load a sound model because each
 * sound model is vendor specific.
 */
TEST_F(SoundTriggerHidlTest, LoadInvalidModelFail) {
  Return<void> hidlReturn;
  int ret = -ENODEV;
  ISoundTriggerHw::PhraseSoundModel model;
  SoundModelHandle handle;

  model.common.type = SoundModelType::UNKNOWN;

  hidlReturn = mSoundTriggerHal->loadPhraseSoundModel(
          model,
          mCallback, 0, [&](int32_t retval, auto res) {
      ret = retval;
      handle = res;
  });

  EXPECT_EQ(Status::EX_NONE, hidlReturn.getStatus().exceptionCode());
  EXPECT_NE(0, ret);
}

/**
 * Test ISoundTriggerHw::unloadSoundModel() method
 *
 * Verifies that:
 *  - the implementation implements the method
 *  - the implementation returns an error when called without a valid loaded sound model
 *
 */
TEST_F(SoundTriggerHidlTest, UnloadModelNoModelFail) {
  Return<int32_t> hidlReturn(0);
  SoundModelHandle halHandle = 0;

  hidlReturn = mSoundTriggerHal->unloadSoundModel(halHandle);

  EXPECT_EQ(Status::EX_NONE, hidlReturn.getStatus().exceptionCode());
  EXPECT_NE(0, hidlReturn);
}

/**
 * Test ISoundTriggerHw::startRecognition() method
 *
 * Verifies that:
 *  - the implementation implements the method
 *  - the implementation returns an error when called without a valid loaded sound model
 *
 * There is no way to verify that implementation actually starts recognition because no model can
 * be loaded.
 */
TEST_F(SoundTriggerHidlTest, StartRecognitionNoModelFail) {
    Return<int32_t> hidlReturn(0);
    SoundModelHandle handle = 0;
    PhraseRecognitionExtra phrase;
    ISoundTriggerHw::RecognitionConfig config;

    config.captureHandle = 0;
    config.captureDevice = AudioDevice::IN_BUILTIN_MIC;
    phrase.id = 0;
    phrase.recognitionModes = (uint32_t)RecognitionMode::VOICE_TRIGGER;
    phrase.confidenceLevel = 0;

    config.phrases.setToExternal(&phrase, 1);

    hidlReturn = mSoundTriggerHal->startRecognition(handle, config, mCallback, 0);

    EXPECT_EQ(Status::EX_NONE, hidlReturn.getStatus().exceptionCode());
    EXPECT_NE(0, hidlReturn);
}

/**
 * Test ISoundTriggerHw::stopRecognition() method
 *
 * Verifies that:
 *  - the implementation implements the method
 *  - the implementation returns an error when called without an active recognition running
 *
 */
TEST_F(SoundTriggerHidlTest, StopRecognitionNoAStartFail) {
    Return<int32_t> hidlReturn(0);
    SoundModelHandle handle = 0;

    hidlReturn = mSoundTriggerHal->stopRecognition(handle);

    EXPECT_EQ(Status::EX_NONE, hidlReturn.getStatus().exceptionCode());
    EXPECT_NE(0, hidlReturn);
}

/**
 * Test ISoundTriggerHw::stopAllRecognitions() method
 *
 * Verifies that:
 *  - the implementation implements this optional method or indicates it is not support by
 *  returning -ENOSYS
 */
TEST_F(SoundTriggerHidlTest, stopAllRecognitions) {
    Return<int32_t> hidlReturn(0);
    SoundModelHandle handle = 0;

    hidlReturn = mSoundTriggerHal->stopAllRecognitions();

    EXPECT_EQ(Status::EX_NONE, hidlReturn.getStatus().exceptionCode());
    EXPECT_TRUE(hidlReturn == 0 || hidlReturn == -ENOSYS);
}


int main(int argc, char** argv) {
  ::testing::AddGlobalTestEnvironment(new SoundTriggerHidlEnvironment);
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
