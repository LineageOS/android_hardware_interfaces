/*
 * Copyright (C) 2019 The Android Open Source Project
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

#pragma once

#include "ScopedWakelock.h"
#include "SubHal.h"

#include <android/hardware/sensors/2.0/ISensors.h>
#include <android/hardware/sensors/2.0/types.h>
#include <fmq/MessageQueue.h>
#include <hardware_legacy/power.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::EventFlag;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::MessageQueue;
using ::android::hardware::MQDescriptor;
using ::android::hardware::Return;
using ::android::hardware::Void;

class HalProxy : public ISensors, public IScopedWakelockRefCounter {
  public:
    using Event = ::android::hardware::sensors::V1_0::Event;
    using OperationMode = ::android::hardware::sensors::V1_0::OperationMode;
    using RateLevel = ::android::hardware::sensors::V1_0::RateLevel;
    using Result = ::android::hardware::sensors::V1_0::Result;
    using SensorInfo = ::android::hardware::sensors::V1_0::SensorInfo;
    using SharedMemInfo = ::android::hardware::sensors::V1_0::SharedMemInfo;
    using ISensorsSubHal = ::android::hardware::sensors::V2_0::implementation::ISensorsSubHal;

    explicit HalProxy();
    // Test only constructor.
    explicit HalProxy(std::vector<ISensorsSubHal*>& subHalList);
    ~HalProxy();

    // Methods from ::android::hardware::sensors::V2_0::ISensors follow.
    Return<void> getSensorsList(getSensorsList_cb _hidl_cb) override;

    Return<Result> setOperationMode(OperationMode mode) override;

    Return<Result> activate(int32_t sensorHandle, bool enabled) override;

    Return<Result> initialize(
            const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
            const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
            const sp<ISensorsCallback>& sensorsCallback) override;

    Return<Result> batch(int32_t sensorHandle, int64_t samplingPeriodNs,
                         int64_t maxReportLatencyNs) override;

    Return<Result> flush(int32_t sensorHandle) override;

    Return<Result> injectSensorData(const Event& event) override;

    Return<void> registerDirectChannel(const SharedMemInfo& mem,
                                       registerDirectChannel_cb _hidl_cb) override;

    Return<Result> unregisterDirectChannel(int32_t channelHandle) override;

    Return<void> configDirectReport(int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
                                    configDirectReport_cb _hidl_cb) override;

    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>& args) override;

    // Below methods from ::android::hardware::sensors::V2_0::ISensorsCallback with a minor change
    // to pass in the sub-HAL index. While the above methods are invoked from the sensors framework
    // via the binder, these methods are invoked from a callback provided to sub-HALs inside the
    // same process as the HalProxy, but potentially running on different threads.
    Return<void> onDynamicSensorsConnected(const hidl_vec<SensorInfo>& dynamicSensorsAdded,
                                           int32_t subHalIndex);

    Return<void> onDynamicSensorsDisconnected(const hidl_vec<int32_t>& dynamicSensorHandlesRemoved,
                                              int32_t subHalIndex);

    // Below methods are for HalProxyCallback

    /**
     * Post events to the event message queue if there is room to write them. Otherwise post the
     * remaining events to a background thread for a blocking write with a kPendingWriteTimeoutNs
     * timeout.
     *
     * @param events The list of events to post to the message queue.
     * @param numWakeupEvents The number of wakeup events in events.
     * @param wakelock The wakelock associated with this post of events.
     */
    void postEventsToMessageQueue(const std::vector<Event>& events, size_t numWakeupEvents,
                                  ScopedWakelock wakelock);

    /**
     * Get the sensor info associated with that sensorHandle.
     *
     * @param sensorHandle The sensor handle.
     *
     * @return The sensor info object in the mapping.
     */
    const SensorInfo& getSensorInfo(int32_t sensorHandle) { return mSensors[sensorHandle]; }

    bool areThreadsRunning() { return mThreadsRun.load(); }

    // Below methods are from IScopedWakelockRefCounter interface
    bool incrementRefCountAndMaybeAcquireWakelock(size_t delta,
                                                  int64_t* timeoutStart = nullptr) override;

    void decrementRefCountAndMaybeReleaseWakelock(size_t delta, int64_t timeoutStart = -1) override;

  private:
    using EventMessageQueue = MessageQueue<Event, kSynchronizedReadWrite>;
    using WakeLockMessageQueue = MessageQueue<uint32_t, kSynchronizedReadWrite>;

    /**
     * The Event FMQ where sensor events are written
     */
    std::unique_ptr<EventMessageQueue> mEventQueue;

    /**
     * The Wake Lock FMQ that is read to determine when the framework has handled WAKE_UP events
     */
    std::unique_ptr<WakeLockMessageQueue> mWakeLockQueue;

    /**
     * Event Flag to signal to the framework when sensor events are available to be read and to
     * interrupt event queue blocking write.
     */
    EventFlag* mEventQueueFlag = nullptr;

    //! Event Flag to signal internally that the wakelock queue should stop its blocking read.
    EventFlag* mWakelockQueueFlag = nullptr;

    /**
     * Callback to the sensors framework to inform it that new sensors have been added or removed.
     */
    sp<ISensorsCallback> mDynamicSensorsCallback;

    /**
     * SubHal object pointers that have been saved from vendor dynamic libraries.
     */
    std::vector<ISensorsSubHal*> mSubHalList;

    //! The list of subhal callbacks for each subhal where the indices correlate with mSubHalList
    std::vector<const sp<IHalProxyCallback>> mSubHalCallbacks;

    /**
     * Map of sensor handles to SensorInfo objects that contains the sensor info from subhals as
     * well as the modified sensor handle for the framework.
     *
     * The subhal index is encoded in the first byte of the sensor handle and the remaining
     * bytes are generated by the subhal to identify the sensor.
     */
    std::map<int32_t, SensorInfo> mSensors;

    //! Map of the dynamic sensors that have been added to halproxy.
    std::map<int32_t, SensorInfo> mDynamicSensors;

    //! The current operation mode for all subhals.
    OperationMode mCurrentOperationMode = OperationMode::NORMAL;

    //! The single subHal that supports directChannel reporting.
    ISensorsSubHal* mDirectChannelSubHal = nullptr;

    //! The timeout for each pending write on background thread for events.
    static const int64_t kPendingWriteTimeoutNs = 5 * INT64_C(1000000000) /* 5 seconds */;

    //! The bit mask used to get the subhal index from a sensor handle.
    static constexpr int32_t kSensorHandleSubHalIndexMask = 0xFF000000;

    /**
     * A FIFO queue of pairs of vector of events and the number of wakeup events in that vector
     * which are waiting to be written to the events fmq in the background thread.
     */
    std::queue<std::pair<std::vector<Event>, size_t>> mPendingWriteEventsQueue;

    //! The most events observed on the pending write events queue for debug purposes.
    size_t mMostEventsObservedPendingWriteEventsQueue = 0;

    //! The max number of events allowed in the pending write events queue
    static constexpr size_t kMaxSizePendingWriteEventsQueue = 100000;

    //! The number of events in the pending write events queue
    size_t mSizePendingWriteEventsQueue = 0;

    //! The mutex protecting writing to the fmq and the pending events queue
    std::mutex mEventQueueWriteMutex;

    //! The condition variable waiting on pending write events to stack up
    std::condition_variable mEventQueueWriteCV;

    //! The thread object ptr that handles pending writes
    std::thread mPendingWritesThread;

    //! The thread object that handles wakelocks
    std::thread mWakelockThread;

    //! The bool indicating whether to end the threads started in initialize
    std::atomic_bool mThreadsRun = true;

    //! The mutex protecting access to the dynamic sensors added and removed methods.
    std::mutex mDynamicSensorsMutex;

    // WakelockRefCount membar vars below

    //! The mutex protecting the wakelock refcount and subsequent wakelock releases and
    //! acquisitions
    std::recursive_mutex mWakelockMutex;

    std::condition_variable_any mWakelockCV;

    //! The refcount of how many ScopedWakelocks and pending wakeup events are active
    size_t mWakelockRefCount = 0;

    int64_t mWakelockTimeoutStartTime = getTimeNow();

    int64_t mWakelockTimeoutResetTime = getTimeNow();

    const char* kWakelockName = "SensorsHAL_WAKEUP";

    /**
     * Initialize the list of SubHal objects in mSubHalList by reading from dynamic libraries
     * listed in a config file.
     */
    void initializeSubHalListFromConfigFile(const char* configFileName);

    /**
     * Initialize the HalProxyCallback vector using the list of subhals.
     */
    void initializeSubHalCallbacks();

    /**
     * Initialize the list of SensorInfo objects in mSensorList by getting sensors from each
     * subhal.
     */
    void initializeSensorList();

    /**
     * Calls the helper methods that all ctors use.
     */
    void init();

    /**
     * Stops all threads by setting the threads running flag to false and joining to them.
     */
    void stopThreads();

    /**
     * Disable all the sensors observed by the HalProxy.
     */
    void disableAllSensors();

    /**
     * Starts the thread that handles pending writes to event fmq.
     *
     * @param halProxy The HalProxy object pointer.
     */
    static void startPendingWritesThread(HalProxy* halProxy);

    //! Handles the pending writes on events to eventqueue.
    void handlePendingWrites();

    /**
     * Starts the thread that handles decrementing the ref count on wakeup events processed by the
     * framework and timing out wakelocks.
     *
     * @param halProxy The HalProxy object pointer.
     */
    static void startWakelockThread(HalProxy* halProxy);

    //! Handles the wakelocks.
    void handleWakelocks();

    /**
     * @param timeLeft The variable that should be set to the timeleft before timeout will occur or
     * unmodified if timeout occurred.
     *
     * @return true if the shared wakelock has been held passed the timeout and should be released
     */
    bool sharedWakelockDidTimeout(int64_t* timeLeft);

    /**
     * Reset all the member variables associated with the wakelock ref count and maybe release
     * the shared wakelock.
     */
    void resetSharedWakelock();

    /**
     * Clear direct channel flags if the HalProxy has already chosen a subhal as its direct channel
     * subhal. Set the directChannelSubHal pointer to the subHal passed in if this is the first
     * direct channel enabled sensor seen.
     *
     * @param sensorInfo The SensorInfo object that may be altered to have direct channel support
     *    disabled.
     * @param subHal The subhal pointer that the current sensorInfo object came from.
     */
    void setDirectChannelFlags(SensorInfo* sensorInfo, ISensorsSubHal* subHal);

    /*
     * Get the subhal pointer which can be found by indexing into the mSubHalList vector
     * using the index from the first byte of sensorHandle.
     *
     * @param sensorHandle The handle used to identify a sensor in one of the subhals.
     */
    ISensorsSubHal* getSubHalForSensorHandle(int32_t sensorHandle);

    /**
     * Checks that sensorHandle's subhal index byte is within bounds of mSubHalList.
     *
     * @param sensorHandle The sensor handle to check.
     *
     * @return true if sensorHandles's subhal index byte is valid.
     */
    bool isSubHalIndexValid(int32_t sensorHandle);

    /**
     * Count the number of wakeup events in the first n events of the vector.
     *
     * @param events The vector of Event objects.
     * @param n The end index not inclusive of events to consider.
     *
     * @return The number of wakeup events of the considered events.
     */
    size_t countNumWakeupEvents(const std::vector<Event>& events, size_t n);

    /*
     * Clear out the subhal index bytes from a sensorHandle.
     *
     * @param sensorHandle The sensor handle to modify.
     *
     * @return The modified version of the sensor handle.
     */
    static int32_t clearSubHalIndex(int32_t sensorHandle);

    /**
     * @param sensorHandle The sensor handle to modify.
     *
     * @return true if subHalIndex byte of sensorHandle is zeroed.
     */
    static bool subHalIndexIsClear(int32_t sensorHandle);
};

/**
 * Callback class used to provide the HalProxy with the index of which subHal is invoking
 */
class HalProxyCallback : public IHalProxyCallback {
    using SensorInfo = ::android::hardware::sensors::V1_0::SensorInfo;

  public:
    HalProxyCallback(HalProxy* halProxy, int32_t subHalIndex)
        : mHalProxy(halProxy), mSubHalIndex(subHalIndex) {}

    Return<void> onDynamicSensorsConnected(
            const hidl_vec<SensorInfo>& dynamicSensorsAdded) override {
        return mHalProxy->onDynamicSensorsConnected(dynamicSensorsAdded, mSubHalIndex);
    }

    Return<void> onDynamicSensorsDisconnected(
            const hidl_vec<int32_t>& dynamicSensorHandlesRemoved) override {
        return mHalProxy->onDynamicSensorsDisconnected(dynamicSensorHandlesRemoved, mSubHalIndex);
    }

    void postEvents(const std::vector<Event>& events, ScopedWakelock wakelock);

    ScopedWakelock createScopedWakelock(bool lock);

  private:
    HalProxy* mHalProxy;
    int32_t mSubHalIndex;

    std::vector<Event> processEvents(const std::vector<Event>& events,
                                     size_t* numWakeupEvents) const;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
