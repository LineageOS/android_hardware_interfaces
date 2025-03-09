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

import android.hardware.contexthub.EndpointId;
import android.hardware.contexthub.Service;

/* This structure is a unified superset of NanoAppInfo and HostEndpointInfo. */
@VintfStability
parcelable EndpointInfo {
    /** Unique identifier of this endpoint. */
    EndpointId id;

    /** Type of this endpoint. */
    EndpointType type;

    /**
     * Name of this endpoint. Endpoint may use this field to identify the initiator of the session
     * request.
     *
     * Depending on type of the endpoint, the following values are used:
     *  - Framework: package name of the process registering this endpoint
     *  - App: package name of the process registering this endpoint
     *  - Native: name of the process registering this endpoint, supplied by client for debugging
     *            purpose.
     *  - Nanoapp: name of the nanoapp, for debugging purpose
     *  - Generic: name of the generic endpoint, for debugging purpose
     */
    String name;

    /**
     * Monotonically increasing version number. The two sides of an endpoint session can use this
     * version number to identify the other side and determine compatibility with each other.
     * The interpretation of the version number is specific to the implementation of an endpoint.
     * The version number should not be used to compare endpoints implementation freshness for
     * different endpoint types.
     *
     * Depending on type of the endpoint, the following values are used:
     *  - Framework: android.os.Build.VERSION.SDK_INT_FULL (populated by ContextHubService)
     *  - App: versionCode (populated by ContextHubService)
     *  - Native: unspecified format (supplied by endpoint code)
     *  - Nanoapp: nanoapp version, typically following 0xMMmmpppp scheme where
     *             MM = major version, mm = minor version, pppp = patch version
     *  - Generic: unspecified format (supplied by endpoint code), following nanoapp versioning
     *             scheme is recommended
     */
    int version;

    /**
     * Tag for this particular endpoint. Optional string that further identifies the submodule
     * that created this endpoint.
     */
    @nullable String tag;

    /**
     * Represents the minimally required permissions in order to message this endpoint. Further
     * permissions may be required on a message-by-message basis.
     */
    String[] requiredPermissions;

    /**
     * List of services provided by this endpoint. Service list should be fixed for the
     * lifetime of an endpoint.
     */
    Service[] services;

    @VintfStability
    @Backing(type="int")
    enum EndpointType {
        /**
         * This endpoint is from the Android framework
         */
        FRAMEWORK = 1,

        /** This endpoint is an Android app. */
        APP = 2,

        /** This endpoint is from an Android native program. */
        NATIVE = 3,

        /** This endpoint is from a nanoapp. */
        NANOAPP = 4,

        /** This endpoint is a generic endpoint (not from a nanoapp). */
        GENERIC = 5,
    }
}
