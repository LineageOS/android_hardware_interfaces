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

#include "SensorsHidlEnvironmentV1_0.h"

#include <log/log.h>

using ::android::hardware::hidl_vec;
using ::android::hardware::sensors::V1_0::ISensors;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SensorInfo;

void SensorsHidlEnvironment::HidlSetUp() {
    resetHal();

    ASSERT_NE(sensors, nullptr) << "sensors is nullptr, cannot get hidl service";

    collectionEnabled = false;
    startPollingThread();

    // In case framework just stopped for test and there is sensor events in the pipe,
    // wait some time for those events to be cleared to avoid them messing up the test.
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

void SensorsHidlEnvironment::HidlTearDown() {
    stopThread = true;
    pollThread.detach();
}

void SensorsHidlEnvironment::resetHal() {
    // wait upto 100ms * 10 = 1s for hidl service.
    constexpr auto RETRY_DELAY = std::chrono::milliseconds(100);

    std::string step;
    bool succeed = false;
    for (size_t retry = 10; retry > 0; --retry) {
        // this do ... while is for easy error handling
        do {
            step = "getService()";
            sensors = ISensors::getService(
                SensorsHidlEnvironment::Instance()->getServiceName<ISensors>());
            if (sensors == nullptr) {
                break;
            }

            step = "poll() check";
            // Poke ISensor service. If it has lingering connection from previous generation of
            // system server, it will kill itself. There is no intention to handle the poll result,
            // which will be done since the size is 0.
            if (!sensors->poll(0, [](auto, const auto&, const auto&) {}).isOk()) {
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
            return;
        }

        // Delay 100ms before retry, hidl service is expected to come up in short time after crash.
        ALOGI("%s unsuccessful, try again soon (remaining retry %zu).", step.c_str(), retry - 1);
        std::this_thread::sleep_for(RETRY_DELAY);
    }

    sensors = nullptr;
}

void SensorsHidlEnvironment::catEvents(std::vector<Event>* output) {
    std::lock_guard<std::mutex> lock(events_mutex);
    if (output) {
        output->insert(output->end(), events.begin(), events.end());
    }
    events.clear();
}

void SensorsHidlEnvironment::setCollection(bool enable) {
    std::lock_guard<std::mutex> lock(events_mutex);
    collectionEnabled = enable;
}

void SensorsHidlEnvironment::addEvent(const Event& ev) {
    std::lock_guard<std::mutex> lock(events_mutex);
    if (collectionEnabled) {
        events.push_back(ev);
    }
}

void SensorsHidlEnvironment::startPollingThread() {
    stopThread = false;
    pollThread = std::thread(pollingThread, this, std::ref(stopThread));
    events.reserve(128);
}

void SensorsHidlEnvironment::pollingThread(SensorsHidlEnvironment* env, std::atomic_bool& stop) {
    ALOGD("polling thread start");

    while (!stop) {
        env->sensors->poll(
            64, [&](auto result, const auto& events, const auto& dynamicSensorsAdded) {
                if (result != Result::OK ||
                    (events.size() == 0 && dynamicSensorsAdded.size() == 0) || stop) {
                    stop = true;
                    return;
                }

                for (const auto& e : events) {
                    env->addEvent(e);
                }
            });
    }
    ALOGD("polling thread end");
}