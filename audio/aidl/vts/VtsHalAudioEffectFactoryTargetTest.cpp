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

#include <memory>
#include <string>
#include <vector>

#define LOG_TAG "VtsHalAudioEffectFactory"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <aidl/android/hardware/audio/effect/IFactory.h>

#include "AudioHalBinderServiceUtil.h"
#include "EffectFactoryHelper.h"
#include "TestUtils.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::media::audio::common::AudioUuid;

/// Effect factory testing.
class EffectFactoryTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override { ASSERT_NO_FATAL_FAILURE(mFactory.ConnectToFactoryService()); }

    void TearDown() override { mFactory.DestroyEffects(); }

    EffectFactoryHelper mFactory = EffectFactoryHelper(GetParam());

    // TODO: these UUID can get from config file
    // ec7178ec-e5e1-4432-a3f4-4657e6795210
    const AudioUuid nullUuid = {static_cast<int32_t>(0xec7178ec),
                                0xe5e1,
                                0x4432,
                                0xa3f4,
                                {0x46, 0x57, 0xe6, 0x79, 0x52, 0x10}};
    const AudioUuid zeroUuid = {
            static_cast<int32_t>(0x0), 0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
    const Descriptor::Identity nullDesc = {.uuid = nullUuid};
    const Descriptor::Identity zeroDesc = {.uuid = zeroUuid};
};

TEST_P(EffectFactoryTest, SetupAndTearDown) {
    // Intentionally empty test body.
}

TEST_P(EffectFactoryTest, CanBeRestarted) {
    ASSERT_NO_FATAL_FAILURE(mFactory.RestartFactoryService());
}

TEST_P(EffectFactoryTest, QueriedDescriptorList) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(std::nullopt, std::nullopt, &descriptors);
    EXPECT_NE(descriptors.size(), 0UL);
}

TEST_P(EffectFactoryTest, DescriptorUUIDNotNull) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(std::nullopt, std::nullopt, &descriptors);
    // TODO: Factory eventually need to return the full list of MUST supported AOSP effects.
    for (auto& desc : descriptors) {
        EXPECT_NE(desc.type, zeroUuid);
        EXPECT_NE(desc.uuid, zeroUuid);
    }
}

TEST_P(EffectFactoryTest, QueriedDescriptorNotExistType) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(nullUuid, std::nullopt, &descriptors);
    EXPECT_EQ(descriptors.size(), 0UL);
}

TEST_P(EffectFactoryTest, QueriedDescriptorNotExistInstance) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(std::nullopt, nullUuid, &descriptors);
    EXPECT_EQ(descriptors.size(), 0UL);
}

TEST_P(EffectFactoryTest, CreateAndDestroyOnce) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(std::nullopt, std::nullopt, &descriptors);
    auto numIds = mFactory.GetEffectIds().size();
    EXPECT_NE(numIds, 0UL);

    auto& effectMap = mFactory.GetEffectMap();
    EXPECT_EQ(effectMap.size(), 0UL);
    mFactory.CreateEffects();
    EXPECT_EQ(effectMap.size(), numIds);
    mFactory.DestroyEffects();
    EXPECT_EQ(effectMap.size(), 0UL);
}

TEST_P(EffectFactoryTest, CreateAndDestroyRepeat) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(std::nullopt, std::nullopt, &descriptors);
    auto numIds = mFactory.GetEffectIds().size();
    EXPECT_NE(numIds, 0UL);

    auto& effectMap = mFactory.GetEffectMap();
    EXPECT_EQ(effectMap.size(), 0UL);
    mFactory.CreateEffects();
    EXPECT_EQ(effectMap.size(), numIds);
    mFactory.DestroyEffects();
    EXPECT_EQ(effectMap.size(), 0UL);

    // Create and destroy again
    mFactory.CreateEffects();
    EXPECT_EQ(effectMap.size(), numIds);
    mFactory.DestroyEffects();
    EXPECT_EQ(effectMap.size(), 0UL);
}

TEST_P(EffectFactoryTest, CreateMultipleInstanceOfSameEffect) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(std::nullopt, std::nullopt, &descriptors);
    auto numIds = mFactory.GetEffectIds().size();
    EXPECT_NE(numIds, 0UL);

    auto& effectMap = mFactory.GetEffectMap();
    EXPECT_EQ(effectMap.size(), 0UL);
    mFactory.CreateEffects();
    EXPECT_EQ(effectMap.size(), numIds);
    // Create effect instances of same implementation
    mFactory.CreateEffects();
    EXPECT_EQ(effectMap.size(), 2 * numIds);

    mFactory.CreateEffects();
    EXPECT_EQ(effectMap.size(), 3 * numIds);

    mFactory.DestroyEffects();
    EXPECT_EQ(effectMap.size(), 0UL);
}

// Expect EX_ILLEGAL_ARGUMENT when create with invalid UUID.
TEST_P(EffectFactoryTest, CreateWithInvalidUuid) {
    std::vector<std::pair<Descriptor::Identity, binder_status_t>> descriptors;
    descriptors.push_back(std::make_pair(nullDesc, EX_ILLEGAL_ARGUMENT));
    descriptors.push_back(std::make_pair(zeroDesc, EX_ILLEGAL_ARGUMENT));

    auto& effectMap = mFactory.GetEffectMap();
    mFactory.CreateEffectsAndExpect(descriptors);
    EXPECT_EQ(effectMap.size(), 0UL);
}

// Expect EX_ILLEGAL_ARGUMENT when destroy null interface.
TEST_P(EffectFactoryTest, DestroyWithInvalidInterface) {
    std::shared_ptr<IEffect> spDummyEffect(nullptr);

    mFactory.DestroyEffectAndExpect(spDummyEffect, EX_ILLEGAL_ARGUMENT);
}

TEST_P(EffectFactoryTest, CreateAndRemoveReference) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(std::nullopt, std::nullopt, &descriptors);
    auto numIds = mFactory.GetEffectIds().size();
    EXPECT_NE(numIds, 0UL);

    auto& effectMap = mFactory.GetEffectMap();
    EXPECT_EQ(effectMap.size(), 0UL);
    mFactory.CreateEffects();
    EXPECT_EQ(effectMap.size(), numIds);
    // remove all reference
    mFactory.ClearEffectMap();
    EXPECT_EQ(effectMap.size(), 0UL);
}

TEST_P(EffectFactoryTest, CreateRemoveReferenceAndCreateDestroy) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(std::nullopt, std::nullopt, &descriptors);
    auto numIds = mFactory.GetEffectIds().size();
    EXPECT_NE(numIds, 0UL);

    auto& effectMap = mFactory.GetEffectMap();
    EXPECT_EQ(effectMap.size(), 0UL);
    mFactory.CreateEffects();
    EXPECT_EQ(effectMap.size(), numIds);
    // remove all reference
    mFactory.ClearEffectMap();
    EXPECT_EQ(effectMap.size(), 0UL);

    // Create and destroy again
    mFactory.CreateEffects();
    EXPECT_EQ(effectMap.size(), numIds);
    mFactory.DestroyEffects();
    EXPECT_EQ(effectMap.size(), 0UL);
}

TEST_P(EffectFactoryTest, CreateRestartAndCreateDestroy) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(std::nullopt, std::nullopt, &descriptors);
    auto numIds = mFactory.GetEffectIds().size();
    auto& effectMap = mFactory.GetEffectMap();
    mFactory.CreateEffects();
    EXPECT_EQ(effectMap.size(), numIds);
    ASSERT_NO_FATAL_FAILURE(mFactory.RestartFactoryService());

    mFactory.CreateEffects();
    EXPECT_EQ(effectMap.size(), numIds);
    mFactory.DestroyEffects();
    EXPECT_EQ(effectMap.size(), 0UL);
}

INSTANTIATE_TEST_SUITE_P(EffectFactoryTest, EffectFactoryTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IFactory::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EffectFactoryTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}