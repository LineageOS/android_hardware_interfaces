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

#include "SensorsHidlEnvironmentV2_0.h"

#include <android/hardware/sensors/2.0/types.h>
#include <log/log.h>

#include <algorithm>
#include <vector>

using ::android::hardware::EventFlag;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SensorInfo;
using ::android::hardware::sensors::V2_0::EventQueueFlagBits;
using ::android::hardware::sensors::V2_0::ISensors;
using ::android::hardware::sensors::V2_0::ISensorsCallback;

template <typename EnumType>
constexpr typename std::underlying_type<EnumType>::type asBaseType(EnumType value) {
    return static_cast<typename std::underlying_type<EnumType>::type>(value);
}

constexpr size_t SensorsHidlEnvironmentV2_0::MAX_RECEIVE_BUFFER_EVENT_COUNT;

void SensorsHalDeathRecipient::serviceDied(
        uint64_t /* cookie */,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& /* service */) {
    ALOGE("Sensors HAL died (likely crashed) during test");
    FAIL() << "Sensors HAL died during test";
}

struct SensorsCallback : ISensorsCallback {
    Return<void> onDynamicSensorsConnected(const hidl_vec<SensorInfo>& /* sensorInfos */) {
        return Return<void>();
    }

    Return<void> onDynamicSensorsDisconnected(const hidl_vec<int32_t>& /* sensorHandles */) {
        return Return<void>();
    }
};

bool SensorsHidlEnvironmentV2_0::resetHal() {
    bool succeed = false;
    do {
        mSensors = ISensors::getService(
            SensorsHidlEnvironmentV2_0::Instance()->getServiceName<ISensors>());
        if (mSensors == nullptr) {
            break;
        }
        mSensors->linkToDeath(mDeathRecipient, 0 /* cookie */);

        // Initialize FMQs
        mEventQueue = std::make_unique<EventMessageQueue>(MAX_RECEIVE_BUFFER_EVENT_COUNT,
                                                          true /* configureEventFlagWord */);

        mWakeLockQueue = std::make_unique<WakeLockQueue>(MAX_RECEIVE_BUFFER_EVENT_COUNT,
                                                         true /* configureEventFlagWord */);

        if (mEventQueue == nullptr || mWakeLockQueue == nullptr) {
            break;
        }

        EventFlag::deleteEventFlag(&mEventQueueFlag);
        EventFlag::createEventFlag(mEventQueue->getEventFlagWord(), &mEventQueueFlag);
        if (mEventQueueFlag == nullptr) {
            break;
        }

        mSensors->initialize(*mEventQueue->getDesc(), *mWakeLockQueue->getDesc(),
                             new SensorsCallback());

        std::vector<SensorInfo> sensorList;
        if (!mSensors->getSensorsList([&](const hidl_vec<SensorInfo>& list) { sensorList = list; })
                 .isOk()) {
            break;
        }

        // stop each sensor individually
        bool ok = true;
        for (const auto& i : sensorList) {
            if (!mSensors->activate(i.sensorHandle, false).isOk()) {
                ok = false;
                break;
            }
        }
        if (!ok) {
            break;
        }

        // mark it done
        succeed = true;
    } while (0);

    if (!succeed) {
        mSensors = nullptr;
    }

    return succeed;
}

void SensorsHidlEnvironmentV2_0::HidlTearDown() {
    mStopThread = true;

    if (mEventQueueFlag != nullptr) {
        // Wake up the event queue so the poll thread can exit
        mEventQueueFlag->wake(asBaseType(EventQueueFlagBits::READ_AND_PROCESS));
        if (mPollThread.joinable()) {
            mPollThread.join();
        }

        EventFlag::deleteEventFlag(&mEventQueueFlag);
    }
}

void SensorsHidlEnvironmentV2_0::startPollingThread() {
    mStopThread = false;
    mPollThread = std::thread(pollingThread, this);
    mEvents.reserve(MAX_RECEIVE_BUFFER_EVENT_COUNT);
}

void SensorsHidlEnvironmentV2_0::readEvents() {
    size_t availableEvents = mEventQueue->availableToRead();

    if (availableEvents == 0) {
        uint32_t eventFlagState = 0;

        mEventQueueFlag->wait(asBaseType(EventQueueFlagBits::READ_AND_PROCESS), &eventFlagState);
        availableEvents = mEventQueue->availableToRead();
    }

    size_t eventsToRead = std::min(availableEvents, mEventBuffer.size());
    if (eventsToRead > 0) {
        if (mEventQueue->read(mEventBuffer.data(), eventsToRead)) {
            mEventQueueFlag->wake(asBaseType(EventQueueFlagBits::EVENTS_READ));
            for (size_t i = 0; i < eventsToRead; i++) {
                addEvent(mEventBuffer[i]);
            }
        }
    }
}

void SensorsHidlEnvironmentV2_0::pollingThread(SensorsHidlEnvironmentV2_0* env) {
    ALOGD("polling thread start");

    while (!env->mStopThread.load()) {
        env->readEvents();
    }

    ALOGD("polling thread end");
}
