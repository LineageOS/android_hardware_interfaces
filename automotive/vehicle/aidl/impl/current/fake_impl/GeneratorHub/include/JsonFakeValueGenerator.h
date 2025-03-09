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

#ifndef android_hardware_automotive_vehicle_aidl_impl_fake_impl_GeneratorHub_include_JsonFakeValueGenerator_H_
#define android_hardware_automotive_vehicle_aidl_impl_fake_impl_GeneratorHub_include_JsonFakeValueGenerator_H_

#include "FakeValueGenerator.h"

#include <json/json.h>

#include <iostream>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {

class JsonFakeValueGenerator : public FakeValueGenerator {
  public:
    // Create a new JSON fake value generator. {@code request.value.stringValue} is the JSON file
    // name. {@code request.value.int32Values[1]} if exists, is the number of iterations. If
    // {@code int32Values} has less than 2 elements, number of iterations would be set to -1, which
    // means iterate indefinitely.
    explicit JsonFakeValueGenerator(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& request);
    // Create a new JSON fake value generator using the specified JSON file path. All the events
    // in the JSON file would be generated for number of {@code iteration}. If iteration is 0, no
    // value would be generated. If iteration is less than 0, it would iterate indefinitely.
    explicit JsonFakeValueGenerator(const std::string& path, int32_t iteration);
    // Create a new JSON fake value generator using the specified JSON file path. All the events
    // in the JSON file would be generated once.
    explicit JsonFakeValueGenerator(const std::string& path);
    // Create a new JSON fake value generator using the JSON content. The first argument is just
    // used to differentiate this function with the one that takes path as input.
    explicit JsonFakeValueGenerator(bool unused, const std::string& content, int32_t iteration);

    ~JsonFakeValueGenerator() = default;

    std::optional<aidl::android::hardware::automotive::vehicle::VehiclePropValue> nextEvent()
            override;
    const std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropValue>&
    getAllEvents();

    // Whether there are events left to replay for this generator.
    bool hasNext();

  private:
    size_t mEventIndex = 0;
    std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropValue> mEvents;
    int64_t mLastEventTimestamp = 0;
    int32_t mNumOfIterations = 0;

    void setBit(std::vector<uint8_t>& bytes, size_t idx);
    void initWithPath(const std::string& path, int32_t iteration);
    void initWithStream(std::istream& is, int32_t iteration);
};

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_fake_impl_GeneratorHub_include_JsonFakeValueGenerator_H_
