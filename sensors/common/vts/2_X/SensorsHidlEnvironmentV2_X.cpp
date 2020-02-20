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

#include "SensorsHidlEnvironmentV2_X.h"

#include <android/hardware/sensors/2.0/types.h>
#include <android/hardware/sensors/2.1/types.h>

#include <log/log.h>

#include <algorithm>
#include <vector>

using ::android::hardware::EventFlag;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V2_0::EventQueueFlagBits;
using ::android::hardware::sensors::V2_1::SensorInfo;
#ifdef SENSORS_HAL_2_1
using ::android::hardware::sensors::V2_1::ISensors;
#else
using ::android::hardware::sensors::V2_0::ISensors;
#endif
using ::android::hardware::sensors::V2_1::ISensorsCallback;

template <typename EnumType>
constexpr typename std::underlying_type<EnumType>::type asBaseType(EnumType value) {
    return static_cast<typename std::underlying_type<EnumType>::type>(value);
}

void SensorsHalDeathRecipient::serviceDied(
        uint64_t /* cookie */,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& /* service */) {
    ALOGE("Sensors HAL died (likely crashed) during test");
    FAIL() << "Sensors HAL died during test";
}

bool SensorsHidlEnvironmentV2_X::resetHal() {
    bool succeed = false;
    do {
        mSensors = wrapISensors(ISensors::getService(mServiceName));
        if (mSensors == nullptr) {
            break;
        }
        mSensors->linkToDeath(mDeathRecipient, 0 /* cookie */);

        // Initialize FMQs
        mWakeLockQueue = std::make_unique<WakeLockQueue>(MAX_RECEIVE_BUFFER_EVENT_COUNT,
                                                         true /* configureEventFlagWord */);

        if (mWakeLockQueue == nullptr) {
            break;
        }

        EventFlag::deleteEventFlag(&mEventQueueFlag);
        EventFlag::createEventFlag(mSensors->getEventQueue()->getEventFlagWord(), &mEventQueueFlag);
        if (mEventQueueFlag == nullptr) {
            break;
        }

        mSensors->initialize(*mWakeLockQueue->getDesc(), new NoOpSensorsCallback());

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

void SensorsHidlEnvironmentV2_X::HidlTearDown() {
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

void SensorsHidlEnvironmentV2_X::startPollingThread() {
    mStopThread = false;
    mEvents.reserve(MAX_RECEIVE_BUFFER_EVENT_COUNT);
    mPollThread = std::thread(pollingThread, this);
}

void SensorsHidlEnvironmentV2_X::readEvents() {
    size_t availableEvents = mSensors->getEventQueue()->availableToRead();

    if (availableEvents == 0) {
        uint32_t eventFlagState = 0;

        mEventQueueFlag->wait(asBaseType(EventQueueFlagBits::READ_AND_PROCESS), &eventFlagState);
        availableEvents = mSensors->getEventQueue()->availableToRead();
    }

    size_t eventsToRead = std::min(availableEvents, mEventBuffer.size());
    if (eventsToRead > 0) {
        if (mSensors->getEventQueue()->read(mEventBuffer.data(), eventsToRead)) {
            mEventQueueFlag->wake(asBaseType(EventQueueFlagBits::EVENTS_READ));
            for (size_t i = 0; i < eventsToRead; i++) {
                addEvent(mEventBuffer[i]);
            }
        }
    }
}

void SensorsHidlEnvironmentV2_X::pollingThread(SensorsHidlEnvironmentV2_X* env) {
    ALOGD("polling thread start");

    while (!env->mStopThread.load()) {
        env->readEvents();
    }

    ALOGD("polling thread end");
}
