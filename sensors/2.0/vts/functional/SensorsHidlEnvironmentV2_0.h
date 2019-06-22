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

#ifndef ANDROID_SENSORS_HIDL_ENVIRONMENT_V2_0_H
#define ANDROID_SENSORS_HIDL_ENVIRONMENT_V2_0_H

#include "sensors-vts-utils/SensorsHidlEnvironmentBase.h"

#include <android/hardware/sensors/1.0/types.h>
#include <android/hardware/sensors/2.0/ISensors.h>
#include <fmq/MessageQueue.h>
#include <utils/StrongPointer.h>

#include <array>
#include <atomic>
#include <memory>

using ::android::sp;
using ::android::hardware::MessageQueue;

class SensorsHidlTest;

class SensorsHalDeathRecipient : public ::android::hardware::hidl_death_recipient {
    virtual void serviceDied(
            uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& service) override;
};

class SensorsHidlEnvironmentV2_0 : public SensorsHidlEnvironmentBase {
   public:
    using Event = ::android::hardware::sensors::V1_0::Event;
    // get the test environment singleton
    static SensorsHidlEnvironmentV2_0* Instance() {
        static SensorsHidlEnvironmentV2_0* instance = new SensorsHidlEnvironmentV2_0();
        return instance;
    }

    virtual void registerTestServices() override {
        registerTestService<android::hardware::sensors::V2_0::ISensors>();
    }

    virtual void HidlTearDown() override;

   protected:
    friend SensorsHidlTest;

    SensorsHidlEnvironmentV2_0() : mEventQueueFlag(nullptr) {}

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
    static void pollingThread(SensorsHidlEnvironmentV2_0* env);

    /**
     * Reads and saves sensor events from the Event FMQ
     */
    void readEvents();

    GTEST_DISALLOW_COPY_AND_ASSIGN_(SensorsHidlEnvironmentV2_0);

    /**
     * Pointer to the Sensors HAL Interface that allows the test to call HAL functions.
     */
    sp<android::hardware::sensors::V2_0::ISensors> mSensors;

    /**
     * Monitors the HAL for crashes, triggering test failure if seen
     */
    sp<SensorsHalDeathRecipient> mDeathRecipient = new SensorsHalDeathRecipient();

    /**
     * Type used to simplify the creation of the Event FMQ
     */
    typedef MessageQueue<Event, ::android::hardware::kSynchronizedReadWrite> EventMessageQueue;

    /**
     * Type used to simplify the creation of the Wake Lock FMQ
     */
    typedef MessageQueue<uint32_t, ::android::hardware::kSynchronizedReadWrite> WakeLockQueue;

    /**
     * The Event FMQ where the test framework is able to read sensor events that the Sensors HAL
     * has written.
     */
    std::unique_ptr<EventMessageQueue> mEventQueue;

    /**
     * The Wake Lock FMQ is used by the test to notify the Sensors HAL whenever it has processed
     * WAKE_UP sensor events.
     */
    std::unique_ptr<WakeLockQueue> mWakeLockQueue;

    /**
     * The Event Queue Flag notifies the test framework when sensor events have been written to the
     * Event FMQ by the Sensors HAL.
     */
    ::android::hardware::EventFlag* mEventQueueFlag;

    /**
     * The maximum number of sensor events that can be read from the Event FMQ at one time.
     */
    static constexpr size_t MAX_RECEIVE_BUFFER_EVENT_COUNT = 128;

    /**
     * An array that is used to store sensor events read from the Event FMQ
     */
    std::array<Event, MAX_RECEIVE_BUFFER_EVENT_COUNT> mEventBuffer;
};

#endif  // ANDROID_SENSORS_HIDL_ENVIRONMENT_V2_0_H
