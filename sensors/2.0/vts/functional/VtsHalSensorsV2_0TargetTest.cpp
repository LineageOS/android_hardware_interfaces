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

#include "VtsHalSensorsV2_XTargetTest.h"

// Test if sensor hal can do UI speed accelerometer streaming properly
TEST_P(SensorsHidlTest, AccelerometerStreamingOperationSlow) {
    testStreamingOperation(SensorTypeVersion::ACCELEROMETER, std::chrono::milliseconds(200),
                           std::chrono::seconds(5), mAccelNormChecker);
}

// Test if sensor hal can do normal speed accelerometer streaming properly
TEST_P(SensorsHidlTest, AccelerometerStreamingOperationNormal) {
    testStreamingOperation(SensorTypeVersion::ACCELEROMETER, std::chrono::milliseconds(20),
                           std::chrono::seconds(5), mAccelNormChecker);
}

// Test if sensor hal can do game speed accelerometer streaming properly
TEST_P(SensorsHidlTest, AccelerometerStreamingOperationFast) {
    testStreamingOperation(SensorTypeVersion::ACCELEROMETER, std::chrono::milliseconds(5),
                           std::chrono::seconds(5), mAccelNormChecker);
}

// Test if sensor hal can do UI speed gyroscope streaming properly
TEST_P(SensorsHidlTest, GyroscopeStreamingOperationSlow) {
    testStreamingOperation(SensorTypeVersion::GYROSCOPE, std::chrono::milliseconds(200),
                           std::chrono::seconds(5), mGyroNormChecker);
}

// Test if sensor hal can do normal speed gyroscope streaming properly
TEST_P(SensorsHidlTest, GyroscopeStreamingOperationNormal) {
    testStreamingOperation(SensorTypeVersion::GYROSCOPE, std::chrono::milliseconds(20),
                           std::chrono::seconds(5), mGyroNormChecker);
}

// Test if sensor hal can do game speed gyroscope streaming properly
TEST_P(SensorsHidlTest, GyroscopeStreamingOperationFast) {
    testStreamingOperation(SensorTypeVersion::GYROSCOPE, std::chrono::milliseconds(5),
                           std::chrono::seconds(5), mGyroNormChecker);
}

// Test if sensor hal can do UI speed magnetometer streaming properly
TEST_P(SensorsHidlTest, MagnetometerStreamingOperationSlow) {
    testStreamingOperation(SensorTypeVersion::MAGNETIC_FIELD, std::chrono::milliseconds(200),
                           std::chrono::seconds(5), NullChecker<EventType>());
}

// Test if sensor hal can do normal speed magnetometer streaming properly
TEST_P(SensorsHidlTest, MagnetometerStreamingOperationNormal) {
    testStreamingOperation(SensorTypeVersion::MAGNETIC_FIELD, std::chrono::milliseconds(20),
                           std::chrono::seconds(5), NullChecker<EventType>());
}

// Test if sensor hal can do game speed magnetometer streaming properly
TEST_P(SensorsHidlTest, MagnetometerStreamingOperationFast) {
    testStreamingOperation(SensorTypeVersion::MAGNETIC_FIELD, std::chrono::milliseconds(5),
                           std::chrono::seconds(5), NullChecker<EventType>());
}

// Test if sensor hal can do accelerometer sampling rate switch properly when sensor is active
TEST_P(SensorsHidlTest, AccelerometerSamplingPeriodHotSwitchOperation) {
    testSamplingRateHotSwitchOperation(SensorTypeVersion::ACCELEROMETER);
    testSamplingRateHotSwitchOperation(SensorTypeVersion::ACCELEROMETER, false /*fastToSlow*/);
}

// Test if sensor hal can do gyroscope sampling rate switch properly when sensor is active
TEST_P(SensorsHidlTest, GyroscopeSamplingPeriodHotSwitchOperation) {
    testSamplingRateHotSwitchOperation(SensorTypeVersion::GYROSCOPE);
    testSamplingRateHotSwitchOperation(SensorTypeVersion::GYROSCOPE, false /*fastToSlow*/);
}

// Test if sensor hal can do magnetometer sampling rate switch properly when sensor is active
TEST_P(SensorsHidlTest, MagnetometerSamplingPeriodHotSwitchOperation) {
    testSamplingRateHotSwitchOperation(SensorTypeVersion::MAGNETIC_FIELD);
    testSamplingRateHotSwitchOperation(SensorTypeVersion::MAGNETIC_FIELD, false /*fastToSlow*/);
}

// Test if sensor hal can do accelerometer batching properly
TEST_P(SensorsHidlTest, AccelerometerBatchingOperation) {
    testBatchingOperation(SensorTypeVersion::ACCELEROMETER);
}

// Test if sensor hal can do gyroscope batching properly
TEST_P(SensorsHidlTest, GyroscopeBatchingOperation) {
    testBatchingOperation(SensorTypeVersion::GYROSCOPE);
}

// Test if sensor hal can do magnetometer batching properly
TEST_P(SensorsHidlTest, MagnetometerBatchingOperation) {
    testBatchingOperation(SensorTypeVersion::MAGNETIC_FIELD);
}

// Test sensor event direct report with ashmem for accel sensor at normal rate
TEST_P(SensorsHidlTest, AccelerometerAshmemDirectReportOperationNormal) {
    testDirectReportOperation(SensorTypeVersion::ACCELEROMETER, SharedMemType::ASHMEM,
                              RateLevel::NORMAL, mAccelNormChecker);
}

// Test sensor event direct report with ashmem for accel sensor at fast rate
TEST_P(SensorsHidlTest, AccelerometerAshmemDirectReportOperationFast) {
    testDirectReportOperation(SensorTypeVersion::ACCELEROMETER, SharedMemType::ASHMEM,
                              RateLevel::FAST, mAccelNormChecker);
}

// Test sensor event direct report with ashmem for accel sensor at very fast rate
TEST_P(SensorsHidlTest, AccelerometerAshmemDirectReportOperationVeryFast) {
    testDirectReportOperation(SensorTypeVersion::ACCELEROMETER, SharedMemType::ASHMEM,
                              RateLevel::VERY_FAST, mAccelNormChecker);
}

// Test sensor event direct report with ashmem for gyro sensor at normal rate
TEST_P(SensorsHidlTest, GyroscopeAshmemDirectReportOperationNormal) {
    testDirectReportOperation(SensorTypeVersion::GYROSCOPE, SharedMemType::ASHMEM,
                              RateLevel::NORMAL, mGyroNormChecker);
}

// Test sensor event direct report with ashmem for gyro sensor at fast rate
TEST_P(SensorsHidlTest, GyroscopeAshmemDirectReportOperationFast) {
    testDirectReportOperation(SensorTypeVersion::GYROSCOPE, SharedMemType::ASHMEM, RateLevel::FAST,
                              mGyroNormChecker);
}

// Test sensor event direct report with ashmem for gyro sensor at very fast rate
TEST_P(SensorsHidlTest, GyroscopeAshmemDirectReportOperationVeryFast) {
    testDirectReportOperation(SensorTypeVersion::GYROSCOPE, SharedMemType::ASHMEM,
                              RateLevel::VERY_FAST, mGyroNormChecker);
}

// Test sensor event direct report with ashmem for mag sensor at normal rate
TEST_P(SensorsHidlTest, MagnetometerAshmemDirectReportOperationNormal) {
    testDirectReportOperation(SensorTypeVersion::MAGNETIC_FIELD, SharedMemType::ASHMEM,
                              RateLevel::NORMAL, NullChecker<EventType>());
}

// Test sensor event direct report with ashmem for mag sensor at fast rate
TEST_P(SensorsHidlTest, MagnetometerAshmemDirectReportOperationFast) {
    testDirectReportOperation(SensorTypeVersion::MAGNETIC_FIELD, SharedMemType::ASHMEM,
                              RateLevel::FAST, NullChecker<EventType>());
}

// Test sensor event direct report with ashmem for mag sensor at very fast rate
TEST_P(SensorsHidlTest, MagnetometerAshmemDirectReportOperationVeryFast) {
    testDirectReportOperation(SensorTypeVersion::MAGNETIC_FIELD, SharedMemType::ASHMEM,
                              RateLevel::VERY_FAST, NullChecker<EventType>());
}

// Test sensor event direct report with gralloc for accel sensor at normal rate
TEST_P(SensorsHidlTest, AccelerometerGrallocDirectReportOperationNormal) {
    testDirectReportOperation(SensorTypeVersion::ACCELEROMETER, SharedMemType::GRALLOC,
                              RateLevel::NORMAL, mAccelNormChecker);
}

// Test sensor event direct report with gralloc for accel sensor at fast rate
TEST_P(SensorsHidlTest, AccelerometerGrallocDirectReportOperationFast) {
    testDirectReportOperation(SensorTypeVersion::ACCELEROMETER, SharedMemType::GRALLOC,
                              RateLevel::FAST, mAccelNormChecker);
}

// Test sensor event direct report with gralloc for accel sensor at very fast rate
TEST_P(SensorsHidlTest, AccelerometerGrallocDirectReportOperationVeryFast) {
    testDirectReportOperation(SensorTypeVersion::ACCELEROMETER, SharedMemType::GRALLOC,
                              RateLevel::VERY_FAST, mAccelNormChecker);
}

// Test sensor event direct report with gralloc for gyro sensor at normal rate
TEST_P(SensorsHidlTest, GyroscopeGrallocDirectReportOperationNormal) {
    testDirectReportOperation(SensorTypeVersion::GYROSCOPE, SharedMemType::GRALLOC,
                              RateLevel::NORMAL, mGyroNormChecker);
}

// Test sensor event direct report with gralloc for gyro sensor at fast rate
TEST_P(SensorsHidlTest, GyroscopeGrallocDirectReportOperationFast) {
    testDirectReportOperation(SensorTypeVersion::GYROSCOPE, SharedMemType::GRALLOC, RateLevel::FAST,
                              mGyroNormChecker);
}

// Test sensor event direct report with gralloc for gyro sensor at very fast rate
TEST_P(SensorsHidlTest, GyroscopeGrallocDirectReportOperationVeryFast) {
    testDirectReportOperation(SensorTypeVersion::GYROSCOPE, SharedMemType::GRALLOC,
                              RateLevel::VERY_FAST, mGyroNormChecker);
}

// Test sensor event direct report with gralloc for mag sensor at normal rate
TEST_P(SensorsHidlTest, MagnetometerGrallocDirectReportOperationNormal) {
    testDirectReportOperation(SensorTypeVersion::MAGNETIC_FIELD, SharedMemType::GRALLOC,
                              RateLevel::NORMAL, NullChecker<EventType>());
}

// Test sensor event direct report with gralloc for mag sensor at fast rate
TEST_P(SensorsHidlTest, MagnetometerGrallocDirectReportOperationFast) {
    testDirectReportOperation(SensorTypeVersion::MAGNETIC_FIELD, SharedMemType::GRALLOC,
                              RateLevel::FAST, NullChecker<EventType>());
}

// Test sensor event direct report with gralloc for mag sensor at very fast rate
TEST_P(SensorsHidlTest, MagnetometerGrallocDirectReportOperationVeryFast) {
    testDirectReportOperation(SensorTypeVersion::MAGNETIC_FIELD, SharedMemType::GRALLOC,
                              RateLevel::VERY_FAST, NullChecker<EventType>());
}

TEST_P(SensorsHidlTest, ConfigureDirectChannelWithInvalidHandle) {
    SensorInfoType sensor;
    SharedMemType memType;
    RateLevel rate;
    if (!getDirectChannelSensor(&sensor, &memType, &rate)) {
        return;
    }

    // Verify that an invalid channel handle produces a BAD_VALUE result
    configDirectReport(sensor.sensorHandle, -1, rate, [](Result result, int32_t /* reportToken */) {
        ASSERT_EQ(result, Result::BAD_VALUE);
    });
}

TEST_P(SensorsHidlTest, CleanupDirectConnectionOnInitialize) {
    constexpr size_t kNumEvents = 1;
    constexpr size_t kMemSize = kNumEvents * kEventSize;

    SensorInfoType sensor;
    SharedMemType memType;
    RateLevel rate;

    if (!getDirectChannelSensor(&sensor, &memType, &rate)) {
        return;
    }

    std::shared_ptr<SensorsTestSharedMemory<SensorTypeVersion, EventType>> mem(
            SensorsTestSharedMemory<SensorTypeVersion, EventType>::create(memType, kMemSize));
    ASSERT_NE(mem, nullptr);

    int32_t directChannelHandle = 0;
    registerDirectChannel(mem->getSharedMemInfo(), [&](Result result, int32_t channelHandle) {
        ASSERT_EQ(result, Result::OK);
        directChannelHandle = channelHandle;
    });

    // Configure the channel and expect success
    configDirectReport(
            sensor.sensorHandle, directChannelHandle, rate,
            [](Result result, int32_t /* reportToken */) { ASSERT_EQ(result, Result::OK); });

    // Call initialize() via the environment setup to cause the HAL to re-initialize
    // Clear the active direct connections so they are not stopped during TearDown
    auto handles = mDirectChannelHandles;
    mDirectChannelHandles.clear();
    getEnvironment()->HidlTearDown();
    getEnvironment()->HidlSetUp();
    if (HasFatalFailure()) {
        return;  // Exit early if resetting the environment failed
    }

    // Attempt to configure the direct channel and expect it to fail
    configDirectReport(
            sensor.sensorHandle, directChannelHandle, rate,
            [](Result result, int32_t /* reportToken */) { ASSERT_EQ(result, Result::BAD_VALUE); });

    // Restore original handles, though they should already be deactivated
    mDirectChannelHandles = handles;
}

TEST_P(SensorsHidlTest, SensorListDoesntContainInvalidType) {
    getSensors()->getSensorsList([&](const auto& list) {
        const size_t count = list.size();
        for (size_t i = 0; i < count; ++i) {
            const auto& s = list[i];
            EXPECT_FALSE(s.type == ::android::hardware::sensors::V2_1::SensorType::HINGE_ANGLE);
        }
    });
}

TEST_P(SensorsHidlTest, FlushNonexistentSensor) {
    SensorInfoType sensor;
    std::vector<SensorInfoType> sensors = getNonOneShotSensors();
    if (sensors.size() == 0) {
        sensors = getOneShotSensors();
        if (sensors.size() == 0) {
            return;
        }
    }
    sensor = sensors.front();
    sensor.sensorHandle = getInvalidSensorHandle();
    runSingleFlushTest(std::vector<SensorInfoType>{sensor}, false /* activateSensor */,
                       0 /* expectedFlushCount */, Result::BAD_VALUE);
}

INSTANTIATE_TEST_SUITE_P(PerInstance, SensorsHidlTest,
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 android::hardware::sensors::V2_0::ISensors::descriptor)),
                         android::hardware::PrintInstanceNameToString);