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

#ifndef ANDROID_SENSORS_HIDL_ENVIRONMENT_V1_0_H
#define ANDROID_SENSORS_HIDL_ENVIRONMENT_V1_0_H

#include <VtsHalHidlTargetTestEnvBase.h>
#include <android/hardware/sensors/1.0/ISensors.h>
#include <android/hardware/sensors/1.0/types.h>
#include <utils/StrongPointer.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using ::android::sp;

class SensorsHidlTest;
class SensorsHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    using Event = ::android::hardware::sensors::V1_0::Event;
    // get the test environment singleton
    static SensorsHidlEnvironment* Instance() {
        static SensorsHidlEnvironment* instance = new SensorsHidlEnvironment();
        return instance;
    }

    virtual void HidlSetUp() override;
    virtual void HidlTearDown() override;

    virtual void registerTestServices() override {
        registerTestService<android::hardware::sensors::V1_0::ISensors>();
    }

    // Get and clear all events collected so far (like "cat" shell command).
    // If output is nullptr, it clears all collected events.
    void catEvents(std::vector<Event>* output);

    // set sensor event collection status
    void setCollection(bool enable);

   private:
    friend SensorsHidlTest;
    // sensors hidl service
    sp<android::hardware::sensors::V1_0::ISensors> sensors;

    SensorsHidlEnvironment() {}

    void addEvent(const Event& ev);
    void startPollingThread();
    void resetHal();
    static void pollingThread(SensorsHidlEnvironment* env, std::atomic_bool& stop);

    bool collectionEnabled;
    std::atomic_bool stopThread;
    std::thread pollThread;
    std::vector<Event> events;
    std::mutex events_mutex;

    GTEST_DISALLOW_COPY_AND_ASSIGN_(SensorsHidlEnvironment);
};

#endif  // ANDROID_SENSORS_HIDL_ENVIRONMENT_V1_0_H