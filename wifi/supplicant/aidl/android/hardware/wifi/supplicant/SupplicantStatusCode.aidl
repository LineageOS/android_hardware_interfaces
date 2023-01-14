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

package android.hardware.wifi.supplicant;

/**
 * Enum values indicating the status of any supplicant operation.
 */
@VintfStability
@Backing(type="int")
enum SupplicantStatusCode {
    /**
     * No errors.
     */
    SUCCESS,
    /**
     * Unknown failure occurred.
     */
    FAILURE_UNKNOWN,
    /**
     * One of the incoming args is invalid.
     */
    FAILURE_ARGS_INVALID,
    /**
     * |ISupplicantIface| AIDL interface object is no longer valid.
     */
    FAILURE_IFACE_INVALID,
    /**
     * Iface with the provided name does not exist.
     */
    FAILURE_IFACE_UNKNOWN,
    /**
     * Iface with the provided name already exists.
     */
    FAILURE_IFACE_EXISTS,
    /**
     * Iface is disabled and cannot be used.
     */
    FAILURE_IFACE_DISABLED,
    /**
     * Iface is not currently disconnected, so cannot reconnect.
     */
    FAILURE_IFACE_NOT_DISCONNECTED,
    /**
     * |ISupplicantNetwork| AIDL interface object is no longer valid.
     */
    FAILURE_NETWORK_INVALID,
    /**
     * Network with the provided id does not exist.
     */
    FAILURE_NETWORK_UNKNOWN,
    FAILURE_UNSUPPORTED,
    /**
     * A different request is currently being processed.
     */
    FAILURE_ONGOING_REQUEST,
}
