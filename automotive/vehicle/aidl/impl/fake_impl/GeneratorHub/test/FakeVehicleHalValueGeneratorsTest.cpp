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

#include <GeneratorHub.h>
#include <VehicleUtils.h>
#include <android-base/file.h>
#include <android-base/thread_annotations.h>
#include <gtest/gtest.h>
#include <utils/SystemClock.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {

using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;

class FakeVehicleHalValueGeneratorsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mHub = std::make_unique<GeneratorHub>(
                [this](const VehiclePropValue& event) { return onHalEvent(event); });
    }

    GeneratorHub* getHub() { return mHub.get(); }

    std::vector<VehiclePropValue> getEvents() {
        std::scoped_lock<std::mutex> lockGuard(mEventsLock);
        return mEvents;
    }

    void clearEvents() {
        std::scoped_lock<std::mutex> lockGuard(mEventsLock);
        mEvents.clear();
    }

    void TearDown() override {
        // Generator callback uses mEvents, must stop generator before destroying mEvents.
        mHub.reset();
    }

  private:
    void onHalEvent(const VehiclePropValue& event) {
        VehiclePropValue eventCopy = event;
        std::scoped_lock<std::mutex> lockGuard(mEventsLock);
        mEvents.push_back(std::move(eventCopy));
    }

    std::unique_ptr<GeneratorHub> mHub;
    std::mutex mEventsLock;
    std::vector<VehiclePropValue> mEvents GUARDED_BY(mEventsLock);
};

class TestFakeValueGenerator : public FakeValueGenerator {
  public:
    void setEvents(const std::vector<VehiclePropValue>& events) {
        mEvents = events;
        mEventIndex = 0;
    }

    std::optional<::aidl::android::hardware::automotive::vehicle::VehiclePropValue> nextEvent()
            override {
        if (mEventIndex == mEvents.size()) {
            return std::nullopt;
        }
        return mEvents[mEventIndex++];
    }

  private:
    std::vector<VehiclePropValue> mEvents;
    size_t mEventIndex = 0;
};

TEST_F(FakeVehicleHalValueGeneratorsTest, testRegisterTestFakeValueGenerator) {
    auto generator = std::make_unique<TestFakeValueGenerator>();
    std::vector<VehiclePropValue> events;
    size_t eventCount = 10;
    int64_t timestamp = elapsedRealtimeNano();
    for (size_t i = 0; i < eventCount; i++) {
        events.push_back(VehiclePropValue{
                .prop = static_cast<int32_t>(i),
                .timestamp = timestamp + static_cast<int64_t>(50 * i),
        });
    }
    generator->setEvents(events);

    getHub()->registerGenerator(0, std::move(generator));

    // All the events require 500ms to generate, so waiting for 1000ms should be enough.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    ASSERT_EQ(getEvents(), events);

    getHub()->unregisterGenerator(0);
}

TEST_F(FakeVehicleHalValueGeneratorsTest, testUnregisterGeneratorStopGeneration) {
    auto generator = std::make_unique<TestFakeValueGenerator>();
    std::vector<VehiclePropValue> events;
    size_t eventCount = 10;
    int64_t timestamp = elapsedRealtimeNano();
    for (size_t i = 0; i < eventCount; i++) {
        events.push_back(VehiclePropValue{
                .prop = static_cast<int32_t>(i),
                .timestamp = timestamp + static_cast<int64_t>(50 * i),
        });
    }
    generator->setEvents(events);

    getHub()->registerGenerator(0, std::move(generator));
    getHub()->unregisterGenerator(0);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    ASSERT_LT(getEvents().size(), static_cast<size_t>(10))
            << "Must stop generating event after generator is unregistered";
}

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
