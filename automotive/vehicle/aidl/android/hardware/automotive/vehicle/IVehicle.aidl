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

import android.hardware.automotive.vehicle.IVehicleCallback;
import android.hardware.automotive.vehicle.StatusCode;
import android.hardware.automotive.vehicle.SubscribeOptions;
import android.hardware.automotive.vehicle.VehiclePropConfig;
import android.hardware.automotive.vehicle.VehiclePropValue;

// @VintfStability
interface IVehicle {
    // Adding return type to method instead of out param String s since there is only one return
    // value.
    /**
     * Print out debugging state for the vehicle hal.
     *
     * The text must be in ASCII encoding only.
     *
     * Performance requirements:
     *
     * The HAL must return from this call in less than 10ms. This call must avoid
     * deadlocks, as it may be called at any point of operation. Any synchronization
     * primitives used (such as mutex locks or semaphores) must be acquired
     * with a timeout.
     *
     */
    String debugDump();

    /**
     * Get a vehicle property value.
     *
     * For VehiclePropertyChangeMode::STATIC properties, this method must always
     * return the same value always.
     * For VehiclePropertyChangeMode::ON_CHANGE properties, it must return the
     * latest available value.
     *
     * Some properties like RADIO_PRESET requires to pass additional data in
     * GET request in VehiclePropValue object.
     *
     * If there is no data available yet, which can happen during initial stage,
     * this call must return immediately with an error code of
     * StatusCode::TRY_AGAIN.
     */
    StatusCode get(in VehiclePropValue requestedPropValue, out VehiclePropValue propValue);

    // Adding return type to method instead of out param VehiclePropConfig[] propConfigs since there
    // is only one return value.
    /**
     * Returns a list of all property configurations supported by this vehicle
     * HAL.
     */
    VehiclePropConfig[] getAllPropConfigs();

    /**
     * Returns a list of property configurations for given properties.
     *
     * If requested VehicleProperty wasn't found it must return
     * StatusCode::INVALID_ARG, otherwise a list of vehicle property
     * configurations with StatusCode::OK
     */
    StatusCode getPropConfigs(in int[] props, out VehiclePropConfig[] propConfigs);

    // Adding return type to method instead of out param StatusCode status since there is only one
    // return value.
    /**
     * Set a vehicle property value.
     *
     * Timestamp of data must be ignored for set operation.
     *
     * Setting some properties require having initial state available. If initial
     * data is not available yet this call must return StatusCode::TRY_AGAIN.
     * For a property with separate power control this call must return
     * StatusCode::NOT_AVAILABLE error if property is not powered on.
     */
    StatusCode set(in VehiclePropValue propValue);

    // Adding return type to method instead of out param StatusCode status since there is only one
    // return value.
    /**
     * Subscribes to property events.
     *
     * Clients must be able to subscribe to multiple properties at a time
     * depending on data provided in options argument.
     *
     * @param listener This client must be called on appropriate event.
     * @param options List of options to subscribe. SubscribeOption contains
     *                information such as property Id, area Id, sample rate, etc.
     */
    StatusCode subscribe(in IVehicleCallback callback, in SubscribeOptions[] options);

    // Adding return type to method instead of out param StatusCode status since there is only one
    // return value.
    /**
     * Unsubscribes from property events.
     *
     * If this client wasn't subscribed to the given property, this method
     * must return StatusCode::INVALID_ARG.
     */
    StatusCode unsubscribe(in IVehicleCallback callback, in int propId);
}
