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

#ifndef ANDROID_HARDWARE_SENSORS_V2_1_CONVERT_H
#define ANDROID_HARDWARE_SENSORS_V2_1_CONVERT_H

#include <android/hardware/sensors/2.1/types.h>
#include <hardware/sensors.h>
#include <sensors/convert.h>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace implementation {

static_assert(sizeof(V1_0::Event) == sizeof(V2_1::Event),
              "New and old Event types must have the same size");
static_assert(sizeof(V1_0::SensorInfo) == sizeof(V2_1::SensorInfo),
              "New and old SensorInfo types must have the same size");

// The following conversion methods are safe as the only difference between
// V1_0 and V2_1 for these types is an added enum value to SensorType which doesn't
// change the memory layout of the types.
inline const V1_0::Event& convertToOldEvent(const V2_1::Event& event) {
    return reinterpret_cast<const V1_0::Event&>(event);
}

inline const std::vector<V1_0::Event>& convertToOldEvents(const std::vector<V2_1::Event>& events) {
    return reinterpret_cast<const std::vector<V1_0::Event>&>(events);
}

inline V1_0::Event* convertToOldEvent(V2_1::Event* event) {
    return reinterpret_cast<V1_0::Event*>(event);
}

inline const V2_1::SensorInfo& convertToNewSensorInfo(const V1_0::SensorInfo& info) {
    return reinterpret_cast<const V2_1::SensorInfo&>(info);
}

inline const V1_0::SensorInfo& convertToOldSensorInfo(const V2_1::SensorInfo& info) {
    return reinterpret_cast<const V1_0::SensorInfo&>(info);
}

inline const V2_1::Event& convertToNewEvent(const V1_0::Event& event) {
    return reinterpret_cast<const V2_1::Event&>(event);
}

inline const std::vector<V2_1::Event>& convertToNewEvents(const std::vector<V1_0::Event>& events) {
    return reinterpret_cast<const std::vector<V2_1::Event>&>(events);
}

inline const hidl_vec<V2_1::Event>& convertToNewEvents(const hidl_vec<V1_0::Event>& events) {
    return reinterpret_cast<const hidl_vec<V2_1::Event>&>(events);
}

inline const hidl_vec<V2_1::SensorInfo>& convertToNewSensorInfos(
        const hidl_vec<V1_0::SensorInfo>& infos) {
    return reinterpret_cast<const hidl_vec<V2_1::SensorInfo>&>(infos);
}

inline const hidl_vec<V1_0::SensorInfo>& convertToOldSensorInfos(
        const hidl_vec<V2_1::SensorInfo>& infos) {
    return reinterpret_cast<const hidl_vec<V1_0::SensorInfo>&>(infos);
}

inline void convertFromSensorEvent(const sensors_event_t& src, V2_1::Event* dst) {
    switch ((SensorType)src.type) {
        case SensorType::HINGE_ANGLE:
            // Only fill in values for hinge angle as other sensors
            // will have it filled in by legacy code.
            *dst = {
                    .timestamp = src.timestamp,
                    .sensorHandle = src.sensor,
                    .sensorType = (SensorType)src.type,
            };
            dst->u.scalar = src.data[0];
            break;
        default:
            V1_0::implementation::convertFromSensorEvent(src, convertToOldEvent(dst));
            break;
    }
}

inline void convertToSensorEvent(const V2_1::Event& src, sensors_event_t* dst) {
    switch (src.sensorType) {
        case SensorType::HINGE_ANGLE:
            // Only fill in values for hinge angle as other sensors
            // will have it filled in by legacy code.
            *dst = {.version = sizeof(sensors_event_t),
                    .sensor = src.sensorHandle,
                    .type = (int32_t)src.sensorType,
                    .reserved0 = 0,
                    .timestamp = src.timestamp};
            dst->data[0] = src.u.scalar;
            break;
        default:
            V1_0::implementation::convertToSensorEvent(convertToOldEvent(src), dst);
            break;
    }
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_SENSORS_V2_1_CONVERT_H
