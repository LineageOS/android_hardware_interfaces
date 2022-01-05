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

package android.hardware.sensors;

import android.hardware.sensors.AdditionalInfo;
import android.hardware.sensors.DynamicSensorInfo;
import android.hardware.sensors.SensorStatus;
import android.hardware.sensors.SensorType;

@VintfStability
@FixedSize
parcelable Event {
    /**
     * Time measured in nanoseconds, in "elapsedRealtimeNano()'s" timebase.
     */
    long timestamp;

    /**
     * sensor identifier
     */
    int sensorHandle;

    SensorType sensorType;

    /**
     * Union discriminated on sensorType
     */
    EventPayload payload;

    /*
     * acceleration values are in meter per second per second (m/s^2)
     * magnetic vector values are in micro-Tesla (uT)
     * orientation values are in degrees
     * gyroscope values are in rad/s
     * temperature is in degrees centigrade (Celsius)
     * distance in centimeters
     * light in SI lux units
     * pressure in hectopascal (hPa)
     * relative humidity in percent
     */
    @VintfStability
    @FixedSize
    union EventPayload {
        /**
         * SensorType::ACCELEROMETER, SensorType::MAGNETIC_FIELD,
         * SensorType::ORIENTATION, SensorType::GYROSCOPE, SensorType::GRAVITY,
         * SensorType::LINEAR_ACCELERATION
         */
        Vec3 vec3;

        /**
         * SensorType::GAME_ROTATION_VECTOR
         */
        Vec4 vec4;

        /**
         * SensorType::MAGNETIC_FIELD_UNCALIBRATED,
         * SensorType::GYROSCOPE_UNCALIBRATED
         * SensorType::ACCELEROMETER_UNCALIBRATED
         */
        Uncal uncal;

        /**
         * SensorType::META_DATA
         */
        MetaData meta;

        /**
         * SensorType::DEVICE_ORIENTATION, SensorType::LIGHT, SensorType::PRESSURE,
         * SensorType::TEMPERATURE, SensorType::PROXIMITY,
         * SensorType::RELATIVE_HUMIDITY, SensorType::AMBIENT_TEMPERATURE,
         * SensorType::SIGNIFICANT_MOTION, SensorType::STEP_DETECTOR,
         * SensorType::TILT_DETECTOR, SensorType::WAKE_GESTURE,
         * SensorType::GLANCE_GESTURE, SensorType::PICK_UP_GESTURE,
         * SensorType::WRIST_TILT_GESTURE, SensorType::STATIONARY_DETECT,
         * SensorType::MOTION_DETECT, SensorType::HEART_BEAT,
         * SensorType::LOW_LATENCY_OFFBODY_DETECT
         */
        float scalar;

        /**
         * SensorType::STEP_COUNTER
         */
        long stepCount;

        /**
         * SensorType::HEART_RATE
         */
        HeartRate heartRate;

        /**
         * SensorType::POSE_6DOF
         */
        Pose6Dof pose6DOF;

        /**
         * SensorType::DYNAMIC_SENSOR_META
         */
        DynamicSensorInfo dynamic;

        /**
         * SensorType::ADDITIONAL_INFO
         */
        AdditionalInfo additional;

        /**
         * The following sensors should use the data field:
         * - Undefined/custom sensor type >= SensorType::DEVICE_PRIVATE_BASE
         * - SensorType::ROTATION_VECTOR, SensorType::GEOMAGNETIC_ROTATION_VECTOR:
         *   - These are Vec4 types with an additional float accuracy field,
         *     where data[4] is the estimated heading accuracy in radians
         *     (-1 if unavailable, and invalid if not in the range (0, 2 * pi]).
         */
        Data data;

        @FixedSize
        @VintfStability
        parcelable Vec4 {
            float x;
            float y;
            float z;
            float w;
        }

        @FixedSize
        @VintfStability
        parcelable Vec3 {
            float x;
            float y;
            float z;
            SensorStatus status;
        }

        @FixedSize
        @VintfStability
        parcelable Uncal {
            float x;
            float y;
            float z;
            float xBias;
            float yBias;
            float zBias;
        }

        @FixedSize
        @VintfStability
        parcelable HeartRate {
            /**
             * Heart rate in beats per minute.
             * Set to 0 when status is SensorStatus::UNRELIABLE or
             * SensorStatus::NO_CONTACT
             */
            float bpm;
            /**
             * Status of the heart rate sensor for this reading.
             */
            SensorStatus status;
        }

        @FixedSize
        @VintfStability
        parcelable MetaData {
            MetaDataEventType what;

            @VintfStability
            @Backing(type="int")
            enum MetaDataEventType {
                META_DATA_FLUSH_COMPLETE = 1,
            }
        }

        @FixedSize
        @VintfStability
        parcelable Pose6Dof {
            float[15] values;
        }

        @FixedSize
        @VintfStability
        parcelable Data {
            float[16] values;
        }
    }
}
