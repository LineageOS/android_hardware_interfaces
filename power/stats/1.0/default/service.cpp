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

#define LOG_TAG "android.hardware.power.stats@1.0-service-mock"

#include <android/log.h>
#include <hidl/HidlTransportSupport.h>

#include "PowerStats.h"

using android::OK;
using android::sp;
using android::status_t;

// libhwbinder:
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

// Generated HIDL files
using android::hardware::power::stats::V1_0::IPowerStats;
using android::hardware::power::stats::V1_0::PowerEntityStateResidencyResult;
using android::hardware::power::stats::V1_0::PowerEntityStateSpace;
using android::hardware::power::stats::V1_0::PowerEntityType;
using android::hardware::power::stats::V1_0::implementation::IStateResidencyDataProvider;
using android::hardware::power::stats::V1_0::implementation::PowerStats;

class DefaultStateResidencyDataProvider : public IStateResidencyDataProvider {
   public:
    DefaultStateResidencyDataProvider(uint32_t id)
        : mPowerEntityId(id), mActiveStateId(0), mSleepStateId(1) {}
    ~DefaultStateResidencyDataProvider() = default;

    bool getResults(std::unordered_map<uint32_t, PowerEntityStateResidencyResult>& results) {
        PowerEntityStateResidencyResult result = { .powerEntityId = mPowerEntityId };
        result.stateResidencyData.resize(2);

        // Using fake numbers here for display only. A real implementation would
        // use actual tracked stats.
        result.stateResidencyData[0] = {
            .powerEntityStateId = mActiveStateId,
            .totalTimeInStateMs = 1,
            .totalStateEntryCount = 2,
            .lastEntryTimestampMs = 3
        };
        result.stateResidencyData[1] = {
            .powerEntityStateId = mSleepStateId,
            .totalTimeInStateMs = 4,
            .totalStateEntryCount = 5,
            .lastEntryTimestampMs = 6,
        };
        results.emplace(mPowerEntityId, result);
        return true;
    }

    std::vector<PowerEntityStateSpace> getStateSpaces() {
        return {{
          .powerEntityId = mPowerEntityId,
          .states = {
              {.powerEntityStateId = mActiveStateId, .powerEntityStateName = "Active"},
              {.powerEntityStateId = mSleepStateId, .powerEntityStateName = "Sleep"}
          }
        }};
    }

   private:
    const uint32_t mPowerEntityId;
    const uint32_t mActiveStateId;
    const uint32_t mSleepStateId;
};

int main(int /* argc */, char** /* argv */) {
    ALOGI("power.stats service 1.0 mock is starting.");

    PowerStats* service = new PowerStats();
    if (service == nullptr) {
        ALOGE("Can not create an instance of power.stats HAL Iface, exiting.");
        return 1;
    }

    uint32_t defaultId = service->addPowerEntity("DefaultEntity", PowerEntityType::SUBSYSTEM);
    auto defaultSdp = std::make_shared<DefaultStateResidencyDataProvider>(defaultId);
    service->addStateResidencyDataProvider(std::move(defaultSdp));

    configureRpcThreadpool(1, true /*callerWillJoin*/);

    status_t status = service->registerAsService();
    if (status != OK) {
        ALOGE("Could not register service for power.stats HAL Iface (%d), exiting.", status);
        return 1;
    }

    ALOGI("power.stats service is ready");
    joinRpcThreadpool();

    // In normal operation, we don't expect the thread pool to exit
    ALOGE("power.stats service is shutting down");
    return 1;
}
