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

#ifndef ANDROID_SENSORS_HIDL_ENVIRONMENT_V2_X_H
#define ANDROID_SENSORS_HIDL_ENVIRONMENT_V2_X_H

#include "ISensorsWrapper.h"
#include "sensors-vts-utils/SensorsHidlEnvironmentBase.h"

#include <android/hardware/sensors/2.1/ISensors.h>
#include <android/hardware/sensors/2.1/types.h>

#include <fmq/MessageQueue.h>
#include <utils/StrongPointer.h>

#include <array>
#include <atomic>
#include <memory>

using ::android::sp;
using ::android::hardware::MessageQueue;
using ::android::hardware::sensors::V2_1::implementation::ISensorsWrapperBase;
using ::android::hardware::sensors::V2_1::implementation::MAX_RECEIVE_BUFFER_EVENT_COUNT;
using ::android::hardware::sensors::V2_1::implementation::NoOpSensorsCallback;
using ::android::hardware::sensors::V2_1::implementation::wrapISensors;

class SensorsHidlTest;

class SensorsHalDeathRecipient : public ::android::hardware::hidl_death_recipient {
    virtual void serviceDied(
            uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& service) override;
};

class SensorsHidlEnvironmentV2_X
    : public SensorsHidlEnvironmentBase<::android::hardware::sensors::V2_1::Event> {
  public:
    virtual void HidlTearDown() override;

  protected:
    friend SensorsHidlTest;
    SensorsHidlEnvironmentV2_X(const std::string& service_name)
        : SensorsHidlEnvironmentBase(service_name), mEventQueueFlag(nullptr) {}

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
    static void pollingThread(SensorsHidlEnvironmentV2_X* env);

    /**
     * Reads and saves sensor events from the Event FMQ
     */
    void readEvents();

    GTEST_DISALLOW_COPY_AND_ASSIGN_(SensorsHidlEnvironmentV2_X);

    /**
     * Pointer to the Sensors HAL Interface that allows the test to call HAL functions.
     */
    sp<ISensorsWrapperBase> mSensors;

    /**
     * Monitors the HAL for crashes, triggering test failure if seen
     */
    sp<SensorsHalDeathRecipient> mDeathRecipient = new SensorsHalDeathRecipient();

    /**
     * Type used to simplify the creation of the Wake Lock FMQ
     */
    typedef MessageQueue<uint32_t, ::android::hardware::kSynchronizedReadWrite> WakeLockQueue;

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
     * An array that is used to store sensor events read from the Event FMQ
     */
    std::array<::android::hardware::sensors::V2_1::Event, MAX_RECEIVE_BUFFER_EVENT_COUNT>
            mEventBuffer;
};

#endif  // ANDROID_SENSORS_HIDL_ENVIRONMENT_V2_X_H
