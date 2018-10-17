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

#include "SensorsHidlEnvironmentV2_0.h"

#include <log/log.h>

#include <vector>

using ::android::hardware::hidl_vec;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SensorInfo;
using ::android::hardware::sensors::V2_0::ISensors;

bool SensorsHidlEnvironmentV2_0::resetHal() {
    std::string step;
    bool succeed = false;
    do {
        step = "getService()";
        sensors = ISensors::getService(
            SensorsHidlEnvironmentV2_0::Instance()->getServiceName<ISensors>());
        if (sensors == nullptr) {
            break;
        }

        step = "getSensorList";
        std::vector<SensorInfo> sensorList;
        if (!sensors
                 ->getSensorsList([&](const hidl_vec<SensorInfo>& list) {
                     sensorList.reserve(list.size());
                     for (size_t i = 0; i < list.size(); ++i) {
                         sensorList.push_back(list[i]);
                     }
                 })
                 .isOk()) {
            break;
        }

        // stop each sensor individually
        step = "stop each sensor";
        bool ok = true;
        for (const auto& i : sensorList) {
            if (!sensors->activate(i.sensorHandle, false).isOk()) {
                ok = false;
                break;
            }
        }
        if (!ok) {
            break;
        }

        // mark it done
        step = "done";
        succeed = true;
    } while (0);

    if (succeed) {
        return true;
    }

    sensors = nullptr;
    return false;
}

void SensorsHidlEnvironmentV2_0::startPollingThread() {
    stopThread = false;
    pollThread = std::thread(pollingThread, this, std::ref(stopThread));
    events.reserve(128);
}

void SensorsHidlEnvironmentV2_0::pollingThread(SensorsHidlEnvironmentV2_0* /*env*/,
                                               std::atomic_bool& stop) {
    ALOGD("polling thread start");

    while (!stop) {
        // TODO: implement reading event queue
        stop = true;
    }
    ALOGD("polling thread end");
}
