//
// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <android/hardware/sensors/2.0/types.h>
#include <fmq/MessageQueue.h>

#include "HalProxy.h"
#include "SensorsSubHal.h"
#include "SubHal.h"

#include <vector>

#undef LOG_TAG
#define LOG_TAG "HalProxy_test"

namespace {

using ::android::hardware::hidl_vec;
using ::android::hardware::MessageQueue;
using ::android::hardware::Return;
using ::android::hardware::sensors::V1_0::EventPayload;
using ::android::hardware::sensors::V1_0::SensorFlagBits;
using ::android::hardware::sensors::V1_0::SensorInfo;
using ::android::hardware::sensors::V1_0::SensorType;
using ::android::hardware::sensors::V2_0::ISensorsCallback;
using ::android::hardware::sensors::V2_0::implementation::HalProxy;
using ::android::hardware::sensors::V2_0::implementation::HalProxyCallback;
using ::android::hardware::sensors::V2_0::subhal::implementation::AllSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::
        AllSupportDirectChannelSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::ContinuousSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::
        DoesNotSupportDirectChannelSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::OnChangeSensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::SensorsSubHal;
using ::android::hardware::sensors::V2_0::subhal::implementation::
        SetOperationModeFailingSensorsSubHal;

using EventMessageQueue = MessageQueue<Event, ::android::hardware::kSynchronizedReadWrite>;
using WakeupMessageQueue = MessageQueue<uint32_t, ::android::hardware::kSynchronizedReadWrite>;

// The barebones sensors callback class passed into halproxy initialize calls
class SensorsCallback : public ISensorsCallback {
  public:
    Return<void> onDynamicSensorsConnected(
            const hidl_vec<SensorInfo>& /*dynamicSensorsAdded*/) override {
        // Nothing yet
        return Return<void>();
    }

    Return<void> onDynamicSensorsDisconnected(
            const hidl_vec<int32_t>& /*dynamicSensorHandlesRemoved*/) override {
        // Nothing yet
        return Return<void>();
    }
};

// Helper declarations follow

/**
 * Tests that for each SensorInfo object from a proxy getSensorsList call each corresponding
 * object from a subhal getSensorsList call has the same type and its last 3 bytes are the
 * same for sensorHandle field.
 *
 * @param proxySensorsList The list of SensorInfo objects from the proxy.getSensorsList callback.
 * @param subHalSenosrsList The list of SensorInfo objects from the subHal.getSensorsList callback.
 */
void testSensorsListFromProxyAndSubHal(const std::vector<SensorInfo>& proxySensorsList,
                                       const std::vector<SensorInfo>& subHalSensorsList);

/**
 * Tests that there is exactly one subhal that allows its sensors to have direct channel enabled.
 * Therefore, all SensorInfo objects that are not from the enabled subhal should be disabled for
 * direct channel.
 *
 * @param sensorsList The SensorInfo object list from proxy.getSensorsList call.
 * @param enabledSubHalIndex The index of the subhal in the halproxy that is expected to be
 *     enabled.
 */
void testSensorsListForOneDirectChannelEnabledSubHal(const std::vector<SensorInfo>& sensorsList,
                                                     size_t enabledSubHalIndex);

/**
 * Construct and return a HIDL Event type thats sensorHandle refers to a proximity sensor
 *    which is a wakeup type sensor.
 *
 * @ return A proximity event.
 */
Event makeProximityEvent();

/**
 * Construct and return a HIDL Event type thats sensorHandle refers to a proximity sensor
 *    which is a wakeup type sensor.
 *
 * @ return A proximity event.
 */
Event makeAccelerometerEvent();

// Tests follow
TEST(HalProxyTest, GetSensorsListOneSubHalTest) {
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> fakeSubHals{&subHal};
    HalProxy proxy(fakeSubHals);

    proxy.getSensorsList([&](const auto& proxySensorsList) {
        subHal.getSensorsList([&](const auto& subHalSensorsList) {
            testSensorsListFromProxyAndSubHal(proxySensorsList, subHalSensorsList);
        });
    });
}

TEST(HalProxyTest, GetSensorsListTwoSubHalTest) {
    ContinuousSensorsSubHal continuousSubHal;
    OnChangeSensorsSubHal onChangeSubHal;
    std::vector<ISensorsSubHal*> fakeSubHals;
    fakeSubHals.push_back(&continuousSubHal);
    fakeSubHals.push_back(&onChangeSubHal);
    HalProxy proxy(fakeSubHals);

    std::vector<SensorInfo> proxySensorsList, combinedSubHalSensorsList;

    proxy.getSensorsList([&](const auto& list) { proxySensorsList = list; });
    continuousSubHal.getSensorsList([&](const auto& list) {
        combinedSubHalSensorsList.insert(combinedSubHalSensorsList.end(), list.begin(), list.end());
    });
    onChangeSubHal.getSensorsList([&](const auto& list) {
        combinedSubHalSensorsList.insert(combinedSubHalSensorsList.end(), list.begin(), list.end());
    });

    testSensorsListFromProxyAndSubHal(proxySensorsList, combinedSubHalSensorsList);
}

TEST(HalProxyTest, SetOperationModeTwoSubHalSuccessTest) {
    ContinuousSensorsSubHal subHal1;
    OnChangeSensorsSubHal subHal2;

    std::vector<ISensorsSubHal*> fakeSubHals{&subHal1, &subHal2};
    HalProxy proxy(fakeSubHals);

    EXPECT_EQ(subHal1.getOperationMode(), OperationMode::NORMAL);
    EXPECT_EQ(subHal2.getOperationMode(), OperationMode::NORMAL);

    Result result = proxy.setOperationMode(OperationMode::DATA_INJECTION);

    EXPECT_EQ(result, Result::OK);
    EXPECT_EQ(subHal1.getOperationMode(), OperationMode::DATA_INJECTION);
    EXPECT_EQ(subHal2.getOperationMode(), OperationMode::DATA_INJECTION);
}

TEST(HalProxyTest, SetOperationModeTwoSubHalFailTest) {
    AllSensorsSubHal subHal1;
    SetOperationModeFailingSensorsSubHal subHal2;

    std::vector<ISensorsSubHal*> fakeSubHals{&subHal1, &subHal2};
    HalProxy proxy(fakeSubHals);

    EXPECT_EQ(subHal1.getOperationMode(), OperationMode::NORMAL);
    EXPECT_EQ(subHal2.getOperationMode(), OperationMode::NORMAL);

    Result result = proxy.setOperationMode(OperationMode::DATA_INJECTION);

    EXPECT_NE(result, Result::OK);
    EXPECT_EQ(subHal1.getOperationMode(), OperationMode::NORMAL);
    EXPECT_EQ(subHal2.getOperationMode(), OperationMode::NORMAL);
}

TEST(HalProxyTest, InitDirectChannelTwoSubHalsUnitTest) {
    AllSupportDirectChannelSensorsSubHal subHal1;
    AllSupportDirectChannelSensorsSubHal subHal2;

    std::vector<ISensorsSubHal*> fakeSubHals{&subHal1, &subHal2};
    HalProxy proxy(fakeSubHals);

    proxy.getSensorsList([&](const auto& sensorsList) {
        testSensorsListForOneDirectChannelEnabledSubHal(sensorsList, 0);
    });
}

TEST(HalProxyTest, InitDirectChannelThreeSubHalsUnitTest) {
    DoesNotSupportDirectChannelSensorsSubHal subHal1;
    AllSupportDirectChannelSensorsSubHal subHal2, subHal3;
    std::vector<ISensorsSubHal*> fakeSubHals{&subHal1, &subHal2, &subHal3};
    HalProxy proxy(fakeSubHals);

    proxy.getSensorsList([&](const auto& sensorsList) {
        testSensorsListForOneDirectChannelEnabledSubHal(sensorsList, 1);
    });
}

TEST(HalProxyTest, PostSingleNonWakeupEvent) {
    constexpr size_t kQueueSize = 5;
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};
    HalProxy proxy(subHals);
    std::unique_ptr<EventMessageQueue> eventQueue =
            std::make_unique<EventMessageQueue>(kQueueSize, true);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue =
            std::make_unique<WakeupMessageQueue>(kQueueSize, true);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    std::vector<Event> events{makeAccelerometerEvent()};
    subHal.postEvents(events, false /* wakeup */);

    EXPECT_EQ(eventQueue->availableToRead(), 1);
}

TEST(HalProxyTest, PostMultipleNonWakeupEvent) {
    constexpr size_t kQueueSize = 5;
    constexpr size_t kNumEvents = 3;
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};
    HalProxy proxy(subHals);
    std::unique_ptr<EventMessageQueue> eventQueue =
            std::make_unique<EventMessageQueue>(kQueueSize, true);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue =
            std::make_unique<WakeupMessageQueue>(kQueueSize, true);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    std::vector<Event> events;
    for (size_t i = 0; i < kNumEvents; i++) {
        events.push_back(makeAccelerometerEvent());
    }
    subHal.postEvents(events, false /* wakeup */);

    EXPECT_EQ(eventQueue->availableToRead(), kNumEvents);
}

TEST(HalProxyTest, PostSingleWakeupEvent) {
    constexpr size_t kQueueSize = 5;
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};
    HalProxy proxy(subHals);
    std::unique_ptr<EventMessageQueue> eventQueue =
            std::make_unique<EventMessageQueue>(kQueueSize, true);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue =
            std::make_unique<WakeupMessageQueue>(kQueueSize, true);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    std::vector<Event> events{makeProximityEvent()};
    subHal.postEvents(events, true /* wakeup */);

    EXPECT_EQ(eventQueue->availableToRead(), 1);
}

TEST(HalProxyTest, PostMultipleWakeupEvents) {
    constexpr size_t kQueueSize = 5;
    constexpr size_t kNumEvents = 3;
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};
    HalProxy proxy(subHals);
    std::unique_ptr<EventMessageQueue> eventQueue =
            std::make_unique<EventMessageQueue>(kQueueSize, true);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue =
            std::make_unique<WakeupMessageQueue>(kQueueSize, true);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    std::vector<Event> events;
    for (size_t i = 0; i < kNumEvents; i++) {
        events.push_back(makeProximityEvent());
    }
    subHal.postEvents(events, true /* wakeup */);

    EXPECT_EQ(eventQueue->availableToRead(), kNumEvents);
}

// Helper implementations follow
void testSensorsListFromProxyAndSubHal(const std::vector<SensorInfo>& proxySensorsList,
                                       const std::vector<SensorInfo>& subHalSensorsList) {
    EXPECT_EQ(proxySensorsList.size(), subHalSensorsList.size());

    for (size_t i = 0; i < proxySensorsList.size(); i++) {
        const SensorInfo& proxySensor = proxySensorsList[i];
        const SensorInfo& subHalSensor = subHalSensorsList[i];

        EXPECT_EQ(proxySensor.type, subHalSensor.type);
        EXPECT_EQ(proxySensor.sensorHandle & 0x00FFFFFF, subHalSensor.sensorHandle);
    }
}

void testSensorsListForOneDirectChannelEnabledSubHal(const std::vector<SensorInfo>& sensorsList,
                                                     size_t enabledSubHalIndex) {
    for (const SensorInfo& sensor : sensorsList) {
        size_t subHalIndex = static_cast<size_t>(sensor.sensorHandle >> 24);
        if (subHalIndex == enabledSubHalIndex) {
            // First subhal should have been picked as the direct channel subhal
            // and so have direct channel enabled on all of its sensors
            EXPECT_NE(sensor.flags & SensorFlagBits::MASK_DIRECT_REPORT, 0);
            EXPECT_NE(sensor.flags & SensorFlagBits::MASK_DIRECT_CHANNEL, 0);
        } else {
            // All other subhals should have direct channel disabled for all sensors
            EXPECT_EQ(sensor.flags & SensorFlagBits::MASK_DIRECT_REPORT, 0);
            EXPECT_EQ(sensor.flags & SensorFlagBits::MASK_DIRECT_CHANNEL, 0);
        }
    }
}

Event makeProximityEvent() {
    Event event;
    event.timestamp = 0xFF00FF00;
    // This is the sensorhandle of proximity, which is wakeup type
    event.sensorHandle = 0x00000008;
    event.sensorType = SensorType::PROXIMITY;
    event.u = EventPayload();
    return event;
}

Event makeAccelerometerEvent() {
    Event event;
    event.timestamp = 0xFF00FF00;
    // This is the sensorhandle of proximity, which is wakeup type
    event.sensorHandle = 0x00000001;
    event.sensorType = SensorType::ACCELEROMETER;
    event.u = EventPayload();
    return event;
}

}  // namespace
