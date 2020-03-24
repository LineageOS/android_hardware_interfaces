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
#include "V2_0/ScopedWakelock.h"

#include <chrono>
#include <set>
#include <thread>
#include <vector>

namespace {

using ::android::hardware::EventFlag;
using ::android::hardware::hidl_vec;
using ::android::hardware::MessageQueue;
using ::android::hardware::Return;
using ::android::hardware::sensors::V1_0::EventPayload;
using ::android::hardware::sensors::V1_0::SensorFlagBits;
using ::android::hardware::sensors::V1_0::SensorInfo;
using ::android::hardware::sensors::V1_0::SensorType;
using ::android::hardware::sensors::V2_0::EventQueueFlagBits;
using ::android::hardware::sensors::V2_0::ISensorsCallback;
using ::android::hardware::sensors::V2_0::WakeLockQueueFlagBits;
using ::android::hardware::sensors::V2_0::implementation::HalProxy;
using ::android::hardware::sensors::V2_0::implementation::HalProxyCallback;
using ::android::hardware::sensors::V2_0::subhal::implementation::AddAndRemoveDynamicSensorsSubHal;
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

// The sensors callback that expects a variable list of sensors to be added
class TestSensorsCallback : public ISensorsCallback {
  public:
    Return<void> onDynamicSensorsConnected(
            const hidl_vec<SensorInfo>& dynamicSensorsAdded) override {
        mSensorsConnected.insert(mSensorsConnected.end(), dynamicSensorsAdded.begin(),
                                 dynamicSensorsAdded.end());
        return Return<void>();
    }

    Return<void> onDynamicSensorsDisconnected(
            const hidl_vec<int32_t>& dynamicSensorHandlesRemoved) override {
        mSensorHandlesDisconnected.insert(mSensorHandlesDisconnected.end(),
                                          dynamicSensorHandlesRemoved.begin(),
                                          dynamicSensorHandlesRemoved.end());
        return Return<void>();
    }

    const std::vector<SensorInfo>& getSensorsConnected() const { return mSensorsConnected; }
    const std::vector<int32_t>& getSensorHandlesDisconnected() const {
        return mSensorHandlesDisconnected;
    }

  private:
    std::vector<SensorInfo> mSensorsConnected;
    std::vector<int32_t> mSensorHandlesDisconnected;
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

void ackWakeupEventsToHalProxy(size_t numEvents, std::unique_ptr<WakeupMessageQueue>& wakelockQueue,
                               EventFlag* wakelockQueueFlag);

bool readEventsOutOfQueue(size_t numEvents, std::unique_ptr<EventMessageQueue>& eventQueue,
                          EventFlag* eventQueueFlag);

std::unique_ptr<EventMessageQueue> makeEventFMQ(size_t size);

std::unique_ptr<WakeupMessageQueue> makeWakelockFMQ(size_t size);

/**
 * Construct and return a HIDL Event type thats sensorHandle refers to a proximity sensor
 *    which is a wakeup type sensor.
 *
 * @return A proximity event.
 */
Event makeProximityEvent();

/**
 * Construct and return a HIDL Event type thats sensorHandle refers to a proximity sensor
 *    which is a wakeup type sensor.
 *
 * @return A proximity event.
 */
Event makeAccelerometerEvent();

/**
 * Make a certain number of proximity type events with the sensorHandle field set to
 * the proper number for AllSensorsSubHal subhal type.
 *
 * @param numEvents The number of events to make.
 *
 * @return The created list of events.
 */
std::vector<Event> makeMultipleProximityEvents(size_t numEvents);

/**
 * Make a certain number of accelerometer type events with the sensorHandle field set to
 * the proper number for AllSensorsSubHal subhal type.
 *
 * @param numEvents The number of events to make.
 *
 * @return The created list of events.
 */
std::vector<Event> makeMultipleAccelerometerEvents(size_t numEvents);

/**
 * Given a SensorInfo vector and a sensor handles vector populate 'sensors' with SensorInfo
 * objects that have the sensorHandle property set to int32_ts from start to start + size
 * (exclusive) and push those sensorHandles also onto 'sensorHandles'.
 *
 * @param start The starting sensorHandle value.
 * @param size The ending (not included) sensorHandle value.
 * @param sensors The SensorInfo object vector reference to push_back to.
 * @param sensorHandles The sensor handles int32_t vector reference to push_back to.
 */
void makeSensorsAndSensorHandlesStartingAndOfSize(int32_t start, size_t size,
                                                  std::vector<SensorInfo>& sensors,
                                                  std::vector<int32_t>& sensorHandles);

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
    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
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
    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    std::vector<Event> events = makeMultipleAccelerometerEvents(kNumEvents);
    subHal.postEvents(events, false /* wakeup */);

    EXPECT_EQ(eventQueue->availableToRead(), kNumEvents);
}

TEST(HalProxyTest, PostSingleWakeupEvent) {
    constexpr size_t kQueueSize = 5;
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};
    HalProxy proxy(subHals);
    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    EventFlag* eventQueueFlag;
    EventFlag::createEventFlag(eventQueue->getEventFlagWord(), &eventQueueFlag);

    EventFlag* wakelockQueueFlag;
    EventFlag::createEventFlag(wakeLockQueue->getEventFlagWord(), &wakelockQueueFlag);

    std::vector<Event> events{makeProximityEvent()};
    subHal.postEvents(events, true /* wakeup */);

    EXPECT_EQ(eventQueue->availableToRead(), 1);

    readEventsOutOfQueue(1, eventQueue, eventQueueFlag);
    ackWakeupEventsToHalProxy(1, wakeLockQueue, wakelockQueueFlag);
}

TEST(HalProxyTest, PostMultipleWakeupEvents) {
    constexpr size_t kQueueSize = 5;
    constexpr size_t kNumEvents = 3;
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};
    HalProxy proxy(subHals);
    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    EventFlag* eventQueueFlag;
    EventFlag::createEventFlag(eventQueue->getEventFlagWord(), &eventQueueFlag);

    EventFlag* wakelockQueueFlag;
    EventFlag::createEventFlag(wakeLockQueue->getEventFlagWord(), &wakelockQueueFlag);

    std::vector<Event> events = makeMultipleProximityEvents(kNumEvents);
    subHal.postEvents(events, true /* wakeup */);

    EXPECT_EQ(eventQueue->availableToRead(), kNumEvents);

    readEventsOutOfQueue(kNumEvents, eventQueue, eventQueueFlag);
    ackWakeupEventsToHalProxy(kNumEvents, wakeLockQueue, wakelockQueueFlag);
}

TEST(HalProxyTest, PostEventsMultipleSubhals) {
    constexpr size_t kQueueSize = 5;
    constexpr size_t kNumEvents = 2;
    AllSensorsSubHal subHal1, subHal2;
    std::vector<ISensorsSubHal*> subHals{&subHal1, &subHal2};
    HalProxy proxy(subHals);
    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    std::vector<Event> events = makeMultipleAccelerometerEvents(kNumEvents);
    subHal1.postEvents(events, false /* wakeup */);

    EXPECT_EQ(eventQueue->availableToRead(), kNumEvents);

    subHal2.postEvents(events, false /* wakeup */);

    EXPECT_EQ(eventQueue->availableToRead(), kNumEvents * 2);
}

TEST(HalProxyTest, PostEventsDelayedWrite) {
    constexpr size_t kQueueSize = 5;
    constexpr size_t kNumEvents = 6;
    AllSensorsSubHal subHal1, subHal2;
    std::vector<ISensorsSubHal*> subHals{&subHal1, &subHal2};
    HalProxy proxy(subHals);
    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    EventFlag* eventQueueFlag;
    EventFlag::createEventFlag(eventQueue->getEventFlagWord(), &eventQueueFlag);

    std::vector<Event> events = makeMultipleAccelerometerEvents(kNumEvents);
    subHal1.postEvents(events, false /* wakeup */);

    EXPECT_EQ(eventQueue->availableToRead(), kQueueSize);

    // readblock a full queue size worth of events out of queue, timeout for half a second
    EXPECT_TRUE(readEventsOutOfQueue(kQueueSize, eventQueue, eventQueueFlag));

    // proxy background thread should have wrote remaining events when it saw space
    EXPECT_TRUE(readEventsOutOfQueue(kNumEvents - kQueueSize, eventQueue, eventQueueFlag));

    EXPECT_EQ(eventQueue->availableToRead(), 0);
}

TEST(HalProxyTest, PostEventsMultipleSubhalsThreaded) {
    constexpr size_t kQueueSize = 5;
    constexpr size_t kNumEvents = 2;
    AllSensorsSubHal subHal1, subHal2;
    std::vector<ISensorsSubHal*> subHals{&subHal1, &subHal2};
    HalProxy proxy(subHals);
    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    std::vector<Event> events = makeMultipleAccelerometerEvents(kNumEvents);

    std::thread t1(&AllSensorsSubHal::postEvents, &subHal1, events, false);
    std::thread t2(&AllSensorsSubHal::postEvents, &subHal2, events, false);

    t1.join();
    t2.join();

    EXPECT_EQ(eventQueue->availableToRead(), kNumEvents * 2);
}

TEST(HalProxyTest, DestructingWithEventsPendingOnBackgroundThread) {
    constexpr size_t kQueueSize = 5;
    constexpr size_t kNumEvents = 6;
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};

    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    HalProxy proxy(subHals);
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    std::vector<Event> events = makeMultipleAccelerometerEvents(kNumEvents);
    subHal.postEvents(events, false /* wakeup */);

    // Destructing HalProxy object with events on the background thread
}

TEST(HalProxyTest, DestructingWithUnackedWakeupEventsPosted) {
    constexpr size_t kQueueSize = 5;
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};

    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    HalProxy proxy(subHals);
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    std::vector<Event> events{makeProximityEvent()};
    subHal.postEvents(events, true /* wakeup */);

    // Not sending any acks back through wakeLockQueue

    // Destructing HalProxy object with unacked wakeup events posted
}

TEST(HalProxyTest, ReinitializeWithEventsPendingOnBackgroundThread) {
    constexpr size_t kQueueSize = 5;
    constexpr size_t kNumEvents = 10;
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};

    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    HalProxy proxy(subHals);
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    std::vector<Event> events = makeMultipleAccelerometerEvents(kNumEvents);
    subHal.postEvents(events, false /* wakeup */);

    eventQueue = makeEventFMQ(kQueueSize);
    wakeLockQueue = makeWakelockFMQ(kQueueSize);

    Result secondInitResult =
            proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);
    EXPECT_EQ(secondInitResult, Result::OK);
    // Small sleep so that pending writes thread has a change to hit writeBlocking call.
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    Event eventOut;
    EXPECT_FALSE(eventQueue->read(&eventOut));
}

TEST(HalProxyTest, ReinitializingWithUnackedWakeupEventsPosted) {
    constexpr size_t kQueueSize = 5;
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};

    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    HalProxy proxy(subHals);
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    std::vector<Event> events{makeProximityEvent()};
    subHal.postEvents(events, true /* wakeup */);

    // Not sending any acks back through wakeLockQueue

    eventQueue = makeEventFMQ(kQueueSize);
    wakeLockQueue = makeWakelockFMQ(kQueueSize);

    Result secondInitResult =
            proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);
    EXPECT_EQ(secondInitResult, Result::OK);
}

TEST(HalProxyTest, InitializeManyTimesInARow) {
    constexpr size_t kQueueSize = 5;
    constexpr size_t kNumTimesToInit = 100;
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};

    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    HalProxy proxy(subHals);

    for (size_t i = 0; i < kNumTimesToInit; i++) {
        Result secondInitResult =
                proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);
        EXPECT_EQ(secondInitResult, Result::OK);
    }
}

TEST(HalProxyTest, OperationModeResetOnInitialize) {
    constexpr size_t kQueueSize = 5;
    AllSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};
    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    HalProxy proxy(subHals);
    proxy.setOperationMode(OperationMode::DATA_INJECTION);
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);
    Event event = makeAccelerometerEvent();
    // Should not be able to inject a non AdditionInfo type event because operation mode should
    // have been reset to NORMAL
    EXPECT_EQ(proxy.injectSensorData(event), Result::BAD_VALUE);
}

TEST(HalProxyTest, DynamicSensorsDiscardedOnInitialize) {
    constexpr size_t kQueueSize = 5;
    constexpr size_t kNumSensors = 5;
    AddAndRemoveDynamicSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};
    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    HalProxy proxy(subHals);

    std::vector<SensorInfo> sensorsToConnect;
    std::vector<int32_t> sensorHandlesToAttemptToRemove;
    makeSensorsAndSensorHandlesStartingAndOfSize(1, kNumSensors, sensorsToConnect,
                                                 sensorHandlesToAttemptToRemove);

    std::vector<int32_t> nonDynamicSensorHandles;
    for (int32_t sensorHandle = 1; sensorHandle < 10; sensorHandle++) {
        nonDynamicSensorHandles.push_back(sensorHandle);
    }

    TestSensorsCallback* callback = new TestSensorsCallback();
    ::android::sp<ISensorsCallback> callbackPtr = callback;
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callbackPtr);
    subHal.addDynamicSensors(sensorsToConnect);

    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callbackPtr);
    subHal.removeDynamicSensors(sensorHandlesToAttemptToRemove);

    std::vector<int32_t> sensorHandlesActuallyRemoved = callback->getSensorHandlesDisconnected();

    // Should not have received the sensorHandles for any dynamic sensors that were removed since
    // all of them should have been removed in the second initialize call.
    EXPECT_TRUE(sensorHandlesActuallyRemoved.empty());
}

TEST(HalProxyTest, DynamicSensorsConnectedTest) {
    constexpr size_t kNumSensors = 3;
    AddAndRemoveDynamicSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};
    HalProxy proxy(subHals);
    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(0);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(0);

    std::vector<SensorInfo> sensorsToConnect;
    std::vector<int32_t> sensorHandlesToExpect;
    makeSensorsAndSensorHandlesStartingAndOfSize(1, kNumSensors, sensorsToConnect,
                                                 sensorHandlesToExpect);

    TestSensorsCallback* callback = new TestSensorsCallback();
    ::android::sp<ISensorsCallback> callbackPtr = callback;
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callbackPtr);
    subHal.addDynamicSensors(sensorsToConnect);

    std::vector<SensorInfo> sensorsSeen = callback->getSensorsConnected();
    EXPECT_EQ(kNumSensors, sensorsSeen.size());
    for (size_t i = 0; i < kNumSensors; i++) {
        auto sensorHandleSeen = sensorsSeen[i].sensorHandle;
        // Note since only one subhal we do not need to change first byte for expected
        auto sensorHandleExpected = sensorHandlesToExpect[i];
        EXPECT_EQ(sensorHandleSeen, sensorHandleExpected);
    }
}

TEST(HalProxyTest, DynamicSensorsDisconnectedTest) {
    constexpr size_t kNumSensors = 3;
    AddAndRemoveDynamicSensorsSubHal subHal;
    std::vector<ISensorsSubHal*> subHals{&subHal};
    HalProxy proxy(subHals);
    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(0);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(0);

    std::vector<SensorInfo> sensorsToConnect;
    std::vector<int32_t> sensorHandlesToExpect;
    makeSensorsAndSensorHandlesStartingAndOfSize(20, kNumSensors, sensorsToConnect,
                                                 sensorHandlesToExpect);

    std::vector<int32_t> nonDynamicSensorHandles;
    for (int32_t sensorHandle = 1; sensorHandle < 10; sensorHandle++) {
        nonDynamicSensorHandles.push_back(sensorHandle);
    }

    std::set<int32_t> nonDynamicSensorHandlesSet(nonDynamicSensorHandles.begin(),
                                                 nonDynamicSensorHandles.end());

    std::vector<int32_t> sensorHandlesToAttemptToRemove;
    sensorHandlesToAttemptToRemove.insert(sensorHandlesToAttemptToRemove.end(),
                                          sensorHandlesToExpect.begin(),
                                          sensorHandlesToExpect.end());
    sensorHandlesToAttemptToRemove.insert(sensorHandlesToAttemptToRemove.end(),
                                          nonDynamicSensorHandles.begin(),
                                          nonDynamicSensorHandles.end());

    TestSensorsCallback* callback = new TestSensorsCallback();
    ::android::sp<ISensorsCallback> callbackPtr = callback;
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callbackPtr);
    subHal.addDynamicSensors(sensorsToConnect);
    subHal.removeDynamicSensors(sensorHandlesToAttemptToRemove);

    std::vector<int32_t> sensorHandlesSeen = callback->getSensorHandlesDisconnected();
    EXPECT_EQ(kNumSensors, sensorHandlesSeen.size());
    for (size_t i = 0; i < kNumSensors; i++) {
        auto sensorHandleSeen = sensorHandlesSeen[i];
        // Note since only one subhal we do not need to change first byte for expected
        auto sensorHandleExpected = sensorHandlesToExpect[i];
        EXPECT_EQ(sensorHandleSeen, sensorHandleExpected);
        EXPECT_TRUE(nonDynamicSensorHandlesSet.find(sensorHandleSeen) ==
                    nonDynamicSensorHandlesSet.end());
    }
}

TEST(HalProxyTest, InvalidSensorHandleSubHalIndexProxyCalls) {
    constexpr size_t kNumSubHals = 3;
    constexpr size_t kQueueSize = 5;
    int32_t kNumSubHalsInt32 = static_cast<int32_t>(kNumSubHals);
    std::vector<AllSensorsSubHal> subHalObjs(kNumSubHals);
    std::vector<ISensorsSubHal*> subHals;
    for (const auto& subHal : subHalObjs) {
        subHals.push_back((ISensorsSubHal*)(&subHal));
    }

    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    HalProxy proxy(subHals);
    // Initialize for the injectSensorData call so callback postEvents is valid
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    // For testing proxy.injectSensorData properly
    proxy.setOperationMode(OperationMode::DATA_INJECTION);

    // kNumSubHalsInt32 index is one off the end of mSubHalList in proxy object
    EXPECT_EQ(proxy.activate(0x00000001 | (kNumSubHalsInt32 << 24), true), Result::BAD_VALUE);
    EXPECT_EQ(proxy.batch(0x00000001 | (kNumSubHalsInt32 << 24), 0, 0), Result::BAD_VALUE);
    EXPECT_EQ(proxy.flush(0x00000001 | (kNumSubHalsInt32 << 24)), Result::BAD_VALUE);
    Event event;
    event.sensorHandle = 0x00000001 | (kNumSubHalsInt32 << 24);
    EXPECT_EQ(proxy.injectSensorData(event), Result::BAD_VALUE);
}

TEST(HalProxyTest, PostedEventSensorHandleSubHalIndexValid) {
    constexpr size_t kQueueSize = 5;
    constexpr int32_t subhal1Index = 0;
    constexpr int32_t subhal2Index = 1;
    AllSensorsSubHal subhal1;
    AllSensorsSubHal subhal2;
    std::vector<ISensorsSubHal*> subHals{&subhal1, &subhal2};

    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    HalProxy proxy(subHals);
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    int32_t sensorHandleToPost = 0x00000001;
    Event eventIn = makeAccelerometerEvent();
    eventIn.sensorHandle = sensorHandleToPost;
    std::vector<Event> eventsToPost{eventIn};
    subhal1.postEvents(eventsToPost, false);

    Event eventOut;
    EXPECT_TRUE(eventQueue->read(&eventOut));

    EXPECT_EQ(eventOut.sensorHandle, (subhal1Index << 24) | sensorHandleToPost);

    subhal2.postEvents(eventsToPost, false);

    EXPECT_TRUE(eventQueue->read(&eventOut));

    EXPECT_EQ(eventOut.sensorHandle, (subhal2Index << 24) | sensorHandleToPost);
}

TEST(HalProxyTest, FillAndDrainPendingQueueTest) {
    constexpr size_t kQueueSize = 5;
    // TODO: Make this constant linked to same limit in HalProxy.h
    constexpr size_t kMaxPendingQueueSize = 100000;
    AllSensorsSubHal subhal;
    std::vector<ISensorsSubHal*> subHals{&subhal};

    std::unique_ptr<EventMessageQueue> eventQueue = makeEventFMQ(kQueueSize);
    std::unique_ptr<WakeupMessageQueue> wakeLockQueue = makeWakelockFMQ(kQueueSize);
    ::android::sp<ISensorsCallback> callback = new SensorsCallback();
    EventFlag* eventQueueFlag;
    EventFlag::createEventFlag(eventQueue->getEventFlagWord(), &eventQueueFlag);
    HalProxy proxy(subHals);
    proxy.initialize(*eventQueue->getDesc(), *wakeLockQueue->getDesc(), callback);

    // Fill pending queue
    std::vector<Event> events = makeMultipleAccelerometerEvents(kQueueSize);
    subhal.postEvents(events, false);
    events = makeMultipleAccelerometerEvents(kMaxPendingQueueSize);
    subhal.postEvents(events, false);

    // Drain pending queue
    for (int i = 0; i < kMaxPendingQueueSize + kQueueSize; i += kQueueSize) {
        ASSERT_TRUE(readEventsOutOfQueue(kQueueSize, eventQueue, eventQueueFlag));
    }

    // Put one event on pending queue
    events = makeMultipleAccelerometerEvents(kQueueSize);
    subhal.postEvents(events, false);
    events = {makeAccelerometerEvent()};
    subhal.postEvents(events, false);

    // Read out to make room for one event on pending queue to write to FMQ
    ASSERT_TRUE(readEventsOutOfQueue(kQueueSize, eventQueue, eventQueueFlag));

    // Should be able to read that last event off queue
    EXPECT_TRUE(readEventsOutOfQueue(1, eventQueue, eventQueueFlag));
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

void ackWakeupEventsToHalProxy(size_t numEvents, std::unique_ptr<WakeupMessageQueue>& wakelockQueue,
                               EventFlag* wakelockQueueFlag) {
    uint32_t numEventsUInt32 = static_cast<uint32_t>(numEvents);
    wakelockQueue->write(&numEventsUInt32);
    wakelockQueueFlag->wake(static_cast<uint32_t>(WakeLockQueueFlagBits::DATA_WRITTEN));
}

bool readEventsOutOfQueue(size_t numEvents, std::unique_ptr<EventMessageQueue>& eventQueue,
                          EventFlag* eventQueueFlag) {
    constexpr int64_t kReadBlockingTimeout = INT64_C(500000000);
    std::vector<Event> events(numEvents);
    return eventQueue->readBlocking(events.data(), numEvents,
                                    static_cast<uint32_t>(EventQueueFlagBits::EVENTS_READ),
                                    static_cast<uint32_t>(EventQueueFlagBits::READ_AND_PROCESS),
                                    kReadBlockingTimeout, eventQueueFlag);
}

std::unique_ptr<EventMessageQueue> makeEventFMQ(size_t size) {
    return std::make_unique<EventMessageQueue>(size, true);
}

std::unique_ptr<WakeupMessageQueue> makeWakelockFMQ(size_t size) {
    return std::make_unique<WakeupMessageQueue>(size, true);
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

std::vector<Event> makeMultipleProximityEvents(size_t numEvents) {
    std::vector<Event> events;
    for (size_t i = 0; i < numEvents; i++) {
        events.push_back(makeProximityEvent());
    }
    return events;
}

std::vector<Event> makeMultipleAccelerometerEvents(size_t numEvents) {
    std::vector<Event> events;
    for (size_t i = 0; i < numEvents; i++) {
        events.push_back(makeAccelerometerEvent());
    }
    return events;
}

void makeSensorsAndSensorHandlesStartingAndOfSize(int32_t start, size_t size,
                                                  std::vector<SensorInfo>& sensors,
                                                  std::vector<int32_t>& sensorHandles) {
    for (int32_t sensorHandle = start; sensorHandle < start + static_cast<int32_t>(size);
         sensorHandle++) {
        SensorInfo sensor;
        // Just set the sensorHandle field to the correct value so as to not have
        // to compare every field
        sensor.sensorHandle = sensorHandle;
        sensors.push_back(sensor);
        sensorHandles.push_back(sensorHandle);
    }
}

}  // namespace
