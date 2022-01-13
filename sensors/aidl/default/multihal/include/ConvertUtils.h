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

#pragma once

#include <android-base/logging.h>

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {
namespace implementation {

static ::aidl::android::hardware::sensors::SensorInfo convertSensorInfo(
        const ::android::hardware::sensors::V2_1::SensorInfo& sensorInfo) {
    ::aidl::android::hardware::sensors::SensorInfo aidlSensorInfo;
    aidlSensorInfo.sensorHandle = sensorInfo.sensorHandle;
    aidlSensorInfo.name = sensorInfo.name;
    aidlSensorInfo.vendor = sensorInfo.vendor;
    aidlSensorInfo.version = sensorInfo.version;
    aidlSensorInfo.type = (::aidl::android::hardware::sensors::SensorType)sensorInfo.type;
    aidlSensorInfo.typeAsString = sensorInfo.typeAsString;
    aidlSensorInfo.maxRange = sensorInfo.maxRange;
    aidlSensorInfo.resolution = sensorInfo.resolution;
    aidlSensorInfo.power = sensorInfo.power;
    aidlSensorInfo.minDelayUs = sensorInfo.minDelay;
    aidlSensorInfo.fifoReservedEventCount = sensorInfo.fifoReservedEventCount;
    aidlSensorInfo.fifoMaxEventCount = sensorInfo.fifoMaxEventCount;
    aidlSensorInfo.requiredPermission = sensorInfo.requiredPermission;
    aidlSensorInfo.maxDelayUs = sensorInfo.maxDelay;
    aidlSensorInfo.flags = sensorInfo.flags;
    return aidlSensorInfo;
}

static void convertToHidlEvent(const ::aidl::android::hardware::sensors::Event& aidlEvent,
                               ::android::hardware::sensors::V2_1::Event* hidlEvent) {
    hidlEvent->timestamp = aidlEvent.timestamp;
    hidlEvent->sensorHandle = aidlEvent.sensorHandle;
    hidlEvent->sensorType = (::android::hardware::sensors::V2_1::SensorType)aidlEvent.sensorType;

    switch (aidlEvent.sensorType) {
        case ::aidl::android::hardware::sensors::SensorType::META_DATA:
            hidlEvent->u.meta.what =
                    (::android::hardware::sensors::V1_0::MetaDataEventType)aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::meta>()
                            .what;
            break;
        case ::aidl::android::hardware::sensors::SensorType::ACCELEROMETER:
        case ::aidl::android::hardware::sensors::SensorType::MAGNETIC_FIELD:
        case ::aidl::android::hardware::sensors::SensorType::ORIENTATION:
        case ::aidl::android::hardware::sensors::SensorType::GYROSCOPE:
        case ::aidl::android::hardware::sensors::SensorType::GRAVITY:
        case ::aidl::android::hardware::sensors::SensorType::LINEAR_ACCELERATION:
            hidlEvent->u.vec3.x =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::vec3>()
                            .x;
            hidlEvent->u.vec3.y =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::vec3>()
                            .y;
            hidlEvent->u.vec3.z =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::vec3>()
                            .z;
            break;
        case ::aidl::android::hardware::sensors::SensorType::GAME_ROTATION_VECTOR:
            hidlEvent->u.vec4.x =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::vec4>()
                            .x;
            hidlEvent->u.vec4.y =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::vec4>()
                            .y;
            hidlEvent->u.vec4.z =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::vec4>()
                            .z;
            hidlEvent->u.vec4.w =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::vec4>()
                            .w;
            break;
        case ::aidl::android::hardware::sensors::SensorType::ROTATION_VECTOR:
        case ::aidl::android::hardware::sensors::SensorType::GEOMAGNETIC_ROTATION_VECTOR:
            std::copy(aidlEvent.payload
                              .get<::aidl::android::hardware::sensors::Event::EventPayload::data>()
                              .values.data(),
                      aidlEvent.payload
                                      .get<::aidl::android::hardware::sensors::Event::EventPayload::
                                                   data>()
                                      .values.data() +
                              5,
                      hidlEvent->u.data.data());
            break;
        case ::aidl::android::hardware::sensors::SensorType::ACCELEROMETER_UNCALIBRATED:
        case ::aidl::android::hardware::sensors::SensorType::MAGNETIC_FIELD_UNCALIBRATED:
        case ::aidl::android::hardware::sensors::SensorType::GYROSCOPE_UNCALIBRATED:
            hidlEvent->u.uncal.x =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::uncal>()
                            .x;
            hidlEvent->u.uncal.y =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::uncal>()
                            .y;
            hidlEvent->u.uncal.z =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::uncal>()
                            .z;
            hidlEvent->u.uncal.x_bias =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::uncal>()
                            .xBias;
            hidlEvent->u.uncal.y_bias =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::uncal>()
                            .yBias;
            hidlEvent->u.uncal.z_bias =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::uncal>()
                            .zBias;
            break;
        case ::aidl::android::hardware::sensors::SensorType::DEVICE_ORIENTATION:
        case ::aidl::android::hardware::sensors::SensorType::LIGHT:
        case ::aidl::android::hardware::sensors::SensorType::PRESSURE:
        case ::aidl::android::hardware::sensors::SensorType::PROXIMITY:
        case ::aidl::android::hardware::sensors::SensorType::RELATIVE_HUMIDITY:
        case ::aidl::android::hardware::sensors::SensorType::AMBIENT_TEMPERATURE:
        case ::aidl::android::hardware::sensors::SensorType::SIGNIFICANT_MOTION:
        case ::aidl::android::hardware::sensors::SensorType::STEP_DETECTOR:
        case ::aidl::android::hardware::sensors::SensorType::TILT_DETECTOR:
        case ::aidl::android::hardware::sensors::SensorType::WAKE_GESTURE:
        case ::aidl::android::hardware::sensors::SensorType::GLANCE_GESTURE:
        case ::aidl::android::hardware::sensors::SensorType::PICK_UP_GESTURE:
        case ::aidl::android::hardware::sensors::SensorType::WRIST_TILT_GESTURE:
        case ::aidl::android::hardware::sensors::SensorType::STATIONARY_DETECT:
        case ::aidl::android::hardware::sensors::SensorType::MOTION_DETECT:
        case ::aidl::android::hardware::sensors::SensorType::HEART_BEAT:
        case ::aidl::android::hardware::sensors::SensorType::LOW_LATENCY_OFFBODY_DETECT:
        case ::aidl::android::hardware::sensors::SensorType::HINGE_ANGLE:
            hidlEvent->u.scalar =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::scalar>();
            break;
        case ::aidl::android::hardware::sensors::SensorType::STEP_COUNTER:
            hidlEvent->u.stepCount = aidlEvent.payload.get<
                    ::aidl::android::hardware::sensors::Event::EventPayload::stepCount>();
            break;
        case ::aidl::android::hardware::sensors::SensorType::HEART_RATE:
            hidlEvent->u.heartRate.bpm = aidlEvent.payload
                                                 .get<::aidl::android::hardware::sensors::Event::
                                                              EventPayload::heartRate>()
                                                 .bpm;
            hidlEvent->u.heartRate.status =
                    (::android::hardware::sensors::V1_0::SensorStatus)aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::
                                         heartRate>()
                            .status;
            break;
        case ::aidl::android::hardware::sensors::SensorType::POSE_6DOF:
            std::copy(std::begin(aidlEvent.payload
                                         .get<::aidl::android::hardware::sensors::Event::
                                                      EventPayload::pose6DOF>()
                                         .values),
                      std::end(aidlEvent.payload
                                       .get<::aidl::android::hardware::sensors::Event::
                                                    EventPayload::data>()
                                       .values),
                      hidlEvent->u.pose6DOF.data());
            break;
        case ::aidl::android::hardware::sensors::SensorType::DYNAMIC_SENSOR_META:
            hidlEvent->u.dynamic.connected =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::dynamic>()
                            .connected;
            hidlEvent->u.dynamic.sensorHandle =
                    aidlEvent.payload
                            .get<::aidl::android::hardware::sensors::Event::EventPayload::dynamic>()
                            .sensorHandle;
            std::copy(std::begin(aidlEvent.payload
                                         .get<::aidl::android::hardware::sensors::Event::
                                                      EventPayload::dynamic>()
                                         .uuid.values),
                      std::end(aidlEvent.payload
                                       .get<::aidl::android::hardware::sensors::Event::
                                                    EventPayload::dynamic>()
                                       .uuid.values),
                      hidlEvent->u.dynamic.uuid.data());
            break;
        case ::aidl::android::hardware::sensors::SensorType::ADDITIONAL_INFO: {
            const AdditionalInfo& additionalInfo = aidlEvent.payload.get<
                    ::aidl::android::hardware::sensors::Event::EventPayload::additional>();
            hidlEvent->u.additional.type =
                    (::android::hardware::sensors::V1_0::AdditionalInfoType)additionalInfo.type;
            hidlEvent->u.additional.serial = additionalInfo.serial;

            switch (additionalInfo.payload.getTag()) {
                case ::aidl::android::hardware::sensors::AdditionalInfo::AdditionalInfoPayload::
                        Tag::dataInt32:
                    std::copy(
                            std::begin(additionalInfo.payload
                                               .get<::aidl::android::hardware::sensors::
                                                            AdditionalInfo::AdditionalInfoPayload::
                                                                    dataInt32>()
                                               .values),
                            std::end(additionalInfo.payload
                                             .get<::aidl::android::hardware::sensors::
                                                          AdditionalInfo::AdditionalInfoPayload::
                                                                  dataInt32>()
                                             .values),
                            hidlEvent->u.additional.u.data_int32.data());
                    break;
                case ::aidl::android::hardware::sensors::AdditionalInfo::AdditionalInfoPayload::
                        Tag::dataFloat:
                    std::copy(
                            std::begin(additionalInfo.payload
                                               .get<::aidl::android::hardware::sensors::
                                                            AdditionalInfo::AdditionalInfoPayload::
                                                                    dataFloat>()
                                               .values),
                            std::end(additionalInfo.payload
                                             .get<::aidl::android::hardware::sensors::
                                                          AdditionalInfo::AdditionalInfoPayload::
                                                                  dataFloat>()
                                             .values),
                            hidlEvent->u.additional.u.data_float.data());
                    break;
                default:
                    ALOGE("Invalid sensor additioanl info tag: %d",
                          additionalInfo.payload.getTag());
                    break;
            }
            break;
        }
        default:
            CHECK_GE((int32_t)aidlEvent.sensorType,
                     (int32_t)::aidl::android::hardware::sensors::SensorType::DEVICE_PRIVATE_BASE);
            std::copy(std::begin(aidlEvent.payload
                                         .get<::aidl::android::hardware::sensors::Event::
                                                      EventPayload::data>()
                                         .values),
                      std::end(aidlEvent.payload
                                       .get<::aidl::android::hardware::sensors::Event::
                                                    EventPayload::data>()
                                       .values),
                      hidlEvent->u.data.data());
            break;
    }
}

static void convertToAidlEvent(const ::android::hardware::sensors::V2_1::Event& hidlEvent,
                               ::aidl::android::hardware::sensors::Event* aidlEvent) {
    aidlEvent->timestamp = hidlEvent.timestamp;
    aidlEvent->sensorHandle = hidlEvent.sensorHandle;
    aidlEvent->sensorType = (::aidl::android::hardware::sensors::SensorType)hidlEvent.sensorType;
    switch (hidlEvent.sensorType) {
        case ::android::hardware::sensors::V2_1::SensorType::META_DATA: {
            ::aidl::android::hardware::sensors::Event::EventPayload::MetaData meta;
            meta.what = (::aidl::android::hardware::sensors::Event::EventPayload::MetaData::
                                 MetaDataEventType)hidlEvent.u.meta.what;
            aidlEvent->payload.set<::aidl::android::hardware::sensors::Event::EventPayload::meta>(
                    meta);
            break;
        }
        case ::android::hardware::sensors::V2_1::SensorType::ACCELEROMETER:
        case ::android::hardware::sensors::V2_1::SensorType::MAGNETIC_FIELD:
        case ::android::hardware::sensors::V2_1::SensorType::ORIENTATION:
        case ::android::hardware::sensors::V2_1::SensorType::GYROSCOPE:
        case ::android::hardware::sensors::V2_1::SensorType::GRAVITY:
        case ::android::hardware::sensors::V2_1::SensorType::LINEAR_ACCELERATION: {
            ::aidl::android::hardware::sensors::Event::EventPayload::Vec3 vec3;
            vec3.x = hidlEvent.u.vec3.x;
            vec3.y = hidlEvent.u.vec3.y;
            vec3.z = hidlEvent.u.vec3.z;
            aidlEvent->payload.set<::aidl::android::hardware::sensors::Event::EventPayload::vec3>(
                    vec3);
            break;
        }
        case ::android::hardware::sensors::V2_1::SensorType::GAME_ROTATION_VECTOR: {
            ::aidl::android::hardware::sensors::Event::EventPayload::Vec4 vec4;
            vec4.x = hidlEvent.u.vec4.x;
            vec4.y = hidlEvent.u.vec4.y;
            vec4.z = hidlEvent.u.vec4.z;
            vec4.w = hidlEvent.u.vec4.w;
            aidlEvent->payload.set<::aidl::android::hardware::sensors::Event::EventPayload::vec4>(
                    vec4);
            break;
        }
        case ::android::hardware::sensors::V2_1::SensorType::ROTATION_VECTOR:
        case ::android::hardware::sensors::V2_1::SensorType::GEOMAGNETIC_ROTATION_VECTOR: {
            ::aidl::android::hardware::sensors::Event::EventPayload::Data data;
            std::copy(hidlEvent.u.data.data(), hidlEvent.u.data.data() + 5,
                      std::begin(data.values));
            aidlEvent->payload.set<::aidl::android::hardware::sensors::Event::EventPayload::data>(
                    data);
            break;
        }
        case ::android::hardware::sensors::V2_1::SensorType::MAGNETIC_FIELD_UNCALIBRATED:
        case ::android::hardware::sensors::V2_1::SensorType::GYROSCOPE_UNCALIBRATED:
        case ::android::hardware::sensors::V2_1::SensorType::ACCELEROMETER_UNCALIBRATED: {
            ::aidl::android::hardware::sensors::Event::EventPayload::Uncal uncal;
            uncal.x = hidlEvent.u.uncal.x;
            uncal.y = hidlEvent.u.uncal.y;
            uncal.z = hidlEvent.u.uncal.z;
            uncal.xBias = hidlEvent.u.uncal.x_bias;
            uncal.yBias = hidlEvent.u.uncal.y_bias;
            uncal.zBias = hidlEvent.u.uncal.z_bias;
            aidlEvent->payload.set<::aidl::android::hardware::sensors::Event::EventPayload::uncal>(
                    uncal);
            break;
        }
        case ::android::hardware::sensors::V2_1::SensorType::DEVICE_ORIENTATION:
        case ::android::hardware::sensors::V2_1::SensorType::LIGHT:
        case ::android::hardware::sensors::V2_1::SensorType::PRESSURE:
        case ::android::hardware::sensors::V2_1::SensorType::PROXIMITY:
        case ::android::hardware::sensors::V2_1::SensorType::RELATIVE_HUMIDITY:
        case ::android::hardware::sensors::V2_1::SensorType::AMBIENT_TEMPERATURE:
        case ::android::hardware::sensors::V2_1::SensorType::SIGNIFICANT_MOTION:
        case ::android::hardware::sensors::V2_1::SensorType::STEP_DETECTOR:
        case ::android::hardware::sensors::V2_1::SensorType::TILT_DETECTOR:
        case ::android::hardware::sensors::V2_1::SensorType::WAKE_GESTURE:
        case ::android::hardware::sensors::V2_1::SensorType::GLANCE_GESTURE:
        case ::android::hardware::sensors::V2_1::SensorType::PICK_UP_GESTURE:
        case ::android::hardware::sensors::V2_1::SensorType::WRIST_TILT_GESTURE:
        case ::android::hardware::sensors::V2_1::SensorType::STATIONARY_DETECT:
        case ::android::hardware::sensors::V2_1::SensorType::MOTION_DETECT:
        case ::android::hardware::sensors::V2_1::SensorType::HEART_BEAT:
        case ::android::hardware::sensors::V2_1::SensorType::LOW_LATENCY_OFFBODY_DETECT:
        case ::android::hardware::sensors::V2_1::SensorType::HINGE_ANGLE:
            aidlEvent->payload.set<::aidl::android::hardware::sensors::Event::EventPayload::scalar>(
                    hidlEvent.u.scalar);
            break;
        case ::android::hardware::sensors::V2_1::SensorType::STEP_COUNTER:
            aidlEvent->payload
                    .set<::aidl::android::hardware::sensors::Event::EventPayload::stepCount>(
                            hidlEvent.u.stepCount);
            break;
        case ::android::hardware::sensors::V2_1::SensorType::HEART_RATE: {
            ::aidl::android::hardware::sensors::Event::EventPayload::HeartRate heartRate;
            heartRate.bpm = hidlEvent.u.heartRate.bpm;
            heartRate.status =
                    (::aidl::android::hardware::sensors::SensorStatus)hidlEvent.u.heartRate.status;
            aidlEvent->payload
                    .set<::aidl::android::hardware::sensors::Event::EventPayload::heartRate>(
                            heartRate);
            break;
        }
        case ::android::hardware::sensors::V2_1::SensorType::POSE_6DOF: {
            ::aidl::android::hardware::sensors::Event::EventPayload::Pose6Dof pose6Dof;
            std::copy(hidlEvent.u.pose6DOF.data(),
                      hidlEvent.u.pose6DOF.data() + hidlEvent.u.pose6DOF.size(),
                      std::begin(pose6Dof.values));
            aidlEvent->payload
                    .set<::aidl::android::hardware::sensors::Event::EventPayload::pose6DOF>(
                            pose6Dof);
            break;
        }
        case ::android::hardware::sensors::V2_1::SensorType::DYNAMIC_SENSOR_META: {
            ::aidl::android::hardware::sensors::DynamicSensorInfo dynamicSensorInfo;
            dynamicSensorInfo.connected = hidlEvent.u.dynamic.connected;
            dynamicSensorInfo.sensorHandle = hidlEvent.u.dynamic.sensorHandle;
            std::copy(hidlEvent.u.dynamic.uuid.data(),
                      hidlEvent.u.dynamic.uuid.data() + hidlEvent.u.dynamic.uuid.size(),
                      std::begin(dynamicSensorInfo.uuid.values));
            aidlEvent->payload
                    .set<::aidl::android::hardware::sensors::Event::EventPayload::dynamic>(
                            dynamicSensorInfo);
            break;
        }
        case ::android::hardware::sensors::V2_1::SensorType::ADDITIONAL_INFO: {
            ::aidl::android::hardware::sensors::AdditionalInfo additionalInfo;
            additionalInfo.type =
                    (::aidl::android::hardware::sensors::AdditionalInfo::AdditionalInfoType)
                            hidlEvent.u.additional.type;
            additionalInfo.serial = hidlEvent.u.additional.serial;

            ::aidl::android::hardware::sensors::AdditionalInfo::AdditionalInfoPayload::Int32Values
                    int32Values;
            std::copy(hidlEvent.u.additional.u.data_int32.data(),
                      hidlEvent.u.additional.u.data_int32.data() +
                              hidlEvent.u.additional.u.data_int32.size(),
                      std::begin(int32Values.values));
            additionalInfo.payload.set<::aidl::android::hardware::sensors::AdditionalInfo::
                                               AdditionalInfoPayload::dataInt32>(int32Values);
            aidlEvent->payload
                    .set<::aidl::android::hardware::sensors::Event::EventPayload::additional>(
                            additionalInfo);
            break;
        }
        default: {
            CHECK_GE((int32_t)hidlEvent.sensorType,
                     (int32_t)::android::hardware::sensors::V2_1::SensorType::DEVICE_PRIVATE_BASE);
            ::aidl::android::hardware::sensors::Event::EventPayload::Data data;
            std::copy(hidlEvent.u.data.data(), hidlEvent.u.data.data() + hidlEvent.u.data.size(),
                      std::begin(data.values));
            aidlEvent->payload.set<::aidl::android::hardware::sensors::Event::EventPayload::data>(
                    data);
            break;
        }
    }
}

}  // namespace implementation
}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl