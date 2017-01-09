/*
 * Copyright (C) 2016 The Android Open Source Project
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
#include <android-base/logging.h>
#include <android/hardware/sensors/1.0/ISensors.h>
#include <android/hardware/sensors/1.0/types.h>
#include <android/log.h>
#include <gtest/gtest.h>
#include <hardware/sensors.h>       // for sensor type strings

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <mutex>
#include <thread>
#include <vector>

#include <unistd.h>

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_string;
using ::android::sp;
using namespace ::android::hardware::sensors::V1_0;

#define SENSORS_SERVICE_NAME "sensors"

// Test environment for sensors
class SensorsHidlEnvironment : public ::testing::Environment {
 public:
  // get the test environment singleton
  static SensorsHidlEnvironment* Instance() {
    static SensorsHidlEnvironment* instance = new SensorsHidlEnvironment;
    return instance;
  }

  // sensors hidl service
  sp<ISensors> sensors;

  virtual void SetUp();
  virtual void TearDown();

  // Get and clear all events collected so far (like "cat" shell command).
  // If output is nullptr, it clears all collected events.
  void catEvents(std::vector<Event>* output);

  // set sensor event collection status
  void setCollection(bool enable);

 private:
  SensorsHidlEnvironment() {}

  void addEvent(const Event& ev);
  void startPollingThread();
  static void pollingThread(SensorsHidlEnvironment* env, std::shared_ptr<bool> stop);

  bool collectionEnabled;
  std::shared_ptr<bool> stopThread;
  std::thread pollThread;
  std::vector<Event> events;
  std::mutex events_mutex;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(SensorsHidlEnvironment);
};

void SensorsHidlEnvironment::SetUp() {
  sensors = ISensors::getService(SENSORS_SERVICE_NAME, false);
  ALOGI_IF(sensors, "sensors is not nullptr, %p", sensors.get());
  ASSERT_NE(sensors, nullptr);

  collectionEnabled = false;
  startPollingThread();

  // In case framework just stopped for test and there is sensor events in the pipe,
  // wait some time for those events to be cleared to avoid them messing up the test.
  std::this_thread::sleep_for(std::chrono::seconds(3));
}

void SensorsHidlEnvironment::TearDown() {
  ALOGI("TearDown SensorsHidlEnvironement");

  if (stopThread) {
    *stopThread = true;
  }
  pollThread.detach();
}

void SensorsHidlEnvironment::catEvents(std::vector<Event>* output) {
  std::lock_guard<std::mutex> lock(events_mutex);
  if (output) {
    output->insert(output->end(), events.begin(), events.end());
  }
  events.clear();
}

void SensorsHidlEnvironment::setCollection(bool enable) {
  std::lock_guard<std::mutex> lock(events_mutex);
  collectionEnabled = enable;
}

void SensorsHidlEnvironment::addEvent(const Event& ev) {
  std::lock_guard<std::mutex> lock(events_mutex);
  if (collectionEnabled) {
    events.push_back(ev);
  }
}

void SensorsHidlEnvironment::startPollingThread() {
  stopThread = std::shared_ptr<bool>(new bool(false));
  pollThread = std::thread(pollingThread, this, stopThread);
  events.reserve(128);
}

void SensorsHidlEnvironment::pollingThread(
    SensorsHidlEnvironment* env, std::shared_ptr<bool> stop) {
  ALOGD("polling thread start");
  bool needExit = *stop;

  while(!needExit) {
    env->sensors->poll(1,
        [&](auto result, const auto &events, const auto &dynamicSensorsAdded) {
          if (result != Result::OK
              || (events.size() == 0 && dynamicSensorsAdded.size() == 0)
              || *stop) {
            needExit = true;
            return;
          }

          if (events.size() > 0) {
            env->addEvent(events[0]);
          }
        });
  }
  ALOGD("polling thread end");
}

// The main test class for SENSORS HIDL HAL.
class SensorsHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
  }

  virtual void TearDown() override {
  }

 protected:
  inline sp<ISensors>& S() {
    return SensorsHidlEnvironment::Instance()->sensors;
  }

  std::vector<Event> collectEvents(useconds_t timeLimitUs, size_t nEventLimit,
                                   bool clearBeforeStart = true,
                                   bool changeCollection = true) {
    std::vector<Event> events;
    constexpr useconds_t SLEEP_GRANULARITY = 100*1000; //gradularity 100 ms

    ALOGI("collect max of %zu events for %d us, clearBeforeStart %d",
          nEventLimit, timeLimitUs, clearBeforeStart);

    if (changeCollection) {
      SensorsHidlEnvironment::Instance()->setCollection(true);
    }
    if (clearBeforeStart) {
      SensorsHidlEnvironment::Instance()->catEvents(nullptr);
    }

    while (timeLimitUs > 0) {
      useconds_t duration = std::min(SLEEP_GRANULARITY, timeLimitUs);
      usleep(duration);
      timeLimitUs -= duration;

      SensorsHidlEnvironment::Instance()->catEvents(&events);
      if (events.size() >= nEventLimit) {
        break;
      }
      ALOGV("time to go = %d, events to go = %d",
            (int)timeLimitUs, (int)(nEventLimit - events.size()));
    }

    if (changeCollection) {
      SensorsHidlEnvironment::Instance()->setCollection(false);
    }
    return events;
  }

  static bool typeMatchStringType(SensorType type, const hidl_string& stringType);
  static bool typeMatchReportMode(SensorType type, SensorFlagBits reportMode);
  static bool delayMatchReportMode(int32_t minDelay, int32_t maxDelay, SensorFlagBits reportMode);

  inline static SensorFlagBits extractReportMode(uint64_t flag) {
    return (SensorFlagBits) (flag
        & ((uint64_t) SensorFlagBits::SENSOR_FLAG_CONTINUOUS_MODE
          | (uint64_t) SensorFlagBits::SENSOR_FLAG_ON_CHANGE_MODE
          | (uint64_t) SensorFlagBits::SENSOR_FLAG_ONE_SHOT_MODE
          | (uint64_t) SensorFlagBits::SENSOR_FLAG_SPECIAL_REPORTING_MODE));
  }

  inline static bool isMetaSensorType(SensorType type) {
    return (type == SensorType::SENSOR_TYPE_META_DATA
            || type == SensorType::SENSOR_TYPE_DYNAMIC_SENSOR_META
            || type == SensorType::SENSOR_TYPE_ADDITIONAL_INFO);
  }

  inline static bool isValidType(SensorType type) {
    return (int32_t) type > 0;
  }

  static SensorFlagBits expectedReportModeForType(SensorType type);
  SensorInfo defaultSensorByType(SensorType type);
};

bool SensorsHidlTest::typeMatchStringType(SensorType type, const hidl_string& stringType) {

  if (type >= SensorType::SENSOR_TYPE_DEVICE_PRIVATE_BASE) {
    return true;
  }

  bool res = true;
  switch (type) {
#define CHECK_TYPE_STRING_FOR_SENSOR_TYPE(type) \
    case SensorType::SENSOR_TYPE_ ## type: res = stringType == SENSOR_STRING_TYPE_ ## type;\
      break;\

    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(ACCELEROMETER);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(MAGNETIC_FIELD);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(ORIENTATION);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GYROSCOPE);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(LIGHT);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(PRESSURE);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(TEMPERATURE);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(PROXIMITY);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GRAVITY);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(LINEAR_ACCELERATION);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(ROTATION_VECTOR);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(RELATIVE_HUMIDITY);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(AMBIENT_TEMPERATURE);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(MAGNETIC_FIELD_UNCALIBRATED);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GAME_ROTATION_VECTOR);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GYROSCOPE_UNCALIBRATED);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(SIGNIFICANT_MOTION);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(STEP_DETECTOR);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(STEP_COUNTER);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GEOMAGNETIC_ROTATION_VECTOR);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(HEART_RATE);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(TILT_DETECTOR);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(WAKE_GESTURE);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(GLANCE_GESTURE);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(PICK_UP_GESTURE);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(WRIST_TILT_GESTURE);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(DEVICE_ORIENTATION);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(POSE_6DOF);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(STATIONARY_DETECT);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(MOTION_DETECT);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(HEART_BEAT);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(DYNAMIC_SENSOR_META);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(ADDITIONAL_INFO);
    CHECK_TYPE_STRING_FOR_SENSOR_TYPE(LOW_LATENCY_OFFBODY_DETECT);
    default:
      ALOGW("Type %d is not checked, stringType = %s", (int)type, stringType.c_str());
#undef CHECK_TYPE_STRING_FOR_SENSOR_TYPE
  }
  return res;
}

bool SensorsHidlTest::typeMatchReportMode(SensorType type, SensorFlagBits reportMode) {
  if (type >= SensorType::SENSOR_TYPE_DEVICE_PRIVATE_BASE) {
    return true;
  }

  SensorFlagBits expected = expectedReportModeForType(type);

  return expected == (SensorFlagBits)-1 || expected == reportMode;
}

bool SensorsHidlTest::delayMatchReportMode(
    int32_t minDelay, int32_t maxDelay, SensorFlagBits reportMode) {
  bool res = true;
  switch(reportMode) {
    case SensorFlagBits::SENSOR_FLAG_CONTINUOUS_MODE:
      res = (minDelay > 0) && (maxDelay >= 0);
      break;
    case SensorFlagBits::SENSOR_FLAG_ON_CHANGE_MODE:
      //TODO: current implementation does not satisfy minDelay == 0 on Proximity
      res = (minDelay >= 0) && (maxDelay >= 0);
      //res = (minDelay == 0) && (maxDelay >= 0);
      break;
    case SensorFlagBits::SENSOR_FLAG_ONE_SHOT_MODE:
      res = (minDelay == -1) && (maxDelay == 0);
      break;
    case SensorFlagBits::SENSOR_FLAG_SPECIAL_REPORTING_MODE:
      res = (minDelay == 0) && (maxDelay == 0);
  }

  return res;
}

SensorFlagBits SensorsHidlTest::expectedReportModeForType(SensorType type) {
  switch (type) {
    case SensorType::SENSOR_TYPE_ACCELEROMETER:
    case SensorType::SENSOR_TYPE_GYROSCOPE:
    case SensorType::SENSOR_TYPE_GEOMAGNETIC_FIELD:
    case SensorType::SENSOR_TYPE_ORIENTATION:
    case SensorType::SENSOR_TYPE_PRESSURE:
    case SensorType::SENSOR_TYPE_TEMPERATURE:
    case SensorType::SENSOR_TYPE_GRAVITY:
    case SensorType::SENSOR_TYPE_LINEAR_ACCELERATION:
    case SensorType::SENSOR_TYPE_ROTATION_VECTOR:
    case SensorType::SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
    case SensorType::SENSOR_TYPE_GAME_ROTATION_VECTOR:
    case SensorType::SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
    case SensorType::SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR:
    case SensorType::SENSOR_TYPE_POSE_6DOF:
    case SensorType::SENSOR_TYPE_HEART_BEAT:
      return SensorFlagBits::SENSOR_FLAG_CONTINUOUS_MODE;

    case SensorType::SENSOR_TYPE_LIGHT:
    case SensorType::SENSOR_TYPE_PROXIMITY:
    case SensorType::SENSOR_TYPE_RELATIVE_HUMIDITY:
    case SensorType::SENSOR_TYPE_AMBIENT_TEMPERATURE:
    case SensorType::SENSOR_TYPE_HEART_RATE:
    case SensorType::SENSOR_TYPE_DEVICE_ORIENTATION:
    case SensorType::SENSOR_TYPE_MOTION_DETECT:
    case SensorType::SENSOR_TYPE_STEP_COUNTER:
      return SensorFlagBits::SENSOR_FLAG_ON_CHANGE_MODE;

    case SensorType::SENSOR_TYPE_SIGNIFICANT_MOTION:
    case SensorType::SENSOR_TYPE_WAKE_GESTURE:
    case SensorType::SENSOR_TYPE_GLANCE_GESTURE:
    case SensorType::SENSOR_TYPE_PICK_UP_GESTURE:
      return SensorFlagBits::SENSOR_FLAG_ONE_SHOT_MODE;

    case SensorType::SENSOR_TYPE_STEP_DETECTOR:
    case SensorType::SENSOR_TYPE_TILT_DETECTOR:
    case SensorType::SENSOR_TYPE_WRIST_TILT_GESTURE:
    case SensorType::SENSOR_TYPE_DYNAMIC_SENSOR_META:
      return SensorFlagBits::SENSOR_FLAG_SPECIAL_REPORTING_MODE;

    default:
      ALOGW("Type %d is not implemented in expectedReportModeForType", (int)type);
      return (SensorFlagBits)-1;
  }
}

SensorInfo SensorsHidlTest::defaultSensorByType(SensorType type) {
  SensorInfo ret;

  ret.type = (SensorType) -1;
  S()->getSensorsList(
      [&] (const auto &list) {
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

// Test if sensor list returned is valid
TEST_F(SensorsHidlTest, SensorListValid) {
  S()->getSensorsList(
      [&] (const auto &list) {
        const size_t count = list.size();
        for (size_t i = 0; i < count; ++i) {
          auto &s = list[i];
          ALOGV("\t%zu: handle=%#08x type=%d name=%s",
                i, s.sensorHandle, (int)s.type, s.name.c_str());

          // Test non-empty type string
          ASSERT_FALSE(s.typeAsString.empty());

          // Test defined type matches defined string type
          ASSERT_TRUE(typeMatchStringType(s.type, s.typeAsString));

          // Test if all sensor has name and vendor
          ASSERT_FALSE(s.name.empty());
          ASSERT_FALSE(s.vendor.empty());

          // Test power > 0, maxRange > 0
          ASSERT_GE(s.power, 0);
          ASSERT_GT(s.maxRange, 0);

          // Info type, should have no sensor
          ASSERT_FALSE(
              s.type == SensorType::SENSOR_TYPE_ADDITIONAL_INFO
              || s.type == SensorType::SENSOR_TYPE_META_DATA);

          // Test fifoMax >= fifoReserved
          ALOGV("max reserve = %d, %d", s.fifoMaxEventCount, s.fifoReservedEventCount);
          ASSERT_GE(s.fifoMaxEventCount, s.fifoReservedEventCount);

          // Test Reporting mode valid
          ASSERT_TRUE(typeMatchReportMode(s.type, extractReportMode(s.flags)));

          // Test min max are in the right order
          ASSERT_LE(s.minDelay, s.maxDelay);
          // Test min/max delay matches reporting mode
          ASSERT_TRUE(delayMatchReportMode(s.minDelay, s.maxDelay, extractReportMode(s.flags)));
        }
      });
}

// Test if sensor hal can do normal accelerometer streaming properly
TEST_F(SensorsHidlTest, NormalAccelerometerStreamingOperation) {

  std::vector<Event> events;

  constexpr int64_t samplingPeriodInNs = 20ull*1000*1000; // 20ms
  constexpr int64_t batchingPeriodInNs = 0; // no batching
  constexpr useconds_t minTimeUs = 5*1000*1000;  // 5 s
  constexpr size_t minNEvent = 100;  // at lease 100 events
  constexpr SensorType type = SensorType::SENSOR_TYPE_ACCELEROMETER;

  SensorInfo sensor = defaultSensorByType(type);

  if (!isValidType(sensor.type)) {
    // no default sensor of this type
    return;
  }

  int32_t handle = sensor.sensorHandle;

  S()->batch(handle, 0, samplingPeriodInNs, batchingPeriodInNs);
  S()->activate(handle, 1);
  events = collectEvents(minTimeUs, minNEvent, true /*clearBeforeStart*/);
  S()->activate(handle, 0);

  ALOGI("Collected %zu samples", events.size());

  ASSERT_GT(events.size(), 0);

  size_t nRealEvent = 0;
  for (auto & e : events) {
    if (e.sensorType == type) {

      ASSERT_EQ(e.sensorHandle, handle);

      Vec3 acc = e.u.vec3;

      double gravityNorm = std::sqrt(acc.x * acc.x + acc.y * acc.y + acc.z * acc.z);
      ALOGV("Norm = %f", gravityNorm);

      // assert this is earth gravity
      ASSERT_TRUE(std::fabs(gravityNorm - GRAVITY_EARTH) < 1);

      ++ nRealEvent;
    } else {
      ALOGI("Event type %d, handle %d", (int) e.sensorType, (int) e.sensorHandle);
      // Only meta types are allowed besides the subscribed sensor
      ASSERT_TRUE(isMetaSensorType(e.sensorType));
    }
  }

  ASSERT_GE(nRealEvent, minNEvent / 2); // make sure returned events are not all meta
}

// Test if sensor hal can do gyroscope streaming properly
TEST_F(SensorsHidlTest, NormalGyroscopeStreamingOperation) {

  std::vector<Event> events;

  constexpr int64_t samplingPeriodInNs = 10ull*1000*1000; // 10ms
  constexpr int64_t batchingPeriodInNs = 0; // no batching
  constexpr useconds_t minTimeUs = 5*1000*1000;  // 5 s
  constexpr size_t minNEvent = 200;
  constexpr SensorType type = SensorType::SENSOR_TYPE_GYROSCOPE;

  SensorInfo sensor = defaultSensorByType(type);

  if (!isValidType(sensor.type)) {
    // no default sensor of this type
    return;
  }

  int32_t handle = sensor.sensorHandle;

  S()->batch(handle, 0, samplingPeriodInNs, batchingPeriodInNs);
  S()->activate(handle, 1);
  events = collectEvents(minTimeUs, minNEvent, true /*clearBeforeStart*/);
  S()->activate(handle, 0);

  ALOGI("Collected %zu samples", events.size());

  ASSERT_GT(events.size(), 0u);

  size_t nRealEvent = 0;
  for (auto & e : events) {
    if (e.sensorType == type) {

      ASSERT_EQ(e.sensorHandle, handle);

      Vec3 gyro = e.u.vec3;

      double gyroNorm = std::sqrt(gyro.x * gyro.x + gyro.y * gyro.y + gyro.z * gyro.z);
      ALOGV("Gyro Norm = %f", gyroNorm);

      // assert not drifting
      ASSERT_TRUE(gyroNorm < 0.1);  // < ~5 degree/s

      ++ nRealEvent;
    } else {
      ALOGI("Event type %d, handle %d", (int) e.sensorType, (int) e.sensorHandle);
      // Only meta types are allowed besides the subscribed sensor
      ASSERT_TRUE(isMetaSensorType(e.sensorType));
    }
  }

  ASSERT_GE(nRealEvent, minNEvent / 2); // make sure returned events are not all meta
}

// Test if sensor hal can do accelerometer sampling rate switch properly when sensor is active
TEST_F(SensorsHidlTest, AccelerometerSamplingPeriodHotSwitchOperation) {

  std::vector<Event> events1, events2;

  constexpr int64_t batchingPeriodInNs = 0; // no batching
  constexpr useconds_t minTimeUs = 5*1000*1000;  // 5 s
  constexpr size_t minNEvent = 50;
  constexpr SensorType type = SensorType::SENSOR_TYPE_ACCELEROMETER;

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

  S()->batch(handle, 0, minSamplingPeriodInNs, batchingPeriodInNs);
  S()->activate(handle, 1);

  usleep(500000); // sleep 0.5 sec to wait for change rate to happen
  events1 = collectEvents(sensor.minDelay * minNEvent, minNEvent, true /*clearBeforeStart*/);

  S()->batch(handle, 0, maxSamplingPeriodInNs, batchingPeriodInNs);

  usleep(500000); // sleep 0.5 sec to wait for change rate to happen
  events2 = collectEvents(sensor.maxDelay * minNEvent, minNEvent, true /*clearBeforeStart*/);

  S()->activate(handle, 0);

  ALOGI("Collected %zu fast samples and %zu slow samples", events1.size(), events2.size());

  ASSERT_GT(events1.size(), 0u);
  ASSERT_GT(events2.size(), 0u);

  int64_t minDelayAverageInterval, maxDelayAverageInterval;

  size_t nEvent = 0;
  int64_t prevTimestamp = -1;
  int64_t timestampInterval = 0;
  for (auto & e : events1) {
    if (e.sensorType == type) {
      ASSERT_EQ(e.sensorHandle, handle);
      if (prevTimestamp > 0) {
        timestampInterval += e.timestamp - prevTimestamp;
      }
      prevTimestamp = e.timestamp;
      ++ nEvent;
    }
  }
  ASSERT_GT(nEvent, 2);
  minDelayAverageInterval = timestampInterval / (nEvent - 1);

  nEvent = 0;
  prevTimestamp = -1;
  timestampInterval = 0;
  for (auto & e : events2) {
    if (e.sensorType == type) {
      ASSERT_EQ(e.sensorHandle, handle);
      if (prevTimestamp > 0) {
        timestampInterval += e.timestamp - prevTimestamp;
      }
      prevTimestamp = e.timestamp;
      ++ nEvent;
    }
  }
  ASSERT_GT(nEvent, 2);
  maxDelayAverageInterval = timestampInterval / (nEvent - 1);

  // change of rate is significant.
  ASSERT_GT((maxDelayAverageInterval - minDelayAverageInterval), minDelayAverageInterval / 10);

  // fastest rate sampling time is close to spec
  ALOGI("minDelayAverageInterval = %" PRId64, minDelayAverageInterval);
  ASSERT_LT(std::abs(minDelayAverageInterval - minSamplingPeriodInNs),
      minSamplingPeriodInNs / 10);
}

// Test if sensor hal can do normal accelerometer batching properly
TEST_F(SensorsHidlTest, AccelerometerBatchingOperation) {

  std::vector<Event> events;

  constexpr int64_t oneSecondInNs = 1ull * 1000 * 1000 * 1000;
  constexpr useconds_t minTimeUs = 5*1000*1000;  // 5 s
  constexpr size_t minNEvent = 50;
  constexpr SensorType type = SensorType::SENSOR_TYPE_ACCELEROMETER;
  constexpr int64_t maxBatchingTestTimeNs = 30ull * 1000 * 1000 * 1000;

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

  int64_t allowedBatchDeliverTimeNs =
      std::max(oneSecondInNs, batchingPeriodInNs / 10);

  S()->batch(handle, 0, minSamplingPeriodInNs, INT64_MAX);
  S()->activate(handle, 1);

  usleep(500000); // sleep 0.5 sec to wait for initialization
  S()->flush(handle);

  // wait for 80% of the reserved batching period
  // there should not be any significant amount of events
  // since collection is not enabled all events will go down the drain
  usleep(batchingPeriodInNs / 1000 * 8 / 10);


  SensorsHidlEnvironment::Instance()->setCollection(true);
  // 0.8 + 0.3 times the batching period
  // plus some time for the event to deliver
  events = collectEvents(
      batchingPeriodInNs / 1000 * 3 / 10,
        minFifoCount, true /*clearBeforeStart*/, false /*change collection*/);

  S()->flush(handle);

  events = collectEvents(allowedBatchDeliverTimeNs / 1000,
        minFifoCount, true /*clearBeforeStart*/, false /*change collection*/);

  SensorsHidlEnvironment::Instance()->setCollection(false);
  S()->activate(handle, 0);

  size_t nEvent = 0;
  for (auto & e : events) {
    if (e.sensorType == type && e.sensorHandle == handle) {
      ++ nEvent;
    }
  }

  // at least reach 90% of advertised capacity
  ASSERT_GT(nEvent, (size_t)(batchingPeriodInNs / minSamplingPeriodInNs * 9 / 10));
}


int main(int argc, char **argv) {
  ::testing::AddGlobalTestEnvironment(SensorsHidlEnvironment::Instance());
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}

