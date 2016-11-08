/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "convert.h"

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace sensors {
namespace V1_0 {
namespace implementation {

void convertFromSensor(const sensor_t &src, SensorInfo *dst) {
    dst->name = src.name;
    dst->vendor = src.vendor;
    dst->version = src.version;
    dst->sensorHandle = src.handle;
    dst->type = (SensorType)src.type;
    dst->maxRange = src.maxRange;
    dst->resolution = src.resolution;
    dst->power = src.power;
    dst->minDelay = src.minDelay;
    dst->fifoReservedEventCount = src.fifoReservedEventCount;
    dst->fifoMaxEventCount = src.fifoMaxEventCount;
    dst->typeAsString = src.stringType;
    dst->requiredPermission = src.requiredPermission;
    dst->maxDelay = src.maxDelay;
    dst->flags = src.flags;
}

void convertToSensor(
        const ::android::hardware::sensors::V1_0::SensorInfo &src,
        sensor_t *dst) {
    dst->name = strdup(src.name.c_str());
    dst->vendor = strdup(src.vendor.c_str());
    dst->version = src.version;
    dst->handle = src.sensorHandle;
    dst->type = (int)src.type;
    dst->maxRange = src.maxRange;
    dst->resolution = src.resolution;
    dst->power = src.power;
    dst->minDelay = src.minDelay;
    dst->fifoReservedEventCount = src.fifoReservedEventCount;
    dst->fifoMaxEventCount = src.fifoMaxEventCount;
    dst->stringType = strdup(src.typeAsString.c_str());
    dst->requiredPermission = strdup(src.requiredPermission.c_str());
    dst->maxDelay = src.maxDelay;
    dst->flags = src.flags;
    dst->reserved[0] = dst->reserved[1] = 0;
}

void convertFromSensorEvent(const sensors_event_t &src, Event *dst) {
    typedef ::android::hardware::sensors::V1_0::SensorType SensorType;
    typedef ::android::hardware::sensors::V1_0::MetaDataEventType MetaDataEventType;

    dst->sensorHandle = src.sensor;
    dst->sensorType = (SensorType)src.type;
    dst->timestamp = src.timestamp;

    switch (dst->sensorType) {
        case SensorType::SENSOR_TYPE_META_DATA:
        {
            dst->u.meta.what = (MetaDataEventType)src.meta_data.what;
            break;
        }

        case SensorType::SENSOR_TYPE_ACCELEROMETER:
        case SensorType::SENSOR_TYPE_GEOMAGNETIC_FIELD:
        case SensorType::SENSOR_TYPE_ORIENTATION:
        case SensorType::SENSOR_TYPE_GYROSCOPE:
        case SensorType::SENSOR_TYPE_GRAVITY:
        case SensorType::SENSOR_TYPE_LINEAR_ACCELERATION:
        {
            dst->u.vec3.x = src.acceleration.x;
            dst->u.vec3.y = src.acceleration.y;
            dst->u.vec3.z = src.acceleration.z;
            dst->u.vec3.status = (SensorStatus)src.acceleration.status;
            break;
        }

        case SensorType::SENSOR_TYPE_ROTATION_VECTOR:
        case SensorType::SENSOR_TYPE_GAME_ROTATION_VECTOR:
        case SensorType::SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR:
        {
            dst->u.vec4.x = src.data[0];
            dst->u.vec4.y = src.data[1];
            dst->u.vec4.z = src.data[2];
            dst->u.vec4.w = src.data[3];
            break;
        }

        case SensorType::SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
        case SensorType::SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
        {
            dst->u.uncal.x = src.uncalibrated_gyro.x_uncalib;
            dst->u.uncal.y = src.uncalibrated_gyro.y_uncalib;
            dst->u.uncal.z = src.uncalibrated_gyro.z_uncalib;
            dst->u.uncal.x_bias = src.uncalibrated_gyro.x_bias;
            dst->u.uncal.y_bias = src.uncalibrated_gyro.y_bias;
            dst->u.uncal.z_bias = src.uncalibrated_gyro.z_bias;
            break;
        }

        case SensorType::SENSOR_TYPE_DEVICE_ORIENTATION:
        case SensorType::SENSOR_TYPE_LIGHT:
        case SensorType::SENSOR_TYPE_PRESSURE:
        case SensorType::SENSOR_TYPE_TEMPERATURE:
        case SensorType::SENSOR_TYPE_PROXIMITY:
        case SensorType::SENSOR_TYPE_RELATIVE_HUMIDITY:
        case SensorType::SENSOR_TYPE_AMBIENT_TEMPERATURE:
        case SensorType::SENSOR_TYPE_SIGNIFICANT_MOTION:
        case SensorType::SENSOR_TYPE_STEP_DETECTOR:
        case SensorType::SENSOR_TYPE_TILT_DETECTOR:
        case SensorType::SENSOR_TYPE_WAKE_GESTURE:
        case SensorType::SENSOR_TYPE_GLANCE_GESTURE:
        case SensorType::SENSOR_TYPE_PICK_UP_GESTURE:
        case SensorType::SENSOR_TYPE_WRIST_TILT_GESTURE:
        case SensorType::SENSOR_TYPE_STATIONARY_DETECT:
        case SensorType::SENSOR_TYPE_MOTION_DETECT:
        case SensorType::SENSOR_TYPE_HEART_BEAT:
        {
            dst->u.scalar = src.data[0];
            break;
        }

        case SensorType::SENSOR_TYPE_STEP_COUNTER:
        {
            dst->u.stepCount = src.u64.step_counter;
            break;
        }

        case SensorType::SENSOR_TYPE_HEART_RATE:
        {
            dst->u.heartRate.bpm = src.heart_rate.bpm;
            dst->u.heartRate.status = (SensorStatus)src.heart_rate.status;
            break;
        }

        case SensorType::SENSOR_TYPE_POSE_6DOF:  // 15 floats
        {
            for (size_t i = 0; i < 15; ++i) {
                dst->u.pose6DOF[i] = src.data[i];
            }
            break;
        }

        case SensorType::SENSOR_TYPE_DYNAMIC_SENSOR_META:
        {
            dst->u.dynamic.connected = src.dynamic_sensor_meta.connected;
            dst->u.dynamic.sensorHandle = src.dynamic_sensor_meta.handle;

            memcpy(dst->u.dynamic.uuid.data(),
                   src.dynamic_sensor_meta.uuid,
                   16);

            break;
        }

        case SensorType::SENSOR_TYPE_ADDITIONAL_INFO:
        {
            ::android::hardware::sensors::V1_0::AdditionalInfo *dstInfo =
                &dst->u.additional;

            const additional_info_event_t &srcInfo = src.additional_info;

            dstInfo->type =
                (::android::hardware::sensors::V1_0::AdditionalInfoType)
                    srcInfo.type;

            dstInfo->serial = srcInfo.serial;

            CHECK_EQ(sizeof(dstInfo->u), sizeof(srcInfo.data_int32));
            memcpy(&dstInfo->u, srcInfo.data_int32, sizeof(srcInfo.data_int32));
            break;
        }

        default:
        {
            CHECK_GE((int32_t)dst->sensorType,
                     (int32_t)SensorType::SENSOR_TYPE_DEVICE_PRIVATE_BASE);

            memcpy(dst->u.data.data(), src.data, 16 * sizeof(float));
            break;
        }
    }
}

void convertToSensorEvent(const Event &src, sensors_event_t *dst) {
    dst->version = sizeof(sensors_event_t);
    dst->sensor = src.sensorHandle;
    dst->type = (int32_t)src.sensorType;
    dst->reserved0 = 0;
    dst->timestamp = src.timestamp;
    dst->flags = 0;
    dst->reserved1[0] = dst->reserved1[1] = dst->reserved1[2] = 0;

    switch (src.sensorType) {
        case SensorType::SENSOR_TYPE_META_DATA:
        {
            dst->meta_data.what = (int32_t)src.u.meta.what;
            dst->meta_data.sensor = dst->sensor;
            break;
        }

        case SensorType::SENSOR_TYPE_ACCELEROMETER:
        case SensorType::SENSOR_TYPE_GEOMAGNETIC_FIELD:
        case SensorType::SENSOR_TYPE_ORIENTATION:
        case SensorType::SENSOR_TYPE_GYROSCOPE:
        case SensorType::SENSOR_TYPE_GRAVITY:
        case SensorType::SENSOR_TYPE_LINEAR_ACCELERATION:
        {
            dst->acceleration.x = src.u.vec3.x;
            dst->acceleration.y = src.u.vec3.y;
            dst->acceleration.z = src.u.vec3.z;
            dst->acceleration.status = (int8_t)src.u.vec3.status;
            break;
        }

        case SensorType::SENSOR_TYPE_ROTATION_VECTOR:
        case SensorType::SENSOR_TYPE_GAME_ROTATION_VECTOR:
        case SensorType::SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR:
        {
            dst->data[0] = src.u.vec4.x;
            dst->data[1] = src.u.vec4.y;
            dst->data[2] = src.u.vec4.z;
            dst->data[3] = src.u.vec4.w;
            break;
        }

        case SensorType::SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
        case SensorType::SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
        {
            dst->uncalibrated_gyro.x_uncalib = src.u.uncal.x;
            dst->uncalibrated_gyro.y_uncalib = src.u.uncal.y;
            dst->uncalibrated_gyro.z_uncalib = src.u.uncal.z;
            dst->uncalibrated_gyro.x_bias = src.u.uncal.x_bias;
            dst->uncalibrated_gyro.y_bias = src.u.uncal.y_bias;
            dst->uncalibrated_gyro.z_bias = src.u.uncal.z_bias;
            break;
        }

        case SensorType::SENSOR_TYPE_DEVICE_ORIENTATION:
        case SensorType::SENSOR_TYPE_LIGHT:
        case SensorType::SENSOR_TYPE_PRESSURE:
        case SensorType::SENSOR_TYPE_TEMPERATURE:
        case SensorType::SENSOR_TYPE_PROXIMITY:
        case SensorType::SENSOR_TYPE_RELATIVE_HUMIDITY:
        case SensorType::SENSOR_TYPE_AMBIENT_TEMPERATURE:
        case SensorType::SENSOR_TYPE_SIGNIFICANT_MOTION:
        case SensorType::SENSOR_TYPE_STEP_DETECTOR:
        case SensorType::SENSOR_TYPE_TILT_DETECTOR:
        case SensorType::SENSOR_TYPE_WAKE_GESTURE:
        case SensorType::SENSOR_TYPE_GLANCE_GESTURE:
        case SensorType::SENSOR_TYPE_PICK_UP_GESTURE:
        case SensorType::SENSOR_TYPE_WRIST_TILT_GESTURE:
        case SensorType::SENSOR_TYPE_STATIONARY_DETECT:
        case SensorType::SENSOR_TYPE_MOTION_DETECT:
        case SensorType::SENSOR_TYPE_HEART_BEAT:
        {
            dst->data[0] = src.u.scalar;
            break;
        }

        case SensorType::SENSOR_TYPE_STEP_COUNTER:
        {
            dst->u64.step_counter = src.u.stepCount;
            break;
        }

        case SensorType::SENSOR_TYPE_HEART_RATE:
        {
            dst->heart_rate.bpm = src.u.heartRate.bpm;
            dst->heart_rate.status = (int8_t)src.u.heartRate.status;
            break;
        }

        case SensorType::SENSOR_TYPE_POSE_6DOF:  // 15 floats
        {
            for (size_t i = 0; i < 15; ++i) {
                dst->data[i] = src.u.pose6DOF[i];
            }
            break;
        }

        case SensorType::SENSOR_TYPE_DYNAMIC_SENSOR_META:
        {
            dst->dynamic_sensor_meta.connected = src.u.dynamic.connected;
            dst->dynamic_sensor_meta.handle = src.u.dynamic.sensorHandle;
            dst->dynamic_sensor_meta.sensor = NULL;  // to be filled in later

            memcpy(dst->dynamic_sensor_meta.uuid,
                   src.u.dynamic.uuid.data(),
                   16);

            break;
        }

        case SensorType::SENSOR_TYPE_ADDITIONAL_INFO:
        {
            const ::android::hardware::sensors::V1_0::AdditionalInfo &srcInfo =
                src.u.additional;

            additional_info_event_t *dstInfo = &dst->additional_info;
            dstInfo->type = (int32_t)srcInfo.type;
            dstInfo->serial = srcInfo.serial;

            CHECK_EQ(sizeof(srcInfo.u), sizeof(dstInfo->data_int32));

            memcpy(dstInfo->data_int32,
                   &srcInfo.u,
                   sizeof(dstInfo->data_int32));

            break;
        }

        default:
        {
            CHECK_GE((int32_t)src.sensorType,
                     (int32_t)SensorType::SENSOR_TYPE_DEVICE_PRIVATE_BASE);

            memcpy(dst->data, src.u.data.data(), 16 * sizeof(float));
            break;
        }
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android

