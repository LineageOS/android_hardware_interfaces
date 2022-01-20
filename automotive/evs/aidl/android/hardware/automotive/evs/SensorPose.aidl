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

import android.hardware.automotive.evs.RotationQuaternion;
import android.hardware.automotive.evs.Translation;

/**
 * Provides the orientation and location of a car sensor relative to the android automotive
 * coordinate system:
 * https://source.android.com/devices/sensors/sensor-types#auto_axes
 * The sensor pose defines the transformation to be applied to the android automotive axes to
 * obtain the sensor local axes.
 * The pose consists of rotation, (specified as a quaternions) and translation
 * (vector with x, y, z).
 * This rotation and translation applied to the sensor data in the sensor's local coordinate
 * system transform the data to the automotive coordinate system.
 * i.e.  loc =  ( Rot * Psensor ) + Trans
 * Here loc is a point in automotive coordinate system and Psensor is a point in the sensor's
 * coordinate system.
 * Example:
 * For a sensor on the front bumper and on the left corner of the car with its X axis pointing to
 * the front, the sensor is located at (-2, 4, 0) meters w.r.t android automotive axes and the
 * sensor local axes has a rotation of 90 degrees counter-clockwise w.r.t android automotive axes
 * when viewing the car from top on the +Z axis side:
 *
 *      ↑X sensor
 *    Y←∘______
 *      |      |  front
 *      | car  |
 *      |  ↑Y  |
 *      |  ∘→X |  rear
 *      |______|
 *
 * For this example the rotation and translation will be:
 * Rotation = + 90 degrees around Z axis = (0.7071, 0, 0, 0.7071) as a unit quaternion.
 * Translation = (-2, 4, 0) in meters = (-2000, 4000, 0) in milli-meters.
 * Note: Every sensor type must specify its own pose.
 */
@VintfStability
parcelable SensorPose {
    /**
     * Rotation part of the sensor pose, expressed as a unit quaternion.
     */
    RotationQuaternion rotation;
    /**
     * Translation part of the sensor pose, in (x, y, z) format with milli-meter units.
     */
    Translation translation;
}
