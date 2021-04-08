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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <aidl/android/hardware/power/stats/IPowerStats.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <algorithm>
#include <iterator>
#include <random>
#include <unordered_map>

using aidl::android::hardware::power::stats::Channel;
using aidl::android::hardware::power::stats::EnergyConsumer;
using aidl::android::hardware::power::stats::EnergyConsumerAttribution;
using aidl::android::hardware::power::stats::EnergyConsumerResult;
using aidl::android::hardware::power::stats::EnergyConsumerType;
using aidl::android::hardware::power::stats::EnergyMeasurement;
using aidl::android::hardware::power::stats::IPowerStats;
using aidl::android::hardware::power::stats::PowerEntity;
using aidl::android::hardware::power::stats::State;
using aidl::android::hardware::power::stats::StateResidency;
using aidl::android::hardware::power::stats::StateResidencyResult;

using ndk::SpAIBinder;

#define ASSERT_OK(a)                                     \
    do {                                                 \
        auto ret = a;                                    \
        ASSERT_TRUE(ret.isOk()) << ret.getDescription(); \
    } while (0)

class PowerStatsAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        powerstats = IPowerStats::fromBinder(
                SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(nullptr, powerstats.get());
    }

    template <typename T>
    std::vector<T> getRandomSubset(std::vector<T> const& collection);

    void testNameValid(const std::string& name);

    template <typename T, typename S>
    void testUnique(std::vector<T> const& collection, S T::*field);

    template <typename T, typename S, typename R>
    void testMatching(std::vector<T> const& c1, R T::*f1, std::vector<S> const& c2, R S::*f2);

    std::shared_ptr<IPowerStats> powerstats;
};

// Returns a random subset from a collection
template <typename T>
std::vector<T> PowerStatsAidl::getRandomSubset(std::vector<T> const& collection) {
    if (collection.empty()) {
        return {};
    }

    std::vector<T> selected;
    std::sample(collection.begin(), collection.end(), std::back_inserter(selected),
                rand() % collection.size() + 1, std::mt19937{std::random_device{}()});

    return selected;
}

// Tests whether a name is valid
void PowerStatsAidl::testNameValid(const std::string& name) {
    EXPECT_NE(name, "");
}

// Tests whether the fields in a given collection are unique
template <typename T, typename S>
void PowerStatsAidl::testUnique(std::vector<T> const& collection, S T::*field) {
    std::set<S> cSet;
    for (auto const& elem : collection) {
        EXPECT_TRUE(cSet.insert(elem.*field).second);
    }
}

template <typename T, typename S, typename R>
void PowerStatsAidl::testMatching(std::vector<T> const& c1, R T::*f1, std::vector<S> const& c2,
                                  R S::*f2) {
    std::set<R> c1fields, c2fields;
    for (auto elem : c1) {
        c1fields.insert(elem.*f1);
    }

    for (auto elem : c2) {
        c2fields.insert(elem.*f2);
    }

    EXPECT_EQ(c1fields, c2fields);
}

// Each PowerEntity must have a valid name
TEST_P(PowerStatsAidl, ValidatePowerEntityNames) {
    std::vector<PowerEntity> infos;
    ASSERT_OK(powerstats->getPowerEntityInfo(&infos));

    for (auto info : infos) {
        testNameValid(info.name);
    }
}

// Each power entity must have a unique name
TEST_P(PowerStatsAidl, ValidatePowerEntityUniqueNames) {
    std::vector<PowerEntity> entities;
    ASSERT_OK(powerstats->getPowerEntityInfo(&entities));

    testUnique(entities, &PowerEntity::name);
}

// Each PowerEntity must have a unique ID
TEST_P(PowerStatsAidl, ValidatePowerEntityIds) {
    std::vector<PowerEntity> entities;
    ASSERT_OK(powerstats->getPowerEntityInfo(&entities));

    testUnique(entities, &PowerEntity::id);
}

// Each power entity must have at least one state
TEST_P(PowerStatsAidl, ValidateStateSize) {
    std::vector<PowerEntity> entities;
    ASSERT_OK(powerstats->getPowerEntityInfo(&entities));

    for (auto entity : entities) {
        EXPECT_GT(entity.states.size(), 0);
    }
}

// Each state must have a valid name
TEST_P(PowerStatsAidl, ValidateStateNames) {
    std::vector<PowerEntity> entities;
    ASSERT_OK(powerstats->getPowerEntityInfo(&entities));

    for (auto entity : entities) {
        for (auto state : entity.states) {
            testNameValid(state.name);
        }
    }
}

// Each state must have a name that is unique to the given PowerEntity
TEST_P(PowerStatsAidl, ValidateStateUniqueNames) {
    std::vector<PowerEntity> entities;
    ASSERT_OK(powerstats->getPowerEntityInfo(&entities));

    for (auto entity : entities) {
        testUnique(entity.states, &State::name);
    }
}

// Each state must have an ID that is unique to the given PowerEntity
TEST_P(PowerStatsAidl, ValidateStateUniqueIds) {
    std::vector<PowerEntity> entities;
    ASSERT_OK(powerstats->getPowerEntityInfo(&entities));

    for (auto entity : entities) {
        testUnique(entity.states, &State::id);
    }
}

// State residency must return a valid status
TEST_P(PowerStatsAidl, TestGetStateResidency) {
    std::vector<StateResidencyResult> results;
    ASSERT_OK(powerstats->getStateResidency({}, &results));
}

// State residency must return all results
TEST_P(PowerStatsAidl, TestGetStateResidencyAllResults) {
    std::vector<PowerEntity> entities;
    ASSERT_OK(powerstats->getPowerEntityInfo(&entities));

    std::vector<StateResidencyResult> results;
    ASSERT_OK(powerstats->getStateResidency({}, &results));

    testMatching(entities, &PowerEntity::id, results, &StateResidencyResult::id);
}

// Each result must contain all state residencies
TEST_P(PowerStatsAidl, TestGetStateResidencyAllStateResidencies) {
    std::vector<PowerEntity> entities;
    ASSERT_OK(powerstats->getPowerEntityInfo(&entities));

    std::vector<StateResidencyResult> results;
    ASSERT_OK(powerstats->getStateResidency({}, &results));

    for (auto entity : entities) {
        auto it = std::find_if(results.begin(), results.end(),
                               [&entity](const auto& x) { return x.id == entity.id; });
        ASSERT_NE(it, results.end());

        testMatching(entity.states, &State::id, it->stateResidencyData, &StateResidency::id);
    }
}

// State residency must return results for each requested power entity
TEST_P(PowerStatsAidl, TestGetStateResidencySelectedResults) {
    std::vector<PowerEntity> entities;
    ASSERT_OK(powerstats->getPowerEntityInfo(&entities));
    if (entities.empty()) {
        return;
    }

    std::vector<PowerEntity> selectedEntities = getRandomSubset(entities);
    std::vector<int32_t> selectedIds;
    for (auto const& entity : selectedEntities) {
        selectedIds.push_back(entity.id);
    }

    std::vector<StateResidencyResult> selectedResults;
    ASSERT_OK(powerstats->getStateResidency(selectedIds, &selectedResults));

    testMatching(selectedEntities, &PowerEntity::id, selectedResults, &StateResidencyResult::id);
}

// Energy meter info must return a valid status
TEST_P(PowerStatsAidl, TestGetEnergyMeterInfo) {
    std::vector<Channel> info;
    ASSERT_OK(powerstats->getEnergyMeterInfo(&info));
}

// Each channel must have a valid name
TEST_P(PowerStatsAidl, ValidateChannelNames) {
    std::vector<Channel> channels;
    ASSERT_OK(powerstats->getEnergyMeterInfo(&channels));

    for (auto channel : channels) {
        testNameValid(channel.name);
    }
}

// Each channel must have a valid subsystem
TEST_P(PowerStatsAidl, ValidateSubsystemNames) {
    std::vector<Channel> channels;
    ASSERT_OK(powerstats->getEnergyMeterInfo(&channels));

    for (auto channel : channels) {
        testNameValid(channel.subsystem);
    }
}

// Each channel must have a unique name
TEST_P(PowerStatsAidl, ValidateChannelUniqueNames) {
    std::vector<Channel> channels;
    ASSERT_OK(powerstats->getEnergyMeterInfo(&channels));

    testUnique(channels, &Channel::name);
}

// Each channel must have a unique ID
TEST_P(PowerStatsAidl, ValidateChannelUniqueIds) {
    std::vector<Channel> channels;
    ASSERT_OK(powerstats->getEnergyMeterInfo(&channels));

    testUnique(channels, &Channel::id);
}

// Reading energy meter must return a valid status
TEST_P(PowerStatsAidl, TestReadEnergyMeter) {
    std::vector<EnergyMeasurement> data;
    ASSERT_OK(powerstats->readEnergyMeter({}, &data));
}

// Reading energy meter must return results for all available channels
TEST_P(PowerStatsAidl, TestGetAllEnergyMeasurements) {
    std::vector<Channel> channels;
    ASSERT_OK(powerstats->getEnergyMeterInfo(&channels));

    std::vector<EnergyMeasurement> measurements;
    ASSERT_OK(powerstats->readEnergyMeter({}, &measurements));

    testMatching(channels, &Channel::id, measurements, &EnergyMeasurement::id);
}

// Reading energy must must return results for each selected channel
TEST_P(PowerStatsAidl, TestGetSelectedEnergyMeasurements) {
    std::vector<Channel> channels;
    ASSERT_OK(powerstats->getEnergyMeterInfo(&channels));
    if (channels.empty()) {
        return;
    }

    std::vector<Channel> selectedChannels = getRandomSubset(channels);
    std::vector<int32_t> selectedIds;
    for (auto const& channel : selectedChannels) {
        selectedIds.push_back(channel.id);
    }

    std::vector<EnergyMeasurement> selectedMeasurements;
    ASSERT_OK(powerstats->readEnergyMeter(selectedIds, &selectedMeasurements));

    testMatching(selectedChannels, &Channel::id, selectedMeasurements, &EnergyMeasurement::id);
}

// Energy consumer info must return a valid status
TEST_P(PowerStatsAidl, TestGetEnergyConsumerInfo) {
    std::vector<EnergyConsumer> consumers;
    ASSERT_OK(powerstats->getEnergyConsumerInfo(&consumers));
}

// Each energy consumer must have a unique id
TEST_P(PowerStatsAidl, TestGetEnergyConsumerUniqueId) {
    std::vector<EnergyConsumer> consumers;
    ASSERT_OK(powerstats->getEnergyConsumerInfo(&consumers));

    testUnique(consumers, &EnergyConsumer::id);
}

// Each energy consumer must have a valid name
TEST_P(PowerStatsAidl, ValidateEnergyConsumerNames) {
    std::vector<EnergyConsumer> consumers;
    ASSERT_OK(powerstats->getEnergyConsumerInfo(&consumers));

    for (auto consumer : consumers) {
        testNameValid(consumer.name);
    }
}

// Each energy consumer must have a unique name
TEST_P(PowerStatsAidl, ValidateEnergyConsumerUniqueNames) {
    std::vector<EnergyConsumer> consumers;
    ASSERT_OK(powerstats->getEnergyConsumerInfo(&consumers));

    testUnique(consumers, &EnergyConsumer::name);
}

// Energy consumers of the same type must have ordinals that are 0,1,2,..., N - 1
TEST_P(PowerStatsAidl, ValidateEnergyConsumerOrdinals) {
    std::vector<EnergyConsumer> consumers;
    ASSERT_OK(powerstats->getEnergyConsumerInfo(&consumers));

    std::unordered_map<EnergyConsumerType, std::set<int32_t>> ordinalMap;

    // Ordinals must be unique for each type
    for (auto consumer : consumers) {
        EXPECT_TRUE(ordinalMap[consumer.type].insert(consumer.ordinal).second);
    }

    // Min ordinal must be 0, max ordinal must be N - 1
    for (const auto& [unused, ordinals] : ordinalMap) {
        EXPECT_EQ(0, *std::min_element(ordinals.begin(), ordinals.end()));
        EXPECT_EQ(ordinals.size() - 1, *std::max_element(ordinals.begin(), ordinals.end()));
    }
}

// Energy consumed must return a valid status
TEST_P(PowerStatsAidl, TestGetEnergyConsumed) {
    std::vector<EnergyConsumerResult> results;
    ASSERT_OK(powerstats->getEnergyConsumed({}, &results));
}

// Energy consumed must return data for all energy consumers
TEST_P(PowerStatsAidl, TestGetAllEnergyConsumed) {
    std::vector<EnergyConsumer> consumers;
    ASSERT_OK(powerstats->getEnergyConsumerInfo(&consumers));

    std::vector<EnergyConsumerResult> results;
    ASSERT_OK(powerstats->getEnergyConsumed({}, &results));

    testMatching(consumers, &EnergyConsumer::id, results, &EnergyConsumerResult::id);
}

// Energy consumed must return data for each selected energy consumer
TEST_P(PowerStatsAidl, TestGetSelectedEnergyConsumed) {
    std::vector<EnergyConsumer> consumers;
    ASSERT_OK(powerstats->getEnergyConsumerInfo(&consumers));
    if (consumers.empty()) {
        return;
    }

    std::vector<EnergyConsumer> selectedConsumers = getRandomSubset(consumers);
    std::vector<int32_t> selectedIds;
    for (auto const& consumer : selectedConsumers) {
        selectedIds.push_back(consumer.id);
    }

    std::vector<EnergyConsumerResult> selectedResults;
    ASSERT_OK(powerstats->getEnergyConsumed(selectedIds, &selectedResults));

    testMatching(selectedConsumers, &EnergyConsumer::id, selectedResults,
                 &EnergyConsumerResult::id);
}

// Energy consumed attribution uids must be unique for a given energy consumer
TEST_P(PowerStatsAidl, ValidateEnergyConsumerAttributionUniqueUids) {
    std::vector<EnergyConsumerResult> results;
    ASSERT_OK(powerstats->getEnergyConsumed({}, &results));

    for (auto result : results) {
        testUnique(result.attribution, &EnergyConsumerAttribution::uid);
    }
}

// Energy consumed total energy >= sum total of uid-attributed energy
TEST_P(PowerStatsAidl, TestGetEnergyConsumedAttributedEnergy) {
    std::vector<EnergyConsumerResult> results;
    ASSERT_OK(powerstats->getEnergyConsumed({}, &results));

    for (auto result : results) {
        int64_t totalAttributedEnergyUWs = 0;
        for (auto attribution : result.attribution) {
            totalAttributedEnergyUWs += attribution.energyUWs;
        }
        EXPECT_TRUE(result.energyUWs >= totalAttributedEnergyUWs);
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(PowerStatsAidl);
INSTANTIATE_TEST_SUITE_P(
        PowerStats, PowerStatsAidl,
        testing::ValuesIn(android::getAidlHalInstanceNames(IPowerStats::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
