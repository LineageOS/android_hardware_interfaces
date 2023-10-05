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

#ifndef ANDROID_SENSORS_VTS_ENVIRONMENT_BASE_H
#define ANDROID_SENSORS_VTS_ENVIRONMENT_BASE_H

#include <gtest/gtest.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <log/log.h>

template <class Event>
class IEventCallback {
  public:
    virtual ~IEventCallback() = default;
    virtual void onEvent(const Event& event) = 0;
};

template <class Event>
class SensorsVtsEnvironmentBase {
  public:
    virtual void SetUp() {
        ASSERT_TRUE(resetHal()) << "could not get hidl service";

        mCollectionEnabled = false;
        startPollingThread();

        // In case framework just stopped for test and there is sensor events in the pipe,
        // wait some time for those events to be cleared to avoid them messing up the test.
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    virtual void TearDown() = 0;

    // Get and clear all events collected so far (like "cat" shell command).
    // If output is nullptr, it clears all collected events.
    void catEvents(std::vector<Event>* output) {
        std::lock_guard<std::mutex> lock(mEventsMutex);
        if (output) {
            output->insert(output->end(), mEvents.begin(), mEvents.end());
        }
        mEvents.clear();
    }

    // set sensor event collection status
    void setCollection(bool enable, const std::optional<std::function<bool(const Event&)>>& filter =
                                            std::nullopt) {
        std::lock_guard<std::mutex> lock(mEventsMutex);
        mCollectionEnabled = enable;

        if (enable && filter.has_value()) {
            mEventFilter = *filter;
        } else {
            mEventFilter.reset();
        }
    }

    void registerCallback(IEventCallback<Event>* callback) {
        std::lock_guard<std::mutex> lock(mEventsMutex);
        mCallback = callback;
    }

    void unregisterCallback() {
        std::lock_guard<std::mutex> lock(mEventsMutex);
        mCallback = nullptr;
    }

    std::vector<Event> collectEvents(
            useconds_t timeLimitUs, size_t nEventLimit, bool clearBeforeStart = true,
            bool changeCollection = true,
            const std::optional<std::function<bool(const Event&)>>& filter = std::nullopt) {
        std::vector<Event> events;
        constexpr useconds_t SLEEP_GRANULARITY = 100 * 1000;  // granularity 100 ms

        ALOGI("collect max of %zu events for %d us, clearBeforeStart %d", nEventLimit, timeLimitUs,
              clearBeforeStart);

        if (changeCollection) {
            setCollection(true, filter);
        }
        if (clearBeforeStart) {
            catEvents(nullptr);
        }

        while (timeLimitUs > 0) {
            useconds_t duration = std::min(SLEEP_GRANULARITY, timeLimitUs);
            usleep(duration);
            timeLimitUs -= duration;

            catEvents(&events);
            if (events.size() >= nEventLimit) {
                break;
            }
            ALOGV("time to go = %d, events to go = %d", (int)timeLimitUs,
                  (int)(nEventLimit - events.size()));
        }

        if (changeCollection) {
            setCollection(false);
        }
        return events;
    }

  protected:
    SensorsVtsEnvironmentBase(const std::string& service_name)
        : mCollectionEnabled(false), mCallback(nullptr) {
        mServiceName = service_name;
    }
    SensorsVtsEnvironmentBase(const SensorsVtsEnvironmentBase&) = delete;
    SensorsVtsEnvironmentBase& operator=(const SensorsVtsEnvironmentBase&) = delete;
    virtual ~SensorsVtsEnvironmentBase(){};

    void addEvent(const Event& ev) {
        std::lock_guard<std::mutex> lock(mEventsMutex);
        if (mCollectionEnabled && (!mEventFilter.has_value() || (*mEventFilter)(ev))) {
            mEvents.push_back(ev);
        }

        if (mCallback != nullptr) {
            mCallback->onEvent(ev);
        }
    }

    virtual void startPollingThread() = 0;
    virtual bool resetHal() = 0;

    std::string mServiceName;
    bool mCollectionEnabled;
    std::atomic_bool mStopThread;
    std::thread mPollThread;
    std::vector<Event> mEvents;
    std::optional<std::function<bool(const Event&)>> mEventFilter;
    std::mutex mEventsMutex;

    IEventCallback<Event>* mCallback;
};

#endif  // ANDROID_SENSORS_VTS_ENVIRONMENT_BASE_H