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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <aidl/android/hardware/sensors/BnSensors.h>
#include <aidl/android/hardware/sensors/ISensors.h>
#include <android/binder_manager.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <hardware/sensors.h>
#include <log/log.h>
#include <utils/SystemClock.h>

#include "SensorsAidlEnvironment.h"
#include "SensorsAidlTestSharedMemory.h"
#include "sensors-vts-utils/SensorsVtsEnvironmentBase.h"

#include <cinttypes>
#include <condition_variable>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using aidl::android::hardware::sensors::Event;
using aidl::android::hardware::sensors::ISensors;
using aidl::android::hardware::sensors::SensorInfo;
using aidl::android::hardware::sensors::SensorStatus;
using aidl::android::hardware::sensors::SensorType;
using android::ProcessState;
using std::chrono::duration_cast;

constexpr size_t kEventSize =
        static_cast<size_t>(ISensors::DIRECT_REPORT_SENSOR_EVENT_TOTAL_LENGTH);

namespace {

static void assertTypeMatchStringType(SensorType type, const std::string& stringType) {
    if (type >= SensorType::DEVICE_PRIVATE_BASE) {
        return;
    }

    switch (type) {
#define CHECK_TYPE_STRING_FOR_SENSOR_TYPE(type)                      \
    case SensorType::type:                                           \
        ASSERT_STREQ(SENSOR_STRING_TYPE_##type, stringType.c_str()); \
        break;
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(ACCELEROMETER);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(ACCELEROMETER_LIMITED_AXES);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(ACCELEROMETER_LIMITED_AXES_UNCALIBRATED);
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
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GYROSCOPE_LIMITED_AXES);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GYROSCOPE_LIMITED_AXES_UNCALIBRATED);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GYROSCOPE_UNCALIBRATED);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(HEADING);
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
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(TILT_DETECTOR);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(WAKE_GESTURE);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(WRIST_TILT_GESTURE);
        CHECK_TYPE_STRING_FOR_SENSOR_TYPE(HINGE_ANGLE);
        default:
            FAIL() << "Type " << static_cast<int>(type)
                   << " in android defined range is not checked, "
                   << "stringType = " << stringType;
#undef CHECK_TYPE_STRING_FOR_SENSOR_TYPE
    }
}

bool isDirectChannelTypeSupported(SensorInfo sensor, ISensors::SharedMemInfo::SharedMemType type) {
    switch (type) {
        case ISensors::SharedMemInfo::SharedMemType::ASHMEM:
            return (sensor.flags & SensorInfo::SENSOR_FLAG_BITS_DIRECT_CHANNEL_ASHMEM) != 0;
        case ISensors::SharedMemInfo::SharedMemType::GRALLOC:
            return (sensor.flags & SensorInfo::SENSOR_FLAG_BITS_DIRECT_CHANNEL_GRALLOC) != 0;
        default:
            return false;
    }
}

bool isDirectReportRateSupported(SensorInfo sensor, ISensors::RateLevel rate) {
    unsigned int r = static_cast<unsigned int>(sensor.flags &
                                               SensorInfo::SENSOR_FLAG_BITS_MASK_DIRECT_REPORT) >>
                     static_cast<unsigned int>(SensorInfo::SENSOR_FLAG_SHIFT_DIRECT_REPORT);
    return r >= static_cast<unsigned int>(rate);
}

int expectedReportModeForType(SensorType type) {
    switch (type) {
        case SensorType::ACCELEROMETER:
        case SensorType::ACCELEROMETER_LIMITED_AXES:
        case SensorType::ACCELEROMETER_UNCALIBRATED:
        case SensorType::ACCELEROMETER_LIMITED_AXES_UNCALIBRATED:
        case SensorType::GYROSCOPE:
        case SensorType::GYROSCOPE_LIMITED_AXES:
        case SensorType::MAGNETIC_FIELD:
        case SensorType::ORIENTATION:
        case SensorType::PRESSURE:
        case SensorType::GRAVITY:
        case SensorType::LINEAR_ACCELERATION:
        case SensorType::ROTATION_VECTOR:
        case SensorType::MAGNETIC_FIELD_UNCALIBRATED:
        case SensorType::GAME_ROTATION_VECTOR:
        case SensorType::GYROSCOPE_UNCALIBRATED:
        case SensorType::GYROSCOPE_LIMITED_AXES_UNCALIBRATED:
        case SensorType::GEOMAGNETIC_ROTATION_VECTOR:
        case SensorType::POSE_6DOF:
        case SensorType::HEART_BEAT:
        case SensorType::HEADING:
            return SensorInfo::SENSOR_FLAG_BITS_CONTINUOUS_MODE;

        case SensorType::LIGHT:
        case SensorType::PROXIMITY:
        case SensorType::RELATIVE_HUMIDITY:
        case SensorType::AMBIENT_TEMPERATURE:
        case SensorType::HEART_RATE:
        case SensorType::DEVICE_ORIENTATION:
        case SensorType::STEP_COUNTER:
        case SensorType::LOW_LATENCY_OFFBODY_DETECT:
            return SensorInfo::SENSOR_FLAG_BITS_ON_CHANGE_MODE;

        case SensorType::SIGNIFICANT_MOTION:
        case SensorType::WAKE_GESTURE:
        case SensorType::GLANCE_GESTURE:
        case SensorType::PICK_UP_GESTURE:
        case SensorType::MOTION_DETECT:
        case SensorType::STATIONARY_DETECT:
            return SensorInfo::SENSOR_FLAG_BITS_ONE_SHOT_MODE;

        case SensorType::STEP_DETECTOR:
        case SensorType::TILT_DETECTOR:
        case SensorType::WRIST_TILT_GESTURE:
        case SensorType::DYNAMIC_SENSOR_META:
            return SensorInfo::SENSOR_FLAG_BITS_SPECIAL_REPORTING_MODE;

        default:
            ALOGW("Type %d is not implemented in expectedReportModeForType", (int)type);
            return INT32_MAX;
    }
}

void assertTypeMatchReportMode(SensorType type, int reportMode) {
    if (type >= SensorType::DEVICE_PRIVATE_BASE) {
        return;
    }

    int expected = expectedReportModeForType(type);

    ASSERT_TRUE(expected == INT32_MAX || expected == reportMode)
            << "reportMode=" << static_cast<int>(reportMode)
            << "expected=" << static_cast<int>(expected);
}

void assertDelayMatchReportMode(int32_t minDelayUs, int32_t maxDelayUs, int reportMode) {
    switch (reportMode) {
        case SensorInfo::SENSOR_FLAG_BITS_CONTINUOUS_MODE:
            ASSERT_LT(0, minDelayUs);
            ASSERT_LE(0, maxDelayUs);
            break;
        case SensorInfo::SENSOR_FLAG_BITS_ON_CHANGE_MODE:
            ASSERT_LE(0, minDelayUs);
            ASSERT_LE(0, maxDelayUs);
            break;
        case SensorInfo::SENSOR_FLAG_BITS_ONE_SHOT_MODE:
            ASSERT_EQ(-1, minDelayUs);
            ASSERT_EQ(0, maxDelayUs);
            break;
        case SensorInfo::SENSOR_FLAG_BITS_SPECIAL_REPORTING_MODE:
            // do not enforce anything for special reporting mode
            break;
        default:
            FAIL() << "Report mode " << static_cast<int>(reportMode) << " not checked";
    }
}

void checkIsOk(ndk::ScopedAStatus status) {
    ASSERT_TRUE(status.isOk());
}

}  // namespace

class EventCallback : public IEventCallback<Event> {
  public:
    void reset() {
        mFlushMap.clear();
        mEventMap.clear();
    }

    void onEvent(const Event& event) override {
        if (event.sensorType == SensorType::META_DATA &&
            event.payload.get<Event::EventPayload::Tag::meta>().what ==
                    Event::EventPayload::MetaData::MetaDataEventType::META_DATA_FLUSH_COMPLETE) {
            std::unique_lock<std::recursive_mutex> lock(mFlushMutex);
            mFlushMap[event.sensorHandle]++;
            mFlushCV.notify_all();
        } else if (event.sensorType != SensorType::ADDITIONAL_INFO) {
            std::unique_lock<std::recursive_mutex> lock(mEventMutex);
            mEventMap[event.sensorHandle].push_back(event);
            mEventCV.notify_all();
        }
    }

    int32_t getFlushCount(int32_t sensorHandle) {
        std::unique_lock<std::recursive_mutex> lock(mFlushMutex);
        return mFlushMap[sensorHandle];
    }

    void waitForFlushEvents(const std::vector<SensorInfo>& sensorsToWaitFor,
                            int32_t numCallsToFlush, std::chrono::milliseconds timeout) {
        std::unique_lock<std::recursive_mutex> lock(mFlushMutex);
        mFlushCV.wait_for(lock, timeout,
                          [&] { return flushesReceived(sensorsToWaitFor, numCallsToFlush); });
    }

    const std::vector<Event> getEvents(int32_t sensorHandle) {
        std::unique_lock<std::recursive_mutex> lock(mEventMutex);
        return mEventMap[sensorHandle];
    }

    void waitForEvents(const std::vector<SensorInfo>& sensorsToWaitFor,
                       std::chrono::milliseconds timeout) {
        std::unique_lock<std::recursive_mutex> lock(mEventMutex);
        mEventCV.wait_for(lock, timeout, [&] { return eventsReceived(sensorsToWaitFor); });
    }

  protected:
    bool flushesReceived(const std::vector<SensorInfo>& sensorsToWaitFor, int32_t numCallsToFlush) {
        for (const SensorInfo& sensor : sensorsToWaitFor) {
            if (getFlushCount(sensor.sensorHandle) < numCallsToFlush) {
                return false;
            }
        }
        return true;
    }

    bool eventsReceived(const std::vector<SensorInfo>& sensorsToWaitFor) {
        for (const SensorInfo& sensor : sensorsToWaitFor) {
            if (getEvents(sensor.sensorHandle).size() == 0) {
                return false;
            }
        }
        return true;
    }

    std::map<int32_t, int32_t> mFlushMap;
    std::recursive_mutex mFlushMutex;
    std::condition_variable_any mFlushCV;

    std::map<int32_t, std::vector<Event>> mEventMap;
    std::recursive_mutex mEventMutex;
    std::condition_variable_any mEventCV;
};

class SensorsAidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mEnvironment = new SensorsAidlEnvironment(GetParam());
        mEnvironment->SetUp();

        // Ensure that we have a valid environment before performing tests
        ASSERT_NE(getSensors(), nullptr);
    }

    virtual void TearDown() override {
        for (int32_t handle : mSensorHandles) {
            activate(handle, false);
        }
        mSensorHandles.clear();

        mEnvironment->TearDown();
        delete mEnvironment;
        mEnvironment = nullptr;
    }

  protected:
    std::vector<SensorInfo> getNonOneShotSensors();
    std::vector<SensorInfo> getNonOneShotAndNonSpecialSensors();
    std::vector<SensorInfo> getNonOneShotAndNonOnChangeAndNonSpecialSensors();
    std::vector<SensorInfo> getOneShotSensors();
    std::vector<SensorInfo> getInjectEventSensors();

    void verifyDirectChannel(ISensors::SharedMemInfo::SharedMemType memType);

    void verifyRegisterDirectChannel(
            std::shared_ptr<SensorsAidlTestSharedMemory<SensorType, Event>> mem,
            int32_t* directChannelHandle, bool supportsSharedMemType,
            bool supportsAnyDirectChannel);

    void verifyConfigure(const SensorInfo& sensor, ISensors::SharedMemInfo::SharedMemType memType,
                         int32_t directChannelHandle, bool directChannelSupported);

    void queryDirectChannelSupport(ISensors::SharedMemInfo::SharedMemType memType,
                                   bool* supportsSharedMemType, bool* supportsAnyDirectChannel);

    void verifyUnregisterDirectChannel(int32_t* directChannelHandle, bool supportsAnyDirectChannel);

    void checkRateLevel(const SensorInfo& sensor, int32_t directChannelHandle,
                        ISensors::RateLevel rateLevel, int32_t* reportToken);

    inline std::shared_ptr<ISensors>& getSensors() { return mEnvironment->mSensors; }

    inline SensorsAidlEnvironment* getEnvironment() { return mEnvironment; }

    inline bool isValidType(SensorType sensorType) { return (int)sensorType > 0; }

    std::vector<SensorInfo> getSensorsList();

    int32_t getInvalidSensorHandle() {
        // Find a sensor handle that does not exist in the sensor list
        int32_t maxHandle = 0;
        for (const SensorInfo& sensor : getSensorsList()) {
            maxHandle = std::max(maxHandle, sensor.sensorHandle);
        }
        return maxHandle + 1;
    }

    ndk::ScopedAStatus activate(int32_t sensorHandle, bool enable);
    void activateAllSensors(bool enable);

    ndk::ScopedAStatus batch(int32_t sensorHandle, int64_t samplingPeriodNs,
                             int64_t maxReportLatencyNs) {
        return getSensors()->batch(sensorHandle, samplingPeriodNs, maxReportLatencyNs);
    }

    ndk::ScopedAStatus flush(int32_t sensorHandle) { return getSensors()->flush(sensorHandle); }

    ndk::ScopedAStatus registerDirectChannel(const ISensors::SharedMemInfo& mem,
                                             int32_t* aidlReturn);

    ndk::ScopedAStatus unregisterDirectChannel(int32_t* channelHandle) {
        return getSensors()->unregisterDirectChannel(*channelHandle);
    }

    ndk::ScopedAStatus configDirectReport(int32_t sensorHandle, int32_t channelHandle,
                                          ISensors::RateLevel rate, int32_t* reportToken) {
        return getSensors()->configDirectReport(sensorHandle, channelHandle, rate, reportToken);
    }

    void runSingleFlushTest(const std::vector<SensorInfo>& sensors, bool activateSensor,
                            int32_t expectedFlushCount, bool expectedResult);

    void runFlushTest(const std::vector<SensorInfo>& sensors, bool activateSensor,
                      int32_t flushCalls, int32_t expectedFlushCount, bool expectedResult);

    inline static int32_t extractReportMode(int32_t flag) {
        return (flag & (SensorInfo::SENSOR_FLAG_BITS_CONTINUOUS_MODE |
                        SensorInfo::SENSOR_FLAG_BITS_ON_CHANGE_MODE |
                        SensorInfo::SENSOR_FLAG_BITS_ONE_SHOT_MODE |
                        SensorInfo::SENSOR_FLAG_BITS_SPECIAL_REPORTING_MODE));
    }

    // All sensors and direct channnels used
    std::unordered_set<int32_t> mSensorHandles;
    std::unordered_set<int32_t> mDirectChannelHandles;

  private:
    SensorsAidlEnvironment* mEnvironment;
};

ndk::ScopedAStatus SensorsAidlTest::registerDirectChannel(const ISensors::SharedMemInfo& mem,
                                                          int32_t* aidlReturn) {
    // If registeration of a channel succeeds, add the handle of channel to a set so that it can be
    // unregistered when test fails. Unregister a channel does not remove the handle on purpose.
    // Unregistering a channel more than once should not have negative effect.

    ndk::ScopedAStatus status = getSensors()->registerDirectChannel(mem, aidlReturn);
    if (status.isOk()) {
        mDirectChannelHandles.insert(*aidlReturn);
    }
    return status;
}

std::vector<SensorInfo> SensorsAidlTest::getSensorsList() {
    std::vector<SensorInfo> sensorInfoList;
    checkIsOk(getSensors()->getSensorsList(&sensorInfoList));
    return sensorInfoList;
}

ndk::ScopedAStatus SensorsAidlTest::activate(int32_t sensorHandle, bool enable) {
    // If activating a sensor, add the handle in a set so that when test fails it can be turned off.
    // The handle is not removed when it is deactivating on purpose so that it is not necessary to
    // check the return value of deactivation. Deactivating a sensor more than once does not have
    // negative effect.
    if (enable) {
        mSensorHandles.insert(sensorHandle);
    }
    return getSensors()->activate(sensorHandle, enable);
}

void SensorsAidlTest::activateAllSensors(bool enable) {
    for (const SensorInfo& sensorInfo : getSensorsList()) {
        if (isValidType(sensorInfo.type)) {
            checkIsOk(batch(sensorInfo.sensorHandle, sensorInfo.minDelayUs,
                            0 /* maxReportLatencyNs */));
            checkIsOk(activate(sensorInfo.sensorHandle, enable));
        }
    }
}

std::vector<SensorInfo> SensorsAidlTest::getNonOneShotSensors() {
    std::vector<SensorInfo> sensors;
    for (const SensorInfo& info : getSensorsList()) {
        if (extractReportMode(info.flags) != SensorInfo::SENSOR_FLAG_BITS_ONE_SHOT_MODE) {
            sensors.push_back(info);
        }
    }
    return sensors;
}

std::vector<SensorInfo> SensorsAidlTest::getNonOneShotAndNonSpecialSensors() {
    std::vector<SensorInfo> sensors;
    for (const SensorInfo& info : getSensorsList()) {
        int reportMode = extractReportMode(info.flags);
        if (reportMode != SensorInfo::SENSOR_FLAG_BITS_ONE_SHOT_MODE &&
            reportMode != SensorInfo::SENSOR_FLAG_BITS_SPECIAL_REPORTING_MODE) {
            sensors.push_back(info);
        }
    }
    return sensors;
}

std::vector<SensorInfo> SensorsAidlTest::getNonOneShotAndNonOnChangeAndNonSpecialSensors() {
    std::vector<SensorInfo> sensors;
    for (const SensorInfo& info : getSensorsList()) {
        int reportMode = extractReportMode(info.flags);
        if (reportMode != SensorInfo::SENSOR_FLAG_BITS_ONE_SHOT_MODE &&
            reportMode != SensorInfo::SENSOR_FLAG_BITS_ON_CHANGE_MODE &&
            reportMode != SensorInfo::SENSOR_FLAG_BITS_SPECIAL_REPORTING_MODE) {
            sensors.push_back(info);
        }
    }
    return sensors;
}

std::vector<SensorInfo> SensorsAidlTest::getOneShotSensors() {
    std::vector<SensorInfo> sensors;
    for (const SensorInfo& info : getSensorsList()) {
        if (extractReportMode(info.flags) == SensorInfo::SENSOR_FLAG_BITS_ONE_SHOT_MODE) {
            sensors.push_back(info);
        }
    }
    return sensors;
}

std::vector<SensorInfo> SensorsAidlTest::getInjectEventSensors() {
    std::vector<SensorInfo> out;
    std::vector<SensorInfo> sensorInfoList = getSensorsList();
    for (const SensorInfo& info : sensorInfoList) {
        if (info.flags & SensorInfo::SENSOR_FLAG_BITS_DATA_INJECTION) {
            out.push_back(info);
        }
    }
    return out;
}

void SensorsAidlTest::runSingleFlushTest(const std::vector<SensorInfo>& sensors,
                                         bool activateSensor, int32_t expectedFlushCount,
                                         bool expectedResult) {
    runFlushTest(sensors, activateSensor, 1 /* flushCalls */, expectedFlushCount, expectedResult);
}

void SensorsAidlTest::runFlushTest(const std::vector<SensorInfo>& sensors, bool activateSensor,
                                   int32_t flushCalls, int32_t expectedFlushCount,
                                   bool expectedResult) {
    EventCallback callback;
    getEnvironment()->registerCallback(&callback);

    for (const SensorInfo& sensor : sensors) {
        // Configure and activate the sensor
        batch(sensor.sensorHandle, sensor.maxDelayUs, 0 /* maxReportLatencyNs */);
        activate(sensor.sensorHandle, activateSensor);

        // Flush the sensor
        for (int32_t i = 0; i < flushCalls; i++) {
            SCOPED_TRACE(::testing::Message()
                         << "Flush " << i << "/" << flushCalls << ": "
                         << " handle=0x" << std::hex << std::setw(8) << std::setfill('0')
                         << sensor.sensorHandle << std::dec
                         << " type=" << static_cast<int>(sensor.type) << " name=" << sensor.name);

            EXPECT_EQ(flush(sensor.sensorHandle).isOk(), expectedResult);
        }
    }

    // Wait up to one second for the flush events
    callback.waitForFlushEvents(sensors, flushCalls, std::chrono::milliseconds(1000) /* timeout */);

    // Deactivate all sensors after waiting for flush events so pending flush events are not
    // abandoned by the HAL.
    for (const SensorInfo& sensor : sensors) {
        activate(sensor.sensorHandle, false);
    }
    getEnvironment()->unregisterCallback();

    // Check that the correct number of flushes are present for each sensor
    for (const SensorInfo& sensor : sensors) {
        SCOPED_TRACE(::testing::Message()
                     << " handle=0x" << std::hex << std::setw(8) << std::setfill('0')
                     << sensor.sensorHandle << std::dec << " type=" << static_cast<int>(sensor.type)
                     << " name=" << sensor.name);
        ASSERT_EQ(callback.getFlushCount(sensor.sensorHandle), expectedFlushCount);
    }
}

TEST_P(SensorsAidlTest, SensorListValid) {
    std::vector<SensorInfo> sensorInfoList = getSensorsList();
    std::unordered_map<int32_t, std::vector<std::string>> sensorTypeNameMap;
    for (size_t i = 0; i < sensorInfoList.size(); ++i) {
        const SensorInfo& info = sensorInfoList[i];
        SCOPED_TRACE(::testing::Message()
                     << i << "/" << sensorInfoList.size() << ": "
                     << " handle=0x" << std::hex << std::setw(8) << std::setfill('0')
                     << info.sensorHandle << std::dec << " type=" << static_cast<int>(info.type)
                     << " name=" << info.name);

        // Test type string non-empty only for private sensor typeinfo.
        if (info.type >= SensorType::DEVICE_PRIVATE_BASE) {
            EXPECT_FALSE(info.typeAsString.empty());
        } else if (!info.typeAsString.empty()) {
            // Test type string matches framework string if specified for non-private typeinfo.
            EXPECT_NO_FATAL_FAILURE(assertTypeMatchStringType(info.type, info.typeAsString));
        }

        // Test if all sensor has name and vendor
        EXPECT_FALSE(info.name.empty());
        EXPECT_FALSE(info.vendor.empty());

        // Make sure that sensors of the same type have a unique name.
        std::vector<std::string>& v = sensorTypeNameMap[static_cast<int32_t>(info.type)];
        bool isUniqueName = std::find(v.begin(), v.end(), info.name) == v.end();
        EXPECT_TRUE(isUniqueName) << "Duplicate sensor Name: " << info.name;
        if (isUniqueName) {
            v.push_back(info.name);
        }

        EXPECT_LE(0, info.power);
        EXPECT_LT(0, info.maxRange);

        // Info type, should have no sensor
        EXPECT_FALSE(info.type == SensorType::ADDITIONAL_INFO ||
                     info.type == SensorType::META_DATA);

        EXPECT_GE(info.fifoMaxEventCount, info.fifoReservedEventCount);

        // Test Reporting mode valid
        EXPECT_NO_FATAL_FAILURE(
                assertTypeMatchReportMode(info.type, extractReportMode(info.flags)));

        // Test min max are in the right order
        EXPECT_LE(info.minDelayUs, info.maxDelayUs);
        // Test min/max delay matches reporting mode
        EXPECT_NO_FATAL_FAILURE(assertDelayMatchReportMode(info.minDelayUs, info.maxDelayUs,
                                                           extractReportMode(info.flags)));
    }
}

TEST_P(SensorsAidlTest, SetOperationMode) {
    if (getInjectEventSensors().size() > 0) {
        ASSERT_TRUE(getSensors()->setOperationMode(ISensors::OperationMode::NORMAL).isOk());
        ASSERT_TRUE(getSensors()->setOperationMode(ISensors::OperationMode::DATA_INJECTION).isOk());
        ASSERT_TRUE(getSensors()->setOperationMode(ISensors::OperationMode::NORMAL).isOk());
    } else {
      int errorCode =
          getSensors()
              ->setOperationMode(ISensors::OperationMode::DATA_INJECTION)
              .getExceptionCode();
      ASSERT_TRUE((errorCode == EX_UNSUPPORTED_OPERATION) ||
                  (errorCode == EX_ILLEGAL_ARGUMENT));
    }
}

TEST_P(SensorsAidlTest, InjectSensorEventData) {
    std::vector<SensorInfo> sensors = getInjectEventSensors();
    if (sensors.size() == 0) {
        return;
    }

    ASSERT_TRUE(getSensors()->setOperationMode(ISensors::OperationMode::DATA_INJECTION).isOk());

    EventCallback callback;
    getEnvironment()->registerCallback(&callback);

    // AdditionalInfo event should not be sent to Event FMQ
    Event additionalInfoEvent;
    additionalInfoEvent.sensorType = SensorType::ADDITIONAL_INFO;
    additionalInfoEvent.timestamp = android::elapsedRealtimeNano();

    Event injectedEvent;
    injectedEvent.timestamp = android::elapsedRealtimeNano();
    Event::EventPayload::Vec3 data = {1, 2, 3, SensorStatus::ACCURACY_HIGH};
    injectedEvent.payload.set<Event::EventPayload::Tag::vec3>(data);

    for (const auto& s : sensors) {
        additionalInfoEvent.sensorHandle = s.sensorHandle;
        ASSERT_TRUE(getSensors()->injectSensorData(additionalInfoEvent).isOk());

        injectedEvent.sensorType = s.type;
        injectedEvent.sensorHandle = s.sensorHandle;
        ASSERT_TRUE(getSensors()->injectSensorData(injectedEvent).isOk());
    }

    // Wait for events to be written back to the Event FMQ
    callback.waitForEvents(sensors, std::chrono::milliseconds(1000) /* timeout */);
    getEnvironment()->unregisterCallback();

    for (const auto& s : sensors) {
        auto events = callback.getEvents(s.sensorHandle);
        if (events.empty()) {
            FAIL() << "Received no events";
        } else {
            auto lastEvent = events.back();
            SCOPED_TRACE(::testing::Message()
                         << " handle=0x" << std::hex << std::setw(8) << std::setfill('0')
                         << s.sensorHandle << std::dec << " type=" << static_cast<int>(s.type)
                         << " name=" << s.name);

            // Verify that only a single event has been received
            ASSERT_EQ(events.size(), 1);

            // Verify that the event received matches the event injected and is not the additional
            // info event
            ASSERT_EQ(lastEvent.sensorType, s.type);
            ASSERT_EQ(lastEvent.timestamp, injectedEvent.timestamp);
            ASSERT_EQ(lastEvent.payload.get<Event::EventPayload::Tag::vec3>().x,
                      injectedEvent.payload.get<Event::EventPayload::Tag::vec3>().x);
            ASSERT_EQ(lastEvent.payload.get<Event::EventPayload::Tag::vec3>().y,
                      injectedEvent.payload.get<Event::EventPayload::Tag::vec3>().y);
            ASSERT_EQ(lastEvent.payload.get<Event::EventPayload::Tag::vec3>().z,
                      injectedEvent.payload.get<Event::EventPayload::Tag::vec3>().z);
            ASSERT_EQ(lastEvent.payload.get<Event::EventPayload::Tag::vec3>().status,
                      injectedEvent.payload.get<Event::EventPayload::Tag::vec3>().status);
        }
    }

    ASSERT_TRUE(getSensors()->setOperationMode(ISensors::OperationMode::NORMAL).isOk());
}

TEST_P(SensorsAidlTest, CallInitializeTwice) {
    // Create a helper class so that a second environment is able to be instantiated
    class SensorsAidlEnvironmentTest : public SensorsAidlEnvironment {
      public:
        SensorsAidlEnvironmentTest(const std::string& service_name)
            : SensorsAidlEnvironment(service_name) {}
    };

    if (getSensorsList().size() == 0) {
        // No sensors
        return;
    }

    constexpr useconds_t kCollectionTimeoutUs = 1000 * 1000;  // 1s
    constexpr int32_t kNumEvents = 1;

    // Create a new environment that calls initialize()
    std::unique_ptr<SensorsAidlEnvironmentTest> newEnv =
            std::make_unique<SensorsAidlEnvironmentTest>(GetParam());
    newEnv->SetUp();
    if (HasFatalFailure()) {
        return;  // Exit early if setting up the new environment failed
    }

    activateAllSensors(true);
    // Verify that the old environment does not receive any events
    EXPECT_EQ(getEnvironment()->collectEvents(kCollectionTimeoutUs, kNumEvents).size(), 0);
    // Verify that the new event queue receives sensor events
    EXPECT_GE(newEnv.get()->collectEvents(kCollectionTimeoutUs, kNumEvents).size(), kNumEvents);
    activateAllSensors(false);

    // Cleanup the test environment
    newEnv->TearDown();

    // Restore the test environment for future tests
    getEnvironment()->TearDown();
    getEnvironment()->SetUp();
    if (HasFatalFailure()) {
        return;  // Exit early if resetting the environment failed
    }

    // Ensure that the original environment is receiving events
    activateAllSensors(true);
    EXPECT_GE(getEnvironment()->collectEvents(kCollectionTimeoutUs, kNumEvents).size(), kNumEvents);
    activateAllSensors(false);
}

TEST_P(SensorsAidlTest, CleanupConnectionsOnInitialize) {
    activateAllSensors(true);

    // Verify that events are received
    constexpr useconds_t kCollectionTimeoutUs = 1000 * 1000;  // 1s
    constexpr int32_t kNumEvents = 1;
    ASSERT_GE(getEnvironment()->collectEvents(kCollectionTimeoutUs, kNumEvents).size(), kNumEvents);

    // Clear the active sensor handles so they are not disabled during TearDown
    auto handles = mSensorHandles;
    mSensorHandles.clear();
    getEnvironment()->TearDown();
    getEnvironment()->SetUp();
    if (HasFatalFailure()) {
        return;  // Exit early if resetting the environment failed
    }

    // Verify no events are received until sensors are re-activated
    ASSERT_EQ(getEnvironment()->collectEvents(kCollectionTimeoutUs, kNumEvents).size(), 0);
    activateAllSensors(true);
    ASSERT_GE(getEnvironment()->collectEvents(kCollectionTimeoutUs, kNumEvents).size(), kNumEvents);

    // Disable sensors
    activateAllSensors(false);

    // Restore active sensors prior to clearing the environment
    mSensorHandles = handles;
}

TEST_P(SensorsAidlTest, FlushSensor) {
    std::vector<SensorInfo> sensors = getNonOneShotSensors();
    if (sensors.size() == 0) {
        return;
    }

    constexpr int32_t kFlushes = 5;
    runSingleFlushTest(sensors, true /* activateSensor */, 1 /* expectedFlushCount */,
                       true /* expectedResult */);
    runFlushTest(sensors, true /* activateSensor */, kFlushes, kFlushes, true /* expectedResult */);
}

TEST_P(SensorsAidlTest, FlushOneShotSensor) {
    // Find a sensor that is a one-shot sensor
    std::vector<SensorInfo> sensors = getOneShotSensors();
    if (sensors.size() == 0) {
        return;
    }

    runSingleFlushTest(sensors, true /* activateSensor */, 0 /* expectedFlushCount */,
                       false /* expectedResult */);
}

TEST_P(SensorsAidlTest, FlushInactiveSensor) {
    // Attempt to find a non-one shot sensor, then a one-shot sensor if necessary
    std::vector<SensorInfo> sensors = getNonOneShotSensors();
    if (sensors.size() == 0) {
        sensors = getOneShotSensors();
        if (sensors.size() == 0) {
            return;
        }
    }

    runSingleFlushTest(sensors, false /* activateSensor */, 0 /* expectedFlushCount */,
                       false /* expectedResult */);
}

TEST_P(SensorsAidlTest, Batch) {
    if (getSensorsList().size() == 0) {
        return;
    }

    activateAllSensors(false /* enable */);
    for (const SensorInfo& sensor : getSensorsList()) {
        SCOPED_TRACE(::testing::Message()
                     << " handle=0x" << std::hex << std::setw(8) << std::setfill('0')
                     << sensor.sensorHandle << std::dec << " type=" << static_cast<int>(sensor.type)
                     << " name=" << sensor.name);

        // Call batch on inactive sensor
        // One shot sensors have minDelay set to -1 which is an invalid
        // parameter. Use 0 instead to avoid errors.
        int64_t samplingPeriodNs =
                extractReportMode(sensor.flags) == SensorInfo::SENSOR_FLAG_BITS_ONE_SHOT_MODE
                        ? 0
                        : sensor.minDelayUs;
        checkIsOk(batch(sensor.sensorHandle, samplingPeriodNs, 0 /* maxReportLatencyNs */));

        // Activate the sensor
        activate(sensor.sensorHandle, true /* enabled */);

        // Call batch on an active sensor
        checkIsOk(batch(sensor.sensorHandle, sensor.maxDelayUs, 0 /* maxReportLatencyNs */));
    }
    activateAllSensors(false /* enable */);

    // Call batch on an invalid sensor
    SensorInfo sensor = getSensorsList().front();
    sensor.sensorHandle = getInvalidSensorHandle();
    ASSERT_EQ(batch(sensor.sensorHandle, sensor.minDelayUs, 0 /* maxReportLatencyNs */)
                      .getExceptionCode(),
              EX_ILLEGAL_ARGUMENT);
}

TEST_P(SensorsAidlTest, Activate) {
    if (getSensorsList().size() == 0) {
        return;
    }

    // Verify that sensor events are generated when activate is called
    for (const SensorInfo& sensor : getSensorsList()) {
        SCOPED_TRACE(::testing::Message()
                     << " handle=0x" << std::hex << std::setw(8) << std::setfill('0')
                     << sensor.sensorHandle << std::dec << " type=" << static_cast<int>(sensor.type)
                     << " name=" << sensor.name);

        checkIsOk(batch(sensor.sensorHandle, sensor.minDelayUs, 0 /* maxReportLatencyNs */));
        checkIsOk(activate(sensor.sensorHandle, true));

        // Call activate on a sensor that is already activated
        checkIsOk(activate(sensor.sensorHandle, true));

        // Deactivate the sensor
        checkIsOk(activate(sensor.sensorHandle, false));

        // Call deactivate on a sensor that is already deactivated
        checkIsOk(activate(sensor.sensorHandle, false));
    }

    // Attempt to activate an invalid sensor
    int32_t invalidHandle = getInvalidSensorHandle();
    ASSERT_EQ(activate(invalidHandle, true).getExceptionCode(), EX_ILLEGAL_ARGUMENT);
    ASSERT_EQ(activate(invalidHandle, false).getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

TEST_P(SensorsAidlTest, NoStaleEvents) {
    constexpr std::chrono::milliseconds kFiveHundredMs(500);
    constexpr std::chrono::milliseconds kOneSecond(1000);

    // Register the callback to receive sensor events
    EventCallback callback;
    getEnvironment()->registerCallback(&callback);

    // This test is not valid for one-shot, on-change or special-report-mode sensors
    const std::vector<SensorInfo> sensors = getNonOneShotAndNonOnChangeAndNonSpecialSensors();
    std::chrono::milliseconds maxMinDelay(0);
    for (const SensorInfo& sensor : sensors) {
        std::chrono::milliseconds minDelay = duration_cast<std::chrono::milliseconds>(
                std::chrono::microseconds(sensor.minDelayUs));
        maxMinDelay = std::chrono::milliseconds(std::max(maxMinDelay.count(), minDelay.count()));
    }

    // Activate the sensors so that they start generating events
    activateAllSensors(true);

    // According to the CDD, the first sample must be generated within 400ms + 2 * sample_time
    // and the maximum reporting latency is 100ms + 2 * sample_time. Wait a sufficient amount
    // of time to guarantee that a sample has arrived.
    callback.waitForEvents(sensors, kFiveHundredMs + (5 * maxMinDelay));
    activateAllSensors(false);

    // Save the last received event for each sensor
    std::map<int32_t, int64_t> lastEventTimestampMap;
    for (const SensorInfo& sensor : sensors) {
        SCOPED_TRACE(::testing::Message()
                     << " handle=0x" << std::hex << std::setw(8) << std::setfill('0')
                     << sensor.sensorHandle << std::dec << " type=" << static_cast<int>(sensor.type)
                     << " name=" << sensor.name);

        if (callback.getEvents(sensor.sensorHandle).size() >= 1) {
            lastEventTimestampMap[sensor.sensorHandle] =
                    callback.getEvents(sensor.sensorHandle).back().timestamp;
        }
    }

    // Allow some time to pass, reset the callback, then reactivate the sensors
    usleep(duration_cast<std::chrono::microseconds>(kOneSecond + (5 * maxMinDelay)).count());
    callback.reset();
    activateAllSensors(true);
    callback.waitForEvents(sensors, kFiveHundredMs + (5 * maxMinDelay));
    activateAllSensors(false);

    getEnvironment()->unregisterCallback();

    for (const SensorInfo& sensor : sensors) {
        SCOPED_TRACE(::testing::Message()
                     << " handle=0x" << std::hex << std::setw(8) << std::setfill('0')
                     << sensor.sensorHandle << std::dec << " type=" << static_cast<int>(sensor.type)
                     << " name=" << sensor.name);

        // Skip sensors that did not previously report an event
        if (lastEventTimestampMap.find(sensor.sensorHandle) == lastEventTimestampMap.end()) {
            continue;
        }

        // Ensure that the first event received is not stale by ensuring that its timestamp is
        // sufficiently different from the previous event
        const Event newEvent = callback.getEvents(sensor.sensorHandle).front();
        std::chrono::milliseconds delta =
                duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(
                        newEvent.timestamp - lastEventTimestampMap[sensor.sensorHandle]));
        std::chrono::milliseconds sensorMinDelay = duration_cast<std::chrono::milliseconds>(
                std::chrono::microseconds(sensor.minDelayUs));
        ASSERT_GE(delta, kFiveHundredMs + (3 * sensorMinDelay));
    }
}

void SensorsAidlTest::checkRateLevel(const SensorInfo& sensor, int32_t directChannelHandle,
                                     ISensors::RateLevel rateLevel, int32_t* reportToken) {
    ndk::ScopedAStatus status =
            configDirectReport(sensor.sensorHandle, directChannelHandle, rateLevel, reportToken);

    SCOPED_TRACE(::testing::Message()
                 << " handle=0x" << std::hex << std::setw(8) << std::setfill('0')
                 << sensor.sensorHandle << std::dec << " type=" << static_cast<int>(sensor.type)
                 << " name=" << sensor.name);

    if (isDirectReportRateSupported(sensor, rateLevel)) {
        ASSERT_TRUE(status.isOk());
        if (rateLevel != ISensors::RateLevel::STOP) {
          ASSERT_GT(*reportToken, 0);
        }
    } else {
      ASSERT_EQ(status.getExceptionCode(), EX_ILLEGAL_ARGUMENT);
    }
}

void SensorsAidlTest::queryDirectChannelSupport(ISensors::SharedMemInfo::SharedMemType memType,
                                                bool* supportsSharedMemType,
                                                bool* supportsAnyDirectChannel) {
    *supportsSharedMemType = false;
    *supportsAnyDirectChannel = false;
    for (const SensorInfo& curSensor : getSensorsList()) {
        if (isDirectChannelTypeSupported(curSensor, memType)) {
            *supportsSharedMemType = true;
        }
        if (isDirectChannelTypeSupported(curSensor,
                                         ISensors::SharedMemInfo::SharedMemType::ASHMEM) ||
            isDirectChannelTypeSupported(curSensor,
                                         ISensors::SharedMemInfo::SharedMemType::GRALLOC)) {
            *supportsAnyDirectChannel = true;
        }

        if (*supportsSharedMemType && *supportsAnyDirectChannel) {
            break;
        }
    }
}

void SensorsAidlTest::verifyRegisterDirectChannel(
        std::shared_ptr<SensorsAidlTestSharedMemory<SensorType, Event>> mem,
        int32_t* directChannelHandle, bool supportsSharedMemType, bool supportsAnyDirectChannel) {
    char* buffer = mem->getBuffer();
    size_t size = mem->getSize();

    if (supportsSharedMemType) {
        memset(buffer, 0xff, size);
    }

    int32_t channelHandle;

    ::ndk::ScopedAStatus status = registerDirectChannel(mem->getSharedMemInfo(), &channelHandle);
    if (supportsSharedMemType) {
        ASSERT_TRUE(status.isOk());
        ASSERT_GT(channelHandle, 0);

        // Verify that the memory has been zeroed
        for (size_t i = 0; i < mem->getSize(); i++) {
          ASSERT_EQ(buffer[i], 0x00);
        }
    } else {
        int32_t error = supportsAnyDirectChannel ? EX_ILLEGAL_ARGUMENT : EX_UNSUPPORTED_OPERATION;
        ASSERT_EQ(status.getExceptionCode(), error);
    }
    *directChannelHandle = channelHandle;
}

void SensorsAidlTest::verifyUnregisterDirectChannel(int32_t* channelHandle,
                                                    bool supportsAnyDirectChannel) {
    int result = supportsAnyDirectChannel ? EX_NONE : EX_UNSUPPORTED_OPERATION;
    ndk::ScopedAStatus status = unregisterDirectChannel(channelHandle);
    ASSERT_EQ(status.getExceptionCode(), result);
}

void SensorsAidlTest::verifyDirectChannel(ISensors::SharedMemInfo::SharedMemType memType) {
    constexpr size_t kNumEvents = 1;
    constexpr size_t kMemSize = kNumEvents * kEventSize;

    std::shared_ptr<SensorsAidlTestSharedMemory<SensorType, Event>> mem(
            SensorsAidlTestSharedMemory<SensorType, Event>::create(memType, kMemSize));
    ASSERT_NE(mem, nullptr);

    bool supportsSharedMemType;
    bool supportsAnyDirectChannel;
    queryDirectChannelSupport(memType, &supportsSharedMemType, &supportsAnyDirectChannel);

    for (const SensorInfo& sensor : getSensorsList()) {
        int32_t directChannelHandle = 0;
        verifyRegisterDirectChannel(mem, &directChannelHandle, supportsSharedMemType,
                                    supportsAnyDirectChannel);
        verifyConfigure(sensor, memType, directChannelHandle, supportsAnyDirectChannel);
        verifyUnregisterDirectChannel(&directChannelHandle, supportsAnyDirectChannel);
    }
}

void SensorsAidlTest::verifyConfigure(const SensorInfo& sensor,
                                      ISensors::SharedMemInfo::SharedMemType memType,
                                      int32_t directChannelHandle, bool supportsAnyDirectChannel) {
    SCOPED_TRACE(::testing::Message()
                 << " handle=0x" << std::hex << std::setw(8) << std::setfill('0')
                 << sensor.sensorHandle << std::dec << " type=" << static_cast<int>(sensor.type)
                 << " name=" << sensor.name);

    int32_t reportToken = 0;
    if (isDirectChannelTypeSupported(sensor, memType)) {
        // Verify that each rate level is properly supported
        checkRateLevel(sensor, directChannelHandle, ISensors::RateLevel::NORMAL, &reportToken);
        checkRateLevel(sensor, directChannelHandle, ISensors::RateLevel::FAST, &reportToken);
        checkRateLevel(sensor, directChannelHandle, ISensors::RateLevel::VERY_FAST, &reportToken);
        checkRateLevel(sensor, directChannelHandle, ISensors::RateLevel::STOP, &reportToken);

        // Verify that a sensor handle of -1 is only acceptable when using RateLevel::STOP
        ndk::ScopedAStatus status = configDirectReport(-1 /* sensorHandle */, directChannelHandle,
                                                       ISensors::RateLevel::NORMAL, &reportToken);
        ASSERT_EQ(status.getExceptionCode(), EX_ILLEGAL_ARGUMENT);

        status = configDirectReport(-1 /* sensorHandle */, directChannelHandle,
                                    ISensors::RateLevel::STOP, &reportToken);
        ASSERT_TRUE(status.isOk());
    } else {
        // directChannelHandle will be -1 here, HAL should either reject it as a bad value if there
        // is some level of direct channel report, otherwise return INVALID_OPERATION if direct
        // channel is not supported at all
        int error = supportsAnyDirectChannel ? EX_ILLEGAL_ARGUMENT : EX_UNSUPPORTED_OPERATION;
        ndk::ScopedAStatus status = configDirectReport(sensor.sensorHandle, directChannelHandle,
                                                       ISensors::RateLevel::NORMAL, &reportToken);
        ASSERT_EQ(status.getExceptionCode(), error);
    }
}

TEST_P(SensorsAidlTest, DirectChannelAshmem) {
    verifyDirectChannel(ISensors::SharedMemInfo::SharedMemType::ASHMEM);
}

TEST_P(SensorsAidlTest, DirectChannelGralloc) {
    verifyDirectChannel(ISensors::SharedMemInfo::SharedMemType::GRALLOC);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SensorsAidlTest);
INSTANTIATE_TEST_SUITE_P(Sensors, SensorsAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ISensors::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
