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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_JsonFakeValueGenerator_H_
#define android_hardware_automotive_vehicle_V2_0_impl_JsonFakeValueGenerator_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>

#include <json/json.h>

#include "FakeValueGenerator.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

class JsonFakeValueGenerator : public FakeValueGenerator {
private:
    using Nanos = std::chrono::nanoseconds;
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock, Nanos>;

    struct GeneratorCfg {
        size_t index;
        std::vector<VehiclePropValue> events;
    };

public:
    JsonFakeValueGenerator(const OnHalEvent& onHalEvent);
    ~JsonFakeValueGenerator();
    StatusCode start(const VehiclePropValue& request) override;
    StatusCode stop(const VehiclePropValue& request) override;

private:
    std::vector<VehiclePropValue> parseFakeValueJson(std::istream& is);
    void loop();

private:
    OnHalEvent mOnHalEvent;
    std::thread mThread;
    mutable std::mutex mLock;
    std::condition_variable mCond;
    GeneratorCfg mGenCfg;
    std::atomic_bool mStopRequested{false};
};

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_impl_JsonFakeValueGenerator_H_
