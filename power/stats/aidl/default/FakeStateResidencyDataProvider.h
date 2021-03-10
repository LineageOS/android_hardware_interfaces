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

#include <random>

namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace stats {

class FakeStateResidencyDataProvider : public PowerStats::IStateResidencyDataProvider {
  public:
    FakeStateResidencyDataProvider(const std::string& name, std::vector<State> states)
        : mName(name), mStates(states) {
        for (const auto& state : mStates) {
            StateResidency r;
            r.id = state.id;
            r.totalTimeInStateMs = 0;
            r.totalStateEntryCount = 0;
            r.lastEntryTimestampMs = 0;
            mResidencies.push_back(r);
        }
    }
    ~FakeStateResidencyDataProvider() = default;

    // Methods from PowerStats::IStateResidencyDataProvider
    bool getStateResidencies(
            std::unordered_map<std::string, std::vector<StateResidency>>* residencies) override {
        for (auto& residency : mResidencies) {
            mFakeStateResidency.update(&residency);
        }

        residencies->emplace(mName, mResidencies);
        return true;
    }

    std::unordered_map<std::string, std::vector<State>> getInfo() override {
        return {{mName, mStates}};
    }

  private:
    class FakeStateResidency {
      public:
        FakeStateResidency() : mDistribution(1, 100) {}
        void update(StateResidency* residency) {
            // generates number in the range 1..100
            auto randNum = std::bind(mDistribution, mGenerator);

            residency->totalTimeInStateMs += randNum() * 100;
            residency->totalStateEntryCount += randNum();
            residency->lastEntryTimestampMs += randNum() * 100;
        }

      private:
        std::default_random_engine mGenerator;
        std::uniform_int_distribution<int> mDistribution;
    };

    const std::string mName;
    const std::vector<State> mStates;
    FakeStateResidency mFakeStateResidency;
    std::vector<StateResidency> mResidencies;
};

}  // namespace stats
}  // namespace power
}  // namespace hardware
}  // namespace android
}  // namespace aidl