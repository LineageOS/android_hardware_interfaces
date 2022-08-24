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

#include <string>

#define LOG_TAG "VtsHalAudioEffect"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <aidl/android/hardware/audio/effect/IFactory.h>

#include "AudioHalBinderServiceUtil.h"

using namespace android;

using ndk::ScopedAStatus;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::media::audio::common::AudioUuid;

namespace ndk {
std::ostream& operator<<(std::ostream& str, const ScopedAStatus& status) {
    str << status.getDescription();
    return str;
}
}  // namespace ndk

class EffectFactory : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override { ASSERT_NO_FATAL_FAILURE(ConnectToService()); }

    void TearDown() override {}

    void ConnectToService() {
        serviceName = GetParam();
        factory = IFactory::fromBinder(binderUtil.connectToService(serviceName));
        ASSERT_NE(factory, nullptr);
    }

    void RestartService() {
        ASSERT_NE(factory, nullptr);
        factory = IFactory::fromBinder(binderUtil.restartService());
        ASSERT_NE(factory, nullptr);
    }

    std::shared_ptr<IFactory> factory;
    std::string serviceName;
    AudioHalBinderServiceUtil binderUtil;
    // TODO: these UUID can get from config file
    // ec7178ec-e5e1-4432-a3f4-4657e6795210
    const AudioUuid nullUuid = {static_cast<int32_t>(0xec7178ec),
                                0xe5e1,
                                0x4432,
                                0xa3f4,
                                {0x46, 0x57, 0xe6, 0x79, 0x52, 0x10}};
    const AudioUuid zeroUuid = {
            static_cast<int32_t>(0x0), 0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
};

TEST_P(EffectFactory, SetupAndTearDown) {
    // Intentionally empty test body.
}

TEST_P(EffectFactory, CanBeRestarted) {
    ASSERT_NO_FATAL_FAILURE(RestartService());
}

TEST_P(EffectFactory, QueriedDescriptorList) {
    std::vector<Descriptor::Identity> descriptors;
    ScopedAStatus status = factory->queryEffects(std::nullopt, std::nullopt, &descriptors);
    EXPECT_EQ(EX_NONE, status.getExceptionCode());
    EXPECT_NE(static_cast<int>(descriptors.size()), 0);
}

TEST_P(EffectFactory, DescriptorUUIDNotNull) {
    std::vector<Descriptor::Identity> descriptors;
    ScopedAStatus status = factory->queryEffects(std::nullopt, std::nullopt, &descriptors);
    EXPECT_EQ(EX_NONE, status.getExceptionCode());
    // TODO: Factory eventually need to return the full list of MUST supported AOSP effects.
    for (auto& desc : descriptors) {
        EXPECT_NE(desc.type, zeroUuid);
        EXPECT_NE(desc.uuid, zeroUuid);
    }
}

TEST_P(EffectFactory, QueriedDescriptorNotExistType) {
    std::vector<Descriptor::Identity> descriptors;
    ScopedAStatus status = factory->queryEffects(nullUuid, std::nullopt, &descriptors);
    EXPECT_EQ(EX_NONE, status.getExceptionCode());
    EXPECT_EQ(static_cast<int>(descriptors.size()), 0);
}

TEST_P(EffectFactory, QueriedDescriptorNotExistInstance) {
    std::vector<Descriptor::Identity> descriptors;
    ScopedAStatus status = factory->queryEffects(std::nullopt, nullUuid, &descriptors);
    EXPECT_EQ(EX_NONE, status.getExceptionCode());
    EXPECT_EQ(static_cast<int>(descriptors.size()), 0);
}

INSTANTIATE_TEST_SUITE_P(EffectFactoryTest, EffectFactory,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IFactory::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EffectFactory);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
