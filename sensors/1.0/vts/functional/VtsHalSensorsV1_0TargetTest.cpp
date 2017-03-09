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
#include <cutils/ashmem.h>
#include <VtsHalHidlTargetBaseTest.h>
#include <hardware/sensors.h>       // for sensor type strings

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <sys/mman.h>
#include <unistd.h>

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_string;
using ::android::sp;
using namespace ::android::hardware::sensors::V1_0;

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
  sensors = ::testing::VtsHalHidlTargetBaseTest::getService<ISensors>();
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

class SensorsTestSharedMemory {
 public:
  static SensorsTestSharedMemory* create(SharedMemType type, size_t size);
  SharedMemInfo getSharedMemInfo() const;
  char * getBuffer() const;
  std::vector<Event> parseEvents(int64_t lastCounter = -1, size_t offset = 0) const;
  virtual ~SensorsTestSharedMemory();
 private:
  SensorsTestSharedMemory(SharedMemType type, size_t size);

  SharedMemType mType;
  native_handle_t* mNativeHandle;
  size_t mSize;
  char* mBuffer;

  DISALLOW_COPY_AND_ASSIGN(SensorsTestSharedMemory);
};

SharedMemInfo SensorsTestSharedMemory::getSharedMemInfo() const {
  SharedMemInfo mem = {
    .type = mType,
    .format = SharedMemFormat::SENSORS_EVENT,
    .size = static_cast<uint32_t>(mSize),
    .memoryHandle = mNativeHandle
  };
  return mem;
}

char * SensorsTestSharedMemory::getBuffer() const {
  return mBuffer;
}

std::vector<Event> SensorsTestSharedMemory::parseEvents(int64_t lastCounter, size_t offset) const {

  constexpr size_t kEventSize = static_cast<size_t>(SensorsEventFormatOffset::TOTAL_LENGTH);
  constexpr size_t kOffsetSize = static_cast<size_t>(SensorsEventFormatOffset::SIZE_FIELD);
  constexpr size_t kOffsetToken = static_cast<size_t>(SensorsEventFormatOffset::REPORT_TOKEN);
  constexpr size_t kOffsetType = static_cast<size_t>(SensorsEventFormatOffset::SENSOR_TYPE);
  constexpr size_t kOffsetAtomicCounter =
      static_cast<size_t>(SensorsEventFormatOffset::ATOMIC_COUNTER);
  constexpr size_t kOffsetTimestamp = static_cast<size_t>(SensorsEventFormatOffset::TIMESTAMP);
  constexpr size_t kOffsetData = static_cast<size_t>(SensorsEventFormatOffset::DATA);

  std::vector<Event> events;
  std::vector<float> data(16);

  while (offset + kEventSize <= mSize) {
    int64_t atomicCounter = *reinterpret_cast<uint32_t *>(mBuffer + offset + kOffsetAtomicCounter);
    if (atomicCounter <= lastCounter) {
      break;
    }

    int32_t size = *reinterpret_cast<int32_t *>(mBuffer + offset + kOffsetSize);
    if (size != kEventSize) {
      // unknown error, events parsed may be wrong, remove all
      events.clear();
      break;
    }

    int32_t token = *reinterpret_cast<int32_t *>(mBuffer + offset + kOffsetToken);
    int32_t type = *reinterpret_cast<int32_t *>(mBuffer + offset + kOffsetType);
    int64_t timestamp = *reinterpret_cast<int64_t *>(mBuffer + offset + kOffsetTimestamp);

    ALOGV("offset = %zu, cnt %" PRId64 ", token %" PRId32 ", type %" PRId32 ", timestamp %" PRId64,
        offset, atomicCounter, token, type, timestamp);

    Event event = {
      .timestamp = timestamp,
      .sensorHandle = token,
      .sensorType = static_cast<SensorType>(type),
    };
    event.u.data = android::hardware::hidl_array<float, 16>
        (reinterpret_cast<float*>(mBuffer + offset + kOffsetData));

    events.push_back(event);

    lastCounter = atomicCounter;
    offset += kEventSize;
  }

  return events;
}

SensorsTestSharedMemory::SensorsTestSharedMemory(SharedMemType type, size_t size)
    : mType(type), mSize(0), mBuffer(nullptr) {
  native_handle_t *handle = nullptr;
  char *buffer = nullptr;
  switch(type) {
    case SharedMemType::ASHMEM: {
      int fd;
      handle = ::native_handle_create(1 /*nFds*/, 0/*nInts*/);
      if (handle != nullptr) {
        handle->data[0] = fd = ::ashmem_create_region("SensorsTestSharedMemory", size);
        if (handle->data[0] > 0) {
          // memory is pinned by default
          buffer = static_cast<char *>
              (::mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
          if (buffer != reinterpret_cast<char*>(MAP_FAILED)) {
            break;
          }
          ::native_handle_close(handle);
        }
        ::native_handle_delete(handle);
        handle = nullptr;
      }
      break;
    }
    default:
      break;
  }

  if (buffer != nullptr) {
    mNativeHandle = handle;
    mSize = size;
    mBuffer = buffer;
  }
}

SensorsTestSharedMemory::~SensorsTestSharedMemory() {
  switch(mType) {
    case SharedMemType::ASHMEM: {
      if (mSize != 0) {
        ::munmap(mBuffer, mSize);
        mBuffer = nullptr;

        ::native_handle_close(mNativeHandle);
        ::native_handle_delete(mNativeHandle);

        mNativeHandle = nullptr;
        mSize = 0;
      }
      break;
    }
    default: {
      if (mNativeHandle != nullptr || mSize != 0 || mBuffer != nullptr) {
        ALOGE("SensorsTestSharedMemory %p not properly destructed: "
            "type %d, native handle %p, size %zu, buffer %p",
            this, static_cast<int>(mType), mNativeHandle, mSize, mBuffer);
      }
      break;
    }
  }
}

SensorsTestSharedMemory* SensorsTestSharedMemory::create(SharedMemType type, size_t size) {
  constexpr size_t kMaxSize = 128*1024*1024; // sensor test should not need more than 128M
  if (size == 0 || size >= kMaxSize) {
    return nullptr;
  }

  auto m = new SensorsTestSharedMemory(type, size);
  if (m->mSize != size || m->mBuffer == nullptr) {
    delete m;
    m = nullptr;
  }
  return m;
}

// The main test class for SENSORS HIDL HAL.
class SensorsHidlTest : public ::testing::VtsHalHidlTargetBaseTest {
 public:
  virtual void SetUp() override {
  }

  virtual void TearDown() override {
    // stop all sensors
    for (auto s : mSensorHandles) {
      S()->activate(s, false);
    }
    mSensorHandles.clear();

    // stop all direct report and channels
    for (auto c : mDirectChannelHandles) {
      // disable all reports
      S()->configDirectReport(-1, c, RateLevel::STOP, [] (auto, auto){});
      S()->unregisterDirectChannel(c);
    }
    mDirectChannelHandles.clear();
  }

 protected:
  SensorInfo defaultSensorByType(SensorType type);
  std::vector<Event> collectEvents(useconds_t timeLimitUs, size_t nEventLimit,
        bool clearBeforeStart = true, bool changeCollection = true);

  // implementation wrapper
  Return<void> getSensorsList(ISensors::getSensorsList_cb _hidl_cb) {
    return S()->getSensorsList(_hidl_cb);
  }

  Return<Result> activate(
          int32_t sensorHandle, bool enabled);

  Return<Result> batch(
          int32_t sensorHandle,
          int64_t samplingPeriodNs,
          int64_t maxReportLatencyNs) {
    return S()->batch(sensorHandle, samplingPeriodNs, maxReportLatencyNs);
  }

  Return<Result> flush(int32_t sensorHandle) {
    return S()->flush(sensorHandle);
  }

  Return<Result> injectSensorData(const Event& event) {
    return S()->injectSensorData(event);
  }

  Return<void> registerDirectChannel(
          const SharedMemInfo& mem, ISensors::registerDirectChannel_cb _hidl_cb);

  Return<Result> unregisterDirectChannel(int32_t channelHandle) {
    return S()->unregisterDirectChannel(channelHandle);
  }

  Return<void> configDirectReport(
          int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
          ISensors::configDirectReport_cb _hidl_cb) {
    return S()->configDirectReport(sensorHandle, channelHandle, rate, _hidl_cb);
  }

  inline sp<ISensors>& S() {
    return SensorsHidlEnvironment::Instance()->sensors;
  }

  inline static SensorFlagBits extractReportMode(uint64_t flag) {
    return (SensorFlagBits) (flag
        & ((uint64_t) SensorFlagBits::CONTINUOUS_MODE
          | (uint64_t) SensorFlagBits::ON_CHANGE_MODE
          | (uint64_t) SensorFlagBits::ONE_SHOT_MODE
          | (uint64_t) SensorFlagBits::SPECIAL_REPORTING_MODE));
  }

  inline static bool isMetaSensorType(SensorType type) {
    return (type == SensorType::META_DATA
            || type == SensorType::DYNAMIC_SENSOR_META
            || type == SensorType::ADDITIONAL_INFO);
  }

  inline static bool isValidType(SensorType type) {
    return (int32_t) type > 0;
  }

  static bool typeMatchStringType(SensorType type, const hidl_string& stringType);
  static bool typeMatchReportMode(SensorType type, SensorFlagBits reportMode);
  static bool delayMatchReportMode(int32_t minDelay, int32_t maxDelay, SensorFlagBits reportMode);
  static SensorFlagBits expectedReportModeForType(SensorType type);

  // all sensors and direct channnels used
  std::unordered_set<int32_t> mSensorHandles;
  std::unordered_set<int32_t> mDirectChannelHandles;
};


Return<Result> SensorsHidlTest::activate(int32_t sensorHandle, bool enabled) {
  // If activating a sensor, add the handle in a set so that when test fails it can be turned off.
  // The handle is not removed when it is deactivating on purpose so that it is not necessary to
  // check the return value of deactivation. Deactivating a sensor more than once does not have
  // negative effect.
  if (enabled) {
    mSensorHandles.insert(sensorHandle);
  }
  return S()->activate(sensorHandle, enabled);
}

Return<void> SensorsHidlTest::registerDirectChannel(
    const SharedMemInfo& mem, ISensors::registerDirectChannel_cb cb) {
  // If registeration of a channel succeeds, add the handle of channel to a set so that it can be
  // unregistered when test fails. Unregister a channel does not remove the handle on purpose.
  // Unregistering a channel more than once should not have negative effect.
  S()->registerDirectChannel(mem,
      [&] (auto result, auto channelHandle) {
        if (result == Result::OK) {
          mDirectChannelHandles.insert(channelHandle);
        }
        cb(result, channelHandle);
      });
  return Void();
}

std::vector<Event> SensorsHidlTest::collectEvents(useconds_t timeLimitUs, size_t nEventLimit,
      bool clearBeforeStart, bool changeCollection) {
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

bool SensorsHidlTest::typeMatchStringType(SensorType type, const hidl_string& stringType) {

  if (type >= SensorType::DEVICE_PRIVATE_BASE) {
    return true;
  }

  bool res = true;
  switch (type) {
#define CHECK_TYPE_STRING_FOR_SENSOR_TYPE(type) \
    case SensorType::type: res = stringType == SENSOR_STRING_TYPE_ ## type;\
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
  if (type >= SensorType::DEVICE_PRIVATE_BASE) {
    return true;
  }

  SensorFlagBits expected = expectedReportModeForType(type);

  return expected == (SensorFlagBits)-1 || expected == reportMode;
}

bool SensorsHidlTest::delayMatchReportMode(
    int32_t minDelay, int32_t maxDelay, SensorFlagBits reportMode) {
  bool res = true;
  switch(reportMode) {
    case SensorFlagBits::CONTINUOUS_MODE:
      res = (minDelay > 0) && (maxDelay >= 0);
      break;
    case SensorFlagBits::ON_CHANGE_MODE:
      //TODO: current implementation does not satisfy minDelay == 0 on Proximity
      res = (minDelay >= 0) && (maxDelay >= 0);
      //res = (minDelay == 0) && (maxDelay >= 0);
      break;
    case SensorFlagBits::ONE_SHOT_MODE:
      res = (minDelay == -1) && (maxDelay == 0);
      break;
    case SensorFlagBits::SPECIAL_REPORTING_MODE:
      res = (minDelay == 0) && (maxDelay == 0);
      break;
    default:
      res = false;
  }

  return res;
}

SensorFlagBits SensorsHidlTest::expectedReportModeForType(SensorType type) {
  switch (type) {
    case SensorType::ACCELEROMETER:
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
    case SensorType::MOTION_DETECT:
    case SensorType::STEP_COUNTER:
      return SensorFlagBits::ON_CHANGE_MODE;

    case SensorType::SIGNIFICANT_MOTION:
    case SensorType::WAKE_GESTURE:
    case SensorType::GLANCE_GESTURE:
    case SensorType::PICK_UP_GESTURE:
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
              s.type == SensorType::ADDITIONAL_INFO
              || s.type == SensorType::META_DATA);

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
  constexpr SensorType type = SensorType::ACCELEROMETER;

  SensorInfo sensor = defaultSensorByType(type);

  if (!isValidType(sensor.type)) {
    // no default sensor of this type
    return;
  }

  int32_t handle = sensor.sensorHandle;

  ASSERT_EQ(batch(handle, samplingPeriodInNs, batchingPeriodInNs), Result::OK);
  ASSERT_EQ(activate(handle, 1), Result::OK);
  events = collectEvents(minTimeUs, minNEvent, true /*clearBeforeStart*/);
  ASSERT_EQ(activate(handle, 0), Result::OK);

  ALOGI("Collected %zu samples", events.size());

  ASSERT_GT(events.size(), 0u);

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
  constexpr SensorType type = SensorType::GYROSCOPE;

  SensorInfo sensor = defaultSensorByType(type);

  if (!isValidType(sensor.type)) {
    // no default sensor of this type
    return;
  }

  int32_t handle = sensor.sensorHandle;

  ASSERT_EQ(batch(handle, samplingPeriodInNs, batchingPeriodInNs), Result::OK);
  ASSERT_EQ(activate(handle, 1), Result::OK);
  events = collectEvents(minTimeUs, minNEvent, true /*clearBeforeStart*/);
  ASSERT_EQ(activate(handle, 0), Result::OK);

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
  constexpr size_t minNEvent = 50;
  constexpr SensorType type = SensorType::ACCELEROMETER;

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

  ASSERT_EQ(batch(handle, minSamplingPeriodInNs, batchingPeriodInNs), Result::OK);
  ASSERT_EQ(activate(handle, 1), Result::OK);

  usleep(500000); // sleep 0.5 sec to wait for change rate to happen
  events1 = collectEvents(sensor.minDelay * minNEvent, minNEvent, true /*clearBeforeStart*/);

  ASSERT_EQ(batch(handle, maxSamplingPeriodInNs, batchingPeriodInNs), Result::OK);

  usleep(500000); // sleep 0.5 sec to wait for change rate to happen
  events2 = collectEvents(sensor.maxDelay * minNEvent, minNEvent, true /*clearBeforeStart*/);

  ASSERT_EQ(activate(handle, 0), Result::OK);

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
  ASSERT_GT(nEvent, 2u);
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
  ASSERT_GT(nEvent, 2u);
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
  constexpr SensorType type = SensorType::ACCELEROMETER;
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

  ASSERT_EQ(batch(handle, minSamplingPeriodInNs, INT64_MAX), Result::OK);
  ASSERT_EQ(activate(handle, 1), Result::OK);

  usleep(500000); // sleep 0.5 sec to wait for initialization
  ASSERT_EQ(flush(handle), Result::OK);

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

  ASSERT_EQ(flush(handle), Result::OK);

  events = collectEvents(allowedBatchDeliverTimeNs / 1000,
        minFifoCount, true /*clearBeforeStart*/, false /*change collection*/);

  SensorsHidlEnvironment::Instance()->setCollection(false);
  ASSERT_EQ(activate(handle, 0), Result::OK);

  size_t nEvent = 0;
  for (auto & e : events) {
    if (e.sensorType == type && e.sensorHandle == handle) {
      ++ nEvent;
    }
  }

  // at least reach 90% of advertised capacity
  ASSERT_GT(nEvent, (size_t)(batchingPeriodInNs / minSamplingPeriodInNs * 9 / 10));
}

// Test sensor event direct report with ashmem for gyro sensor
TEST_F(SensorsHidlTest, GyroscopeAshmemDirectReport) {

  constexpr SensorType type = SensorType::GYROSCOPE;
  constexpr size_t kEventSize = 104;
  constexpr size_t kNEvent = 500;
  constexpr size_t kMemSize = kEventSize * kNEvent;

  SensorInfo sensor = defaultSensorByType(type);

  if (!(sensor.flags | SensorFlagBits::MASK_DIRECT_REPORT)
      || !(sensor.flags | SensorFlagBits::DIRECT_CHANNEL_ASHMEM)) {
    // does not declare support
    return;
  }

  std::unique_ptr<SensorsTestSharedMemory>
      mem(SensorsTestSharedMemory::create(SharedMemType::ASHMEM, kMemSize));
  ASSERT_NE(mem, nullptr);

  char* buffer = mem->getBuffer();
  // fill memory with data
  for (size_t i = 0; i < kMemSize; ++i) {
    buffer[i] = '\xcc';
  }

  int32_t channelHandle;
  registerDirectChannel(mem->getSharedMemInfo(),
      [&channelHandle] (auto result, auto channelHandle_) {
          ASSERT_EQ(result, Result::OK);
          channelHandle = channelHandle_;
      });

  // check memory is zeroed
  for (size_t i = 0; i < kMemSize; ++i) {
    ASSERT_EQ(buffer[i], '\0');
  }

  int32_t eventToken;
  configDirectReport(sensor.sensorHandle, channelHandle, RateLevel::NORMAL,
      [&eventToken] (auto result, auto token) {
          ASSERT_EQ(result, Result::OK);
          eventToken = token;
      });

  usleep(1500000); // sleep 1 sec for data, plus 0.5 sec for initialization
  auto events = mem->parseEvents();

  // allowed to be 55% of nominal freq (50Hz)
  ASSERT_GT(events.size(), 50u / 2u);
  ASSERT_LT(events.size(), static_cast<size_t>(110*1.5));

  int64_t lastTimestamp = 0;
  for (auto &e : events) {
    ASSERT_EQ(e.sensorType, type);
    ASSERT_EQ(e.sensorHandle, eventToken);
    ASSERT_GT(e.timestamp, lastTimestamp);

    Vec3 gyro = e.u.vec3;
    double gyroNorm = std::sqrt(gyro.x * gyro.x + gyro.y * gyro.y + gyro.z * gyro.z);
    // assert not drifting
    ASSERT_TRUE(gyroNorm < 0.1);  // < ~5 degree/sa

    lastTimestamp = e.timestamp;
  }

  // stop sensor and unregister channel
  configDirectReport(sensor.sensorHandle, channelHandle, RateLevel::STOP,
        [&eventToken] (auto result, auto) {
            ASSERT_EQ(result, Result::OK);
        });
  ASSERT_EQ(unregisterDirectChannel(channelHandle), Result::OK);
}

int main(int argc, char **argv) {
  ::testing::AddGlobalTestEnvironment(SensorsHidlEnvironment::Instance());
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
// vim: set ts=2 sw=2
