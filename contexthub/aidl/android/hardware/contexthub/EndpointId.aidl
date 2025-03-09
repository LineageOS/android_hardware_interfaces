/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.contexthub;

/* This structure is a unique identifier for an endpoint */
@VintfStability
parcelable EndpointId {
    /**
     * Invalid endpoint ID.
     */
    const long ENDPOINT_ID_INVALID = 0;

    /**
     * Reserved endpoint ID.
     */
    const long ENDPOINT_ID_RESERVED = -1;

    /**
     * Nanoapp ID or randomly generated ID (depending on type). This value uniquely identifies the
     * endpoint within a single hub.
     *
     * ENDPOINT_ID_INVALID(0) is an invalid id and should never be used.
     * ENDPOINT_ID_RESERVED(-1) is reserved for future use.
     * For static/compile-time-generated IDs, topmost bit should be 0.
     * For dynamic/runtime-generated IDs, topmost bit should be 1.
     */
    long id;

    /**
     * Hub ID of the hub hosting this endpoint. A pair of (hubId, id) uniquely identifies the
     * endpoint globally.
     */
    long hubId;
}
