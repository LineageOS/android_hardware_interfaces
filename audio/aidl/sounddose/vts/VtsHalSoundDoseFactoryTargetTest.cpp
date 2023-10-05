/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "VtsHalSoundDose.Factory"
#include <android-base/logging.h>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/audio/sounddose/ISoundDoseFactory.h>
#include <android/binder_manager.h>

#include <memory>

namespace android::hardware::audio::common::testing {

namespace detail {

inline ::testing::AssertionResult assertIsOk(const char* expr, const ::ndk::ScopedAStatus& status) {
    if (status.isOk()) {
        return ::testing::AssertionSuccess();
    }
    return ::testing::AssertionFailure()
           << "Expected the transaction \'" << expr << "\' to succeed\n"
           << "  but it has failed with: " << status;
}

}  // namespace detail

}  // namespace android::hardware::audio::common::testing

// Test that the transaction status 'isOk'
#define EXPECT_IS_OK(ret) \
    EXPECT_PRED_FORMAT1(::android::hardware::audio::common::testing::detail::assertIsOk, ret)

using namespace android;

using aidl::android::hardware::audio::core::sounddose::ISoundDose;
using aidl::android::hardware::audio::sounddose::ISoundDoseFactory;

class SoundDoseFactory : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override { ASSERT_NO_FATAL_FAILURE(ConnectToService(GetParam())); }

    void TearDown() override {}

    void ConnectToService(const std::string& interfaceName) {
        ndk::SpAIBinder binder =
                ndk::SpAIBinder(AServiceManager_waitForService(interfaceName.c_str()));
        if (binder == nullptr) {
            LOG(ERROR) << "Failed to get service " << interfaceName;
        } else {
            LOG(DEBUG) << "Succeeded to get service " << interfaceName;
        }
        soundDoseFactory = ISoundDoseFactory::fromBinder(binder);
        ASSERT_NE(soundDoseFactory, nullptr);
    }

    std::shared_ptr<ISoundDoseFactory> soundDoseFactory;
};

// @VsrTest = VSR-5.5-002.001
TEST_P(SoundDoseFactory, GetSoundDoseForSameModule) {
    const std::string module = "primary";

    std::shared_ptr<ISoundDose> soundDose1;
    EXPECT_IS_OK(soundDoseFactory->getSoundDose(module, &soundDose1));

    if (soundDose1 == nullptr) {
        LOG(WARNING) << "Primary module does not support sound dose";
        return;
    }

    std::shared_ptr<ISoundDose> soundDose2;
    EXPECT_IS_OK(soundDoseFactory->getSoundDose(module, &soundDose2));
    EXPECT_NE(nullptr, soundDose2);
    EXPECT_EQ(soundDose1->asBinder(), soundDose2->asBinder())
            << "getSoundDose must return the same interface for the same module";
}

INSTANTIATE_TEST_SUITE_P(
        SoundDoseFactoryTest, SoundDoseFactory,
        testing::ValuesIn(android::getAidlHalInstanceNames(ISoundDoseFactory::descriptor)),
        android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SoundDoseFactory);
