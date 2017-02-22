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

#define LOG_TAG "AudioEffectHidlHalTest"
#include <android-base/logging.h>
#include <cutils/native_handle.h>

#include <android/hardware/audio/effect/2.0/IEffectsFactory.h>
#include <android/hardware/audio/effect/2.0/types.h>

#include <gtest/gtest.h>

using ::android::hardware::audio::common::V2_0::Uuid;
using ::android::hardware::audio::effect::V2_0::EffectDescriptor;
using ::android::hardware::audio::effect::V2_0::IEffect;
using ::android::hardware::audio::effect::V2_0::IEffectsFactory;
using ::android::hardware::audio::effect::V2_0::Result;
using ::android::hardware::Return;
using ::android::hardware::Status;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::sp;

// The main test class for Audio Effect HIDL HAL.
class AudioEffectHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    effectsFactory = IEffectsFactory::getService();
    ASSERT_NE(effectsFactory, nullptr);
  }

  virtual void TearDown() override {}

  sp<IEffectsFactory> effectsFactory;
};

// A class for test environment setup (kept since this file is a template).
class AudioEffectHidlEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}

 private:
};

TEST_F(AudioEffectHidlTest, EnumerateEffects) {
  Result retval = Result::NOT_INITIALIZED;
  size_t effectCount = 0;
  Return<void> ret = effectsFactory->getAllDescriptors(
      [&](Result r, const hidl_vec<EffectDescriptor>& result) {
        retval = r;
        effectCount = result.size();
      });
  EXPECT_TRUE(ret.isOk());
  EXPECT_EQ(retval, Result::OK);
  EXPECT_GT(effectCount, 0u);
}

TEST_F(AudioEffectHidlTest, CreateEffect) {
  bool gotEffect = false;
  Uuid effectUuid;
  Return<void> ret = effectsFactory->getAllDescriptors(
      [&](Result r, const hidl_vec<EffectDescriptor>& result) {
        if (r == Result::OK && result.size() > 0) {
          gotEffect = true;
          effectUuid = result[0].uuid;
        }
      });
  ASSERT_TRUE(ret.isOk());
  ASSERT_TRUE(gotEffect);
  Result retval = Result::NOT_INITIALIZED;
  sp<IEffect> effect;
  ret = effectsFactory->createEffect(
      effectUuid, 1 /* session */, 1 /* ioHandle */,
      [&](Result r, const sp<IEffect>& result, uint64_t /*effectId*/) {
        retval = r;
        if (r == Result::OK) {
          effect = result;
        }
      });
  EXPECT_TRUE(ret.isOk());
  EXPECT_EQ(retval, Result::OK);
  EXPECT_NE(effect, nullptr);
}

TEST_F(AudioEffectHidlTest, GetDescriptor) {
  hidl_vec<EffectDescriptor> allDescriptors;
  Return<void> ret = effectsFactory->getAllDescriptors(
      [&](Result r, const hidl_vec<EffectDescriptor>& result) {
        if (r == Result::OK) {
          allDescriptors = result;
        }
      });
  ASSERT_TRUE(ret.isOk());
  ASSERT_GT(allDescriptors.size(), 0u);
  for (size_t i = 0; i < allDescriptors.size(); ++i) {
    ret = effectsFactory->getDescriptor(
        allDescriptors[i].uuid, [&](Result r, const EffectDescriptor& result) {
          EXPECT_EQ(r, Result::OK);
          EXPECT_EQ(result, allDescriptors[i]);
        });
  }
  EXPECT_TRUE(ret.isOk());
}

int main(int argc, char** argv) {
  ::testing::AddGlobalTestEnvironment(new AudioEffectHidlEnvironment);
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;
  return status;
}
