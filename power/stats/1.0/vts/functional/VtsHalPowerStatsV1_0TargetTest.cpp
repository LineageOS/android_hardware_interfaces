/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "android.power.stats.vts"

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/power/stats/1.0/IPowerStats.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <inttypes.h>
#include <algorithm>
#include <random>
#include <thread>

namespace android {
namespace power {
namespace stats {
namespace vts {
namespace {

using android::sp;
using android::hardware::hidl_vec;
using android::hardware::kSynchronizedReadWrite;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::power::stats::V1_0::EnergyData;
using android::hardware::power::stats::V1_0::IPowerStats;
using android::hardware::power::stats::V1_0::RailInfo;
using android::hardware::power::stats::V1_0::Status;

}  // namespace

typedef hardware::MessageQueue<EnergyData, kSynchronizedReadWrite> MessageQueueSync;
// Test environment for Power HIDL HAL.
class PowerStatsHidlEnv : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static PowerStatsHidlEnv* Instance() {
        static PowerStatsHidlEnv* instance = new PowerStatsHidlEnv;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IPowerStats>(); }
};

class PowerStatsHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        service_ = ::testing::VtsHalHidlTargetTestBase::getService<IPowerStats>(
            PowerStatsHidlEnv::Instance()->getServiceName<IPowerStats>());
        ASSERT_NE(service_, nullptr);
    }

    virtual void TearDown() override {}

    sp<IPowerStats> service_;
};

TEST_F(PowerStatsHidlTest, ValidateRailInfo) {
    hidl_vec<RailInfo> rails[2];
    Status s;
    auto cb = [&rails, &s](hidl_vec<RailInfo> rail_subsys, Status status) {
        rails[0] = rail_subsys;
        s = status;
    };
    Return<void> ret = service_->getRailInfo(cb);
    EXPECT_TRUE(ret.isOk());
    if (s == Status::SUCCESS) {
        /* Rails size should be non-zero on SUCCESS*/
        ASSERT_NE(rails[0].size(), 0);
        /* check if indices returned are unique*/
        set<uint32_t> ids;
        for (auto rail : rails[0]) {
            ASSERT_TRUE(ids.insert(rail.index).second);
        }
        auto cb = [&rails, &s](hidl_vec<RailInfo> rail_subsys, Status status) {
            rails[1] = rail_subsys;
            s = status;
        };
        Return<void> ret = service_->getRailInfo(cb);
        EXPECT_TRUE(ret.isOk());
        ASSERT_EQ(s, Status::SUCCESS);
        ASSERT_EQ(rails[0].size(), rails[1].size());
        /* check if data returned by two calls to getRailInfo is same*/
        for (int i = 0; i < rails[0].size(); i++) {
            ASSERT_NE(rails[0][i].railName, "");
            ASSERT_NE(rails[0][i].subsysName, "");
            int j = 0;
            bool match = false;
            for (j = 0; j < rails[1].size(); j++) {
                if (rails[0][i].index == rails[1][j].index) {
                    ASSERT_EQ(rails[0][i].railName, rails[1][i].railName);
                    ASSERT_EQ(rails[0][i].subsysName, rails[1][i].subsysName);
                    match = true;
                    break;
                }
            }
            ASSERT_TRUE(match);
        }
    } else if (s == Status::FILESYSTEM_ERROR) {
        ALOGI("ValidateRailInfo returned FILESYSTEM_ERROR");
        ASSERT_EQ(rails[0].size(), 0);
    } else if (s == Status::NOT_SUPPORTED) {
        ALOGI("ValidateRailInfo returned NOT_SUPPORTED");
        ASSERT_EQ(rails[0].size(), 0);
    } else if (s == Status::INVALID_INPUT) {
        ALOGI("ValidateRailInfo returned INVALID_INPUT");
        ASSERT_EQ(rails[0].size(), 0);
    } else if (s == Status::INSUFFICIENT_RESOURCES) {
        ALOGI("ValidateRailInfo returned INSUFFICIENT_RESOURCES");
        ASSERT_EQ(rails[0].size(), 0);
    }
}

TEST_F(PowerStatsHidlTest, ValidateAllPowerData) {
    hidl_vec<EnergyData> measurements[2];
    Status s;
    auto cb = [&measurements, &s](hidl_vec<EnergyData> measure, Status status) {
        measurements[0] = measure;
        s = status;
    };
    Return<void> ret = service_->getEnergyData(hidl_vec<uint32_t>(), cb);
    EXPECT_TRUE(ret.isOk());
    if (s == Status::SUCCESS) {
        /*measurements size should be non-zero on SUCCESS*/
        ASSERT_NE(measurements[0].size(), 0);
        auto cb = [&measurements, &s](hidl_vec<EnergyData> measure, Status status) {
            measurements[1] = measure;
            s = status;
        };
        Return<void> ret = service_->getEnergyData(hidl_vec<uint32_t>(), cb);
        EXPECT_TRUE(ret.isOk());
        ASSERT_EQ(s, Status::SUCCESS);
        /*Both calls should returns same amount of data*/
        ASSERT_EQ(measurements[0].size(), measurements[1].size());
        /*Check is energy and timestamp are monotonically increasing*/
        for (int i = 0; i < measurements[0].size(); i++) {
            int j;
            for (j = 0; j < measurements[1].size(); j++) {
                if (measurements[0][i].index == measurements[1][j].index) {
                    EXPECT_GE(measurements[1][j].timestamp, measurements[0][i].timestamp);
                    EXPECT_GE(measurements[1][j].energy, measurements[0][i].energy);
                    break;
                }
            }
            /*Check is indices for two call match*/
            ASSERT_NE(j, measurements[1].size());
        }
    } else if (s == Status::FILESYSTEM_ERROR) {
        ALOGI("ValidateAllPowerData returned FILESYSTEM_ERROR");
        ASSERT_EQ(measurements[0].size(), 0);
    } else if (s == Status::NOT_SUPPORTED) {
        ALOGI("ValidateAllPowerData returned NOT_SUPPORTED");
        ASSERT_EQ(measurements[0].size(), 0);
    } else if (s == Status::INVALID_INPUT) {
        ALOGI("ValidateAllPowerData returned INVALID_INPUT");
        ASSERT_EQ(measurements[0].size(), 0);
    } else if (s == Status::INSUFFICIENT_RESOURCES) {
        ALOGI("ValidateAllPowerData returned INSUFFICIENT_RESOURCES");
        ASSERT_EQ(measurements[0].size(), 0);
    }
}

TEST_F(PowerStatsHidlTest, ValidateFilteredPowerData) {
    hidl_vec<RailInfo> rails;
    hidl_vec<EnergyData> measurements;
    hidl_vec<uint32_t> indices;
    std::string debugString;
    Status s;
    auto cb = [&rails, &s](hidl_vec<RailInfo> rail_subsys, Status status) {
        rails = rail_subsys;
        s = status;
    };
    Return<void> ret = service_->getRailInfo(cb);
    EXPECT_TRUE(ret.isOk());
    std::time_t seed = std::time(nullptr);
    std::srand(seed);
    if (s == Status::SUCCESS) {
        size_t sz = std::max(1, (int)(std::rand() % rails.size()));
        indices.resize(sz);
        for (int i = 0; i < sz; i++) {
            int j = std::rand() % rails.size();
            indices[i] = rails[j].index;
            debugString += std::to_string(indices[i]) + ", ";
        }
        debugString += "\n";
        ALOGI("ValidateFilteredPowerData for indices: %s", debugString.c_str());
        auto cb = [&measurements, &s](hidl_vec<EnergyData> measure, Status status) {
            measurements = measure;
            s = status;
        };
        Return<void> ret = service_->getEnergyData(indices, cb);
        EXPECT_TRUE(ret.isOk());
        if (s == Status::SUCCESS) {
            /* Make sure that all the measurements are returned */
            ASSERT_EQ(sz, measurements.size());
            for (int i = 0; i < measurements.size(); i++) {
                int j;
                bool match = false;
                /* Check that the measurement belongs to the requested index */
                for (j = 0; j < indices.size(); j++) {
                    if (indices[j] == measurements[i].index) {
                        match = true;
                        break;
                    }
                }
                ASSERT_TRUE(match);
            }
        }
    } else {
        /* size should be zero is stats is NOT SUCCESS */
        ASSERT_EQ(rails.size(), 0);
    }
}

void readEnergy(sp<IPowerStats> service_, uint32_t timeMs) {
    std::unique_ptr<MessageQueueSync> mQueue;
    Status s;
    uint32_t railsInSample;
    uint32_t totalSamples;
    auto cb = [&s, &mQueue, &totalSamples, &railsInSample](
                  const hardware::MQDescriptorSync<EnergyData>& in, uint32_t numSamples,
                  uint32_t railsPerSample, Status status) {
        mQueue.reset(new (std::nothrow) MessageQueueSync(in));
        s = status;
        totalSamples = numSamples;
        railsInSample = railsPerSample;
    };
    service_->streamEnergyData(timeMs, 10, cb);
    if (s == Status::SUCCESS) {
        ASSERT_NE(nullptr, mQueue);
        ASSERT_TRUE(mQueue->isValid());
        bool rc;
        int sampleCount = 0;
        uint32_t totalQuants = railsInSample * totalSamples;
        uint64_t timeout_ns = 10000000000;
        if (totalSamples > 0) {
            uint32_t batch = std::max(1, (int)((std::rand() % totalSamples) * railsInSample));
            ALOGI("Read energy, timsMs: %u, batch: %u", timeMs, batch);
            std::vector<EnergyData> data(batch);
            while (sampleCount < totalQuants) {
                rc = mQueue->readBlocking(&data[0], batch, timeout_ns);
                if (rc == false) {
                    break;
                }
                sampleCount = sampleCount + batch;
                if (batch > totalQuants - sampleCount) {
                    batch = 1;
                }
            }
            ASSERT_EQ(totalQuants, sampleCount);
        }
    } else if (s == Status::FILESYSTEM_ERROR) {
        ASSERT_FALSE(mQueue->isValid());
        ASSERT_EQ(totalSamples, 0);
        ASSERT_EQ(railsInSample, 0);
    } else if (s == Status::NOT_SUPPORTED) {
        ASSERT_FALSE(mQueue->isValid());
        ASSERT_EQ(totalSamples, 0);
        ASSERT_EQ(railsInSample, 0);
    } else if (s == Status::INVALID_INPUT) {
        ASSERT_FALSE(mQueue->isValid());
        ASSERT_EQ(totalSamples, 0);
        ASSERT_EQ(railsInSample, 0);
    } else if (s == Status::INSUFFICIENT_RESOURCES) {
        ASSERT_FALSE(mQueue->isValid());
        ASSERT_EQ(totalSamples, 0);
        ASSERT_EQ(railsInSample, 0);
    }
}

TEST_F(PowerStatsHidlTest, StreamEnergyData) {
    std::time_t seed = std::time(nullptr);
    std::srand(seed);
    std::thread thread1 = std::thread(readEnergy, service_, std::rand() % 5000);
    thread1.join();
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(PowerStatsHidlEnv::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    PowerStatsHidlEnv::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}

}  // namespace vts
}  // namespace stats
}  // namespace power
}  // namespace android
