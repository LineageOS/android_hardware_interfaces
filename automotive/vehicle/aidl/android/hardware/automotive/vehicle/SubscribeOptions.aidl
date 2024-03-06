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
 * Encapsulates information about subscription to vehicle property events.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable SubscribeOptions {
    /** Property to subscribe */
    int propId;
    /**
     * Optional areas to subscribe for this property, if empty, would subscribe
     * to all areas configured for this property.
     */
    int[] areaIds;
    /**
     * Sample rate in Hz.
     *
     * Must be provided for properties with
     * VehiclePropertyChangeMode::CONTINUOUS. The value must be within
     * VehiclePropConfig#minSamplingRate .. VehiclePropConfig#maxSamplingRate
     * for a given property.
     * This value indicates how many updates per second client wants to receive.
     */
    float sampleRate;

    /**
     * Requested resolution of property updates.
     *
     * This value indicates the resolution at which continuous property updates should be sent to
     * the platform. For example, if resolution is 0.01, the subscribed property value should be
     * rounded to two decimal places. If the incoming resolution value is not an integer multiple of
     * 10, VHAL should return a StatusCode::INVALID_ARG.
     */
    float resolution = 0.0f;

    /**
     * Whether to enable variable update rate.
     *
     * This only applies for continuous property. If variable update rate is
     * enabled, for each given areaId, if VHAL supports variable update rate for
     * the [propId, areaId], VHAL must ignore duplicate property value events
     * and only sends changed value events (a.k.a treat continuous as an
     * on-change property).
     *
     * If VHAL does not support variable update rate for the [propId, areaId],
     * indicated by 'supportVariableUpdateRate' in 'VehicleAreaConfig', or if
     * this property is not a continuous property, this option must be ignored.
     */
    boolean enableVariableUpdateRate;
}
