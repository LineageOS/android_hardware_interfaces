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
#include <set>
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
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::kEffectNullUuid;
using aidl::android::hardware::audio::effect::kEffectZeroUuid;
using aidl::android::hardware::audio::effect::Processing;
using aidl::android::media::audio::common::AudioSource;
using aidl::android::media::audio::common::AudioStreamType;
using aidl::android::media::audio::common::AudioUuid;

/// Effect factory testing.
class EffectFactoryTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        mFactoryHelper = std::make_unique<EffectFactoryHelper>(GetParam());
        connectAndGetFactory();
    }

    void TearDown() override {
        for (auto& effect : mEffects) {
            const auto status = mEffectFactory->destroyEffect(effect);
            EXPECT_STATUS(EX_NONE, status);
        }
    }

    std::unique_ptr<EffectFactoryHelper> mFactoryHelper;
    std::shared_ptr<IFactory> mEffectFactory;
    std::vector<std::shared_ptr<IEffect>> mEffects;
    const Descriptor::Identity kNullDesc = {.uuid = kEffectNullUuid};
    const Descriptor::Identity kZeroDesc = {.uuid = kEffectZeroUuid};

    template <typename Functor>
    void ForEachId(const std::vector<Descriptor::Identity> ids, Functor functor) {
        for (const auto& id : ids) {
            SCOPED_TRACE(id.toString());
            functor(id);
        }
    }
    template <typename Functor>
    void ForEachEffect(std::vector<std::shared_ptr<IEffect>> effects, Functor functor) {
        for (auto& effect : effects) {
            functor(effect);
        }
    }

    std::vector<std::shared_ptr<IEffect>> createWithIds(
            const std::vector<Descriptor::Identity> ids,
            const binder_status_t expectStatus = EX_NONE) {
        std::vector<std::shared_ptr<IEffect>> effects;
        for (const auto& id : ids) {
            std::shared_ptr<IEffect> effect;
            EXPECT_STATUS(expectStatus, mEffectFactory->createEffect(id.uuid, &effect));
            if (expectStatus == EX_NONE) {
                EXPECT_NE(effect, nullptr) << " null effect with uuid: " << id.uuid.toString();
                effects.push_back(std::move(effect));
            }
        }
        return effects;
    }
    void destroyEffects(std::vector<std::shared_ptr<IEffect>> effects,
                        const binder_status_t expectStatus = EX_NONE) {
        for (const auto& effect : effects) {
            EXPECT_STATUS(expectStatus, mEffectFactory->destroyEffect(effect));
        }
    }
    void creatAndDestroyIds(const std::vector<Descriptor::Identity> ids) {
        for (const auto& id : ids) {
            auto effects = createWithIds({id});
            ASSERT_NO_FATAL_FAILURE(destroyEffects(effects));
        }
    }
    void connectAndGetFactory() {
        ASSERT_NO_FATAL_FAILURE(mFactoryHelper->ConnectToFactoryService());
        mEffectFactory = mFactoryHelper->GetFactory();
        ASSERT_NE(mEffectFactory, nullptr);
    }
};

TEST_P(EffectFactoryTest, SetupAndTearDown) {
    // Intentionally empty test body.
}

TEST_P(EffectFactoryTest, CanBeRestarted) {
    ASSERT_NO_FATAL_FAILURE(mFactoryHelper->RestartFactoryService());
}

/**
 * @brief Check at least support list of effect must be supported by aosp:
 * https://developer.android.com/reference/android/media/audiofx/AudioEffect
 */
TEST_P(EffectFactoryTest, ExpectAllAospEffectTypes) {
    std::vector<Descriptor::Identity> ids;
    std::set<AudioUuid> typeUuidSet(
            {aidl::android::hardware::audio::effect::kBassBoostTypeUUID,
             aidl::android::hardware::audio::effect::kEqualizerTypeUUID,
             aidl::android::hardware::audio::effect::kEnvReverbTypeUUID,
             aidl::android::hardware::audio::effect::kPresetReverbTypeUUID,
             aidl::android::hardware::audio::effect::kDynamicsProcessingTypeUUID,
             aidl::android::hardware::audio::effect::kHapticGeneratorTypeUUID,
             aidl::android::hardware::audio::effect::kVirtualizerTypeUUID});

    EXPECT_IS_OK(mEffectFactory->queryEffects(std::nullopt, std::nullopt, std::nullopt, &ids));
    EXPECT_TRUE(ids.size() >= typeUuidSet.size());
    for (const auto& id : ids) {
        typeUuidSet.erase(id.type);
    }
    std::string msg = " missing type UUID:\n";
    for (const auto& uuid : typeUuidSet) {
        msg += (uuid.toString() + "\n");
    }
    SCOPED_TRACE(msg);
    EXPECT_EQ(0UL, typeUuidSet.size());
}

TEST_P(EffectFactoryTest, QueryNullTypeUuid) {
    std::vector<Descriptor::Identity> ids;
    EXPECT_IS_OK(mEffectFactory->queryEffects(kEffectNullUuid, std::nullopt, std::nullopt, &ids));
    EXPECT_EQ(ids.size(), 0UL);
}

TEST_P(EffectFactoryTest, QueriedNullImplUuid) {
    std::vector<Descriptor::Identity> ids;
    EXPECT_IS_OK(mEffectFactory->queryEffects(std::nullopt, kEffectNullUuid, std::nullopt, &ids));
    EXPECT_EQ(ids.size(), 0UL);
}

TEST_P(EffectFactoryTest, QueriedNullProxyUuid) {
    std::vector<Descriptor::Identity> ids;
    EXPECT_IS_OK(mEffectFactory->queryEffects(std::nullopt, std::nullopt, kEffectNullUuid, &ids));
    EXPECT_EQ(ids.size(), 0UL);
}

// create all effects, and then destroy them all together
TEST_P(EffectFactoryTest, CreateAndDestroyEffects) {
    std::vector<Descriptor::Identity> ids;
    EXPECT_IS_OK(mEffectFactory->queryEffects(std::nullopt, std::nullopt, std::nullopt, &ids));
    EXPECT_NE(ids.size(), 0UL);

    std::vector<std::shared_ptr<IEffect>> effects;
    effects = createWithIds(ids);
    EXPECT_EQ(ids.size(), effects.size());
    destroyEffects(effects);
}

TEST_P(EffectFactoryTest, CreateMultipleInstanceOfSameEffect) {
    std::vector<Descriptor::Identity> ids;
    EXPECT_IS_OK(mEffectFactory->queryEffects(std::nullopt, std::nullopt, std::nullopt, &ids));
    EXPECT_NE(ids.size(), 0UL);

    std::vector<std::shared_ptr<IEffect>> effects = createWithIds(ids);
    EXPECT_EQ(ids.size(), effects.size());
    std::vector<std::shared_ptr<IEffect>> effects2 = createWithIds(ids);
    EXPECT_EQ(ids.size(), effects2.size());
    std::vector<std::shared_ptr<IEffect>> effects3 = createWithIds(ids);
    EXPECT_EQ(ids.size(), effects3.size());

    destroyEffects(effects);
    destroyEffects(effects2);
    destroyEffects(effects3);
}

// create and destroy each effect one by one
TEST_P(EffectFactoryTest, CreateAndDestroyEffectsOneByOne) {
    std::vector<Descriptor::Identity> ids;
    EXPECT_IS_OK(mEffectFactory->queryEffects(std::nullopt, std::nullopt, std::nullopt, &ids));
    EXPECT_NE(ids.size(), 0UL);

    creatAndDestroyIds(ids);
}

// for each effect: repeat 3 times create and destroy
TEST_P(EffectFactoryTest, CreateAndDestroyRepeat) {
    std::vector<Descriptor::Identity> ids;
    EXPECT_IS_OK(mEffectFactory->queryEffects(std::nullopt, std::nullopt, std::nullopt, &ids));
    EXPECT_NE(ids.size(), 0UL);

    creatAndDestroyIds(ids);
    creatAndDestroyIds(ids);
    creatAndDestroyIds(ids);
}

// Expect EX_ILLEGAL_ARGUMENT when create with invalid UUID.
TEST_P(EffectFactoryTest, CreateWithInvalidUuid) {
    std::vector<Descriptor::Identity> ids = {kNullDesc, kZeroDesc};
    auto effects = createWithIds(ids, EX_ILLEGAL_ARGUMENT);
    EXPECT_EQ(effects.size(), 0UL);
}

// Expect EX_ILLEGAL_ARGUMENT when destroy null interface.
TEST_P(EffectFactoryTest, DestroyWithInvalidInterface) {
    std::shared_ptr<IEffect> spDummyEffect(nullptr);
    destroyEffects({spDummyEffect}, EX_ILLEGAL_ARGUMENT);
}

// Same descriptor ID should work after service restart.
TEST_P(EffectFactoryTest, CreateDestroyWithRestart) {
    std::vector<Descriptor::Identity> ids;
    EXPECT_IS_OK(mEffectFactory->queryEffects(std::nullopt, std::nullopt, std::nullopt, &ids));
    EXPECT_NE(ids.size(), 0UL);
    creatAndDestroyIds(ids);

    mFactoryHelper->RestartFactoryService();

    connectAndGetFactory();
    creatAndDestroyIds(ids);
}

// Effect handle invalid after restart.
TEST_P(EffectFactoryTest, EffectInvalidAfterRestart) {
    std::vector<Descriptor::Identity> ids;
    EXPECT_IS_OK(mEffectFactory->queryEffects(std::nullopt, std::nullopt, std::nullopt, &ids));
    EXPECT_NE(ids.size(), 0UL);
    std::vector<std::shared_ptr<IEffect>> effects = createWithIds(ids);

    ASSERT_NO_FATAL_FAILURE(mFactoryHelper->RestartFactoryService());

    connectAndGetFactory();
    destroyEffects(effects, EX_ILLEGAL_ARGUMENT);
}

// expect no error with the queryProcessing interface, but don't check number of processing
TEST_P(EffectFactoryTest, QueryProcess) {
    std::vector<Processing> processing;
    EXPECT_IS_OK(mEffectFactory->queryProcessing(std::nullopt, &processing));

    Processing::Type streamType =
            Processing::Type::make<Processing::Type::streamType>(AudioStreamType::SYSTEM);
    std::vector<Processing> processingFilteredByStream;
    EXPECT_IS_OK(mEffectFactory->queryProcessing(streamType, &processingFilteredByStream));

    Processing::Type source =
            Processing::Type::make<Processing::Type::source>(AudioSource::DEFAULT);
    std::vector<Processing> processingFilteredBySource;
    EXPECT_IS_OK(mEffectFactory->queryProcessing(source, &processingFilteredBySource));

    EXPECT_TRUE(processing.size() >= processingFilteredByStream.size());
    EXPECT_TRUE(processing.size() >= processingFilteredBySource.size());
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