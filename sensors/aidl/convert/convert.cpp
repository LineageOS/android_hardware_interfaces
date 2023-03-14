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

#include "aidl/sensors/convert.h"
#include "android-base/logging.h"

namespace android {
namespace hardware {
namespace sensors {
namespace implementation {

using aidl::android::hardware::sensors::AdditionalInfo;
using aidl::android::hardware::sensors::DynamicSensorInfo;
using aidl::android::hardware::sensors::Event;
using aidl::android::hardware::sensors::ISensors;
using aidl::android::hardware::sensors::SensorInfo;
using aidl::android::hardware::sensors::SensorStatus;
using aidl::android::hardware::sensors::SensorType;

status_t convertToStatus(ndk::ScopedAStatus status) {
    if (status.isOk()) {
        return OK;
    } else {
        switch (status.getExceptionCode()) {
            case EX_ILLEGAL_ARGUMENT: {
                return BAD_VALUE;
            }
            case EX_SECURITY: {
                return PERMISSION_DENIED;
            }
            case EX_UNSUPPORTED_OPERATION: {
                return INVALID_OPERATION;
            }
            case EX_SERVICE_SPECIFIC: {
                switch (status.getServiceSpecificError()) {
                    case ISensors::ERROR_BAD_VALUE: {
                        return BAD_VALUE;
                    }
                    case ISensors::ERROR_NO_MEMORY: {
                        return NO_MEMORY;
                    }
                    default: {
                        return UNKNOWN_ERROR;
                    }
                }
            }
            default: {
                return UNKNOWN_ERROR;
            }
        }
    }
}

void convertToSensor(const SensorInfo& src, sensor_t* dst) {
    dst->name = strdup(src.name.c_str());
    dst->vendor = strdup(src.vendor.c_str());
    dst->version = src.version;
    dst->handle = src.sensorHandle;
    dst->type = (int)src.type;
    dst->maxRange = src.maxRange;
    dst->resolution = src.resolution;
    dst->power = src.power;
    dst->minDelay = src.minDelayUs;
    dst->fifoReservedEventCount = src.fifoReservedEventCount;
    dst->fifoMaxEventCount = src.fifoMaxEventCount;
    dst->stringType = strdup(src.typeAsString.c_str());
    dst->requiredPermission = strdup(src.requiredPermission.c_str());
    dst->maxDelay = src.maxDelayUs;
    dst->flags = src.flags;
    dst->reserved[0] = dst->reserved[1] = 0;
}

void convertToSensorEvent(const Event& src, sensors_event_t* dst) {
    *dst = {.version = sizeof(sensors_event_t),
            .sensor = src.sensorHandle,
            .type = (int32_t)src.sensorType,
            .reserved0 = 0,
            .timestamp = src.timestamp};

    switch (src.sensorType) {
        case SensorType::META_DATA: {
            // Legacy HALs expect the handle reference in the meta data field.
            // Copy it over from the handle of the event.
            dst->meta_data.what = (int32_t)src.payload.get<Event::EventPayload::meta>().what;
            dst->meta_data.sensor = src.sensorHandle;
            // Set the sensor handle to 0 to maintain compatibility.
            dst->sensor = 0;
            break;
        }

        case SensorType::ACCELEROMETER:
        case SensorType::MAGNETIC_FIELD:
        case SensorType::ORIENTATION:
        case SensorType::GYROSCOPE:
        case SensorType::GRAVITY:
        case SensorType::LINEAR_ACCELERATION: {
            dst->acceleration.x = src.payload.get<Event::EventPayload::vec3>().x;
            dst->acceleration.y = src.payload.get<Event::EventPayload::vec3>().y;
            dst->acceleration.z = src.payload.get<Event::EventPayload::vec3>().z;
            dst->acceleration.status = (int32_t)src.payload.get<Event::EventPayload::vec3>().status;
            break;
        }

        case SensorType::GAME_ROTATION_VECTOR: {
            dst->data[0] = src.payload.get<Event::EventPayload::vec4>().x;
            dst->data[1] = src.payload.get<Event::EventPayload::vec4>().y;
            dst->data[2] = src.payload.get<Event::EventPayload::vec4>().z;
            dst->data[3] = src.payload.get<Event::EventPayload::vec4>().w;
            break;
        }

        case SensorType::ROTATION_VECTOR:
        case SensorType::GEOMAGNETIC_ROTATION_VECTOR: {
            dst->data[0] = src.payload.get<Event::EventPayload::data>().values[0];
            dst->data[1] = src.payload.get<Event::EventPayload::data>().values[1];
            dst->data[2] = src.payload.get<Event::EventPayload::data>().values[2];
            dst->data[3] = src.payload.get<Event::EventPayload::data>().values[3];
            dst->data[4] = src.payload.get<Event::EventPayload::data>().values[4];
            break;
        }

        case SensorType::MAGNETIC_FIELD_UNCALIBRATED:
        case SensorType::GYROSCOPE_UNCALIBRATED:
        case SensorType::ACCELEROMETER_UNCALIBRATED: {
            dst->uncalibrated_gyro.x_uncalib = src.payload.get<Event::EventPayload::uncal>().x;
            dst->uncalibrated_gyro.y_uncalib = src.payload.get<Event::EventPayload::uncal>().y;
            dst->uncalibrated_gyro.z_uncalib = src.payload.get<Event::EventPayload::uncal>().z;
            dst->uncalibrated_gyro.x_bias = src.payload.get<Event::EventPayload::uncal>().xBias;
            dst->uncalibrated_gyro.y_bias = src.payload.get<Event::EventPayload::uncal>().yBias;
            dst->uncalibrated_gyro.z_bias = src.payload.get<Event::EventPayload::uncal>().zBias;
            break;
        }

        case SensorType::HINGE_ANGLE:
        case SensorType::DEVICE_ORIENTATION:
        case SensorType::LIGHT:
        case SensorType::PRESSURE:
        case SensorType::PROXIMITY:
        case SensorType::RELATIVE_HUMIDITY:
        case SensorType::AMBIENT_TEMPERATURE:
        case SensorType::SIGNIFICANT_MOTION:
        case SensorType::STEP_DETECTOR:
        case SensorType::TILT_DETECTOR:
        case SensorType::WAKE_GESTURE:
        case SensorType::GLANCE_GESTURE:
        case SensorType::PICK_UP_GESTURE:
        case SensorType::WRIST_TILT_GESTURE:
        case SensorType::STATIONARY_DETECT:
        case SensorType::MOTION_DETECT:
        case SensorType::HEART_BEAT:
        case SensorType::LOW_LATENCY_OFFBODY_DETECT: {
            dst->data[0] = src.payload.get<Event::EventPayload::scalar>();
            break;
        }

        case SensorType::STEP_COUNTER: {
            dst->u64.step_counter = src.payload.get<Event::EventPayload::stepCount>();
            break;
        }

        case SensorType::HEART_RATE: {
            dst->heart_rate.bpm = src.payload.get<Event::EventPayload::heartRate>().bpm;
            dst->heart_rate.status =
                    (int8_t)src.payload.get<Event::EventPayload::heartRate>().status;
            break;
        }

        case SensorType::POSE_6DOF: {  // 15 floats
            for (size_t i = 0; i < 15; ++i) {
                dst->data[i] = src.payload.get<Event::EventPayload::pose6DOF>().values[i];
            }
            break;
        }

        case SensorType::DYNAMIC_SENSOR_META: {
            dst->dynamic_sensor_meta.connected =
                    src.payload.get<Event::EventPayload::dynamic>().connected;
            dst->dynamic_sensor_meta.handle =
                    src.payload.get<Event::EventPayload::dynamic>().sensorHandle;
            dst->dynamic_sensor_meta.sensor = NULL;  // to be filled in later

            memcpy(dst->dynamic_sensor_meta.uuid,
                   src.payload.get<Event::EventPayload::dynamic>().uuid.values.data(), 16);

            break;
        }

        case SensorType::ADDITIONAL_INFO: {
            const AdditionalInfo& srcInfo = src.payload.get<Event::EventPayload::additional>();

            additional_info_event_t* dstInfo = &dst->additional_info;
            dstInfo->type = (int32_t)srcInfo.type;
            dstInfo->serial = srcInfo.serial;

            switch (srcInfo.payload.getTag()) {
                case AdditionalInfo::AdditionalInfoPayload::Tag::dataInt32: {
                    const auto& values =
                            srcInfo.payload.get<AdditionalInfo::AdditionalInfoPayload::dataInt32>()
                                    .values;
                    CHECK_EQ(values.size() * sizeof(int32_t), sizeof(dstInfo->data_int32));
                    memcpy(dstInfo->data_int32, values.data(), sizeof(dstInfo->data_int32));
                    break;
                }
                case AdditionalInfo::AdditionalInfoPayload::Tag::dataFloat: {
                    const auto& values =
                            srcInfo.payload.get<AdditionalInfo::AdditionalInfoPayload::dataFloat>()
                                    .values;
                    CHECK_EQ(values.size() * sizeof(float), sizeof(dstInfo->data_float));
                    memcpy(dstInfo->data_float, values.data(), sizeof(dstInfo->data_float));
                    break;
                }
                default: {
                    LOG(ERROR) << "Invalid sensor additional info tag: ",
                            (int)srcInfo.payload.getTag();
                }
            }
            break;
        }

        case SensorType::HEAD_TRACKER: {
            const auto& ht = src.payload.get<Event::EventPayload::headTracker>();
            dst->head_tracker.rx = ht.rx;
            dst->head_tracker.ry = ht.ry;
            dst->head_tracker.rz = ht.rz;
            dst->head_tracker.vx = ht.vx;
            dst->head_tracker.vy = ht.vy;
            dst->head_tracker.vz = ht.vz;
            dst->head_tracker.discontinuity_count = ht.discontinuityCount;
            break;
        }

        case SensorType::ACCELEROMETER_LIMITED_AXES:
        case SensorType::GYROSCOPE_LIMITED_AXES:
            dst->limited_axes_imu.x = src.payload.get<Event::EventPayload::limitedAxesImu>().x;
            dst->limited_axes_imu.y = src.payload.get<Event::EventPayload::limitedAxesImu>().y;
            dst->limited_axes_imu.z = src.payload.get<Event::EventPayload::limitedAxesImu>().z;
            dst->limited_axes_imu.x_supported =
                    src.payload.get<Event::EventPayload::limitedAxesImu>().xSupported;
            dst->limited_axes_imu.y_supported =
                    src.payload.get<Event::EventPayload::limitedAxesImu>().ySupported;
            dst->limited_axes_imu.z_supported =
                    src.payload.get<Event::EventPayload::limitedAxesImu>().zSupported;
            break;

        case SensorType::ACCELEROMETER_LIMITED_AXES_UNCALIBRATED:
        case SensorType::GYROSCOPE_LIMITED_AXES_UNCALIBRATED:
            dst->limited_axes_imu_uncalibrated.x_uncalib =
                    src.payload.get<Event::EventPayload::limitedAxesImuUncal>().x;
            dst->limited_axes_imu_uncalibrated.y_uncalib =
                    src.payload.get<Event::EventPayload::limitedAxesImuUncal>().y;
            dst->limited_axes_imu_uncalibrated.z_uncalib =
                    src.payload.get<Event::EventPayload::limitedAxesImuUncal>().z;
            dst->limited_axes_imu_uncalibrated.x_bias =
                    src.payload.get<Event::EventPayload::limitedAxesImuUncal>().xBias;
            dst->limited_axes_imu_uncalibrated.y_bias =
                    src.payload.get<Event::EventPayload::limitedAxesImuUncal>().yBias;
            dst->limited_axes_imu_uncalibrated.z_bias =
                    src.payload.get<Event::EventPayload::limitedAxesImuUncal>().zBias;
            dst->limited_axes_imu_uncalibrated.x_supported =
                    src.payload.get<Event::EventPayload::limitedAxesImuUncal>().xSupported;
            dst->limited_axes_imu_uncalibrated.y_supported =
                    src.payload.get<Event::EventPayload::limitedAxesImuUncal>().ySupported;
            dst->limited_axes_imu_uncalibrated.z_supported =
                    src.payload.get<Event::EventPayload::limitedAxesImuUncal>().zSupported;
            break;

        case SensorType::HEADING:
            dst->heading.heading = src.payload.get<Event::EventPayload::heading>().heading;
            dst->heading.accuracy = src.payload.get<Event::EventPayload::heading>().accuracy;
            break;

        default: {
            CHECK_GE((int32_t)src.sensorType, (int32_t)SensorType::DEVICE_PRIVATE_BASE);

            memcpy(dst->data, src.payload.get<Event::EventPayload::data>().values.data(),
                   16 * sizeof(float));
            break;
        }
    }
}

void convertFromSensorEvent(const sensors_event_t& src, Event* dst) {
    *dst = {
            .timestamp = src.timestamp,
            .sensorHandle = src.sensor,
            .sensorType = (SensorType)src.type,
    };

    switch (dst->sensorType) {
        case SensorType::META_DATA: {
            Event::EventPayload::MetaData meta;
            meta.what = (Event::EventPayload::MetaData::MetaDataEventType)src.meta_data.what;
            // Legacy HALs contain the handle reference in the meta data field.
            // Copy that over to the handle of the event. In legacy HALs this
            // field was expected to be 0.
            dst->sensorHandle = src.meta_data.sensor;
            dst->payload.set<Event::EventPayload::Tag::meta>(meta);
            break;
        }

        case SensorType::ACCELEROMETER:
        case SensorType::MAGNETIC_FIELD:
        case SensorType::ORIENTATION:
        case SensorType::GYROSCOPE:
        case SensorType::GRAVITY:
        case SensorType::LINEAR_ACCELERATION: {
            Event::EventPayload::Vec3 vec3;
            vec3.x = src.acceleration.x;
            vec3.y = src.acceleration.y;
            vec3.z = src.acceleration.z;
            vec3.status = (SensorStatus)src.acceleration.status;
            dst->payload.set<Event::EventPayload::Tag::vec3>(vec3);
            break;
        }

        case SensorType::GAME_ROTATION_VECTOR: {
            Event::EventPayload::Vec4 vec4;
            vec4.x = src.data[0];
            vec4.y = src.data[1];
            vec4.z = src.data[2];
            vec4.w = src.data[3];
            dst->payload.set<Event::EventPayload::Tag::vec4>(vec4);
            break;
        }

        case SensorType::ROTATION_VECTOR:
        case SensorType::GEOMAGNETIC_ROTATION_VECTOR: {
            Event::EventPayload::Data data;
            memcpy(data.values.data(), src.data, 5 * sizeof(float));
            dst->payload.set<Event::EventPayload::Tag::data>(data);
            break;
        }

        case SensorType::MAGNETIC_FIELD_UNCALIBRATED:
        case SensorType::GYROSCOPE_UNCALIBRATED:
        case SensorType::ACCELEROMETER_UNCALIBRATED: {
            Event::EventPayload::Uncal uncal;
            uncal.x = src.uncalibrated_gyro.x_uncalib;
            uncal.y = src.uncalibrated_gyro.y_uncalib;
            uncal.z = src.uncalibrated_gyro.z_uncalib;
            uncal.xBias = src.uncalibrated_gyro.x_bias;
            uncal.yBias = src.uncalibrated_gyro.y_bias;
            uncal.zBias = src.uncalibrated_gyro.z_bias;
            dst->payload.set<Event::EventPayload::Tag::uncal>(uncal);
            break;
        }

        case SensorType::DEVICE_ORIENTATION:
        case SensorType::LIGHT:
        case SensorType::PRESSURE:
        case SensorType::PROXIMITY:
        case SensorType::RELATIVE_HUMIDITY:
        case SensorType::AMBIENT_TEMPERATURE:
        case SensorType::SIGNIFICANT_MOTION:
        case SensorType::STEP_DETECTOR:
        case SensorType::TILT_DETECTOR:
        case SensorType::WAKE_GESTURE:
        case SensorType::GLANCE_GESTURE:
        case SensorType::PICK_UP_GESTURE:
        case SensorType::WRIST_TILT_GESTURE:
        case SensorType::STATIONARY_DETECT:
        case SensorType::MOTION_DETECT:
        case SensorType::HEART_BEAT:
        case SensorType::LOW_LATENCY_OFFBODY_DETECT:
        case SensorType::HINGE_ANGLE: {
            dst->payload.set<Event::EventPayload::Tag::scalar>((float)src.data[0]);
            break;
        }

        case SensorType::STEP_COUNTER: {
            dst->payload.set<Event::EventPayload::Tag::stepCount>(src.u64.step_counter);
            break;
        }

        case SensorType::HEART_RATE: {
            Event::EventPayload::HeartRate heartRate;
            heartRate.bpm = src.heart_rate.bpm;
            heartRate.status = (SensorStatus)src.heart_rate.status;
            dst->payload.set<Event::EventPayload::Tag::heartRate>(heartRate);
            break;
        }

        case SensorType::POSE_6DOF: {  // 15 floats
            Event::EventPayload::Pose6Dof pose6DOF;
            for (size_t i = 0; i < 15; ++i) {
                pose6DOF.values[i] = src.data[i];
            }
            dst->payload.set<Event::EventPayload::Tag::pose6DOF>(pose6DOF);
            break;
        }

        case SensorType::DYNAMIC_SENSOR_META: {
            DynamicSensorInfo dynamic;
            dynamic.connected = src.dynamic_sensor_meta.connected;
            dynamic.sensorHandle = src.dynamic_sensor_meta.handle;

            memcpy(dynamic.uuid.values.data(), src.dynamic_sensor_meta.uuid, 16);
            dst->payload.set<Event::EventPayload::Tag::dynamic>(dynamic);
            break;
        }

        case SensorType::ADDITIONAL_INFO: {
            AdditionalInfo info;
            const additional_info_event_t& srcInfo = src.additional_info;
            info.type = (AdditionalInfo::AdditionalInfoType)srcInfo.type;
            info.serial = srcInfo.serial;

            AdditionalInfo::AdditionalInfoPayload::Int32Values data;
            CHECK_EQ(data.values.size() * sizeof(int32_t), sizeof(srcInfo.data_int32));
            memcpy(data.values.data(), srcInfo.data_int32, sizeof(srcInfo.data_int32));
            info.payload.set<AdditionalInfo::AdditionalInfoPayload::Tag::dataInt32>(data);

            dst->payload.set<Event::EventPayload::Tag::additional>(info);
            break;
        }

        case SensorType::HEAD_TRACKER: {
            Event::EventPayload::HeadTracker headTracker;
            headTracker.rx = src.head_tracker.rx;
            headTracker.ry = src.head_tracker.ry;
            headTracker.rz = src.head_tracker.rz;
            headTracker.vx = src.head_tracker.vx;
            headTracker.vy = src.head_tracker.vy;
            headTracker.vz = src.head_tracker.vz;
            headTracker.discontinuityCount = src.head_tracker.discontinuity_count;

            dst->payload.set<Event::EventPayload::Tag::headTracker>(headTracker);
            break;
        }

        case SensorType::ACCELEROMETER_LIMITED_AXES:
        case SensorType::GYROSCOPE_LIMITED_AXES: {
            Event::EventPayload::LimitedAxesImu limitedAxesImu;
            limitedAxesImu.x = src.limited_axes_imu.x;
            limitedAxesImu.y = src.limited_axes_imu.y;
            limitedAxesImu.z = src.limited_axes_imu.z;
            limitedAxesImu.xSupported = src.limited_axes_imu.x_supported;
            limitedAxesImu.ySupported = src.limited_axes_imu.y_supported;
            limitedAxesImu.zSupported = src.limited_axes_imu.z_supported;
            dst->payload.set<Event::EventPayload::Tag::limitedAxesImu>(limitedAxesImu);
            break;
        }

        case SensorType::ACCELEROMETER_LIMITED_AXES_UNCALIBRATED:
        case SensorType::GYROSCOPE_LIMITED_AXES_UNCALIBRATED: {
            Event::EventPayload::LimitedAxesImuUncal limitedAxesImuUncal;
            limitedAxesImuUncal.x = src.limited_axes_imu_uncalibrated.x_uncalib;
            limitedAxesImuUncal.y = src.limited_axes_imu_uncalibrated.y_uncalib;
            limitedAxesImuUncal.z = src.limited_axes_imu_uncalibrated.z_uncalib;
            limitedAxesImuUncal.xBias = src.limited_axes_imu_uncalibrated.x_bias;
            limitedAxesImuUncal.yBias = src.limited_axes_imu_uncalibrated.y_bias;
            limitedAxesImuUncal.zBias = src.limited_axes_imu_uncalibrated.z_bias;
            limitedAxesImuUncal.xSupported = src.limited_axes_imu_uncalibrated.x_supported;
            limitedAxesImuUncal.ySupported = src.limited_axes_imu_uncalibrated.y_supported;
            limitedAxesImuUncal.zSupported = src.limited_axes_imu_uncalibrated.z_supported;
            dst->payload.set<Event::EventPayload::Tag::limitedAxesImuUncal>(limitedAxesImuUncal);
            break;
        }

        case SensorType::HEADING: {
            Event::EventPayload::Heading heading;
            heading.heading = src.heading.heading;
            heading.accuracy = src.heading.accuracy;
            dst->payload.set<Event::EventPayload::heading>(heading);
            break;
        }

        default: {
            CHECK_GE((int32_t)dst->sensorType, (int32_t)SensorType::DEVICE_PRIVATE_BASE);

            Event::EventPayload::Data data;
            memcpy(data.values.data(), src.data, 16 * sizeof(float));
            dst->payload.set<Event::EventPayload::Tag::data>(data);
            break;
        }
    }
}

void convertFromASensorEvent(const ASensorEvent& src, Event* dst) {
    convertFromSensorEvent(common::convertASensorEvent(src), dst);
}

}  // namespace implementation
}  // namespace sensors
}  // namespace hardware
}  // namespace android
