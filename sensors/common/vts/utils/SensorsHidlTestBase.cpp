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

#include "SensorsHidlTestBase.h"

#include "sensors-vts-utils/GrallocWrapper.h"
#include "sensors-vts-utils/SensorsTestSharedMemory.h"

#include <hardware/sensors.h>  // for sensor type strings
#include <log/log.h>
#include <utils/SystemClock.h>

#include <cinttypes>

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::sensors::V1_0::SensorFlagShift;
using ::android::hardware::sensors::V1_0::SensorsEventFormatOffset;

const Vec3NormChecker SensorsHidlTestBase::sAccelNormChecker(
    Vec3NormChecker::byNominal(GRAVITY_EARTH, 1.0f /*m/s^2*/));
const Vec3NormChecker SensorsHidlTestBase::sGyroNormChecker(
    Vec3NormChecker::byNominal(0.f, 0.1f /*rad/s*/));

std::vector<Event> SensorsHidlTestBase::collectEvents(useconds_t timeLimitUs, size_t nEventLimit,
                                                      bool clearBeforeStart,
                                                      bool changeCollection) {
    std::vector<Event> events;
    constexpr useconds_t SLEEP_GRANULARITY = 100 * 1000;  // granularity 100 ms

    ALOGI("collect max of %zu events for %d us, clearBeforeStart %d", nEventLimit, timeLimitUs,
          clearBeforeStart);

    if (changeCollection) {
        getEnvironment()->setCollection(true);
    }
    if (clearBeforeStart) {
        getEnvironment()->catEvents(nullptr);
    }

    while (timeLimitUs > 0) {
        useconds_t duration = std::min(SLEEP_GRANULARITY, timeLimitUs);
        usleep(duration);
        timeLimitUs -= duration;

        getEnvironment()->catEvents(&events);
        if (events.size() >= nEventLimit) {
            break;
        }
        ALOGV("time to go = %d, events to go = %d", (int)timeLimitUs,
              (int)(nEventLimit - events.size()));
    }

    if (changeCollection) {
        getEnvironment()->setCollection(false);
    }
    return events;
}

void SensorsHidlTestBase::assertTypeMatchStringType(SensorType type,
                                                    const hidl_string& stringType) {
    if (type >= SensorType::DEVICE_PRIVATE_BASE) {
        return;
    }

    switch (type) {
#define CHECK_TYPE_STRING_FOR_SENSOR_TYPE(type)                      \
    case SensorType::type:                                           \
        ASSERT_STREQ(SENSOR_STRING_TYPE_##type, stringType.c_str()); \
        break;
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(ACCELEROMETER);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(ACCELEROMETER_UNCALIBRATED);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(ADDITIONAL_INFO);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(AMBIENT_TEMPERATURE);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(DEVICE_ORIENTATION);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(DYNAMIC_SENSOR_META);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GAME_ROTATION_VECTOR);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GEOMAGNETIC_ROTATION_VECTOR);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GLANCE_GESTURE);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GRAVITY);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GYROSCOPE);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GYROSCOPE_UNCALIBRATED);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(HEART_BEAT);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(HEART_RATE);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(LIGHT);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(LINEAR_ACCELERATION);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(LOW_LATENCY_OFFBODY_DETECT);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(MAGNETIC_FIELD);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(MAGNETIC_FIELD_UNCALIBRATED);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(MOTION_DETECT);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(ORIENTATION);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(PICK_UP_GESTURE);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(POSE_6DOF);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(PRESSURE);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(PROXIMITY);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(RELATIVE_HUMIDITY);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(ROTATION_VECTOR);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(SIGNIFICANT_MOTION);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(STATIONARY_DETECT);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(STEP_COUNTER);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(STEP_DETECTOR);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(TEMPERATURE);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(TILT_DETECTOR);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(WAKE_GESTURE);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(WRIST_TILT_GESTURE);
        default:
            FAIL() << "Type " << static_cast<int>(type)
                   << " in android defined range is not checked, "
                   << "stringType = " << stringType;
#undef CHECK_TYPE_STRING_FOR_SENSOR_TYPE
    }
}

void SensorsHidlTestBase::assertTypeMatchReportMode(SensorType type, SensorFlagBits reportMode) {
    if (type >= SensorType::DEVICE_PRIVATE_BASE) {
        return;
    }

    SensorFlagBits expected = expectedReportModeForType(type);

    ASSERT_TRUE(expected == (SensorFlagBits)-1 || expected == reportMode)
        << "reportMode=" << static_cast<int>(reportMode)
        << "expected=" << static_cast<int>(expected);
}

void SensorsHidlTestBase::assertDelayMatchReportMode(int32_t minDelay, int32_t maxDelay,
                                                     SensorFlagBits reportMode) {
    switch (reportMode) {
        case SensorFlagBits::CONTINUOUS_MODE:
            ASSERT_LT(0, minDelay);
            ASSERT_LE(0, maxDelay);
            break;
        case SensorFlagBits::ON_CHANGE_MODE:
            ASSERT_LE(0, minDelay);
            ASSERT_LE(0, maxDelay);
            break;
        case SensorFlagBits::ONE_SHOT_MODE:
            ASSERT_EQ(-1, minDelay);
            ASSERT_EQ(0, maxDelay);
            break;
        case SensorFlagBits::SPECIAL_REPORTING_MODE:
            // do not enforce anything for special reporting mode
            break;
        default:
            FAIL() << "Report mode " << static_cast<int>(reportMode) << " not checked";
    }
}

// return -1 means no expectation for this type
SensorFlagBits SensorsHidlTestBase::expectedReportModeForType(SensorType type) {
    switch (type) {
        case SensorType::ACCELEROMETER:
        case SensorType::ACCELEROMETER_UNCALIBRATED:
        case SensorType::GYROSCOPE:
        case SensorType::MAGNETIC_FIELD:
        case SensorType::ORIENTATION:
        case SensorType::PRESSURE:
        case SensorType::TEMPERATURE:
        case SensorType::GRAVITY:
        case SensorType::LINEAR_ACCELERATION:
        case SensorType::ROTATION_VECTOR:
        case SensorType::MAGNETIC_FIELD_UNCALIBRATED:
        case SensorType::GAME_ROTATION_VECTOR:
        case SensorType::GYROSCOPE_UNCALIBRATED:
        case SensorType::GEOMAGNETIC_ROTATION_VECTOR:
        case SensorType::POSE_6DOF:
        case SensorType::HEART_BEAT:
            return SensorFlagBits::CONTINUOUS_MODE;

        case SensorType::LIGHT:
        case SensorType::PROXIMITY:
        case SensorType::RELATIVE_HUMIDITY:
        case SensorType::AMBIENT_TEMPERATURE:
        case SensorType::HEART_RATE:
        case SensorType::DEVICE_ORIENTATION:
        case SensorType::STEP_COUNTER:
        case SensorType::LOW_LATENCY_OFFBODY_DETECT:
            return SensorFlagBits::ON_CHANGE_MODE;

        case SensorType::SIGNIFICANT_MOTION:
        case SensorType::WAKE_GESTURE:
        case SensorType::GLANCE_GESTURE:
        case SensorType::PICK_UP_GESTURE:
        case SensorType::MOTION_DETECT:
        case SensorType::STATIONARY_DETECT:
            return SensorFlagBits::ONE_SHOT_MODE;

        case SensorType::STEP_DETECTOR:
        case SensorType::TILT_DETECTOR:
        case SensorType::WRIST_TILT_GESTURE:
        case SensorType::DYNAMIC_SENSOR_META:
            return SensorFlagBits::SPECIAL_REPORTING_MODE;

        default:
            ALOGW("Type %d is not implemented in expectedReportModeForType", (int)type);
            return (SensorFlagBits)-1;
    }
}

bool SensorsHidlTestBase::isDirectReportRateSupported(SensorInfo sensor, RateLevel rate) {
    unsigned int r = static_cast<unsigned int>(sensor.flags & SensorFlagBits::MASK_DIRECT_REPORT) >>
                     static_cast<unsigned int>(SensorFlagShift::DIRECT_REPORT);
    return r >= static_cast<unsigned int>(rate);
}

bool SensorsHidlTestBase::isDirectChannelTypeSupported(SensorInfo sensor, SharedMemType type) {
    switch (type) {
        case SharedMemType::ASHMEM:
            return (sensor.flags & SensorFlagBits::DIRECT_CHANNEL_ASHMEM) != 0;
        case SharedMemType::GRALLOC:
            return (sensor.flags & SensorFlagBits::DIRECT_CHANNEL_GRALLOC) != 0;
        default:
            return false;
    }
}

void SensorsHidlTestBase::testDirectReportOperation(SensorType type, SharedMemType memType,
                                                    RateLevel rate,
                                                    const SensorEventsChecker& checker) {
    constexpr size_t kEventSize = static_cast<size_t>(SensorsEventFormatOffset::TOTAL_LENGTH);
    constexpr size_t kNEvent = 4096;
    constexpr size_t kMemSize = kEventSize * kNEvent;

    constexpr float kNormalNominal = 50;
    constexpr float kFastNominal = 200;
    constexpr float kVeryFastNominal = 800;

    constexpr float kNominalTestTimeSec = 1.f;
    constexpr float kMaxTestTimeSec = kNominalTestTimeSec + 0.5f;  // 0.5 second for initialization

    SensorInfo sensor = defaultSensorByType(type);

    if (!isValidType(sensor.type)) {
        // no default sensor of this type
        return;
    }

    if (!isDirectReportRateSupported(sensor, rate)) {
        return;
    }

    if (!isDirectChannelTypeSupported(sensor, memType)) {
        return;
    }

    std::unique_ptr<SensorsTestSharedMemory> mem(
        SensorsTestSharedMemory::create(memType, kMemSize));
    ASSERT_NE(mem, nullptr);

    char* buffer = mem->getBuffer();
    // fill memory with data
    for (size_t i = 0; i < kMemSize; ++i) {
        buffer[i] = '\xcc';
    }

    int32_t channelHandle;
    registerDirectChannel(mem->getSharedMemInfo(),
                          [&channelHandle](auto result, auto channelHandle_) {
                              ASSERT_EQ(result, Result::OK);
                              channelHandle = channelHandle_;
                          });

    // check memory is zeroed
    for (size_t i = 0; i < kMemSize; ++i) {
        ASSERT_EQ(buffer[i], '\0');
    }

    int32_t eventToken;
    configDirectReport(sensor.sensorHandle, channelHandle, rate,
                       [&eventToken](auto result, auto token) {
                           ASSERT_EQ(result, Result::OK);
                           eventToken = token;
                       });

    usleep(static_cast<useconds_t>(kMaxTestTimeSec * 1e6f));
    auto events = mem->parseEvents();

    // find norminal rate
    float nominalFreq = 0.f;
    switch (rate) {
        case RateLevel::NORMAL:
            nominalFreq = kNormalNominal;
            break;
        case RateLevel::FAST:
            nominalFreq = kFastNominal;
            break;
        case RateLevel::VERY_FAST:
            nominalFreq = kVeryFastNominal;
            break;
        case RateLevel::STOP:
            FAIL();
    }

    // allowed to be between 55% and 220% of nominal freq
    ASSERT_GT(events.size(), static_cast<size_t>(nominalFreq * 0.55f * kNominalTestTimeSec));
    ASSERT_LT(events.size(), static_cast<size_t>(nominalFreq * 2.2f * kMaxTestTimeSec));

    int64_t lastTimestamp = 0;
    bool typeErrorReported = false;
    bool tokenErrorReported = false;
    bool timestampErrorReported = false;
    std::vector<Event> sensorEvents;
    for (auto& e : events) {
        if (!tokenErrorReported) {
            EXPECT_EQ(eventToken, e.sensorHandle)
                << (tokenErrorReported = true,
                    "Event token does not match that retured from configDirectReport");
        }

        if (isMetaSensorType(e.sensorType)) {
            continue;
        }
        sensorEvents.push_back(e);

        if (!typeErrorReported) {
            EXPECT_EQ(type, e.sensorType)
                << (typeErrorReported = true,
                    "Type in event does not match type of sensor registered.");
        }
        if (!timestampErrorReported) {
            EXPECT_GT(e.timestamp, lastTimestamp)
                << (timestampErrorReported = true, "Timestamp not monotonically increasing");
        }
        lastTimestamp = e.timestamp;
    }

    std::string s;
    EXPECT_TRUE(checker.check(sensorEvents, &s)) << s;

    // stop sensor and unregister channel
    configDirectReport(sensor.sensorHandle, channelHandle, RateLevel::STOP,
                       [](auto result, auto) { EXPECT_EQ(result, Result::OK); });
    EXPECT_EQ(unregisterDirectChannel(channelHandle), Result::OK);
}

void SensorsHidlTestBase::testStreamingOperation(SensorType type,
                                                 std::chrono::nanoseconds samplingPeriod,
                                                 std::chrono::seconds duration,
                                                 const SensorEventsChecker& checker) {
    std::vector<Event> events;
    std::vector<Event> sensorEvents;

    const int64_t samplingPeriodInNs = samplingPeriod.count();
    const int64_t batchingPeriodInNs = 0;  // no batching
    const useconds_t minTimeUs = std::chrono::microseconds(duration).count();
    const size_t minNEvent = duration / samplingPeriod;

    SensorInfo sensor = defaultSensorByType(type);

    if (!isValidType(sensor.type)) {
        // no default sensor of this type
        return;
    }

    if (std::chrono::microseconds(sensor.minDelay) > samplingPeriod) {
        // rate not supported
        return;
    }

    int32_t handle = sensor.sensorHandle;

    ASSERT_EQ(batch(handle, samplingPeriodInNs, batchingPeriodInNs), Result::OK);
    ASSERT_EQ(activate(handle, 1), Result::OK);
    events = collectEvents(minTimeUs, minNEvent, true /*clearBeforeStart*/);
    ASSERT_EQ(activate(handle, 0), Result::OK);

    ALOGI("Collected %zu samples", events.size());

    ASSERT_GT(events.size(), 0u);

    bool handleMismatchReported = false;
    bool metaSensorTypeErrorReported = false;
    for (auto& e : events) {
        if (e.sensorType == type) {
            // avoid generating hundreds of error
            if (!handleMismatchReported) {
                EXPECT_EQ(e.sensorHandle, handle)
                    << (handleMismatchReported = true,
                        "Event of the same type must come from the sensor registered");
            }
            sensorEvents.push_back(e);
        } else {
            // avoid generating hundreds of error
            if (!metaSensorTypeErrorReported) {
                EXPECT_TRUE(isMetaSensorType(e.sensorType))
                    << (metaSensorTypeErrorReported = true,
                        "Only meta types are allowed besides the type registered");
            }
        }
    }

    std::string s;
    EXPECT_TRUE(checker.check(sensorEvents, &s)) << s;

    EXPECT_GE(sensorEvents.size(),
              minNEvent / 2);  // make sure returned events are not all meta
}

void SensorsHidlTestBase::testSamplingRateHotSwitchOperation(SensorType type, bool fastToSlow) {
    std::vector<Event> events1, events2;

    constexpr int64_t batchingPeriodInNs = 0;          // no batching
    constexpr int64_t collectionTimeoutUs = 60000000;  // 60s
    constexpr size_t minNEvent = 50;

    SensorInfo sensor = defaultSensorByType(type);

    if (!isValidType(sensor.type)) {
        // no default sensor of this type
        return;
    }

    int32_t handle = sensor.sensorHandle;
    int64_t minSamplingPeriodInNs = sensor.minDelay * 1000ll;
    int64_t maxSamplingPeriodInNs = sensor.maxDelay * 1000ll;

    if (minSamplingPeriodInNs == maxSamplingPeriodInNs) {
        // only support single rate
        return;
    }

    int64_t firstCollectionPeriod = fastToSlow ? minSamplingPeriodInNs : maxSamplingPeriodInNs;
    int64_t secondCollectionPeriod = !fastToSlow ? minSamplingPeriodInNs : maxSamplingPeriodInNs;

    // first collection
    ASSERT_EQ(batch(handle, firstCollectionPeriod, batchingPeriodInNs), Result::OK);
    ASSERT_EQ(activate(handle, 1), Result::OK);

    usleep(500000);  // sleep 0.5 sec to wait for change rate to happen
    events1 = collectEvents(collectionTimeoutUs, minNEvent);

    // second collection, without stop sensor
    ASSERT_EQ(batch(handle, secondCollectionPeriod, batchingPeriodInNs), Result::OK);

    usleep(500000);  // sleep 0.5 sec to wait for change rate to happen
    events2 = collectEvents(collectionTimeoutUs, minNEvent);

    // end of collection, stop sensor
    ASSERT_EQ(activate(handle, 0), Result::OK);

    ALOGI("Collected %zu fast samples and %zu slow samples", events1.size(), events2.size());

    ASSERT_GT(events1.size(), 0u);
    ASSERT_GT(events2.size(), 0u);

    int64_t minDelayAverageInterval, maxDelayAverageInterval;
    std::vector<Event>& minDelayEvents(fastToSlow ? events1 : events2);
    std::vector<Event>& maxDelayEvents(fastToSlow ? events2 : events1);

    size_t nEvent = 0;
    int64_t prevTimestamp = -1;
    int64_t timestampInterval = 0;
    for (auto& e : minDelayEvents) {
        if (e.sensorType == type) {
            ASSERT_EQ(e.sensorHandle, handle);
            if (prevTimestamp > 0) {
                timestampInterval += e.timestamp - prevTimestamp;
            }
            prevTimestamp = e.timestamp;
            ++nEvent;
        }
    }
    ASSERT_GT(nEvent, 2u);
    minDelayAverageInterval = timestampInterval / (nEvent - 1);

    nEvent = 0;
    prevTimestamp = -1;
    timestampInterval = 0;
    for (auto& e : maxDelayEvents) {
        if (e.sensorType == type) {
            ASSERT_EQ(e.sensorHandle, handle);
            if (prevTimestamp > 0) {
                timestampInterval += e.timestamp - prevTimestamp;
            }
            prevTimestamp = e.timestamp;
            ++nEvent;
        }
    }
    ASSERT_GT(nEvent, 2u);
    maxDelayAverageInterval = timestampInterval / (nEvent - 1);

    // change of rate is significant.
    ALOGI("min/maxDelayAverageInterval = %" PRId64 " %" PRId64, minDelayAverageInterval,
          maxDelayAverageInterval);
    EXPECT_GT((maxDelayAverageInterval - minDelayAverageInterval), minDelayAverageInterval / 10);

    // fastest rate sampling time is close to spec
    EXPECT_LT(std::abs(minDelayAverageInterval - minSamplingPeriodInNs),
              minSamplingPeriodInNs / 10);

    // slowest rate sampling time is close to spec
    EXPECT_LT(std::abs(maxDelayAverageInterval - maxSamplingPeriodInNs),
              maxSamplingPeriodInNs / 10);
}

void SensorsHidlTestBase::testBatchingOperation(SensorType type) {
    std::vector<Event> events;

    constexpr int64_t maxBatchingTestTimeNs = 30ull * 1000 * 1000 * 1000;
    constexpr int64_t oneSecondInNs = 1ull * 1000 * 1000 * 1000;

    SensorInfo sensor = defaultSensorByType(type);

    if (!isValidType(sensor.type)) {
        // no default sensor of this type
        return;
    }

    int32_t handle = sensor.sensorHandle;
    int64_t minSamplingPeriodInNs = sensor.minDelay * 1000ll;
    uint32_t minFifoCount = sensor.fifoReservedEventCount;
    int64_t batchingPeriodInNs = minFifoCount * minSamplingPeriodInNs;

    if (batchingPeriodInNs < oneSecondInNs) {
        // batching size too small to test reliably
        return;
    }

    batchingPeriodInNs = std::min(batchingPeriodInNs, maxBatchingTestTimeNs);

    ALOGI("Test batching for %d ms", (int)(batchingPeriodInNs / 1000 / 1000));

    int64_t allowedBatchDeliverTimeNs = std::max(oneSecondInNs, batchingPeriodInNs / 10);

    ASSERT_EQ(batch(handle, minSamplingPeriodInNs, INT64_MAX), Result::OK);
    ASSERT_EQ(activate(handle, 1), Result::OK);

    usleep(500000);  // sleep 0.5 sec to wait for initialization
    ASSERT_EQ(flush(handle), Result::OK);

    // wait for 80% of the reserved batching period
    // there should not be any significant amount of events
    // since collection is not enabled all events will go down the drain
    usleep(batchingPeriodInNs / 1000 * 8 / 10);

    getEnvironment()->setCollection(true);
    // clean existing collections
    collectEvents(0 /*timeLimitUs*/, 0 /*nEventLimit*/, true /*clearBeforeStart*/,
                  false /*change collection*/);

    // 0.8 + 0.2 times the batching period
    usleep(batchingPeriodInNs / 1000 * 8 / 10);
    ASSERT_EQ(flush(handle), Result::OK);

    // plus some time for the event to deliver
    events = collectEvents(allowedBatchDeliverTimeNs / 1000, minFifoCount,
                           false /*clearBeforeStart*/, false /*change collection*/);

    getEnvironment()->setCollection(false);
    ASSERT_EQ(activate(handle, 0), Result::OK);

    size_t nEvent = 0;
    for (auto& e : events) {
        if (e.sensorType == type && e.sensorHandle == handle) {
            ++nEvent;
        }
    }

    // at least reach 90% of advertised capacity
    ASSERT_GT(nEvent, (size_t)(minFifoCount * 9 / 10));
}
