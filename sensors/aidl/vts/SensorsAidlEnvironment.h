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

#ifndef ANDROID_SENSORS_AIDL_ENVIRONMENT_H
#define ANDROID_SENSORS_AIDL_ENVIRONMENT_H

#include "sensors-vts-utils/SensorsVtsEnvironmentBase.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <aidl/android/hardware/sensors/ISensors.h>
#include <fmq/AidlMessageQueue.h>

using aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using aidl::android::hardware::sensors::Event;
using aidl::android::hardware::sensors::ISensors;
using aidl::android::hardware::sensors::ISensorsCallback;

static constexpr size_t MAX_RECEIVE_BUFFER_EVENT_COUNT = 256;

class SensorsAidlTest;

class SensorsAidlEnvironment : public SensorsVtsEnvironmentBase<Event> {
  public:
    virtual void TearDown() override;

  protected:
    friend SensorsAidlTest;
    SensorsAidlEnvironment(const std::string& service_name);

    /**
     * Resets the HAL with new FMQs and a new Event Flag
     *
     * @return bool true if successful, false otherwise
     */
    bool resetHal() override;

    /**
     * Starts the polling thread that reads sensor events from the Event FMQ
     */
    void startPollingThread() override;

    /**
     * Thread responsible for calling functions to read Event FMQ
     *
     * @param env SensorEnvironment to being polling for events on
     */
    static void pollingThread(SensorsAidlEnvironment* env);

    /**
     * Reads and saves sensor events from the Event FMQ
     */
    void readEvents();

    /**
     * Pointer to the Sensors HAL Interface that allows the test to call HAL functions.
     */
    std::shared_ptr<ISensors> mSensors;
    std::shared_ptr<ISensorsCallback> mCallback;

    ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;

    /**
     * Type used to simplify the creation of the Wake Lock FMQ
     */
    typedef android::AidlMessageQueue<int32_t, SynchronizedReadWrite> WakeLockQueue;
    typedef android::AidlMessageQueue<Event, SynchronizedReadWrite> EventQueue;

    /**
     * The Wake Lock FMQ is used by the test to notify the Sensors HAL whenever it has processed
     * WAKE_UP sensor events.
     */
    std::unique_ptr<WakeLockQueue> mWakeLockQueue;
    std::unique_ptr<EventQueue> mEventQueue;

    /**
     * The Event Queue Flag notifies the test framework when sensor events have been written to the
     * Event FMQ by the Sensors HAL.
     */
    ::android::hardware::EventFlag* mEventQueueFlag;

    std::atomic_bool mStopThread;
    std::thread mPollThread;

    /**
     * An array that is used to store sensor events read from the Event FMQ
     */
    std::array<Event, MAX_RECEIVE_BUFFER_EVENT_COUNT> mEventBuffer;
};

#endif  // ANDROID_SENSORS_AIDL_ENVIRONMENT_H
