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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_GeneratorHub_H_
#define android_hardware_automotive_vehicle_V2_0_impl_GeneratorHub_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <queue>
#include <thread>
#include <unordered_map>

#include "FakeValueGenerator.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

/**
 * This is the scheduler for all VHAL event generators. It manages all generators and uses priority
 * queue to maintain generated events ordered by timestamp. The scheduler uses a single thread to
 * keep querying and updating the event queue to make sure events from all generators are produced
 * in order.
 */
class GeneratorHub {
private:
    struct VhalEvent {
        int32_t cookie;  // Cookie is used to find the associated generator.
        VehiclePropValue val;
    };
    // Comparator used by priority queue to keep track of soonest event.
    struct GreaterByTime {
        bool operator()(const VhalEvent& lhs, const VhalEvent& rhs) const {
            return lhs.val.timestamp > rhs.val.timestamp;
        }
    };

    using OnHalEvent = std::function<void(const VehiclePropValue& event)>;

public:
    GeneratorHub(const OnHalEvent& onHalEvent);
    ~GeneratorHub();

    /**
     * Register a new generator. The generator will be discarded if it could not produce next event.
     * The existing generator will be overridden if it has the same cookie.
     */
    void registerGenerator(int32_t cookie, FakeValueGeneratorPtr generator);

    void unregisterGenerator(int32_t cookie);

private:
    /**
     * Main loop of the single thread to producing event and updating event queue.
     */
    void run();

    bool hasNext(int32_t cookie);

private:
    std::priority_queue<VhalEvent, std::vector<VhalEvent>, GreaterByTime> mEventQueue;
    std::unordered_map<int32_t, FakeValueGeneratorPtr> mGenerators;
    OnHalEvent mOnHalEvent;

    mutable std::mutex mLock;
    std::condition_variable mCond;
    std::thread mThread;
    std::atomic<bool> mShuttingDownFlag{false};
};

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_impl_GeneratorHub_H_
