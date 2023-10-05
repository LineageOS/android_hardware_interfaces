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

import android.hardware.automotive.vehicle.GetValueRequests;
import android.hardware.automotive.vehicle.IVehicleCallback;
import android.hardware.automotive.vehicle.SetValueRequests;
import android.hardware.automotive.vehicle.SubscribeOptions;
import android.hardware.automotive.vehicle.VehiclePropConfigs;

// Vehicle HAL interface.
@VintfStability
interface IVehicle {
    /* An invalid memory ID. */
    const long INVALID_MEMORY_ID = 0;
    /* Maximum number of shared memory files for every subscription client. */
    const int MAX_SHARED_MEMORY_FILES_PER_CLIENT = 3;

    /**
     * Returns a list of all property configurations supported by this vehicle
     * HAL.
     *
     * @return A parcelable object that either contains a list of configs if
     *    they fit the binder memory limitation or a shared memory file that
     *    contains the configs. Must be parsed using
     *    {@code android-automotive-large-parcelable} library.
     */
    VehiclePropConfigs getAllPropConfigs();

    /**
     * Returns a list of property configurations for given properties.
     *
     * If one of the requested VehicleProperty wasn't found it must return
     * {@link StatusCode#INVALID_ARG}, otherwise a list of vehicle property
     * configurations with {@link StatusCode#OK}.
     *
     * @param props A list of property IDs to get configurations for.
     * @return A parcelable object that either contains a list of configs if
     *    they fit the binder memory limitation or a shared memory file that
     *    contains the configs. Must be parsed using
     *    {@code android-automotive-large-parcelable} library.
     */
    VehiclePropConfigs getPropConfigs(in int[] props);

    /**
     * Get vehicle property values asynchronously.
     *
     * The {@link IVehicleCallback#onGetValues} method will be called when
     * values are fetched. The method might be called multiple times, and each
     * time with a subset of properties that have been fetched. E.g., if you
     * request properties [A, B, C], the callback might be called twice with
     * [A, C] and with [B]. Caller should not expect any order for
     * {@link IVehicleCallback#onGetValues}.
     *
     * If this method returns error, it means we fail to get all the properties.
     * If this method returns OK, there are still chances we fail to get some
     * properties, which are indicated by {@link GetValueResult#status}.
     *
     * For {@link VehiclePropertyChangeMode#STATIC} properties, this method must
     * always return the same value.
     * For {@link VehiclePropertyChangeMode#ON_CHANGE} properties, it must
     * return the latest available value. For cachable properties, the value
     * within cache would be returned without talking with the actual car bus.
     *
     * Some properties like {@code RADIO_PRESET} requires to pass additional
     * data in {@link VehiclePropValue} object.
     *
     * If there is no data available yet, which can happen during initial stage,
     * {@link GetValueResult#status} contains {@link StatusCode#TRY_AGAIN}.
     *
     * Caller must pass a unique RequestID for each request, if any of the
     * given request ID is duplicate with one of the pending request ID, this
     * function must return {@link StatusCode#INVALID_ARG}.
     *
     * To prevent confusion, duplicate properties (same property ID and same
     * area ID) are not allowed in a single call. This function must return
     * {@link StatusCode#INVALID_ARG} for duplicate properties.
     *
     * The {@link VehiclePropValue#timestamp} field in request is ignored. The
     * {@link VehiclePropValue#timestamp} field in {@link GetValueResult} must
     * be the system uptime since boot when the value changes for
     * ON_CHANGE property or when the value is checked according to polling rate
     * for CONTINUOUS property. Note that for CONTINUOUS property, VHAL client
     * reading the property multiple times between the polling interval will get
     * the same timestamp.
     *
     * @param callback A callback interface, whose 'onGetValues' would be called
     *    after the value is fetched. Caller should use
     *    {@code android-automotive-large-parcelable} library to parse the
     *    returned {@link GetValueResult} object.
     * @param requests An object that contains either a list of requested
     *    properties or a shared memory file that contains the properties. The
     *    object must be parsed using helper libraries on sender and receiver.
     */
    void getValues(IVehicleCallback callback, in GetValueRequests requests);

    /**
     * Set vehicle property values.
     *
     * The {@link IVehicleCallback#onSetValues} function would be called after
     * the values set request are sent through vehicle bus or failed to set.
     * If the bus protocol supports confirmation, the callback would be called
     * after getting the confirmation.
     *
     * For some vehicle bus such as CAN bus where confirmation is not supported,
     * OnSetValues does not necessarily mean the value changes would be
     * reflected in {@link #getValues} immediately.
     *
     * If the output status contains error, it means we fail to set all the
     * properties. If we failed to set some of the values, they would be
     * reflected as non OK {@link SetValueResult#status}.
     *
     * The order each property in the request is set is not guaranteed. If
     * caller needs to make sure certain order in setting values, caller should
     * set one value, wait for its callback and then set the other value.
     *
     * Timestamp of data must be ignored for set operation.
     *
     * Setting some properties requires having initial state available. If
     * initial data is not available yet, the {@link SetValueResult#status}
     * must be {@link StatusCode#TRY_AGAIN}. For a property with separate power
     * control the {@link SetValueResult#status} must be
     * {@link StatusCode#NOT_AVAILABLE} if property is not powered
     * on.
     *
     * Caller must pass a unique RequestID for each request, if any of the
     * given request ID is duplicate with one of the pending request ID, this
     * function must return {@link StatusCode#INVALID_ARG}.
     *
     * To prevent confusion, duplicate properties (same property ID and same
     * area ID) are not allowed in a single call. This function must return
     * {@link StatusCode#INVALID_ARG} for duplicate properties.
     *
     * @param callback The callback, whose 'onSetValues' would be called after
     *    set value request is sent to bus.
     * @param requests An object that contains a list of {@link SetValueRequest}
     *    or a shared memory file that stores the list of requests if they
     *    exceed binder memory limitation, must be parsed using helper libraries
     *    on sender and receiver.
     */
    void setValues(IVehicleCallback callback, in SetValueRequests requests);

    /**
     * Subscribes to property events.
     *
     * Clients must be able to subscribe to multiple properties at a time
     * depending on data provided in options argument.
     *
     * For one callback, there is only one subscription for one property.
     * A new subscription with a different sample rate would override the old
     * subscription. One property could be subscribed multiple times for
     * different callbacks.
     *
     * If error is returned, some of the properties failed to subscribe.
     * Caller is safe to try again, since subscribing to an already subscribed
     * property is okay.
     *
     * The specified sample rate is just a guidance. It is not guaranteed that
     * the sample rate is achievable depending on how the polling refresh rate
     * is. The actual property event rate might be higher/lower than the
     * specified sampleRate, for example, if the polling rate can be 5 times/s
     * or 10 times/s, subscribing to a sample rate of 7 might use the 5 times/s
     * polling rate, thus generating 5 events/s. We only require that on
     * average, the {@code minSampleRate} and {@code maxSampleRate} can be
     * achieved, all the sampleRate within min and max would on average
     * generates events with rate >= {@code minSampleRate} and <=
     * {@code maxSampleRate}.
     *
     * The {@link VehiclePropValue#timestamp} field for each property event must
     * be the system uptime since boot when the value changes for
     * ON_CHANGE property or when the value is checked according to polling rate
     * for CONTINUOUS property. Note that for CONTINUOUS property, VHAL client
     * reading the property multiple times between the polling interval will get
     * the same timestamp.
     * For example, if the polling rate for a property is 10 times/s, no matter
     * what the sampleRate specified in {@code options}, the timestamp for
     * the timestamp is updated 10 times/s.
     *
     * If a property is unavailable for reading because it depends on some power
     * state which is off, property change event may not be generated until the
     * property becomes available. For ON_CHANGE property, if the property
     * changes from NOT_AVAILABLE to OKAY for reading some or all area(s), for
     * each area that becomes available for reading, one property change event
     * must be generated. The event must contain the current value for the area
     * and must have {@code AVAILABLE} status.
     *
     * @param callback The subscription callbacks.
     *    {@link IVehicleCallback#onPropertyEvent} would be called when a new
     *    property event arrives.
     *    {@link IVehicleCallback#onPropertySetError} would be called when a
     *    property set request failed asynchronously. This is usually caused by
     *    a property set failure message sent from the vehicle bus.
     * @param options List of options to subscribe. SubscribeOption contains
     *    information such as property Id, area Id, sample rate, etc.
     *    For continuous properties, sample rate must be provided. If sample
     *    rate is less than {@link VehiclePropConfig#minSampleRate}, the sample
     *    rate would be minSampleRate. If sample rate is larger than
     *    {@link VehiclePropValue#maxSampleRate}, the sample rate would be
     *    maxSampleRate.
     * @param maxSharedMemoryFileCount The maximum number of shared memory files
     *    allocated for in VHAL for this subscription. When a memory file is
     *    handled back to the client, it cannot be used by VHAL to deliver
     *    another event until the buffer is returned to VHAL by calling
     *    returnSharedMemory. A larger maxSharedMemoryFileCount means a better
     *    performance while handling large bursts of data, but also means larger
     *    memory footprint. If you don't expect events arriving very frequently,
     *    a recommended value is 2. A value of 0 means for each new property,
     *    a new shared memory file would be created and no shared memory file
     *    would ever be reused. This should only be configured for infrequent
     *    events or devices with limited memory. This value must be >=0 and
     *    < {@link MAX_SHARED_MEMORY_FILES_PER_CLIENT}.
     */
    void subscribe(in IVehicleCallback callback, in SubscribeOptions[] options,
            int maxSharedMemoryFileCount);

    /**
     * Unsubscribes from property events.
     *
     * If 'callback' is not valid this method must return
     * {@link StatusCode#INVALID_ARG}. If a specified propId was not subscribed
     * before, this method must ignore that propId.
     *
     * If error is returned, some of the properties failed to unsubscribe.
     * Caller is safe to try again, since unsubscribing an already unsubscribed
     * property is okay.
     *
     * @param callback The callback used in the previous subscription.
     * @param propIds The IDs for the properties to unsubscribe.
     */
    void unsubscribe(in IVehicleCallback callback, in int[] propIds);

    /**
     * Return a shared memory file back to VHAL for recycle.
     *
     * This must be called after a shared memory file returned by
     * {@link IVehicleCallback#onPropertyEvent} is no longer in-use by the
     * client. This is usually called at the end of 'onPropertyEvent'.
     *
     * If the 'callback' is not valid or 'sharedMemoryId' does not match any
     * SharedMemoryId in 'VehiclePropValues' passed to
     * {@link IVehicleCallback#onPropertyEvent}, this method must return
     * {@link StatusCode#INVALID_ARG}.
     *
     * @param callback The callback used in subscription.
     * @param sharedMemoryId The ID returned by 'onPropertyEvent' representing
     *    the used shared memory file to return.
     */
    void returnSharedMemory(in IVehicleCallback callback, long sharedMemoryId);
}
