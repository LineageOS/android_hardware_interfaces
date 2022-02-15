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

package android.hardware.automotive.evs;

import android.hardware.automotive.evs.UltrasonicSensor;

/**
 * Structure identifies and describes an ultrasonics array in the car.
 *
 * A ultrasonics array represents a group of ultrasonic sensors within the
 * car. These may be sensors that are physically connected to the same hardware
 * control unit or represent a logical group of sensors like front and back.
 * The HAL is responsible for filling out this structure for each Ultrasonics
 * Array.
 */
@VintfStability
parcelable UltrasonicsArrayDesc {
    /**
     * Unique identifier for the ultrasonic array. This may be a path or name of the
     * physical control device or a string identifying a logical group of sensors forming an array
     * such as "front_array" and "back_array".
     */
    @utf8InCpp
    String ultrasonicsArrayId;
    /**
     * Maximum number of readings (points on waveform) provided per sensor in
     * each data frame. Used by client to pre-allocate required memory buffer for
     * incoming data.
     */
    int maxReadingsPerSensorCount;
    /**
     * Maximum number of receiver sensors in a data frame. Must be between 1
     * and sensorCount. Used by client to pre-allocate required memory buffer for
     * incoming data.
     */
    int maxReceiversCount;
    /**
     * The order of sensors specified must be in clockwise order around the car, starting
     * from front left-most sensor.
     */
    UltrasonicSensor[] sensors;
}
