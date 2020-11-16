/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_SENSORS_V2_0_H
#define ANDROID_HARDWARE_SENSORS_V2_0_H

#include "Sensors.h"

#include <android/hardware/sensors/2.0/ISensors.h>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

struct SensorsV2_0 : public ::android::hardware::sensors::V2_X::implementation::Sensors<ISensors> {
  Return<void>
  getSensorsList(V2_0::ISensors::getSensorsList_cb _hidl_cb) override {
    std::vector<V1_0::SensorInfo> sensors;
    for (const auto &sensor : mSensors) {
      sensors.push_back(V2_1::implementation::convertToOldSensorInfo(
          sensor.second->getSensorInfo()));
    }

    // Call the HIDL callback with the SensorInfo
    _hidl_cb(sensors);

    return Void();
  }
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_SENSORS_V2_0_H