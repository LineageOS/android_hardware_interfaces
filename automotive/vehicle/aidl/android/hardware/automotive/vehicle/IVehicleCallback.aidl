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

import android.hardware.automotive.vehicle.GetValueResults;
import android.hardware.automotive.vehicle.SetValueResults;
import android.hardware.automotive.vehicle.StatusCode;
import android.hardware.automotive.vehicle.VehiclePropErrors;
import android.hardware.automotive.vehicle.VehiclePropValues;

@VintfStability
interface IVehicleCallback {
    /**
     * Callback for {@link IVehicle#getValues} function.
     *
     * Called when some of the values to fetch are ready. This might be called
     * once or multiple times for one 'getValues' request. Each callback
     * contains part of the requested values. It is guaranteed that all the
     * requested values would be returned in one of the callbacks, but the order
     * each values are ready is not guaranteed.
     *
     * @param responses An object either contains a list of
     *    {@link GetValueResult} if they fits the binder memory limitation or a
     *    shared memory file that contains responses. Each
     *    {@link GetValueResult} either contains the property value or contains
     *    an error happened while getting the value.
     *
     *    {@link GetValueResult} also contains a requestId which indicates which
     *    request this response is for. The responses object should be parsed by
     *    {@code android-automotive-large-parcelable} library.
     */
    oneway void onGetValues(in GetValueResults responses);

    /**
     * Callback for {@link IVehicle#setValues} function.
     *
     * Called when VHAL have finished handling some of the property set request.
     * This might be called once or multiple times for one 'setValues' requests.
     * Each callback contains part of the requested values. It is guaranteed
     * that all the set value statuses would be returned in one of the
     * callbacks, but the order each values are set is not guaranteed.
     *
     * @param responses A list of {@link SetValueResult}. Each SetValueResult
     *    contains a status indicating the status for setting the specific
     *    property. The requestId indicates which request the response is for.
     */
    oneway void onSetValues(in SetValueResults responses);

    /**
     * Event callback happens whenever one or more variables that the API user
     * has subscribed to need to be reported. This may be based purely on
     * threshold and frequency (a regular subscription, see subscribe call's
     * arguments) or when the {@link IVehicle#setValues} method was called and
     * the actual change needs to be reported.
     *
     * @param propValues The updated property values wrapped in an object.
     *    If the properties fit within binder limitation, they would be in
     *    {@code propValues.payloads}, otherwise, they would be in a shared
     *    memory file {@code propValues.sharedMemoryFd}.
     *    The shared memory file is created by VHAL and must be returned to
     *    VHAL using {@link IVehicle#returnSharedMemory} after use. There are
     *    limited number of memory files created for each subscription, if
     *    the client doesn't return the shared memory, the client might not get
     *    event in the future.
     * @param sharedMemoryFileCount Number of shared memory file allocated for
     *    this subscription. This value could be used to tweak
     *    {@code maxSharedMemoryFileCount} in {@link IVehicle#subscribe}. For
     *    example, if you usually see sharedMemoryFileCount being the
     *    maxSharedMemoryFileCount you set, this means you might need to
     *    increase maxSharedMemoryFileCount.
     */
    oneway void onPropertyEvent(in VehiclePropValues propValues, int sharedMemoryFileCount);

    /**
     * Set property value is usually asynchronous operation. Thus even if
     * client received {@link StatusCode#OK} from {@link IVehicle#setValues}, or
     * received {@link StatusCode#OK} in {@link #onSetValues}, this doesn't
     * guarantee that the value was successfully propagated to the vehicle
     * network. If such rare event occurs this method must be called.
     *
     * @param errors A list of property set errors. If the VHAL implementation
     *     does not batch the errors, this may only contain one error.
     */
    oneway void onPropertySetError(in VehiclePropErrors errors);
}
