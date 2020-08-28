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
#include "ISensorsWrapper.h"

#include "android/hardware/sensors/1.0/ISensors.h"
#include "android/hardware/sensors/1.0/types.h"
#include "android/hardware/sensors/2.0/ISensors.h"
#include "android/hardware/sensors/2.0/ISensorsCallback.h"
#include "android/hardware/sensors/2.1/ISensors.h"
#include "android/hardware/sensors/2.1/ISensorsCallback.h"
#include "android/hardware/sensors/2.1/types.h"

#include <utils/LightRefBase.h>

#include <cassert>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace implementation {

using ::android::hardware::MessageQueue;
using ::android::hardware::MQDescriptorSync;
using ::android::hardware::Return;
using ::android::hardware::sensors::V1_0::ISensors;
using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::RateLevel;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SharedMemInfo;
using ::android::hardware::sensors::V2_1::Event;
using ::android::hardware::sensors::V2_1::ISensorsCallback;

// TODO: Look into providing this as a param if it needs to be a different value
// than the framework.
static constexpr size_t MAX_RECEIVE_BUFFER_EVENT_COUNT = 256;

/*
 * The ISensorsWrapper interface includes all function from supported Sensors HAL versions. This
 * allows for the SensorDevice to use the ISensorsWrapper interface to interact with the Sensors
 * HAL regardless of the current version of the Sensors HAL that is loaded. Each concrete
 * instantiation of ISensorsWrapper must correspond to a specific Sensors HAL version. This design
 * is beneficial because only the functions that change between Sensors HAL versions must be newly
 * implemented, any previously implemented function that does not change may remain the same.
 *
 * Functions that exist across all versions of the Sensors HAL should be implemented as pure
 * virtual functions which forces the concrete instantiations to implement the functions.
 *
 * Functions that do not exist across all versions of the Sensors HAL should include a default
 * implementation that generates an error if called. The default implementation should never
 * be called and must be overridden by Sensors HAL versions that support the function.
 */
class ISensorsWrapperBase : public VirtualLightRefBase {
  public:
    virtual bool supportsPolling() const = 0;

    virtual bool supportsMessageQueues() const = 0;

    virtual void linkToDeath(android::sp<android::hardware::hidl_death_recipient> deathRecipient,
                             uint64_t cookie) = 0;

    virtual Return<void> getSensorsList(
            ::android::hardware::sensors::V2_1::ISensors::getSensorsList_2_1_cb _hidl_cb) = 0;

    virtual Return<Result> setOperationMode(OperationMode mode) = 0;

    virtual Return<Result> activate(int32_t sensorHandle, bool enabled) = 0;

    virtual Return<Result> batch(int32_t sensorHandle, int64_t samplingPeriodNs,
                                 int64_t maxReportLatencyNs) = 0;

    virtual Return<Result> flush(int32_t sensorHandle) = 0;

    virtual Return<Result> injectSensorData(const Event& event) = 0;

    virtual Return<void> registerDirectChannel(const SharedMemInfo& mem,
                                               ISensors::registerDirectChannel_cb _hidl_cb) = 0;

    virtual Return<Result> unregisterDirectChannel(int32_t channelHandle) = 0;

    virtual Return<void> configDirectReport(int32_t sensorHandle, int32_t channelHandle,
                                            RateLevel rate,
                                            ISensors::configDirectReport_cb _hidl_cb) = 0;

    virtual Return<void> poll(int32_t /* maxCount */, ISensors::poll_cb /* _hidl_cb */) {
        // Enforce this method is never invoked as it should be overridden if it's meant to be used.
        assert(false);
        return Return<void>();
    }

    virtual EventMessageQueueWrapperBase* getEventQueue() { return nullptr; }

    virtual Return<Result> initialize(const MQDescriptorSync<uint32_t>& /* wakeLockDesc */,
                                      const ::android::sp<ISensorsCallback>& /* callback */) {
        // Enforce this method is never invoked as it should be overridden if it's meant to be used.
        assert(false);
        return Result::INVALID_OPERATION;
    }
};

template <typename T>
class SensorsWrapperBase : public ISensorsWrapperBase {
  public:
    SensorsWrapperBase(sp<T> sensors) : mSensors(sensors){};

    void linkToDeath(android::sp<android::hardware::hidl_death_recipient> deathRecipient,
                     uint64_t cookie) override {
        mSensors->linkToDeath(deathRecipient, cookie);
    }

    virtual Return<void> getSensorsList(
            ::android::hardware::sensors::V2_1::ISensors::getSensorsList_2_1_cb _hidl_cb) override {
        return mSensors->getSensorsList(
                [&](const auto& list) { _hidl_cb(convertToNewSensorInfos(list)); });
    }

    Return<Result> setOperationMode(OperationMode mode) override {
        return mSensors->setOperationMode(mode);
    }

    Return<Result> activate(int32_t sensorHandle, bool enabled) override {
        return mSensors->activate(sensorHandle, enabled);
    }

    Return<Result> batch(int32_t sensorHandle, int64_t samplingPeriodNs,
                         int64_t maxReportLatencyNs) override {
        return mSensors->batch(sensorHandle, samplingPeriodNs, maxReportLatencyNs);
    }

    Return<Result> flush(int32_t sensorHandle) override { return mSensors->flush(sensorHandle); }

    virtual Return<Result> injectSensorData(const Event& event) override {
        return mSensors->injectSensorData(convertToOldEvent(event));
    }

    Return<void> registerDirectChannel(const SharedMemInfo& mem,
                                       ISensors::registerDirectChannel_cb _hidl_cb) override {
        return mSensors->registerDirectChannel(mem, _hidl_cb);
    }

    Return<Result> unregisterDirectChannel(int32_t channelHandle) override {
        return mSensors->unregisterDirectChannel(channelHandle);
    }

    Return<void> configDirectReport(int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
                                    ISensors::configDirectReport_cb _hidl_cb) override {
        return mSensors->configDirectReport(sensorHandle, channelHandle, rate, _hidl_cb);
    }

  protected:
    sp<T> mSensors;
};

class ISensorsWrapperV1_0 : public SensorsWrapperBase<hardware::sensors::V1_0::ISensors> {
  public:
    ISensorsWrapperV1_0(sp<hardware::sensors::V1_0::ISensors> sensors)
        : SensorsWrapperBase(sensors){};

    bool supportsPolling() const override { return true; }

    bool supportsMessageQueues() const override { return false; }

    Return<void> poll(int32_t maxCount,
                      hardware::sensors::V1_0::ISensors::poll_cb _hidl_cb) override {
        return mSensors->poll(maxCount, _hidl_cb);
    }
};

class ISensorsWrapperV2_0 : public SensorsWrapperBase<hardware::sensors::V2_0::ISensors> {
  public:
    typedef MessageQueue<::android::hardware::sensors::V1_0::Event,
                         ::android::hardware::kSynchronizedReadWrite>
            EventMessageQueue;

    ISensorsWrapperV2_0(sp<hardware::sensors::V2_0::ISensors> sensors)
        : SensorsWrapperBase(sensors) {
        auto eventQueue = std::make_unique<EventMessageQueue>(MAX_RECEIVE_BUFFER_EVENT_COUNT,
                                                              true /* configureEventFlagWord */);
        mEventQueue = std::make_unique<EventMessageQueueWrapperV1_0>(eventQueue);
    };

    bool supportsPolling() const override { return false; }

    bool supportsMessageQueues() const override { return true; }

    EventMessageQueueWrapperBase* getEventQueue() override { return mEventQueue.get(); }

    Return<Result> initialize(const MQDescriptorSync<uint32_t>& wakeLockDesc,
                              const ::android::sp<ISensorsCallback>& callback) override {
        return mSensors->initialize(*mEventQueue->getDesc(), wakeLockDesc, callback);
    }

  private:
    std::unique_ptr<EventMessageQueueWrapperV1_0> mEventQueue;
};

class ISensorsWrapperV2_1 : public SensorsWrapperBase<hardware::sensors::V2_1::ISensors> {
  public:
    typedef MessageQueue<Event, ::android::hardware::kSynchronizedReadWrite> EventMessageQueueV2_1;

    ISensorsWrapperV2_1(sp<hardware::sensors::V2_1::ISensors> sensors)
        : SensorsWrapperBase(sensors) {
        auto eventQueue = std::make_unique<EventMessageQueueV2_1>(
                MAX_RECEIVE_BUFFER_EVENT_COUNT, true /* configureEventFlagWord */);
        mEventQueue = std::make_unique<EventMessageQueueWrapperV2_1>(eventQueue);
    };

    bool supportsPolling() const override { return false; }

    bool supportsMessageQueues() const override { return true; }

    EventMessageQueueWrapperBase* getEventQueue() override { return mEventQueue.get(); }

    Return<void> getSensorsList(
            ::android::hardware::sensors::V2_1::ISensors::getSensorsList_2_1_cb _hidl_cb) override {
        return mSensors->getSensorsList_2_1(_hidl_cb);
    }

    Return<Result> injectSensorData(const Event& event) override {
        return mSensors->injectSensorData_2_1(event);
    }

    Return<Result> initialize(const MQDescriptorSync<uint32_t>& wakeLockDesc,
                              const ::android::sp<ISensorsCallback>& callback) override {
        return mSensors->initialize_2_1(*mEventQueue->getDesc(), wakeLockDesc, callback);
    }

  private:
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