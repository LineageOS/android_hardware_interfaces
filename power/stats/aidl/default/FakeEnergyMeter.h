/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <PowerStats.h>

#include <android-base/chrono_utils.h>

#include <chrono>
#include <random>

namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace stats {

class FakeEnergyMeter : public PowerStats::IEnergyMeter {
  public:
    FakeEnergyMeter(std::vector<std::pair<std::string, std::string>> channelNames) {
        int32_t channelId = 0;
        for (const auto& [name, subsystem] : channelNames) {
            Channel c;
            c.id = channelId++;
            c.name = name;
            c.subsystem = subsystem;

            EnergyMeasurement m;
            m.id = c.id;
            m.timestampMs = 0;
            m.durationMs = 0;
            m.energyUWs = 0;

            mChannels.push_back(c);
            mEnergyMeasurements.push_back(m);
        }
    }
    ~FakeEnergyMeter() = default;
    ndk::ScopedAStatus readEnergyMeter(const std::vector<int32_t>& in_channelIds,
                                       std::vector<EnergyMeasurement>* _aidl_return) override {
        for (auto& measurement : mEnergyMeasurements) {
            mFakeEnergyMeasurement.update(&measurement);
        }

        if (in_channelIds.empty()) {
            *_aidl_return = mEnergyMeasurements;
        } else {
            for (int32_t id : in_channelIds) {
                // check for invalid ids
                if (id < 0 || id >= mEnergyMeasurements.size()) {
                    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
                }

                _aidl_return->push_back(mEnergyMeasurements[id]);
            }
        }

        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus getEnergyMeterInfo(std::vector<Channel>* _aidl_return) override {
        *_aidl_return = mChannels;
        return ndk::ScopedAStatus::ok();
    }

  private:
    class FakeEnergyMeasurement {
      public:
        FakeEnergyMeasurement() : mDistribution(1, 100) {}
        void update(EnergyMeasurement* measurement) {
            // generates number in the range 1..100
            auto randNum = std::bind(mDistribution, mGenerator);

            // Get current time since boot in milliseconds
            uint64_t now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                                   ::android::base::boot_clock::now())
                                   .time_since_epoch()
                                   .count();
            measurement->timestampMs = now;
            measurement->durationMs = now;
            measurement->energyUWs += randNum() * 100;
        }

      private:
        std::default_random_engine mGenerator;
        std::uniform_int_distribution<int> mDistribution;
    };

    std::vector<Channel> mChannels;
    FakeEnergyMeasurement mFakeEnergyMeasurement;
    std::vector<EnergyMeasurement> mEnergyMeasurements;
};

}  // namespace stats
}  // namespace power
}  // namespace hardware
}  // namespace android
}  // namespace aidl