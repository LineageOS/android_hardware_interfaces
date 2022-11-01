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

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <android/binder_auto_utils.h>

#include "TestUtils.h"
#include "effect-impl/EffectUUID.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::EffectNullUuid;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Processing;
using aidl::android::media::audio::common::AudioUuid;

class EffectFactoryHelper {
  public:
    explicit EffectFactoryHelper(const std::string& name) : mServiceName(name) {}

    void ConnectToFactoryService() {
        mEffectFactory = IFactory::fromBinder(binderUtil.connectToService(mServiceName));
        ASSERT_NE(mEffectFactory, nullptr);
    }

    void RestartFactoryService() {
        ASSERT_NE(mEffectFactory, nullptr);
        mEffectFactory = IFactory::fromBinder(binderUtil.restartService());
        ASSERT_NE(mEffectFactory, nullptr);
        ClearEffectMap();
    }

    void QueryEffects(const std::optional<AudioUuid>& in_type,
                      const std::optional<AudioUuid>& in_instance,
                      const std::optional<AudioUuid>& in_proxy,
                      std::vector<Descriptor::Identity>* _aidl_return) {
        ASSERT_NE(mEffectFactory, nullptr);
        EXPECT_IS_OK(mEffectFactory->queryEffects(in_type, in_instance, in_proxy, _aidl_return));
        mIds = *_aidl_return;
    }

    void QueryProcessing(const std::optional<Processing::Type>& in_type,
                         std::vector<Processing>* _aidl_return) {
        ASSERT_NE(mEffectFactory, nullptr);
        EXPECT_IS_OK(mEffectFactory->queryProcessing(in_type, _aidl_return));
        // only update the whole list if no filter applied
        if (!in_type.has_value()) {
            mProcesses = *_aidl_return;
        }
    }

    void CreateEffects() {
        for (const auto& id : mIds) {
            std::shared_ptr<IEffect> effect;
            EXPECT_IS_OK(mEffectFactory->createEffect(id.uuid, &effect));
            EXPECT_NE(effect, nullptr) << id.toString();
            if (effect) {
                mEffectIdMap[effect] = id;
            }
        }
    }

    void QueryAndCreateEffects(const AudioUuid& type = EffectNullUuid) {
        std::vector<Descriptor::Identity> ids;
        ASSERT_NE(mEffectFactory, nullptr);

        if (type == EffectNullUuid) {
            EXPECT_IS_OK(
                    mEffectFactory->queryEffects(std::nullopt, std::nullopt, std::nullopt, &ids));
        } else {
            EXPECT_IS_OK(mEffectFactory->queryEffects(type, std::nullopt, std::nullopt, &ids));
        }
        for (const auto& id : ids) {
            ASSERT_EQ(id.type, type);
            std::shared_ptr<IEffect> effect;
            EXPECT_IS_OK(mEffectFactory->createEffect(id.uuid, &effect));
            EXPECT_NE(effect, nullptr) << id.toString();
            if (effect) {
                mEffectIdMap[effect] = id;
            }
        }
    }

    void CreateEffectsAndExpect(
            const std::vector<std::pair<Descriptor::Identity, binder_exception_t>>& uuid_status) {
        ASSERT_NE(mEffectFactory, nullptr);
        for (const auto& it : uuid_status) {
            std::shared_ptr<IEffect> effect;
            auto status = mEffectFactory->createEffect(it.first.uuid, &effect);
            EXPECT_STATUS(it.second, status);
            if (effect) {
                mEffectIdMap[effect] = it.first;
            }
        }
    }

    void DestroyEffectAndExpect(std::shared_ptr<IEffect>& instance, binder_exception_t exception) {
        ASSERT_NE(mEffectFactory, nullptr);
        auto status = mEffectFactory->destroyEffect(instance);
        EXPECT_STATUS(exception, status);
    }

    void QueryAndCreateAllEffects() {
        ASSERT_NE(mEffectFactory, nullptr);
        EXPECT_IS_OK(mEffectFactory->queryEffects(std::nullopt, std::nullopt, std::nullopt,
                                                  &mCompleteIds));
        for (const auto& id : mCompleteIds) {
            std::shared_ptr<IEffect> effect;
            EXPECT_IS_OK(mEffectFactory->createEffect(id.uuid, &effect));
            EXPECT_NE(effect, nullptr) << id.toString();
            mEffectIdMap[effect] = id;
        }
    }

    void DestroyEffects(const binder_exception_t expected = EX_NONE, const int remaining = 0) {
        ASSERT_NE(mEffectFactory, nullptr);

        for (auto it = mEffectIdMap.begin(); it != mEffectIdMap.end();) {
            auto erased = it++;
            auto status = mEffectFactory->destroyEffect(erased->first);
            EXPECT_STATUS(expected, status);
            if (status.isOk()) {
                mEffectIdMap.erase(erased);
            }
        }
        EXPECT_EQ((unsigned int)remaining, mEffectIdMap.size());
    }

    std::shared_ptr<IFactory> GetFactory() { return mEffectFactory; }
    const std::vector<Descriptor::Identity>& GetEffectIds() { return mIds; }
    const std::vector<Descriptor::Identity>& GetCompleteEffectIdList() const {
        return mCompleteIds;
    }
    const std::map<std::shared_ptr<IEffect>, Descriptor::Identity>& GetEffectMap() const {
        return mEffectIdMap;
    }
    void ClearEffectMap() { mEffectIdMap.clear(); }

  private:
    std::shared_ptr<IFactory> mEffectFactory;
    std::string mServiceName;
    AudioHalBinderServiceUtil binderUtil;
    std::vector<Descriptor::Identity> mIds;
    std::vector<Descriptor::Identity> mCompleteIds;
    std::vector<Processing> mProcesses;

    std::map<std::shared_ptr<IEffect>, Descriptor::Identity> mEffectIdMap;
};
