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
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::media::audio::common::AudioUuid;

class EffectFactoryHelper {
  public:
    EffectFactoryHelper(const std::string& name) : mServiceName(name) {}

    void ConnectToFactoryService() {
        mEffectFactory = IFactory::fromBinder(binderUtil.connectToService(mServiceName));
        ASSERT_NE(mEffectFactory, nullptr);
    }

    void RestartFactoryService() {
        ASSERT_NE(mEffectFactory, nullptr);
        mEffectFactory = IFactory::fromBinder(binderUtil.restartService());
        ASSERT_NE(mEffectFactory, nullptr);
    }

    void QueryAllEffects() {
        EXPECT_NE(mEffectFactory, nullptr);
        ScopedAStatus status =
                mEffectFactory->queryEffects(std::nullopt, std::nullopt, &mCompleteIds);
        EXPECT_EQ(status.getExceptionCode(), EX_NONE);
    }

    void QueryEffects(const std::optional<AudioUuid>& in_type,
                      const std::optional<AudioUuid>& in_instance,
                      std::vector<Descriptor::Identity>* _aidl_return) {
        EXPECT_NE(mEffectFactory, nullptr);
        ScopedAStatus status = mEffectFactory->queryEffects(in_type, in_instance, _aidl_return);
        EXPECT_EQ(status.getExceptionCode(), EX_NONE);
        mIds = *_aidl_return;
    }

    void CreateEffects() {
        EXPECT_NE(mEffectFactory, nullptr);
        ScopedAStatus status;
        for (const auto& id : mIds) {
            std::shared_ptr<IEffect> effect;
            status = mEffectFactory->createEffect(id.uuid, &effect);
            EXPECT_EQ(status.getExceptionCode(), EX_NONE) << id.toString();
            EXPECT_NE(effect, nullptr) << id.toString();
            mEffectIdMap[effect] = id;
        }
    }

    void DestroyEffects() {
        EXPECT_NE(mEffectFactory, nullptr);
        ScopedAStatus status;
        for (const auto& it : mEffectIdMap) {
            status = mEffectFactory->destroyEffect(it.first);
            EXPECT_EQ(status.getExceptionCode(), EX_NONE) << it.second.toString();
        }
        mEffectIdMap.clear();
    }

    std::shared_ptr<IFactory> GetFactory() { return mEffectFactory; }
    const std::vector<Descriptor::Identity>& GetEffectIds() { return mIds; }
    const std::vector<Descriptor::Identity>& GetCompleteEffectIdList() { return mCompleteIds; }
    const std::unordered_map<std::shared_ptr<IEffect>, Descriptor::Identity>& GetEffectMap() {
        return mEffectIdMap;
    }

  private:
    std::shared_ptr<IFactory> mEffectFactory;
    std::string mServiceName;
    AudioHalBinderServiceUtil binderUtil;
    std::vector<Descriptor::Identity> mIds;
    std::vector<Descriptor::Identity> mCompleteIds;
    std::unordered_map<std::shared_ptr<IEffect>, Descriptor::Identity> mEffectIdMap;
};

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
    EXPECT_NE(static_cast<int>(descriptors.size()), 0);
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
    EXPECT_EQ(static_cast<int>(descriptors.size()), 0);
}

TEST_P(EffectFactoryTest, QueriedDescriptorNotExistInstance) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(std::nullopt, nullUuid, &descriptors);
    EXPECT_EQ(static_cast<int>(descriptors.size()), 0);
}

TEST_P(EffectFactoryTest, CreateAndDestroyRepeat) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(std::nullopt, std::nullopt, &descriptors);
    int numIds = static_cast<int>(mFactory.GetEffectIds().size());
    EXPECT_NE(numIds, 0);

    EXPECT_EQ(static_cast<int>(mFactory.GetEffectMap().size()), 0);
    mFactory.CreateEffects();
    EXPECT_EQ(static_cast<int>(mFactory.GetEffectMap().size()), numIds);
    mFactory.DestroyEffects();
    EXPECT_EQ(static_cast<int>(mFactory.GetEffectMap().size()), 0);

    // Create and destroy again
    mFactory.CreateEffects();
    EXPECT_EQ(static_cast<int>(mFactory.GetEffectMap().size()), numIds);
    mFactory.DestroyEffects();
    EXPECT_EQ(static_cast<int>(mFactory.GetEffectMap().size()), 0);
}

TEST_P(EffectFactoryTest, CreateMultipleInstanceOfSameEffect) {
    std::vector<Descriptor::Identity> descriptors;
    mFactory.QueryEffects(std::nullopt, std::nullopt, &descriptors);
    int numIds = static_cast<int>(mFactory.GetEffectIds().size());
    EXPECT_NE(numIds, 0);

    EXPECT_EQ(static_cast<int>(mFactory.GetEffectMap().size()), 0);
    mFactory.CreateEffects();
    EXPECT_EQ(static_cast<int>(mFactory.GetEffectMap().size()), numIds);
    // Create effect instances of same implementation
    mFactory.CreateEffects();
    EXPECT_EQ(static_cast<int>(mFactory.GetEffectMap().size()), 2 * numIds);

    mFactory.CreateEffects();
    EXPECT_EQ(static_cast<int>(mFactory.GetEffectMap().size()), 3 * numIds);

    mFactory.DestroyEffects();
    EXPECT_EQ(static_cast<int>(mFactory.GetEffectMap().size()), 0);
}

INSTANTIATE_TEST_SUITE_P(EffectFactoryTest, EffectFactoryTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IFactory::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EffectFactoryTest);

/// Effect testing.
class AudioEffect : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(mFactory.ConnectToFactoryService());
        ASSERT_NO_FATAL_FAILURE(mFactory.CreateEffects());
    }

    void TearDown() override {
        CloseEffects();
        ASSERT_NO_FATAL_FAILURE(mFactory.DestroyEffects());
    }

    void OpenEffects() {
        auto open = [](const std::shared_ptr<IEffect>& effect) {
            ScopedAStatus status = effect->open();
            EXPECT_EQ(status.getExceptionCode(), EX_NONE);
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(open));
    }

    void CloseEffects() {
        auto close = [](const std::shared_ptr<IEffect>& effect) {
            ScopedAStatus status = effect->close();
            EXPECT_EQ(status.getExceptionCode(), EX_NONE);
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(close));
    }

    void GetEffectDescriptors() {
        auto get = [](const std::shared_ptr<IEffect>& effect) {
            Descriptor desc;
            ScopedAStatus status = effect->getDescriptor(&desc);
            EXPECT_EQ(status.getExceptionCode(), EX_NONE);
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(get));
    }

    template <typename Functor>
    void ForEachEffect(Functor functor) {
        auto effectMap = mFactory.GetEffectMap();
        ScopedAStatus status;
        for (const auto& it : effectMap) {
            SCOPED_TRACE(it.second.toString());
            functor(it.first);
        }
    }

    EffectFactoryHelper mFactory = EffectFactoryHelper(GetParam());
};

TEST_P(AudioEffect, OpenEffectTest) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
}

TEST_P(AudioEffect, OpenAndCloseEffect) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

TEST_P(AudioEffect, CloseUnopenedEffectTest) {
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

TEST_P(AudioEffect, DoubleOpenCloseEffects) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());

    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());

    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

TEST_P(AudioEffect, GetDescriptors) {
    EXPECT_NO_FATAL_FAILURE(GetEffectDescriptors());
}

TEST_P(AudioEffect, DescriptorIdExistAndUnique) {
    auto checker = [&](const std::shared_ptr<IEffect>& effect) {
        Descriptor desc;
        std::vector<Descriptor::Identity> idList;
        ScopedAStatus status = effect->getDescriptor(&desc);
        EXPECT_EQ(status.getExceptionCode(), EX_NONE);
        mFactory.QueryEffects(desc.common.id.type, desc.common.id.uuid, &idList);
        EXPECT_EQ(static_cast<int>(idList.size()), 1);
    };
    EXPECT_NO_FATAL_FAILURE(ForEachEffect(checker));

    // Check unique with a set
    auto stringHash = [](const Descriptor::Identity& id) {
        return std::hash<std::string>()(id.toString());
    };
    auto vec = mFactory.GetCompleteEffectIdList();
    std::unordered_set<Descriptor::Identity, decltype(stringHash)> idSet(0, stringHash);
    for (auto it : vec) {
        EXPECT_EQ(static_cast<int>(idSet.count(it)), 0);
        idSet.insert(it);
    }
}

INSTANTIATE_TEST_SUITE_P(AudioEffectTest, AudioEffect,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IFactory::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioEffect);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}