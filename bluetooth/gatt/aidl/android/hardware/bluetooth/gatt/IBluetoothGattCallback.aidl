/*
 * Copyright (C) 2025 The Android Open Source Project
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

package android.hardware.bluetooth.gatt;

/**
 * The interface from the Bluetooth offload stack to the host stack.
 */
@VintfStability
interface IBluetoothGattCallback {
    /** Reported error type in errorReport. */
    @VintfStability
    @Backing(type="int")
    enum Error {
        /**
         * Default value. This value means Error wasn't explicitly initialized and must be discarded
         * by the host stack.
         */
        UNKNOWN,

        /**
         * Indicates that an ATT Error Response PDU was received with the error code 0x12
         * (Database Out Of Sync). The host stack is responsible for reading the database hash and
         * initiating the service discovery procedure to update the database. Host applications are
         * responsible for registering the services again after the GATT service discovery procedure
         * is complete.
         */
        DATABASE_OUT_OF_SYNC,

        /**
         * Indicates that the remote device failed to respond within the expected time.
         * The host stack is required to disconnect the underlying ACL link or EATT
         * channel for this connection.
         */
        RESPONSE_TIMEOUT,

        /**
         * Indicates a protocol violation occurred. The host stack is required to
         * disconnect the underlying ACL link or logical channel for this session.
         */
        PROTOCOL_VIOLATION,
    }

    /**
     * Represents the status of a GATT offload operation, indicating success or
     * specific local failure reasons.
     */
    @VintfStability
    @Backing(type="int")
    enum Status {
        /**
         * The operation completed successfully.
         */
        SUCCESS = 0,

        /**
         * The provided endpoint ID is invalid or unknown to the system.
         * This could mean the endpoint does not exist or is not registered.
         */
        INVALID_ENDPOINT_ID,

        /**
         * The requested GATT role (e.g., client or server) is not supported
         * by the specified endpoint for this operation.
         */
        UNSUPPORTED_ROLE,

        /**
         * The system or the endpoint lacks sufficient resources (e.g., memory,
         * processing power, available connections) to fulfill the request.
         */
        INSUFFICIENT_RESOURCES,

        /**
         * A general failure occurred that does not fit into other specific error
         * categories. This typically indicates an internal error on the host
         * or endpoint side that prevented the operation from completing.
         */
        FAILURE,
    }

    /**
     * Invoked when IBluetoothGatt.registerService() has been completed.
     *
     * @param sessionId The unique identifier for the GATT session that was previously
     *        assigned when the service was offloaded
     * @param status Status indicating success or failure
     * @param reason Reason string of the operation failure for debugging purposes
     */
    void registerServiceComplete(int sessionId, in Status status, in String reason);

    /**
     * Invoked to report completion of unregisterService request, or to notify the host
     * stack that the session was closed by the offload application.
     *
     * @param sessionId The unique identifier for the GATT session that was previously
     *        assigned when the service was offloaded
     * @param reason Reason string of the operation for debugging purposes
     */
    void unregisterServiceComplete(int sessionId, in String reason);

    /**
     * Report that the offloaded services for the selected ACL connection have been cleared,
     * and that all pending ATT procedures for these services have completed.
     *
     * @param aclConnectionHandle The handle of the ACL connection on which the services are
     *        cleared
     * @param reason Reason string of the operation for debugging purposes
     */
    void clearServicesComplete(int aclConnectionHandle, in String reason);

    /**
     * Invoked when offload stack notifies host stack that a error has occurred on
     * the GATT connection. Host stack is responsible for handling the error
     * appropriately based on the type of error. See the {@link Error} enum.
     *
     * @param aclConnectionHandle The handle of the ACL connection on which the error
     *        occurred
     * @param localCid The local Channel ID (CID) associated with the L2CAP channel
     * over which the GATT error was reported
     * @param error The reported error. See the {@link Error} enum
     * @param reason Reason string of the error for debugging purposes
     */
    void errorReport(in int aclConnectionHandle, in int localCid, in Error error, in String reason);
}
