/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "PowerStats.h"

#include <android-base/logging.h>

#include <numeric>

namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace stats {

void PowerStats::addStateResidencyDataProvider(std::unique_ptr<IStateResidencyDataProvider> p) {
    if (!p) {
        return;
    }

    int32_t id = mPowerEntityInfos.size();
    auto info = p->getInfo();

    size_t index = mStateResidencyDataProviders.size();
    mStateResidencyDataProviders.emplace_back(std::move(p));

    for (const auto& [entityName, states] : info) {
        PowerEntity i = {
                .id = id++,
                .name = entityName,
                .states = states,
        };
        mPowerEntityInfos.emplace_back(i);
        mStateResidencyDataProviderIndex.emplace_back(index);
    }
}

void PowerStats::addEnergyConsumer(std::unique_ptr<IEnergyConsumer> p) {
    if (!p) {
        return;
    }

    EnergyConsumerType type = p->getType();
    std::string name = p->getName();
    int32_t count = count_if(mEnergyConsumerInfos.begin(), mEnergyConsumerInfos.end(),
                             [&type](const EnergyConsumer& c) { return type == c.type; });
    int32_t id = mEnergyConsumers.size();
    mEnergyConsumerInfos.emplace_back(
            EnergyConsumer{.id = id, .ordinal = count, .type = type, .name = name});
    mEnergyConsumers.emplace_back(std::move(p));
}

void PowerStats::setEnergyMeter(std::unique_ptr<IEnergyMeter> p) {
    mEnergyMeter = std::move(p);
}

ndk::ScopedAStatus PowerStats::getPowerEntityInfo(std::vector<PowerEntity>* _aidl_return) {
    *_aidl_return = mPowerEntityInfos;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerStats::getStateResidency(const std::vector<int32_t>& in_powerEntityIds,
                                                 std::vector<StateResidencyResult>* _aidl_return) {
    if (mPowerEntityInfos.empty()) {
        return ndk::ScopedAStatus::ok();
    }

    // If in_powerEntityIds is empty then return data for all supported entities
    if (in_powerEntityIds.empty()) {
        std::vector<int32_t> v(mPowerEntityInfos.size());
        std::iota(std::begin(v), std::end(v), 0);
        return getStateResidency(v, _aidl_return);
    }

    std::unordered_map<std::string, std::vector<StateResidency>> stateResidencies;

    for (const int32_t id : in_powerEntityIds) {
        // check for invalid ids
        if (id < 0 || id >= mPowerEntityInfos.size()) {
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
        }

        // Check to see if we already have data for the given id
        std::string powerEntityName = mPowerEntityInfos[id].name;
        if (stateResidencies.find(powerEntityName) == stateResidencies.end()) {
            mStateResidencyDataProviders.at(mStateResidencyDataProviderIndex.at(id))
                    ->getStateResidencies(&stateResidencies);
        }

        // Append results if we have them
        auto stateResidency = stateResidencies.find(powerEntityName);
        if (stateResidency != stateResidencies.end()) {
            StateResidencyResult res = {
                    .id = id,
                    .stateResidencyData = stateResidency->second,
            };
            _aidl_return->emplace_back(res);
        } else {
            // Failed to get results for the given id.
            LOG(ERROR) << "Failed to get results for " << powerEntityName;
        }
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerStats::getEnergyConsumerInfo(std::vector<EnergyConsumer>* _aidl_return) {
    *_aidl_return = mEnergyConsumerInfos;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerStats::getEnergyConsumed(const std::vector<int32_t>& in_energyConsumerIds,
                                                 std::vector<EnergyConsumerResult>* _aidl_return) {
    if (mEnergyConsumers.empty()) {
        return ndk::ScopedAStatus::ok();
    }

    // If in_powerEntityIds is empty then return data for all supported energy consumers
    if (in_energyConsumerIds.empty()) {
        std::vector<int32_t> v(mEnergyConsumerInfos.size());
        std::iota(std::begin(v), std::end(v), 0);
        return getEnergyConsumed(v, _aidl_return);
    }

    for (const auto id : in_energyConsumerIds) {
        // check for invalid ids
        if (id < 0 || id >= mEnergyConsumers.size()) {
            return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
        }

        auto optionalResult = mEnergyConsumers[id]->getEnergyConsumed();
        if (optionalResult) {
            EnergyConsumerResult result = optionalResult.value();
            result.id = id;
            _aidl_return->emplace_back(result);
        } else {
            // Failed to get results for the given id.
            LOG(ERROR) << "Failed to get results for " << mEnergyConsumerInfos[id].name;
        }
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PowerStats::getEnergyMeterInfo(std::vector<Channel>* _aidl_return) {
    if (!mEnergyMeter) {
        return ndk::ScopedAStatus::ok();
    }

    return mEnergyMeter->getEnergyMeterInfo(_aidl_return);
}

ndk::ScopedAStatus PowerStats::readEnergyMeter(const std::vector<int32_t>& in_channelIds,
                                               std::vector<EnergyMeasurement>* _aidl_return) {
    if (!mEnergyMeter) {
        return ndk::ScopedAStatus::ok();
    }

    return mEnergyMeter->readEnergyMeter(in_channelIds, _aidl_return);
}

}  // namespace stats
}  // namespace power
}  // namespace hardware
}  // namespace android
}  // namespace aidl
