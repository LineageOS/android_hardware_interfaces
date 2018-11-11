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

#ifndef ANDROID_SENSORS_HIDL_ENVIRONMENT_BASE_H
#define ANDROID_SENSORS_HIDL_ENVIRONMENT_BASE_H

#include <VtsHalHidlTargetTestEnvBase.h>

#include <android/hardware/sensors/1.0/types.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class IEventCallback {
   public:
    virtual ~IEventCallback() = default;
    virtual void onEvent(const ::android::hardware::sensors::V1_0::Event& event) = 0;
};

class SensorsHidlEnvironmentBase : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    using Event = ::android::hardware::sensors::V1_0::Event;
    virtual void HidlSetUp() override;
    virtual void HidlTearDown() override;

    // Get and clear all events collected so far (like "cat" shell command).
    // If output is nullptr, it clears all collected events.
    void catEvents(std::vector<Event>* output);

    // set sensor event collection status
    void setCollection(bool enable);

    void registerCallback(IEventCallback* callback);
    void unregisterCallback();

   protected:
    SensorsHidlEnvironmentBase() : mCollectionEnabled(false), mCallback(nullptr) {}

    void addEvent(const Event& ev);

    virtual void startPollingThread() = 0;
    virtual bool resetHal() = 0;

    bool mCollectionEnabled;
    std::atomic_bool mStopThread;
    std::thread mPollThread;
    std::vector<Event> mEvents;
    std::mutex mEventsMutex;

    IEventCallback* mCallback;

    GTEST_DISALLOW_COPY_AND_ASSIGN_(SensorsHidlEnvironmentBase);
};

#endif  // ANDROID_SENSORS_HIDL_ENVIRONMENT_BASE_H