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

#ifndef android_hardware_gnss_common_vts_GnssCallbackEventQueue_H_
#define android_hardware_gnss_common_vts_GnssCallbackEventQueue_H_

#include <log/log.h>

#include <condition_variable>
#include <deque>
#include <list>
#include <mutex>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

/*
 * Producer/consumer queue for storing/retrieving callback events from GNSS HAL.
 */
template <class T>
class GnssCallbackEventQueue {
  public:
    GnssCallbackEventQueue(const std::string& name) : name_(name), called_count_(0){};
    ~GnssCallbackEventQueue() { reset(); }

    /* Adds callback event to the end of the queue. */
    void store(const T& event);

    /*
     * Removes the callack event at the front of the queue, stores it in event parameter
     * and returns true. Returns false on timeout and event is not populated.
     */
    bool retrieve(T& event, int timeout_seconds);

    /*
     * Removes parameter count number of callack events at the front of the queue, stores
     * them in event_list parameter and returns the number of events retrieved. Waits up to
     * timeout_seconds to retrieve each event. If timeout occurs, it returns the number of
     * items retrieved which will be less than count.
     */
    int retrieve(std::list<T>& event_list, int count, int timeout_seconds);

    /* Returns the number of events pending to be retrieved from the callback event queue. */
    int size() const;

    /* Returns the number of callback events received since last reset(). */
    int calledCount() const;

    /* Clears the callback event queue and resets the calledCount() to 0. */
    void reset();

  private:
    GnssCallbackEventQueue(const GnssCallbackEventQueue&) = delete;
    GnssCallbackEventQueue& operator=(const GnssCallbackEventQueue&) = delete;

    std::string name_;
    int called_count_;
    mutable std::recursive_mutex mtx_;
    std::condition_variable_any cv_;
    std::deque<T> events_;
};

template <class T>
void GnssCallbackEventQueue<T>::store(const T& event) {
    std::unique_lock<std::recursive_mutex> lock(mtx_);
    events_.push_back(event);
    ++called_count_;
    lock.unlock();
    cv_.notify_all();
}

template <class T>
bool GnssCallbackEventQueue<T>::retrieve(T& event, int timeout_seconds) {
    std::unique_lock<std::recursive_mutex> lock(mtx_);
    cv_.wait_for(lock, std::chrono::seconds(timeout_seconds), [&] { return !events_.empty(); });
    if (events_.empty()) {
        return false;
    }
    event = events_.front();
    events_.pop_front();
    return true;
}

template <class T>
int GnssCallbackEventQueue<T>::retrieve(std::list<T>& event_list, int count, int timeout_seconds) {
    for (int i = 0; i < count; ++i) {
        T event;
        if (!retrieve(event, timeout_seconds)) {
            return i;
        }
        event_list.push_back(event);
    }

    return count;
}

template <class T>
int GnssCallbackEventQueue<T>::size() const {
    std::unique_lock<std::recursive_mutex> lock(mtx_);
    return events_.size();
}

template <class T>
int GnssCallbackEventQueue<T>::calledCount() const {
    std::unique_lock<std::recursive_mutex> lock(mtx_);
    return called_count_;
}

template <class T>
void GnssCallbackEventQueue<T>::reset() {
    std::unique_lock<std::recursive_mutex> lock(mtx_);
    if (!events_.empty()) {
        ALOGW("%u unprocessed events discarded in callback queue %s", (unsigned int)events_.size(),
              name_.c_str());
    }
    events_.clear();
    called_count_ = 0;
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_common_vts_GnssCallbackEventQueue_H_
