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

@VintfStability
parcelable Service {
    /**
     * Type of the Service. This field defines the messaging format used for this service.
     * The format refers to how the data would be marhsalled in messages between host endpoint (on
     * Android) and endpoint on the Context Hub or generic hub.
     */
    RpcFormat format;

    /**
     * Uniquely identifies the interface (scoped to type). Conventions depend on interface type.
     * Examples:
     *   1. AOSP-defined AIDL: android.hardware.something.IFoo/default
     *   2. Vendor-defined AIDL: com.example.something.IBar/default
     *   3. Pigweed RPC with Protobuf: com.example.proto.ExampleService
     */
    String serviceDescriptor;

    /** Breaking changes should be a major version bump. */
    int majorVersion;
    /** Monotonically increasing minor version. */
    int minorVersion;

    /** Hook for additional detail in vendor-specific type */
    ParcelableHolder extendedInfo;

    /**
     * Supported messaging format for the service between the host and the hubs.
     */
    @VintfStability
    @Backing(type="int")
    enum RpcFormat {
        /**
         * Customized format for messaging. Fully customized and opaque messaging format.
         */
        CUSTOM = 0,
        /**
         * Binder-based messaging. The host endpoint is defining this service in Stable AIDL.
         * Messages between endpoints that uses this service will be using the binder marhsalling
         * format.
         */
        AIDL = 1,
        /**
         * Pigweed RPC messaging with Protobuf. This endpoint is a Pigweed RPC. Messages between
         * endpoints will use Pigweed RPC marshalling format (protobuf).
         */
        PW_RPC_PROTOBUF = 2,
    }
}
