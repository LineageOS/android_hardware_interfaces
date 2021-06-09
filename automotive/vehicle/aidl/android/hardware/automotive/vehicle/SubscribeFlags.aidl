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

// @VintfStability
@Backing(type="int")
enum SubscribeFlags {
    UNDEFINED = 0x0,

    /**
     * Subscribe to event that was originated in vehicle HAL
     * (most likely this event came from the vehicle itself).
     */
    EVENTS_FROM_CAR = 0x1,

    /**
     * Use this flag to subscribe on events when IVehicle#set(...) was called by
     * vehicle HAL's client (e.g. Car Service).
     */
    EVENTS_FROM_ANDROID = 0x2,

    /**
     * Property event for this property should be passed through shared memory with only this
     * property's data included. This can be helpful for reducing memory copy in upper layer
     * for data with bigger payload. If payload size is small, VHAL can send this through non-shared
     * memory path instead.
     */
    EXCLUSIVE_SHARED_MEMORY = 0x4,
}
