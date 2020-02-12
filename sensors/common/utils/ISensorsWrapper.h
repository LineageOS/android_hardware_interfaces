/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_SENSORS_V2_1_ISENSORSWRAPPER_H
#define ANDROID_HARDWARE_SENSORS_V2_1_ISENSORSWRAPPER_H

#include "EventMessageQueueWrapper.h"
#include "convertV2_1.h"

#include <android/hardware/sensors/2.1/ISensors.h>
#include <android/hardware/sensors/2.1/types.h>
#include <binder/IBinder.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <log/log.h>

#include <atomic>

/**
 * ISensorsWrapperBase wraps around the V2_1::ISensors APIs to make any HAL 2.0/2.1 interface
 * appear as a HAL 2.1 implementation. This ensures the maximum amount of code can be shared
 * between VTS, default implementations, and the sensors framework.
 */

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace implementation {

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::RateLevel;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SharedMemInfo;

// TODO: Look into providing this as a param if it needs to be a different value
// than the framework.
static constexpr size_t MAX_RECEIVE_BUFFER_EVENT_COUNT = 256;

class ISensorsWrapperBase : public RefBase {
  public:
    virtual ~ISensorsWrapperBase() {}

    virtual EventMessageQueueWrapperBase& getEventQueue() = 0;
    virtual Return<void> getSensorsList(V2_1::ISensors::getSensorsList_2_1_cb _hidl_cb) = 0;
    virtual Return<Result> injectSensorData(const V2_1::Event& event) = 0;
    virtual Return<Result> initialize(
            const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
            const sp<V2_1::ISensorsCallback>& sensorsCallback) = 0;

    // V2_0::ISensors implementation
    void linkToDeath(android::sp<android::hardware::hidl_death_recipient> deathRecipient,
                     uint64_t cookie) {
        getSensors()->linkToDeath(deathRecipient, cookie);
    }

    Return<Result> activate(int32_t sensorHandle, bool enabled) {
        return getSensors()->activate(sensorHandle, enabled);
    }

    Return<Result> batch(int32_t sensorHandle, int64_t samplingPeriodNs,
                         int64_t maxReportLatencyNs) {
        return getSensors()->batch(sensorHandle, samplingPeriodNs, maxReportLatencyNs);
    }

    Return<Result> flush(int32_t sensorHandle) { return getSensors()->flush(sensorHandle); }

    Return<void> registerDirectChannel(const SharedMemInfo& mem,
                                       V2_0::ISensors::registerDirectChannel_cb _hidl_cb) {
        return getSensors()->registerDirectChannel(mem, _hidl_cb);
    }

    Return<Result> unregisterDirectChannel(int32_t channelHandle) {
        return getSensors()->unregisterDirectChannel(channelHandle);
    }

    Return<void> configDirectReport(int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
                                    V2_0::ISensors::configDirectReport_cb _hidl_cb) {
        return getSensors()->configDirectReport(sensorHandle, channelHandle, rate, _hidl_cb);
    }

    Return<Result> setOperationMode(OperationMode mode) {
        return getSensors()->setOperationMode(mode);
    }

  private:
    virtual V2_0::ISensors* getSensors() = 0;
};

class ISensorsWrapperV2_0 : public ISensorsWrapperBase {
  public:
    typedef MessageQueue<V1_0::Event, ::android::hardware::kSynchronizedReadWrite>
            EventMessageQueue;

    ISensorsWrapperV2_0(sp<V2_0::ISensors>& sensors) : mSensors(sensors) {
        auto eventQueue = std::make_unique<EventMessageQueue>(MAX_RECEIVE_BUFFER_EVENT_COUNT,
                                                              true /* configureEventFlagWord */);
        mEventQueue = std::make_unique<EventMessageQueueWrapperV1_0>(eventQueue);
    }

    EventMessageQueueWrapperBase& getEventQueue() override { return *mEventQueue; }

    Return<Result> initialize(
            const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
            const sp<V2_1::ISensorsCallback>& sensorsCallback) override {
        return mSensors->initialize(*mEventQueue->getDesc(), wakeLockDescriptor, sensorsCallback);
    }

    Return<void> getSensorsList(V2_1::ISensors::getSensorsList_2_1_cb _hidl_cb) override {
        return getSensors()->getSensorsList(
                [&](const auto& list) { _hidl_cb(convertToNewSensorInfos(list)); });
    }

    Return<Result> injectSensorData(const V2_1::Event& event) override {
        return mSensors->injectSensorData(convertToOldEvent(event));
    }

  private:
    V2_0::ISensors* getSensors() override { return mSensors.get(); }

    sp<V2_0::ISensors> mSensors;
    std::unique_ptr<EventMessageQueueWrapperV1_0> mEventQueue;
};

class ISensorsWrapperV2_1 : public ISensorsWrapperBase {
  public:
    typedef MessageQueue<V2_1::Event, ::android::hardware::kSynchronizedReadWrite>
            EventMessageQueue;

    ISensorsWrapperV2_1(sp<V2_1::ISensors>& sensors) : mSensors(sensors) {
        auto eventQueue = std::make_unique<EventMessageQueue>(MAX_RECEIVE_BUFFER_EVENT_COUNT,
                                                              true /* configureEventFlagWord */);
        mEventQueue = std::make_unique<EventMessageQueueWrapperV2_1>(eventQueue);
    }

    EventMessageQueueWrapperBase& getEventQueue() override { return *mEventQueue; }

    Return<Result> initialize(
            const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
            const sp<V2_1::ISensorsCallback>& sensorsCallback) override {
        return mSensors->initialize_2_1(*mEventQueue->getDesc(), wakeLockDescriptor,
                                        sensorsCallback);
    }

    Return<void> getSensorsList(V2_1::ISensors::getSensorsList_2_1_cb _hidl_cb) override {
        return mSensors->getSensorsList_2_1(_hidl_cb);
    }

    Return<Result> injectSensorData(const V2_1::Event& event) override {
        return mSensors->injectSensorData_2_1(event);
    }

  private:
    V2_0::ISensors* getSensors() override { return mSensors.get(); }

    sp<V2_1::ISensors> mSensors;
    std::unique_ptr<EventMessageQueueWrapperV2_1> mEventQueue;
};

inline sp<ISensorsWrapperV2_0> wrapISensors(sp<V2_0::ISensors> sensors) {
    return new ISensorsWrapperV2_0(sensors);
}

inline sp<ISensorsWrapperV2_1> wrapISensors(sp<V2_1::ISensors> sensors) {
    return new ISensorsWrapperV2_1(sensors);
}

class NoOpSensorsCallback : public ISensorsCallback {
  public:
    Return<void> onDynamicSensorsConnected(
            const hidl_vec<V1_0::SensorInfo>& /* sensorInfos */) override {
        return Return<void>();
    }

    Return<void> onDynamicSensorsDisconnected(
            const hidl_vec<int32_t>& /* sensorHandles */) override {
        return Return<void>();
    }

    Return<void> onDynamicSensorsConnected_2_1(
            const hidl_vec<SensorInfo>& /* sensorInfos */) override {
        return Return<void>();
    }
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_SENSORS_V2_1_ISENSORSWRAPPER_H
