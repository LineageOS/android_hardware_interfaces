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

using aidl::android::hardware::power::stats::Channel;
using aidl::android::hardware::power::stats::EnergyMeasurement;
using aidl::android::hardware::power::stats::IPowerStats;
using aidl::android::hardware::power::stats::PowerEntity;
using aidl::android::hardware::power::stats::StateResidencyResult;

using ndk::SpAIBinder;

class PowerStatsAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        powerstats = IPowerStats::fromBinder(
                SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(nullptr, powerstats.get());
    }

    std::shared_ptr<IPowerStats> powerstats;
};

TEST_P(PowerStatsAidl, TestReadEnergyMeter) {
    std::vector<EnergyMeasurement> data;
    ASSERT_TRUE(powerstats->readEnergyMeters({}, &data).isOk());
}

// Each PowerEntity must have a valid name
TEST_P(PowerStatsAidl, ValidatePowerEntityNames) {
    std::vector<PowerEntity> infos;
    ASSERT_TRUE(powerstats->getPowerEntityInfo(&infos).isOk());

    for (auto info : infos) {
        EXPECT_NE(info.name, "");
    }
}

// Each power entity must have a unique name
TEST_P(PowerStatsAidl, ValidatePowerEntityUniqueNames) {
    std::vector<PowerEntity> infos;
    ASSERT_TRUE(powerstats->getPowerEntityInfo(&infos).isOk());

    std::set<std::string> names;
    for (auto info : infos) {
        EXPECT_TRUE(names.insert(info.name).second);
    }
}

// Each PowerEntity must have a unique ID
TEST_P(PowerStatsAidl, ValidatePowerEntityIds) {
    std::vector<PowerEntity> infos;
    ASSERT_TRUE(powerstats->getPowerEntityInfo(&infos).isOk());

    std::set<int32_t> ids;
    for (auto info : infos) {
        EXPECT_TRUE(ids.insert(info.id).second);
    }
}

// Each state must have a valid name
TEST_P(PowerStatsAidl, ValidateStateNames) {
    std::vector<PowerEntity> infos;
    ASSERT_TRUE(powerstats->getPowerEntityInfo(&infos).isOk());

    for (auto info : infos) {
        for (auto state : info.states) {
            EXPECT_NE(state.name, "");
        }
    }
}

// Each state must have a name that is unique to the given PowerEntity
TEST_P(PowerStatsAidl, ValidateStateUniqueNames) {
    std::vector<PowerEntity> infos;
    ASSERT_TRUE(powerstats->getPowerEntityInfo(&infos).isOk());

    for (auto info : infos) {
        std::set<std::string> stateNames;
        for (auto state : info.states) {
            EXPECT_TRUE(stateNames.insert(state.name).second);
        }
    }
}

// Each state must have an ID that is unique to the given PowerEntity
TEST_P(PowerStatsAidl, ValidateStateUniqueIds) {
    std::vector<PowerEntity> infos;
    ASSERT_TRUE(powerstats->getPowerEntityInfo(&infos).isOk());

    for (auto info : infos) {
        std::set<int32_t> stateIds;
        for (auto state : info.states) {
            EXPECT_TRUE(stateIds.insert(state.id).second);
        }
    }
}

TEST_P(PowerStatsAidl, TestGetStateResidency) {
    std::vector<StateResidencyResult> results;
    ASSERT_TRUE(powerstats->getStateResidency({}, &results).isOk());
}

TEST_P(PowerStatsAidl, TestGetEnergyMeterInfo) {
    std::vector<Channel> info;
    ASSERT_TRUE(powerstats->getEnergyMeterInfo(&info).isOk());
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
