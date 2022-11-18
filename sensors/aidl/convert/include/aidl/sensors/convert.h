/*
 * Copyright (C) 2022 The Android Open Source Project
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

#pragma once

#include <aidl/android/hardware/sensors/ISensors.h>
#include <hardware/sensors.h>

namespace android {
namespace hardware {
namespace sensors {
namespace implementation {

status_t convertToStatus(ndk::ScopedAStatus status);
void convertToSensor(const aidl::android::hardware::sensors::SensorInfo& src, sensor_t* dst);
void convertToSensorEvent(const aidl::android::hardware::sensors::Event& src, sensors_event_t* dst);
void convertFromSensorEvent(const sensors_event_t& src,
                            aidl::android::hardware::sensors::Event* dst);

}  // namespace implementation
}  // namespace sensors
}  // namespace hardware
}  // namespace android
