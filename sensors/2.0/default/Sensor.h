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

#ifndef ANDROID_HARDWARE_SENSORS_V2_0_SENSOR_H
#define ANDROID_HARDWARE_SENSORS_V2_0_SENSOR_H

#include <android/hardware/sensors/1.0/types.h>

using ::android::hardware::sensors::V1_0::SensorInfo;

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

class Sensor {
   public:
    Sensor();

    const SensorInfo& getSensorInfo() const;
    bool batch(int32_t samplingPeriodNs);
    void activate(bool enable);

   protected:
    bool mIsEnabled;
    int64_t mSamplingPeriodNs;
    SensorInfo mSensorInfo;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_SENSORS_V2_0_SENSOR_H