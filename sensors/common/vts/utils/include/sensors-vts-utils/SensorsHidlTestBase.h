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

#ifndef ANDROID_SENSORS_HIDL_TEST_BASE_H
#define ANDROID_SENSORS_HIDL_TEST_BASE_H

#include "sensors-vts-utils/SensorEventsChecker.h"
#include "sensors-vts-utils/SensorsHidlEnvironmentBase.h"

#include <VtsHalHidlTargetTestBase.h>
#include <android/hardware/sensors/1.0/ISensors.h>
#include <android/hardware/sensors/1.0/types.h>

#include <unordered_set>
#include <vector>

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::sensors::V1_0::Event;
using ::android::hardware::sensors::V1_0::ISensors;
using ::android::hardware::sensors::V1_0::RateLevel;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SensorFlagBits;
using ::android::hardware::sensors::V1_0::SensorInfo;
using ::android::hardware::sensors::V1_0::SensorType;
using ::android::hardware::sensors::V1_0::SharedMemInfo;
using ::android::hardware::sensors::V1_0::SharedMemType;

class SensorsHidlTestBase : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual SensorsHidlEnvironmentBase* getEnvironment() = 0;
    virtual void SetUp() override {}

    virtual void TearDown() override {
        // stop all sensors
        for (auto s : mSensorHandles) {
            activate(s, false);
        }
        mSensorHandles.clear();

        // stop all direct report and channels
        for (auto c : mDirectChannelHandles) {
            // disable all reports
            configDirectReport(-1, c, RateLevel::STOP, [](auto, auto) {});
            unregisterDirectChannel(c);
        }
        mDirectChannelHandles.clear();
    }

    // implementation wrapper
    virtual SensorInfo defaultSensorByType(SensorType type) = 0;
    virtual Return<void> getSensorsList(ISensors::getSensorsList_cb _hidl_cb) = 0;

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

    std::vector<Event> collectEvents(useconds_t timeLimitUs, size_t nEventLimit,
                                     bool clearBeforeStart = true, bool changeCollection = true);

    inline static SensorFlagBits extractReportMode(uint64_t flag) {
        return (SensorFlagBits)(flag & ((uint64_t)SensorFlagBits::CONTINUOUS_MODE |
                                        (uint64_t)SensorFlagBits::ON_CHANGE_MODE |
                                        (uint64_t)SensorFlagBits::ONE_SHOT_MODE |
                                        (uint64_t)SensorFlagBits::SPECIAL_REPORTING_MODE));
    }

    inline static bool isMetaSensorType(SensorType type) {
        return (type == SensorType::META_DATA || type == SensorType::DYNAMIC_SENSOR_META ||
                type == SensorType::ADDITIONAL_INFO);
    }

    inline static bool isValidType(SensorType type) { return (int32_t)type > 0; }

    void testStreamingOperation(SensorType type, std::chrono::nanoseconds samplingPeriod,
                                std::chrono::seconds duration, const SensorEventsChecker& checker);
    void testSamplingRateHotSwitchOperation(SensorType type, bool fastToSlow = true);
    void testBatchingOperation(SensorType type);
    void testDirectReportOperation(SensorType type, SharedMemType memType, RateLevel rate,
                                   const SensorEventsChecker& checker);

    static void assertTypeMatchStringType(SensorType type, const hidl_string& stringType);
    static void assertTypeMatchReportMode(SensorType type, SensorFlagBits reportMode);
    static void assertDelayMatchReportMode(int32_t minDelay, int32_t maxDelay,
                                           SensorFlagBits reportMode);
    static SensorFlagBits expectedReportModeForType(SensorType type);
    static bool isDirectReportRateSupported(SensorInfo sensor, RateLevel rate);
    static bool isDirectChannelTypeSupported(SensorInfo sensor, SharedMemType type);

   protected:
    // checkers
    static const Vec3NormChecker sAccelNormChecker;
    static const Vec3NormChecker sGyroNormChecker;

    // all sensors and direct channnels used
    std::unordered_set<int32_t> mSensorHandles;
    std::unordered_set<int32_t> mDirectChannelHandles;
};

#endif  // ANDROID_SENSORS_HIDL_TEST_BASE_H