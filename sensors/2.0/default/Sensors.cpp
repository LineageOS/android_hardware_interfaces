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

#include "Sensors.h"

#include <android/hardware/sensors/2.0/types.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

using ::android::hardware::sensors::V1_0::Event;
using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::RateLevel;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SharedMemInfo;
using ::android::hardware::sensors::V2_0::SensorTimeout;

constexpr const char* kWakeLockName = "SensorsHAL_WAKEUP";

Sensors::Sensors()
    : mEventQueueFlag(nullptr),
      mOutstandingWakeUpEvents(0),
      mReadWakeLockQueueRun(false),
      mAutoReleaseWakeLockTime(0),
      mHasWakeLock(false) {
    std::shared_ptr<AccelSensor> accel =
        std::make_shared<AccelSensor>(1 /* sensorHandle */, this /* callback */);
    mSensors[accel->getSensorInfo().sensorHandle] = accel;
}

Sensors::~Sensors() {
    deleteEventFlag();
    mReadWakeLockQueueRun = false;
    mWakeLockThread.join();
}

// Methods from ::android::hardware::sensors::V2_0::ISensors follow.
Return<void> Sensors::getSensorsList(getSensorsList_cb _hidl_cb) {
    std::vector<SensorInfo> sensors;
    for (const auto& sensor : mSensors) {
        sensors.push_back(sensor.second->getSensorInfo());
    }

    // Call the HIDL callback with the SensorInfo
    _hidl_cb(sensors);

    return Void();
}

Return<Result> Sensors::setOperationMode(OperationMode mode) {
    for (auto sensor : mSensors) {
        sensor.second->setOperationMode(mode);
    }
    return Result::OK;
}

Return<Result> Sensors::activate(int32_t sensorHandle, bool enabled) {
    auto sensor = mSensors.find(sensorHandle);
    if (sensor != mSensors.end()) {
        sensor->second->activate(enabled);
        return Result::OK;
    }
    return Result::BAD_VALUE;
}

Return<Result> Sensors::initialize(
    const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
    const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
    const sp<ISensorsCallback>& sensorsCallback) {
    Result result = Result::OK;

    // Save a reference to the callback
    mCallback = sensorsCallback;

    // Create the Event FMQ from the eventQueueDescriptor. Reset the read/write positions.
    mEventQueue =
        std::make_unique<EventMessageQueue>(eventQueueDescriptor, true /* resetPointers */);

    // Ensure that any existing EventFlag is properly deleted
    deleteEventFlag();

    // Create the EventFlag that is used to signal to the framework that sensor events have been
    // written to the Event FMQ
    if (EventFlag::createEventFlag(mEventQueue->getEventFlagWord(), &mEventQueueFlag) != OK) {
        result = Result::BAD_VALUE;
    }

    // Create the Wake Lock FMQ that is used by the framework to communicate whenever WAKE_UP
    // events have been successfully read and handled by the framework.
    mWakeLockQueue =
        std::make_unique<WakeLockMessageQueue>(wakeLockDescriptor, true /* resetPointers */);

    if (!mCallback || !mEventQueue || !mWakeLockQueue || mEventQueueFlag == nullptr) {
        result = Result::BAD_VALUE;
    }

    // Start the thread to read events from the Wake Lock FMQ
    mReadWakeLockQueueRun = true;
    mWakeLockThread = std::thread(startReadWakeLockThread, this);

    return result;
}

Return<Result> Sensors::batch(int32_t sensorHandle, int64_t samplingPeriodNs,
                              int64_t /* maxReportLatencyNs */) {
    auto sensor = mSensors.find(sensorHandle);
    if (sensor != mSensors.end()) {
        sensor->second->batch(samplingPeriodNs);
        return Result::OK;
    }
    return Result::BAD_VALUE;
}

Return<Result> Sensors::flush(int32_t sensorHandle) {
    auto sensor = mSensors.find(sensorHandle);
    if (sensor != mSensors.end()) {
        return sensor->second->flush();
    }
    return Result::BAD_VALUE;
}

Return<Result> Sensors::injectSensorData(const Event& event) {
    auto sensor = mSensors.find(event.sensorHandle);
    if (sensor != mSensors.end()) {
        return sensor->second->injectEvent(event);
    }

    return Result::BAD_VALUE;
}

Return<void> Sensors::registerDirectChannel(const SharedMemInfo& /* mem */,
                                            registerDirectChannel_cb _hidl_cb) {
    _hidl_cb(Result::INVALID_OPERATION, -1 /* channelHandle */);
    return Return<void>();
}

Return<Result> Sensors::unregisterDirectChannel(int32_t /* channelHandle */) {
    return Result::INVALID_OPERATION;
}

Return<void> Sensors::configDirectReport(int32_t /* sensorHandle */, int32_t /* channelHandle */,
                                         RateLevel /* rate */, configDirectReport_cb _hidl_cb) {
    _hidl_cb(Result::INVALID_OPERATION, 0 /* reportToken */);
    return Return<void>();
}

void Sensors::postEvents(const std::vector<Event>& events, bool wakeup) {
    std::lock_guard<std::mutex> lock(mWriteLock);
    if (mEventQueue->write(events.data(), events.size())) {
        mEventQueueFlag->wake(static_cast<uint32_t>(EventQueueFlagBits::READ_AND_PROCESS));

        if (wakeup) {
            // Keep track of the number of outstanding WAKE_UP events in order to properly hold
            // a wake lock until the framework has secured a wake lock
            updateWakeLock(events.size(), 0 /* eventsHandled */);
        }
    }
}

void Sensors::updateWakeLock(int32_t eventsWritten, int32_t eventsHandled) {
    std::lock_guard<std::mutex> lock(mWakeLockLock);
    int32_t newVal = mOutstandingWakeUpEvents + eventsWritten - eventsHandled;
    if (newVal < 0) {
        mOutstandingWakeUpEvents = 0;
    } else {
        mOutstandingWakeUpEvents = newVal;
    }

    if (eventsWritten > 0) {
        // Update the time at which the last WAKE_UP event was sent
        mAutoReleaseWakeLockTime = ::android::uptimeMillis() +
                                   static_cast<uint32_t>(SensorTimeout::WAKE_LOCK_SECONDS) * 1000;
    }

    if (!mHasWakeLock && mOutstandingWakeUpEvents > 0 &&
        acquire_wake_lock(PARTIAL_WAKE_LOCK, kWakeLockName) == 0) {
        mHasWakeLock = true;
    } else if (mHasWakeLock) {
        // Check if the wake lock should be released automatically if
        // SensorTimeout::WAKE_LOCK_SECONDS has elapsed since the last WAKE_UP event was written to
        // the Wake Lock FMQ.
        if (::android::uptimeMillis() > mAutoReleaseWakeLockTime) {
            ALOGD("No events read from wake lock FMQ for %d seconds, auto releasing wake lock",
                  SensorTimeout::WAKE_LOCK_SECONDS);
            mOutstandingWakeUpEvents = 0;
        }

        if (mOutstandingWakeUpEvents == 0 && release_wake_lock(kWakeLockName) == 0) {
            mHasWakeLock = false;
        }
    }
}

void Sensors::readWakeLockFMQ() {
    while (mReadWakeLockQueueRun.load()) {
        constexpr int64_t kReadTimeoutNs = 500 * 1000 * 1000;  // 500 ms
        uint32_t eventsHandled = 0;

        // Read events from the Wake Lock FMQ. Timeout after a reasonable amount of time to ensure
        // that any held wake lock is able to be released if it is held for too long.
        mWakeLockQueue->readBlocking(&eventsHandled, 1 /* count */, kReadTimeoutNs);
        updateWakeLock(0 /* eventsWritten */, eventsHandled);
    }
}

void Sensors::startReadWakeLockThread(Sensors* sensors) {
    sensors->readWakeLockFMQ();
}

void Sensors::deleteEventFlag() {
    status_t status = EventFlag::deleteEventFlag(&mEventQueueFlag);
    if (status != OK) {
        ALOGI("Failed to delete event flag: %d", status);
    }
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
