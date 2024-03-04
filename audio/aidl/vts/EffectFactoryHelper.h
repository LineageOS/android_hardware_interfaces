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

#include <aidl/Vintf.h>
#include <android/binder_auto_utils.h>
#include <system/audio_effects/aidl_effects_utils.h>

#include "AudioHalBinderServiceUtil.h"
#include "TestUtils.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IFactory;
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
    }

    std::shared_ptr<IFactory> GetFactory() const { return mEffectFactory; }

    static std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> getAllEffectDescriptors(
            std::string serviceName, std::optional<AudioUuid> type = std::nullopt) {
        AudioHalBinderServiceUtil util;
        auto names = android::getAidlHalInstanceNames(serviceName);
        std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> result;

        for (const auto& name : names) {
            auto factory = IFactory::fromBinder(util.connectToService(name));
            if (factory) {
                if (std::vector<Descriptor> descs;
                    factory->queryEffects(std::nullopt, std::nullopt, std::nullopt, &descs)
                            .isOk()) {
                    for (const auto& desc : descs) {
                        if (type.has_value() && desc.common.id.type != type.value()) {
                            continue;
                        }
                        result.emplace_back(factory, desc);
                    }
                }
            }
        }
        return result;
    }

    static int getHalVersion(const std::shared_ptr<IFactory>& factory) {
        int version = 0;
        return (factory && factory->getInterfaceVersion(&version).isOk()) ? version : 0;
    }

    static bool isReopenSupported(const std::shared_ptr<IFactory>& factory) {
        return EffectFactoryHelper::getHalVersion(factory) >=
               aidl::android::hardware::audio::effect::kReopenSupportedVersion;
    }

  private:
    std::shared_ptr<IFactory> mEffectFactory;
    std::string mServiceName;
    AudioHalBinderServiceUtil binderUtil;
};
