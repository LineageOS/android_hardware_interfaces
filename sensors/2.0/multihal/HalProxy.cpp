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

#include "HalProxy.h"

#include <android/hardware/sensors/2.0/types.h>

#include <dlfcn.h>

#include <fstream>
#include <functional>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

typedef ISensorsSubHal*(SensorsHalGetSubHalFunc)(uint32_t*);

// TODO: Use this wake lock name as the prefix to all sensors HAL wake locks acquired.
// constexpr const char* kWakeLockName = "SensorsHAL_WAKEUP";

// TODO: Use the following class as a starting point for implementing the full HalProxyCallback
// along with being inspiration for how to implement the ScopedWakelock class.
/**
 * Callback class used to provide the HalProxy with the index of which subHal is invoking
 */
class SensorsCallbackProxy : public ISensorsCallback {
  public:
    SensorsCallbackProxy(wp<HalProxy>& halProxy, int32_t subHalIndex)
        : mHalProxy(halProxy), mSubHalIndex(subHalIndex) {}

    Return<void> onDynamicSensorsConnected(
            const hidl_vec<SensorInfo>& dynamicSensorsAdded) override {
        sp<HalProxy> halProxy(mHalProxy.promote());
        if (halProxy != nullptr) {
            return halProxy->onDynamicSensorsConnected(dynamicSensorsAdded, mSubHalIndex);
        }
        return Return<void>();
    }

    Return<void> onDynamicSensorsDisconnected(
            const hidl_vec<int32_t>& dynamicSensorHandlesRemoved) override {
        sp<HalProxy> halProxy(mHalProxy.promote());
        if (halProxy != nullptr) {
            return halProxy->onDynamicSensorsDisconnected(dynamicSensorHandlesRemoved,
                                                          mSubHalIndex);
        }
        return Return<void>();
    }

  private:
    wp<HalProxy>& mHalProxy;
    int32_t mSubHalIndex;
};

HalProxy::HalProxy() {
    const char* kMultiHalConfigFile = "/vendor/etc/sensors/hals.conf";
    initializeSubHalListFromConfigFile(kMultiHalConfigFile);
    initializeSensorList();
}

HalProxy::HalProxy(std::vector<ISensorsSubHal*>& subHalList) : mSubHalList(subHalList) {
    initializeSensorList();
}

HalProxy::~HalProxy() {
    // TODO: Join any running threads and clean up FMQs and any other allocated
    // state.
}

Return<void> HalProxy::getSensorsList(getSensorsList_cb _hidl_cb) {
    _hidl_cb(mSensorList);
    return Void();
}

Return<Result> HalProxy::setOperationMode(OperationMode mode) {
    Result result = Result::OK;
    size_t subHalIndex;
    for (subHalIndex = 0; subHalIndex < mSubHalList.size(); subHalIndex++) {
        ISensorsSubHal* subHal = mSubHalList[subHalIndex];
        result = subHal->setOperationMode(mode);
        if (result != Result::OK) {
            ALOGE("setOperationMode failed for SubHal: %s", subHal->getName().c_str());
            break;
        }
    }
    if (result != Result::OK) {
        // Reset the subhal operation modes that have been flipped
        for (size_t i = 0; i < subHalIndex; i++) {
            ISensorsSubHal* subHal = mSubHalList[i];
            subHal->setOperationMode(mCurrentOperationMode);
        }
    } else {
        mCurrentOperationMode = mode;
    }
    return result;
}

Return<Result> HalProxy::activate(int32_t sensorHandle, bool enabled) {
    return getSubHalForSensorHandle(sensorHandle)
            ->activate(clearSubHalIndex(sensorHandle), enabled);
}

Return<Result> HalProxy::initialize(
        const ::android::hardware::MQDescriptorSync<Event>& eventQueueDescriptor,
        const ::android::hardware::MQDescriptorSync<uint32_t>& wakeLockDescriptor,
        const sp<ISensorsCallback>& sensorsCallback) {
    Result result = Result::OK;

    // TODO: clean up sensor requests, if not already done elsewhere through a death recipient, and
    // clean up any other resources that exist (FMQs, flags, threads, etc.)

    mDynamicSensorsCallback = sensorsCallback;

    // Create the Event FMQ from the eventQueueDescriptor. Reset the read/write positions.
    mEventQueue =
            std::make_unique<EventMessageQueue>(eventQueueDescriptor, true /* resetPointers */);

    // Create the EventFlag that is used to signal to the framework that sensor events have been
    // written to the Event FMQ
    if (EventFlag::createEventFlag(mEventQueue->getEventFlagWord(), &mEventQueueFlag) != OK) {
        result = Result::BAD_VALUE;
    }

    // Create the Wake Lock FMQ that is used by the framework to communicate whenever WAKE_UP
    // events have been successfully read and handled by the framework.
    mWakeLockQueue =
            std::make_unique<WakeLockMessageQueue>(wakeLockDescriptor, true /* resetPointers */);

    if (!mDynamicSensorsCallback || !mEventQueue || !mWakeLockQueue || mEventQueueFlag == nullptr) {
        result = Result::BAD_VALUE;
    }

    // TODO: start threads to read wake locks and process events from sub HALs.

    return result;
}

Return<Result> HalProxy::batch(int32_t sensorHandle, int64_t samplingPeriodNs,
                               int64_t maxReportLatencyNs) {
    return getSubHalForSensorHandle(sensorHandle)
            ->batch(clearSubHalIndex(sensorHandle), samplingPeriodNs, maxReportLatencyNs);
}

Return<Result> HalProxy::flush(int32_t sensorHandle) {
    return getSubHalForSensorHandle(sensorHandle)->flush(clearSubHalIndex(sensorHandle));
}

Return<Result> HalProxy::injectSensorData(const Event& /* event */) {
    // TODO: Proxy API call to appropriate sub-HAL.
    return Result::INVALID_OPERATION;
}

Return<void> HalProxy::registerDirectChannel(const SharedMemInfo& /* mem */,
                                             registerDirectChannel_cb _hidl_cb) {
    // TODO: During init, discover the first sub-HAL in the config that has sensors with direct
    // channel support, if any, and proxy the API call there.
    _hidl_cb(Result::INVALID_OPERATION, -1 /* channelHandle */);
    return Return<void>();
}

Return<Result> HalProxy::unregisterDirectChannel(int32_t /* channelHandle */) {
    // TODO: During init, discover the first sub-HAL in the config that has sensors with direct
    // channel support, if any, and proxy the API call there.
    return Result::INVALID_OPERATION;
}

Return<void> HalProxy::configDirectReport(int32_t /* sensorHandle */, int32_t /* channelHandle */,
                                          RateLevel /* rate */, configDirectReport_cb _hidl_cb) {
    // TODO: During init, discover the first sub-HAL in the config that has sensors with direct
    // channel support, if any, and proxy the API call there.
    _hidl_cb(Result::INVALID_OPERATION, 0 /* reportToken */);
    return Return<void>();
}

Return<void> HalProxy::debug(const hidl_handle& /* fd */, const hidl_vec<hidl_string>& /* args */) {
    // TODO: output debug information
    return Return<void>();
}

Return<void> HalProxy::onDynamicSensorsConnected(
        const hidl_vec<SensorInfo>& /* dynamicSensorsAdded */, int32_t /* subHalIndex */) {
    // TODO: Map the SensorInfo to the global list and then invoke the framework's callback.
    return Return<void>();
}

Return<void> HalProxy::onDynamicSensorsDisconnected(
        const hidl_vec<int32_t>& /* dynamicSensorHandlesRemoved */, int32_t /* subHalIndex */) {
    // TODO: Unmap the SensorInfo from the global list and then invoke the framework's callback.
    return Return<void>();
}

void HalProxy::initializeSubHalListFromConfigFile(const char* configFileName) {
    std::ifstream subHalConfigStream(configFileName);
    if (!subHalConfigStream) {
        ALOGE("Failed to load subHal config file: %s", configFileName);
    } else {
        std::string subHalLibraryFile;
        while (subHalConfigStream >> subHalLibraryFile) {
            void* handle = dlopen(subHalLibraryFile.c_str(), RTLD_NOW);
            if (handle == nullptr) {
                ALOGE("dlopen failed for library: %s", subHalLibraryFile.c_str());
            } else {
                SensorsHalGetSubHalFunc* sensorsHalGetSubHalPtr =
                        (SensorsHalGetSubHalFunc*)dlsym(handle, "sensorsHalGetSubHal");
                if (sensorsHalGetSubHalPtr == nullptr) {
                    ALOGE("Failed to locate sensorsHalGetSubHal function for library: %s",
                          subHalLibraryFile.c_str());
                } else {
                    std::function<SensorsHalGetSubHalFunc> sensorsHalGetSubHal =
                            *sensorsHalGetSubHalPtr;
                    uint32_t version;
                    ISensorsSubHal* subHal = sensorsHalGetSubHal(&version);
                    if (version != SUB_HAL_2_0_VERSION) {
                        ALOGE("SubHal version was not 2.0 for library: %s",
                              subHalLibraryFile.c_str());
                    } else {
                        ALOGV("Loaded SubHal from library: %s", subHalLibraryFile.c_str());
                        mSubHalList.push_back(subHal);
                    }
                }
            }
        }
    }
}

void HalProxy::initializeSensorList() {
    for (size_t subHalIndex = 0; subHalIndex < mSubHalList.size(); subHalIndex++) {
        ISensorsSubHal* subHal = mSubHalList[subHalIndex];
        auto result = subHal->getSensorsList([&](const auto& list) {
            for (SensorInfo sensor : list) {
                if ((sensor.sensorHandle & kSensorHandleSubHalIndexMask) != 0) {
                    ALOGE("SubHal sensorHandle's first byte was not 0");
                } else {
                    ALOGV("Loaded sensor: %s", sensor.name.c_str());
                    sensor.sensorHandle |= (subHalIndex << 24);
                    setDirectChannelFlags(&sensor, subHal);
                    mSensorList.push_back(sensor);
                }
            }
        });
        if (!result.isOk()) {
            ALOGE("getSensorsList call failed for SubHal: %s", subHal->getName().c_str());
        }
    }
}

void HalProxy::setDirectChannelFlags(SensorInfo* sensorInfo, ISensorsSubHal* subHal) {
    bool sensorSupportsDirectChannel =
            (sensorInfo->flags & (V1_0::SensorFlagBits::MASK_DIRECT_REPORT |
                                  V1_0::SensorFlagBits::MASK_DIRECT_CHANNEL)) != 0;
    if (mDirectChannelSubHal == nullptr && sensorSupportsDirectChannel) {
        mDirectChannelSubHal = subHal;
    } else if (mDirectChannelSubHal != nullptr && subHal != mDirectChannelSubHal) {
        // disable direct channel capability for sensors in subHals that are not
        // the only one we will enable
        sensorInfo->flags &= ~(V1_0::SensorFlagBits::MASK_DIRECT_REPORT |
                               V1_0::SensorFlagBits::MASK_DIRECT_CHANNEL);
    }
}

ISensorsSubHal* HalProxy::getSubHalForSensorHandle(uint32_t sensorHandle) {
    return mSubHalList[static_cast<size_t>(sensorHandle >> 24)];
}

uint32_t HalProxy::clearSubHalIndex(uint32_t sensorHandle) {
    return sensorHandle & (~kSensorHandleSubHalIndexMask);
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
