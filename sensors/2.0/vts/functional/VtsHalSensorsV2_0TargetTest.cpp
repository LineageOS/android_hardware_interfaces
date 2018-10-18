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

#define LOG_TAG "sensors_hidl_hal_test"

#include "SensorsHidlEnvironmentV2_0.h"
#include "sensors-vts-utils/SensorsHidlTestBase.h"

#include <android/hardware/sensors/2.0/ISensors.h>
#include <android/hardware/sensors/2.0/types.h>
#include <log/log.h>
#include <utils/SystemClock.h>

#include <cinttypes>
#include <vector>

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::SensorStatus;
using ::android::hardware::sensors::V1_0::Vec3;

// The main test class for SENSORS HIDL HAL.

class SensorsHidlTest : public SensorsHidlTestBase {
   protected:
    SensorInfo defaultSensorByType(SensorType type) override;
    std::vector<SensorInfo> getSensorsList();
    // implementation wrapper
    Return<void> getSensorsList(ISensors::getSensorsList_cb _hidl_cb) override {
        return getSensors()->getSensorsList(_hidl_cb);
    }

    Return<Result> activate(int32_t sensorHandle, bool enabled) override;

    Return<Result> batch(int32_t sensorHandle, int64_t samplingPeriodNs,
                         int64_t maxReportLatencyNs) override {
        return getSensors()->batch(sensorHandle, samplingPeriodNs, maxReportLatencyNs);
    }

    Return<Result> flush(int32_t sensorHandle) override {
        return getSensors()->flush(sensorHandle);
    }

    Return<Result> injectSensorData(const Event& event) override {
        return getSensors()->injectSensorData(event);
    }

    Return<void> registerDirectChannel(const SharedMemInfo& mem,
                                       ISensors::registerDirectChannel_cb _hidl_cb) override;

    Return<Result> unregisterDirectChannel(int32_t channelHandle) override {
        return getSensors()->unregisterDirectChannel(channelHandle);
    }

    Return<void> configDirectReport(int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
                                    ISensors::configDirectReport_cb _hidl_cb) override {
        return getSensors()->configDirectReport(sensorHandle, channelHandle, rate, _hidl_cb);
    }

    inline sp<::android::hardware::sensors::V2_0::ISensors>& getSensors() {
        return SensorsHidlEnvironmentV2_0::Instance()->mSensors;
    }

    SensorsHidlEnvironmentBase* getEnvironment() override {
        return SensorsHidlEnvironmentV2_0::Instance();
    }
};

Return<Result> SensorsHidlTest::activate(int32_t sensorHandle, bool enabled) {
    // If activating a sensor, add the handle in a set so that when test fails it can be turned off.
    // The handle is not removed when it is deactivating on purpose so that it is not necessary to
    // check the return value of deactivation. Deactivating a sensor more than once does not have
    // negative effect.
    if (enabled) {
        mSensorHandles.insert(sensorHandle);
    }
    return getSensors()->activate(sensorHandle, enabled);
}

Return<void> SensorsHidlTest::registerDirectChannel(const SharedMemInfo& mem,
                                                    ISensors::registerDirectChannel_cb cb) {
    // If registeration of a channel succeeds, add the handle of channel to a set so that it can be
    // unregistered when test fails. Unregister a channel does not remove the handle on purpose.
    // Unregistering a channel more than once should not have negative effect.
    getSensors()->registerDirectChannel(mem, [&](auto result, auto channelHandle) {
        if (result == Result::OK) {
            mDirectChannelHandles.insert(channelHandle);
        }
        cb(result, channelHandle);
    });
    return Void();
}

SensorInfo SensorsHidlTest::defaultSensorByType(SensorType type) {
    SensorInfo ret;

    ret.type = (SensorType)-1;
    getSensors()->getSensorsList([&](const auto& list) {
        const size_t count = list.size();
        for (size_t i = 0; i < count; ++i) {
            if (list[i].type == type) {
                ret = list[i];
                return;
            }
        }
    });

    return ret;
}

std::vector<SensorInfo> SensorsHidlTest::getSensorsList() {
    std::vector<SensorInfo> ret;

    getSensors()->getSensorsList([&](const auto& list) {
        const size_t count = list.size();
        ret.reserve(list.size());
        for (size_t i = 0; i < count; ++i) {
            ret.push_back(list[i]);
        }
    });

    return ret;
}

// Test if sensor list returned is valid
TEST_F(SensorsHidlTest, SensorListValid) {
    getSensors()->getSensorsList([&](const auto& list) {
        const size_t count = list.size();
        for (size_t i = 0; i < count; ++i) {
            const auto& s = list[i];
            SCOPED_TRACE(::testing::Message()
                         << i << "/" << count << ": "
                         << " handle=0x" << std::hex << std::setw(8) << std::setfill('0')
                         << s.sensorHandle << std::dec << " type=" << static_cast<int>(s.type)
                         << " name=" << s.name);

            // Test non-empty type string
            EXPECT_FALSE(s.typeAsString.empty());

            // Test defined type matches defined string type
            EXPECT_NO_FATAL_FAILURE(assertTypeMatchStringType(s.type, s.typeAsString));

            // Test if all sensor has name and vendor
            EXPECT_FALSE(s.name.empty());
            EXPECT_FALSE(s.vendor.empty());

            // Test power > 0, maxRange > 0
            EXPECT_LE(0, s.power);
            EXPECT_LT(0, s.maxRange);

            // Info type, should have no sensor
            EXPECT_FALSE(s.type == SensorType::ADDITIONAL_INFO || s.type == SensorType::META_DATA);

            // Test fifoMax >= fifoReserved
            EXPECT_GE(s.fifoMaxEventCount, s.fifoReservedEventCount)
                << "max=" << s.fifoMaxEventCount << " reserved=" << s.fifoReservedEventCount;

            // Test Reporting mode valid
            EXPECT_NO_FATAL_FAILURE(assertTypeMatchReportMode(s.type, extractReportMode(s.flags)));

            // Test min max are in the right order
            EXPECT_LE(s.minDelay, s.maxDelay);
            // Test min/max delay matches reporting mode
            EXPECT_NO_FATAL_FAILURE(
                assertDelayMatchReportMode(s.minDelay, s.maxDelay, extractReportMode(s.flags)));
        }
    });
}

// Test if sensor list returned is valid
TEST_F(SensorsHidlTest, SetOperationMode) {
    std::vector<SensorInfo> sensorList = getSensorsList();

    bool needOperationModeSupport =
        std::any_of(sensorList.begin(), sensorList.end(),
                    [](const auto& s) { return (s.flags & SensorFlagBits::DATA_INJECTION) != 0; });
    if (!needOperationModeSupport) {
        return;
    }

    ASSERT_EQ(Result::OK, getSensors()->setOperationMode(OperationMode::NORMAL));
    ASSERT_EQ(Result::OK, getSensors()->setOperationMode(OperationMode::DATA_INJECTION));
    ASSERT_EQ(Result::OK, getSensors()->setOperationMode(OperationMode::NORMAL));
}

// Test if sensor list returned is valid
TEST_F(SensorsHidlTest, InjectSensorEventData) {
    std::vector<SensorInfo> sensorList = getSensorsList();
    std::vector<SensorInfo> sensorSupportInjection;

    bool needOperationModeSupport =
        std::any_of(sensorList.begin(), sensorList.end(), [&sensorSupportInjection](const auto& s) {
            bool ret = (s.flags & SensorFlagBits::DATA_INJECTION) != 0;
            if (ret) {
                sensorSupportInjection.push_back(s);
            }
            return ret;
        });
    if (!needOperationModeSupport) {
        return;
    }

    ASSERT_EQ(Result::OK, getSensors()->setOperationMode(OperationMode::NORMAL));
    ASSERT_EQ(Result::OK, getSensors()->setOperationMode(OperationMode::DATA_INJECTION));

    for (const auto& s : sensorSupportInjection) {
        switch (s.type) {
            case SensorType::ACCELEROMETER:
            case SensorType::GYROSCOPE:
            case SensorType::MAGNETIC_FIELD: {
                usleep(100000);  // sleep 100ms

                Event dummy;
                dummy.timestamp = android::elapsedRealtimeNano();
                dummy.sensorType = s.type;
                dummy.sensorHandle = s.sensorHandle;
                Vec3 v = {1, 2, 3, SensorStatus::ACCURACY_HIGH};
                dummy.u.vec3 = v;

                EXPECT_EQ(Result::OK, getSensors()->injectSensorData(dummy));
                break;
            }
            default:
                break;
        }
    }
    ASSERT_EQ(Result::OK, getSensors()->setOperationMode(OperationMode::NORMAL));
}

// Test if sensor hal can do UI speed accelerometer streaming properly
TEST_F(SensorsHidlTest, AccelerometerStreamingOperationSlow) {
    testStreamingOperation(SensorType::ACCELEROMETER, std::chrono::milliseconds(200),
                           std::chrono::seconds(5), sAccelNormChecker);
}

// Test if sensor hal can do normal speed accelerometer streaming properly
TEST_F(SensorsHidlTest, AccelerometerStreamingOperationNormal) {
    testStreamingOperation(SensorType::ACCELEROMETER, std::chrono::milliseconds(20),
                           std::chrono::seconds(5), sAccelNormChecker);
}

// Test if sensor hal can do game speed accelerometer streaming properly
TEST_F(SensorsHidlTest, AccelerometerStreamingOperationFast) {
    testStreamingOperation(SensorType::ACCELEROMETER, std::chrono::milliseconds(5),
                           std::chrono::seconds(5), sAccelNormChecker);
}

// Test if sensor hal can do UI speed gyroscope streaming properly
TEST_F(SensorsHidlTest, GyroscopeStreamingOperationSlow) {
    testStreamingOperation(SensorType::GYROSCOPE, std::chrono::milliseconds(200),
                           std::chrono::seconds(5), sGyroNormChecker);
}

// Test if sensor hal can do normal speed gyroscope streaming properly
TEST_F(SensorsHidlTest, GyroscopeStreamingOperationNormal) {
    testStreamingOperation(SensorType::GYROSCOPE, std::chrono::milliseconds(20),
                           std::chrono::seconds(5), sGyroNormChecker);
}

// Test if sensor hal can do game speed gyroscope streaming properly
TEST_F(SensorsHidlTest, GyroscopeStreamingOperationFast) {
    testStreamingOperation(SensorType::GYROSCOPE, std::chrono::milliseconds(5),
                           std::chrono::seconds(5), sGyroNormChecker);
}

// Test if sensor hal can do UI speed magnetometer streaming properly
TEST_F(SensorsHidlTest, MagnetometerStreamingOperationSlow) {
    testStreamingOperation(SensorType::MAGNETIC_FIELD, std::chrono::milliseconds(200),
                           std::chrono::seconds(5), NullChecker());
}

// Test if sensor hal can do normal speed magnetometer streaming properly
TEST_F(SensorsHidlTest, MagnetometerStreamingOperationNormal) {
    testStreamingOperation(SensorType::MAGNETIC_FIELD, std::chrono::milliseconds(20),
                           std::chrono::seconds(5), NullChecker());
}

// Test if sensor hal can do game speed magnetometer streaming properly
TEST_F(SensorsHidlTest, MagnetometerStreamingOperationFast) {
    testStreamingOperation(SensorType::MAGNETIC_FIELD, std::chrono::milliseconds(5),
                           std::chrono::seconds(5), NullChecker());
}

// Test if sensor hal can do accelerometer sampling rate switch properly when sensor is active
TEST_F(SensorsHidlTest, AccelerometerSamplingPeriodHotSwitchOperation) {
    testSamplingRateHotSwitchOperation(SensorType::ACCELEROMETER);
    testSamplingRateHotSwitchOperation(SensorType::ACCELEROMETER, false /*fastToSlow*/);
}

// Test if sensor hal can do gyroscope sampling rate switch properly when sensor is active
TEST_F(SensorsHidlTest, GyroscopeSamplingPeriodHotSwitchOperation) {
    testSamplingRateHotSwitchOperation(SensorType::GYROSCOPE);
    testSamplingRateHotSwitchOperation(SensorType::GYROSCOPE, false /*fastToSlow*/);
}

// Test if sensor hal can do magnetometer sampling rate switch properly when sensor is active
TEST_F(SensorsHidlTest, MagnetometerSamplingPeriodHotSwitchOperation) {
    testSamplingRateHotSwitchOperation(SensorType::MAGNETIC_FIELD);
    testSamplingRateHotSwitchOperation(SensorType::MAGNETIC_FIELD, false /*fastToSlow*/);
}

// Test if sensor hal can do accelerometer batching properly
TEST_F(SensorsHidlTest, AccelerometerBatchingOperation) {
    testBatchingOperation(SensorType::ACCELEROMETER);
}

// Test if sensor hal can do gyroscope batching properly
TEST_F(SensorsHidlTest, GyroscopeBatchingOperation) {
    testBatchingOperation(SensorType::GYROSCOPE);
}

// Test if sensor hal can do magnetometer batching properly
TEST_F(SensorsHidlTest, MagnetometerBatchingOperation) {
    testBatchingOperation(SensorType::MAGNETIC_FIELD);
}

// Test sensor event direct report with ashmem for accel sensor at normal rate
TEST_F(SensorsHidlTest, AccelerometerAshmemDirectReportOperationNormal) {
    testDirectReportOperation(SensorType::ACCELEROMETER, SharedMemType::ASHMEM, RateLevel::NORMAL,
                              sAccelNormChecker);
}

// Test sensor event direct report with ashmem for accel sensor at fast rate
TEST_F(SensorsHidlTest, AccelerometerAshmemDirectReportOperationFast) {
    testDirectReportOperation(SensorType::ACCELEROMETER, SharedMemType::ASHMEM, RateLevel::FAST,
                              sAccelNormChecker);
}

// Test sensor event direct report with ashmem for accel sensor at very fast rate
TEST_F(SensorsHidlTest, AccelerometerAshmemDirectReportOperationVeryFast) {
    testDirectReportOperation(SensorType::ACCELEROMETER, SharedMemType::ASHMEM,
                              RateLevel::VERY_FAST, sAccelNormChecker);
}

// Test sensor event direct report with ashmem for gyro sensor at normal rate
TEST_F(SensorsHidlTest, GyroscopeAshmemDirectReportOperationNormal) {
    testDirectReportOperation(SensorType::GYROSCOPE, SharedMemType::ASHMEM, RateLevel::NORMAL,
                              sGyroNormChecker);
}

// Test sensor event direct report with ashmem for gyro sensor at fast rate
TEST_F(SensorsHidlTest, GyroscopeAshmemDirectReportOperationFast) {
    testDirectReportOperation(SensorType::GYROSCOPE, SharedMemType::ASHMEM, RateLevel::FAST,
                              sGyroNormChecker);
}

// Test sensor event direct report with ashmem for gyro sensor at very fast rate
TEST_F(SensorsHidlTest, GyroscopeAshmemDirectReportOperationVeryFast) {
    testDirectReportOperation(SensorType::GYROSCOPE, SharedMemType::ASHMEM, RateLevel::VERY_FAST,
                              sGyroNormChecker);
}

// Test sensor event direct report with ashmem for mag sensor at normal rate
TEST_F(SensorsHidlTest, MagnetometerAshmemDirectReportOperationNormal) {
    testDirectReportOperation(SensorType::MAGNETIC_FIELD, SharedMemType::ASHMEM, RateLevel::NORMAL,
                              NullChecker());
}

// Test sensor event direct report with ashmem for mag sensor at fast rate
TEST_F(SensorsHidlTest, MagnetometerAshmemDirectReportOperationFast) {
    testDirectReportOperation(SensorType::MAGNETIC_FIELD, SharedMemType::ASHMEM, RateLevel::FAST,
                              NullChecker());
}

// Test sensor event direct report with ashmem for mag sensor at very fast rate
TEST_F(SensorsHidlTest, MagnetometerAshmemDirectReportOperationVeryFast) {
    testDirectReportOperation(SensorType::MAGNETIC_FIELD, SharedMemType::ASHMEM,
                              RateLevel::VERY_FAST, NullChecker());
}

// Test sensor event direct report with gralloc for accel sensor at normal rate
TEST_F(SensorsHidlTest, AccelerometerGrallocDirectReportOperationNormal) {
    testDirectReportOperation(SensorType::ACCELEROMETER, SharedMemType::GRALLOC, RateLevel::NORMAL,
                              sAccelNormChecker);
}

// Test sensor event direct report with gralloc for accel sensor at fast rate
TEST_F(SensorsHidlTest, AccelerometerGrallocDirectReportOperationFast) {
    testDirectReportOperation(SensorType::ACCELEROMETER, SharedMemType::GRALLOC, RateLevel::FAST,
                              sAccelNormChecker);
}

// Test sensor event direct report with gralloc for accel sensor at very fast rate
TEST_F(SensorsHidlTest, AccelerometerGrallocDirectReportOperationVeryFast) {
    testDirectReportOperation(SensorType::ACCELEROMETER, SharedMemType::GRALLOC,
                              RateLevel::VERY_FAST, sAccelNormChecker);
}

// Test sensor event direct report with gralloc for gyro sensor at normal rate
TEST_F(SensorsHidlTest, GyroscopeGrallocDirectReportOperationNormal) {
    testDirectReportOperation(SensorType::GYROSCOPE, SharedMemType::GRALLOC, RateLevel::NORMAL,
                              sGyroNormChecker);
}

// Test sensor event direct report with gralloc for gyro sensor at fast rate
TEST_F(SensorsHidlTest, GyroscopeGrallocDirectReportOperationFast) {
    testDirectReportOperation(SensorType::GYROSCOPE, SharedMemType::GRALLOC, RateLevel::FAST,
                              sGyroNormChecker);
}

// Test sensor event direct report with gralloc for gyro sensor at very fast rate
TEST_F(SensorsHidlTest, GyroscopeGrallocDirectReportOperationVeryFast) {
    testDirectReportOperation(SensorType::GYROSCOPE, SharedMemType::GRALLOC, RateLevel::VERY_FAST,
                              sGyroNormChecker);
}

// Test sensor event direct report with gralloc for mag sensor at normal rate
TEST_F(SensorsHidlTest, MagnetometerGrallocDirectReportOperationNormal) {
    testDirectReportOperation(SensorType::MAGNETIC_FIELD, SharedMemType::GRALLOC, RateLevel::NORMAL,
                              NullChecker());
}

// Test sensor event direct report with gralloc for mag sensor at fast rate
TEST_F(SensorsHidlTest, MagnetometerGrallocDirectReportOperationFast) {
    testDirectReportOperation(SensorType::MAGNETIC_FIELD, SharedMemType::GRALLOC, RateLevel::FAST,
                              NullChecker());
}

// Test sensor event direct report with gralloc for mag sensor at very fast rate
TEST_F(SensorsHidlTest, MagnetometerGrallocDirectReportOperationVeryFast) {
    testDirectReportOperation(SensorType::MAGNETIC_FIELD, SharedMemType::GRALLOC,
                              RateLevel::VERY_FAST, NullChecker());
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(SensorsHidlEnvironmentV2_0::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    SensorsHidlEnvironmentV2_0::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
// vim: set ts=2 sw=2
