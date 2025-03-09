/*
 * Copyright (C) 2023 The Android Open Source Project
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

@VintfStability
@Backing(type="byte")
enum Reason {
    /**
     * Unspecified reason.
     */
    UNSPECIFIED = 0,

    /**
     * Out of memory. There's not enough memory to perform this operation.
     */
    OUT_OF_MEMORY,

    /**
     * Timeout. This operation timed out.
     */
    TIMEOUT,

    /**
     * Endpoint rejected this openEndpointSession request.
     */
    OPEN_ENDPOINT_SESSION_REQUEST_REJECTED,

    /**
     * Endpoint requested closeEndpointSession.
     */
    CLOSE_ENDPOINT_SESSION_REQUESTED,

    /**
     * Invalid endpoint.
     */
    ENDPOINT_INVALID,

    /**
     * Endpoint is now stopped.
     */
    ENDPOINT_GONE,

    /**
     * Endpoint crashed.
     */
    ENDPOINT_CRASHED,

    /**
     * Hub was reset or is resetting.
     */
    HUB_RESET,
}
