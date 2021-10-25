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
#include <JsonFakeValueGenerator.h>
#include <LinearFakeValueGenerator.h>
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

    static std::string getTestFilePath(const char* filename) {
        static std::string baseDir = android::base::GetExecutableDirectory();
        return baseDir + "/" + filename;
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

TEST_F(FakeVehicleHalValueGeneratorsTest, testLinerFakeValueGeneratorFloat) {
    std::unique_ptr<LinearFakeValueGenerator> generator =
            std::make_unique<LinearFakeValueGenerator>(toInt(VehicleProperty::PERF_VEHICLE_SPEED),
                                                       /*middleValue=*/50.0,
                                                       /*initValue=*/30.0,
                                                       /*dispersion=*/50.0,
                                                       /*increment=*/20.0,
                                                       /*interval=*/10000000);
    getHub()->registerGenerator(0, std::move(generator));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto events = getEvents();
    // We should get 10 events ideally, but let's be safe here.
    ASSERT_LE((size_t)5, events.size());
    int value = 30;
    for (size_t i = 0; i < 5; i++) {
        EXPECT_EQ(std::vector<float>({static_cast<float>(value)}), events[i].value.floatValues);
        value = (value + 20) % 100;
    }
}

TEST_F(FakeVehicleHalValueGeneratorsTest, testLinerFakeValueGeneratorInt32) {
    std::unique_ptr<LinearFakeValueGenerator> generator =
            std::make_unique<LinearFakeValueGenerator>(toInt(VehicleProperty::INFO_MODEL_YEAR),
                                                       /*middleValue=*/50.0,
                                                       /*initValue=*/30.0,
                                                       /*dispersion=*/50.0,
                                                       /*increment=*/20.0,
                                                       /*interval=*/10000000);
    getHub()->registerGenerator(0, std::move(generator));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto events = getEvents();
    // We should get 10 events ideally, but let's be safe here.
    ASSERT_LE((size_t)5, events.size());
    int value = 30;
    for (size_t i = 0; i < 5; i++) {
        EXPECT_EQ(std::vector<int32_t>({value}), events[i].value.int32Values);
        value = (value + 20) % 100;
    }
}

TEST_F(FakeVehicleHalValueGeneratorsTest, testLinerFakeValueGeneratorInt64) {
    std::unique_ptr<LinearFakeValueGenerator> generator =
            std::make_unique<LinearFakeValueGenerator>(toInt(VehicleProperty::ANDROID_EPOCH_TIME),
                                                       /*middleValue=*/50.0,
                                                       /*initValue=*/30.0,
                                                       /*dispersion=*/50.0,
                                                       /*increment=*/20.0,
                                                       /*interval=*/10000000);
    getHub()->registerGenerator(0, std::move(generator));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto events = getEvents();
    // We should get 10 events ideally, but let's be safe here.
    ASSERT_LE((size_t)5, events.size());
    int value = 30;
    for (size_t i = 0; i < 5; i++) {
        EXPECT_EQ(std::vector<int64_t>({value}), events[i].value.int64Values);
        value = (value + 20) % 100;
    }
}

TEST_F(FakeVehicleHalValueGeneratorsTest, testLinerFakeValueGeneratorUsingRequest) {
    VehiclePropValue request;
    request.value.int32Values = {0, toInt(VehicleProperty::PERF_VEHICLE_SPEED)};
    request.value.floatValues = {/*middleValue=*/50.0, /*dispersion=*/50.0, /*increment=*/20.0};
    request.value.int64Values = {/*interval=*/10000000};

    std::unique_ptr<LinearFakeValueGenerator> generator =
            std::make_unique<LinearFakeValueGenerator>(request);
    getHub()->registerGenerator(0, std::move(generator));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto events = getEvents();
    // We should get 10 events ideally, but let's be safe here.
    ASSERT_LE((size_t)5, events.size());
    int value = 50;
    for (size_t i = 0; i < 5; i++) {
        EXPECT_EQ(std::vector<float>({static_cast<float>(value)}), events[i].value.floatValues);
        value = (value + 20) % 100;
    }
}

TEST_F(FakeVehicleHalValueGeneratorsTest, testLinerFakeValueGeneratorInvalidInitValue) {
    std::unique_ptr<LinearFakeValueGenerator> generator =
            std::make_unique<LinearFakeValueGenerator>(toInt(VehicleProperty::PERF_VEHICLE_SPEED),
                                                       /*middleValue=*/50.0,
                                                       // Out of range
                                                       /*initValue=*/110.0,
                                                       /*dispersion=*/50.0,
                                                       /*increment=*/20.0,
                                                       /*interval=*/10000000);
    getHub()->registerGenerator(0, std::move(generator));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto events = getEvents();
    // We should get 10 events ideally, but let's be safe here.
    ASSERT_LE((size_t)5, events.size());

    // Init value would be set to middleValue if given initValue is not valid.
    int value = 50;
    for (size_t i = 0; i < 5; i++) {
        EXPECT_EQ(std::vector<float>({static_cast<float>(value)}), events[i].value.floatValues);
        value = (value + 20) % 100;
    }
}

TEST_F(FakeVehicleHalValueGeneratorsTest, testJsonFakeValueGenerator) {
    long currentTime = elapsedRealtimeNano();

    std::unique_ptr<JsonFakeValueGenerator> generator =
            std::make_unique<JsonFakeValueGenerator>(getTestFilePath("prop.json"), 2);
    getHub()->registerGenerator(0, std::move(generator));

    // wait for some time.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<VehiclePropValue> expectedValues = {
            VehiclePropValue{
                    .areaId = 0,
                    .value.int32Values = {8},
                    .prop = 289408000,
            },
            VehiclePropValue{
                    .areaId = 0,
                    .value.int32Values = {4},
                    .prop = 289408000,
            },
            VehiclePropValue{
                    .areaId = 0,
                    .value.int32Values = {16},
                    .prop = 289408000,
            },
            VehiclePropValue{
                    .areaId = 0,
                    .value.int32Values = {10},
                    .prop = 289408000,
            },
    };

    // We have two iterations.
    for (size_t i = 0; i < 4; i++) {
        expectedValues.push_back(expectedValues[i]);
    }

    auto events = getEvents();

    long lastEventTime = currentTime;
    for (auto& event : events) {
        EXPECT_GT(event.timestamp, lastEventTime);
        lastEventTime = event.timestamp;
        event.timestamp = 0;
    }

    EXPECT_EQ(events, expectedValues);
}

TEST_F(FakeVehicleHalValueGeneratorsTest, testJsonFakeValueGeneratorIterateIndefinitely) {
    std::unique_ptr<JsonFakeValueGenerator> generator =
            std::make_unique<JsonFakeValueGenerator>(getTestFilePath("prop.json"), -1);
    getHub()->registerGenerator(0, std::move(generator));

    // wait for some time.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto events = getEvents();

    // Send 1 iteration takes 4ms + 1ms interval between iteration, so for 100ms we should get about
    // 20 iteration, which is 80 events.
    EXPECT_GT(events.size(), static_cast<size_t>(50));
}

TEST_F(FakeVehicleHalValueGeneratorsTest, testJsonFakeValueGeneratorUsingRequest) {
    long currentTime = elapsedRealtimeNano();

    VehiclePropValue request = {.value = {
                                        .stringValue = getTestFilePath("prop.json"),
                                        .int32Values = {0, 2},
                                }};

    std::unique_ptr<JsonFakeValueGenerator> generator =
            std::make_unique<JsonFakeValueGenerator>(request);
    getHub()->registerGenerator(0, std::move(generator));

    // wait for some time.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<VehiclePropValue> expectedValues = {
            VehiclePropValue{
                    .areaId = 0,
                    .value.int32Values = {8},
                    .prop = 289408000,
            },
            VehiclePropValue{
                    .areaId = 0,
                    .value.int32Values = {4},
                    .prop = 289408000,
            },
            VehiclePropValue{
                    .areaId = 0,
                    .value.int32Values = {16},
                    .prop = 289408000,
            },
            VehiclePropValue{
                    .areaId = 0,
                    .value.int32Values = {10},
                    .prop = 289408000,
            },
    };

    // We have two iterations.
    for (size_t i = 0; i < 4; i++) {
        expectedValues.push_back(expectedValues[i]);
    }

    auto events = getEvents();

    long lastEventTime = currentTime;
    for (auto& event : events) {
        EXPECT_GT(event.timestamp, lastEventTime);
        lastEventTime = event.timestamp;
        event.timestamp = 0;
    }

    EXPECT_EQ(events, expectedValues);
}

TEST_F(FakeVehicleHalValueGeneratorsTest, testJsonFakeValueGeneratorInvalidFile) {
    VehiclePropValue request = {.value = {
                                        .stringValue = getTestFilePath("prop_invalid.json"),
                                        .int32Values = {0, 2},
                                }};

    std::unique_ptr<JsonFakeValueGenerator> generator =
            std::make_unique<JsonFakeValueGenerator>(request);
    getHub()->registerGenerator(0, std::move(generator));

    // wait for some time.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_TRUE(getEvents().empty());
}

TEST_F(FakeVehicleHalValueGeneratorsTest, testJsonFakeValueGeneratorNonExistingFile) {
    VehiclePropValue request = {.value = {
                                        .stringValue = "non_existing_file",
                                        .int32Values = {0, 2},
                                }};

    std::unique_ptr<JsonFakeValueGenerator> generator =
            std::make_unique<JsonFakeValueGenerator>(request);
    getHub()->registerGenerator(0, std::move(generator));

    // wait for some time.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_TRUE(getEvents().empty());
}

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
