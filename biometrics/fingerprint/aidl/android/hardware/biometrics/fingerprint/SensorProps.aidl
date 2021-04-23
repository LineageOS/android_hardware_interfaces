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

package android.hardware.biometrics.fingerprint;

import android.hardware.biometrics.common.CommonProps;
import android.hardware.biometrics.fingerprint.FingerprintSensorType;
import android.hardware.biometrics.fingerprint.SensorLocation;

@VintfStability
parcelable SensorProps {
    /**
     * Statically configured properties that apply to this fingerprint sensor.
     */
    CommonProps commonProps;

    /**
     * A statically configured sensor type representing this fingerprint sensor.
     */
    FingerprintSensorType sensorType = FingerprintSensorType.UNKNOWN;

    /**
     * A list of display-specific locations from where the sensor is usable from. See SensorLocation
     * for more details.
     */
    SensorLocation[] sensorLocations;

    /**
     * Must be set to true for sensors that support "swipe" gestures via
     * android.view.KeyEvent#KEYCODE_SYSTEM_NAVIGATION_*.
     */
    boolean supportsNavigationGestures;

    /**
     * Specifies whether or not the implementation supports ISession#detectInteraction.
     */
    boolean supportsDetectInteraction;

    /**
     * Whether the HAL is responsible for detecting and processing of display touches. This is only
     * applicable to under-display fingerprint sensors (UDFPS). If the value is false, the framework
     * will be responsible for handling the display touch events and passing them down to the HAL by
     * using ISession#onPointerDown and ISession#onPointerUp. If the value is true, the framework
     * will not notify the HAL about touch events.
     *
     * This value must be ignored for non-UDFPS sensors.
     */
    boolean halHandlesDisplayTouches;

    /**
     * Whether the HAL is responsible for fingerprint illumination, for example through enabling the
     * display's high-brightness mode. This is only applicable to optical under-display fingerprint
     * sensors (optical UDFPS). If the value is false, the framework will be responsible for
     * illuminating the finger and reporting ISession#onUiReady. If the value is true, the framework
     * will not illuminate the finger and will not report ISession#onUiReady.
     *
     * This value must be ignored for sensors that aren't optical UDFPS.
     */
    boolean halControlsIllumination;
}
