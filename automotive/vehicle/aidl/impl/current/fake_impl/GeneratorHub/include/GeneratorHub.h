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

#ifndef android_hardware_interfaces_automotive_vehicle_aidl_fake_impl_GeneratorHub_include_GeneratorHub_h_
#define android_hardware_interfaces_automotive_vehicle_aidl_fake_impl_GeneratorHub_include_GeneratorHub_h_

#include "FakeValueGenerator.h"

#include <android-base/thread_annotations.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {

// This is the scheduler for all VHAL event generators. It manages all generators and uses priority
// queue to maintain generated events ordered by timestamp. The scheduler uses a single thread to
// keep querying and updating the event queue to make sure events from all generators are produced
// in order.
class GeneratorHub {
  public:
    using OnHalEvent = std::function<void(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& event)>;

    explicit GeneratorHub(OnHalEvent&& onHalEvent);
    ~GeneratorHub();

    // Register a new generator. The generator will be discarded if it could not produce next event.
    // The existing generator will be overridden if it has the same generatorId.
    void registerGenerator(int32_t generatorId, std::unique_ptr<FakeValueGenerator> generator);

    // Unregister a generator with the generatorId. If no registered generator is found, this
    // function does nothing. Returns true if the generator is unregistered.
    bool unregisterGenerator(int32_t generatorId);

  private:
    struct VhalEvent {
        int32_t generatorId;
        aidl::android::hardware::automotive::vehicle::VehiclePropValue val;
    };

    // Comparator used by priority queue to keep track of soonest event.
    struct GreaterByTime {
        bool operator()(const VhalEvent& lhs, const VhalEvent& rhs) const {
            return lhs.val.timestamp > rhs.val.timestamp;
        }
    };

    std::priority_queue<VhalEvent, std::vector<VhalEvent>, GreaterByTime> mEventQueue;
    std::mutex mGeneratorsLock;
    std::unordered_map<int32_t, std::unique_ptr<FakeValueGenerator>> mGenerators
            GUARDED_BY(mGeneratorsLock);
    OnHalEvent mOnHalEvent;
    std::condition_variable mCond;
    std::thread mThread;
    std::atomic<bool> mShuttingDownFlag{false};

    // Main loop of the single thread to producing event and updating event queue.
    void run();
};

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_interfaces_automotive_vehicle_aidl_fake_impl_GeneratorHub_include_GeneratorHub_h_
