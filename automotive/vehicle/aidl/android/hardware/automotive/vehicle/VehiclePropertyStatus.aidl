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

package android.hardware.automotive.vehicle;

/**
 * Property status is a dynamic value that may change based on the vehicle state.
 */
@VintfStability
@Backing(type="int")
enum VehiclePropertyStatus {
    /**
     * Property is available and behaving normally
     */
    AVAILABLE = 0x00,
    /**
     * Same as {@link #NOT_AVAILABLE_GENERAL}.
     */
    UNAVAILABLE = 0x01,
    /**
     * A property in this state is not available for reading and writing.  This
     * is a transient state that depends on the availability of the underlying
     * implementation (e.g. hardware or driver). It MUST NOT be used to
     * represent features that this vehicle is always incapable of.  A get() of
     * a property in this state MAY return an undefined value, but MUST
     * correctly describe its status as UNAVAILABLE A set() of a property in
     * this state MAY return NOT_AVAILABLE. The HAL implementation MUST ignore
     * the value of the status field when writing a property value coming from
     * Android.
     *
     * This represents a general not-available status. If more detailed info is
     * known, a more specific not-available status should be used instead.
     */
    NOT_AVAILABLE_GENERAL = 0x01,
    /**
     * There is an error with this property.
     */
    ERROR = 0x02,
    // All NOT_AVAILABLE_XXX status starts with 0x1000.
    /**
     * The property is not available because the underlying feature is disabled.
     */
    NOT_AVAILABLE_DISABLED = 0x1000 | 0x01,
    /**
     * The property is not available because the vehicle speed is too low.
     */
    NOT_AVAILABLE_SPEED_LOW = 0x1000 | 0x02,
    /**
     * The property is not available because the vehicle speed is too high.
     */
    NOT_AVAILABLE_SPEED_HIGH = 0x1000 | 0x03,
    /**
     * The property is not available because of bad camera or sensor visibility. Examples
     * might be bird poop blocking the camera or a bumper cover blocking an ultrasonic sensor.
     */
    NOT_AVAILABLE_POOR_VISIBILITY = 0x1000 | 0x04,
    /**
     * The property cannot be accessed due to safety reasons. Eg. System could be
     * in a faulty state, an object or person could be blocking the requested
     * operation such as closing a trunk door, etc.
     */
    NOT_AVAILABLE_SAFETY = 0x1000 | 0x05,
    /**
     * The property is not available because the sub-system for the feature is
     * not connected.
     *
     * E.g. the trailer light property is in this state if the trailer is not
     * attached.
     */
    NOT_AVAILABLE_SUBSYSTEM_NOT_CONNECTED = 0x1000 | 0x06,
}
