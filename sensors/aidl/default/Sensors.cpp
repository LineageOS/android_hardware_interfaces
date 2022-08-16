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

#include "sensors-impl/Sensors.h"

#include <aidl/android/hardware/common/fmq/SynchronizedReadWrite.h>

using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::aidl::android::hardware::sensors::Event;
using ::aidl::android::hardware::sensors::ISensors;
using ::aidl::android::hardware::sensors::ISensorsCallback;
using ::aidl::android::hardware::sensors::SensorInfo;
using ::ndk::ScopedAStatus;

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {

ScopedAStatus Sensors::activate(int32_t in_sensorHandle, bool in_enabled) {
    auto sensor = mSensors.find(in_sensorHandle);
    if (sensor != mSensors.end()) {
        sensor->second->activate(in_enabled);
        return ScopedAStatus::ok();
    }

    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
}

ScopedAStatus Sensors::batch(int32_t in_sensorHandle, int64_t in_samplingPeriodNs,
                             int64_t /* in_maxReportLatencyNs */) {
    auto sensor = mSensors.find(in_sensorHandle);
    if (sensor != mSensors.end()) {
        sensor->second->batch(in_samplingPeriodNs);
        return ScopedAStatus::ok();
    }

    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
}

ScopedAStatus Sensors::configDirectReport(int32_t /* in_sensorHandle */,
                                          int32_t /* in_channelHandle */,
                                          ISensors::RateLevel /* in_rate */,
                                          int32_t* _aidl_return) {
    *_aidl_return = EX_UNSUPPORTED_OPERATION;

    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus Sensors::flush(int32_t in_sensorHandle) {
    auto sensor = mSensors.find(in_sensorHandle);
    if (sensor != mSensors.end()) {
        return sensor->second->flush();
    }

    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
}

ScopedAStatus Sensors::getSensorsList(std::vector<SensorInfo>* _aidl_return) {
    for (const auto& sensor : mSensors) {
        _aidl_return->push_back(sensor.second->getSensorInfo());
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Sensors::initialize(
        const MQDescriptor<Event, SynchronizedReadWrite>& in_eventQueueDescriptor,
        const MQDescriptor<int32_t, SynchronizedReadWrite>& in_wakeLockDescriptor,
        const std::shared_ptr<::aidl::android::hardware::sensors::ISensorsCallback>&
                in_sensorsCallback) {
    ScopedAStatus result = ScopedAStatus::ok();

    mEventQueue = std::make_unique<AidlMessageQueue<Event, SynchronizedReadWrite>>(
            in_eventQueueDescriptor, true /* resetPointers */);

    // Ensure that all sensors are disabled.
    for (auto sensor : mSensors) {
        sensor.second->activate(false);
    }

    // Stop the Wake Lock thread if it is currently running
    if (mReadWakeLockQueueRun.load()) {
        mReadWakeLockQueueRun = false;
        mWakeLockThread.join();
    }

    // Save a reference to the callback
    mCallback = in_sensorsCallback;

    // Ensure that any existing EventFlag is properly deleted
    deleteEventFlag();

    // Create the EventFlag that is used to signal to the framework that sensor events have been
    // written to the Event FMQ
    if (EventFlag::createEventFlag(mEventQueue->getEventFlagWord(), &mEventQueueFlag) != OK) {
        result = ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    // Create the Wake Lock FMQ that is used by the framework to communicate whenever WAKE_UP
    // events have been successfully read and handled by the framework.
    mWakeLockQueue = std::make_unique<AidlMessageQueue<int32_t, SynchronizedReadWrite>>(
            in_wakeLockDescriptor, true /* resetPointers */);

    if (!mCallback || !mEventQueue || !mWakeLockQueue || mEventQueueFlag == nullptr) {
        result = ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    // Start the thread to read events from the Wake Lock FMQ
    mReadWakeLockQueueRun = true;
    mWakeLockThread = std::thread(startReadWakeLockThread, this);
    return result;
}

ScopedAStatus Sensors::injectSensorData(const Event& in_event) {
    auto sensor = mSensors.find(in_event.sensorHandle);
    if (sensor != mSensors.end()) {
        return sensor->second->injectEvent(in_event);
    }
    return ScopedAStatus::fromServiceSpecificError(static_cast<int32_t>(ERROR_BAD_VALUE));
}

ScopedAStatus Sensors::registerDirectChannel(const ISensors::SharedMemInfo& /* in_mem */,
                                             int32_t* _aidl_return) {
    *_aidl_return = EX_UNSUPPORTED_OPERATION;

    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus Sensors::setOperationMode(OperationMode in_mode) {
    for (auto sensor : mSensors) {
        sensor.second->setOperationMode(in_mode);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Sensors::unregisterDirectChannel(int32_t /* in_channelHandle */) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
