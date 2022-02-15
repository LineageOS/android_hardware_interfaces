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

#include "SensorsAidlEnvironment.h"

#include <android/binder_manager.h>
#include <log/log.h>

#include <aidl/android/hardware/sensors/BnSensorsCallback.h>

using aidl::android::hardware::sensors::BnSensorsCallback;
using aidl::android::hardware::sensors::SensorInfo;
using android::hardware::EventFlag;
using ndk::ScopedAStatus;
using ndk::SpAIBinder;

namespace {

void serviceDied(void* /* cookie */) {
    ALOGE("Sensors HAL died (likely crashed) during test");
    FAIL() << "Sensors HAL died during test";
}

class NoOpSensorsCallback : public BnSensorsCallback {
  public:
    ScopedAStatus onDynamicSensorsConnected(
            const std::vector<SensorInfo>& /* sensorInfos */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus onDynamicSensorsDisconnected(
            const std::vector<int32_t>& /* sensorHandles */) override {
        return ScopedAStatus::ok();
    }
};

}  // anonymous namespace

SensorsAidlEnvironment::SensorsAidlEnvironment(const std::string& service_name)
    : SensorsVtsEnvironmentBase(service_name),
      mCallback(ndk::SharedRefBase::make<NoOpSensorsCallback>()),
      mDeathRecipient(AIBinder_DeathRecipient_new(serviceDied)) {}

bool SensorsAidlEnvironment::resetHal() {
    bool succeed = false;
    do {
        mSensors = ISensors::fromBinder(
                SpAIBinder(AServiceManager_waitForService(mServiceName.c_str())));
        if (mSensors == nullptr) {
            break;
        }

        AIBinder_linkToDeath(mSensors->asBinder().get(), mDeathRecipient.get(), this);

        // Initialize FMQs
        mWakeLockQueue = std::make_unique<WakeLockQueue>(MAX_RECEIVE_BUFFER_EVENT_COUNT,
                                                         true /* configureEventFlagWord */);
        mEventQueue = std::make_unique<EventQueue>(MAX_RECEIVE_BUFFER_EVENT_COUNT,
                                                   true /* configureEventFlagWord */);

        if (mWakeLockQueue == nullptr || mEventQueue == nullptr) {
            break;
        }

        EventFlag::deleteEventFlag(&mEventQueueFlag);
        EventFlag::createEventFlag(mEventQueue->getEventFlagWord(), &mEventQueueFlag);
        if (mEventQueueFlag == nullptr) {
            break;
        }

        mSensors->initialize(mEventQueue->dupeDesc(), mWakeLockQueue->dupeDesc(), mCallback);

        std::vector<SensorInfo> sensorList;
        if (!mSensors->getSensorsList(&sensorList).isOk()) {
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

void SensorsAidlEnvironment::TearDown() {
    mStopThread = true;

    if (mEventQueueFlag != nullptr) {
        // Wake up the event queue so the poll thread can exit
        mEventQueueFlag->wake(ISensors::EVENT_QUEUE_FLAG_BITS_READ_AND_PROCESS);
        if (mPollThread.joinable()) {
            mPollThread.join();
        }

        EventFlag::deleteEventFlag(&mEventQueueFlag);
    }
}

void SensorsAidlEnvironment::startPollingThread() {
    mStopThread = false;
    mEvents.reserve(MAX_RECEIVE_BUFFER_EVENT_COUNT);
    mPollThread = std::thread(pollingThread, this);
}

void SensorsAidlEnvironment::readEvents() {
    size_t availableEvents = mEventQueue->availableToRead();

    if (availableEvents == 0) {
        uint32_t eventFlagState = 0;

        mEventQueueFlag->wait(ISensors::EVENT_QUEUE_FLAG_BITS_READ_AND_PROCESS, &eventFlagState);
        availableEvents = mEventQueue->availableToRead();
    }

    size_t eventsToRead = std::min(availableEvents, mEventBuffer.size());
    if (eventsToRead > 0) {
        if (mEventQueue->read(mEventBuffer.data(), eventsToRead)) {
            mEventQueueFlag->wake(ISensors::EVENT_QUEUE_FLAG_BITS_EVENTS_READ);
            for (size_t i = 0; i < eventsToRead; i++) {
                addEvent(mEventBuffer[i]);
            }
        }
    }
}

void SensorsAidlEnvironment::pollingThread(SensorsAidlEnvironment* env) {
    ALOGD("polling thread start");

    while (!env->mStopThread.load()) {
        env->readEvents();
    }

    ALOGD("polling thread end");
}
