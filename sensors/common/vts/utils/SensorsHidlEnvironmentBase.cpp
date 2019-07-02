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

#include "SensorsHidlEnvironmentBase.h"

void SensorsHidlEnvironmentBase::HidlSetUp() {
    ASSERT_TRUE(resetHal()) << "could not get hidl service";

    mCollectionEnabled = false;
    startPollingThread();

    // In case framework just stopped for test and there is sensor events in the pipe,
    // wait some time for those events to be cleared to avoid them messing up the test.
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

void SensorsHidlEnvironmentBase::HidlTearDown() {
    mStopThread = true;
    mPollThread.detach();
}

void SensorsHidlEnvironmentBase::catEvents(std::vector<Event>* output) {
    std::lock_guard<std::mutex> lock(mEventsMutex);
    if (output) {
        output->insert(output->end(), mEvents.begin(), mEvents.end());
    }
    mEvents.clear();
}

void SensorsHidlEnvironmentBase::setCollection(bool enable) {
    std::lock_guard<std::mutex> lock(mEventsMutex);
    mCollectionEnabled = enable;
}

void SensorsHidlEnvironmentBase::addEvent(const Event& ev) {
    std::lock_guard<std::mutex> lock(mEventsMutex);
    if (mCollectionEnabled) {
        mEvents.push_back(ev);
    }

    if (mCallback != nullptr) {
        mCallback->onEvent(ev);
    }
}

void SensorsHidlEnvironmentBase::registerCallback(IEventCallback* callback) {
    std::lock_guard<std::mutex> lock(mEventsMutex);
    mCallback = callback;
}

void SensorsHidlEnvironmentBase::unregisterCallback() {
    std::lock_guard<std::mutex> lock(mEventsMutex);
    mCallback = nullptr;
}